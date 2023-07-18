# U-Boot顶层Makefile #  
**文中图文借鉴于学长学习经验及官方文档**  
整体`make`过程  
![make 过程](https://img1.imgtp.com/2023/07/18/c7vzgZDi.jpg)
## U-Boot 目录说明 ##
`uboot`编译前有如下目录
![uboot files](https://img1.imgtp.com/2023/07/18/CQUiBS1B.jpg)  
![目录说明](https://img1.imgtp.com/2023/07/18/2qVb540B.jpg)  
![目录说明](https://img1.imgtp.com/2023/07/18/gQ7JOD7y.jpg)  
- `Arch`
  其中包含了与架构相关的代码，所使用的NUC977为`ARM`架构所以涉及到的是`arch/arm`下的文件
- `CPU`  
  该目录下同样与CPU架构相关，含有`arch/arm/cpu/arm926ejs`  
- `Board`  
  该目录则与开发板厂商相关，`board/nuvoton/nuc970`即适配当前使用的`NUC977`开发板，故`uboot`移植的时候即可参考`board/nuvoton/nuc970`开发板  
- `Configs`  
  `uboot`配置文件，编译前必须使用`defconfig`配置`uboot`  
## U-Boot 顶层 Makefile 分析 ##  
### 版本 ###

    #  
    # SPDX-License-Identifier:      GPL-2.0+  
    #  

    VERSION = 2016  
    PATCHLEVEL = 11  
    SUBLEVEL =  
    EXTRAVERSION =  
    NAME =  
`VERSION`即为版本号，`PATCHLEVEL`即为补丁版本号，`SUBLEVEL`是次版本号  
### MAKEFLAGS 变量 ###
在makefile中有两个特殊的变量：`SHELL`和`MAKEFLAGS`，这两个变量除非使用`unexport`声明，否则的话在整个make的执行过程中，它们的值始终自动的传递给子make。  

    MAKEFLAGS += -rR --include-dir=$(CURDIR)  
    特殊变量 MAKEFLAGS 会自动传递给下一级make -rR表示禁止使用内置的隐含规则和变量定义 
    --include-dir指明搜索路径 $(CURDIR)当前文件夹  
### 控制输出 ###
uboot 默认编译是不会在终端中显示完整的命令，都是短命令。  

在终端中输出短命令虽然看起来很清爽，但是不利于分析 uboot 的编译过程。可以通过设置变量`V=1`来实现完整的命令输出，这个在调试 uboot 的时候很有用  
即编译的时候带上`V=1`，eg. `make V=1`
```
    ifeq ("$(origin V)", "command line")
        KBUILD_VERBOSE = $(V)
    endif
    ifndef KBUILD_VERBOSE
        KBUILD_VERBOSE = 0
    endif

    ifeq ($(KBUILD_VERBOSE),1)
        quiet =
        Q =
    else
        quiet=quiet_
        Q = @
    endif
```
ifeq 来判断`$(origin V)`和`command line`是否相等，`origin`函数用于求得变量来源。

比如在命令行中输入`V=1`的话那么`KBUILD_VERBOSE=1`。如果没有在命令行输入V的话`KBUILD_VERBOSE=0`。

判断`KBUILD_VERBOSE`是否为1，如果`KBUILD_VERBOSE`为`1`的话变量`quiet`和`Q`都为空，如果`KBUILD_VERBOSE=0`的话变量`quiet`为`quiet_`，变量`Q`为`@`

Makefile 中会用到变量`quiet`和`Q`来控制编译的时候是否在终端输出完整的命令

$(Q)$(MAKE) $(build)=tools，

如果`V=0`的话上述命令展开就是`@ make $(build)=tools`，make 在执行的时候默认会在终端输出命令，但是在命令前面加上`@`就不会在终端输出命令了。

sym 命令分为`quiet_cmd_sym`和`cmd_sym`两个版本，这两个命令的功能都是一样的，区别在于 make 执行的时候输出的命令不同。`quiet_cmd_xxx`命令输出信息少，也就是短命令，而`cmd_xxx` 命令输出信息多，也就是完整的命令。  
### 静默输出 ###  
`make -s`

    ifneq ($(filter 4.%,$(MAKE_VERSION)),)  # make-4
    ifneq ($(filter %s ,$(firstword x$(MAKEFLAGS))),)
        quiet=silent_
    endif
    else                                    # make-3.8x
    ifneq ($(filter s% -s%,$(MAKEFLAGS)),)
        quiet=silent_
    endif
    endif

    export quiet Q KBUILD_VERBOSE  
有时候我们在编译 uboot 的时候不需要输出命令，这个时候就可以使用 uboot 的静默输出功能。编译的时候使用“make -s”即可实现静默输出。  
### 代码检查 ###
`make C=0/1/2`

    ifeq ("$(origin C)", "command line")
        KBUILD_CHECKSRC = $(C)
    endif
    ifndef KBUILD_CHECKSRC
        KBUILD_CHECKSRC = 0
    endif  
- `C=0`时，`KBUILD_CHECKSRC = C 即0`，则会进入`ifndef KBUILD_CHECKSRC`代码块，其值保持为0，则会关闭代码检查功能  
- `C=1`时，`KBUILD_CHECKSRC = C 即1`，则不会进入`ifndef KBUILD_CHECKSRC`代码块，则会打开代码检查功能(代码注释中提到，C=1时仅对重新编译的文件进行代码检查，C=2时则对所有的源文件进行代码检查)
### 模块编译 ###

    # If building an external module we do not care about the all: rule
    # but instead _all depend on modules
    PHONY += all
    ifeq ($(KBUILD_EXTMOD),)
    _all: all
    else
    _all: modules
    endif

    ifeq ($(KBUILD_SRC),)
            # building in the source tree
            srctree := .
    else
            ifeq ($(KBUILD_SRC)/,$(dir $(CURDIR)))
                    # building in a subdirectory of the source tree
                    srctree := ..
            else
                    srctree := $(KBUILD_SRC)
            endif
    endif
    objtree         := .
    src             := $(srctree)
    obj             := $(objtree)

    VPATH           := $(srctree)$(if $(KBUILD_EXTMOD),:$(KBUILD_EXTMOD))

    export srctree objtree VPATH

    # Make sure CDPATH settings don't interfere
    unexport CDPATH  
当没有指定编译模块时，`KBUILD_EXTMOD`为空，则`_all = all`，否则`_all = modules`，通过该代码段可以在uboot中指定编译哪些模块  
### 获取主机架构和系统信息 ###

    HOSTARCH := $(shell uname -m | \
            sed -e s/i.86/x86/ \
                -e s/sun4u/sparc64/ \
                -e s/arm.*/arm/ \
                -e s/sa110/arm/ \
                -e s/ppc64/powerpc/ \
                -e s/ppc/powerpc/ \
                -e s/macppc/powerpc/\
                -e s/sh.*/sh/)

    HOSTOS := $(shell uname -s | tr '[:upper:]' '[:lower:]' | \
                sed -e 's/\(cygwin\).*/cygwin/')  
`uname -m`即可获取主机的架构，`uname -s`获取主机操作系统，则此处所用`HOSTARCH = x86_64`，`HOSTOS = Linux`(基于WSL2搭建开发板Linux开发平台)
### 设置目标架构、交叉编译器和配置文件 ###
    # set default to nothing for native builds
    ifeq ($(HOSTARCH),$(ARCH))
    CROSS_COMPILE ?=
    endif

    # Force set CROSS_COMPILE to arm-linux-
    CROSS_COMPILE=arm-linux-

    KCONFIG_CONFIG  ?= .config  
将交叉编译工具强制设置为了`arm-linux-`
### 交叉编译工具变量设置 ###
    # Make variables (CC, etc...)

    AS              = $(CROSS_COMPILE)as
    # Always use GNU ld
    ifneq ($(shell $(CROSS_COMPILE)ld.bfd -v 2> /dev/null),)
    LD              = $(CROSS_COMPILE)ld.bfd
    else
    LD              = $(CROSS_COMPILE)ld
    endif
    CC              = $(CROSS_COMPILE)gcc
    CPP             = $(CC) -E
    AR              = $(CROSS_COMPILE)ar
    NM              = $(CROSS_COMPILE)nm
    LDR             = $(CROSS_COMPILE)ldr
    STRIP           = $(CROSS_COMPILE)strip
    OBJCOPY         = $(CROSS_COMPILE)objcopy
    OBJDUMP         = $(CROSS_COMPILE)objdump
    AWK             = awk
    PERL            = perl
    PYTHON          = python
    DTC             = dtc
    CHECK           = sparse  
### 其余变量导出 ###
ARCH CPU BOARD VENDOR SOC CPUDIR BOARDDIR这7个变量在顶层Makefile中没有定义，

而是在 uboot 根目录下有个文件叫做 config.mk文件中

ARCH := $(CONFIG_SYS_ARCH:"%"=%)

CPU := $(CONFIG_SYS_CPU:"%"=%)  

而上述七个变量在*.config文件中进行了定义  
![config文件生成](https://img1.imgtp.com/2023/07/18/lxz6jxEH.jpg)
