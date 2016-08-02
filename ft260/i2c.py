from ft260 import FT260

class I2C:
    def __init__(self, ft260:FT260):
        self._ft260 = ft260

    def read(self, address:int, length:int, flag:int=0x06, timeout=1000):
        return self._ft260.i2c_read(address, length, flag, timeout)

    def write(self, address:int, data:bytes, flag:int=0x06, timeout=None):
        self._ft260.i2c_write(address, data, flag, timeout)

    def read_register(self, address:int, register:int):
        self.write(address, register.to_bytes(1, byteorder='little') )
        return self.read(address,1)[0]

    def write_register(self, address:int, register:int, value:int):
        self.write(address, bytes([register, value]))