#!/usr/bin/env python
import time
import serial
from datetime import datetime
print("Python Data Packet Generator")
ser = serial.Serial('/dev/ttyUSB0')
#time.sleep(10)
for x in range(1000):
	sendcommand = 'd'
	sendcommand += str(1)
	sendcommand += str(1)
	sendcommand += str(3)
	sendcommand += str(3)
	sendcommand += str('\n')
	#print "Data Pkt: " + x
	#print datetime.now().strftime('%H:%M:%S.%f')
	bauni = bytearray(sendcommand)
	ser.write(bauni)
	#print "sendcommand written to serial port"
	time.sleep(5)
