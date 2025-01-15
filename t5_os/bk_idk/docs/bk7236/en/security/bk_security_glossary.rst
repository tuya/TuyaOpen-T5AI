Security Terms
=======================

:link_to_translation:`zh_CN:[中文]`

.. note::

  When describing system security, the security terminology introduced in this section will be used directly!

Terms
---------------------

Related predicates and abbreviations:

  - ``S`` - Security world, which means the security world. It is a concept related to TrustZone in ARM Cortex-M33.
  - ``NS`` - Non-Security world, indicating a non-secure world. It is a concept related to TrustZone in ARM Cortex-M33.
  - ``S/NS Aware`` - specifically refers to devices that can perform security control based on S/NS signals.
  - ``P`` - Privilege, indicates that M33 runs at privileged level.
  - ``NP`` - Non-Privilege, indicates that M33 runs at a non-privileged level.
  - ``M33`` - ARM Cortex-M33 processor.
  - ``TZ`` - TrustZone, a security mechanism introduced in M33, isolates CPU cores into S/NS through TZ.
  - ``PPC`` - Peripheral Protection Control, BK7236 peripheral security attribute configuration and controller.
  - ``MPC`` - Memory Protection Control, BK7236 block storage security attribute configuration and controller.
  - ``BL1`` - specifically refers to the BootROM solidified in the chip.
  - ``BL2`` - Bootloader, specifically MCUBOOT in BK7236.
  - ``LCS`` - Life Cycle State.
