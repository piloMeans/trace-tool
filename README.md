### Environment

- kernel linux 4.14
- ftrace open 

### How to use

- open the ftrace.

set `/proc/sys/kernel/ftrace_enabled` and `/sys/kernel/debug/tracing/tracing_on` to 1, so that
we can use ftrace mcount.

- build the module and load

```bash

# you shold modify the macro CPU_NUM in write_3.c first !!!!!

make
#sudo insmod function.ko  # please do not use function.ko
#sudo insmod write.ko

sudo insmod write.ko
```

- modify the function 

Modify the `testfunction_1` in ~~`function.c`~~ `write_3.c`, you can do something youself want. 
However, DO NOT modify the ~~embedded asm~~ assemble code in `pre_handler.S` and `ret_handler.S`,
as they are use to save and restore the regs before your function. Without them, the kernel 
will crash after you insert the `write.ko`.

You can change the insert point as you change the `udp_send_skb` in write\_3.c to some other
function (Remember that the function name should can be find in the `/proc/kallsyms` file)


- view result

If you use `trace_printk`, you can see the output in `sys/kernel/debug/tracing/trace_pipe`.
If you use `printk`, you can see the output int `var/log/message` or dmesg.
You can also do the file operation in the function.
