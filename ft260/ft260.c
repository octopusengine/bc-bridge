
#include "bc/bridge.h"

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
#include <sys/time.h>
#include <sys/file.h>

#define VENDOR_ID 0x0403
#define DEVICE_ID 0x6030

#define REPORT_ID_CHIP_CODE 0xA0
#define REPORT_ID_SYSTEM_SETTING 0xA1
#define REPORT_ID_GPIO 0xB0
#define REPORT_ID_I2C_STATUS 0xC0
#define REPORT_ID_UART_STATUS 0xE0

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
        cur_vid = (str) ? strtol(str, NULL, 16) : -1;
        str = udev_device_get_sysattr_value(usb_dev, "idProduct");
        cur_did = (str) ? strtol(str, NULL, 16) : -1;

        if (cur_vid == VENDOR_ID && cur_did == DEVICE_ID)
        {
            
            enumerate_hid = udev_enumerate_new(udev);
            udev_enumerate_add_match_parent(enumerate_hid, usb_dev);
            udev_enumerate_add_match_subsystem(enumerate_hid, "hidraw");
            udev_enumerate_scan_devices(enumerate_hid);
            devices_hid = udev_enumerate_get_list_entry(enumerate_hid);

            udev_list_entry_foreach(entry_hid, devices_hid)
            {
                const char *path = udev_list_entry_get_name(entry_hid);
                hid = udev_device_new_from_syspath(udev, path);

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
    self->_i2c_fd_hid = open(info->i2c_hid_path, O_RDWR | O_NONBLOCK);
    if (self->_i2c_fd_hid < 0)
    {
        perror("Unable to open hid0");
        return false;
    }
    if (flock(self->_i2c_fd_hid, LOCK_EX|LOCK_NB) == -1)
    {
        perror("Unable to lock hid0");
        return false;
    }

    return true;
}

bool bc_bridge_close(bc_bridge_t *self)
{
    flock(self->_i2c_fd_hid, LOCK_UN);
    close(self->_i2c_fd_hid);
    return true;
}
void bc_bridge_i2c_lock(bc_bridge_t *self);
void bc_bridge_i2c_unlock(bc_bridge_t *self);
bool bc_bridge_i2c_write(bc_bridge_t *self, bc_bridge_i2c_transfer_t *transfer);
bool bc_bridge_i2c_read(bc_bridge_t *self, bc_bridge_i2c_transfer_t *transfer);
bool bc_bridge_led_set_state(bc_bridge_t *self, bc_bridge_led_state_t state);