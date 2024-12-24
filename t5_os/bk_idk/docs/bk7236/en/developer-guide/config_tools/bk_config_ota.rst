.. _bk_config_ota:

OTA Configuration
==========================

:link_to_translation:`zh_CN:[中文]`

OTA is mainly used to configure OTA strategy and security counters for anti-version downgrade attacks.

OTA Configuration Table
----------------------------

The OTA configuration table is a CSV file, the following is a configuration example:

+-------------------------+--------------------+
| Field                   | Value              |
+=========================+====================+
| strategy                | SWAP               |
+-------------------------+--------------------+
| app_security_counter    | 3                  |
+-------------------------+--------------------+


The partition table is fixed to two columns:

 - ``strategy`` - optional, indicating the strategy of MCUBOOT OTA, which can be ``SWAP`` , ``XIP`` or ``OVERWRITE`` , respectively corresponding to MCUBOOT_SWAP_USING_MOVE, MCUBOOT_DIRECT_XIP and MCUBOOT_OVERWRITE_ONLY in MCUBOOT
 - ``app_security_counter`` - required, represents the version security counter. It must be configured when secure boot is enabled to prevent version downgrade attacks. After Secure Boot verifies that the version is legitimate
    This value will be read, compared with the safety counter in OTP/NV, and the start is only allowed if the safety counter of the upgraded version is not less than the value in OTP/NV.

.. _bk_config_ota_strategy:

OTA upgrade strategy and its corresponding partition requirements
----------------------------------------------------------------------------------
The following will explain to you three upgrade strategies based on MCUBOOT. Please refer to the partition configuration document : ref: `Partition table <_ bk_config_partitions_table > ` to understand together. For specific principles, please refer to MCUBOOT’s official documentation `<https://docs.mcuboot.com/design.html>`

SWAP
++++++++++++++++++++++++++++++
SWAP always runs in the primary partition, and the firmware to be upgraded will be placed in the secondary partition during OTA download. **The sizes of the two must be the same**. When upgrading, MCUBOOT will exchange the contents of the secondary partition with the primary partition and try to run
If the firmware in the primary partition can run successfully at this time, the contents of the primary partition will be retained and will be booted from the primary partition thereafter. If the startup fails, the contents of the secondary partition will be swapped back to the primary partition , ensuring that you always have a working version.

XIP
+++++++++++++++++++++++++++
XIP also requires two identical partitions, but they are no longer primary and secondary. Instead, when upgrading, they will directly try to boot from the other partition. For example, when the program is running in partition A and the firmware to be upgraded is downloaded through OTA and placed in partition B, MCUBOOT will try
Run the firmware in the B partition. If it can run successfully, it will keep running in the B partition. If it fails, MCUBOOT will restart the previous firmware from the A partition. It also ensures that you always have the version you want to run. For the sake of unification, we still retain
`primary_` and `secondary_` prefixes, and the primary partition is used as the A partition, and the secondary partition is used as the B partition.

OVERWRITE
++++++++++++++++++++++++++++
There is only one primary partition in OVERWRITE to run firmware, and there is no secondary partition. We use the ota partition to store the firmware to be upgraded, and the size of this partition can be significantly smaller than the main partition. In the OVERWRITE strategy, we compress the firmware to be upgraded and
Put it into the ota partition. This partition is not used as an executable partition. Instead, it is decompressed and overwrites the primary partition during upgrade . The program still runs on the primary partition.
There is a disadvantage and an advantage to using this strategy, the disadvantage is that if the firmware to be upgraded does not work, the board will be bricked. Under this policy, only the legality of the firmware to be upgraded can be guaranteed, that is, the firmware is complete, untampered, and safe, but it cannot be guaranteed that the firmware itself is
Can run without glitches. The advantage is that it can save a lot of space, and the size of the ota partition can be determined according to the compressed size of the firmware to be upgraded. We recommend that you set the size of the ota partition to 50% of the total size of all primary partitions.

.. warning::
  When using the OVERWRITE strategy, please make sure that the upgraded firmware can run, otherwise it will be irreversible!