
#include "bc/bridge.h"

#include <libudev.h>
#include <linux/hidraw.h>
#include <linux/input.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/file.h>
#include <bc/os.h>
#include <bc/tag.h>
#include <bc/bridge.h>


#define VENDOR_ID 0x0403
#define DEVICE_ID 0x6030

#define REPORT_ID_CHIP_CODE 0xA0
#define REPORT_ID_SYSTEM_SETTING 0xA1
#define REPORT_ID_GPIO 0xB0
#define REPORT_ID_I2C_STATUS 0xC0
#define REPORT_ID_UART_STATUS 0xE0

static bool _bc_bridge_i2c_set_channel(bc_bridge_t *self, bc_bridge_i2c_channel_t i2c_channel);

static bool _bc_bridge_ft260_i2c_set_clock_speed(int fd_hid, uint32_t speed);
static bool _bc_bridge_ft260_get_i2c_bus_status(int fd_hid, uint8_t *bus_status);
static bool _bc_bridge_ft260_i2c_write(int fd_hid, uint8_t address, uint8_t *data, uint8_t length);
static bool _bc_bridge_ft260_i2c_read(int fd_hid, uint8_t address, uint8_t *data, uint8_t length);

//static bool _bc_bridge_ft260_uart_set_default_configuration(int fd_hid);
static void _bc_bridge_ft260_check_fd(int fd_hid);
static int32_t _bc_bridge_get_now_in_ms();
static bool _bc_bridge_ft260_get_feature(int fd_hid, void *buffer, size_t length);
static bool _bc_bridge_ft260_set_feature(int fd_hid, uint8_t *buffer, size_t length);

bool bc_bridge_scan(bc_bridge_device_info_t *devices, uint8_t *length)
{
    struct udev *udev;
    struct udev_enumerate *enumerate, *enumerate_hid;
    struct udev_list_entry *devices_usb, *entry, *devices_hid, *entry_hid;
    struct udev_device *usb_dev;
    struct udev_device* hid = NULL;
    *length = 0; 

    /* Create the udev object */
    udev = udev_new();

    if (!udev)
    {
        perror("Can't create udev\n");
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
        const char *str;
        unsigned short cur_vid;
        unsigned short cur_did;
        const char* path = udev_list_entry_get_name(entry);
        const char* i2c_hid_path=NULL;
        const char* uart_hid_path=NULL;

        usb_dev = udev_device_new_from_syspath(udev, path);
    
        str = udev_device_get_sysattr_value(usb_dev, "idVendor");
        cur_vid = (unsigned short) ((str) ? strtol(str, NULL, 16) : -1);
        str = udev_device_get_sysattr_value(usb_dev, "idProduct");
        cur_did = (unsigned short) ((str) ? strtol(str, NULL, 16) : -1);

        if (cur_vid == VENDOR_ID && cur_did == DEVICE_ID)
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

                if (i2c_hid_path==NULL)
                {
                    i2c_hid_path = udev_device_get_devnode(hid);
                }
                else if (uart_hid_path==NULL)
                {
                    uart_hid_path = udev_device_get_devnode(hid);
                }
                else
                {
                    perror("Unexpected hid device");
                    return false;
                }
            }

            if ((i2c_hid_path!=NULL) && (uart_hid_path!=NULL))
            {

                devices[*length].usb_path = (char *)udev_device_get_devnode(usb_dev);
                devices[*length].i2c_hid_path = (char *)i2c_hid_path;
                devices[*length].uart_hid_path = (char *)uart_hid_path;                

                //printf("%s %s \n", i2c_hid_path, uart_hid_path);

                *length += 1;
            }

        }
    }

    return true;
}

bool bc_bridge_open(bc_bridge_t *self, bc_bridge_device_info_t *info)
{
    memset(self, 0, sizeof(*self));

    self->_i2c_fd_hid = open(info->i2c_hid_path, O_RDWR | O_NONBLOCK);
    if (self->_i2c_fd_hid < 0)
    {
        perror("Unable to open hid0\n");
        return false;
    }
    
    if (flock(self->_i2c_fd_hid, LOCK_EX|LOCK_NB) == -1)
    {
        perror("Unable to lock hid0\n");
        return false;
    }

    bc_os_mutex_init( &self->_i2c_mutex );
    _bc_bridge_ft260_i2c_set_clock_speed(self->_i2c_fd_hid, 100);
    _bc_bridge_i2c_set_channel(self, BC_BRIDGE_I2C_CHANNEL_1);

    return true;
}

bool bc_bridge_close(bc_bridge_t *self)
{
    flock(self->_i2c_fd_hid, LOCK_UN);
    close(self->_i2c_fd_hid);
    return true;
}

bool bc_bridge_i2c_write(bc_bridge_t *self, bc_bridge_i2c_transfer_t *transfer)
{
    bool status=false;

    bc_os_mutex_lock(&(self->_i2c_mutex));

    if (_bc_bridge_i2c_set_channel(self, transfer->channel))
    {
        status = _bc_bridge_ft260_i2c_write(self->_i2c_fd_hid, transfer->device_address, transfer->buffer,
                                            transfer->length);
    }

    bc_os_mutex_unlock(&(self->_i2c_mutex));

    return status;
}

bool bc_bridge_i2c_read(bc_bridge_t *self, bc_bridge_i2c_transfer_t *transfer)
{
    bool status=false;

    bc_os_mutex_lock(&(self->_i2c_mutex));

    if (_bc_bridge_i2c_set_channel(self, transfer->channel)){
        status = _bc_bridge_ft260_i2c_read(self->_i2c_fd_hid, transfer->device_address, transfer->buffer,
                                           transfer->length);
    }

    bc_os_mutex_unlock(&(self->_i2c_mutex));

    return status;
}

bool bc_bridge_i2c_write_register(bc_bridge_t *self, bc_bridge_i2c_transfer_register_t *transfer)
{
    bool status;
    uint8_t buffer[60];

    if (transfer->address_16_bit)
    {
        if (transfer->length > 58)
        {
            return false;
        }
        buffer[0] = (uint8_t) (transfer->address >> 8);
        buffer[1] = (uint8_t) transfer->address;
        memcpy(buffer + 2,transfer->buffer, transfer->length);
    }
    else
    {
        if (transfer->length > 59)
        {
            return false;
        }
        buffer[0] = (uint8_t) transfer->address;
        memcpy(buffer + 1, transfer->buffer, transfer->length);
    }

    bc_os_mutex_lock(&(self->_i2c_mutex));

    if (_bc_bridge_i2c_set_channel(self, transfer->channel))
    {
        status = _bc_bridge_ft260_i2c_write(self->_i2c_fd_hid, transfer->device_address, buffer,
                                            (transfer->address_16_bit ? 2 : 1) + transfer->length);
    }

    bc_os_mutex_unlock(&(self->_i2c_mutex));

    return  status;
}

bool bc_bridge_i2c_read_register(bc_bridge_t *self, bc_bridge_i2c_transfer_register_t *transfer)
{
    bool status;
    uint8_t buffer[2];

    if (transfer->length > 60)
    {
        return false;
    }

    if (transfer->address_16_bit)
    {
        buffer[0] = (uint8_t) (transfer->address >> 8);
        buffer[1] = (uint8_t) transfer->address;
    }
    else
    {
        buffer[0] = (uint8_t) transfer->address;
    }

    bc_os_mutex_lock(&(self->_i2c_mutex));

    if (_bc_bridge_i2c_set_channel(self, transfer->channel))
    {
        status = _bc_bridge_ft260_i2c_write(self->_i2c_fd_hid, transfer->device_address, buffer,
                                            (transfer->address_16_bit ? 2 : 1));
        if (status)
        {
            status = _bc_bridge_ft260_i2c_read(self->_i2c_fd_hid, transfer->device_address, transfer->buffer,
                                               transfer->length);
        }
    }

    bc_os_mutex_unlock(&(self->_i2c_mutex));

    return status;

}

bool bc_bridge_led_set_state(bc_bridge_t *self, bc_bridge_led_state_t state);


static bool _bc_bridge_i2c_set_channel(bc_bridge_t *self, bc_bridge_i2c_channel_t i2c_channel)
{
    uint8_t buffer[1];

    if (self->_i2c_channel == i2c_channel)
    {
        return true;
    }

    buffer[0] = (uint8_t) i2c_channel;

    if (_bc_bridge_ft260_i2c_write(self->_i2c_fd_hid, 0x70, buffer, sizeof(buffer)))
    {
        self->_i2c_channel = i2c_channel;
        return true;
    }

    return false;
}

bool ft260_i2c_reset(int fd_hid)
{
    uint8_t buffer[2];
    uint8_t bus_status;
    int32_t start;

    buffer[0] = REPORT_ID_SYSTEM_SETTING;
    buffer[1] = 0x20;

    if (!_bc_bridge_ft260_set_feature(fd_hid, buffer, sizeof(buffer)))
    {
        return false;
    }

    start = _bc_bridge_get_now_in_ms();
    //kontroluje se stav i2c, pokud do 1s nedojde k resetu vraci false
    do
    {
        if( !_bc_bridge_ft260_get_i2c_bus_status(fd_hid, &bus_status) ){
            return false;
        }
        if(bus_status==0x20){
            return true;
        }

    }while(_bc_bridge_get_now_in_ms() - start < 1000 );

    return false;
}

static bool _bc_bridge_ft260_i2c_set_clock_speed(int fd_hid, uint32_t speed)
{
    uint8_t buffer[4];

    if ((speed < 60) || (speed > 3400))
    {
        return false;
    }

    buffer[0] = REPORT_ID_SYSTEM_SETTING;
    buffer[1] = 0x22;
    buffer[2] = (uint8_t) speed;
    buffer[3] = (uint8_t) (speed >> 8);

    return _bc_bridge_ft260_set_feature(fd_hid, buffer, sizeof(buffer));
}

static bool _bc_bridge_ft260_get_i2c_bus_status(int fd_hid, uint8_t *bus_status)
{
    uint8_t buffer[5];

    buffer[0] = 0xC0;

    if (!_bc_bridge_ft260_get_feature(fd_hid, buffer, sizeof(buffer)))
    {
        return false;
    }

    *bus_status = buffer[1];
    return true;
}

static bool _bc_bridge_ft260_i2c_write(int fd_hid, uint8_t address, uint8_t *data, uint8_t length)
{
    uint8_t buffer[64];
    uint8_t bus_status=0;

    if (length > 60)
    {
        return false;
    }

    memcpy(&buffer[4], data, length);

    buffer[0] = (uint8_t) (0xD0 + ((length - 1) / 4)); /* I2C write */
    buffer[1] = address; /* Slave address */
    buffer[2] = 0x06; /* Start and Stop */
    buffer[3] = length;

    _bc_bridge_ft260_check_fd(fd_hid);

    if (write(fd_hid, buffer, 4 + length) != (4 + length))
    {
        return false;
    }

    //TODO co delat pokud je 1 bit v 1 a neplati ostatni ?
    if (!_bc_bridge_ft260_get_i2c_bus_status(fd_hid, &bus_status) ||
            ( ((bus_status & 0x01) == 0) &&  ( (bus_status & 0x1E) != 0x00 ) ) )
    {
        fprintf(stderr, "ft260_i2c_write bus_status %x %d \n", address, bus_status);
        return false;
    }

    return true;
}

static bool _bc_bridge_ft260_i2c_read(int fd_hid, uint8_t address, uint8_t *data, uint8_t length)
{
    uint8_t buffer[64];
    uint8_t bus_status=0;
    struct timeval timeout;

    int res;

    fd_set set;

    if (length > 60)
    {
        return false;
    }

    buffer[0] = 0xC2; /* I2C write */
    buffer[1] = address; /* Slave address */
    buffer[2] = 0x06; /* Start and Stop */
    buffer[3] = length;
    buffer[4] = 0;

    _bc_bridge_ft260_check_fd(fd_hid);

    if (write(fd_hid, buffer, 5) == -1)
    {
        return false;
    }

    if (!_bc_bridge_ft260_get_i2c_bus_status(fd_hid, &bus_status) ||
            ( ((bus_status & 0x01) == 0) &&  ( (bus_status & 0x1E) != 0x00 ) ) )
    {
        fprintf(stderr, "ft260_i2c_read bus_status %x %d \n", address, bus_status);
        return false;
    }

    FD_ZERO(&set); /* clear the set */
    FD_SET(fd_hid, &set); /* add our file descriptor to the set */
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;

    res = select(fd_hid + 1, &set, NULL, NULL, &timeout);

    if (res == -1 || res == 0)
    {
        return false;
    }

    res = (int) read(fd_hid, buffer, 64);

    if (res == -1)
    {
        return false;
    }

    if (length != buffer[1])
    {
        //fprintf(stderr, "ft260_i2c_read nesedi length a buffer %d %d\n", length, buffer[1]);
        return false;
    }

    memcpy(data, buffer + 2,  buffer[1] );

    return true;
}

//static bool _ft260_uart_reset(int fd_hid)
//{
//    uint8_t buffer[2];
//
//    buffer[0] = REPORT_ID_SYSTEM_SETTING;
//    buffer[1] = 0x40;
//
//    _bc_bridge_ft260_set_feature(fd_hid, buffer, sizeof(buffer));
//
//    // TODO WHY NEEDED
//    sleep(1);
//    return true;
//}

//static bool _bc_bridge_ft260_uart_set_default_configuration(int fd_hid)
//{
//    uint8_t buffer[11];
//    buffer[0] = REPORT_ID_SYSTEM_SETTING;
//    buffer[1] = 0x41;
//    buffer[2] = 0x04;//No flow control mode
//    buffer[3] = 0x80; // baud rate: 9600 = 0x2580
//    buffer[4] = 0x25;
//    buffer[5] = 0x00;
//    buffer[6] = 0x00;
//    buffer[7] = 0x08; //data bits: 8 = data bits
//    buffer[8] = 0x00; //parity: 0 = No parity
//    buffer[9] = 0x02; //stop bits: 2 = two stop bits
//    buffer[10] = 0;//breaking: 0 = no break
//    return _bc_bridge_ft260_set_feature(fd_hid, buffer, sizeof(buffer));
//}

//static void _ft260_uart_print_configuration(int fd_hid)
//{
//    uint8_t buffer[10];
//
//    buffer[0] = REPORT_ID_UART_STATUS;
//
//    _bc_bridge_ft260_get_feature(fd_hid, buffer, sizeof(buffer));
//    uint32_t baud_rate;
//    memcpy(&baud_rate, &buffer[2], sizeof(baud_rate));
//
//    fprintf(stderr,"flow ctrl %d \n", buffer[1] );
//    fprintf(stderr,"baud rate %d \n", baud_rate );
//    fprintf(stderr,"data bit %d \n", buffer[6] );
//    fprintf(stderr,"parity %d \n", buffer[7] );
//    fprintf(stderr,"stop bit %d \n", buffer[8] );
//    fprintf(stderr,"breaking %d \n", buffer[9] );
//}

/**
 *
 * @param fd file descriptor
 */
static void _bc_bridge_ft260_check_fd(int fd_hid)
{
    struct stat s;
    fstat(fd_hid, &s);
    if (s.st_nlink<1)
    {
        perror("device is not connect");
        exit(EXIT_FAILURE);
    }
}

static int32_t _bc_bridge_get_now_in_ms()
{
    struct timeval now;
    gettimeofday(&now,NULL);
    return (int32_t) ((1000 * now.tv_sec  ) + (now.tv_usec / 1000));
}

static bool _bc_bridge_ft260_get_feature(int fd_hid, void *buffer, size_t length)
{
    _bc_bridge_ft260_check_fd(fd_hid);
    return ioctl(fd_hid, HIDIOCGFEATURE(length), buffer) == -1 ? false : true;
}

static bool _bc_bridge_ft260_set_feature(int fd_hid, uint8_t *buffer, size_t length)
{
    _bc_bridge_ft260_check_fd(fd_hid);
    return ioctl(fd_hid, HIDIOCSFEATURE(length), buffer) == -1 ? false : true;
}