.. _project_cmake_use_armino_lib:

CMake Uses Armino Lib
==============================================

Overview
--------------------------

The way to reference armino components in pure CMake components is armino::component_name. In the example, the c1 component in the anywhere directory will use the armino component c component::

    target_link_libraries(c1 armino::c)

:example:`example <build_system/110_use_armino_lib_in_pure_cmake>`


