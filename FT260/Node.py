from FT260 import find_devices, FT260
from tag.Humidity import Humidity

class Node:
    def __init__(self, ft260:FT260):
        self._ft260 = ft260

        i2c = self._ft260.get_i2c()

        for address in self._ft260.i2c_scan([Humidity.ADDRESS]):
            if Humidity.ADDRESS == address :
                humidity = Humidity(i2c)
                print('humidity', humidity.read_humidity(), '%')
            
if __name__ == '__main__':
    ft = FT260(find_devices()[0])
    node = Node(ft)