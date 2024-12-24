六.Bootloader image file generation
--------------------------------------

- 1.Directly compile and generate bootloader.bin. The specific compilation steps are as follows:

  - 1）Access the bootloader code directory.
  - 2）make clean
  - 3）make
  - 4）Run the generate.bat script in the PACK directory and generate bootloader.bin and the corresponding bootloader_crc.bin in the generate_out directory