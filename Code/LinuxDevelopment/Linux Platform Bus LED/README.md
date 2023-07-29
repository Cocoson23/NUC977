# 总线设备驱动框架 #  
- 总线  
- 设备  
- 驱动  

总线设备驱动框架中，将总线视为根节点，将设备与驱动分离分别以链表的形式挂接在总线节点上。  
## Platform Bus ##  
专为没有专属总线的设备设置出的总线，以形成统一的驱动框架。如：BEEP，LED等设备。  

- platform device  
- platform driver  
  
### Platform Device ###  
在平台设备总线框架中，将设备的描述分离为了`platform device`模块，其负责对设备资源进行描述，如：设备的寄存器地址、所需使用到的引脚号等，均可以通过该模块进行描述然后传递给`platform driver`模块使用。  

  
- 创建平台设备  
  `struct platform_device *platform_device_alloc(const char *name, int id)`  
  - name  
  设置设备名称，此处必须与`platform_driver`所设置名称一致，不然设备与驱动间就无法准确匹配  
  - id  
  设备序号，如，`("led", 1)`则设备名称即为`led1`，将其置为-1时设备名称为name本身不带序号  
- 使用resource结构体对设备资源进行描述  
  ```
  struct resource device_resource[] = {
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
  ```  

- 将设备资源写入平台设备结构体中  
  `int platform_device_add_resources(struct platform_device *pdev, const struct resource *res, unsigned int num)`  
  - pdev即为平台设备结构体  
  - res即为设备资源结构体  
  - num为要添加的资源结构体数  
  
- 将平台设备挂接到平台设备总线  
  `int platform_device_add(struct platform_device *pdev)`
  
编译完成后即可生成`platform_device.ko`，可单独控制设备模块的插入与删除。  
### Platform Driver ###
平台设备驱动作为一个单独的模块，在字符设备驱动框架实现设备驱动功能的基础上构建了平台设备驱动的结构，通过构建`platform_driver`结构体实现与`platform_device`的匹配并通过`platform_get_resource`或`platform_get_resource_byname`获取设备资源，即可完成对设备的控制。  

- 构建平台设备驱动结构体  
  ```
  struct platform_driver led_driver = {
	    // 驱动名字，通过platform总线与设备进行匹配
	    .driver.name = {	
		"led",
	    },
	    .probe = led_probe,	// 匹配成功调用接口
	    .remove = led_remove,	// 任意一方离开时总线调用
    };
  ```  
- 实现`probe`函数与`remove`函数  
  - probe  
  在probe函数中可以实现字符设备号申请与注册等操作，并创建设备类与设备文件以省略mknod操作  
  `class_create(THIS_MODULE, "class_name")`  
  `device_create(device.class, NULL, device.devnum, NULL, "device_name")`  
  - remove  
  在remove函数中完成字符设备号注销操作与设备文件删除、设备类删除的操作  
  `device_del(device);`  
  `class_destroy(class);`  
- 将原有设备驱动代码移植到平台设备驱动文件中  

在模块init函数与exit函数中分别调用`platform_driver_register(&platform_driver)`与`platform_driver_unregister(&platform_driver);`实现平台设备驱动的挂载与注销。  

在设备与驱动模块插入并匹配成功后即可在/sys/class/下看到所插入的设备类，在/dev/下看到插入的设备文件，即可通过所实现的app.c实现对设备的控制。