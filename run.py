#!/usr/bin/env python
import argparse
import os
import sys
import time
import clean
import statistic
import common
import re

def start(args):
	ratio = int(args.ratio*1000000000)
	out,err = common.execute('cat /proc/cpuinfo | grep processor | wc -l')
	core = int(out)

	os.chdir('./module')
	f = open('./write_3.c','w')
	template = open('./write_template.c_'+str(args.type)).read()

	with open('/proc/kallsyms','r') as allsymsf:
		allsyms=allsymsf.read()
	
	for i in range(len(functable)):
		pattern=r'\b'+functable[i][FUNCNAME]+r'\.(isra|constprop)\.[0-9]+\b'
		result=re.search(pattern, allsyms)
		
		if result!=None:
			temp=list(functable[i])
			temp[FUNCNAME]=result.group(0)
			functable[i]=tuple(temp)

	debug=0
	if args.all:
		debug=1
	func_str_template='{0,0,0,SKB_IDX,SK_IDX,TYPE,"FUNCNAME"},\n'
	ft_string=''
	sp_ft_string=''
	for i in functable:
		ft_string=ft_string+ func_str_template.replace('SKB_IDX',str(i[SKB_IDX])).\
				replace('SK_IDX',str(i[SK_IDX])).replace('TYPE',str(i[TYPE])).\
				replace('FUNCNAME',i[FUNCNAME])
	for i in spec_functable:
		sp_ft_string=sp_ft_string+ func_str_template.replace('SKB_IDX',str(i[SKB_IDX])).\
				replace('SK_IDX',str(i[SK_IDX])).replace('TYPE',str(i[TYPE])).\
				replace('FUNCNAME',i[FUNCNAME])
	ft_string=ft_string.strip('\n').strip(',')
	sp_ft_string=sp_ft_string.strip('\n').strip(',')

	template = template.replace('cpu_num', str(core)).replace('sample_ratio', str(ratio)).\
			replace('spec_func_table_size',str(len(spec_functable))).\
			replace('SPEC_FUNCTABLE',sp_ft_string).\
			replace('func_table_size',str(len(functable))).replace('FUNCTABLE',ft_string).\
			replace('debug',str(debug))

	f.write(template)
	f.close()
	# os.system('make')
	# os.system('sudo insmod write.ko')
        if args.fake:
            print("already generate the file")
            sys.exit()
	out,err=common.execute('''
	make
	sudo insmod write.ko
	''')
	if out!='' or err!='':
		print out
		print err

def stop(args):

	out,err = common.execute('''
	cd ./module
	sudo rmmod write
	make clean
	rm ./write_3.c
	''')
	if out!='' or err!='':
		print out
		print err

def analysis(args):
	filename = str(time.time())
	lasttime= args.time
	port = args.port

	out,err = common.execute('''
	grep -i '#define debug' module/write_3.c | cut -d ' ' -f3
	''')
	debug=0
	if int(out)==1:
		debug=1

	out,err = common.execute('''
	mkdir -p trace/
	cd trace
	sudo cat /sys/kernel/debug/tracing/trace_pipe > /dev/null &
	sleep 2
	sudo kill `pgrep -x cat`
	sudo cat /sys/kernel/debug/tracing/trace_pipe > FILENAME &
	sleep LASTTIME
	sudo kill `pgrep -x cat`
	'''.replace('FILENAME', filename).replace('LASTTIME',str(lasttime)))

	if out!='' or err!='':
		print out
		print err

	if not debug:
		os.chdir('./trace')
		out,err = common.execute('''
		sed -i '/CPU/d' FILENAME
		sed -i 's/^.*:[[:space:]]//' FILENAME
		'''.replace('FILENAME',filename))
		if out!='' or err!='':
			print out
			print err
	# os.system('sudo cat /sys/kernel/debug/tracing/trace_pipe > /dev/null &')
	# time.sleep(2)
	# os.system('sudo kill `pgrep -x cat`')

	# os.system('sudo cat /sys/kernel/debug/tracing/trace_pipe > ' + filename +' &')
	# time.sleep(lasttime)
	# os.system('sudo kill `pgrep -x cat`')

	# os.system("sed -i '/CPU/d' "+filename)
	# os.system("sed -i 's/^.*:[[:space:]]*//' " +filename)
		clean.run(filename, 'temp', functable)
		statistic.run('temp', port, functable, vague=args.vague)

if __name__=='__main__':
	parser = argparse.ArgumentParser()
	subcmd = parser.add_subparsers()

	parser_start = subcmd.add_parser('start', help = 'Insert the module')
	parser_start.add_argument('-a',dest='all',help='output all without sampling', default=False, action='store_true')
	parser_start.add_argument('-t',dest='type',help='type of template', required=True, type=int)
	parser_start.add_argument('-s', type=float, dest='ratio', default=0.1, help='sample ratio, range is [0-1]')
	parser_start.add_argument('--fake', dest='fake',default=False,action='store_true', help='Only generate the target file, without running.')
	parser_start.set_defaults(func = start)

	parser_stop = subcmd.add_parser('stop', help = 'Remove the module')
	parser_stop.set_defaults(func = stop)

	parser_analysis = subcmd.add_parser('deal', help= 'Get the trace and do statistics work')
	parser_analysis.add_argument('-t', type=int , dest='time', required='True',help='the sampling time (second)')
	parser_analysis.add_argument('-p', type=int , dest='port', required='True',help='the port to be analysis (src/dst)')
	parser_analysis.add_argument('-v', dest='vague', default=False, help='statistic in vague mode', action='store_true')
	#parser_analysis.add_argument('-o', type=str , dest='output',help='output the result to file')

	parser_analysis.set_defaults(func=analysis)

	args=parser.parse_args()

	TYPE=1
	SKB_IDX=2
	SK_IDX=3
	FUNCNAME=0
	functable,spec_functable = common.parse('config.json')

	args.func(args)


