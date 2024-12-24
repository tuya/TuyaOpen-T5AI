.. _project_pure_cmake_1:

Pure CMake 1
==============================================

Overview
-----------------------------

In the example, foo is built using pure CMake, placed in the main component, and can be imported as follows::

    armino_component_register(SRCS "main.c" INCLUDE_DIRS .)
    add_subdirectory(foo)
    target_link_libraries(${COMPONENT_LIB} PUBLIC foo)

:example:`example <build_system/107_pure_cmake_in_main>`
