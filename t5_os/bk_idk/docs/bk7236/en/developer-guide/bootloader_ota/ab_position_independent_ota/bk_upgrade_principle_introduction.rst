The upgrade principle of the AB partition is location independent
--------------------------------------------------------------------

:link_to_translation:`zh_CN:[中文]`

 - When the APP(application) runs in partition A and performs OTA upgrade,
    - the upgrade firmware is downloaded from the server to partition B according to the partition table. After the download is successful, the reboot is performed.
    - Then, the bootloader performs hash verification to verify the accuracy of the data. If there is no problem with the data, the jump address is set to the start address of area A and the bit value of the Flash-related control register is set to 1, then it runs to partition B.
    -  Otherwise, set the jump address to the start address of area A and set the bit value of the Flash-related control register to 0, or run in partition A.

 - Similarly, when the APP is running in partition B and performing an over-the-air (OTA) upgrade,
    - image A is downloaded from the server. After the image is downloaded, reboot is performed.
    - Then, the bootloader hashes to verify the accuracy of the data, and sets the jump address to the start address of area A and the bit value of the Flash-related control register to 0, then it runs to partition A.
    - Otherwise, set the jump address to the start address of area A and set the bit value of the Flash-related control register to 1, or run in partition B.

