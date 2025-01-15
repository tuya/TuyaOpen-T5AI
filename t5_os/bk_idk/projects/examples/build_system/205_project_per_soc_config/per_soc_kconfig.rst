.. _project_per_soc_config:

SoC Common Config
==============================================

Overview
-----------------------------

As the statement in example "soc common config", we now have one common config file for project, two specific config files for soc,

 - if we conduct "make bk7231n PROJETC=/example/build_system/205_project_per_soc_config", the print will be "CONFIG_FOO=100"
 - if we conduct "make bk7256 PROJETC=/example/build_system/205_project_per_soc_config", the print will be "CONFIG_FOO=1000"

:example:`example <build_system/205_project_per_soc_config>`
