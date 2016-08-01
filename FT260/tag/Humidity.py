from I2C import I2C
import ctypes


class Humidity:
    
    ADDRESS             = 0x5F

    WHO_AM_I            = 0x0F
    CTRL_REG1           = 0x20
    CTRL_REG2           = 0x21
    CTRL_REG3           = 0x22

    STATUS_REG          = 0x27
    HUMIDITY_L_REG      = 0x28
    HUMIDITY_H_REG      = 0x29
    TEMP_L_REG          = 0x2A
    TEMP_H_REG          = 0x2B

    CALIB_REG_START     = 0x30
    CALIB_REG_END       = 0x3F

    def __init__(self, i2c:I2C):
        self._i2c = i2c

        buf = self._i2c.read_register(self.ADDRESS, self.CTRL_REG1)
        buf |= 0x80 # turn on the device
        buf |= 0x01 # 1Hz
        self._i2c.write_register(self.ADDRESS, self.CTRL_REG1, buf)

        self.load_calibration()


    def load_calibration(self):
        self.H0_rH = ctypes.c_uint8(self._i2c.read_register(self.ADDRESS, 0x30)).value / 2.0
        self.H1_rH = ctypes.c_uint8(self._i2c.read_register(self.ADDRESS, 0x31)).value / 2.0

        T0_degC_x8 = self._i2c.read_register(self.ADDRESS, 0x32)
        T1_degC_x8 = self._i2c.read_register(self.ADDRESS, 0x33)
        T1_T0_msb = self._i2c.read_register(self.ADDRESS, 0x35)

        self.T0_degC = ctypes.c_uint16( T0_degC_x8 | ((0x03 & T1_T0_msb) << 8) ).value / 8.0
        self.T1_degC = ctypes.c_uint16( T1_degC_x8 | ((0x0C & T1_T0_msb) << 6) ).value / 8.0

        self.H0_T0_OUT = ctypes.c_int16( self._i2c.read_register(self.ADDRESS, 0x36) | ( self._i2c.read_register(self.ADDRESS, 0x37) << 8 ) ).value
        self.H1_T0_OUT = ctypes.c_int16( self._i2c.read_register(self.ADDRESS, 0x3A) | ( self._i2c.read_register(self.ADDRESS, 0x3B) << 8 ) ).value

        self.T0_OUT = ctypes.c_int16( self._i2c.read_register(self.ADDRESS, 0x3C) | ( self._i2c.read_register(self.ADDRESS, 0x3D) << 8 ) ).value
        self.T1_OUT = ctypes.c_int16( self._i2c.read_register(self.ADDRESS, 0x3E) | ( self._i2c.read_register(self.ADDRESS, 0x3F) << 8 ) ).value

        self._t_grad = (self.T1_degC - self.T0_degC) / ( self.T1_OUT - self.T0_OUT)
        self._h_grad = (self.H1_rH - self.H0_rH) / (self.H1_T0_OUT - self.H0_T0_OUT)

    def read_temperature(self):
        status = self._i2c.read_register(self.ADDRESS, self.STATUS_REG)
        if status & 0x01 : #test temperature ready
            t_out = ctypes.c_int16( self._i2c.read_register(self.ADDRESS, self.TEMP_L_REG) | ( self._i2c.read_register(self.ADDRESS, self.TEMP_H_REG ) << 8 ) ).value
            temperature = self.T0_degC + ((t_out - self.T0_OUT) * self._t_grad )
            return temperature

    def read_humidity(self):
        status = self._i2c.read_register(self.ADDRESS, self.STATUS_REG)
        if status & 0x02 :
            h_out = ctypes.c_int16( self._i2c.read_register(self.ADDRESS, self.HUMIDITY_L_REG) | ( self._i2c.read_register(self.ADDRESS, self.HUMIDITY_H_REG ) << 8 ) ).value
            humidity = self.H0_rH + ((h_out - self.H0_T0_OUT) * self._h_grad )
            return humidity