PWD = $(shell pwd)
KVER = $(shell uname -r)
KERNEL_SRC = /home/qlqcetc/nuc977bsp/02.linux_kernel

obj-m := led_driver.o

all:
	make -C $(KERNEL_SRC) M=$(PWD)
	arm-linux-gcc led.c -o led
	cp led led_driver.ko /mnt/d/
clean:
	rm -rf led .built-in.* .led_driver.* *.ko *.ko.cmd *.mod.o.cmd *.o.cmd *.cmd *.o *.mod.c *.order *.symvers .tmp_versions
