AB partition location independent of upgrade interface
-------------------------------------------------------

:link_to_translation:`zh_CN:[中文]`

- 1 Get ota current execute partition info；

    uint8 bk_ota_get_current_partition(void);

    .. note::
        return value brief introduction：
        0： means current execute A partition.
        1： means current execute B partition.

- 2  customer can use this callback to register cb fuction . when download success and return a execute partition value

    void bk_ota_register_temp_partition_callback(callback_func cb);

    .. note::
        return value brief introduction：
        3： means temporary execute A partition.
        4： means temporary execute B partition.

- 3 when do ota position independent update,customer finally decided which partition to execute.

    void bk_ota_double_check_for_execution(void);

- 4 In the current SDK version, the above (2,3) has been implemented (controlled by CONFIG_OTA_UPDATE_DEFAULT_PARTITION) to facilitate self-testing. Customer needs in the projects/customization/config_ab/config/bk7236/config) will shut CONFIG_OTA_UPDATE_DEFAULT_PARTITION the macro definition.

- After the app successfully downloads the image, the customer needs to return to the partition that needs to be temporarily executed; See Figure 1 below.

.. figure:: ../../../../_static/ab_position_indes_api_introduction.png
    :align: center
    :alt: ab_position_indes_api_introduction
    :figclass: align-center

    Figure 1 the interface of executing the partition temporarily

- After the app image is successfully upgraded, the customer needs to confirm that the current upgrade partition is ok in the app and call bk_ota_double_check_for_execution. (See Figure 2 below for reference components/bk_init)

.. figure:: ../../../../_static/ab_position_indes_api_introduction2.png
    :align: center
    :alt: ab_position_indes_api_introduction2
    :figclass: align-center

    Figure 2 the interface of executing the partition finally