六.Bootloader.bin的镜像更新说明
-------------------------------

:link_to_translation:`en:[English]`

- 1.为了优化bootloader.bin镜像的版本发布，从tag:v2.0.1.8版本开始：bootloader镜像的打包路径发生改变，由原先的默认路径middleware/boards/更改为components/bk_libs目录下。

  .. important::
    自从tag:v2.0.1.10之后的版本，已实现全自动插入bootloader.bin中的分区表，无需再手动修改bootloader中分区表。

  - 1.以bk_idk中的7258工程为例修改分区步骤如下：
      - 1.通过修改bk_idk/projects/app/config/bk7258/bk7258_partitions.csv文件来修改分区；
      - 2.直接make bk7258即可；
      - 3.无需手动修改components/bk_libs/bk7258/bootloader/normal_bootloader/bootloader.bin；
  - 2.以bk_avdk中customization/config_ab工程为例修改分区步骤如下：
      - 1.当通过修改projects/customization/config_ab/config/bk7258/bk7258_partitions.csv文件来修改分区；
      - 2.直接make bk7258 PROJECT = customization/config_ab即可；
      - 3.无需手动修改components/bk_libs/bk7258/bootloader/normal_bootloader/bootloader.bin；

  .. important::
    bootloader镜像的打包路径发生改变，由原先的默认路径middleware/boards/更改为components/bk_libs目录下。

  - 1.以7258工程为例：
      - bootloader原路径为：middleware/boards/bk7258/bootloader.bin；
      - bootloader现路径为：components/bk_libs/bk7258/bootloader/normal_bootloader/bootloader.bin；
  - 2.以7258的customization/config_ab工程为例：
      - bootloader现路径为：components/bk_libs/bk7258/bootloader/ab_bootloader/bootloader.bin；


- 2.若升级过程中出现问题，请联系我们FAE支持。


