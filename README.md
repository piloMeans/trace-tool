### Environment

- kernel linux 4.14
- ftrace open 

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

### Q&A

- why no output

Situation 1, you just shutdown the ftrace, you should keep the ftrace open. You can do this by 
setting `/proc/sys/kernel/ftrace_enabled` and `/sys/kernel/debug/tracing/tracing_on` to 1

Situation 2, you set a too low sample ratio or your load program have too less network communication. 


