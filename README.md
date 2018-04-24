### Environment

- kernel linux 4.14
- ftrace open 

### prerequisite

- kernel-headers

If you can find the corresponding version of the kernel-headers through 
`yum list | grep kernel-headers`, you should use yum to install it. Just
run the command `yum install kernel-headers-xxx`

If no search in the yum list, you can download the corresponding rpm from
this [link](https://pkgs.org/download/kernel-headers),and run the command `rpm -ivh xxxx.rpm` to install it.

If still no search the version you need in the link above, just compile it. Good luck....


- python numpy library

NumPy is the fundamental package for scientific computing with Python.
run command `pip install numpy` to install it



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
