.._bk_config_ota:

OTA Configuration
==========================

:link_to_translation:`zh_CN:[中文]`

OTA is mainly used to configure security counters for packaging and anti-version downgrade attacks.

OTA Configuration Table
----------------------------

The OTA configuration table is a CSV file, the following is a configuration example:

+--------------------+------------+--------------------+---------+
| Bin                | Version    | SecurityCounter    | OTA     |
+====================+============+====================+=========+
| bl2.bin            |            | 1                  | FALSE   |
+--------------------+------------+--------------------+---------+
| tfm_s.bin          |            | 1                  | TRUE    |
+--------------------+------------+--------------------+---------+
| cpu0_app.bin       |            | 1                  | TRUE    |
+--------------------+------------+--------------------+---------+

The partition table is fixed to four columns:

  - ``Bin`` - Required, indicating the name of the executable binary file. For the specific binary file xx.bin, the corresponding OTA partition name is secondary_xx, refer to :ref:`partition naming rules <bk_config_partitions_table>`.
  - ``Version`` - optional, generally no configuration required.
  - ``SecurityCounter`` - optional, represents the version security counter. It must be configured when secure boot is enabled to prevent version downgrade attacks. After Secure Boot verifies that the version is legitimate
    This value will be read, compared with the safety counter in OTP/NV, and the start is only allowed if the safety counter of the upgraded version is not less than the value in OTP/NV.
  - ``OTA`` - Optional. Indicates whether the corresponding Bin should be packaged in ota.bin.

.. note::

   1. Only partitions prefixed with secondary in partitions.csv are allowed to do OTA. Only when the partition has OTA capability, can it be configured in ota.csv whether to perform OTA.
   2. The configuration of ``SecurityCounter`` and ``OTA`` must be consistent.
