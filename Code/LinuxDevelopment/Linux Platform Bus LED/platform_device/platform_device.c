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
#include <linux/platform_device.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("cocoson");
MODULE_DESCRIPTION("LED Driver");

struct platform_device *mydev;
struct resource led_resource[] = {
	{
		.start = 0xB8003040,
		.end = 0xB8003044,
		.flags = IORESOURCE_MEM
	},
	{
		.start = 0,
		.end = 1,
		.name = "pinnum"
	}
};

// 插入模块时调用该函数
static int __init led_platform_device_module_init(void)
{ 
	// create a platform device
	mydev = platform_device_alloc("led", -1);

	// add resources to dev
	platform_device_add_resources(mydev, led_resource, 2);

	// insert platform dev into platform bus
	platform_device_add(mydev);

	printk("Insmod my device\n");
	return 0;
}

// 卸载模块时调用该函数
static void __exit led_platform_device_module_exit(void)
{
	platform_device_del(mydev);
	printk("Rmsmod my device\n");
}

// 将函数module_init设置为模块加载函数
module_init(led_platform_device_module_init);
// 将函数module_exit设置为模块删除函数
module_exit(led_platform_device_module_exit);

