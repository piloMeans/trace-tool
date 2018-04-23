#!/bin/bash

trace_file=$1
trace_temp=$2
output_file=$3
port=$4
final_result=$5
sed -i '/CPU/d' ${trace_file}
sed 's/^.*:[[:space:]]*//' ${trace_file} > ${trace_temp}
#awk '{for(i=6;i<=15;i++){printf $i""FS} print ""}' ${trace_file} > ${trace_temp}


./run.py ${trace_temp} ${output_file}

./clean.py -f ${output_file} -p $port >  ${final_result}
