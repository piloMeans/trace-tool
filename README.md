### Environment

- Ubuntu 16.04 (kernel version 4.19)
- ftrace open 

### prerequisite

- kernel 

current kernel version is 4.19 (selfbuild)
if you want to use in other version,just do some porting job. 

and kernel-headers is needed.


- python numpy library

NumPy is the fundamental package for scientific computing with Python.

```bash
# install pip tool in you server
apt-get install python-pip

# install numpy library 
pip install numpy
```


### How to use

- run the python script `run.py` 

Following is a example

```bash

# insert the module, set the sample ratio 0.01
./run.py start -s 0.01

# collect the data for 20 second, and analysis the data . (the port we interest is 11211) 
./run.py deal -t 20 -p 11211

# remove the module
./run.py stop							
```

### Something

- why no output

Situation 1, you just shutdown the ftrace, you should keep the ftrace open. You can do this by 
setting `/proc/sys/kernel/ftrace_enabled` and `/sys/kernel/debug/tracing/tracing_on` to 1

Situation 2, you set a too low sample ratio or your load program have too less network communication. 

- NOT support probe reenter

You __`should not` kprobe/ftrace the same function__ in this tool, or something weird may happen!!(maybe you 
need to reboot to set everything ok)
