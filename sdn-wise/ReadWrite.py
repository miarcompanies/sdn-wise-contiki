#!/usr/bin/env python
import sys
import time
import serial
import networkx as nx
import threading
from threading import Thread
import networkx as nx
def readUART(Topo):
	try:
		ser = serial.Serial('/dev/ttyUSB0',115200)
	        time.sleep(30)
		prev_length = 0	
		print 'Reading  ...'
		while 1:
			time.sleep(3)
			mtype = ser.readline()
			if 'ReportTopo' in mtype:
				topo = ser.readline()
				print 'Topo:'+topo
				topoarray = map(int, topo.split(","))
				print "Topo in Array:"
				print topoarray
				length = len(topoarray)
				print length
				if length != prev_length:
					Topo.clear()
					prev_length = length
				Topo.add_node(topoarray[0])
				for num in range(2,length-2,3): 
					Topo.add_node(topoarray[num])
					Topo.add_edge(topoarray[0], topoarray[num],weight=topoarray[num+2])
				print Topo.nodes()
				print Topo.edges(data=True)
				#for (u,v,d) in Topo.edges(data=True):
				#	print d['weight']	
			elif 'Request' in mtype:
				req = ser.readline()
                                print 'Request:'+req
                                reqarray = map(int, req.split(","))
                                print "Request in Array:"
                                print reqarray
				print 'Shortest Path from %d to %d: ' % (reqarray[0], reqarray[3])
				try:
					shortpath =  nx.dijkstra_path(Topo,reqarray[0],reqarray[3],weight=True)
					print shortpath
					for x in range(len(shortpath)-1):
						unicastcommand = str(shortpath[x])
						unicastcommand += str(shortpath[x])
						unicastcommand += 'u'
						unicastcommand += str(shortpath[x+1])
						unicastcommand += str(shortpath[x+1])
						unicastcommand += str(shortpath[x+1]) 
						print "Unicast Command to send: "+unicastcommand
						bauni = bytearray(unicastcommand)
                        			ser.write(bauni)
				except Exception:
					broadcastcommand = str(reqarray[0])
					broadcastcommand += str(reqarray[0])
					broadcastcommand += 'b'
					broadcastcommand += str(reqarray[3])
					broadcastcommand += str(reqarray[3]) 
					broadcastcommand += str(reqarray[3])
					print "Broadcast Command to send: "+broadcastcommand
					babro = bytearray(broadcastcommand)
                                        ser.write(babro)
        				print "Node %d not reachable from %d" % (reqarray[3],reqarray[0])
			else:
				print mtype
	except (KeyboardInterrupt):
		sys.exit()
def writeUART(Topo):
	try:
	        ser = serial.Serial('/dev/ttyUSB0',115200)
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
	Topo = nx.Graph() 
	#Topo.add_node(1.0)
	threadwrite = threading.Thread(target = writeUART, args = [Topo])
	threadwrite.Daemon = True
	threadwrite.start()
	threadread = threading.Thread(target = readUART, args = [Topo])
	threadread.Daemon = True
	threadread.start()
