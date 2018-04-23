#!/usr/bin/python

import argparse
import os
import time
import clean
import statistic

def start(args):
	ratio = int(args.ratio*1000000000)
	core = args.core
	os.chdir('./module')
	f = open('./write_3.c','w')
	template = open('./write_template.c').read()
	template = template.replace('cpu_num', str(core)).replace('sample_ratio', str(ratio))
	f.write(template)
	f.close()
	os.system('make')
	os.system('sudo insmod write.ko')

def stop(args):
	os.chdir('./module')
	os.system('sudo rmmod write')
	os.system('make clean')
	os.system('rm ./write_3.c')	

def analysis(args):
	os.chdir('./trace')
	filename = str(time.time())
	lasttime= args.time
	port = args.port
	os.system('sudo cat /sys/kernel/debug/tracing/trace_pipe > /dev/null &')
	time.sleep(2)
	os.system('sudo kill `pgrep -x cat`')

	os.system('sudo cat /sys/kernel/debug/tracing/trace_pipe > ' + filename +' &')
	time.sleep(lasttime)
	os.system('sudo kill `pgrep -x cat`')

	os.system("sed -i '/CPU/d' "+filename)
	os.system("sed -i 's/^.*:[[:space:]]*//' " +filename)
	clean.run(filename, 'temp')
	statistic.run('temp', port)


parser = argparse.ArgumentParser()
subcmd = parser.add_subparsers()

parser_start = subcmd.add_parser('start', help = 'Insert the module')
parser_start.add_argument('-s', type=float, dest='ratio',required='True', help='sample ratio, range is [0-1]')
parser_start.add_argument('-c', type=int, dest='core',required='True', help='core number of current machine.')
parser_start.set_defaults(func = start)

parser_stop = subcmd.add_parser('stop', help = 'Remove the module')
parser_stop.set_defaults(func = stop)

parser_analysis = subcmd.add_parser('deal', help= 'Get the trace and do statistics work')
parser_analysis.add_argument('-t', type=int , dest='time', required='True',help='the sampling time (second)')
parser_analysis.add_argument('-p', type=int , dest='port', required='True',help='the port to be analysis (src/dst)')
#parser_analysis.add_argument('-o', type=str , dest='output',help='output the result to file')

parser_analysis.set_defaults(func=analysis)

args=parser.parse_args()
args.func(args)


