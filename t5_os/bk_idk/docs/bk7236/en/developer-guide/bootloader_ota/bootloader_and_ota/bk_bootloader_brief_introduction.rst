Introduction to Bootloader and general Ota
==============================================

:link_to_translation:`zh_CN:[中文]`

一.overview
----------------------------

This document mainly describes the system startup and OTA process, and helps engineers engaged in BSP development and maintenance understand the system startup and OTA process, and better analyze and solve problems.

二.Introduction to the startup module
----------------------------------------

The startup module involves bootrom, bootloader, and applications. The startup process is as follows:

- 1.After the CPU starts, jump to the ROM address and run the ROM code.
- 2.If secure boot is enabled in EFUSE, perform a signature verification. After the signature verification is successful, set the reboot signal to enable the system to boot from the flash.
- 3.If Secure boot is not enabled in EFUSE, set the reboot signal to enable the system to boot from the flash.
- 4.Run the bootloader code in the Flash.
- 5.After the Bootloader code is executed, jump to the applications code.

三.Introduction to the Bootloader
------------------------------------

The Bootloader is divided into two modules: l_bootloader and up_bootloader. l_bootloader implements the uart download function. up_bootloader Implements OTA upgrade.

- 1.The distribution of logical addresses of the two modules of Bootloader is shown in Figure 1:

.. figure:: ../../../../_static/bootloader_logic_adress.png
    :align: center
    :alt: bootloader_logic_adress
    :figclass: align-center

    Figure 1 Logical distribution of bootloader

- 2.The execution process of Bootloader is shown in Figure 2:

.. figure:: ../../../../_static/bootloader_process_execution_e.png
    :align: center
    :alt: bootloader_process_execution_e
    :figclass: align-center

    Figure 2 the bootloader execution process
.. note::

  The Download partition flag in FIG. 2 refers to that during OTA upgrade, it will judge whether there is any data in the OTA data area, that is, whether it is 0xFFFFFFFF or not. If there is no upgrade data in the OTA upgrade area, it will directly jump to the app area.
  If it is not 0xFFFFFFFF, an OTA upgrade is performed.