三．up_bootloader简介
----------------------------

:link_to_translation:`en:[English]`

- 1.up_bootloader作用：
  实现OTA升级功能。

- 2.OTA升级原理：
  OTA（Over-the-AirTechnology）即空中下载技术，支持网络端远程升级固件。在嵌入式设备OTA 中，通常通过串口或者网络等方式，将升级数据包下载到Flash，然后将下载得到的数据包搬运到MCU的代码执行区域进行覆盖，以完成设备固件升级更新的功能。
  嵌入式设备的OTA升级一般不基于文件系统，而是通过对Flash划分为不同的功能区域来完成OTA升级功能。在嵌入式系统方案里，要完成一次OTA固件远端升级，通常需要以下三个核心阶段：

  - 1）上传新固件到OTA服务器
  - 2）设备端下载新的OTA升级固件
  - 3）bootloader对OTA固件进行校验、解密、解压缩和搬运（搬运到对应flash分区）、hash验证。

- 3.OTA升级过程：

  - 1）采用http协议从服务器下载OTA固件，然后将下载到的OTA升级固件写到flash的download分区，成功之后并重启设备，如图3所示，此过程在app线程中完成（具体细节如下）:

    - 1.1）一边从服务器下载固件数据一边写到flash的download分区；（每次写一包1k数据到download分区，然后将写进download分区的1k数据读出来比较，确保数据的完整性）

    - 1.2）升级固件全部下载成功之后，进行reboot，然后从bootrom启动。

  - 2）在重启后up_bootloader会对download分区的升级文件进行OTA操作解压解密后搬运到对应的appcode分区，校验成功后擦除download分区后，跳转到app分区正常执行，流程图如图4所示。（具体细节如下）:
  
    - 1.1）首先，bootloader会去检测download分区是否有数据，如果有数据，则进行下一步骤（crc校验），否则，则直接跳转到app所在分区，并执行app代码；
    
    - 1.2）如果download分区有数据，首先会检测downlaod分区的前四个字节是否等于magic值，如果等于，则进行crc校验，否则直接跳转到app所在分区，并执行app代码；
    
    - 1.3）对download分区数据做crc校验（包含固件head以及固件内容分别做crc），如果crc校验通过，则进行解压、解密流程；否则删除download分区数据，然后直接跳转到app所在分区，并执行app代码；
    
    - 1.4）对download分区数据进行解压、解密操作，并将数据写到app分区，之后做对app分区数据做hash检验，确保写到app分区的数据准确性；
    
    - 1.5）对aopp分区数据进行hash校验，若hash校验通过，则将download分区数据擦除，并进行reboot；若hash校验失败，则reboot操作，重复进行上述流程。

.. figure:: ../../../../_static/bootloader_app_process.png
    :align: center
    :alt: bootloader_process
    :figclass: align-center


