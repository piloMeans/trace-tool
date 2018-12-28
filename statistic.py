#!/usr/bin/env python


import os
import numpy
import math
import sys
import argparse


class stastic_tuple:
	def __init__(self):
		self.avg = []
		self.p50 = []
		self.count=0



def endian(before,length):
    after=0
    for i in range(0,length):
        after = after << 8
        after = after | ( before & 0xff)
        before = before >>  8
    return after  
def ntohl(before):
    return endian(before,4)
def ntohs(before):
    return endian(before,2)
def int2ip(before):
	res=''
	for i in range(4):
		res = str(before %256) + '.' +res	
		before = before / 256
	return res[:-1]

def sub(a, b):
	mid = len(a)-8
	if mid >0 :
		high_a = a[0:mid]
		low_a = a[mid:]
	else:
		high_a = '0'
		low_a = a
	mid = len(b)-8
	if mid>0:
		high_b = b[0:mid]
		low_b = b[mid:]
	else:
		hihg_b = '0'
		low_b = b
	
	res = (int(high_a,16) - int(high_b, 16)) * 1000000000 + int(low_a, 16) - int(low_b,16)
	return res


def stastic(index, s):
	data =[]
	for i in index:
		temp=s[i].split('/')
		start = int(temp[5])+6
		tempdata=[]
		for j in range(start, start+int(temp[5])-1):
			tempdata.append( sub(temp[j+1], temp[j]))
		data.append(tempdata)

	res=stastic_tuple()
	res.count = len(data)
	data=numpy.array(data)
	res.avg = numpy.around(numpy.mean(data,0),6).tolist()
	res.p50 = numpy.around(numpy.percentile(data, 50, axis=0),6).tolist()
	return res

def output(sample, stastic_res, functable):
	temp = sample.split('/')
	Func="Func\t"
	for i in range(6, 6+int(temp[5])):
		Func+=functable[int(temp[i])][0]+"\t";
	print("%s\n" % Func)
	print("%d\n" % stastic_res.count)	
	stastic_avg="AVG\t"
	stastic_p50="P50\t"
	#print stastic_res
	for i in stastic_res.avg:
		stastic_avg += str(i)+"\t"
	for i in stastic_res.p50:
		stastic_p50 += str(i)+"\t"
	print("%s\n" % stastic_avg)
	print("%s\n" % stastic_p50)
			
def func(ttuple, index, s, functable):
	path=dict()
	
	for i in index:
		temp = s[i].split('/')
		length = int(temp[5])
#		if length > 20:
#			continue
		key=[]
		for j in range(6, 6+length):
			key.append(int(temp[j]))
		key = tuple(key)
		if path.has_key(key):
			path[key].append(i)
		else:
			path[key]=[i]
		
	print('src %s:%d dst %s:%d proto %d\n' %(int2ip(ntohl(ttuple[0])), ntohs(ttuple[3]), int2ip(ntohl(ttuple[1])), ntohs(ttuple[4]), ttuple[2]))
			
	for i in path:
		temp_res = stastic(path[i], s)
		output(s[path[i][0]], temp_res, functable)
	print(100*"=")

def	run(filename, port, functable):

	# seperate based on the 5-tuple
	tuple_5 = dict()
	s= open(filename, 'r').read().split('\n')[:-1]
	for i in s:
		temp = i.split('/')
		if int(temp[3]) != ntohs(port) and int(temp[4]) != ntohs(port):
			continue
		tuple_temp = (int(temp[0], 16),int(temp[1], 16), int(temp[2]),int(temp[3]),int(temp[4]))
		if tuple_5.has_key(tuple_temp):
			tuple_5[tuple_temp].append(s.index(i))
		else:
			tuple_5[tuple_temp]=[s.index(i)]
	for i in tuple_5:
		func(i, tuple_5[i], s, functable)

# functable={
# 	0: "skb_free_head",
# 	1: "ip_rcv",
# 	2: "__netif_receive_skb_core",
# 	3: "ip_local_deliver",
# 	4: "ip_local_out",
# 	5: "ip_output",
# 	6: "__dev_queue_xmit",
# 	7: "napi_gro_receive",
# 	8: "udp_send_skb",
# 	9: "tcp_transmit_skb",
# 	10:"br_handle_frame_finish",
# 	11:"netif_receive_skb_internal",
# 	12:"ovs_vport_receive",
# 	13:"ovs_execute_actions",
# 	14:"e1000_xmit_frame",
# 	15:"ixgbe_xmit_frame",
# 	16:"ip_rcv_finish",
# 	17:"ip_forward",
# 	18:"ip_forward_finish",
# 	19:"ixgbevf_xmit_frame"
#    }
	
if __name__ == '__main__':

	parser = argparse.ArgumentParser('visual the result')
	parser.add_argument('-f','--file', dest='filename', help='file name of data', required=True)
	parser.add_argument('-p','--port', dest='port', type=int, help='port of src/dst', required=True)
	args = parser.parse_args()


	filename = args.filename
	port = args.port

	functable , spec_functable = common.parse('config.json')
	run(filename, port, functable)
