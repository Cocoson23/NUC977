# NUC977 Study #
## Doc Links ##  
- 环境搭建  
  `Windows11` `WSL2` `Ubuntu22.04` `VSCode` 
  - [wsl安装](https://github.com/Cocoson23/NUC977/blob/master/Notes/00-WSL-Ubuntu%E6%90%AD%E5%BB%BA.md)
  - [Linux开发环境搭建](https://github.com/Cocoson23/NUC977/blob/master/Notes/02-BuildLinuxEnv.md)
- 入门  
  - [酷客NUC977开发板使用入门](https://github.com/Cocoson23/NUC977/blob/master/Notes/01-Start.md)  
  - [U-Boot简介](https://github.com/Cocoson23/NUC977/blob/master/Notes/03-U-Boot.md)  
  - [U-Boot Makefile](https://github.com/Cocoson23/NUC977/blob/master/Notes/04-U-Boot-Makefile.md)  
  - [Compile and Burn](https://github.com/Cocoson23/NUC977/blob/master/Notes/05-Compile%26Burn.md)
- 驱动  
  - [Linux cdev Driver](https://github.com/Cocoson23/NUC977/blob/master/Notes/06-Linux%20cdev%20driver.md)
  - [Linux Platform Device Driver](https://github.com/Cocoson23/NUC977/blob/master/Code/LinuxDevelopment/Linux%20Platform%20Bus%20LED/README.md)  
## Labs ##
### Bare Machine ###  
- [GPIO Control LEDs](https://github.com/Cocoson23/NUC977/tree/master/Code/Bare%20Machine/01-GPIOLED)  
### Linux Development ###  
- [GPIO Control LEDs based on Linux](https://github.com/Cocoson23/NUC977/tree/master/Code/LinuxDevelopment/LED)  
- [GPIO Control LEDs based on Platform Bus](https://github.com/Cocoson23/NUC977/tree/master/Code/LinuxDevelopment/Linux%20Platform%20Bus%20LED)  
## Tricks ##
- 给`Git`添加代理  
  `git config --global https.proxy 'http://127.0.0.1:xxxx'` => xxxx为`port number`  
  `git config --global http.proxy 'http://127.0.0.1:xxxx'`
