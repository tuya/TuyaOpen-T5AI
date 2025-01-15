AB partition has the advantage of location independence
-------------------------------------------------------

:link_to_translation:`zh_CN:[中文]`

 - There are two advantages to having a traditional OTA comparison:
    - Advantage 1: After the download, the reboot of the AB partition is faster. Since the image of the AB solution is not compressed, there is no decompression time during the upgrade.
    - Advantage 2: When upgrading B from A, you can modify the.csv file to configure the start address of block B. Block B can be downloaded to any location. When the upgrade is successful, you can mirror block B from any location. (Avoid using problematic versions)

