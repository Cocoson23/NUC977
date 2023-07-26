#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/ioctl.h>
#include <asm/io.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/pci.h>
#include <linux/mm.h>
#include <linux/export.h>
#include <asm/uaccess.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("cocoson");
MODULE_DESCRIPTION("LED Driver");

#define LEDON _IO('L', 1)
#define LEDOFF _IO('L' ,0)

struct {  
	// 创建cdev结构体
	struct cdev *cdev;
	dev_t devnum;
	unsigned int *CLK_PCLKEN0;
	unsigned int *GPIOPB_DIR;
	unsigned int *GPIOPB_DATAOUT;

	atomic_t ledavailable;
	spinlock_t spinlock;
	struct mutex mutexlock;
} led;

// LED0 LED1 ON
int led_on(int i)
{
	switch (i)
	{
	// ARM中对普通内存与特殊功能寄存器进行了统一编址，普通内存内容会被cache读入，直接对地址进行读写操作可能是对
	// cache中的内容进行，而没有回写入设备，所以应当使用iowrite32 ioread32来告诉内核这是设备地址要直接读写内存而不经过cache
	case 0:
		iowrite32(ioread32(led.GPIOPB_DATAOUT) & 0xfffffff2, led.GPIOPB_DATAOUT);
		//*led.GPIOPB_DATAOUT &= 0xfffffff2;
		break;
	case 1:
		iowrite32(ioread32(led.GPIOPB_DATAOUT) & 0xfffffff1, led.GPIOPB_DATAOUT);
		//*led.GPIOPB_DATAOUT &= 0xfffffff1;
		break;
	default:
		break;
	}
	
	return 0;
}

// LED0 LED1 OFF
int led_off(int i)
{
	switch (i)
	{
	case 0:
		iowrite32(ioread32(led.GPIOPB_DATAOUT) | 0x00000001, led.GPIOPB_DATAOUT);
		//*led.GPIOPB_DATAOUT |= 0x00000001;
		break;
	case 1:
		iowrite32(ioread32(led.GPIOPB_DATAOUT) | 0x00000002, led.GPIOPB_DATAOUT);
		//*led.GPIOPB_DATAOUT |= 0x00000002;
		break;
	default:
		break;
	}
	
	return 0;
}

int led_init(struct inode *i, struct file *f) 
{
	// 获取自旋锁
	//spin_lock(&led.spinlock);
	// 获取互斥锁
	// mutex_lock(&led.mutexlock);

	led.GPIOPB_DIR = ioremap(0xB8003040, 4);
	led.GPIOPB_DATAOUT = ioremap(0xB8003044, 4);
	led.CLK_PCLKEN0 = ioremap(0xB0000218, 4);

	// 初始化时钟
	iowrite32(ioread32(led.CLK_PCLKEN0) | 0x00000008, led.CLK_PCLKEN0);
	// 初始化GPIO方向
	iowrite32(ioread32(led.GPIOPB_DIR) | 0x00000003, led.GPIOPB_DIR);
	// 灯初始化熄灭
	iowrite32(ioread32(led.GPIOPB_DATAOUT) | 0x00000003, led.GPIOPB_DATAOUT);
	//*led.GPIOPB_DATAOUT |= 0x00000003;
	
	// 释放自旋锁
	// spin_unlock(&led.spinlock);
	// 释放互斥锁
	// mutex_unlock(&led.mutexlock);

	printk("Register init finished\n");

	return 0;
}

int led_release(struct inode *i, struct file *f)
{
	led_off(0);
	led_off(1);

	// 解除内存映射
    iounmap(led.GPIOPB_DIR);
    iounmap(led.GPIOPB_DATAOUT);
	iounmap(led.CLK_PCLKEN0);

	return 0;
}

long led_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	switch (cmd)
	{
	case LEDON:
		led_on(arg);
		break;
	case LEDOFF:
		led_off(arg);
		break;
	default:
		break;
	};

	return 0;
}

ssize_t led_read(struct file *f, char __user *buf, size_t size, loff_t *offset)
{
	int led_val[2];
	
	led_val[0] = (ioread32(led.GPIOPB_DATAOUT) & (1 << 0)) >> 0;
	led_val[1] = (ioread32(led.GPIOPB_DATAOUT) & (1 << 1)) >> 1;

	// put_user 一次性传递一个数据块 char int short
	// 所以此处使用 copy_to_user
	if(copy_to_user((void*)buf, led_val, sizeof(led_val))) {}

	return sizeof(led_val);
}

static struct file_operations ledops = {
  // 专门挂接设备初始化动作
  .open = led_init,
  // 专门挂接设备释放动作
  .release = led_release,
  // 挂接设备控制函数
  .unlocked_ioctl = led_ioctl,
  // 增加模块计数
  .owner = THIS_MODULE,
  // 读取LED0 LED1值
  .read = led_read,
}; 



// 插入模块时调用该函数
static int __init led_module_init(void)
{ 
	// 初始化LED spinlock
	spin_lock_init(&led.spinlock);
	// 初始化LED mutex
	mutex_init(&led.mutexlock);

	// 申请主次设备号
	alloc_chrdev_region(&led.devnum, 0, 1, "LED");	
	printk("devnum: %d\n", MAJOR(led.devnum));
	
	// 构造cdev结构体 (char device struct)
	led.cdev = cdev_alloc();

	// 使用ops结构体初始化dev结构体
	cdev_init(led.cdev, &ledops);
	
	//注册到字符设备驱动框架中
	cdev_add(led.cdev, led.devnum, 1);

	printk("driver add success\n");
	printk("Insert my led driver\n");
	return 0;
}

// 模块析构函数
// 卸载模块时调用该函数
static void __exit led_module_exit(void)
{
	printk("led driver delete start\n");
	// 从字符设备驱动框架中注销驱动
	cdev_del(led.cdev);
	// 注销设备号
	unregister_chrdev_region(led.devnum, 1);
	
	printk("my led driver deleted\n");
}

// 将函数module_init设置为模块加载函数
module_init(led_module_init);
// 将函数module_exit设置为模块删除函数
module_exit(led_module_exit);

