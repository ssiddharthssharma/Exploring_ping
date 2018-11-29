from scapy.all import *
import os

for x in range(0,3):
	packet = IP(dst="8.8.8.8", ttl=20)/ICMP()/str(os.getpid())
	packet.show()
	reply = sr1(packet)
	print(reply.show())