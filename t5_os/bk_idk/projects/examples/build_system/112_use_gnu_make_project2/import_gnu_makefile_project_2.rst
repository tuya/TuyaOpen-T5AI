.. _project_import_gnu_makefile_project_2:

Import GNU Makefile Project 2
==============================================

Overview
--------------------------

Another way to import a GNU project is to import a project written in a GNU Makefile via add_custom_command.

Example to import a foo component written with a Makefile into the c1 component::

    armino_component_register(SRCS c1.c INCLUDE_DIRS include)

    add_custom_command(OUTPUT ${COMPONENT_DIR}/foo/libfoo.a
        COMMAND ${COMPONENT_DIR}/foo/build.sh ${COMPONENT_DIR}/foo ${CMAKE_C_COMPILER}
        VERBATIM
        COMMENT "Build external project"
        )
    add_custom_target(foo_build DEPENDS ${COMPONENT_DIR}/foo/libfoo.a)

    add_library(foo STATIC IMPORTED GLOBAL)
    add_dependencies(foo foo_build)
    set_target_properties(foo PROPERTIES IMPORTED_LOCATION ${COMPONENT_DIR}/foo/libfoo.a)
    set_target_properties(foo PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${COMPONENT_DIR}/foo/include)

    target_link_libraries(${COMPONENT_LIB} PUBLIC foo)

The above use case first calls ``armino_component_register`` to register a standard armino component ``c1``, then adds a command to generate ``libfoo.a`` through ``add_custom_command()``, and then calls ``add_custom_target()`` to add the target ``foo_build``.

The next four commands create the ``foo`` target, setting the location and header directory for the library corresponding to ``foo``. Finally, attach the target ``foo`` to the armino standard component ``c1``.

.. note::

    When using ``add_custom_command()`` it should be noted that the file generated after OUTPUT must be directly used in the component CMakeLists.txt corresponding to the armino component, so that only in this way will the dependency rule of the Makefile be triggered to call the added COMMAND. Otherwise, since the generated file is not used in CMakeLists.txt, the Makefile will think this file is not needed by the build system and will not trigger the command call!

