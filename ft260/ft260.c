#include "ft260.h"

#include <libudev.h>
#include <linux/hidraw.h>
#include <linux/input.h>
#include <linux/types.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define VENDOR_ID 0x0403
#define DEVICE_ID 0x6030

#define REPORT_ID_CHIP_CODE 0xA0
#define REPORT_ID_SYSTEM_SETTING 0xA1
#define REPORT_ID_GPIO 0xB0
#define REPORT_ID_I2C_STATUS 0xC0
#define REPORT_ID_UART_STATUS 0xE0

static int hid_i2c;
static int hid_uart;

static bool ft260_set_feature(int hid, uint8_t *buffer, size_t length);
static bool ft260_get_feature(int hid, void *buffer, size_t length);

bool ft260_open(void)
{
    struct udev *udev;
    struct udev_enumerate *enumerate, *enumerate_hid;
    struct udev_list_entry *devices, *entry, *devices_hid, *entry_hid;
    struct udev_device *usb_dev;
    struct udev_device* hid = NULL;

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
    devices = udev_enumerate_get_list_entry(enumerate);

    udev_list_entry_foreach(entry, devices)
    {
        const char *str;
        unsigned short cur_vid;
        unsigned short cur_did;
        const char* path = udev_list_entry_get_name(entry);

        usb_dev = udev_device_new_from_syspath(udev, path);
    
        str = udev_device_get_sysattr_value(usb_dev, "idVendor");
        cur_vid = (str) ? strtol(str, NULL, 16) : -1;
        str = udev_device_get_sysattr_value(usb_dev, "idProduct");
        cur_did = (str) ? strtol(str, NULL, 16) : -1;

        if (cur_vid == VENDOR_ID && cur_did == DEVICE_ID)
        {
            fprintf(stderr,"path %s \n", udev_device_get_devnode(usb_dev));

            fprintf(stderr,"idVendor %hx \n", cur_vid );
            fprintf(stderr,"idProduct %hx \n", cur_did );

            enumerate_hid = udev_enumerate_new(udev);
            udev_enumerate_add_match_parent(enumerate_hid, usb_dev);
            udev_enumerate_add_match_subsystem(enumerate_hid, "hidraw");
            udev_enumerate_scan_devices(enumerate_hid);
            devices_hid = udev_enumerate_get_list_entry(enumerate_hid);

            bool success_i2c = false;
            bool success_uart = false;

            udev_list_entry_foreach(entry_hid, devices_hid)
            {
                const char *path = udev_list_entry_get_name(entry_hid);
                hid = udev_device_new_from_syspath(udev, path);
                const char *hid_path = udev_device_get_devnode(hid);

                fprintf(stderr, "hid_path %s \n", hid_path);

                if (!hid_i2c)
                {
                    hid_i2c = open(hid_path, O_RDWR | O_NONBLOCK);

                    if (hid_i2c < 0)
                    {
                        perror("Unable to open hid0");

                        return false;
                    }
                    else
                    {
                        success_i2c = true;
                    }
                }
                else if (!hid_uart)
                {
                    hid_uart = open(hid_path, O_RDWR | O_NONBLOCK);
                    
                    if (hid_uart < 0)
                    {
                        perror("Unable to open hid1");

                        return false;
                    }
                    else
                    {
                        success_uart = true;
                    }
                }
                else
                {
                    perror("Unexpected hid device");

                    return false;
                }
            }

            return (success_i2c && success_uart) ? true : false;
        }
    }

    return false;
}

bool ft260_close(void)
{
    if (close(hid_i2c) == -1)
    {
        return false;
    }

    if (close(hid_uart) == -1)
    {
        return false;
    }

    return true;
}


// TODO Move out :-)
void print_buffer(uint8_t *buffer, int length)
{
    int i;

    printf("print_buffer res: %d\n", length);

    for (i = 0; i < length; i++)
    {
        printf("%d %02x %d\n", i, buffer[i], buffer[i]);
    }
}


bool ft260_check_chip_version(void)
{
    uint8_t buffer[13];

    buffer[0] = REPORT_ID_CHIP_CODE;

    if (!ft260_get_feature(hid_i2c, buffer, sizeof(buffer)))
    {
        return false;
    }

    return (buffer[1] == 0x02) && (buffer[2] == 0x60) ? true : false;
}

bool ft260_led(ft260_led_state_t state)
{
    uint8_t buffer[5];

    buffer[0] = REPORT_ID_GPIO;

    if (!ft260_get_feature(hid_i2c, buffer, sizeof(buffer)))
    {
        return false;
    }

    buffer[3] &= 0x7F;

    if (state == FT260_LED_STATE_ON)
    {
        buffer[3] |= 0x80;
    }

    buffer[4] |= 0x80;

    if (!ft260_set_feature(hid_i2c, buffer, sizeof(buffer)))
    {
        return false;
    }

    return true;
}

bool ft260_i2c_reset(void)
{
    uint8_t buffer[2];

    buffer[0] = REPORT_ID_SYSTEM_SETTING;
    buffer[1] = 0x20;

    if (!ft260_set_feature(hid_i2c, buffer, sizeof(buffer)))
    {
        return false;
    }

    // TODO WHY NEEDED
    sleep(1);

    return true;
}

bool ft260_i2c_get_bus_status(uint8_t *bus_status)
{
    uint8_t buffer[5];

    buffer[0] = 0xC0;

    if (!ft260_get_feature(hid_i2c, buffer, sizeof(buffer)))
    {
        return false;
    }

    *bus_status = buffer[1];
    return true;
}

bool ft260_i2c_set_clock_speed(uint32_t speed)
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

    return ft260_set_feature(hid_i2c, buffer, sizeof(buffer));
}

bool ft260_i2c_get_clock_speed(uint32_t *speed)
{
    uint8_t buffer[5];

    buffer[0] = REPORT_ID_I2C_STATUS;

    if (!ft260_get_feature(hid_i2c, buffer, sizeof(buffer)))
    {
        return false;
    }

    *speed = (uint32_t) buffer[2];
    *speed |= ((uint32_t) buffer[3]) << 8;

    return true;
}

bool ft260_i2c_write(uint8_t address, uint8_t *data, uint8_t length)
{
    uint8_t buffer[64];

    if (length > 60)
    {
        return false;
    }

    memcpy(&buffer[4], data, length);

    buffer[0] = 0xD0 + ((length - 1) / 4); /* I2C write */
    buffer[1] = address; /* Slave address */
    buffer[2] = 0x06; /* Start and Stop */
    buffer[3] = length;

    return write(hid_i2c, buffer, 4 + length) == (4 + length) ? true : false;
}

bool ft260_i2c_read(uint8_t address, uint8_t *data, uint8_t length)
{
    uint8_t buffer[64];

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

    if (write(hid_i2c, buffer, 5) == -1)
    {
        return false;
    }

    struct timeval timeout;

    FD_ZERO(&set); /* clear the set */
    FD_SET(hid_i2c, &set); /* add our file descriptor to the set */
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000;

    res = select(hid_i2c + 1, &set, NULL, NULL, &timeout);

    if (res == -1 || res == 0)
    {
        return false;
    }

    res = read(hid_i2c, buffer, 64);

    if (res == -1)
    {
	return false;
    }

    if (length != buffer[1])
    {
        return false;
    }

    memcpy(data, buffer + 2, length > buffer[1] ? buffer[1] : length );

    return true;
}

bool ft260_i2c_set_bus(ft260_i2c_bus_t i2c_bus)
{
    uint8_t buffer[1];

    buffer[0] = (uint8_t) i2c_bus;

    return ft260_i2c_write(0x70, buffer, sizeof(buffer));
}

bool ft260_i2c_is_device_exists(uint8_t address)
{
    // TODO WHY THIS
    uint8_t buffer[1];
    uint8_t bus_status;

    if (!ft260_i2c_write(address, buffer, 1))
    {
        return false;
    }

    if (!ft260_i2c_get_bus_status(&bus_status)){
        return false;
    }

    if (bus_status & 0x04) //bit 2 = slave address was not acknowledged during last operation
    {
        return false;
    }

    return true;
}

void ft260_i2c_scan(void)
{
    uint8_t address;

    for (address = 1; address < 128; address++)
    {
        if (ft260_i2c_is_device_exists(address))
        {
             printf("address: %hhx %d \n", address, address);
        }
    }
}

bool ft260_uart_reset(void)
{
    uint8_t buffer[2];

    buffer[0] = REPORT_ID_SYSTEM_SETTING;
    buffer[1] = 0x40;

    ft260_set_feature(hid_uart, buffer, sizeof(buffer));

    // TODO WHY NEEDED
    sleep(1);
    return true;
}

bool ft260_uart_set_default_configuration(void)
{
    uint8_t buffer[11];
    buffer[0] = REPORT_ID_SYSTEM_SETTING;
    buffer[1] = 0x41;
    buffer[2] = 0x04;//No flow control mode
    buffer[3] = 0x80; // baud rate: 9600 = 0x2580
    buffer[4] = 0x25;
    buffer[5] = 0x00;
    buffer[6] = 0x00;
    buffer[7] = 0x08; //data bits: 8 = data bits
    buffer[8] = 0x00; //parity: 0 = No parity
    buffer[9] = 0x02; //stop bits: 2 = two stop bits
    buffer[10] = 0;//breaking: 0 = no break
    return ft260_set_feature(hid_uart, buffer, sizeof(buffer));
}

void ft260_uart_print_configuration(void)
{
    uint8_t buffer[10];

    buffer[0] = REPORT_ID_UART_STATUS;

    ft260_get_feature(hid_uart, buffer, sizeof(buffer));
    uint32_t baud_rate;
    memcpy(&baud_rate, &buffer[2], sizeof(baud_rate));
    
    fprintf(stderr,"flow ctrl %d \n", buffer[1] );
    fprintf(stderr,"baud rate %d \n", baud_rate );
    fprintf(stderr,"data bit %d \n", buffer[6] );
    fprintf(stderr,"parity %d \n", buffer[7] );
    fprintf(stderr,"stop bit %d \n", buffer[8] );
    fprintf(stderr,"breaking %d \n", buffer[9] );
}


static bool ft260_set_feature(int hid, uint8_t *buffer, size_t length)
{
    return ioctl(hid, HIDIOCSFEATURE(length), buffer) == -1 ? false : true;
}

static bool ft260_get_feature(int hid, void *buffer, size_t length)
{
    return ioctl(hid, HIDIOCGFEATURE(length), buffer) == -1 ? false : true;
}






#if 0

int ft260_uart_write(uint8_t *data, uint8_t length)
{
    uint8_t buffer[ 2+data_length ];

    memcpy(buffer+4,data,data_length);

    buffer[0] = 0xF0 + ((data_length-1) / 4); 
    buffer[1] = data_length;

    return write(hid_uart, buffer, sizeof(buffer));
}

int ft260_uart_read(char *data, char data_length){
    char buffer[64];
    int res;
    fd_set set;
    struct timeval timeval_timeout;
    FD_ZERO(&set); /* clear the set */
    FD_SET(hid_uart, &set); /* add our file descriptor to the set */
    timeval_timeout.tv_sec = 0;
    timeval_timeout.tv_usec = 500000;
    res = select(hid_uart + 1, &set, NULL, NULL, &timeval_timeout);
    if(res == -1)/* an error accured */
        return -1;
    else if(res == 0)/* a timeout occured */
        return -1;
    else
        res = read(hid_uart, buffer, 64);
        
    if(res<0){
        return res;
    }    
    //print_buffer(buffer, res);

    if( data_length > buffer[1] ){
        data_length = buffer[1];
    }
    memcpy(data,buffer+2,data_length);
    return data_length;
}


#endif

