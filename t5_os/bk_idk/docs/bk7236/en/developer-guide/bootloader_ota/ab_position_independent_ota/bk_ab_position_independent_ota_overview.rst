AB Partition location independent OTA Description
============================================================

:link_to_translation:`zh_CN:[中文]`

一、overview
----------------------------

 - AB partition location independent scheme indicates that the program image is stored in any flash location after the bootloader partition, and the program can be run. Assume that there are two execution zones (zone A and Zone B). When the program is running in zone A, the upgrade image is successfully downloaded to zone B through OTA and the reboot is performed. In the bootloader, Flash-related control register bit is set to determine which zone to execute. If the bit value is 1, the program will run to zone B; If the bit value is 0, the program will run to zone A. The distribution of flash Angle and cpu Angle corresponding to the AB partition is shown in Figure 1 below (only three partitions, such as Bootloader and app (A/B), are included, user, RF and Net are not listed, please note).

.. figure:: ../../../../_static/ab_flash_cpu_partition.png
    :align: center
    :alt: ab_flash_cpu_partition
    :figclass: align-center

    Figure 1 Partition of flash view &cpu view
