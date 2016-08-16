from ft260 import find_devices, FT260
from i2c import I2C
from uart import UART
from tag.Humidity import Humidity

class Node:
    def __init__(self, ft260:FT260):
        self._ft260 = ft260

        i2c = I2C(self._ft260)

        i2c.write(112, b'\x01') #prepnuti i2c 

        humidity = Humidity(i2c)
        print('humidity', humidity.read_humidity(), '%')
            
        # for address in self._ft260.i2c_scan([Humidity.ADDRESS]):
        #     if Humidity.ADDRESS == address :
        #         humidity = Humidity(i2c)
        #         print('humidity', humidity.read_humidity(), '%')
            
if __name__ == '__main__':
    ft = FT260(find_devices()[0])
    node = Node(ft)