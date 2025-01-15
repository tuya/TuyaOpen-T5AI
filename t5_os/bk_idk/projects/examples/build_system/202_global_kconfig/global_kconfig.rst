.. _project_global_kconfig:

Add Global Kconfig
==============================================

Overview
-----------------------------

A Kconfig file can be defined for a component to implement global component configuration.If you want to add configuration options at the top level of menuconfig,rather than in the "Component Configuration" submenu, you can define these options in the KConfig.projbuild file in the same directory as the CMakeLists.txt file.

Usually a project-specific Kconfig.projbuild is added to the main component. But be careful when adding configurations in this file as these are included in the overall project configuration.
Where possible, create a KConfig file for component configuration.

:example:`example <build_system/202_global_kconfig>`

