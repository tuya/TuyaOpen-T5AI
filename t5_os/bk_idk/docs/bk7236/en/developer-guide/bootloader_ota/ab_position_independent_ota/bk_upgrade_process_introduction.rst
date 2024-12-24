This section describes the upgrade process of AB partition independent of location
-----------------------------------------------------------------------------------------

:link_to_translation:`zh_CN:[中文]`

  The location-independent upgrade process of an AB partition consists of three parts: Download the upgrade image. The Bootloader confirms the integrity process of the upgrade image. The APP confirms the final running partition (some customers need, the final execution of which partition is decided by them, controlled by the macro, if the customer does not feedback, the default jump partition is executed). During the upgrade, three flag bits are involved: they are saved in a certain 4K space of the flash (the current location is 0x3FD000).

.. important::
    - flag1: ota_temp_exec_flag: 3 -A; 4--B (temp update).
    - flag2: cust_confirm_flag: 1: indicates that the ota is successfully downloaded. 3 - A; 4--B (indicates that no ota is performed and the final decision on which partition to execute is made in the app).
    - flag3: ota_exec_flag: 0/0xFF -A; 1 - B.

The three flags used in the ab partition upgrade process are described as follows:

1）ota_temp_exec_flag: After the upgrade image is downloaded, the customer temporarily decides which partition to jump to.
.. note::

  - 3: temporarily decide to jump to partition A on behalf of the customer;
  - 4: temporarily decide to jump to partition B on behalf of the customer;(The flag bit is controlled according to the macro, if the customer does not feedback, the default flag bit is executed), and then the value will be converted, 3 will be converted to 0 (A partition), 4 will be converted to 1 (B partition), and 0/1 will be written into flash.

2）cust_confirm_flag: There are two meanings: a) The mark that the upgrade image download is complete;b) After the restart, the customer finally decides which partition to execute.
.. note::

  - 1: indicates that the download is successful (the value is set to 1 only when the download is successful), and then the data is written into the flash.
  - 3: indicates execution zone A.
  - 4: indicates execution zone B.(The flag bit is controlled according to the macro. If the customer does not respond, the default flag bit is executed.) The code will eventually change to set ota_exec_flag to 0/1, and then write it into the flash.

3）ota_exec_flag: Before downloading the upgrade image, know the partition to be upgraded based on the current code.
.. note::

  - 0: upgrade zone A.(0xFF: indicates the first burn run in partition A)
  - 1: upgrade zone B.(If the download process and checkout are successful, after reboot, it will be executed on this partition, which should be the same as the default partition represented by cust_confirm_flag), and then write to the flash.

Figure 2 shows the process for downloading the upgrade image of the AB partition:

  .. figure:: ../../../../_static/ab_app_en.png
     :align: center
     :alt: ab_app_en
     :figclass: align-center

     Figure 2 the image of download process

.. note::
    Figure 2 mainly implements the download function of the upgrade image and sets the corresponding flag. The combination of flag bits is shown in the red remarks above.

The jump process of AB partition is shown in Figure 3.

  .. figure:: ../../../../_static/ab_bootloader_en.png
     :align: center
     :alt: ab_bootloader_en
     :figclass: align-center

     Figure 3 bootloader jump process

.. note::
    - 1. Figure 3 Determines which partition to jump to based on the flag in Figure 2 and hash verification;
    - 2. Before the jump, set the flash register (the start address and length of area A, the offset of area B against area A) and the jump address to the start address of area A;
    - 3. Directly jump to area A: In this process, the flash related control register bit[0] = 0 is set first;
    - 4. Directly jump to area B: In this process, the Flash-related control register bit[0] = 1 will be set first;
