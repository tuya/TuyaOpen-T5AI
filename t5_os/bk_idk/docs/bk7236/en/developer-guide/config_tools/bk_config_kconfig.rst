.. _bk_config_kconfig:

Kconfig Configuration
==========================

:link_to_translation:`zh_CN:[中文]`

This section introduces frequency ask questions of Armino Kconfig.

For Kconfig configuration example, please refer to :ref:`Kconfig configuration example <project_minimum_kconfig>`.

Kconfig Files
----------------------------------------------------------

The Armino Kconfig organization is shown in the following diagram:

::

     -armino/
         - components/
             - c1/
                 -Kconfig
         - middleware/
             - soc/
                 - bk7236/
                     -bk7236.defconfig
                 - bk7258/
                     -bk7258.defconfig
         - projects/
             - my_project/
                - config/
                    - common/
                        - config
                    - bk7236/
                        - config
                    - bk7258/
                        - config
                - Kconfig.projbuild
                - main/
                    - Kconfig
                - components/
                    - c2/
                        - Kconfig

These files can be grouped into two categories:

  - Configuration definition file:

    - Kconfig - usually defined in components.
    - Kconfig.projbuild - defined in the component or project directory.
  - Configuration modification file:

    - middleware/soc/bk7236.defconfig - Default configuration of BK7236.
    - my_projects/config/common/config - configuration shared by different BK72xx in the project.
    - my_projects/config/bk7236/config - BK7236 project configuration.

.. important::

   The configuration items can only be modified through configuration modification files after they are defined in definition files.
   If you try to modify a configuration that is not defined, no definitions will be generated in sdkconfig.h.

.. note::

   All the items defined in Kconfig.projbuild will be show in the top menu of menuconfig. BK7236 does not support menuconfig, so Kconfig.projbuild
   works same as Kconfig.

In the Armino project, the priority of Kconfig configuration is::

 my_projects/config/bk7236/config > my_projects/config/common/config > bk7236.defconfig > configuration definition files

Menuconfig Supporting
----------------------------------------------------------

Armino Kconfig does not support menuconfig.

Configuration Dependencies
----------------------------------------------------------

Interdependent configuration items are automatically changed to correct values, only when using menuconfig configuration.
Armino does not support menuconfig, so when changing a configuration via configuration modification file,
the configurations that depend on this configuration will not be changed automatically.

.. important::

  All configuration items in Armino need to be specified explicitly, or use the default value.

Using Kconfig in CMakeLists.txt
----------------------------------------------------------

The configuration items defined in Kconfig can be directly used in CMakeLists.txt, but you should NOT put
armino_component_register() in any configuration condition, refer to :ref:`Disabling components via Kconfig <project_disable_component>`.



