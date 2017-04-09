#!/usr/bin/env python
import sys
import time
import serial
import networkx as nx
import threading
from threading import Thread

def readUART():
	try:
		ser = serial.Serial('/dev/ttyUSB1',115200)
	        time.sleep(30)
		print 'Reading  ...'
		while 1:
			time.sleep(3)
			mtype = ser.readline()
			if 'ReportTopo' in mtype:
				print 'Topo:'+ser.readline()
			else:
				print mtype
	except (KeyboardInterrupt):
		sys.exit()
def writeUART():
	try:
	        ser = serial.Serial('/dev/ttyUSB1',115200)
	        time.sleep(30)
		#status = raw_input('Please enter your command - write Exit to quit\n')
		print 'Please enter your command - write Exit to quit\n'
		status = sys.stdin.readline() 
		while 1:
        		ba = bytearray(status)
	        	ser.write(ba)
	        	if status == 'Exit':
        	        	ser.close()
               			sys.exit()
				break
		        #status = raw_input('Please enter your command - write Exit to quit\n')
			print 'Please enter your command - write Exit to quit\n'
	  		status = sys.stdin.readline()
	except (KeyboardInterrupt):
                sys.exit()
if __name__=='__main__':
	print("Simple Python Controller for SDN-WISE Starting .....")
	#ser = serial.Serial('/dev/ttyUSB0')
	#time.sleep(30)
	threadwrite = threading.Thread(target = writeUART)
	threadwrite.Daemon = True
	threadwrite.start()
	threadread = threading.Thread(target = readUART)
	threadread.Daemon = True
	threadread.start()
