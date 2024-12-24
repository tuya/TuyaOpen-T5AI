.. _project_component_dependency:

Components Dependency
==============================================

Overview
-----------------------------

The example contains four components::

    - components/
        - c1/
            - c1.c
            - c1.h
            - c1_internal
                - c1_internal.c1
                - c1_internal.h
            - include/
                - bk_api_c1.h
        - c2/
            - c2.c
            - include
                - bk_api_c2.h
        - c3/
            - c3.c
            - include
                - bk_api_c3.h
        - c4/
            - c4.c
            - include
                - bk_api_c4.h

:example:`example <build_system/104_dependency>`
