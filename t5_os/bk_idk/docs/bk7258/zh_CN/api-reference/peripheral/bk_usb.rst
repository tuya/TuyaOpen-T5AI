USB
================

:link_to_translation:`en:[English]`


.. toctree::
   :maxdepth: 1

   USB Host UVC+UAC <usb/bk_usbh_uvc_uac>


USB 简介
------------------
USB（Universal Serial Bus）是一种通用的总线标准，用于连接主机和外设设备。USB 主机可以通过 USB 接口与 USB 设备连接，实现数据传输、电源供给等功能。

USB IF（USB Implementers Forum）是 USB 标准的制定者，它制定了 USB 标准，包括 USB 1.1、USB 2.0、USB 3.0 等，定义了 USB 接口的物理层、数据链路层、传输层、会话层、表示层等协议，以及 USB 设备类（Device Class）标准，常见的设备类包括 HID（Human Interface Device，人机接口设备）、MSC（Mass Storage Class，大容量存储设备）、CDC（Communication Device Class，通信设备）、Audio、Video 等。

博通BK7258芯片内置 USB-OTG外设，支持各种各样的 USB 应用，包括 USB 多媒体类应用，USB 通信类应用，USB 存储类应用，USB 人机交互类应用等。

USB 基本使用
------------------

.. figure:: ../../../_static/usb/usb_open_close.png
    :align: center
    :alt: usb open close 
    :figclass: align-center

    USB open/close

USB 测试示例
------------------

注：BK7258 USB 默认运行在CPU2，使用默认avdk调试需要连接UART2协助输入命令测试，依赖CLI命令打开编译后调试。 测试目标：USB模块开关正常，枚举正常，解析正常，断开/连接等状态记录正常。

作为HOST 识别枚举Video+Audio融合设备为例，输入命令和LOG信息如下图所示：

1、在默认串口输入 bootcore 2 1 启动CPU2 ，log如下

.. figure:: ../../../_static/usb/cli_boot_cpu2.png
    :align: center
    :alt: usb open close 
    :figclass: align-center

    CLI Boot CPU2

2、uart2 串口输入usb power 1c 1，log如下

.. figure:: ../../../_static/usb/cli_usb_power_on.png
    :align: center
    :alt: usb power on 
    :figclass: align-center

    CLI USB PowerON

3、uart2 串口输入usb open_host，枚举成功log如下

.. figure:: ../../../_static/usb/cli_usb_open_host.png
    :align: center
    :alt: usb open_host
    :figclass: align-center

    CLI USB Open Host

4、uart2 串口输入usb check_dev 0(是否支持video)/usb check_dev 1(是否支持mic)/usb check_dev 2(是否支持speaker)，支持需要的类设备log如下

.. figure:: ../../../_static/usb/cli_usb_check_support_dev.png
    :align: center
    :alt: usb open_host
    :figclass: align-center

    CLI USB Check video/mic/speaker

5、uart2 串口输入usb close,log如下

.. figure:: ../../../_static/usb/cli_usb_close.png
    :align: center
    :alt: usb open_host
    :figclass: align-center

    CLI USB Close

6、uart2 串口输入usb power 1c 0,log如下

.. figure:: ../../../_static/usb/cli_usb_power_down.png
    :align: center
    :alt: usb open_host
    :figclass: align-center

    CLI USB PowerDown


USB API Status
------------------

+---------------------------------------------+-------------+
| API                                         | BK7258_cp2  |
+=============================================+=============+
| :cpp:func:`bk_usb_driver_init`              | Y           |
+---------------------------------------------+-------------+
| :cpp:func:`bk_usb_driver_deinit`            | Y           |
+---------------------------------------------+-------------+
| :cpp:func:`bk_usb_power_ops`                | Y           |
+---------------------------------------------+-------------+
| :cpp:func:`bk_usb_open`                     | Y           |
+---------------------------------------------+-------------+
| :cpp:func:`bk_usb_close`                    | Y           |
+---------------------------------------------+-------------+
| :cpp:func:`bk_usb_control_transfer_init`    | Y           |
+---------------------------------------------+-------------+
| :cpp:func:`bk_usb_control_transfer_deinit`  | Y           |
+---------------------------------------------+-------------+
| :cpp:func:`bk_usb_control_transfer`         | Y           |
+---------------------------------------------+-------------+
| :cpp:func:`bk_usb_device_set_using_status`  | Y           |
+---------------------------------------------+-------------+
| :cpp:func:`bk_usb_get_device_connect_status`| Y           |
+---------------------------------------------+-------------+
| :cpp:func:`bk_usb_check_device_supported`   | Y           |
+---------------------------------------------+-------------+

USB Port Number
------------------------

+-------------+-------------+-------------+
| Capability  | BK7258_cp1  | BK7258_cp2  |
+=============+=============+=============+
| Port Number | 1           | 1           |
+-------------+-------------+-------------+

USB Pin and GPIO Map
---------------------------------

+----------+-----------+-----------+
| USB Pin  | BK7258_cp1| BK7258_cp2|
+==========+===========+===========+
| USB DP   | 12        | 12        |
+----------+-----------+-----------+
| USB DN   | 13        | 13        |
+----------+-----------+-----------+
| USB power| 28        | 28        |
+----------+-----------+-----------+

USB API Reference
---------------------

.. include:: ../../_build/inc/usb.inc

USB API Typedefs
---------------------

.. include:: ../../_build/inc/usb_types.inc

