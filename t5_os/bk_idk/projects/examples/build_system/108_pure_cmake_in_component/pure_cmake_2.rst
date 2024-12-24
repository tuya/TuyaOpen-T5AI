.. _project_pure_cmake_2:

Pure CMake 2
==============================================

Overview
-----------------------------

In the example, foo is built with pure CMake, placed in the c1 component, and can be imported using the following methods::

    armino_component_register(SRCS "c1.c" INCLUDE_DIRS include)
    add_subdirectory(foo)
    target_link_libraries(${COMPONENT_LIB} PUBLIC foo)

:example:`example <build_system/108_pure_cmake_in_component>`



