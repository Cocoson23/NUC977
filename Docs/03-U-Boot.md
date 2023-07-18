# U-Boot #
Linux 系统要启动就必须需要一个` bootloader `程序（类似Windows中通过`BIOS`启动），这段 bootloader 程序会先初始化DDR 等外设，然后将 Linux 内核从 flash(NAND，NOR FLASH，SD，MMC 等)拷贝到 DDR 中，最后启动 Linux 内核。  

U-boot 的全称是 Universal Boot Loader，U-boot是一个遵循 GPL 协议的开源软件，U-boot是一个裸机代码，可以看作是一个裸机综合例程。  
| 种类 | 描述 |
| ---- | ---- |
| uboot官方提供的uboot代码 | 由uboot官方进行维护开发，版本更新快，基本包含常用芯片 |
| 半导体厂商提供的uboot代码 | 半导体厂商进行维护，专门针对自家芯片，对于自家生产的芯片有更好的支持 |
| 开发板厂商提供的uboot代码  | 在半导体厂商提供的uboot代码基础上增加了对自家开发板的支持 | 
***  
## U-Boot 编译 ##
- 编译工具链：`arm-linux-gcc`  
- 源码：`/home/qlqcetc/nuc977bsp/01.uBoot_v201611`
- 生成文件：
  1. u-boot.bin 
   完整功能的`u-boot`
  2. u-boot-spl.bin
   非完整功能的`u-boot`，仅在`NAND Boot`时使用
  3. mkimage

## NUC977进入uboot ##
使用putty连接开发板后，按键`RST`同时立即敲击任何按键即可取消`autoloader`进入`uboot`  
![Entry uboot](https://img1.imgtp.com/2023/07/18/U4E1MUdm.jpg)

## U-Boot命令 ##
U-Boot类似于裸机代码，在未引导OS的情况下可以通过U-Boot命令设置一些环境变量
### 编译 ###
- 清除之前编译的数据  
  `make distclean`  
- 设置`U-Boot`为出厂设置  
  `make nuc970_config`  
- 编译`U-Boot`  
  `make all`  
### 信息查询 ###  
  - `bdinfo`  
  可以查看板子的各项信息  
  - `printenv`
  输出环境变量信息  
  - `version`  
  查看`uboot`版本号  
### 环境变量操作 ###
  - 新建环境变量 setevn后面直接加名称与相对应的值，然后保存
  `setenv envname val`  
  `saveenv`
```
    环境变量的操作涉及到两个命令：setenv 和 saveenv，命令 setenv 用于设置或者修改环境变量的值。  
    命令 saveenv 用于保存修改后的环境变量，一般环境变量是存放在外部 flash 中的，uboot 启动的时候会将环境变量从 flash   
    读取到DRAM 中。所以使用命令 setenv 修改的是 DRAM中的环境变量值，修改以后要使用 saveenv 命令将修改后的环境  
    变量保存到 flash 中，否则的话uboot 下一次重启会继续使用以前的环境变量值。  
```  
### 网络操作 ###
![与网络相关的环境变量](https://img1.imgtp.com/2023/07/18/8k0N35Lv.jpg)  
...