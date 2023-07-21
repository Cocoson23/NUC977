#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <asm/io.h>
#include <linux/cdev.h>
#include <linux/fs.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("cocoson");
MODULE_DESCRIPTION("LED Driver");

unsigned int *CLK_PCLKEN0;
unsigned int *GPIOPB_DIR;
unsigned int *GPIOPB_DATAOUT;

// LED0 LED1 ON
int led_on(struct inode *i, struct file *f)
{
	*GPIOPB_DATAOUT &= 0xfffffffC;
	printk("LED ON\n");
	return 0;
}

// LED0 LED1 OFF
int led_off(struct inode *i, struct file *f)
{
	*GPIOPB_DATAOUT |= 0x00000003;
	printk("LED OFF\n");
	return 0;
}

static struct file_operations ledops = {
  .open = led_on,
  .release = led_off,
}; 

// 创建cdev结构体
struct cdev *mycdev;
	
dev_t devnum;

// 插入模块时调用该函数
static int __init led_init(void)
{
	//CLK_PCLKEN0 = ioremap(0xB0000218, 4);
	GPIOPB_DIR = ioremap(0xB8003040, 4);
	GPIOPB_DATAOUT = ioremap(0xB8003044, 4);
	//*CLK_PCLKEN0 = 0x00000008;
	*GPIOPB_DIR |= 0x00000003;
	printk("Register init finished\n");
	
	// 申请主次设备号
	alloc_chrdev_region(&devnum, 0, 1, "LED");	
	printk("devnum: %d\n", MAJOR(devnum));

	// 构造cdev结构体 (char device struct)
	mycdev = cdev_alloc();

	// 使用ops结构体初始化dev结构体
	cdev_init(mycdev, &ledops);
	
	//注册到字符设备驱动框架中
	cdev_add(mycdev, devnum, 1);

	printk("driver add success\n");
	printk("Insert my led driver\n");
	return 0;
}

// 模块析构函数
// 卸载模块时调用该函数
static void __exit led_exit(void)
{
	printk("led driver delete start\n");
	// 从字符设备驱动框架中注销驱动
	cdev_del(mycdev);
	// 注销设备号
	unregister_chrdev_region(devnum, 1);

	// 解除内存映射
        iounmap(CLK_PCLKEN0);
        iounmap(GPIOPB_DIR);
        iounmap(GPIOPB_DATAOUT);
	
	printk("my led driver deleted\n");
}

// 将函数hello_init设置为模块加载函数
module_init(led_init);
// 将函数hello_exit设置为模块删除函数
module_exit(led_exit);

