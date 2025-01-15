Quick Start Guide
==============================================

:link_to_translation:`zh_CN:[中文]`

This article demonstrates:

 - Armino code and toolchain download
 - Setup software development environment
 - How to configure Armino project
 - How to build the project and flash the bin to board

Introduction
--------------------------------------------------------

BK7236 is a SoC that integrates the following features:

 - Wi-Fi 6 (2.4 GHz Band)
 - Bluetooth Low Energy 5.4
 - ARMv8-M Star (M33F) MCU
 - Multiple peripherals
 - Built-in TrustEngine security hardware

Powered by 22nm technology, BK7236 provides a robust, highly integrated platform, which helps
meet the continuous demands for efficient power usage, compact design, security, high performance,
reliability and high security.

Beken provides basic hardware and software resources to help application developers realize
their ideas using BK7236 series hardware. The software development framework Armino is
intended for development of Internet-of-Things (IoT) applications with Wi-Fi, Bluetooth, power
management, security and other system features.

Preparation
--------------------------------------------------------

Hardware：

 - BK7236 Demo board
 - BKFIL.exe
 - PC（Windows & Ubuntu）

.. note::

  Armino supports compilation on the Linux platform and also supports compilation on the Linux virtual machine on the Windows platform;
  Armino supports firmware burning on Windows/Linux platforms (refer to the guidance document in the burning tool).

Software：

 - GCC ARM tool chain, used to compile BK7236
 - Build tools, including CMake
 - Armino source code
 - BKFIL.exe


Introduction to Development Board
--------------------------------------------------------

Click the following link to learn more about the development boards supported by Armino:

.. toctree::
    :maxdepth: 1

        BK7236 <bk7236>

Armino SDK Code download
--------------------------------------------------------------------

We can download Armino from gitlab::

    mkdir -p ~/armino
    cd ~/armino
    git clone https://gitlab.bekencorp.com/armino/bk_idk.git


We also can download Armino from github::

	mkdir -p ~/armino
	cd ~/armino
	git clone https://github.com/bekencorp/bk_idk.git	


Then switch to the stable branch Tag node, such as v2.0.1.12::

    git checkout -B your_branch_name v2.0.1.12

.. note::

    The gitlab always has the latest Armino code, only authorised account can download the code. Please contact the your BK7236 project owner to get relevant accounts.



Setup Build Environment
--------------------------------------------------------------------

.. note::

      The Armino compilation environment requires Ubuntu 20.04 LTS version and above. This chapter will take Ubuntu 20.04 LTS version as an example to introduce the construction of the entire compilation environment.


Tool Chain Installation
----------------------------------------------------------------

Click `Donwload <https://dl.bekencorp.com/tools/toolchain/arm/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2>`_ to download the BK7236 toolchain.

After downloading the tool kit, decompress it to '/opt/'::

    $ sudo tar -xvjf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 -C /opt/


.. note::

    Tool chain the default path is configured in the middleware/soc/bk7236/bk7236.defconfig, you can modify ``CONFIG_TOOLCHAIN_PATH`` to set to your owner toolchain path:

    CONFIG_TOOLCHAIN_PATH="/opt/gcc-arm-none-eabi-10.3-2021.10/bin"


Depended libraries installation
-----------------------------------------------------------------

Enter the following command in the terminal to install python\CMake\Ninja::

    sudo dpkg --add-architecture i386
    sudo apt-get update
    sudo apt-get install build-essential cmake python3 python3-pip doxygen ninja-build libc6:i386 libstdc++6:i386 libncurses5-dev lib32z1 -y
    sudo pip3 install pycryptodome click

Document Compilation Dependent Library Installation
------------------------------------------------------------------------------

Click `Doc <https://docs.bekencorp.com/arminodoc/bk_idk/bk7236/zh_CN/v2.0.1/index.html>`_ you can get the latest Armino documents. So you don't need to build the document youself.

If you really need to build the documents, following python3 packages are required::

    sudo pip3 install sphinx_rtd_theme future breathe blockdiag sphinxcontrib-seqdiag sphinxcontrib-actdiag sphinxcontrib-nwdiag sphinxcontrib.blockdiag


If you default Python is Python2, please set it to Python3::

    sudo ln -s /usr/bin/python3 /usr/bin/python


Build The Project
------------------------------------

Run following commands to build BK7236 app project::

    cd ~/armino/bk_idk
    make bk7236

The app project startup:

 - WiFi/BLE
 - Common pheripheral drivers
 - Armino default Cli

You can build other projects with PROJECT parameter, e.g. run "make bk7236 PROJECT=security/secureboot" 
can build projects/security/secureboot etc.

Configuration project
------------------------------------

We can also use the project configuration file for differentiated configuration::

    Project Profile Override Chip Profile Override Default Configuration
    Example： bk7236/config >> bk7236.defconfig >> KConfig
    + Example of project configuration file：
        projects/app/config/bk7236/config
    + Sample chip configuration file：
        middleware/soc/bk7236/bk7236.defconfig
    + Sample KConfig configuration file：
        middleware/arch/cm33/Kconfig
        components/bk_cli/Kconfig

Click :ref:`Kconfig Configuration <bk_config_kconfig>` to learn more about Armino Kconfig.

New project
------------------------------------

The default project is projects/app. For new projects, please refer to projects/app project


Burn Code
------------------------------------

On the Windows platform, Armino currently supports UART burning.



Burn through serial port
****************************************

.. note::

    Armino supports UART burning. It is recommended to use the CH340 serial port tool board to download.

Serial port burning tool is shown in the figure below:

.. figure:: ../../_static/download_tool_uart.png
    :align: center
    :alt: Uart
    :figclass: align-center

    UART

Download burning tools (BKFILL.exe)：

	http://dl.bekencorp.com/tools/flash/
	Get the latest version in this directory. Ex：BEKEN_BKFIL_V2.1.6.0_20231123.zip

The snapshot of BKFILL.exe downloading.

.. figure:: ../../_static/download_uart_bk7236_en.png
    :align: center
    :alt: BKFIL GUI
    :figclass: align-center

    BKFIL GUI


Burn the serial port DL_UART0, click ``Download`` to burn the image, and then power down and restart the device after burning.

Click :ref:`BKFIL <bk_tool_bkfil>` to learn more about BKFIL.

Serial port Log and Command Line
------------------------------------

Currently the BK7236 use the DL_UART0 as the Log output and Cli input; You can get the supported command list through the help command.
