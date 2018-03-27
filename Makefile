#CONFIG_MODULE_FORCE_UNLOAD=y
#override EXTRA_CFLAGS+= -g -O0


#CONFIG_MODULES_SIG=n

#obj-m+=write.o
#obj-m+=write_2.o
#obj-m+=function_a.o
#obj-m+=myjprobe_5.o
#function_a-m= function.o helper.o

obj-m+=write.o
write-m= write_3.o ret_helper.o pre_helper.o


all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean

