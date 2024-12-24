.. _project_component_build_options:

Set Component Build Option
==============================================

Overview
-----------------------------

When compiling source files for a specific component,you can use ``target_compile_options`` command to pass compiler options::

    target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-variable)

If you specify compiler flags for a single source file, you can use CMake's `set_source_files_properties`_ command::

    set_source_files_properties(compile_options.c
        PROPERTIES COMPILE_FLAGS
        -Wno-unused-variable
    )

Note that, the above two commands can only be called after the ``armino_component_register`` command in the component CMakeLists file.

:example:`example <build_system/102_build_options>`

.. _set_source_files_properties: https://cmake.org/cmake/help/latest/command/set_source_files_properties.html
