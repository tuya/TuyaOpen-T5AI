.. _project_pure_config_kconfig:

Pure Config Kconfig
==============================================

Overview
-----------------------------

A component registered by API shown below, is called pure configuration component::

        armino_component_register()

This kind of components may not contain any source files and header files except the Kconfig configuration file.

.. note:

    Pure configuration components need to call armino_component_register() to register themselves with the build system.

:example:`example <build_system/203_config_only>`

