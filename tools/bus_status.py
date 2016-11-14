#!/usr/bin/python3
import sys

bus_status = int(sys.argv[1], 16)

if (bus_status & 0x01) != 0 :
    print("bit 0 = controller busy: all other status bits invalid")

if (bus_status & 0x02) != 0 :
    print("bit 1 = error condition")

if (bus_status & 0x04) != 0 :
    print("bit 2 = slave address was not acknowledged during last")

if (bus_status & 0x08) != 0 :
    print("bit 3 = data not acknowledged during last operation")

if (bus_status & 0x10) != 0 :
    print("bit 4 = arbitration lost during last operation")

if (bus_status & 0x20) != 0 :
    print("bit 5 = controller idle")

if (bus_status & 0x40) != 0 :
    print("bit 6 = bus busy")
