# 搭建Linux开发环境 #
## Ubuntu安装 ##
- 使用 `Vmware` 搭配 `Ubuntu` 进行搭建
- 使用`WSL2`进行搭建
### 安装所需软件 ###
- 安装32位兼容包
  `#apt-get install lib32stdc++6 libc6:i386 `  
  但此处需要先添加32位程序包  
  `dpkg --add-architecture i386`  
  `apt install libc6:i386 libstdc++6:i386`  
  `sudo apt-get update`  
  `apt install libncurses5-dev lib32z1`
- 安装g++  
  `#apt-get install g++`  
- 安装libncurses5-dev 支持包  
  `#apt-get install libncurses5-dev`  
- 安装BSP  
  使用提供的`install.sh`一键解压`linuxbsp`目录中的所有压缩包到指定路径  
### 设置交叉编译器环境变量 ###  
于`/root/.bashrc`中最后一行添加`export PATH=$PATH:/usr/local/arm_linux_4.8/bin`  
随即在命令行输入`source /root/.bashrc`, **注意需要在root下执行**
在`root`下命令行输入`arm-linux-gcc -v`  
![交叉编译配置成功示意图](https://i.imgur.com/dj8WuBS.png)  
### 安装NuVCOM驱动程序 ###
NuWriter 必须在 Windows 系统中安装 VCOM 驱动程序才能使用 NuWriter 工具。请依据下列步骤来 安
装 WinUSB4NuVCOM 驱动程序：
- 1. 用数据线连接开发板的 Micro USB 接口到电脑。
- 2. 设置开发板拨码开关为 USB 启动。

| BOOT | Cfg0 | Cfg1 |
| ---- | ---- | ---- |
| NAND | ON | OFF |
| SPI | OFF | OFF |
|USB | ON | ON |  
- 3. 在计算机中执行 WinUSB4NuVCOM.exe 开始安装驱动程序。