#!/usr/bin/env python
import sys
import time
import serial
import networkx as nx
import threading
from threading import Thread
from datetime import datetime
import networkx as nx
def readUART(Topo):
	try:
		ser = serial.Serial('/dev/ttyUSB1',115200)
	        #time.sleep(10)
		prev_length = []
		length = []
		for t in range(10):
			prev_length.append(0)
			length.append(0)	
		while 1:
			#time.sleep(3)
			mtype = ser.readline()
			if 'Report' in mtype:
				topo = ser.readline()
				print 'Topo:'+topo
				topoarray = map(int, topo.split(","))
				#print datetime.utcnow().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
				#print datetime.now().strftime('%H:%M:%S.%f')[:-3]
				print datetime.now().strftime('%H:%M:%S.%f')
				print "Topo in Array:"
				print topoarray
				for s in range(10): #10 nodes assumed
					if topoarray[0] == s+1:
						length[s] = len(topoarray)
						print length[s]
						if length[s] != prev_length[s]:#topology changes
							if topoarray[0] == 3: #will be 2
								resetcommand = str(2)
	                                                	resetcommand += str(2)
		                                                resetcommand += 'r'
        		                        	        resetcommand += 'f'
                		        	                resetcommand += str(2)
                			                        resetcommand += str('\n')
        	                		                print "Reset Command to send: "+resetcommand
	                                		        bauni = bytearray(resetcommand)
								ser.write(bauni)
								print "Reset Command Sent at "
                                                		print datetime.now().strftime('%H:%M:%S.%f')
							Topo.clear()
						prev_length[s] = length[s]
				Topo.add_node(topoarray[0])
				for num in range(2,len(topoarray)-2,3): 
					Topo.add_node(topoarray[num])
					Topo.add_edge(topoarray[0], topoarray[num],weight=topoarray[num+2])
					Topo.add_edge(topoarray[num], topoarray[0],weight=topoarray[num+2])
				print Topo.nodes()
				print Topo.edges(data=True)
				#for (u,v,d) in Topo.edges(data=True):
				#	print d['weight']	
			elif 'Request' in mtype:
				print datetime.now().strftime('%H:%M:%S.%f')	
				req = ser.readline()
                                print 'Request:'+req
                                reqarray = map(int, req.split(","))
                                print "Request in Array:"
                                print reqarray
				print 'Shortest Path from %d to %d: ' % (reqarray[0], reqarray[2])
				try:
					shortpath =  nx.dijkstra_path(Topo,reqarray[0],reqarray[2],weight=True)
					print shortpath
					#if (reqarray[0] == 1 and (len(shortpath) > 2)):
					if (len(shortpath) > 2):
						nxh = shortpath[1]
						for x in range(len(shortpath)-1):
							unicastcommand = str(shortpath[0]-1)
							unicastcommand += str(shortpath[0]-1)
							unicastcommand += 'u'
							unicastcommand += str(shortpath[x+1]-1)
							unicastcommand += str(nxh-1)
							unicastcommand += str('\n') 
							print "Unicast Command to send: "+unicastcommand
							bauni = bytearray(unicastcommand)
				                        ser.write(bauni)
							print datetime.now().strftime('%H:%M:%S.%f')
							print "Command written to serial port"
							time.sleep(2)
					else:
						unicastcommand = str(shortpath[0]-1)
						unicastcommand += str(shortpath[0]-1)
						unicastcommand += 'u'
						unicastcommand += str(shortpath[1]-1)
						unicastcommand += str(shortpath[1]-1)
						unicastcommand += str('\n')
						print "Unicast Command to send: "+unicastcommand
						bauni = bytearray(unicastcommand)
						ser.write(bauni)
						print datetime.now().strftime('%H:%M:%S.%f')
				#		for x in range(len(shortpath)-1):
				#			unicastcommand = str(shortpath[x]-1)
				#			unicastcommand += str(shortpath[x]-1)
				#			unicastcommand += 'u'
				#			unicastcommand += str(shortpath[x+1]-1)
				#			unicastcommand += str(shortpath[x+1]-1)
				#			unicastcommand += str('\n') 
				#			print "Unicast Command to send: "+unicastcommand
				#			bauni = bytearray(unicastcommand)
			              #                  ser.write(bauni)
				except Exception:
					#dropcommand = str(reqarray[0]-1)
					#dropcommand += str(reqarray[0]-1)
					#dropcommand += 'd'
					#dropcommand += str(reqarray[2]-1)
					#dropcommand += str(reqarray[2]-1) 
					#dropcommand += str('\n')
					#print "Drop Packet Command to send: "+dropcommand
					#babro = bytearray(dropcommand)
                                        #ser.write(babro)
        				print "Node %d not reachable from %d" % (reqarray[2],reqarray[0])
			else:
				print mtype
	except (KeyboardInterrupt):
		sys.exit()
def writeUART(Topo):
	try:
	        ser = serial.Serial('/dev/ttyUSB1',115200)
	        #time.sleep(10)
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
	print datetime.now().strftime('%H:%M:%S.%f')
	print("Simple Python Controller for SDN-WISE Starting .....")	
	Topo = nx.DiGraph()
	threadwrite = threading.Thread(target = writeUART, args = [Topo])
	threadwrite.Daemon = True
	threadwrite.start()
	threadread = threading.Thread(target = readUART, args = [Topo])
	threadread.Daemon = True
	threadread.start()

