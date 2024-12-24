.. _project_simplest_component:

Minimum Component
==============================================

Overview
-----------------------------

The simplest component CMakeLists.txt is as follows, calling ``armino_component_register()`` Register the component with the build system::

    armino_component_register(SRCS c1.c INCLUDE_DIRS include)

:example:`example <build_system/101_simplest_component>`
