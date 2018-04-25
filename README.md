### Environment

- kernel linux 4.14
- ftrace open 

### prerequisite

- kernel-headers

```bash
#Find the kernel-headers of the corresponding version and install it

apt-cache search linux-headers
apt-get install linux-headers-$(uname -r)
```
If no search the version you need in the link above, just compile it. Good luck....


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

# insert the module, set the sample ratio 0.001, and the core number of the server is 16
./run.py start -s 0.001 -c 16		

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
need reboot to set everything ok  > <)
