#!/usr/bin/python3
#

import sys
import usb.core
import usb.util as util
import usb.core as core
import threading
import functools

VENDOR_ID=0x0403
DEVICE_ID=0x6030

def find_devices():
    return list(usb.core.find(idVendor=VENDOR_ID, idProduct=DEVICE_ID, find_all=True))


REPORT_ID_CHIP_CODE = 0xA0
REPORT_ID_SYSTEM_SETTING = 0xA1
REPORT_ID_GPIO = 0xB0
REPORT_ID_I2C_STATUS = 0xC0
REPORT_ID_UART_STATUS = 0xE0

clk_ctl_12MHz = 0
clk_ctl_24MHz = 1
clk_ctl_48MHz = 2

def synchronized(f):
    @functools.wraps(f)
    def wrapper(self, *args, **kwargs):
        try:
            self.lock.acquire()
            return f(self, *args, **kwargs)
        finally:
            self.lock.release()
    return wrapper

class ft260:
    def __init__(self, device:usb.core.Device):
        self._device = device
        self.lock = threading.RLock()

        cfg = self._device.get_active_configuration()

        if self._device.is_kernel_driver_active(0):
            self._device.detach_kernel_driver(0)
        usb.util.claim_interface(self._device, 0)
        
        self._system_setting = self.get_report(REPORT_ID_SYSTEM_SETTING,25)

        i2c_intf = None
        self._i2c_ep_out = None
        self._i2c_ep_in = None
        
        uart_intf = None
        self._uart_ep_out = None
        self._uart_ep_in = None

        if cfg.bNumInterfaces == 2 :
            if self._device.is_kernel_driver_active(1):
                self._device.detach_kernel_driver(1)
            usb.util.claim_interface(self._device, 1)

            i2c_intf = cfg[(0,0)]
            uart_intf = cfg[(1,0)]
        
        else:
            if self.chip_mode == 1 :
                i2c_intf = cfg[(0,0)]
            elif self.chip_mode == 2 :
                uart_intf = cfg[(0,0)]
        
        if i2c_intf :
            self._i2c_ep_out = usb.util.find_descriptor(i2c_intf, custom_match = lambda e: usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_OUT)
            self._i2c_ep_in = usb.util.find_descriptor(i2c_intf, custom_match = lambda e: usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_IN)
        if uart_intf :
            self._uart_ep_out = usb.util.find_descriptor(uart_intf, custom_match = lambda e: usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_OUT)
            self._uart_ep_in = usb.util.find_descriptor(uart_intf, custom_match = lambda e: usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_IN)
    
    def get_report(self, report_id:int, length:int ):
        return self._device.ctrl_transfer(
                bmRequestType = 0b10100001,
                bRequest = 0x01,
                wValue =  report_id | 0x300, #0x300 = (0x03 << 8)
                wIndex = 0,
                data_or_wLength = length )

    def set_report(self, report_id:int, data:bytes ):
        print('set_report', data)
        return self._device.ctrl_transfer(
                bmRequestType = 0b00100001,
                bRequest = 0x09,
                wValue =  report_id | 0x300, 
                wIndex = 0,
                data_or_wLength = data)

    def _reload_system_setting(self):
        self._system_setting = self.get_report(REPORT_ID_SYSTEM_SETTING,25)

    @property
    def chip_mode(self):
        return self._system_setting[1]

    @property
    def clk_ctl(self):
        "System Clock"
        return self._system_setting[2]

    @clk_ctl.setter
    def clk_ctl(self, value:int):
        "Set System Clock"
        if value != self.clk_ctl and value in (clk_ctl_12MHz, clk_ctl_24MHz, clk_ctl_48MHz):
            self._set_system_setting(0x01, value)

    def _set_system_setting(self, request:int,value:int):
        self.set_report(REPORT_ID_SYSTEM_SETTING, bytes([REPORT_ID_SYSTEM_SETTING, request, value]))
        self._reload_system_setting()


    @synchronized
    def i2c_reset():
        self.set_report(REPORT_ID_SYSTEM_SETTING, bytes([REPORT_ID_SYSTEM_SETTING, 0x20]))

    @synchronized
    def i2c_write(self, address:int, data:list, flag:int=0x06, timeout=None):
        buf = bytes( [0xD0, address, flag, len(data)] + data )
        #print('write', " ".join("%02x" % b for b in buf) )
        return self._i2c_ep_out.write(buf, timeout=timeout)

    @synchronized
    def i2c_read(self, address:int, length:int, flag:int=0x06, timeout=1000):
        buf = bytes( [0xC2, address, flag] ) + length.to_bytes(2, byteorder='little')
        #print('read-write',  " ".join("%02x" % b for b in buf) )
        self._i2c_ep_out.write(buf)
        r = self._i2c_ep_in.read(64, timeout=timeout)
        #print("read", " ".join("%02x" % b for b in r))
        return r[2:length+2].tolist()

    @synchronized
    def i2c_scan(self, address_range:list = range(1,127) ):
        find = []
        for address in address_range:
            buf = self.i2c_read(address,4)
            if buf != [255,255,255,255]:
                #print("%02x" % i, buf)
                find.append(address)
        return find

if __name__ == '__main__':
    ft = ft260(find_devices()[0])
    print(ft.i2c_scan())
    print(ft.i2c_scan([32, 80, 100]))
    





        