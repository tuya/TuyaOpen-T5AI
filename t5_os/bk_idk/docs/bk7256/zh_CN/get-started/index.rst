快速入门
=======================

:link_to_translation:`en:[English]`

本文档以 BK7256 开发板为例，通过一个简单的示例项目向您展示

 - 如何构建Armino编译环境；
 - 以及如何配置 Armino 工程；
 - 如何编译、下载固件；

准备工作
------------------------

硬件：

 - BK7256 开发板
 - 串口烧录工具
 - 电脑（Windows 与 Ubuntu）

.. note::

  Armino 支持在 Linux 平台编译，也支持在 Windows 平台的 Linux 虚拟机上编译;
  Armino 支持在 Windows/Linux 平台进行固件烧录(参考烧录工具中指导文档)。

软件：

 - RISCV工具链，用于编译 Armino RISCV版本；
 - 构建工具，包含 CMake；
 - Armino 源代码；
 - 串口烧录软件；




Armino SDK代码下载
------------------------------------

您可从 gitlab 上下载 Armino::

    mkdir -p ~/armino
    cd ~/armino
    git clone https://gitlab.bekencorp.com/armino/bk_idk.git

然后切换到稳定分支Tag节点, 如v2.0.1.12::

    git checkout -B your_branch_name v2.0.1.12

.. note::

    从官网的gitlab下载的为最近的SDK代码，相关账号找项目上审核申请。


构建编译环境
------------------------------------

.. note::

    Armino 编译环境要求Ubuntu 20.04 LTS 版本及以上，本章节将以 Ubuntu 20.04 LTS 版本为例，介绍整个编译环境的搭建。


工具链安装
------------------------------------

BK7256工具链下载路径如下：

	工具链下载：
	http://dl.bekencorp.com/tools/toolchain/
	在此目录下获取最新版本，如：toolchain_v5.2.1.tgz

工具包下载后，通过如下操作命令解压至 /opt/risc-v目录下::

    $ sudo tar -zxvf toolchain_v5.2.1.tgz -C /


.. note::

    工具链默认路径在middleware/soc/bk7256/bk7256.defconfig文件中定义，客户可自行配置::

        CONFIG_TOOLCHAIN_PATH="/opt/risc-v/nds32le-elf-mculib-v5/bin"

    工具链也可支持相对路径配置，如工具链放在sdk目录下::

        CONFIG_TOOLCHAIN_PATH="toolchain_v5.2.1/nds32le-elf-mculib-v5/bin"



程序编译依赖库安装
------------------------------------

在终端输入下述命令安装 python3，CMake，Ninja，Crypto以及依赖库::

    sudo dpkg --add-architecture i386
    sudo apt-get update
    sudo apt-get install build-essential cmake python3 python3-pip doxygen ninja-build libc6:i386 libstdc++6:i386 libncurses5-dev lib32z1 -y
    sudo pip3 install pycryptodome click

文档编译依赖库安装
------------------------------------

在终端上输入如下命令安装文档编译所需要的 python 依赖::

    sudo pip3 install sphinx_rtd_theme future breathe blockdiag sphinxcontrib-seqdiag sphinxcontrib-actdiag sphinxcontrib-nwdiag sphinxcontrib.blockdiag


设置Python默认版本::

    sudo ln -s /usr/bin/python3 /usr/bin/python


编译工程
------------------------------------

在终端下输入下述命令编译 Armino 默认工程，PROJECT为可选参数，默认为app，默认工程为启动 WiFi，BLE，初始化常见驱动，并启动 Armino 默认 Cli 程序::

    cd ~/armino/bk_idk
    make bk7256

- BK7256为双CPU核系统，默认配置为双核，编译BK7256平台CPU0系统，会自动编译CPU1和CPU0，并将CPU0，CPU1系统一起打包



- BK7256平台默认工程使用的是FreeRTOS V10.4系统::

    components/os_source/freertos_v10

- BK7256平台支持鸿蒙OS V3.0.1系统，可编译harmony工程::

    cd ~/armino/bk_idk
    make bk7256 PROJECT=harmony


.. _WiFi-menuconfig:

配置工程
------------------------------------

- 您可以通过 menuconfig 来更改 Armino 默认配置项，以bk7256为例进行操作说明

    + 终端键入命令,其中ARMINO_SOC用于指定芯片::

        cd ~/armino/bk_idk
        make menuconfig ARMINO_SOC=bk7256

    + menuconfig配置界面呈现如下图:
        .. figure:: ../../_static/menuconfig.png
            :align: center
            :alt: menuconfig gui
            :figclass: align-center

            Menuconfig

    + 通过上下键选择，并按回车键进入组件配置项的目录：
      (Top)-->ARMINO Configuration-->Components Configuration

    + 配置当前目录所罗列的组件配置项，并按"S"键保存

    + 保存后，从menuconfig配置界面配置的差异配置项将更新到工程配置文件projects/app/config/bk7256.config中

    + 终端输入命令，编译工程bk7256::

        cd ~/armino/bk_idk
        make bk7256

    + 编译时将以工程配置文件 Override 芯片配置文件 Override 默认配置的优先级进行差异化配置
      如： bk7256.config >> bk7256.defconfig >> KConfig

- 您也可以手动直接通过工程配置文件来进行差异化配置::

    工程配置文件 Override 芯片配置文件 Override 默认配置
    如： bk7256/config >> bk7256.defconfig >> KConfig
    + 工程配置文件示例：
        projects/app/config/bk7256/config
        projects/harmony/config/bk7256/config
    + 芯片配置文件示例：
        middleware/soc/bk7256/bk7256.defconfig
    + KConfig配置文件示例：
        middleware/arch/riscv/Kconfig
        components/bk_cli/Kconfig

- 重要的配置说明
    + 操作系统配置为FreeRTOS V10::

        #
        # FreeRTOS
        #
        CONFIG_FREERTOS=y
        # CONFIG_LITEOS_M is not set
        CONFIG_FREERTOS_V9=n
        CONFIG_FREERTOS_V10=y

    + 操作系统配置为鸿蒙OS::

        # LITEOS_M
        CONFIG_LITEOS_M_V3=y
        CONFIG_LITEOS_M_BK=y

        # FreeRTOS
        CONFIG_FREERTOS=n
        CONFIG_FREERTOS_V9=n
        CONFIG_FREERTOS_V10=n

- 系列芯片宏的使用与差异

    + 宏CONFIG_SOC_BK7256XX 表示BK7256系列::

        属于BK7235/BK7237/BK7256公共芯片宏，CPU1也需要定义该宏
		配置方式：CONFIG_SOC_BK7256XX=y
		

    + 区分同系列芯片的宏(不用于区分其他芯片)::

		CONFIG_SOC_BK7256, CPU1需要定义该宏和CONFIG_SLAVE_CORE组合区分BK7256_CPU1
		配置方式：CONFIG_SOC_BK7256=y
		

    + 字符串系列芯片的宏(用于编译阶段区分其他芯片)::

         CPU0/CPU1有区分：
		 CPU0写法：CONFIG_SOC_STR="bk7256"
		 CPU1写法：CONFIG_SOC_STR="bk7256_cp1"


    + 双核CPU0,CPU1相关宏区分(用于代码区分)::
         
         CONFIG_DUAL_CORE            #双核功能
         CONFIG_MASTER_CORE          #依赖于CONFIG_DUAL_CORE，CPU0与CPU1的区别
         CONFIG_SLAVE_CORE           #依赖于CONFIG_DUAL_CORE，CPU0与CPU1的区别
         单核：上面的三个宏都不定义

- 模块选择CPUx执行
	 
    + 因为是双核AMP系统架构，CPU0和CPU1的软件独立编译，但SDK是一套，所以CPU0和CPU1的的部分功能差异需要使用宏区分。
      比如TRNG随机数控制器只有一份，使用双核配置时，应用程序需要互斥配置在哪一个系统(CPU0 or CPU1)中执行。
      假设CPU0需要使用TRNG，而CPU1不需要使用，则bk7256.defconfig中的CONFIG_TRNG=y，而bk7256_cp1.defconfig中配置为CONFIG_TRNG=n。
      在软件代码中，使用CONFIG_TRNG宏隔离调用，防止部分公共代码文件(CPU0和CPU1代码相同部分)在CPU1上编译报错。
    + 示例如下::
         
         #if CONFIG_TRNG             #使用模块功能开关宏隔离代码
         #include "driver/trng.h"
         #endif
         ...
         #if CONFIG_TRNG             #使用模块功能开关宏隔离代码
         bk_rand();
         #endif

新建工程
------------------------------------

默认工程为projects/app，新建工程可参考projects/app工程


烧录代码
------------------------------------

Armino 支持在 Windows/Linux 平台进行固件烧录, 烧录方法参考烧录工具中指导文档。
以Windows 平台为例， Armino 目前支持 UART 烧录。



通过串口烧录
********************

.. note::

    Armino 支持 UART 烧录，推荐使用 CH340 串口工具小板进行下载。

串口烧录工具如下图所示:

.. figure:: ../../_static/download_tool_uart.png
    :align: center
    :alt: Uart
    :figclass: align-center

    UART

烧录工具（BKFIL）获取：

	https://dl.bekencorp.com/tools/flash/
	在此目录下获取最新版本，如：BEKEN_BKFIL_V2.1.6.0_20231123.zip

BKFIL.exe 界面及相关配置如下图所示：

.. figure:: ../../_static/download_uart_bk7256.png
    :align: center
    :alt: BKFIL GUI
    :figclass: align-center

    BKFIL GUI

选择烧录串口 UART1，点击 ``烧录`` 进行版本烧录, 烧录完成之后掉电重启设备。

.. note::

    您可以通过阅读 BKFIL 压缩包中的文档了解 BKFIL 更详细的使用方法。

串口 Log 及 Command Line
------------------------------------

- 目前BK7256平台，串口Log及Command Line命令输入在UART1口；可通过help命令查看支持命令列表；
- CPU1的Log也通过CPU0的UART1串口输出，CPU1的Log带“cpu1”标记；
- CPU1的Command Line可通过CPU0的UART1执行，如::

    cpu1 help //输出cpu1的命令列表
    cpu1 time //输出cpu1的当前运行时间


编译选项及链接选项
------------------------------------

 - BK7256平台，默认编译选项"-mstrict-align"，链接选项"-Wl,--defsym,memcpy=memcpy_ss"
 - 若单独编译lib库，需要增加编译选项"-mstrict-align"
 - 若不使用平台的链接命令，如编译鸿蒙系统，对于Andes v5.1.1工具链，需要增加链接选项"-Wl,--defsym,memcpy=memcpy_ss"
