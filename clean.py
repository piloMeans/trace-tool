#!/usr/bin/env python

import os
import sys
class record:
	def __init__(self):
		self.func_id=[]
		self.tstamp=[]
		self.proto=''
		self.src_ip=''
		self.dst_ip=''
		self.sport=''
		self.dport=''
		self.data=''
	def clear(self):
		self.func_id=[]
		self.tstamp=[]
		self.proto=''
		self.src_ip=''
		self.dst_ip=''
		self.sport=''
		self.dport=''
		self.data=''

def output(item, f):
	out_string=""
	out_string+=item.src_ip+"/"
	out_string+=item.dst_ip+"/"
	out_string+=item.proto+"/"
	out_string+=item.sport+"/"
	out_string+=item.dport+"/"
	out_string+=str(len(item.func_id))+"/"
	for i in item.func_id:
		out_string+=i+"/"
	for i in item.tstamp:
		out_string+=i+"/"
	out_string=out_string[:-1]+"\n"
	f.write(out_string)

def run(filename, outfile):

	s = open(filename,'r').read().split('\n')[:-1]

	addrhash=dict()
	
	f=open(outfile,'w')
	
	#count=0
	for i in s:
	#	count=count+1
	#	print count
		temp= i.split(' ')
	
		if temp[1]=='==':	#switch the key
			if not addrhash.has_key(temp[2]):
				addrhash[temp[2]]=record()
			addrhash[temp[2]].clear()
			for j in addrhash[temp[0]].func_id:
				addrhash[temp[2]].func_id.append(j)
			for j in addrhash[temp[0]].tstamp:
				addrhash[temp[2]].tstamp.append(j)
			addrhash[temp[2]].src_ip = addrhash[temp[0]].src_ip
			addrhash[temp[2]].dst_ip = addrhash[temp[0]].dst_ip
			addrhash[temp[2]].proto = addrhash[temp[0]].proto
			addrhash[temp[2]].sport = addrhash[temp[0]].sport
			addrhash[temp[2]].dport = addrhash[temp[0]].dport
			addrhash[temp[2]].data = addrhash[temp[0]].data
			addrhash[temp[0]].clear()
			continue
	
		skb = temp[SKB]
		key = temp[SKB_HEAD]
		data = temp[DATA]
		func_id = temp[FUNC_ID]
		tstamp = temp[TSTAMP]
		src_ip = temp[SADDR]
		dst_ip = temp[DADDR]
		proto = temp[PROTO]
		sport = temp[SPORT]
		dport = temp[DPORT]
	
		if addrhash.has_key(key):
	
			if func_table[int(func_id)][1]==4 or func_table[int(func_id)][1]==3:		# it's the end function, del the key
	
				# record into another value
				#print addrhash[key].proto
				addrhash[key].func_id.append(func_id)
				addrhash[key].tstamp.append(tstamp)
				if addrhash[key].proto!='':
					if  int(addrhash[key].proto) ==6  or int(addrhash[key].proto) ==17:
						output(addrhash[key], f)
				addrhash[key].clear()
				continue
			if addrhash[key].data!='' and data!=addrhash[key].data:
				if func_table[int(func_id)][1]!=1 and func_table[int(func_id)][1]!=2:
					last_id = addrhash[key].func_id.pop()
					last_tstamp=addrhash[key].tstamp.pop()
					if addrhash[key].proto!='':
						if int(addrhash[key].proto) ==6  or int(addrhash[key].proto) ==17:
							output(addrhash[key], f)
					addrhash[key].clear()
					addrhash[key].func_id.append(last_id)
					addrhash[key].tstamp.append(last_tstamp)
	
			addrhash[key].func_id.append(func_id)
			addrhash[key].tstamp.append(tstamp)
			if func_table[int(func_id)][1]!=1 and func_table[int(func_id)][1]!=2: 		
				addrhash[key].src_ip = src_ip
				addrhash[key].dst_ip = dst_ip
				addrhash[key].proto = proto
				addrhash[key].sport = sport
				addrhash[key].dport = dport
				addrhash[key].data = data
		else:
			# insert a key
			if func_table[int(func_id)][1]==1 or func_table[int(func_id)][1]==2 :
				item=record()
				addrhash[key]=item;
				addrhash[key].func_id.append(func_id)
				addrhash[key].tstamp.append(tstamp)
	
	f.close()

SKB=0
SKB_HEAD=1
DATA=2
SADDR=3
DADDR=4
PROTO=5
SPORT=6
DPORT=7
FUNC_ID=8
TSTAMP=9

func_table=[
	('skb_free_head',				4),
	('ip_rcv',						0),
	('__netif_receive_skb_core',	0),
	('ip_local_deliver',			3),
	('ip_local_out',				0),
	('ip_output',					0),
	('__dev_queue_xmit',			0),
	('napi_gro_receive',			1),
	('udp_send_skb',				2),
	('tcp_transmit_skb',			2),
	('br_handle_frame_finish',		0),
	('netif_receive_skb_internal',	0),
	('ovs_vport_receive',			0),
	('ovs_execute_actions',			0),
	('e100_xmit_frame',				3),
	('ixgbe_xmit_frame',			3),
	('ip_rcv_finish',				0),
	('ip_forward',					0),
	('ip_forward_finish',			0),
	('ixgbevf_xmit_frame',			3),
]
	
if __name__ == '__main__':
	filename = sys.argv[1]
	outfile = sys.argv[2]
	run(filename, outfile)
