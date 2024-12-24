enter deep sleep or low voltage sleep mode,  using the RTC wakeup source description
============================================================================================

:link_to_translation:`zh_CN:[中文]`

enter deep sleep , using the RTC wakeup source description
----------------------------------------------------------------

- 1. Configure the parameters of the RTC wakeup source

    alarm_info_t low_valtage_alarm = {

                                      "low_vol",

                                      3000*RTC_TICKS_PER_1MS,//eg:set wakeup time: 3000ms

                                      1,

                                      cli_pm_rtc_callback,

                                      NULL

                                      };

    bk_alarm_register(AON_RTC_ID_1, &low_valtage_alarm);


- 2. bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL);

comment：set wakeup source(RTC)


- 3. bk_pm_sleep_mode_set(PM_MODE_DEEP_SLEEP);

comment：set sleep mode(deep sleep), it will enter deep sleep

comment::
 - 1. if the WIFI and BT are working, the SDK will stop the WIFI and BT work, WIFI and BT will enter sleep before entering deep sleep.
 - 2. if you open mult-media(audio,video) before entering deep sleep,you should close them before entering deep sleep.


enter super deep sleep , using the RTC wakeup source description
-------------------------------------------------------------------

- 1. bk_rtc_ana_register_wakeup_source(0x5)

comments：set sleep time, input value supported as bellow

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

comment：set sleep mode(super deep sleep)

comment::
 - 1. if the WIFI and BT are working, the SDK will stop the WIFI and BT work, WIFI and BT will enter sleep before entering super deep sleep.
 - 2. if you open mult-media(audio,video) before entering super deep sleep,you should close them before entering super deep sleep.


enter low voltage sleep mode,  using the RTC wakeup source description
-------------------------------------------------------------------------

- 1. Configure the parameters of the RTC wakeup source

    alarm_info_t low_valtage_alarm = {

                                    "low_vol",

                                    3000*RTC_TICKS_PER_1MS,//eg:set wakeup time: 3000ms

                                    1,

                                    cli_pm_rtc_callback,

                                    NULL

                                    };

     bk_alarm_register(AON_RTC_ID_1, &low_valtage_alarm);

- 2. bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL);
     comment：set wakeup source(RTC)


- 3. bk_pm_sleep_mode_set(PM_MODE_LOW_VOLTAGE);

comment：set sleep mode(low voltage)(comment：the low voltage is default sleep mode.When it meet the entering low voltage required tickets, it will enter the low voltage sleep mode)


- 4. bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x1,0x0);

comment：vote the APP ticket.


comment:

 - 1. BT and WIFI vote themselves sleep ticket.The APP ticket is voted by the user code.Util all the modules vote themselves sleep ticket, the system can enter low voltage sleep mode.

WIFI and BT call "bk_pm_module_vote_sleep_ctrl()"function to vote their ticket，tell the system(mcu) that they have entered sleep.

 - 2. when BT and WIFI power off，the system(pm module) will auto vote the WIFI and BT sleep tickets.

 - 3. before entering sleep, it need close audio,video,cpu1,cpu2 first（if application open them).