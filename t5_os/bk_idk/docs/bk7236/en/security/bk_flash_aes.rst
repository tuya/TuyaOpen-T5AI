.. _bk_security_flash_aes_crc:

FLASH AES Encryption
===========================

:link_to_translation:`zh_CN:[中文]`

.. figure:: picture/security_flash.svg
     :align: center
     :alt: 8
     :figclass: align-center

As shown in the figure above, BK7236 FLASH provides two access interfaces: data port and command port:

  - Data port access, that is, the visitor (generally refers to the CPU) directly accesses the FLASH through the SPI interface, using the FLASH physical address.
  - Command port access, the visitor does not directly access the FLASH, but indirectly accesses the FLASH through the FLASH Controller.

   - During read access, the FLASH controller reads out the original data in the FLASH through SPI, checks and removes the CRC, then uses the AES KEY in the OTP to decrypt the data, and finally returns.
   - Write access is the reverse operation of reading, that is, first encrypt, then add CRC, and finally write the CRC/encrypted data into FLASH through SPI.

.. note::

  Note that when reading data through the command port, if the CRC check fails, it will retry to read again, and the CRC check will pass directly. Therefore, if the original data stored in FLASH
  If the CRC is incorrect, the CPU will never be able to read it successfully, indicating that it is stuck.

Usually, the FLASH instruction port is only used in the following three situations:

  - CPU fetches instructions from FLASH.
  - BL1/BL2 secure boot.
  - OTA upgrades.

In other cases, the FLASH data port should be used to access the FLASH.

Virtual Address vs Physical Address
---------------------------------------------------

The virtual address refers to the address used by the visitor. The physical address refers to the address where the SPI accesses the FLASH, that is, the address offset of the FLASH itself.
Since the FLASH controller may map the address when accessing, the virtual address may be different from the physical address.


When using the data port, the virtual address is the physical address. When using the instruction port, the relationship between the virtual address and the physical address is:

  physical_addr = (virtual_addr/32)*34

For example, when the CPU fetches instructions, it will use the instruction port. When the CPU fetches instructions from address 0x1000, the actual physical address accessed is 0x1100.

Configuration
----------------

To use the FLASH AES encryption function, the following configurations are required:

  - Enable and configure AES KEY in security.csv, please refer to :ref: `Security configuration <bk_config_security>`.
  - Enable FLASH AES function in EFUSE, and deploy KEY to OTP, please refer to :ref:`OTP EFUSE configuration <bk_config_otp_efuse>`.

.. important::

  BK7236 does not support automatic generation of random FLASH AES KEY at the moment, users need to manually configure AES KEY, in order to ensure storage in
  The commands in FLASH are safe, so you need to keep your FLASH AES KEY safe.
