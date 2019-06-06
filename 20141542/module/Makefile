obj-m = stopwatch_driver.o
CC := arm-none-linux-gnueabi-gcc


all : driver

KDIR := /work/achroimx_kernel
PWD := $(shell pwd)


driver :
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean :
	rm -rf *.o
	rm -rf *.ko
	rm -rf *.mod.*
	rm -rf *.order
	rm -rf *.symvers
	rm -rf .tmp*
	rm -rf .stopwatch*
