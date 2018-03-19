#CONFIG_MODULE_FORCE_UNLOAD=y
#override EXTRA_CFLAGS+= -g -O0


#CONFIG_MODULES_SIG=n

obj-m+=write.o
obj-m+=function.o


all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean

