四．L_bootloader profile
----------------------------

l_bootloader: mainly downloads uart. Figure 2 shows the flowchart.

  - 1.After romcode is executed, the system jumps to address 0x0 of the flash.
  - 2.In this case, the system checks whether there is data in the uart rx_buf. If there is data, the system preemps the uart bus and starts to download data.
  - 3.If there is no data in the uart rx_buf file, the up_boot is executed at the specified IP address (the logical IP address) after a certain number of times of cyclic detection.
  - 4.After the system starts normally, it runs the app service. After receiving the download handshake information, the uart automatically reboot to access the l_bootloader download program.

