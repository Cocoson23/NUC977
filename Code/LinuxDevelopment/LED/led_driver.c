#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/ioctl.h>
#include <asm/io.h>
#include <linux/cdev.h>
#include <linux/fs.h>

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
} led;

// LED0 LED1 ON
int led_on(int i)
{
	switch (i)
	{
	case 0:
		*led.GPIOPB_DATAOUT &= 0xfffffff2;
		printk("LED0 ON\n");
		break;
	case 1:
		*led.GPIOPB_DATAOUT &= 0xfffffff1;
		printk("LED1 ON\n");
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
		*led.GPIOPB_DATAOUT |= 0x00000001;
		printk("LED0 OFF\n");
		break;
	case 1:
		*led.GPIOPB_DATAOUT |= 0x00000002;
		printk("LED1 OFF\n");
		break;
	default:
		break;
	}
	
	return 0;
}

int led_init(struct inode *i, struct file *f) 
{
	//CLK_PCLKEN0 = ioremap(0xB0000218, 4);
	led.GPIOPB_DIR = ioremap(0xB8003040, 4);
	led.GPIOPB_DATAOUT = ioremap(0xB8003044, 4);
	//*CLK_PCLKEN0 = 0x00000008;
	*led.GPIOPB_DIR |= 0x00000003;
	// 灯初始化熄灭
	*led.GPIOPB_DATAOUT |= 0x00000003;
	printk("Register init finished\n");

	return 0;
}

int led_release(struct inode *i, struct file *f)
{
	*led.GPIOPB_DATAOUT |= 0x00000003;

	// 解除内存映射
    //iounmap(led.CLK_PCLKEN0);
    iounmap(led.GPIOPB_DIR);
    iounmap(led.GPIOPB_DATAOUT);

	return 0;
}

long led_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	switch (cmd)
	{
	case LEDON:
		printk("cmd: %d\n", LEDON);
		led_on(arg);
		break;
	case LEDOFF:
		printk("cmd: %d\n", LEDOFF);
		led_off(arg);
		break;
	default:
		break;
	};

	return 0;
}

static struct file_operations ledops = {
  // 专门挂接设备初始化动作
  .open = led_init,
  // 专门挂接设备释放动作
  .release = led_release,
  // 挂接设备控制函数
  .unlocked_ioctl = led_ioctl,
}; 



// 插入模块时调用该函数
static int __init led_module_init(void)
{
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

