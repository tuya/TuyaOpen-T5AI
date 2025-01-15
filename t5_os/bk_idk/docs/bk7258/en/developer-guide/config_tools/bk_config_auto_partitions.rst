.. _bk_config_auto_partitions_:

Automated partition configuration
===================================

:link_to_translation:`zh_CN:[中文]`

.. note::
    This section aims to introduce how to adapt automated partitioning to new projects

Automated partition configuration table：
-------------------------------------------

The FLASH Automated partition table is a CSV file, the following is a configuration example:

+--------------------+------------+---------+----------+-------+--------+
| #Name              | Offset     | Size    | Type     | Read  | Write  |
+====================+============+=========+==========+=======+========+
| bootloader         |            | 68k     | code     | TRUE  | FALSE  |
+--------------------+------------+---------+----------+-------+--------+
| app                |            | 1836k   | code     | TRUE  | FALSE  |
+--------------------+------------+---------+----------+-------+--------+
| app1               |            | 476k    | code     | TRUE  | FALSE  |
+--------------------+------------+---------+----------+-------+--------+
| app2               |            | 136k    | code     | TRUE  | FALSE  |
+--------------------+------------+---------+----------+-------+--------+
| download           |            | 1428k   | data     | TRUE  |        |
+--------------------+------------+---------+----------+-------+--------+
| usr_config         |            | 136k    | data     | TRUE  |        |
+--------------------+------------+---------+----------+-------+--------+
| easyflash          |0x3fc000    | 8k      | data     | TRUE  |        |
+--------------------+------------+---------+----------+-------+--------+
| rf_firmware        |0x3fe000    | 4k      | data     | TRUE  |        |
+--------------------+------------+---------+----------+-------+--------+
| net_param          |0x3ff000    | 4k      | data     | TRUE  |        |
+--------------------+------------+---------+----------+-------+--------+

.. _bk_config_auto_partitions_table:

The partition table is fixed to 6 columns:

  - ``Name`` - Required option, indicating the partition name.
  - **Partition names must be unique**。The build system will generate a set of macros based on the partition name.Only a unique partition name can ensure that the generated macros will not be defined repeatedly.
  - binname represents the name of the binary file placed in the partition. The partition whose Type is configured as code corresponds to the binary file generated in the build system. For example, the binary file placed in the bootloader partition is bootloader.bin;the binary file placed in the app partition is app.bin.
  - Reserved partitions, reserved partitions are partitions that have specific uses for Armino:

    - bootloader - Type is configured as code, which corresponds to bootloader.bin compiled by the build system. Size is 34/32 times the actual size of bootloader.bin.
    - app - Type is configured as code, which corresponds to app.bin compiled by the build system. Size is 34/32 times the actual size of app.bin.
    - app1 - Type is configured as code, corresponding to app1.bin compiled by the build system, Size is 34/32 times the actual size of app1.bin
    - app2 - Type is configured as code, which corresponds to app2.bin compiled by the build system.Size is 34/32 times the actual size of app2.bin.
    - download - A unique partition in Armino. This partition will be used by ota. When the ota function is not needed, the partition size can be reserved and configured as 4K.
    - usr_config - A unique partition in Armino. This partition will be used by the file system. The size of this partition can be increased or decreased appropriately in units of 4K.
    - easyflash - a unique partition in Armino, Size and Offset are fixed and cannot be customized.
    - rf_firmware, net_param - unique partitions in Armino, RF calibration, Wi-Fi/TCPIP will use these two partitions.Size and Offset are fixed and cannot be customized.
  - ``Offset`` - Optional. Indicates the physical offset of the partition in FLASH.
  - ``Size`` - required. Represents the partition size, the unit k/K represents Kbytes, m/M represents Mbytes, and 4K alignment is usually recommended.
    - When the partition Type field is configured as code, the size is recommended to be aligned to 68k
  - ``Type`` - required. When configured as code, it means that the partition is an executable code area.When configured as data, it means that the partition is a non-executable data area.
  - ``Read`` - Optional. The partition has readable permissions. If not configured, the table will not be readable.
  - ``Write`` - Optional. The partition has writable permissions. If not configured, the table cannot be writable.

Taking the bk7258 chip as an example, refer to /projects/app to create a new project/projects/template. The project directory structure is as follows:

    .. figure:: picture/bkfil_project_dir_arch_old.png
      :align: center
      :alt: 8
      :figclass: align-center

If there is a need to modify the partition, to facilitate the modification, you usually need to follow the following steps:

    - Modify partition information based on the automated partition CSV table config/bk7258/bk7258_partitions.csv

      - Reserved partitions need to follow the modification rules of reserved partitions
      - It is recommended that the new partition be added between the user_config and easyflash partitions, and the required size is recommended to be obtained by reducing the user_config partition size.
      -  The sum of the sizes of all partitions must not exceed the actual size of the flash used
    - Configure the macro CONFIG_OVERRIDE_FLASH_PARTITION=y in the config/bk7258/config file
    - Find the ARMINO partition automation support component and add the new project name template to components/part_table/CMakeLists.txt and the support list defined in the components/part_table/part_table.mk file is as shown below:

    The components/part_table/CMakeLists.txt file is modified as follows:

    .. figure:: picture/bkfil_cmakeliststxt_support_list.png
      :align: center
      :alt: 8
      :figclass: align-center

    The components/part_table/part_table.mk file is modified as follows:

    .. figure:: picture/bkfil_makefile_support_list.png
      :align: center
      :alt: 8
      :figclass: align-center

    - Compile the project template - Compile the server shell terminal and enter make bk7258 PROJECT=template

    .. figure:: picture/bkfil_project_dir_arch_new.png
      :align: center
      :alt: 8
      :figclass: align-center

    You can see that the files config/bk7258/configuration.json and config/bk7258/partitions.csv have been added to the project directory.At the same time,The contents of the main/vendor_flash.c and main/vendor_flash_partition.h files will be updated simultaneously.