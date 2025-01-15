设置RTC作为唤醒源，系统进入深度睡眠和低压睡眠说明
=================================================

:link_to_translation:`en:[English]`

设置RTC作为唤醒源，系统进入深度睡眠调用函数和调用顺序说明
---------------------------------------------------------
- 1. 配置RTC唤醒源的参数

    alarm_info_t low_valtage_alarm = {

                                      "low_vol",

                                      3000*RTC_TICKS_PER_1MS,//设置为3000ms后唤醒，唤醒时间可以根据需求更改

                                      1,

                                      cli_pm_rtc_callback,

                                      NULL

                                      };

    bk_alarm_register(AON_RTC_ID_1, &low_valtage_alarm);


- 2. bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL);

    向系统中设置RTC方式作为唤醒源

- 3. bk_pm_sleep_mode_set(PM_MODE_DEEP_SLEEP);

    设置睡眠模式，进入深度睡眠


设置RTC作为唤醒源，系统进入超深度睡眠调用函数和调用顺序说明
-----------------------------------------------------------
- 1. bk_rtc_ana_register_wakeup_source(0x5)

    设置睡眠时间，支持的时间设置如下：

::

    0x0:  1/32 s
    0x1:  1/16 s
    0x2:  1/8  s
    \.\.\.
    0x5:  1    s
    0x6:  2    s
    0x7:  4    s
    \.\.\.
    0x13: 256  s
    0x14: 512  s
    0x15: 1024 s

- 2. bk_pm_sleep_mode_set(PM_MODE_SUPER_DEEP_SLEEP);

    设置睡眠模式，进入超深度睡眠


设置RTC作为唤醒源，系统进入低压睡眠调用函数和调用顺序说明
---------------------------------------------------------------------
- 1. 配置RTC唤醒源的参数;

    alarm_info_t low_valtage_alarm = {

                                      "low_vol",

                                      3000*RTC_TICKS_PER_1MS,//设置为3000ms后唤醒，唤醒时间可以根据需求更改

                                      1,

                                      cli_pm_rtc_callback,

                                      NULL

                                      };

    bk_alarm_register(AON_RTC_ID_1, &low_valtage_alarm);


- 2. bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL);

    向系统中设置RTC方式作为唤醒源

- 3. bk_pm_sleep_mode_set(PM_MODE_LOW_VOLTAGE);

    设置睡眠模式，进入低压睡眠
    (备注：当前系统默认设置为低压睡眠，只需要满足投的票，就可以进入低压睡眠)

- 4. bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x1,0x0);

    向系统中投应用程序的票

备注:

 - 1. BT和WIFI的票，BT和WIFI模块内部进入睡眠后自己投上，SDK内部做好，应用程序不用关注

 - 2. 当系统中BT或WIFI模块没有上电时，系统会自动给这两个模块投上sleep的票。

 - 3. 最求最优功耗时，当系统开启音频，视频，LCD，CPU1，CPU2后，进入低压睡眠前需要把音频，视频，LCD，CPU1,CPU2先关闭，再进入睡眠。