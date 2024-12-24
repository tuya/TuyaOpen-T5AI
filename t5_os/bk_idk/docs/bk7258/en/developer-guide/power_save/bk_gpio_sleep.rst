enter deep sleep or low voltage sleep mode,  using the GPIO wakeup source description
========================================================================================

:link_to_translation:`zh_CN:[中文]`

enter deep sleep , using the GPIO wakeup source description
--------------------------------------------------------------
- 1. bk_gpio_register_wakeup_source(GPIO_18,GPIO_INT_TYPE_FALLING_EDGE);

    a. wakeup GPIO id : GPIO18;

    b. GPIO wake up type：falling edge type

::

    GPIO_INT_TYPE_FALLING_EDGE: falling edge type;
    GPIO_INT_TYPE_RISING_EDGE: rising dege type;
    GPIO_INT_TYPE_LOW_LEVEL: low level type;
    GPIO_INT_TYPE_HIGH_LEVEL: high level type;

- 2. bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_GPIO, NULL);

    set wakeup source(GPIO)


- 3. bk_pm_sleep_mode_set(PM_MODE_DEEP_SLEEP);

    set sleep mode(deep sleep)

    (The system wakes up from deep sleep，it can call the function - bk_gpio_get_wakeup_gpio_id() - to get which gpio id wakeup the system)

comment：
- 1)if the WIFI and BT are working, the SDK will stop the WIFI and BT work, WIFI and BT will enter sleep before entering deep sleep.
- 2)if you open mult-media(audio,video) before entering deep sleep,you should close them before entering deep sleep.


enter super deep sleep , using the GPIO wakeup source description
-------------------------------------------------------------------
- 1. bk_gpio_ana_register_wakeup_source(GPIO_12,GPIO_INT_TYPE_HIGH_LEVEL);

    a. wakeup GPIO id : GPIO12;

    (Different from deep sleep, super deep sleep supports only GPIO_0 to GPIO_15 as wakeup source, and the number of GPIOs that simultaneously served as wakeup source is 2.)

    b. GPIO wake up type：falling edge type

::

    GPIO_INT_TYPE_LOW_LEVEL: low level type;
    GPIO_INT_TYPE_HIGH_LEVEL: high level type;

    (Super deep sleep only supports two wakeup mode, and the wakeup modes of two GPIOs that serve as wakeup source simultaneously must be consistent.)

- 2. bk_pm_sleep_mode_set(PM_MODE_SUPER_DEEP_SLEEP);

    set sleep mode(superdeep sleep)

    (The system wakes up from super deep sleep，it can call the function - bk_gpio_get_wakeup_gpio_id() - to get which gpio id wakeup the system)

comment：
- 1)if the WIFI and BT are working, the SDK will stop the WIFI and BT work, WIFI and BT will enter sleep before entering super deep sleep.
- 2)if you open mult-media(audio,video) before entering super deep sleep,you should close them before entering super deep sleep.


enter low voltage sleep mode,  using the GPIO wakeup source description
--------------------------------------------------------------------------

- 1. bk_gpio_register_wakeup_source(GPIO_18,GPIO_INT_TYPE_FALLING_EDGE);

  a.wakeup GPIO id:GPIO18;

  b.GPIO wake up type:falling edge type

- 2. bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_GPIO, NULL);

    set wakeup source(GPIO)

- 3. bk_pm_sleep_mode_set(PM_MODE_LOW_VOLTAGE);

    set sleep mode(low voltage)(comment：the low voltage is default sleep mode.When it meet the entering low voltage required tickets, it will enter the low voltage sleep mode)

- 4. bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x1,0x0);

    vote the APP ticket.


comment：

 - 1. BT and WIFI vote themselves sleep ticket.The APP ticket is voted by the user code.Util all the modules vote themselves sleep ticket, the system can enter low voltage sleep mode。

WIFI and BT call "bk_pm_module_vote_sleep_ctrl()"function to vote their ticket，tell the system(mcu) that they have entered sleep.

 - 2. when BT and WIFI power off，the system(pm module) will auto vote the WIFI and BT sleep tickets。

 - 3. before entering sleep, it need close audio,video,cpu1,cpu2 first（if application open them).
