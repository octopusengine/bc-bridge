#!/usr/bin/python3
#
import sys
import usb.core
import usb.util as util
import usb.core as core
import threading
import math
import time
from tools import *

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

INPUT  = 0
OUTPUT = 1 

UART_MODE_OFF = 0
UART_MODE_RTS_CTS = 1
UART_MODE_DTR_DSR = 2
UART_MODE_XON_XOFF = 3
UART_MODE_NO_FLOW_CONTROL = 4

PARITY_NO = 0
PARITY_ODD = 1
PARITY_EVEN = 2
PARITY_HIGH = 3
PARITY_LOW = 4

class FT260:
    def __init__(self, device:usb.core.Device):
        self._device = device
        
        cfg = self._device.get_active_configuration()

        if self._device.is_kernel_driver_active(0):
            self._device.detach_kernel_driver(0)
        usb.util.claim_interface(self._device, 0)
        
        self._system_setting = self.get_report(REPORT_ID_SYSTEM_SETTING,25)

        i2c_intf = None
        self._i2c_ep_out = None
        self._i2c_ep_in = None
        self._i2c_lock = threading.RLock()
        
        uart_intf = None
        self._uart_ep_out = None
        self._uart_ep_in = None
        self._uart_lock = threading.RLock()

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
        wIndex = 0
        if report_id == REPORT_ID_UART_STATUS and self._i2c_ep_out :
            wIndex = 1
        return self._device.ctrl_transfer(
                bmRequestType = 0b10100001,
                bRequest = 0x01,
                wValue =  report_id | 0x300, #0x300 = (0x03 << 8)
                wIndex = wIndex,
                data_or_wLength = length )

    def set_report(self, report_id:int, data:bytes ):
        wIndex = 0
        if report_id == REPORT_ID_UART_STATUS and self._i2c_ep_out :
            wIndex = 1
        return self._device.ctrl_transfer(
                bmRequestType = 0b00100001,
                bRequest = 0x09,
                wValue =  report_id | 0x300, 
                wIndex = wIndex,
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

    def _set_system_setting(self, request:int, value:int):
        self.set_report(REPORT_ID_SYSTEM_SETTING, bytes([REPORT_ID_SYSTEM_SETTING, request, value]))
        self._reload_system_setting()

    def i2c_reset(self):
        with self._i2c_lock:
            self.set_report(REPORT_ID_SYSTEM_SETTING, b'\xA1\x20')
            time.sleep(1)

    @property
    def i2c_clock_speed(self):
        "Ranges from 60K bps to 3400K bps"
        i2c_status = self.get_report(REPORT_ID_I2C_STATUS,5)
        return int.from_bytes(i2c_status[2:4],'little')

    @i2c_clock_speed.setter
    def i2c_clock_speed(self, value:int ):
        if 60 <= value <= 3400: 
            self.set_report(REPORT_ID_SYSTEM_SETTING, b'\xA1\x22'+value.to_bytes(2, byteorder='little')  )
        else:
            raise ValueError('out of range')
            
    def i2c_write(self, address:int, data:bytes, flag:int=0x06, timeout=None):
        if len(data) == 0 :
            raise ValueError('empty data')
        if len(data) > 64 :
            raise ValueError('max len data 64')
        with self._i2c_lock:
            buf = bytes( [ 0xD0 + ((len(data)-1) // 4), address, flag, len(data)] ) + data
            #print('write', " ".join("%02x" % b for b in buf) )
            return self._i2c_ep_out.write(buf, timeout=timeout)

    def i2c_read(self, address:int, length:int, flag:int=0x06, timeout=1000):
        if length<0 or length > 64 :
            raise ValueError('length out of range 1 - 64')
        with self._i2c_lock:
            buf = bytes( [0xC2, address, flag] ) + length.to_bytes(2, byteorder='little')
            #print('read-write',  " ".join("%02x" % b for b in buf) )
            self._i2c_ep_out.write(buf)
            r = self._i2c_ep_in.read( 66, timeout=timeout)
            #print("read", " ".join("%02x" % b for b in r))
            return bytes(r[2:length+2])

    def i2c_scan(self, address_range:list = range(1,127) ):
        with self._i2c_lock:
            find = []
            for address in address_range:
                buf = self.i2c_read(address,4)
                if buf != b'\xFF\xFF\xFF\xFF':
                    #print("%02x" % i, buf)
                    find.append(address)
            return find

    def gpio_read_all(self):
        buf = self.get_report(REPORT_ID_GPIO, 5)
        return bytes(buf[1:])
    
    def gpio_write_all(self, data:bytes):
        if len(data) != 4 :
            raise ValueError
        return self.set_report(REPORT_ID_GPIO, b'\xB0'+data )

    def uart_reset(self):
        with self._uart_lock:
            self.set_report(REPORT_ID_SYSTEM_SETTING, b'\xA1\x40')
            time.sleep(1)

    @property
    def uart_status(self):
        return self.get_report(REPORT_ID_UART_STATUS,10)

    @property
    def uart_mode(self):
        return self._system_setting[6]

    @uart_mode.setter
    def uart_mode(self, value:int):
        if -1 < value < 5 :
            self._set_system_setting(0x03, value)
        else:
            raise ValueError

    @property
    def uart_baud_rate(self):
        "Ranges from 60K bps to 3400K bps"
        return int.from_bytes( self.uart_status[2:6],'little')

    @uart_baud_rate.setter
    def uart_baud_rate(self, value:int ):
        self.set_report(REPORT_ID_SYSTEM_SETTING, b'\xA1\x42'+value.to_bytes(4, byteorder='little')  )

    @property
    def uart_data_bit(self):
        return self.uart_status[6]
    
    @uart_data_bit.setter
    def uart_data_bit(self, value):
        if value == 7 or value == 8 :
            self.set_report(REPORT_ID_SYSTEM_SETTING, b'\xA1\x43'+value.to_bytes(1, byteorder='little')  )
        else:
            raise ValueError('7 or 8')

    @property
    def uart_parity(self): 
        return self.uart_status[7]

    @uart_parity.setter
    def uart_parity(self, value):
        if -1 < value < 5 :
            self.set_report(REPORT_ID_SYSTEM_SETTING, b'\xA1\x44'+value.to_bytes(1, byteorder='little')  )
        else:
            raise ValueError

    @property
    def uart_stop_bit(self):
        return self.uart_status[8]

    @uart_stop_bit.setter
    def uart_stop_bit(self, value):
        if value == 0 or value == 2 :
            self.set_report(REPORT_ID_SYSTEM_SETTING, b'\xA1\x45'+value.to_bytes(1, byteorder='little')  )
        else:
            raise ValueError('0 or 2')

    @property
    def uart_breaking(self):
        return bool(self.uart_status[9])

    @uart_breaking.setter
    def uart_breaking(self, value:bool):
        self.set_report(REPORT_ID_SYSTEM_SETTING, b'\xA1\x46'+(  b'\x01' if value else b'\x01' )  )
        
    def uart_write(self, data:bytes, timeout=None):
        if len(data) == 0 :
            raise ValueError('empty data')
        if len(data) > 64 :
            raise ValueError('max len data 64')
        with self._uart_lock:
            buf = bytes( [ 0xF0 + ((len(data)-1) // 4), len(data)] ) + data 
            return self._uart_ep_out.write(buf, timeout=timeout)

    def uart_read(self, length:int, timeout=1000):
        if length<0 or length > 64 :
            raise ValueError('length out of range 1 - 64')
        with self._uart_lock:
            r = self._uart_ep_in.read((math.ceil(length/4)*4)+2, timeout=timeout)
            return bytes(r[2:length+2])

if __name__ == '__main__':
    ft = FT260(find_devices()[0])
    # print(ft.i2c_scan())
    #print(ft.i2c_scan([32, 80, 100]))

    #print(ft.i2c_clock_speed)
    #ft.i2c_clock_speed = 100
    #print(ft.i2c_clock_speed)

    #human_print_gpio(ft.gpio_read_all())

    # ft.uart_reset()
    
    # print(ft.uart_baud_rate)
    ft.uart_baud_rate = 9600
    # print(ft.uart_baud_rate)

    # ft.uart_write(b'haha')
    # print(ft.uart_read(1, timeout=3000))





        