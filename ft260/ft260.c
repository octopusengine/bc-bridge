#include <libudev.h>
#include <linux/hidraw.h>
#include <linux/input.h>
#include <linux/types.h>
#include <string.h>
/*
* For the systems that don't have the new version of hidraw.h in userspace.
*/
#ifndef HIDIOCSFEATURE
#warning Please have your distro update the userspace kernel headers
#define HIDIOCSFEATURE(len) _IOC(_IOC_WRITE | _IOC_READ, 'H', 0x06, len) //LIBUSB_REQUEST_GET_DESCRIPTOR
#define HIDIOCGFEATURE(len) _IOC(_IOC_WRITE | _IOC_READ, 'H', 0x07, len) //LIBUSB_REQUEST_SET_DESCRIPTOR
#endif
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "ft260.h"

char *get_hid_path(unsigned short vendor_id, unsigned short product_id,
                   unsigned short interface_id) {
  struct udev *udev;
  struct udev_enumerate *enumerate;
  struct udev_list_entry *devices, *dev_list_entry;
  struct udev_device *dev;
  struct udev_device *usb_dev;
  struct udev_device *intf_dev;
  char *ret_path = NULL;
  /* Create the udev object */
  udev = udev_new();
  if (!udev) {
    printf("Can't create udev\n");
    return NULL;
  }
  /* Create a list of the devices in the 'hidraw' subsystem. */
  enumerate = udev_enumerate_new(udev);
  udev_enumerate_add_match_subsystem(enumerate, "hidraw");
  udev_enumerate_scan_devices(enumerate);
  devices = udev_enumerate_get_list_entry(enumerate);
  /* udev_list_entry_foreach is a macro which expands to a loop. */
  udev_list_entry_foreach(dev_list_entry, devices) {
    const char *path;
    const char *dev_path;
    const char *str;
    unsigned short cur_vid;
    unsigned short cur_pid;
    unsigned short cur_interface_id;
    path = udev_list_entry_get_name(dev_list_entry);
    dev = udev_device_new_from_syspath(udev, path);
    dev_path = udev_device_get_devnode(dev);
    /* Find the next parent device, with matching
    subsystem "usb" and devtype value "usb_device" */
    usb_dev =
        udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
    if (!usb_dev) {
      printf("Unable to find parent usb device.");
      return NULL;
    }
    str = udev_device_get_sysattr_value(usb_dev, "idVendor");
    cur_vid = (str) ? strtol(str, NULL, 16) : -1;
    str = udev_device_get_sysattr_value(usb_dev, "idProduct");
    cur_pid = (str) ? strtol(str, NULL, 16) : -1;
    intf_dev = udev_device_get_parent_with_subsystem_devtype(dev, "usb",
                                                             "usb_interface");
    if (!intf_dev) {
      printf("Unable to find parent usb interface.");
      return NULL;
    }
    str = udev_device_get_sysattr_value(intf_dev, "bInterfaceNumber");
    cur_interface_id = (str) ? strtol(str, NULL, 16) : -1;
    //printf("vid=%x pid=%x interface=%d\n", cur_vid, cur_pid, cur_interface_id);
    if (cur_vid == vendor_id && cur_pid == product_id &&
        cur_interface_id == interface_id) {
      ret_path = strdup(dev_path);
      udev_device_unref(dev);
      break;
    }
    udev_device_unref(dev);
  }
  /* Free the enumerator object */
  udev_enumerate_unref(enumerate);
  udev_unref(udev);
  return ret_path;
}