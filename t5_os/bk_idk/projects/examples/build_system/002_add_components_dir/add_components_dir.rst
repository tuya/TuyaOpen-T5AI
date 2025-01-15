.. _project_add_component_dir:

Add Component Dir
=============================

Overview
-----------------------------

You can set EXTRA_COMPONENTS_DIRS in the top-level CMakeLists.txt of the project to import components other than the default component search directory::

        cmake_minimum_required(VERSION 3.5)
        set(EXTRA_COMPONENTS_DIRS my_component_dir)
        include($ENV{ARMINO_PATH}/tools/build_tools/cmake/project.cmake)
        project(myProject)

:example:`example <build_system/002_add_components_dir>`

