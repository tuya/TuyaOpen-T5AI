Introduction to the process of verifying the hash function of AB partition location independent
----------------------------------------------------------------------------------------------------

:link_to_translation:`zh_CN:[中文]`

The hash algorithm is used to ensure the integrity of downloaded data. The hash verification process consists of the following two steps:

- 1. Obtain the head information of the rbl in the upgrade firmware (the rbl information is stored at the last 4k position of the upgrade firmware).
- 2. Then calculate the hash value of the entire firmware using the hash256 algorithm and compare the hash value stored in the rbl. If two hash values are the same, the hash passes and the firmware is complete; otherwise, the downloaded data is incomplete.
- 3. Upgrade zone B from zone A as an example.

  - 1.1) After A downloads B, reboot jumps to the bootloader and obtains the rbl information of firmware B.
  - 1.2) Then calculate the hash value of upgrade B using hash256 algorithm. If the hash value is equal to the hash value in rbl, the data of upgrade B is complete. Then set the bit[0] of the Flash-related control register to 1; The command is directly executed in partition B.
  - 1.3) If the hash value calculated by hash256 algorithm is inconsistent with the hash value in rbl, delete the data in partition B and set bit[0] of the Flash-related control register to 0; Then continue with partition A.
