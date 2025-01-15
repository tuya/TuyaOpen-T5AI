AB分区位置无关的升级性格接口介绍
---------------------------------

:link_to_translation:`en:[English]`

- 1 获取当前的执行分区；

    uint8 bk_ota_get_current_partition(void);

    .. note::
        返回值简介：
        0： means current execute A partition.
        1： means current execute B partition.

- 2 当app镜像下载成功之后，客户需要调用该接口，返回将升级的的分区；

    void bk_ota_register_temp_partition_callback(callback_func cb);

    .. note::
        返回值简介：
        3： means temporary execute A partition.
        4： means temporary execute B partition.

- 3 当执行完升级重启之后，客户需要在app中，确认当前的升级分区ok.

    void bk_ota_double_check_for_execution(void);

- 4 当前发布的SDK版本中，上述（2，3）为方便自测试，已实现（被CONFIG_OTA_UPDATE_DEFAULT_PARTITION控制），客户需要在（projects/customization/config_ab/config/bk7236/config）将CONFIG_OTA_UPDATE_DEFAULT_PARTITION 这个宏定义关闭。

- app下载镜像成功之后，需要客户返回需要临时执行的分区；见下图1

.. figure:: ../../../../_static/ab_position_indes_api_introduction.png
    :align: center
    :alt: ab_position_indes_api_introduction
    :figclass: align-center

    图1 临时执行分区的接口

- app升级镜像成功之后，需要客户在app中确认当前升级分区ok，调用bk_ota_double_check_for_execution；（参考components/bk_init见下图2）

.. figure:: ../../../../_static/ab_position_indes_api_introduction2.png
    :align: center
    :alt: ab_position_indes_api_introduction2
    :figclass: align-center

    图2 确认执行分区的接口