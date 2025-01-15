.. _bk_config_security:

Security Configuration
==========================

:link_to_translation:`zh_CN:[中文]`

.. _bk_config_security_security:

General Security Configuration
----------------------------------------

Secure boot configuration is mainly used to configure BL1 and BL2 secure boot related information:

  - ``secureboot_en`` - optional, indicating whether to enable secure boot, default is not enabled.
  - ``flash_aes_en`` - optional, indicating whether to enable FLASH AES encryption, it is not enabled by default.
  - ``flash_aes_key`` - optional, indicating the FLASH AES encryption key, must be configured when ``flash_aes_en`` is enabled.
  - ``root_key_type`` - optional. By default ec256 is used. Configure the public key type of BL1/BL2 signature, the optional values are ec256, rsa2048.
  - ``root_pubkey`` - optional, **must be configured in mass production version**. BL1/BL2 public key file.
  - ``root_privkey`` - optional, **must be configured in mass production version**. BL1/BL2 private key file. The build tools sign the BL2 with the private key.
  - ``debug_key_type`` - Optional. By default ec256 is used. Configure the public key type of secure debugging signature, the optional values are ec256, rsa2048.
  - ``debug_pubkey`` - optional, **must be configured for mass production version**. Secure debugging public key file.
  - ``debug_privkey`` - optional, **must be configured for mass production version**. Secure debug private key file.

.. note::

   To simplify configuration, BL1/BL2 are signed with the same public key.

.. _bk_config_security_ppc:

Security Peripheral Configuration
-------------------------------------------

Security peripherals are configured through ppc.csv, the configuration example is as follows:

ppc.csv
++++++++++++++++++++++++++

+--------------------+------------+--------------------+
| Device             | Secure     | Privilege          |
+====================+============+====================+
| UART0              | FALSE      | TRUE               |
+--------------------+------------+--------------------+
| UART1              | TRUE       | TRUE               |
+--------------------+------------+--------------------+
| GPIO0              | FALSE      | TRUE               |
+--------------------+------------+--------------------+
| GPIO1              | TRUE       | TRUE               |
+--------------------+------------+--------------------+

The configuration table has three fixed columns:

  - ``Device`` - Required. Indicates the device name, such as UART1 etc.
  - ``Secure`` - Optional. The valid value can be:

    - TRUE - The device is a secure peripheral, which can only be accessed in the secure world.
    - FALSE - The device is a non-secure peripheral, which can only be accessed in non-secure world.
    - NA - The device is not used in the system.
    - Not configured - Same as NA.
  - ``Privilege`` - Optional,  The valid value can be:

    - TRUE - The device is a privilege peripheral, which can only be accessed in the CPU privilege state.
    - FALSE - The device is a non-privilege peripheral.
    - NA - The device is not used in the system.
    - Not configured - Same as NA.

.. note::

  If the configurable device is not in ppc.csv, it means the device is not used in the system.

gpio_dev.csv
++++++++++++++++++++++++++++++++++

gpio_dev.csv is used to configure the mapping relationship between peripherals and GPIO, the configuration example is as follows:

+--------------------+------------+
| GPIO               | Device     |
+====================+============+
| GPIO0              | UART1      |
+--------------------+------------+
| GPIO1              | UART1      |
+--------------------+------------+


The configuration table fixes two columns:

  - ``GPIO`` - Required. GPIOx stands for GPIO x.
  - ``Device`` - Required. Indicates which peripheral the GPIO is used by.

The purpose of gpio_dev.csv is to ensure that peripheral security attributes are consistent with their associated GPIO attributes. The Armino build system will
Convenient ppc.csv configuration table, for those peripherals that will be applied to the system, find all GPIOs from gpio_dev.csv,
If these GPIO attributes do not match the peripheral security attributes, a prompt message will be given, and the GPIO security attributes will be automatically corrected.

Take UART1 in the above example as an example, it can be seen from gpio_dev.csv that UART1 will use GPIO0/GPIO1, so the security of UART1
All attributes must be consistent with GPIO0/1. According to the ppc.csv configuration, UART1 is configured as safe, and GPIO0/1 are configured as non-safe/safe respectively.
Among them, the security attribute of GPIO0 is different from that of UART1, so the build system will automatically adjust the security attribute of GPIO0 to be safe.

.. note::

   If the peripheral will not be used in the system, none of the GPIOs used by the peripheral will be adjusted according to gpio_dev.csv.

Code Generation
++++++++++++++++++++++++++

For a row in ppc.csv, two macros are generated in security.h to identify device security attributes.

+--------------------+------------+--------------------+
| Device             | Secure     | Privilege          |
+====================+============+====================+
| uart0              | FALSE      | FALSE              |
+--------------------+------------+--------------------+

Take the uart0 configuration in the above table as an example, the following code will be generated::

   #define GEN_SECURITY_DEV_UART0_IS_SECURE 0
   #define GEN_SECURITY_DEV_UART0_IS_PRIVILEGE 0

In addition, four external interfaces will be generated::

   #define DEV_IS_PRIVILEGE(dev) GEN_SECURITY_DEV_##dev##_IS_PRIVILEGE
   #define DEV_ID_IS_PRIVILEGE(dev, id) GEN_SECURITY_DEV_##dev##id##_IS_PRIVILEGE
   #define DEV_IS_SECURE(dev) GEN_SECURITY_DEV_##dev##_IS_SECURE
   #define DEV_ID_IS_SECURE(dev, id) GEN_SECURITY_DEV_##dev##id##_IS_SECURE

In Armino, peripheral security attributes can be accessed by calling the above four external interfaces.

.. _bk_config_security_mpc:

Security Block Peripheral Configuration
--------------------------------------------------

Block peripherals are configured through mpc.csv, the configuration example is as follows:

+--------------------+------------+--------------------+------------+
| Device             | Offset     | Size               | Secure     |
+====================+============+====================+============+
| MEM0               |            | 16k                | TRUE       |
+--------------------+------------+--------------------+------------+
| MEM0               |            | 32k                | FALSE      |
+--------------------+------------+--------------------+------------+
| MEM0               |            | 16k                | TRUE       |
+--------------------+------------+--------------------+------------+
| MEM1               | 0x10000    | 64k                | TRUE       |
+--------------------+------------+--------------------+------------+
| FLASH_CODE         | 0x20000    | 64k                | FALSE      |
+--------------------+------------+--------------------+------------+
| FLASH_DATA         | 0x30000    | 64k                | TRUE       |
+--------------------+------------+--------------------+------------+

The configuration table has four fixed columns:

  - ``Device`` - Required. device name.
  - ``Offset`` - optional. Device internal offset.

    - Not configured, if it is the first configuration block of the device, please set Offset to 0, otherwise it indicates the end address of the previous block.
    - Offset + Size cannot exceed the total storage size of the device.
    - Offset A block of addresses on the same device cannot overlap.
  - ``Size`` - Required. Configure block size. Must be aligned to the device block size.
  - ``Secure`` - optional. Configure the block security attribute, the value is False when not configured.

About configurable devices, device storage space size, device block size, please refer to :ref:`Block memory security access <bk_security_overview_mpc>`.

.. note::

   When the device block is not configured, it defaults to non-secure.

.. _bk_config_security_flash:

FLASH security attribute configuration
++++++++++++++++++++++++++++++++++++++++++

In the mpc.csv configuration table, the FLASH configuration is special because it includes the configuration of the instruction port and the data port. FLASH command port/data port configuration use FLASH_CODE/FLASH_DATA respectively
Indicates the device name, and always uses the physical address during configuration. For the FLASH address, please refer to :ref:`FLASH Security <bk_security_flash_aes_crc>`.

.. important::

  When the FLASH instruction port is configured, the physical address must be 68K aligned, so the converted virtual address is 64K aligned.

