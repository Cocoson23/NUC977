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
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>


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

	struct resource *addr;
	struct resource *pin_num;
	struct class *class;
	struct device *device;
} led;

// LED0 LED1 ON
int led_on(int i)
{
	switch (i)
	{
	case 0:
		iowrite32(ioread32(led.GPIOPB_DATAOUT) & 0xfffffff2, led.GPIOPB_DATAOUT);
		break;
	case 1:
		iowrite32(ioread32(led.GPIOPB_DATAOUT) & 0xfffffff1, led.GPIOPB_DATAOUT);
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
		break;
	case 1:
		iowrite32(ioread32(led.GPIOPB_DATAOUT) | 0x00000002, led.GPIOPB_DATAOUT);
		break;
	default:
		break;
	}
	
	return 0;
}

int led_init(struct inode *i, struct file *f) 
{


	// 初始化时钟
	// iowrite32(ioread32(led.CLK_PCLKEN0) | 0x00000008, led.CLK_PCLKEN0);
	// 初始化GPIO方向
	iowrite32(ioread32(led.GPIOPB_DIR) | 0x00000003, led.GPIOPB_DIR);
	// 灯初始化熄灭
	iowrite32(ioread32(led.GPIOPB_DATAOUT) | 0x00000003, led.GPIOPB_DATAOUT);

	return 0;
}

int led_release(struct inode *i, struct file *f)
{
	led_off(0);
	led_off(1);

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
	
	led_val[0] = (ioread32(led.GPIOPB_DATAOUT) & (1 << 0)) >> led.pin_num->start;
	led_val[1] = (ioread32(led.GPIOPB_DATAOUT) & (1 << 1)) >> led.pin_num->end;

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


int led_probe(struct platform_device *dev)
{
	led.addr = platform_get_resource(dev, IORESOURCE_MEM, 0);
	led.pin_num = platform_get_resource_byname(dev, 0, "pinnum");

	// 申请主次设备号
	alloc_chrdev_region(&led.devnum, 0, 1, "LED");	
	printk("devnum: %d\n", MAJOR(led.devnum));
	
	// 构造cdev结构体 (char device struct)
	led.cdev = cdev_alloc();

	// 使用ops结构体初始化dev结构体
	cdev_init(led.cdev, &ledops);
	
	//注册到字符设备驱动框架中
	cdev_add(led.cdev, led.devnum, 1);

	led.GPIOPB_DIR = ioremap((phys_addr_t)led.addr->start, 4);
	led.GPIOPB_DATAOUT = ioremap((phys_addr_t)led.addr->end, 4);
	// led.CLK_PCLKEN0 = ioremap(led_addr->end, 4);

	// create cdev file
	led.class = class_create(THIS_MODULE, "led");
	led.device = device_create(led.class, NULL,
			     led.devnum, NULL, "led");

	printk("driver matched!\n");
	return 0;
}
int led_remove(struct platform_device *dev)
{
	// delete cdev file
	device_del(led.device);
	class_destroy(led.class);

	// 从字符设备驱动框架中注销驱动
	cdev_del(led.cdev);
	// 注销设备号
	unregister_chrdev_region(led.devnum, 1);

	// 解除内存映射
    iounmap(led.GPIOPB_DIR);
    iounmap(led.GPIOPB_DATAOUT);
	//iounmap(led.CLK_PCLKEN0);

	printk("driver removed!\n");
	return 0;
}


// create platform driver
struct platform_driver led_driver = {
	// 驱动名字，通过platform总线与设备进行匹配
	.driver.name = {	
		"led",
	},
	.probe = led_probe,	// 匹配成功调用接口
	.remove = led_remove,	// 任意一方离开时总线调用
};

// 插入模块时调用该函数
static int __init led_platform_driver_module_init(void)
{ 
	platform_driver_register(&led_driver);

	printk("driver add success\n");
	printk("Insert my led driver\n");
	return 0;
}

// 模块析构函数
// 卸载模块时调用该函数
static void __exit led_platform_driver_module_exit(void)
{
	platform_driver_unregister(&led_driver);
	
	printk("my led driver deleted\n");
}

// 将函数module_init设置为模块加载函数
module_init(led_platform_driver_module_init);
// 将函数module_exit设置为模块删除函数
module_exit(led_platform_driver_module_exit);

