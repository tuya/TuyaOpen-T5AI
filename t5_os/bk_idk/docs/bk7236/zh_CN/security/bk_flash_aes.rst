.. _bk_security_flash_aes_crc:


FLASH AES 加密与 CRC
=========================

:link_to_translation:`en:[English]`

.. figure:: picture/security_flash.svg
    :align: center
    :alt: 8                                                                                           
    :figclass: align-center

如上图所示，BK7236 FLASH 提供了两中访问接口：数据口与指令口：

 - 数据口访问，即访问者（一般指 CPU）直接通过 SPI 接口访问 FLASH，使用的是 FLASH 物理地址。
 - 指令口访问，访问者不直接访问 FLASH，而是经由 FLASH Controller 间接访问 FLASH。
 
  - 读访问时，FLASH 控制器通过 SPI 读出 FLASH 中的原始数据，检验并去掉 CRC，然后使用 OTP 中 AES KEY 对数据解密，最后返回。
  - 写访问为读的逆操作，即先加密，再加 CRC，最后通过 SPI 将加 CRC/加密的数据写入 FLASH 中。

.. note::

 注意通过指令口读数据时，如果 CRC 检验失败，则会重新偿试再次读取，直接 CRC 检验通过。因此，如果存放在 FLASH 中原始数据
 的 CRC 不对，则 CPU 永远无法读取成功，表示出卡死。


通常，FLASH 指令口仅用在下述三种情况中：

 - CPU 从 FLASH 取指。
 - BL1/BL2 安全启动。
 - OTA 升级。

其他情况都应该走 FLASH 数据口访问 FLASH。

虚拟地址与物理地址
-------------------------------

虚拟地址是指访问者使用的地址。物理地址是指的 SPI 访问 FLASH 的地址，也即 FLASH 自身的地址偏移。
由于访问时 FLASH 控制器可能会对地址进行映射，因此虚拟地址可能与物理地址不同。


当走数据口时，虚拟地址即为物理地址。走指令口时，虚拟地址与物理地址关系为：

 physical_addr = (virtual_addr/32)*34

例如，CPU 取指时会走指令口，当 CPU 从地址 0x1000 取指时，实际访问到的物理地址为 0x1100。

配置
----------------

要使用 FLASH AES 加密功能，需做下述配置：

 - security.csv 中使能并配置 AES KEY，请参考 :ref:`安全配置<bk_config_security>`。
 - EFUSE 中使能 FLASH AES 功能，并将 KEY 部署到 OTP 中，请参考 :ref:`OTP EFUSE 配置<bk_config_otp_efuse>`。

.. important::

 BK7236 暂不支持自动生成随机 FLASH AES KEY，需要用户手动配置 AES KEY，为确保存放在
 FLASH 中指令安全，您需要妥善保管好您的 FLASH AES KEY。
