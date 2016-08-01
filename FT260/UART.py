from FT260 import FT260
class UART:
    def __init__(self, ft260:FT260):
        self._ft260 = ft260

    def read(self, length:int, timeout=1000):
        return self._ft260.uart_read(length, timeout)

    def uart_write(self, data:bytes, timeout=None):
        self._ft260.uart_write(data, timeout)

