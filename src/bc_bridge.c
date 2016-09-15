
#include "bc_bridge.h"
#include "bc_log.h"
#include <linux/hidraw.h>
#include <linux/input.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <libudev.h>
#include <fcntl.h>

#define BC_BRIDGE_DEBUG 1

#define REPORT_ID_CHIP_CODE 0xA0
#define REPORT_ID_SYSTEM_SETTING 0xA1
#define REPORT_ID_GPIO 0xB0
#define REPORT_ID_I2C_STATUS 0xC0
#define REPORT_ID_UART_STATUS 0xE0

//
//BOOT  GPIO5 DIO11   pin 18
//RESET GPIO4 DIO10   pin 17

typedef enum
{
    BC_BRIDGE_I2C_CLOCK_SPEED_100KHZ = 0,
    BC_BRIDGE_I2C_CLOCK_SPEED_400KHZ = 1

} bc_bridge_i2c_clock_speed_t;

static bool _bc_bridge_i2c_set_channel(bc_bridge_t *self, bc_bridge_i2c_channel_t i2c_channel);
static bool _bc_bridge_ft260_i2c_set_clock_speed(int fd_hid, bc_bridge_i2c_clock_speed_t clock_speed);
static bool _bc_bridge_ft260_get_i2c_bus_status(int fd_hid, uint8_t *bus_status);
static bool _bc_bridge_ft260_i2c_write(int fd_hid, uint8_t address, uint8_t *buffer, uint8_t length, uint8_t flag);
static bool _bc_bridge_ft260_i2c_read(int fd_hid, uint8_t address, uint8_t *buffer, uint8_t length);
static bool _bc_bridge_ft260_i2c_reset(int fd_hid);
static void _bc_bridge_ft260_check_fd(int fd_hid);
static bool _bc_bridge_ft260_get_feature(int fd_hid, uint8_t *buffer, size_t length);
static bool _bc_bridge_ft260_set_feature(int fd_hid, uint8_t *buffer, size_t length);

bool bc_bridge_scan(bc_bridge_device_info_t *devices, uint8_t *device_count)
{
    struct udev *udev;
    struct udev_enumerate *enumerate, *enumerate_hid;
    struct udev_list_entry *devices_usb, *entry, *devices_hid, *entry_hid;
    struct udev_device *usb_dev;
    struct udev_device *hid = NULL;

    *device_count = 0;

    /* Create the udev object */
    udev = udev_new();

    if (udev == NULL)
    {
        bc_log_error("bc_bridge_scan: call failed: udev_new");

        return false;
    }

    //enumerate over usb device
    enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "usb");
    udev_enumerate_add_match_property(enumerate, "DEVTYPE", "usb_device");
    udev_enumerate_scan_devices(enumerate);
    devices_usb = udev_enumerate_get_list_entry(enumerate);

    udev_list_entry_foreach(entry, devices_usb)
    {
        const char *str_id_vendor;
        const char *str_id_product;

        uint16_t id_vendor;
        uint16_t id_product;

        const char* path = udev_list_entry_get_name(entry);
        const char* path_i2c=NULL;
        const char* path_uart=NULL;

        usb_dev = udev_device_new_from_syspath(udev, path);

        str_id_vendor = udev_device_get_sysattr_value(usb_dev, "idVendor");
        str_id_product = udev_device_get_sysattr_value(usb_dev, "idProduct");

        id_vendor = (uint16_t) ((str_id_vendor) ? strtol(str_id_vendor, NULL, 16) : -1);
        id_product = (uint16_t) ((str_id_product) ? strtol(str_id_product, NULL, 16) : -1);

        if (id_vendor == 0x0403 && id_product == 0x6030)
        {
            enumerate_hid = udev_enumerate_new(udev);
            udev_enumerate_add_match_parent(enumerate_hid, usb_dev);
            udev_enumerate_add_match_subsystem(enumerate_hid, "hidraw");
            udev_enumerate_scan_devices(enumerate_hid);
            devices_hid = udev_enumerate_get_list_entry(enumerate_hid);

            udev_list_entry_foreach(entry_hid, devices_hid)
            {
                const char *entry_hid_path = udev_list_entry_get_name(entry_hid);
                hid = udev_device_new_from_syspath(udev, entry_hid_path);

                if (path_i2c==NULL)
                {
                    path_i2c = udev_device_get_devnode(hid);
                }
                else if (path_uart==NULL)
                {
                    path_uart = udev_device_get_devnode(hid);
                }
                else
                {
                    perror("Unexpected hid device");
                    return false;
                }
            }

            if ((path_i2c != NULL) && (path_uart != NULL))
            {

                devices[*device_count].usb_path = (char *) udev_device_get_devnode(usb_dev);
                devices[*device_count].path_i2c = (char *) path_i2c;
                devices[*device_count].path_uart = (char *) path_uart;

                bc_log_debug("bc_bridge_scan: found device %s, %s", path_i2c, path_uart);

                *device_count += 1;
            }
        }
    }

    return true;
}

bool bc_bridge_open(bc_bridge_t *self, bc_bridge_device_info_t *info)
{
    bc_log_info("bc_bridge_open");

    memset(self, 0, sizeof(*self));

    bc_os_mutex_init(&self->_mutex);

    self->_fd_i2c = open(info->path_i2c, O_RDWR);

    if (self->_fd_i2c == -1)
    {
        bc_log_error("bc_bridge_open: call failed: open");

        return false;
    }

    if (flock(self->_fd_i2c, LOCK_EX | LOCK_NB) == -1)
    {
        bc_log_error("bc_bridge_open: call failed: flock");

        return false;
    }

    if (!_bc_bridge_ft260_i2c_set_clock_speed(self->_fd_i2c, BC_BRIDGE_I2C_CLOCK_SPEED_100KHZ))
    {
        bc_log_error("bc_bridge_open: call failed: _bc_bridge_ft260_i2c_set_clock_speed");

        return false;
    }

    if (!_bc_bridge_i2c_set_channel(self, BC_BRIDGE_I2C_CHANNEL_1))
    {
        bc_log_error("bc_bridge_open: call failed: _bc_bridge_i2c_set_channel");

        return false;
    }

    return true;
}

bool bc_bridge_close(bc_bridge_t *self)
{
    bc_log_info("bc_bridge_close");

    bc_os_mutex_lock(&self->_mutex);

    if (flock(self->_fd_i2c, LOCK_UN) == -1)
    {
        bc_log_error("bc_bridge_close: call failed: flock");

        bc_os_mutex_unlock(&self->_mutex);

        return false;
    }

    if (close(self->_fd_i2c) == -1)
    {
        bc_log_error("bc_bridge_close: call failed: close");

        bc_os_mutex_unlock(&self->_mutex);

        return false;
    }

    bc_os_mutex_unlock(&self->_mutex);

    bc_os_mutex_destroy(&self->_mutex);

    return true;
}

bool bc_bridge_is_alive(bc_bridge_t *self)
{
    struct stat s;

    if (fstat(self->_fd_i2c, &s) == -1)
    {
        bc_log_fatal("bc_bridge_is_alive: call failed: fstat");

        return false;
    }

    if (s.st_nlink < 1)
    {
        return false;
    }

    return true;
}

bool bc_bridge_i2c_reset(bc_bridge_t *self)
{
    bc_os_mutex_lock(&self->_mutex);

    bc_log_info("bc_bridge_i2c_reset");

    if (!_bc_bridge_ft260_i2c_reset(self->_fd_i2c))
    {
        bc_log_error("bc_bridge_i2c_reset: call failed: _bc_bridge_ft260_i2c_reset");

        bc_os_mutex_unlock(&self->_mutex);

        return false;
    }

    bc_os_mutex_unlock(&self->_mutex);

    return true;
}

bool bc_bridge_i2c_write(bc_bridge_t *self, bc_bridge_i2c_transfer_t *transfer)
{
    uint8_t buffer[60];

    bc_os_mutex_lock(&self->_mutex);

    if (transfer->address_16_bit)
    {
        if (transfer->length > 58)
        {
            bc_log_error("bc_bridge_i2c_write: length is too big");

            bc_os_mutex_unlock(&self->_mutex);

            return false;
        }

        buffer[0] = (uint8_t) (transfer->address >> 8);
        buffer[1] = (uint8_t) transfer->address;

        memcpy(&buffer[2], transfer->buffer, transfer->length);
    }
    else
    {
        if (transfer->length > 59)
        {
            bc_log_error("bc_bridge_i2c_write: length is too big");

            bc_os_mutex_unlock(&self->_mutex);

            return false;
        }

        buffer[0] = (uint8_t) transfer->address;

        memcpy(&buffer[1], transfer->buffer, transfer->length);
    }

#if BC_BRIDGE_DEBUG == 1
    if (transfer->address_16_bit)
    {
        bc_log_dump(transfer->buffer, transfer->length, "bc_bridge_i2c_write: channel 0x%02X, device 0x%02X, address 0x%04X, length %d bytes",
                    transfer->channel, transfer->device_address, transfer->address, transfer->length);
    }
    else
    {
        bc_log_dump(transfer->buffer, transfer->length, "bc_bridge_i2c_write: channel 0x%02X, address 0x%02X, length %d bytes",
                    transfer->channel, transfer->device_address, transfer->address, transfer->length);
    }
#endif

    if (_bc_bridge_i2c_set_channel(self, transfer->channel))
    {
        if (!_bc_bridge_ft260_i2c_write(self->_fd_i2c, transfer->device_address, buffer,
                                            (transfer->address_16_bit ? 2 : 1) + transfer->length, 0x06))
        {
            bc_log_warning("bc_bridge_i2c_write: call failed: _bc_bridge_ft260_i2c_write");

            bc_os_mutex_unlock(&self->_mutex);

            return false;
        }
    }

    bc_os_mutex_unlock(&self->_mutex);

    return true;
}

bool bc_bridge_i2c_read(bc_bridge_t *self, bc_bridge_i2c_transfer_t *transfer)
{
    uint8_t buffer[2];

    bc_os_mutex_lock(&self->_mutex);

    if (transfer->length > 60)
    {
        bc_log_error("bc_bridge_i2c_read: length is too big");

        bc_os_mutex_unlock(&self->_mutex);

        return false;
    }

#if BC_BRIDGE_DEBUG == 1
    if (transfer->address_16_bit)
    {
        bc_log_dump(NULL, 0, "bc_bridge_i2c_read: channel 0x%02X, device 0x%02X, address 0x%04X, length %d bytes",
                    transfer->channel, transfer->device_address, transfer->address, transfer->length);
    }
    else
    {
        bc_log_dump(NULL, 0, "bc_bridge_i2c_read: channel 0x%02X, device 0x%02X, address 0x%02X, length %d bytes",
                    transfer->channel, transfer->device_address, transfer->address, transfer->length);
    }
#endif

    if (transfer->address_16_bit)
    {
        buffer[0] = (uint8_t) (transfer->address >> 8);
        buffer[1] = (uint8_t) transfer->address;
    }
    else
    {
        buffer[0] = (uint8_t) transfer->address;
    }

    if (!_bc_bridge_i2c_set_channel(self, transfer->channel))
    {
        bc_log_error("bc_bridge_i2c_read: call failed: _bc_bridge_i2c_set_channel");

        bc_os_mutex_unlock(&self->_mutex);

        return false;
    }

    if (!_bc_bridge_ft260_i2c_write(self->_fd_i2c, transfer->device_address, buffer,
                                    transfer->address_16_bit ? 2 : 1, 0x02))
    {
        bc_log_warning("bc_bridge_i2c_read: call failed: _bc_bridge_ft260_i2c_write");

        bc_os_mutex_unlock(&self->_mutex);

        return false;
    }

    if (!_bc_bridge_ft260_i2c_read(self->_fd_i2c, transfer->device_address, transfer->buffer,
                                   transfer->length))
    {
        bc_log_warning("bc_bridge_i2c_read: call failed: _bc_bridge_ft260_i2c_read");

        bc_os_mutex_unlock(&self->_mutex);

        return false;
    }

#if BC_BRIDGE_DEBUG == 1
    bc_log_dump(transfer->buffer, transfer->length, "bc_bridge_i2c_read: read %d bytes", transfer->length);
#endif

    bc_os_mutex_unlock(&self->_mutex);

    return true;
}

bool bc_bridge_i2c_ping(bc_bridge_t *self, bc_bridge_i2c_channel_t channel, uint8_t device_address)
{
    uint8_t report[64];
    uint8_t bus_status;
    bc_tick_t tick_timeout;

    int res;

    _bc_bridge_i2c_set_channel(self, channel);

    //bc_log_debug("bc_bridge_i2c_ping: device 0x%02X", device_address);

    report[0] = (uint8_t) 0xD0; /* I2C write */
    report[1] = device_address; /* Slave address */
    report[2] = 0x06; /* Start and Stop */
    report[3] = 1;
    report[4] = 0x00;

    tick_timeout = bc_tick_get() + 100;

//    //wait on i2c redy
//    do
//    {
//        if (bc_tick_get() >= tick_timeout)
//        {
//            return false;
//        }
//
//        if (!_bc_bridge_ft260_get_i2c_bus_status(self->_fd_i2c, &bus_status))
//        {
//            return false;
//        }
//
//    } while ((bus_status & 0x11) != 0);
//    //bit 0 = controller busy: all other status bits invalid
//    //bit 5 = controller idle

    res = write(self->_fd_i2c, report, 5);

    if (res == -1)
    {
        return false;
    }

    if (res != 5)
    {
        return false;
    }

    do
    {
        if (bc_tick_get() >= tick_timeout)
        {
            return false;
        }
        if (!_bc_bridge_ft260_get_i2c_bus_status(self->_fd_i2c, &bus_status))
        {
            return false;
        }

    } while ((bus_status & 0x01) != 0);

    if ((bus_status & 0x1E) != 0)
    {
        return false;
    }

    return true;
}


bool bc_bridge_led_set_state(bc_bridge_t *self, bc_bridge_led_state_t state)
{
    uint8_t buffer[5];

    bc_os_mutex_lock(&self->_mutex);

    buffer[0] = REPORT_ID_GPIO;

    if (!_bc_bridge_ft260_get_feature(self->_fd_i2c, buffer, sizeof(buffer)))
    {
        bc_os_mutex_unlock(&self->_mutex);

        return false;
    }

    if (state == BC_BRIDGE_LED_STATE_ON)
    {
        buffer[3] |= 0x80;
    }
    else
    {
        buffer[3] &= 0x7F;
    }

    /* Set output direction */
    buffer[4] |= 0x80;

    if (!_bc_bridge_ft260_set_feature(self->_fd_i2c, buffer, sizeof(buffer)))
    {
        bc_os_mutex_unlock(&self->_mutex);

        return false;
    }

    bc_os_mutex_unlock(&self->_mutex);

    return true;
}

bool bc_bridge_led_get_state(bc_bridge_t *self, bc_bridge_led_state_t *state)
{
    uint8_t buffer[5];

    bc_os_mutex_lock(&self->_mutex);

    buffer[0] = REPORT_ID_GPIO;

    if (!_bc_bridge_ft260_get_feature(self->_fd_i2c, buffer, sizeof(buffer)))
    {
        bc_os_mutex_unlock(&self->_mutex);

        return false;
    }

    *state = (buffer[3] & 0x80) != 0 ? BC_BRIDGE_LED_STATE_ON : BC_BRIDGE_LED_STATE_OFF;

    bc_os_mutex_unlock(&self->_mutex);

    return true;
}

static bool _bc_bridge_i2c_set_channel(bc_bridge_t *self, bc_bridge_i2c_channel_t i2c_channel)
{
    uint8_t buffer[1];

    int i;

    bc_log_debug("_bc_bridge_i2c_set_channel: switching to channel: %d", (uint8_t) i2c_channel);

    if (self->_i2c_channel == i2c_channel)
    {
        return true;
    }

    switch (i2c_channel)
    {
        case BC_BRIDGE_I2C_CHANNEL_0:
        {
            buffer[0] = 1;
            break;
        }
        case BC_BRIDGE_I2C_CHANNEL_1:
        {
            buffer[0] = 2;
            break;
        }
        default:
        {
            return false;
        }
    }

    for (i = 0; i < 3; i++)
    {
        if (_bc_bridge_ft260_i2c_write(self->_fd_i2c, 0x70, buffer, sizeof(buffer), 0x06))
        {
            self->_i2c_channel = i2c_channel;

            return true;
        }

        bc_os_task_sleep(10);
    }

    bc_log_error("_bc_bridge_i2c_set_channel: call failed: _bc_bridge_ft260_i2c_write");

    return false;
}

static bool _bc_bridge_ft260_i2c_reset(int fd_hid)
{
    uint8_t report[2];
    uint8_t bus_status;
    bc_tick_t tick_start;

    report[0] = REPORT_ID_SYSTEM_SETTING;
    report[1] = 0x20;

    if (!_bc_bridge_ft260_set_feature(fd_hid, report, sizeof(report)))
    {
        bc_log_error("_bc_bridge_ft260_i2c_reset: call failed: _bc_bridge_ft260_set_feature");

        return false;
    }

    tick_start = bc_tick_get();
    //kontroluje se stav i2c, pokud do 1s nedojde k resetu vraci false
    // TODO Jenze tohle je cpu intensive - melo by se mezi tim spat
    do
    {
        if (!_bc_bridge_ft260_get_i2c_bus_status(fd_hid, &bus_status))
        {
            bc_log_error("_bc_bridge_ft260_i2c_reset: call failed: _bc_bridge_ft260_get_i2c_bus_status");

            return false;
        }

        if (bus_status == 0x20)
        {
            return true;
        }

    } while ((bc_tick_get() - tick_start) < 1000);

    return false;
}

static bool _bc_bridge_ft260_i2c_set_clock_speed(int fd_hid, bc_bridge_i2c_clock_speed_t clock_speed)
{
    uint8_t report[4];

    switch (clock_speed)
    {
        case BC_BRIDGE_I2C_CLOCK_SPEED_100KHZ:
        {
            bc_log_debug("_bc_bridge_ft260_i2c_set_clock_speed: requested 100kHz");

            report[0] = REPORT_ID_SYSTEM_SETTING;
            report[1] = 0x22;
            report[2] = 0x64;
            report[3] = 0x00;

            break;
        }
        case BC_BRIDGE_I2C_CLOCK_SPEED_400KHZ:
        {
            bc_log_debug("_bc_bridge_ft260_i2c_set_clock_speed: requested 400kHz");

            report[0] = REPORT_ID_SYSTEM_SETTING;
            report[1] = 0x22;
            report[2] = 0x90;
            report[3] = 0x01;

            break;
        }
        default:
        {
            bc_log_error("_bc_bridge_ft260_i2c_set_clock_speed: speed is not supported");

            return false;
        }
    }

    if (!_bc_bridge_ft260_set_feature(fd_hid, report, sizeof(report)))
    {
        bc_log_error("_bc_bridge_ft260_i2c_set_clock_speed: call failed: _bc_bridge_ft260_set_feature");

        return false;
    }

    return true;
}

static bool _bc_bridge_ft260_get_i2c_bus_status(int fd_hid, uint8_t *bus_status)
{
    uint8_t report[5];

    report[0] = 0xC0;

    if (!_bc_bridge_ft260_get_feature(fd_hid, report, sizeof(report)))
    {
        bc_log_error("_bc_bridge_ft260_get_i2c_bus_status: call failed: _bc_bridge_ft260_get_feature");

        return false;
    }

    *bus_status = report[1];

    return true;
}

static bool _bc_bridge_ft260_i2c_write(int fd_hid, uint8_t address, uint8_t *buffer, uint8_t length, uint8_t flag)
{
    uint8_t report[64];
    uint8_t bus_status;
    bc_tick_t tick_timeout;

    int res;

    if (length > 60)
    {
        bc_log_error("_bc_bridge_ft260_i2c_write: length is too big");

        return false;
    }

    memcpy(&report[4], buffer, length);

    report[0] = (uint8_t) (0xD0 + ((length - 1) / 4)); /* I2C write */
    report[1] = address; /* Slave address */
    report[2] = flag;
    report[3] = length;

    tick_timeout = bc_tick_get() + 100;
//    //wait on i2c redy
//    do
//    {
//        if (bc_tick_get() >= tick_timeout)
//        {
//            bc_log_error("_bc_bridge_ft260_i2c_write: i2c ready timeout");
//
//            return false;
//        }
//
//        if (!_bc_bridge_ft260_get_i2c_bus_status(fd_hid, &bus_status))
//        {
//            bc_log_error("_bc_bridge_ft260_i2c_write: call failed: _bc_bridge_ft260_get_i2c_bus_status");
//
//            return false;
//        }
//
//    } while ((bus_status & 0x11) != 0);
//    //bit 0 = controller busy: all other status bits invalid
//    //bit 5 = controller idle

    res = write(fd_hid, report, 4 + length);

    if (res == -1)
    {
        bc_log_error("_bc_bridge_ft260_i2c_write: call failed: write");

        return false;
    }

    if (res != (4 + length))
    {
        bc_log_error("_bc_bridge_ft260_i2c_write: requested length does not match");

        return false;
    }

    do
    {
        if (bc_tick_get() >= tick_timeout)
        {
            bc_log_error("_bc_bridge_ft260_i2c_write: i2c ready timeout bus_status check");

            return false;
        }
        if (!_bc_bridge_ft260_get_i2c_bus_status(fd_hid, &bus_status))
        {
            bc_log_error("_bc_bridge_ft260_i2c_write: call failed: _bc_bridge_ft260_get_i2c_bus_status");

            return false;
        }

    } while ((bus_status & 0x01) != 0);

    if ((bus_status & 0x1E) != 0)
    {
        // TODO log something
        return false;
    }

    return true;
}

static bool _bc_bridge_ft260_i2c_read(int fd_hid, uint8_t address, uint8_t *buffer, uint8_t length)
{
    uint8_t report[64];
    uint8_t bus_status;
    bc_tick_t tick_timeout;

    struct timeval tv;

    int res;

    fd_set fds;

    if (length > 60)
    {
        bc_log_error("_bc_bridge_ft260_i2c_read: length is too big");

        return false;
    }

    report[0] = 0xC2; /* I2C write */
    report[1] = address; /* Slave address */
    report[2] = 0x06; /* Start and Stop */
    report[3] = length;
    report[4] = 0;

    tick_timeout = bc_tick_get() + 100;

//    //wait on i2c redy
//    // TODO CPU intensive - upravit...
//    do
//    {
//        if (bc_tick_get() >= tick_timeout)
//        {
//            bc_log_error("_bc_bridge_ft260_i2c_read: i2c ready timeout");
//
//            return false;
//        }
//
//        if (!_bc_bridge_ft260_get_i2c_bus_status(fd_hid, &bus_status))
//        {
//            bc_log_error("_bc_bridge_ft260_i2c_read: call failed: _bc_bridge_ft260_get_i2c_bus_status");
//
//            return false;
//        }
//
//    } while ((bus_status & 0x11) != 0);
//
//    //bit 0 = controller busy: all other status bits invalid
//    //bit 5 = controller idle

    if (write(fd_hid, report, 5) == -1)
    {
        bc_log_error("_bc_bridge_ft260_i2c_read: call failed: write");

        return false;
    }

    do
    {
        if (bc_tick_get() >= tick_timeout)
        {
            bc_log_error("_bc_bridge_ft260_i2c_read: i2c ready timeout check bus_status");

            return false;
        }

        if (!_bc_bridge_ft260_get_i2c_bus_status(fd_hid, &bus_status))
        {
            bc_log_error("_bc_bridge_ft260_i2c_read: call failed: _bc_bridge_ft260_get_i2c_bus_status");

            return false;
        }

    } while ((bus_status & 0x01) != 0);

    if ((bus_status & 0x1E) != 0)
    {
        bc_log_error("_bc_bridge_ft260_i2c_read: bus error");

        return false;
    }

    FD_ZERO(&fds);
    FD_SET(fd_hid, &fds);

    tv.tv_sec = 0;
    tv.tv_usec = 1e9;

    res = select(fd_hid + 1, &fds, NULL, NULL, &tv);

    if (res == -1)
    {
        bc_log_error("_bc_bridge_ft260_i2c_read: call failed: select");

        return false;
    }

    if (res == 0)
    {
        bc_log_error("_bc_bridge_ft260_i2c_read: read timeout");

        return false;
    }

    if (read(fd_hid, report, 64) == -1)
    {
        bc_log_error("_bc_bridge_ft260_i2c_read: call failed: read");

        return false;
    }

    if (length != report[1])
    {
        bc_log_warning("_bc_bridge_ft260_i2c_read: requested length does not match");

        return false;
    }

    memcpy(buffer, &report[2],  report[1]);

    return true;
}

static void _bc_bridge_ft260_check_fd(int fd_hid)
{
    struct stat s;

    if (fstat(fd_hid, &s) == -1)
    {
        bc_log_fatal("_bc_bridge_ft260_check_fd: call failed: fstat");
    }

    if (s.st_nlink < 1)
    {
        bc_log_fatal("_bc_bridge_ft260_check_fd: device not connected");
    }
}

static bool _bc_bridge_ft260_get_feature(int fd_hid, uint8_t *buffer, size_t length)
{
    _bc_bridge_ft260_check_fd(fd_hid);

    if (ioctl(fd_hid, HIDIOCGFEATURE(length), buffer) == -1)
    {
        bc_log_error("_bc_bridge_ft260_get_feature: call failed: ioctl");

        return false;
    }

    return true;
}

static bool _bc_bridge_ft260_set_feature(int fd_hid, uint8_t *buffer, size_t length)
{
    _bc_bridge_ft260_check_fd(fd_hid);

    if (ioctl(fd_hid, HIDIOCSFEATURE(length), buffer) == -1)
    {
        bc_log_error("_bc_bridge_ft260_set_feature: call failed: ioctl");

        return false;
    }

    return true;
}
