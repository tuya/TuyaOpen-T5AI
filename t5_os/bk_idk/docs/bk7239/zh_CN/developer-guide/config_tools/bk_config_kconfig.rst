.. _bk_config_kconfig:

Kconfig 配置
========================

:link_to_translation:`en:[English]`

本节介绍 Armino Kconfig 使用过程中常见问题。关于 Kconfig 配置示例可参考 :ref:`Kconfig 配置示例 <201_build_project_demo>`。

Kconfig 配置文件
-------------------------

Armino Kconfig 组织如下图所示：

::

    - armino/
        - components/
            - c1/
                - Kconfig
        - middleware/
            - soc/
                - bk7239/
                    - bk7239.defconfig
        - projects/
            - my_project/
                - config/
                    - bk7239/config
                - Kconfig.projbuild
                - main/
                    - Kconfig
                - components/
                    - c2/
                        - Kconfig

可以将这些文件归为两类：

 - 配置定义文件: 

   - Kconfig - 通常定义在组件中。
   - Kconfig.projbuild - 定义在组件或者工程目录下。
 - 配置修改文件:

   - bk7239.defconfig - Armino 提供的 BK7239 默认配置。
   - bk7239/config - 工程中 BK7239 专有配置。

.. important::

  特定配置项只有在 Kconfig/Kconfig.projbuild 中定义之后才能通过配置修改文件去修改该配置，
  如果偿试修改一个未定义的配置，则不会在 sdkconfig.h 中生成任何定义。

.. note::

  Kconfig.projbuild 是针对 menuconfig 来设计的，当向 Kconfig.projbuild 中增加一项时，
  会在 menuconfig 顶层菜单中显示该配置项。BK7239 不支持 menuconfig，因此 Kconfig.projbuild
  与 Kconfig 作用相同。

在 Armino 工程中，Kconfig 配置优先级为:

  bk7239/config > bk7239.defconfig > 配置定义文件

menuconfig 支持
-------------------------

BK7239 Kconfig 配置不支持 menuconfig 配置。

配置依赖
-------------------------

相互依赖的配置项自动更改成正确的值，仅发生在使用 menuconfig 配置时，
Armino 不支持 menuconfig 配置，因此，当通过配置修改文件更改某项配置时，
与该项配置相互依赖的配置不会自动更改。

.. important::

 在 Armino 中所有配置项均需显式指定，或者使用默认值。

CMakeLists.txt 中使用 Kconfig
---------------------------------------

在 CMakeLists.txt 中可直接使用 Kconfig 中定义的配置项，唯一要注意的问题不要将
armino_component_register() 放在任何配置条件中，可参考 :ref:`通过 Kconfig 禁用组件 <207_build_kconfig_disable_component>`。




