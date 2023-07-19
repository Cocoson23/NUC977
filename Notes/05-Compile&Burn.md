# Compile and Burn #
## U-Boot ##

    源码：xxx/nuc977bsp/01.uBoot_v201611
    编译工具链：arm-linux-gcc
    生成文件：
    1. u-boot.bin
    2. u-boot-spl.bin (./spl/u-boot-spl.bin)
    3. mkimage  
### Steps ###
1. `cd xxx/01.uBoot_v201611`  
2. `make distclean` 清除所有`obj code`
3. `make nuc970_config` 将`uboot`设置为出厂设置
4. `make all` 编译`uboot`
UBoot 编译完成后会在/tools 子文件夹下生成 mkimage 工具，在编译内核生成 uImage 时会用到这
个工具。`cp tools/mkimage /bin`  并安装
## Linux kernel compile ##

    源码：xxx/nuc977bsp/02.linux_kernel
    编译工具链：arm-linux-gcc
    内核打包工具：mkimage
    生成文件：
    1.970uimage  
### Steps ###
1. `cd xxx/02.linux_kernel`  
2. `make clean` 清除之前的编译数据
3. `cp nuc977_5inch_tft.config .config ` 使用内核出厂配置，此处使用的与5inch屏幕对应的版本
4. `make menuconfig` 若有需求可裁剪内核
5. `make uImage` 编译内核 (会在xxx/nuc977bsp/image中产生镜像文件)  
## Build busybox filesystem ##

    源码：xxx/nuc977bsp/03.tools/busybox-1.22.1/
    编译工具链：arm-linux-gcc
    生成文件：
    bin sbin usr 三个文件夹和 linuxrc 文件  
### Steps ###
1. `cd xxx/03.tools/busybox-1.22.1/`  
2. `make clean` 清除之前的编译数据
3. `make menuconfig` 若有需求可裁剪文件系统
4. `make` 编译  
5. `make install` 安装
执行到这里，就会在xxx/nuc977bsp/03.tools/busybox-1.22.1/_install 路径下生
成所需要的文件和目录。将生成的 bin sbin usr 三个文件夹和 linuxrc 文件复制到酷客提供的
rootfs 文件夹下，替换原文件即可  
## Pack filesystem ##

    源码：xxx/nuc977bsp/03.tools/yaffs2utils
    编译工具链： gcc
    内核打包工具：mkyaffs2 （由上述源码生产）
    文件系统：/home/qlqcetc/nuc977bsp/rootfs_*
    生成文件：
    1. rootfs_yaffs2.img  
有些时候我们需要对开发板的文件系统进行更改，添加或删除一些文件。  
- 通过 U 盘或者内存卡将文件复制到开发板的文件系统里  
  例如在host上写好了GPIO控制LED的代码并通过交叉编译工具链进行了编译生成可执行文件，此时就可以拷贝到U盘或内存卡上，再插入开发板对应的接口存入开发板的文件系统中运行
- 通过更改`rootfs_yaffs2.img`这个文件系统镜像，这样就可以将修改好的文件系统直接烧录到板子上
  亦或是在打包文件系统时直接将想要添加的可执行文件打包到文件系统烧录到开发板中  

酷客开发板提供了两个文件系统的基础版本
- `xxx/nuc977bsp/rootfs_mini`内部只含有`Linux`系统运
行的必要文件  
- `xxx/nuc977bsp/rootfs_qt`带有移植好的`QT`运行环境的文件系统  
### Steps ###
1. `cd xxx/nuc977bsp/03.tools/yaffs2utils`  
2. `make clean` 清除之前的编译数据
3. `make` 编译
4. `cp mkyaffs2 /bin` 将编译完成生成的`mkyaffs2`程序安装到`/bin`
5. `cd xxx/nuc977bsp`   
6. `mkyaffs2 --inband-tags -p 2048 rootfs rootfs_yaffs2.img` 将文件系统打包为镜像文件 => 其中`rootfs`即为所准备的文件目录，此处准备的是带有qt版本的文件目录`rootfs_qt`  
![packfs](https://s2.loli.net/2023/07/19/tfLxQNzo3cZsVWE.png)  
打包完毕后镜像文件生成到`xxx/nuc977bsp/`路径下`rootfs_yaffs2.img`
![打包完毕](https://s2.loli.net/2023/07/19/LJ16ElxWQVPsbzo.png)  
## Burn ##
将`uboot`、`kernel`、`filesystem`烧录到`NAND Flash`，并设置NUC970系列芯片从`NAND Flash`中开机  
1. 将开发板设置为USB开机  
2. 打开`NuWriter`根据开发板上NUC Chip型号选择对应ini文件  
![NuWriter](https://s2.loli.net/2023/07/19/L9u8BFUVPGaJtmj.png)  
3. (**Optional**)擦除全盘，当且仅当文件系统有所更新或无法正常开机的情况下进行  
4. 烧录  
- `u-boot-spl.bin`

  1. 输入Image档案数据：
  - Chose Type : NAND
  - Image Name : u-boot-spl.bin
  - Image Type : uBoot
  - Image encrypt : Disable
  - Image execute address : 0x200
  1. 按下“Burn”
  2. 等待进度表完成
  3. 按下“Verify”确认烧入资料是否正确  
- `u-boot.bin`

  1. 输入Image档案数据：
  - Chose Type : NAND
  - Image Name : u-boot.bin
  - Image Type : Data
  - Image encrypt : Disable
  - Image execute address : 0x100000
  1. 按下“Burn”
  2. 等待进度表完成
  3. 按下“Verify”确认烧入资料是否正确  
- `env.txt`

  1. 输入Image档案数据：
  - Chose Type : NAND
  - Image Name : env.txt
  - Image Type : Environment
  - Image encrypt : Disable
  - Image execute address : 0x80000
  1. 按下“Burn”
  2. 等待进度表完成
  3. 按下“Verify”确认烧入资料是否正确  
- `970uimage`

  1. 输入Image档案数据：
  - Chose Type : NAND
  - Image Name : 970uimage_5inch.tft
  - Image Type : Data
  - Image encrypt : Disable
  - Image execute address : 0x200000
  1. 按下“Burn”
  2. 等待进度表完成
  3. 按下“Verify”确认烧入资料是否正确  
- `rootfs_yaffs2`

  1. 输入Image档案数据：
  - Chose Type : NAND
  - Image Name : rootfs_yaffs2_qt_5inch_tft.img
  - Image Type : Data
  - Image encrypt : Disable
  - Image execute address : 0x1600000
  1. 按下“Burn”
  2. 等待进度表完成
  3. 按下“Verify”确认烧入资料是否正确  
5. 将开发板设置为`NAND Flash`启动，按下`RST`复位键重启即可启动`Linux`  
![Linux](https://s2.loli.net/2023/07/19/XYQ3O58uvZKFrtq.png)