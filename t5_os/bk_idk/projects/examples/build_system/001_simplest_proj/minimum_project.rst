.. _project_build_simplest_proj:

Minimum Project
=============================

Overview
-----------------------------

This project demonstrates a minimum project.

Each project has a top-level ``CMakeLists.txt`` file that contains build settings for the entire project, a minimal project looks like this::

    cmake_minimum_required(VERSION 3.5)
    include($ENV{ARMINO_PATH}/tools/build_tools/cmake/project.cmake)
    project(myProject)

:example:`example <build_system/001_simplest_proj>`

CMakeLists.txt Description
---------------------------------

Add two lines of code to each project in the order above:

- ``cmake_minimum_required(VERSION 3.5)`` Must be placed on the first line of the CMakeLists.txt file, it tells CMake the minimum version number required to build the project. ARMINO supports version 3.5 or higher.
- ``include($ENV{ARMINO_PATH}/tools/build_tools/cmake/project.cmake)`` The rest of CMake's functions are imported to complete tasks such as configuring projects, searching for components, etc.
- ``project(myProject)`` A project is created, and a project name is specified. This name will be used as the name of the final output binaries,namely ``myProject.elf`` and ``myProject.bin``.Only one project can be defined per CMakeLists.txt file.


Default project variable
---------------------------------

The following variables have default values that users can override to customize build behavior.

- ``COMPONENT_DIRS``：The search directory for components, defaults to  ``ARMINO_PATH/components``, ``ARMINO_PATH/middleware``, ``PROJECT_DIR/components``,
  and ``EXTRA_COMPONENTS_DIRS``.Override this variable if you do not want to search for components in these locations.
- ``EXTRA_COMPONENTS_DIRS``：A list of additional optional directories to search for components. The path can be relative to the project directory or absolute.
- ``COMPONENTS``：List of component names to build into the project, defaults to all components retrieved under the  ``COMPONENT_DIRS`` directory.Use this variable to "lean" the project to reduce build time.
  Note that if a component specifies another component it depends on via  ``COMPONENT_REQUIRES`` directory.it will be automatically added to ``COMPONENTS``.

The paths in the above variables can be absolute paths, or relative paths relative to the project directory.

.. note::

    Please use `set command of cmake`_ to set these variables,Such as ``set(VARIABLE "VALUE")``. Note that,``set()`` command needs to be placed before ``include(...)`` .

.. _set command of cmake: https://cmake.org/cmake/help/latest/command/set.html
