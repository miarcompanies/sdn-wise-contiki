#!/usr/bin/env python
import time
import serial
import networkx as nx
print("Simple Python Controller for SDN-WISE Starting .....")
ser = serial.Serial('/dev/ttyUSB0')
time.sleep(50)
status = raw_input('Please enter your command - write Exit to quit\n')
while 1:
	ba = bytearray(status)
	ser.write(ba)
	if status == 'Exit':
		ser.close()
		break
	status = raw_input('Please enter your command - write Exit to quit\n')
