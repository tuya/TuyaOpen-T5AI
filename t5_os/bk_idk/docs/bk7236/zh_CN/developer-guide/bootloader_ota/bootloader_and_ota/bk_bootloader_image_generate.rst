四.Bootloader镜像文件生成
----------------------------

:link_to_translation:`en:[English]`

- 1.直接编译生成bootloader.bin；具体编译步骤如下：

  - 1）进入bootloader代码目录下；
  - 2）make clean
  - 3）make
  - 4）在PACK目录下执行generate.bat脚本，在generate_out目录下生成bootloader.bin以及对应的bootloader_crc.bin
