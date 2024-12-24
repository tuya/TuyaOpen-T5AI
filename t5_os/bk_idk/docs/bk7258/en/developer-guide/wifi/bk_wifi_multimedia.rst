**Armino Wi-Fi Video and Audio Transmission User Guide**
===========================================================

:link_to_translation:`en:[English]`

**Overview**
--------------
Audio and video transmission needs to ensure the real-time and integrity of the transmitted information. When the audio and video data streams are transmitted using Wi-Fi, ARMINO SDK has made specific optimizations for this scenario in order to ensure the transmission quality. This chapter introduces the API related to Wi-Fi audio and video transmission and the usage guide.

**APIs Introduction**
-----------------------------
Wi-Fi audio and video transmission related interfaces are mainly of the following two types:

 - Enable/Query Wi-Fi audio/video transmission policy APIs

::

    bk_err_t bk_wifi_set_wifi_media_mode(bool flag);
    bk_err_t bk_wifi_get_wifi_media_mode_config(bool *flag);

Call bk_wifi_set_wifi_media_mode() interface to enable the Wi-Fi audio/video transmission policy, and call bk_wifi_get_wifi_media_mode_config() interface to query whether the policy is configured to take effect.

 - Enable/Query Wi-Fi audio/video transmission anti-interference APIs

::

    bk_err_t bk_wifi_set_video_quality(uint8_t quality);
    bk_err_t bk_wifi_get_video_quality_config(uint8_t *quality);

Call bk_wifi_set_video_quality() interface to turn on the audio/video transmission anti-interference selection for audio/video smoothness in complex environment. Call the bk_wifi_get_video_quality_config() interface to query whether the policy is in effect.

The macro definitions related to the immunity interface are shown in the following table:

+-------------------------------+----------------------------------------------+
| **Macro Definition**          | **Description**                              |
+===============================+==============================================+
| WIFI_VIDEO_QUALITY_FD         | video quality choose Fluent definition(FD)   |
+-------------------------------+----------------------------------------------+
| WIFI_VIDEO_QUALITY_SD         | video quality choose Standard definition(SD) |
+-------------------------------+----------------------------------------------+
| WIFI_VIDEO_QUALITY_HD         | video quality choose High definition(HD)     |
+-------------------------------+----------------------------------------------+

**APIs Usage**
**********************

When using the Wi-Fi audio/video function, please call the audio/video transmission policy APIs during audio/video initialization and before Wi-Fi connection:

   bk_err_t bk_wifi_set_wifi_media_mode(bool flag);

This interface is a mandatory option to optimize the real-time and integrity of audio/video transmission information, and is configured not to use this policy by default.
If the above interface is called before turning on audio/video, please call the above interface after turning off Wi-Fi audio/video to configure it as:

    bk_wifi_set_wifi_media_mode(false);

.. important::
    *Wi-Fi power consumption will be affected if the above modes are not configured when the video and audio function turned off.*

If the audio/video operating environment is quite complex, you can choose to configure the audio/video transmission policy interface along with the audio/video transmission anti-interference interface:

    bk_err_t bk_wifi_set_video_quality(uint8_t quality);

This interface is Wi-Fi audio/video selectable with three stops to choose from:

 - Fluent Definition(FD): This gear has strong anti-interference ability, which can ensure the stable transmission of audio and video in more complicated environment;
 - Standard Definition(SD): This gear in a relatively clean environment can be a stable transmission, in a relatively complex environment, audio and video have a slight stutter;
 - High Definition(HD): This gear priority to ensure high bandwidth audio and video transmission, suitable for cleaner environments, complex environments with audio and video lag phenomenon;

It is recommended to choose the appropriate gear according to the product use environment to ensure the smoothness and stability of audio and video transmission.
If the above interface is called before turning on the audio/video, please call the above interface after turning off the Wi-Fi audio/video to configure it as:

    bk_wifi_set_video_quality(WIFI_VIDEO_QUALITY_HD);

It has been ensured that the ARMINO SDK does not affect its strategic transmission when switching to other functional modes;

.. important::
    *The audio/video transmission anti-interference APIs is based on the Wi-Fi audio/video transmission policy APIs to take effect, the audio/video transmission anti-interference APIs configuration does not take effect by calling it alone.*

**Reference Materials**
**************************
  Please refer to the following link for the description of the specific APIs of Wi-Fi audio and video transmission：

 - :cpp:func:`bk_wifi_set_wifi_media_mode`
 - :cpp:func:`bk_wifi_set_video_quality`
 - :cpp:func:`bk_wifi_get_video_quality_config`
 - :cpp:func:`bk_wifi_get_wifi_media_mode_config`
