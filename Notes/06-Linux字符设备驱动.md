# 字符设备驱动开发 #  
## 字符设备驱动简介 ##

字符设备是 Linux 驱动中最基本的一类设备驱动，字符设备就是一个一个字节，按照字节流进行读写操作的设备，读写数据是分先后顺序的。比如我们最常见的点灯、按键、IIC、SPI，LCD 等等都是字符设备，这些设备的驱动就叫做字符设备驱动。

在 Linux 中一切皆为文件，驱动加载成功以后会在“/dev”目录下生成一个相应的文件，应用程序通过对这个名为“/dev/xxx”(xxx 是具体的驱动文件名字)的文件进行相应的操作即可实现对硬件的操作。

应用程序运行在用户空间，而 Linux 驱动属于内核的一部分，因此驱动运行于内核空间。当我们在用户空间想要实现对内核的操作，比如使用 open 函数打开/dev/led 这个驱动，因为用户空间不能直接对内核进行操作，因此必须使用一个叫做“系统调用”的方法来实现从用户空间“陷入”到内核空间，这样才能实现对底层驱动的操作。

这一流程的实现是一层一层的系统调用实现的：
ARM接口编程阶段写的驱动代码无法直接放入linux内核进行编译，而应该作为模块进行插入。  
![Driver Process](https://s2.loli.net/2023/07/20/9UwhtxWe3Du4HFg.png)  

**为什么学习了ARM接口编程，还需要学习基于Linux的驱动开发？**  
- ARM接口编程实现的即是裸机环境下硬件的驱动代码(如通过BSP或读写寄存器实现对各式硬件的控制)，用户直接通过编程与硬件进行交互。  
- 基于Linux的驱动开发中由于操作系统的存在用户无法直接与硬件进行交互，则需要通过系统调用进入`kernel mode`调用已经实现的驱动完成对硬件映射的地址进行读写操作完成与硬件的交互。  

裸机环境下的代码，往往是由一个`while(1)`循环，将所有的功能串行执行，效率低下。而基于Linux的驱动开发提供了抽象接口给用户，使得用户不用过多关心底层实现提高了开发效率，同时可使用并发编程、各种软件生态等特性来提高性能。  
**微内核与宏内核**  
宏内核，大量的操作都集成在内核中，由操作系统进行整体的管理。而微内核则是将`kernel`的内容最小化，维持一个OS的基本运行，当外部拓展模块发生异常时并不会引发`kernel`的crash。  

**编译模块之前应当先完成对内核的编译**  

## Linux 字符驱动形式 ##  
Linux 字符驱动有两种运行方式
- 将驱动编译进 Linux 内核中，这样当 Linux 内核启动的时候就会自动运行驱动程序。  
- 将驱动编译成模块(Linux 下模块扩展名为.ko)，在Linux 内核启动以后使用“insmod”命令加载驱动模块`*.ko`。在调试驱动的时候一般都选择将其编译为模块，这样我们修改驱动以后只需要编译一下驱动代码即可，不需要编译整个 Linux 代码。  

由于操作系统的存在，用户空间无法直接读写物理地址，所以采用了模块将设备物理地址映射到内存进行操作以实现驱动。  
- 如在内核中使用`ioremap`将设备物理地址映射至虚拟地址，再通过`cdev`结构体的`ops`结构体成员中的各式`ioctl` `read` `write` 接口实现用户空间对设备的访问和控制。  
- 或是在用户空间使用`mmap`函数间接调用`ops`中的`mmap`成员函数将设备物理地址所对应的虚拟地址直接传递到用户层供用户层直接对地址进行操作。**(这种方法一般应用于大批量数据传递且传递速度要求较快的设备，如LCD、Camera等，即可直接在用户层对设备进行存取操作而避免频繁地进出内核。)**  

模块中实现的驱动函数通过导出符号最多也是只有其它模块能够调用其函数，应用层仍然无法使用，如实现了`led_on()`在应用层无法直接调用，故使用C语言回调特性将函数地址暴露给用户层(不使用函数名，而是直接使用函数地址进行调用)。linux则将字符设备区分为主设备号及次设备号，每一个设备由一个`struct`形成，内部存有其驱动的中各函数地址，再以链表的形式将同主设备号的设备结构体串联起来，不同主设备号的设备则以数组形式存储，整体结构类似于哈希表的存储方式。**(Linux内核具体实现不一定如此，只是便于理解这样描述)** 但以主次设备号来调用驱动仍显麻烦，按照 **Linux 一切皆文件** 的思想则可以将主次设备对应的字符设备抽象为文件，通过对字符设备文件进行操作相应地回调驱动中所注册的各式驱动函数实现了驱动的执行。    
  
**APP open 字符设备文件 => 根据主次设备号找到对应设备对应的cdev结构体 => 根据cdev结构体的ops成员调用注册为open操作的函数led_open**   

**完成设备驱动步骤**：  
1. 申请设备号  
```  
// 申请设备号
int alloc_chrdev_region(dev_t *dev, unsigned baseminor, unsigned count, 
                        const char *name);  
// 注销设备号
void unregister_chrdev_region(dev_t from, unsigned count);
```  
- dev：传入的参数，保存申请到的设备号

- baseminor：次设备号起始地址，一般为0开始

- count：要申请的设备号数量

- name：设备名字

- from：要释放的设备号

- count：表示从 from 开始，要释放的设备号数量  

2. 完成设备驱动，将实现的驱动函数封装进结构体  
```
static struct file_operations test_fops = {
  .owner = THIS_MODULE,
  .open = chrtest_open,
  .read = chrtest_read,
  .write = chrtest_write,
  .release = chrtest_release,
};  
```  
还有更多的操作可以注册，详见文档  

3. 注册设备驱动   
```
// 注册字符设备  
static inline int register_chrdev(unsigned int major, const char *name,  
const struct file_operations *fops);    
// 注销字符设备  
static inline void unregister_chrdev(unsigned int major, const char *name);  
```
### 设备号分配 ###

**静态分配设备号**：这个设备号可以是驱动开发者静态的指定一个设备号，比如选择 200 这个主设备号。有一些常用的设备号已经被 Linux 内核开发者给分配掉了，具体分配的内容可以查看文档 Documentation/devices.txt。并不是说内核开发者已经分配掉的主设备号我们就不能用了，具体能不能用还得看我们的硬件平台运行过程中有没有使用这个主设备号，使用`cat /proc/devices`命令即可查看当前系统中所有已经使用了的设备号。可以使用`MKDEV()`指定设备号。

**动态分配设备号**：Linux 社区推荐使用动态分配设备号，在注册字符设备之前先申请一个设备号，系统会自动给你一个没有被使用的设备号，这样就避免了冲突。卸载驱动的时候释放掉这个设备号即可。  

Linux中，将设备抽象为了设备文件，则可以通过系统调用去访问设备文件，间接获取到设备的主次设备号以致于查找到对应的结构体以调用相应的驱动函数。  

## Sample: Hello ##
文件需求
- hello.c
```
	#include <linux/module.h>
	#include <linux/kernel.h>
	#include <linux/version.h>
	#include <asm/io.h>

    MODULE_LICENSE("Dual BSD/GPL");
    MODULE_AUTHOR("cocoson");
    MODULE_DESCRIPTION("A sample Hello Module");

    // 模块构造函数
    // 插入模块时调用该函数
    static int hello_init(void)
    {
    printk(KERN_ALERT "Hello, world\n");
    return 0;
    }

    // 模块析构函数
    // 卸载模块时调用该函数
    static void hello_exit(void)
    {
    printk(KERN_ALERT "Goodbye, cruel world\n");
    }

    // 将函数hello_init设置为模块加载函数
    module_init(hello_init);
    // 将函数hello_exit设置为模块删除函数
    module_exit(hello_exit);  
```
- Makefile  
  - `obj-m` 指定要编译的源文件是`hello.c`
  - `KERNEL_SRC` 指定内核路径  
  - `make -C` 切换路径  
  - `M=$(PWD)` 指定源文件的位置  
```  
PWD = $(shell pwd)
KVER = $(shell uname -r)
# 必须为绝对路径
KERNEL_SRC = /xxx/nuc977bsp/02.linux_kernel

obj-m := hello.o

all:
        make -C $(KERNEL_SRC) M=$(PWD)  
#展开为：
# make -C /xxx/nuc977bsp/02.linux_kernel M=xxx/hello/ modules

clean:
        rm -rf *.ko *.cmd *.o *.mod.c *.order *.symvers .tmp_versions  
 ```
#### Steps ####
1. 将`hello.ko`拷贝至开发板`/lib/modules`  
2. 使用`insmod hello.ko`安装模块  
3. 使用`lsmod`打印当前模块  
4. 使用`rmmod hello`卸载指定模块  
![hello module](https://s2.loli.net/2023/07/20/1nqFCfey6AXoWpK.png)  
### 模块命令 ###
- insmode 插入模块(存在依赖关系时插入必须按依赖关系插入)  
- rmmod 删除模块(同样需要按照依赖关系进行删除)
- module_param(参数名, 参数类型, 权限) 声明参数  
- depmod 将`/lib/modules`下的模块关系生成对应文件
- modprobe 带依赖关系地插入模块(`*.ko`必须放置在`/lib/modules`下，先用`depmod`生成依赖关系文档更佳)  
- EXPORT_SYMBOL(func_name) 将当前模块中的函数声明为其它模块可调用(调用模块中需要使用`extern`先声明要调用的函数)  
- ioremap 将物理地址映射到虚拟地址(驱动无法直接操作PA)  
## Lab： LED Driver ##
基于Linux字符设备框架实现GPIO控制LED灯亮灭  
[GPIO Control LED based on Linux](https://github.com/Cocoson23/NUC977/tree/master/Code/LinuxDevelopment/LED)  