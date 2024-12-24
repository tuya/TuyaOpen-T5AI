*********************************
**BEKEN Wi-Fi 抓包指南**
*********************************

=====================================
**卓越的网络分析工具——OmniPeek**
=====================================

**1. 初识 Omnipeek**
-----------------------------

OmniPeek 被誉为世界上功能最强大的网络协议分析器，原属于世界领先的数据包捕获和分析 Savvius 公司的产品，（以前称为 WildPackets）在数据包捕获，深度数据包检测和网络诊断解决方案领域领先 25 年，现已被 Liveaction 收购。Liveaction 之前主要针对网络数据流级别的分析，收购 Savvius 之后，两者整合形成互补实现了从流到数据包的全面的分析平台。OmniPeek 能对数据包进行深入的解析，分析上千种协议，从多个维度全方位的分析网络流量，提供数十种可直观的图表报告，提供专家级网络事件的分析和诊断。

**1.1 OmniPeek 面板**
++++++++++++++++++++++++++++++

.. figure:: ../../../_static/omni_panel.png
   :align: center

   Omnipeek 界面

OmniPeek 提供了多种维度的分析方法和图表供使用，上图展示了指南针面板，该面板是一个交互式的取证的仪表板，显示网络随时间的利用率，包括事件、协议、流、节点、信道、VLAN、数据速率、应用程序和国家统计信息。这些统计信息显示在可选的数据源窗口小部件中，可以从实时捕获或从单个捕获文件中查看。

**1.2 OmniPeek 应用程序分析**
+++++++++++++++++++++++++++++++++

"应用程序"仪表板在捕获窗口中显示应用程序的关键统计信息。此应用程序可见性可洞悉一天、一周、一个月或一年中特定时间的网络上的用户行为和流量模式。它可以帮助分析人员更好地了解谁将访问哪个网站以及何时使用哪些应用程序。

**1.3 OmniPeek 统计分析**
+++++++++++++++++++++++++++++++

.. figure:: ../../../_static/omni_graphs.png
   :align: center

   Omnipeek 图表界面

OmniPeek 和捕获引擎实时计算各种关键统计数据，并以直观的图形显示形式呈现这些统计数据。您可以采用多种格式保存、复制、打印或自动生成有关这些统计信息的定期报告。

**1.4 总结**
++++++++++++++++++

OmniPeek 是功能强大的网络分析工具，对于初学者也能轻松上手，但是由于它的功能太过丰富，上面简短的介绍无法让大家了解透彻，更多细节部分请访问官方网站了解 `Onmipeek官方网站 <https://www.liveaction.com/>`_。

**2. OmniPeek 驱动**
---------------------------

根据软件安装目录 ``Omnipeek\Drivers`` 中的驱动信息，目前 OmniPeek 可以支持两类抓包网卡，一类基于 Atheros 芯片 ``Atheros USB 802.11n Wireless LAN card``，只支持 11N 模式；一类基于 ``Ralink rt2870`` ，可以支持 802.11bgn/ac。推荐使用 ``ASUS USB-AC55`` 或者 ``Netgear A6210`` 两款 USB 抓包网卡。

**3. 抓包场景**
----------------------------

- Wi-Fi 连接问题，通过抓包分析关键帧
- Wi-Fi 性能问题，通过抓包分析重传、聚合度、Phy Rate 等
- Wi-Fi 功耗问题，通过抓包分析 PM 值确认当前的休眠唤醒状态

**4. 常见问题**
------------------------------

**4.1 OmniPeek 抓包示例**
++++++++++++++++++++++++++++++

- **扫描阶段**

.. figure:: ../../../_static/scan.png
   :align: center

   扫描阶段

- **认证阶段**

.. figure:: ../../../_static/auth.png
   :align: center

   认证阶段

- **四次握手阶段**

.. figure:: ../../../_static/eapol.png
   :align: center

   四次握手阶段

- **解密**

若能抓到上述完整的认证过程，可以通过 ``Tools-Decrypt WLAN Packets`` 输入 SSID 和密码进行解密。

.. figure:: ../../../_static/decrypt.png
   :align: center

   解密加密包

===========================
**Wireshark 使用指南**
===========================

**1. 概述**
---------------------------

**1.1 Wireshark 是什么？**
++++++++++++++++++++++++++++++

Wireshark 是一款自由开源的网络协议分析器，它在全球被广泛使用。通过 Wireshark，你可以实时捕获网络的传入和传出数据包，并将其用于网络故障排除、数据包分析、软件和通信协议开发等。它适用于所有主流的桌面操作系统，如 Windows、Linux、macOS、BSD等。

**1.2 Wireshark 常见应用场景**
++++++++++++++++++++++++++++++++

- 网络管理员会使用 Wireshark 来检查网络问题
- 软件测试工程师会使用 Wireshark 抓包，来分析自己测试的软件
- 从事 socket 编程的工程师会使用 Wireshark 来调试
- 运维人员用于日常工作，应急响应等
- 无线网络协议学习、测试、开发等

总之，与网络相关的东西，都可能会用到 Wireshark。

**1.3 Wireshark 特点**
+++++++++++++++++++++++++++++

- 支持多平台，如 Windows、Linux、macOS等
- 支持超过 1000 种协议，从基础的 IP、DHCP 协议到高级的专用协议
- 创建多种统计分析并支持图表显示
- 开源、免费
- 清晰的菜单栏和简明的布局，增强了实用性，通过不同过滤不同色彩协议
- 其他等等

**2. Wi-Fi6 无线抓包指南**
-----------------------------------------------

Wireshark 是世界上最重要和最广泛使用的网络协议分析器。它可以让你在微观层面上看到网络上发生了什么。Wi-Fi6 也被称为 802.11ax ，该标准建立在 802.11ac 的基础上，同时增加了效率、灵活性和可伸缩性，允许新网络和现有网络通过下一代应用程序提高速度和容量。Intel AX200/AX210 适配器是为支持 IEEE-802.11ax 标准（Wi-Fi6 技术）和 Wi-Fi 联盟认证而设计。该产品支持 2×2 的 Wi-Fi6 技术，包括 UL/DL OFDMA 和 1024QAM 等新功能，提供高达 2.4Gbps 的数据速率和增加的网络容量。

**2.1 硬件需求**
++++++++++++++++++++++++++

- 运行 Ubuntu 18.04 或以上的笔记本电脑
- 安装 Intel AX200/AX210 802.11ax 无线网卡的笔记本电脑

**2.2 固件安装**
+++++++++++++++++++++++++++

::

    1. sudo apt-get update -y
    2. sudo apt-get upgrade -y

系统更新后，重新启动系统以应用更改。由于系统当前 Kernel 并不包含最新的驱动程序，你可以选择升级 Linux 内核 5.1 或者安装新的驱动程序，二选一，并使用最新的无线适配器固件。升级 Linux 内核需要升级到最新的 Linux 内核 >5.1，并使用最新的无线适配器固件。下载和安装内核的官方网站:

::

    cd /tmp/
    wget -c https://kernel.ubuntu.com/~kernel-ppa/mainline/v5.1/linux-headers-5.1.0-050100_5.1.0-050100.201905052130_all.deb
    wget -c https://kernel.ubuntu.com/~kernel-ppa/mainline/v5.1/linux-headers-5.1.0-050100-generic_5.1.0-050100.201905052130_amd64.deb
    wget -c https://kernel.ubuntu.com/~kernel-ppa/mainline/v5.1/linux-image-unsigned-5.1.0-050100-generic_5.1.0-050100.201905052130_amd64.deb
    wget -c https://kernel.ubuntu.com/~kernel-ppa/mainline/v5.1/linux-modules-5.1.0-050100-generic_5.1.0-050100.201905052130_amd64.deb
    sudo dpkg -i *.deb

安装成功后，重新启动 ubuntu 系统
::

    sudo reboot

然后检查 Linux 内核版本
::

    uname -a

升级 Wi-Fi 驱动程序，请参考如下命令：
::

    git clone --single-branch --branch release/core45 https://git.kernel.org/pub/scm/linux/kernel/git/iwlwifi/backport-iwlwifi.git
    cd backport-iwlwifi/
    make defconfig-iwlwifi-public
    sed -i 's/CPTCFG_IWLMVM_VENDOR_CMDS=y/# CPTCFG_IWLMVM_VENDOR_CMDS is not set/' .config
    make -j4
    sudo make install

安装 Intel AX200/AX210 无线网络最新固件。因为驱动程序本身没有进入 Ubuntu18.04 和 Ubuntu19.04 Linux 内核的内核。所以需要手动安装 AX200/AX210 的固件。访问下面的连接 `Intel 固件 <https://www.intel.com/content/www/us/en/support/articles/000005511/network-and-i-o/wireless-networking.html>`_ 安装固件步骤如下：

- 将文件复制到特定于发行版的固件目录/lib/固件中
- 如果目录不工作，请参考您的分发文档
- 如果您自己配置内核，请确保启用了固件加载

**2.3 软件安装**
--------------------------

- 将无线适配器配置为监视模式

为了查看 802.11 头文件，您必须在监视器模式下捕获。为接口手动打开或关闭监视模式的最简单方法是使用 aircrack-ng 中的 airmon-ng 脚本；您的发行版可能已经有了一个用于 aircrack-ng 的包，所以我们需要先安装气裂包。

::

    sudo apt-get install build-essential autoconf automake libtool pkg-config libnl-3-dev libnl-genl-3-dev libssl-dev ethtool shtool rfkill zlib1g-dev libpcap-dev libsqlite3-dev libpcre3-dev libhwloc-dev libcmocka-dev hostapd wpasupplicant tcpdump screen iw usbutils
    sudo apt-get install aircrack-ng

Note that the behavior of airmon-ng will differ between drivers that support the new mac80211 framework and drivers that don’t. For drivers that support it, a command such as
请注意，在支持新 mac80211 框架的驱动程序和不支持新框架的驱动程序之间，airmon-ng 的行为将有所不同。

::

    sudo airmon-ng start wlan0

然后 Linux 终端将产生输出：（例子，仅作参考）
::

    1. Interface Chipset Driver
    2. wlan0 Intel 4965 a/b/g/n iwl4965 – [phy0]
    3. (monitor mode enabled on mon0)

"在 mon0 上启用的监视模式"意味着必须在 "mon0" 接口上捕获，而不是在 "wlan0" 接口上捕获。若要关闭监视器模式，请使用如下命令：
::

    sudo airmon-ng stop mon0

不能用如下命令
::

    sudo airmon-ng stop wlan0

- 安装 Wireshark

安装 Wireshark 开发版本以支持最新的 Wi-Fi6 规范，添加 PPA 存储库并安装 Wireshark
::

    1. sudo add-apt-repository ppa:wireshark-dev/stable
    2. sudo apt update
    3. sudo apt -y install wireshark
    4. sudo apt -y install wireshark-qt

安装 Wireshark 开发版本以获得开发版本，添加 sudo add-apt-repository ppa:dreibh/ppa ，从存储库安装 Wireshark
::

    1. sudo apt update
    2. sudo apt -y install wireshark

- 启动 Wireshark

::

    sudo wireshark

抓取无线空口包并检查，是否包含 11ax 信标帧，如果可以正常解析，安装成功。


:link_to_translation:`en:[English]`

