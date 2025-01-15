FLASH AES Encryption
===========================

:link_to_translation:`zh_CN:[中文]`

.. figure:: picture/security_flash.svg
     :align: center
     :alt: 8
     :figclass: align-center

As shown in the figure above, BK7236 FLASH provides two access interfaces: data port and command port. In order to ensure the safety of instructions, FLASH instruction port access provides
AES encryption mechanism. When writing data via the FLASH command port, the FLASH controller will use the AES KEY stored in the OTP to automatically
The written data is encrypted; when reading data through the FLASH command port, the FLASH controller will use the AES KEY to decrypt the read data.

Usually, the FLASH instruction port is only used in the following three situations:

  - CPU fetches instructions from FLASH.
  - BL1/BL2 secure boot.
  - OTA upgrades.

In other cases, the FLASH data port should be used to access the FLASH.

Configuration
----------------

To use the FLASH AES encryption function, the following configurations are required:

  - Enable and configure AES KEY in security.csv, please refer to :ref: `Security configuration <bk_config_security>`.
  - Enable FLASH AES function in EFUSE, and deploy KEY to OTP, please refer to :ref:`OTP EFUSE configuration <bk_config_otp_efuse>`.

.. important::

  BK7236 does not support automatic generation of random FLASH AES KEY at the moment, users need to manually configure AES KEY, in order to ensure storage in
  The commands in FLASH are safe, so you need to keep your FLASH AES KEY safe.
