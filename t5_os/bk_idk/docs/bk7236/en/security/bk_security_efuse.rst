.. _bk_securify_efuse:

EFUSE
=======================

:link_to_translation:`zh_CN:[中文]`

Overview
------------------------

EFUSE (Electrically Programming Efuse) is used to configure safety-related features in BK7236. It can only be configured from 0 to 1, and cannot be configured from 1 to 0.

BK7236 EFUSE has a total of 4x8 and a total of 32 Bits. Before secure boot is enabled, it can be configured through :ref:`BKFIL<bk_tool_bkfil>`. For details, please refer to :ref:`EFUSE configuration <bk_config_otp_efuse>`.

+-------+------------------------------+-----------------------------------------------------------------------------------------------------------------------+
| Bit   | Name                         | description                                                                                                           |
+=======+==============================+=======================================================================================================================+
| 0     | security boot enable         | 0: Disable secure boot. 1: Enable Secure Boot.                                                                        |
+-------+------------------------------+-----------------------------------------------------------------------------------------------------------------------+
| 1     | security boot debug mode     | 0: Enable secure boot debug information. 1: Disable secure boot debug information.                                    |
+-------+------------------------------+-----------------------------------------------------------------------------------------------------------------------+
| 2     | fast boot disable            | 0: enable fast boot; 1: disable fast boot.                                                                            |
+-------+------------------------------+-----------------------------------------------------------------------------------------------------------------------+
| 3     | boot mode                    | 0: traditional download mode; 1: secure boot mode.                                                                    |
+-------+------------------------------+-----------------------------------------------------------------------------------------------------------------------+
| 4     | security boot clock select   | 0: use XTAL clock for secure boot; 1: enable PLL for secure boot.                                                     |
+-------+------------------------------+-----------------------------------------------------------------------------------------------------------------------+
| 5     | random delay enable          | 0: Disable random delay; 1: Enable random delay.                                                                      |
+-------+------------------------------+-----------------------------------------------------------------------------------------------------------------------+
| 6     | direct jump enable           | 0: disable direct jump of secure boot; 1: enable direct jump of secure boot.                                          |
+-------+------------------------------+-----------------------------------------------------------------------------------------------------------------------+
| 7     | security boot critical error | 0: Enable critical error printing during secure boot; 1: Disable critical error printing during secure boot.          |
+-------+------------------------------+-----------------------------------------------------------------------------------------------------------------------+
| 8~19  | reserved                     | Reserved                                                                                                              |
+-------+------------------------------+-----------------------------------------------------------------------------------------------------------------------+
| 20    | attack_nmi_enable            | 0: No NMI is generated when an injection attack is detected; 1: NMI is generated when an injection attack is detected.|
+-------+------------------------------+-----------------------------------------------------------------------------------------------------------------------+
| 21    | spi_to_ahb_disable           | 0: SPI to AHB channel is not disabled; 1: SPI to AHB channel is disabled.                                             |
+-------+------------------------------+-----------------------------------------------------------------------------------------------------------------------+
| 22    | auto_reset_enable[0]         | Temperature and voltage abnormal reset                                                                                |
+-------+------------------------------+-----------------------------------------------------------------------------------------------------------------------+
| 23    | auto_reset_enable[1]         | Temperature and voltage abnormal reset                                                                                |
+-------+------------------------------+-----------------------------------------------------------------------------------------------------------------------+
| 24~28 | reserved                     | Reserved                                                                                                              |
+-------+------------------------------+-----------------------------------------------------------------------------------------------------------------------+
| 29    | flash aes enable             | 0: FLASH AES encryption is disabled; 1: FLASH AES encryption is enabled.                                              |
+-------+------------------------------+-----------------------------------------------------------------------------------------------------------------------+
| 30    | spi_dld_disable              | 0: SPI download enabled; 1: SPI download disabled.                                                                    |
+-------+------------------------------+-----------------------------------------------------------------------------------------------------------------------+
| 31    | swd_disable                  | 0: enable CPU SWD; 1: disable CPU SWD.                                                                                |
+-------+------------------------------+-----------------------------------------------------------------------------------------------------------------------+

.. _efuse_bit0:

BIT(0) - security boot enable
++++++++++++++++++++++++++++++++++++++++++++++++

Two bits of EFUSE are related to secure boot enable:

  - :ref:`BIT(3)<efuse_bit3>` - boot mode is used to set the boot mode. 0 means traditional download mode, and the version can be downloaded through UART at this time; 1 means safe boot mode,
    After setting to secure boot mode, you need to set :ref:`BIT(3)<efuse_bit3>` to 1 to enable secure boot.
  - :ref:`BIT(0)<efuse_bit0>` - Enable Secure Boot. Only valid when :ref:`BIT(3)<efuse_bit3>` is set to 1 (secure boot mode), indicating whether to enable secure boot.

When :ref:`BIT(0)<efuse_bit0>` is set to 0, it means that the secure boot is not enabled. At this time, BL1 considers the public key in IMAGE to be legal, that is, it will not use the public key in IMAGE and the one in OTP.
:ref:`BL1 ROTPK HASH<otp_bl1_rotpk_hash>` for comparison, only for IMAGE integrity verification. At this time, even if the public key HASH system is not configured in OTP, the system can still start.

When :ref:`BIT(0)<efuse_bit0>` is set to 1, it means secure boot is enabled, and :ref:`BL1 ROTPK HASH<otp_bl1_rotpk_hash>` must be configured in OTP, otherwise the boot will fail.

.. note::

   Once Secure Boot is enabled, it cannot be downloaded and configured through BKFIL! Therefore, this BIT(3) should be enabled last after deploying all other OTP/EFUSE configurations.
   When using :ref:`config scripts <bk_config_otp_efuse_scripts>` for deployment, please make sure that all OTP/EFUSE items that need to be configured are included in the configuration scripts.

.. _efuse_bit1:

BIT(1) - security boot debug mode
++++++++++++++++++++++++++++++++++++++++++++++++

BL1 defines two levels of debugging information for users to locate problems:

  - :ref:`BIT(1)<efuse_bit1>` - Controls general debug information.
  - :ref:`BIT(7)<efuse_bit7>` - Control fatal errors.

In addition to errors, general debugging information also includes some process log printing. Serious errors usually refer to errors that will cause BL1 to fail to start. Currently, BL1 supports the following serious errors:

+------------+---------------------------------------+
| Error code | Meaning                               |
+============+=======================================+
| 0x1        | Error reading EFUSE 1                 |
+------------+---------------------------------------+
| 0x2        | Error reading EFUSE 2                 |
+------------+---------------------------------------+
| 0x3        | Error reading FLASH 1                 |
+------------+---------------------------------------+
| 0x11       | CPU exception NMI                     |
+------------+---------------------------------------+
| 0x12       | CPU exception MemMange                |
+------------+---------------------------------------+
| 0x13       | CPU exception HardFault               |
+------------+---------------------------------------+
| 0x14       | CPU exception BusFault                |
+------------+---------------------------------------+
| 0x15       | CPU exception UserFault               |
+------------+---------------------------------------+
| 0x16       | CPU exception SecurityFault           |
+------------+---------------------------------------+
| 0x21       | OTP is empty                          |
+------------+---------------------------------------+
| 0x22       | public key is empty                   |
+------------+---------------------------------------+
| 0x23       | Jump BIN verification failed          |
+------------+---------------------------------------+
| 0x1xxx     | OTP read failure                      |
+------------+---------------------------------------+
| 0x8yyyyyyy | Signature verification failed         |
+------------+---------------------------------------+

Among them, xxx refers to the OFFSET of OTP, and yyyyyyy refers to the specific failure point of signature verification.

For serious errors, only the error code is displayed when printing, such as "E16" means that the CPU is abnormal SecurityFault.

.. note::

  When critical errors are enabled, the BL1 will not initialize the UART during safe boot, and will only initialize the UART for printing when an unrecoverable serious error occurs.
  Therefore, critical errors do not affect normal boot functionality and do not pose security issues.

.. important::

   Generally, during the development stage, especially before the secure boot has been successfully configured on any board, it is recommended to enable the normal log, so that more debugging information can be seen;
   Normal logging should be turned off after getting familiar with the secure boot configuration, or during mass production. For serious errors, it is recommended not to close it in the mass production version.

.. _efuse_bit2:

BIT(2) - fast boot disable
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(2) Enable Fast Boot startup when set to 0, and disable Fast Boot startup when set to 1.

Fast Boot is used to control the process of waking up the system from Deep Sleep. When Fast Boot is enabled, Deep Sleep will skip the safe boot after waking up and jump directly to the application;
When Fast Boot is turned off, it will do a complete safe boot similar to power-on restart.

.. important::

   When Fast Boot is enabled, the startup speed is faster, but it is not safe; when Fast Boot is disabled, the startup speed is slower, but it is safe and reliable.
   The application should decide whether to enable or disable Fast Boot based on actual needs.

.. _efuse_bit3:

BIT(3) - boot mode
++++++++++++++++++++++++++++++++++++++++++++++++

It is used to configure the startup mode, please refer to :ref:`BIT(0)<efuse_bit0>`.

.. _efuse_bit4:

BIT(4) - secure boot clock select
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(4) is used to enable/disable safe boot high frequency mode.

  - 0 means CPU and FLASH use XTAL as the clock, usually 26Mhz.
  - 1 means enable PLL, CPU and FLASH configuration

in high frequency mode. In the high-frequency mode, the safe boot speed is faster, and it is generally recommended to enable the high-frequency mode for applications that require boot performance.

.. _efuse_bit5:

BIT(5) - random delay enable
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(5) is used to control the random delay of the shutdown judgment statement in BootROM:

  - 0 means random delay is off.
  - 1 means on. when random delayWhen enabled, BL1 will make a random delay before calling key functions, through this mechanism to slow down the impact of :ref:`fault injection attack <fault_injection_attack>`.

.. note::

  Enabling the random delay will increase the secure boot time, unless the application has a particularly high defense against injection attacks, it is generally not recommended to enable the random delay function!

.. _efuse_bit6:

BIT(6) - Direct Jump
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(6) is used to configure the way of jumping to Bootloader after BL1 signature verification is completed:

  - 0 means an indirect jump. When using an indirect jump, BL1 does not jump directly to BL2 after the signature verification, but jumps to an intermediate BIN first, and then jumps to BL2 from the intermediate BIN.
  - 1 means jump directly. When using direct jump, BL1 jumps directly to BL2 after the signature verification is completed.

.. note::

   When using indirect jump, the intermediate BIN will also be included in BL2 for signature verification, so it is safe.

.. _efuse_bit7:

BIT(7) - security boot critical error
++++++++++++++++++++++++++++++++++++++++++++++++

See :ref:`BIT(1)<efuse_bit1>`. .

.. _efuse_bit20:

BIT(20) - attack NMI
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(20) is used to configure whether an NMI exception is generated after detecting a :ref:`fault injection attack <fault_injection_attack>`:

  - 0 - BL1 does not perform fault injection attack detection at the hardware level.
  - 1 - BL1 performs fault injection attack detection at the hardware level, and generates an NMI exception when an attack is detected.

.. note::

   For applications that are particularly concerned about fault injection attacks, it is recommended to enable this switch.

.. _efuse_bit21:

BIT(21) - spi to ahb disable
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(21) is used to disable SPI to AHB channel:

  - 0 - SPI to AHB channel enable. At this time, the BK7236 registers can be directly operated through the SPI interface.
  - 1 - SPI to AHB channel off. At this time, the BK7236 registers cannot be operated through the SPI interface.

It should be noted that BIT(21) and :ref:`BIT(30) spi flash download disable<efuse_bit30>` are independent of each other and need to be configured separately.

.. important::

   When Secure Boot is enabled, the SPI to AHB connection must be closed.

.. _efuse_bit29:

BIT(29) - flash aes enable
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(29) is used to enable FLASH AES encryption:

  - 0 - FLASH AES encryption disabled.
  - 1 - FLASH AES encryption enable. This must configure :ref:`FASH AES KEY <otp_flash_aes_key>`.

.. _efuse_bit30:

BIT(30) - spi download disable
++++++++++++++++++++++++++++++++++++++++++++++++

Disable SPI download function:

  - 0 - Enable internal SPI FLASH channel, support SPI download.
  - 1 - Internal SPI FLASH channel closed, does not support SPI download.

It should be noted that BIT(30) and :ref:`BIT(21) spi to ahb disable<efuse_bit21>` are independent of each other and need to be configured separately.

.. important::

   To avoid potential security risks, SPI downloads should be disabled in production builds. However, do not disable SPI downloads until Secure Boot has been successfully deployed,
   In this way, when the secure boot deployment fails, the version can still be downloaded to FLASH through SPI download. Otherwise, once the secure boot deployment fails, try again
   Also unable to download the version, the board becomes bricked.

.. _efuse_bit31:

BIT(31) - SWD debug
++++++++++++++++++++++++++++++++++++++++++++++++

BIT(31) is used to control the switch of CPU debug port:

  - 0 - CPU debugging is enabled, BK7236 supports SWD debugging.
  - 1 - CPU debugging off. At this time, the CPU debugging function must be enabled through :ref:`secure debugging <security_secure_debug>`.

.. important::

  When secure boot is enabled, SWD debugging needs to be turned off.
