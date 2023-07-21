#WSL2 搭建记录

##环境简述：
- Windows11
- Ubuntu22.04

##安装流程
### WSL2安装
注意WSL对系统与配置等要求！！！

**1. Download wsl2**
- __[wsl install](https://docs.microsoft.com/zh-cn/windows/wsl/install)__ - 官方参考

    PowerShell code 'code'
    wsl --install


**2. Install Ubuntu**
    
    wsl --install -d Distribution Name

    将Distribution Name替换为想要安装的Ubuntu发行版本

    也可以使用Microsoft Store搜索Ubuntu选择对应的发行版进行安装。  

![ubuntu.png](https://s2.loli.net/2022/08/17/xtD7ZlynJORmNa4.png)

**3. Regist Account**


等待Ubuntu安装完毕后，按照指示输入用户名及密码。此处用户名与密码即为后续使用时的用户名及密码。

**4. Install Windows Subsystem for Linux Preview**

在微软应用商店下载该软件，安装完毕后重启计算机。

![J73DYP2B8OHK47@6M_86XPH.png](https://s2.loli.net/2022/08/17/ATzgXDV6tRE9aHe.png)


**5. Optional**

完成上述操作后已经可以正常使用子系统了，可Ubuntu默认安装到C盘，为了给C盘腾空间，将Ubuntu进行迁移。

1. 导出分发版为tar到目标盘(此处以d盘为例)

    wsl --export Ubuntu-20.04 d:\wsl-ubuntu20.04.tar

2. 注销当前分发版

    wsl --unregister Ubuntu-20.04

3. 重新导入并安装WSL在目标盘

    wsl --import Ubuntu-20.04 d:\wsl-ubuntu20.04 d:\wsl-ubuntu20.04.tar

4. 设置默认登录用户为安装时用户名

    ubuntu2004 config --default-user Username

5. 删除tar文件(Optional)

    del d:\wsl-ubuntu20.04.tar

**6. End**

至此WSL2下的Ubuntu22.04已经安装完毕，即可点击Windows Subsystem for Linux Preview图标打开Ubuntu

![wsl.png](https://s2.loli.net/2022/08/17/DBHGUjrRpsvq7TQ.png)

打开后默认为root，可用su进行切换。

![end.png](https://s2.loli.net/2022/08/17/zncFOQ2uvSJW8Re.png)

**Enjoy Ubuntu！**