.. _project_proj_build_options:

Override Default Compile Options
==============================================

Overview
-----------------------------

After project() in the top-level CMakeLists.txt, use Wno-extra to override the default Wextra::

        cmake_minimum_required(VERSION 3.5)
        include($ENV{ARMINO_PATH}/tools/build_tools/cmake/project.cmake)
        project(myProject)

        armino_build_set_property(COMPILE_OPTIONS "-Wno-extra" APPEND)

.. note:

  Your compile options should be set after project() because the default build specification is set inside project() .

:example:`example <build_system/005_proj_build_options>`


