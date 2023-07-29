PWD = $(shell pwd)
KVER = $(shell uname -r)
KERNEL_SRC = /home/qlqcetc/nuc977bsp/02.linux_kernel

obj-m := platform_driver.o

all:
	make -C $(KERNEL_SRC) M=$(PWD)
	arm-linux-gcc ../led.c -o ledapp
	cp ledapp platform_driver.ko /mnt/d/
clean:
	rm -rf *.ko *.o *.symvers *.order .*.mod.c .*.o.cmd .*.ko.cmd *.mod.c
