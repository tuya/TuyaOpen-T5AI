.. _project_common_config:

SoC Common Config
==============================================

Overview
-----------------------------

The order in which armino loads Kconfig is as follows. For the same configuration item, the value loaded later will override the value loaded first：

 - group Kconfig default configuration
 - :middleware:: Target specific default configuration in `<soc/bk72xx/bk72xx.defconfig>`
 - Project-related, target-common configuration is defined in the project root directory /config/common/config
 - Project-related, target-specific configuration defined in the project root directory /config/bk72xx/config

Applications can set project/target related configuration items by configuring
"project root directory/config/common/config" and "project root directory/config/bk72xx/config", where bk72xx is a specific SoC, such as bk7236, etc.

:example:`example <build_system/204_project_common_config>`

