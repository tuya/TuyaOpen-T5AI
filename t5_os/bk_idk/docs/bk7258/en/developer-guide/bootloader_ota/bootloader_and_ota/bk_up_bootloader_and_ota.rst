五．Up_bootloader profile
----------------------------

- 1.up_bootloader function:
  Implements the OTA upgrade function.
- 2.OTA upgrade principle:
  Over-the-air (OTA) (over-the-air download technology) supports remote firmware upgrade on the network. In the embedded device OTA, the upgrade data packet is usually downloaded to the Flash through the serial port or network, and then the downloaded data packet is transported to the MCU code execution area for coverage to complete the device firmware upgrade and update function.
  The OTA upgrade of embedded devices is generally not based on the file system, but by dividing Flash into different functional areas to complete the OTA upgrade function. In embedded system scenarios, to complete an OTA firmware remote upgrade, the following three core stages are usually required:

  - 1）Upload the new firmware to OTA server
  - 2）Download the new OTA firmware on the device
  - 3）The bootloader verifies, decrypts, decompress, and moves OTA firmware (to the corresponding flash partition), and hashes the OTA firmware.
- 3.OTA upgrade process:

  - 1）Use http protocol to download OTA firmware from the server, and then write the downloaded OTA upgrade firmware to the flash download partition, and restart the device after success, as shown in Figure 3, this process is completed in the app thread (details are as follows) :

    - 1）Write firmware data to the flash download partition while downloading firmware data from the server; (Write a packet of 1k data to the download partition each time, and then read and compare the 1k data written into the download partition to ensure data integrity)

    - 2）After the firmware upgrade is successfully downloaded, run the reboot command and then start from bootrom.

  - 2）After the reboot, up_bootloader performs OTA operations on the upgrade file in the download partition, decompresses and decrypts the upgrade file, moves it to the corresponding appcode partition, erases the download partition after the verification is successful, and jumps to the app partition for normal execution, as shown in Figure 4. (Details are as follows) :

    - 1）First, the bootloader will detect whether there is data in the download partition. If there is data, it will perform the next step (crc check); otherwise, it will directly jump to the partition where the app is located and execute the app code;

    - 2）If there is data in the download partition, it will first detect whether the first four bytes of the downlaod partition are equal to the magic value. If so, crc check will be performed; otherwise, it will directly jump to the partition where the app is located and execute the app code;

    - 3）Perform crc check on the download partition data (including crc for firmware head and firmware content). If crc check passes, decompress and decrypt the data; Otherwise, delete the download partition data, jump directly to the partition where the app resides, and run the app code.

    - 4）Decompress and decrypt the download partition data, write the data to the app partition, and then do the hash test on the app partition data to ensure the accuracy of the data written to the app partition;

    - 5）Perform hash verification on the aopp partition data. If the hash verification passes, erase the download partition data and perform reboot; If the hash verification fails, reboot and repeat the preceding steps.

.. figure:: ../../../../_static/bootloader_app_process_e.png
    :align: center
    :alt: bootloader_app_process_e
    :figclass: align-center