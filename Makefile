obj-m := hello_driver.o

# Path to the Linux kernel source code
KERNEL_SOURCE := /lib/modules/$(shell uname -r)/build

# Rules for building the module
all:
	make -C $(KERNEL_SOURCE) M=$(PWD) modules

clean:
	make -C $(KERNEL_SOURCE) M=$(PWD) clean
