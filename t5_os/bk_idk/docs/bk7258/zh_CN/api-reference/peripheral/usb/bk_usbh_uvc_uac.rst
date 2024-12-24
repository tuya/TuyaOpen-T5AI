USB Host UVC+UAC
================

:link_to_translation:`en:[English]`

uvc_uac_api_common是BK7258的 USB UVC + UAC 主机驱动程序，支持从 USB 设备读取/写入/控制多媒体流。当前最多同时支持 1 路摄像头 + 1 路麦克风 + 1 路播放器数据流。
具体的综合使用示例，请参考avdk多媒体代码，谢谢。

UVC 主机驱动基本使用流程
------------------------------------

.. figure:: ../../../../_static/usb/uvc_start_stop.png
    :align: center
    :alt: uvc start stop
    :figclass: align-center

    UVC start/stop


UVC 主机驱动CLI测试示例
------------------------------------

注：BK7258 USB 默认运行在CPU2，使用默认avdk调试需要连接UART2协助输入命令测试。 测试目标：UVC 获得参数正常，设置参数正常，Buffer管理正常，启动传输正常，关闭传输正常。

作为HOST 运行Video设备为例，输入命令和LOG信息如下图所示：

.. figure:: ../../../../_static/usb/cli_uvc_connect_disconnect.png
    :align: center
    :alt: uvc connect disconnect
    :figclass: align-center

    UVC Connect/Disconnect

.. figure:: ../../../../_static/usb/cli_uvc_get_param.png
    :align: center
    :alt: uvc get param 
    :figclass: align-center

    UVC Get Param

.. figure:: ../../../../_static/usb/cli_uvc_set_param.png
    :align: center
    :alt: uvc set param 
    :figclass: align-center

    UVC Set Param

注：根据获得参数来设置参数，设置最大包长1024字节，分辨率1280X720 索引1，帧率30fps，视频数据格式索引1(实例中1为MJPG)。

.. figure:: ../../../../_static/usb/cli_uvc_register_buffer_ops.png
    :align: center
    :alt: uvc reg buffer ops
    :figclass: align-center

    UVC Register Buffer ops

注：代码参考视频多媒体，buffer管理部分。

.. figure:: ../../../../_static/usb/cli_uvc_start.png
    :align: center
    :alt: uvc start
    :figclass: align-center

    UVC Start

.. figure:: ../../../../_static/usb/cli_uvc_stop.png
    :align: center
    :alt: uvc stop
    :figclass: align-center

    UVC Stop


UAC 主机驱动基本使用流程
------------------------------------

.. figure:: ../../../../_static/usb/uac_start_stop.png
    :align: center
    :alt: usb open close 
    :figclass: align-center

    UAC start/stop


UAC 主机驱动CLI测试示例
------------------------------------

注：BK7258 USB 默认运行在CPU2，使用默认avdk调试需要连接UART2协助输入命令测试。 测试目标：UVC 获得参数正常，设置参数正常，Buffer管理正常，启动传输正常，关闭传输正常。

作为HOST 运行Video+Audio融合设备为例，输入命令和LOG信息如下图所示：

.. figure:: ../../../../_static/usb/cli_uac_connect_disconnect.png
    :align: center
    :alt: uvc connect disconnect
    :figclass: align-center

    UAC Connect/Disconnect

.. figure:: ../../../../_static/usb/cli_uac_get_param.png
    :align: center
    :alt: uvc get param 
    :figclass: align-center

    UAC Get Param

.. figure:: ../../../../_static/usb/cli_uac_set_param.png
    :align: center
    :alt: uvc set param 
    :figclass: align-center

    UAC Set Param

注：根据获得参数来设置参数，设置采样率，音频格式。

.. figure:: ../../../../_static/usb/cli_uac_register_buffer_ops.png
    :align: center
    :alt: uvc reg buffer ops
    :figclass: align-center

    UAC Register Buffer ops

注：代码参考音频多媒体，buffer管理部分。

.. figure:: ../../../../_static/usb/cli_uac_mic_start.png
    :align: center
    :alt: uvc mic start
    :figclass: align-center

    UAC MIC Start

.. figure:: ../../../../_static/usb/cli_uac_mic_stop.png
    :align: center
    :alt: uvc mic stop
    :figclass: align-center

    UAC MIC Stop

.. figure:: ../../../../_static/usb/cli_uac_speaker_start.png
    :align: center
    :alt: uvc start
    :figclass: align-center

    UAC SPK Start

.. figure:: ../../../../_static/usb/cli_uac_speaker_stop.png
    :align: center
    :alt: uvc stop
    :figclass: align-center

    UAC SPK Stop



USB UVC UAC Reference
---------------------

.. include:: ../../../_build/inc/uvc_uac_api_common.inc