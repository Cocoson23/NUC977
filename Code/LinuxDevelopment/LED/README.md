# LED Linux Driver #
- Makefile  
- led_driver.c  
- led.c
## led_driver.c ##
LED linux Driver file  
以 Linux 驱动框架为基础进行了LED驱动搭建，将LED以模块形式实现模块初始化及卸载函数。同时定义`led_on()` `led_off()`函数实现`LED0` `LED1`的亮灭。  
具体实现中，于`led_module_init()`函数实现GPIO初始化及字符设备驱动设计流程：  
1. 申请主次设备号
2. 构造cdev结构体
3. 使用ops结构体初始化dev结构体，并将`led_on` `led_off`函数装进`led_ioctl`函数中，赋值给ops结构体的`unlocked_ioctl`成员，ops结构体的`open` `release`成员则由`led_init` `led_release`赋值，以实现打开字符设备文件时完成GPIO初始化，传入对应指令时完成灯的亮灭操作，关闭字符设备文件时熄灭灯并取消内存映射关系。  

4. 注册字符设备驱动而`led_module_exit`函数中则完成了如下流程：  
   - 将字符设备驱动从框架中注销  
   -  注销设备号  

## Makefile ##  
实现对整体项目文件编译的管理  
1. 使用`obj-m`指定编译的驱动模块源文件
2. 使用交叉编译工具编译应用层源码
3. 指定`make clean`清除的文件
## led.c ##
应用层控制驱动代码，使用`ioctl` 向打开的字符设备文件传输命令实现循环点亮熄灭LED  
其中`ioctl`所需传递的`command`可使用`_IO` `_IOR` `_IOW`等函数生成  
## 使用驱动 ##
- `make`完成后将`led_driver.ko` `led`拷贝至开发板
- 将`led_driver.ko`移动至`/lib/modules/`并执行`insmod led_driver.ko`插入模块
- `mknod led_cdevf c devnum minor`生成字符设备文件`led_cdevf`，其中`devnum`为主设备号`minor`为次设备号
- 将`led_cdevf` `led`移动至同一目录下
- `./led`执行代码
## 改进 ##
