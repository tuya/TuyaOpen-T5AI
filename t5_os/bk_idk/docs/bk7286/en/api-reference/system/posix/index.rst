POSIX APIs
=======================

:link_to_translation:`zh_CN:[中文]`

POSIX APIs Introduce
------------------------

 -  At present, the posix interface used by platform Armino only support the FreeRTOS V10.4, which is disabled by default
 -  Use after opening macro definition CONFIG_FREERTOS_POSIX

.. note::

 - When using the posix function of FreeRTOS, you need to reference the FreeRTOS_POSIX.h header file before referencing the posix-related header files;
 - Related configurations can be customized in components/bk_rtos/freertos/posix/freertos_impl/include/portable/bk/FreeRTOS_POSIX_portable.h.


.. toctree::
    :maxdepth: 1

    unistd APIs <unistd>
    utils APIs <utils>
    errno APIs <errno>
    fcntl APIs <fcntl>
    signal APIs <signal>
    mqueue APIs <mqueue>
    pthread APIs <pthread>
    sched APIs <sched>
    semaphore APIs <semaphore>
    time APIs <time>
