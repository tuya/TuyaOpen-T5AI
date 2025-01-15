**Armino Wi-Fi音视频传输使用指南**
===================================

:link_to_translation:`en:[English]`

**概述**
----------
音视频传输需要保证传输信息的实时性、完整性，当音视频数据流使用Wi-Fi传输时，为了保证传输质量，ARMINO SDK对该场景做了特定优化，本章节介绍Wi-Fi音视频传输相关接口及使用指南。

**接口介绍**
-----------------------------
Wi-Fi音视频传输相关接口主要有如下两类：

 - 开启/查询Wi-Fi音视频传输策略接口

::

    bk_err_t bk_wifi_set_wifi_media_mode(bool flag);
    bk_err_t bk_wifi_get_wifi_media_mode_config(bool *flag);

调用 bk_wifi_set_wifi_media_mode()接口开启Wi-Fi音视频传输策略，调用bk_wifi_get_wifi_media_mode_config()接口查询该策略是否配置生效。

 - 开启/查询音视频传输抗干扰接口

::

    bk_err_t bk_wifi_set_video_quality(uint8_t quality);
    bk_err_t bk_wifi_get_video_quality_config(uint8_t *quality);

调用bk_wifi_set_video_quality()接口开启音视频传输抗干扰性选择，以适应复杂环境的音视频流畅性。调用bk_wifi_get_video_quality_config()接口查询该策略是否生效。

抗干扰性接口相关宏定义如下表所示：

+-------------------------------+----------------------------------------------+
| **宏定义**                    | **描述**                                     |
+===============================+==============================================+
| WIFI_VIDEO_QUALITY_FD         | video quality choose Fluent definition(FD)   |
+-------------------------------+----------------------------------------------+
| WIFI_VIDEO_QUALITY_SD         | video quality choose Standard definition(SD) |
+-------------------------------+----------------------------------------------+
| WIFI_VIDEO_QUALITY_HD         | video quality choose High definition(HD)     |
+-------------------------------+----------------------------------------------+

**接口使用**
**********************

使用Wi-Fi音视频功能时，请在音视频初始化时，Wi-Fi连接之前调用音视频传输策略接口：

   bk_err_t bk_wifi_set_wifi_media_mode(bool flag);

该接口是优化音视频传输信息的实时性，完整性的必选项，默认配置为不使用该策略。
如果在开启音视频前调用了上述接口，请在关闭Wi-Fi音视频之后调用上述接口配置为：

    bk_wifi_set_wifi_media_mode(false);

.. important::
    *若在关闭时未配置上述模式会影响Wi-Fi功耗*

如果音视频运行环境相当复杂，可以在配置音视频传输策略接口的同时选择配置音视频传输抗干扰接口：

    bk_err_t bk_wifi_set_video_quality(uint8_t quality);

该接口为Wi-Fi音视频可选项，有三个挡位可选择：

 - 流畅(FD): 该挡位有较强的抗干扰能力，可以在较复杂环境下保证音视频的稳定传输；
 - 标准(SD): 该挡位在相对干净环境下可以稳定传输，在相对复杂环境下音视频有轻微卡顿；
 - 高清(HD): 该挡位优先保证高带宽音视频传输，适用于较干净的环境，复杂环境下音视频有卡顿现象；

建议根据产品使用环境选择合适的挡位以保证音视频传输的流畅性和稳定性。
如果在开启音视频前调用了上述接口，请在关闭Wi-Fi音视频之后调用上述接口配置为：

    bk_wifi_set_video_quality(WIFI_VIDEO_QUALITY_HD);

已确保ARMINO SDK 切换到其它功能模式时不影响其策略性传输，默认情况下为高清(HD)挡位；

.. important::
    *音视频传输抗干扰接口是基于Wi-Fi音视频传输策略接口生效的，单独调用音视频传输抗干扰接口配置不生效。*

**参考链接**
**************************
  Wi-Fi 音视频传输具体API接口介绍说明请参阅如下链接：

 - :cpp:func:`bk_wifi_set_wifi_media_mode`
 - :cpp:func:`bk_wifi_set_video_quality`
 - :cpp:func:`bk_wifi_get_video_quality_config`
 - :cpp:func:`bk_wifi_get_wifi_media_mode_config`
