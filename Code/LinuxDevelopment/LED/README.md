# LED Linux Driver #
- Makefile  
- led_driver.c  
- led.c
## led_driver.c ##
LED linux Driver file  
以 Linux 驱动框架为基础进行了LED驱动搭建，将LED以模块形式实现模块初始化及卸载函数。同时定义`led_on()` `led_off()`函数实现`LED0` `LED1`的亮灭。  
具体实现中，于`led_init()`函数实现GPIO初始化及字符设备驱动设计流程：  
1. 申请主次设备号
2. 构造cdev结构体
3. 使用ops结构体初始化dev结构体，并将`led_on` `led_off`函数赋值给ops结构体的`open` `release`元素，以实现打开字符设备文件时灯亮，关闭字符设备文件时灯灭。
4. 注册字符设备驱动
而`led_exit()`函数中则完成了如下流程：
1. 将字符设备驱动从框架中注销
2. 注销设备号
3. 解除内存映射
## Makefile ##  
实现对整体项目文件编译的管理  
1. 使用`obj-m`指定编译的驱动模块源文件
2. 使用交叉编译工具编译应用层源码
3. 指定`make clean`清除的文件
## led.c ##
应用层控制驱动代码，使用`open` `close`字符设备文件实现循环点亮熄灭LED  
## 使用驱动 ##
- `make`完成后将`led_driver.ko` `led`拷贝至开发板
- 将`led_driver.ko`移动至`/lib/modules/`并执行`insmod led_driver.ko`插入模块
- `mknod led_cdevf c devnum minor`生成字符设备文件`led_cdevf`，其中`devnum`为主设备号`minor`为次设备号
- 将`led_cdevf` `led`移动至同一目录下
- `./led`执行代码
