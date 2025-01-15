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

+--------------------+------------+--------------------+
| Device             | Secure     | Privilege          |
+====================+============+====================+
| uart0              | N          | N                  |
+--------------------+------------+--------------------+
| uart1              | Y          | Y                  |
+--------------------+------------+--------------------+
| gpio0              | Y          | Y                  |
+--------------------+------------+--------------------+
| gpio1              | Y          | N                  |
+--------------------+------------+--------------------+


The configuration table has three fixed columns:

  - ``Device`` - Required. Indicates the device name, and the supported device names refer to :ref:`Device name <bk_config_security_devicename>`.
  - ``Secure`` - Optional, indicates whether the device is a secure peripheral, which can only be accessed in the secure world. When not configured, it indicates a non-secure peripheral.
  - ``Privilege`` - optional, indicating whether the device is a privileged peripheral, and the privileged peripheral can only be accessed in the processor privileged state. When not configured, indicates an unprivileged peripheral.

.. note::

  If the configurable device is not in ppc.csv, it means non-secure device, non-privileged device.


.. _bk_config_security_devicename:

Configurable Device
++++++++++++++++++++++++++

TBC

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
| mem0               |            | 16k                | TRUE       |
+--------------------+------------+--------------------+------------+
| mem0               |            | 32k                | FALSE      |
+--------------------+------------+--------------------+------------+
| mem0               |            | 16k                | TRUE       |
+--------------------+------------+--------------------+------------+
| mem1               | 0x10000    | 64k                | TRUE       |
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

FLASH Data Bus Access Security Configuration
----------------------------------------------

TODO

.. _bk_config_security_dma:

DMA Security Configuration
----------------------------

TODO
