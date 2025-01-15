Bootloader.bin Image update description
---------------------------------------------

:link_to_translation:`zh_CN:[中文]`

- 1. For better maintenance release, start from v2.0.1.8: The packaging path of the bootloader image is changed from the original default path middleware/boards/ to the components/bk_libs directory.

  .. important::
    In versions later than v2.0.1.10, the partition table in bootloader.bin is automatically modified.

  - 1. This section uses the customization/config_ab project in bk_avdk as an example to modify partitions:
  -
    - 1. When changing the partition by modifying the file,the bk7258_partitions csv path is: projects/customization/config_ab/config/bk7258/;
    - 2. Directly make bk7258 PROJECT=customization/config_ab;
    - 3. No need to modify the components/bk_libs/bk7258/bootloader/normal_bootloader/bootloader.bin;

  .. important::
    the packaging path of the bootloader image is changed from the original default path middleware/boards/ to the components/bk_libs directory.

  - 1. Take the customization/config_ab project of 7258 as an example:
  -
    - bootloader path for now: the components/bk_libs/bk7258/bootloader/ab_bootloader/bootloader.bin;


- 2.please contact our FAE,when ota fails.