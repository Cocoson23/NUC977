# 使用入门 #
开发板出厂时已经在 NAND FLASH 中烧录好 linux 系统了，开机后将会自动启动使用 QT 开
发的 demo 程序。  
## 开机 ##
开机过程如下：  
1. 用数据线连接开发板的 Micro USB 接口到电脑。  
2. 设置开发板拨码开关为 NAND FLASH 启动。  

| BOOT | Cfg0 | Cfg1 |
| ---- | ---- | ---- |
| NAND | ON | OFF |
| SPI  | OFF| OFF |
| USB  | ON | ON  |  
## 连接开发板 ##
于Windows平台使用`Putty`与开发板进行通信  
1. 安装`CH340`驱动
2. 打开`Putty`并将`Serial Line`设置为对应COM号，波特率设置为`115200`，连接方式设置为`Serial`,最后点击`Open`
3. 按下开发板`RST`复位按钮后`Putty`窗口显示开发板信息