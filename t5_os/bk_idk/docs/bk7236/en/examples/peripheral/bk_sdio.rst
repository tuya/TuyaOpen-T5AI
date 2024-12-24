FatFS file system & SD card operation
=====================================

:link_to_translation:`en:[English]`

1 Filesytem Description
-------------------------------------
	The BEKEN project supports the peripherals such as SD cards, FLASH, and USB to store user data. When users actually operate data, they usually only need to call open-source file system functions to read and write user data to the storage device.

	The SDK project mainly uses the FatFS file system to achieve read and write operations on SD cards and USB; We mainly use EasyFlash to operate on flash. Both are open-source applications. And in SDK projects, users only need to perform simple configurations to operate storage devices such as SD cards and flash through file system functions.

	In the SDK project, the FatFS file system is used for file management of SD cards. Users only need to perform simple configurations to use the SD card. This document mainly introduces the API and cli commands for operating the FatFS file system.

	FatFs is a universal FAT file system designed for small embedded systems. It is entirely written in ANSI C language and is completely independent of the underlying I/O media, so it can be easily ported to other processors such as 8051, PIC, ARM, etc.

	FatFs supports formats such as FAT12, FAT16, FAT32, etc. Therefore, based on SDIO or SPI Flash chip drivers, we can port the FatFs file system code into the project and use various functions of the file system to achieve read and write operations on SD cards or SPI Flash chips in "file" format.

2 FatFS demo code path
-------------------------------------
	demo code path：``components\bk_cli\cli_fatfs.c`` ``components\fatfs\test_fatfs.c``

3 FatFS API Description
-------------------------------------
	The FATFS file system has been ported in the SDK project. For detailed introduction of FATFS related APIs, please refer to the link: ``http://elm-chan.org/fsw/ff/00index_e.html``

	The following is an introduction to commonly used FatFS API functions:

	1. f_mount

	 a) Function:	Register and unregister a workspace (file system object) on the FatFs module
	 b) Definition:	FRESHULT f_mount (FATFS * fs, const TCHAR * path, BYTE opt)
	 c) Parameter:	fs: pointer to workspace (file system object); path: Register/Unregister the logical drive letter for the workspace; opt: Registration or Cancellation Options;
	 d) Return:	Operation result

	2. f_open

	 a) Function:	Create/Open a File Object
	 b) Definition:	FRESHULT f_open (FIL * fp, const TCHAR * path, BYTE mode)
	 c) Parameter:	fp: Pointer to the file object structure to be created;	path: File name pointer, specifying the file name to be created or opened; mode: Access type and opening method, specified by a combination of the following standards
	 d) Return:	Operation result

	3. f_close

	 a) Function:	Close an open file
	 b) Definition:	FRESHULT f_close (FIL * fp)
	 c) Parameter:	fp: Pointer to the open file object structure that will be closed
	 d) Return:	Operation result

	4. f_read

	 a) Function:	Read data from an open file
	 b) Definition:	FRESHULT f_read (FIL * fp, void * buff, UINT btr, UINT * br)
	 c) Parameter:	fp: Pointer to the open file object structure that will be read; buff: Pointer to the buffer where the read data is stored; btr: The number of bytes to be read; br: Pointer to the UINT variable that returns the number of bytes read, and returns the actual number of bytes read;
	 d) Return:	Operation result

	5. f_write

	 a) Function:	Write data to an open file
	 b) Definition:	FRESHULT f_write (FIL * fp, void * buff, UINT btw, UINT * bw)
	 c) Parameter:	fp: Pointer to the open file object structure that will be written to; buff: Pointer to the buffer that stores written data; btw: The number of bytes to be written; bw: Pointer to the UINT variable that returns the number of bytes written, and returns the actual number of bytes written;
	 d) Return:	Operation result

	FatFs also has many API operation functions, which will not be explained in detail here. For detailed information, please refer to the FatFs file system official website.

4 FatFS macro configuration and commandline description
-------------------------------------------------------

	SDIO demo macro configuration：

	+--------------------------------------+-----------------------------------------------------------+--------------------------------------------+---------+
	|                 NAME                 |      Description                                          |                  File                      |  value  |
	+======================================+===========================================================+============================================+=========+
	|CONFIG_SDIO_V1P0                      | support SDIO version V1P0,unused                          | ``middleware\soc\bk7236\bk7236.defconfig`` |    n    |
	+--------------------------------------+-----------------------------------------------------------+--------------------------------------------+---------+
	|CONFIG_SDIO_V2P0                      | support SDIO version V2P0,current version                 | ``middleware\soc\bk7236\bk7236.defconfig`` |    y    |
	+--------------------------------------+-----------------------------------------------------------+--------------------------------------------+---------+
	|CONFIG_SDIO_HOST                      | set to host mode for connecting SD card                   | ``middleware\soc\bk7236\bk7236.defconfig`` |    y    |
	+--------------------------------------+-----------------------------------------------------------+--------------------------------------------+---------+
	|CONFIG_SDIO_SLAVE                     | support SDIO slave mode,unused                            | ``middleware\soc\bk7236\bk7236.defconfig`` |    n    |
	+--------------------------------------+-----------------------------------------------------------+--------------------------------------------+---------+
	|CONFIG_SDIO_CHANNEL_EN                | support SDIO logical channel,depend on CONFIG_SDIO_SLAVE  | ``middleware\soc\bk7236\bk7236.defconfig`` |    n    |
	+--------------------------------------+-----------------------------------------------------------+--------------------------------------------+---------+
	|CONFIG_SDIO_BIDIRECT_CHANNEL_EN       | support SDIO bidirect channel,depend on CONFIG_SDIO_SLAVE | ``middleware\soc\bk7236\bk7236.defconfig`` |    n    |
	+--------------------------------------+-----------------------------------------------------------+--------------------------------------------+---------+
	|CONFIG_SDIO_GDMA_EN                   | support SDIO GDMA,unsuport,depend on CONFIG_SDIO_SLAVE    | ``middleware\soc\bk7236\bk7236.defconfig`` |    n    |
	+--------------------------------------+-----------------------------------------------------------+--------------------------------------------+---------+
	|CONFIG_GPIO_NOTIFY_TRANSACTION_EN     | support notify transaction,,depend on CONFIG_SDIO_SLAVE   | ``middleware\soc\bk7236\bk7236.defconfig`` |    n    |
	+--------------------------------------+-----------------------------------------------------------+--------------------------------------------+---------+
	|CONFIG_SDIO_4LINES_EN                 | support SDIO 4 line,if no,support 1 line                  | ``middleware\soc\bk7236\bk7236.defconfig`` |    n    |
	+--------------------------------------+-----------------------------------------------------------+--------------------------------------------+---------+
	|CONFIG_SDIO_TEST_EN                   | support SDIO testself                                     | ``middleware\soc\bk7236\bk7236.defconfig`` |    n    |
	+--------------------------------------+-----------------------------------------------------------+--------------------------------------------+---------+
	|CONFIG_SDIO_DEBUG_EN                  | support SDIO debug                                        | ``middleware\soc\bk7236\bk7236.defconfig`` |    n    |
	+--------------------------------------+-----------------------------------------------------------+--------------------------------------------+---------+
	|CONFIG_SDIO_HOST_DEFAULT_CLOCK_FREQ=7 | set SDIO host clk,default 100k                            | ``middleware\soc\bk7236\bk7236.defconfig`` |    7    |
	+--------------------------------------+-----------------------------------------------------------+--------------------------------------------+---------+

	SD card demo macro configuration：

	+--------------------------------------+---------------------------+--------------------------------------------+---------+
	|                 NAME                 |      Description          |                  File                      |  value  |
	+======================================+===========================+============================================+=========+
	|CONFIG_SDCARD_HOST                    | support SD card,host mode | ``middleware\soc\bk7236\bk7236.defconfig`` |    y    |
	+--------------------------------------+---------------------------+--------------------------------------------+---------+
	|CONFIG_SDCARD_V2P0                    | support SD version V2P0   | ``middleware\soc\bk7236\bk7236.defconfig`` |    y    |
	+--------------------------------------+---------------------------+--------------------------------------------+---------+
	|CONFIG_SDCARD_V1P0                    | support SD version V1P0   | ``middleware\soc\bk7236\bk7236.defconfig`` |    n    |
	+--------------------------------------+---------------------------+--------------------------------------------+---------+
	|CONFIG_SDCARD_BUSWIDTH_4LINE          | support SD card 4 line    | ``middleware\soc\bk7236\bk7236.defconfig`` |    n    |
	+--------------------------------------+---------------------------+--------------------------------------------+---------+

	FatFS demo macro configuration：

	+--------------------------------------+------------------------+--------------------------------------------+---------+
	|                 NAME                 |      Description       |                  File                      |  value  |
	+======================================+========================+============================================+=========+
	|CONFIG_FATFS                          | support fatfs          | ``middleware\soc\bk7236\bk7236.defconfig`` |    y    |
	+--------------------------------------+------------------------+--------------------------------------------+---------+
	|CONFIG_FATFS_SDCARD                   | SD card support fatfs  | ``middleware\soc\bk7236\bk7236.defconfig`` |    y    |
	+--------------------------------------+------------------------+--------------------------------------------+---------+

	The FatFS commandline supported in demo：

	+----------------------------------------+------------------------------------------------+----------------------------------------+
	|             Command                    |            Param                               |              Description               |
	+========================================+================================================+========================================+
	|                                        | {M|U}: mount|unmount                           |                                        |
	|  fatfstest {M|U} {DISK_NUMBER}         +------------------------------------------------+  apply for or release the workspace    |
	|                                        | {DISK_NUMBER}:logical driver number            |  for logical drivers                   |
	+----------------------------------------+------------------------------------------------+----------------------------------------+
	| fatfstest G {DISK_NUMBER}              | {DISK_NUMBER}:logical driver number            | get the size of remaining disk space   |
	+----------------------------------------+------------------------------------------------+----------------------------------------+
	| fatfstest S {DISK_NUMBER}              | {DISK_NUMBER}:logical driver number            | scan all files on disk                 |
	+----------------------------------------+------------------------------------------------+----------------------------------------+
	| fatfstest F {DISK_NUMBER}              | {DISK_NUMBER}:logical driver number            | format disk                            |
	+----------------------------------------+------------------------------------------------+----------------------------------------+
	|                                        | {DISK_NUMBER}:logical driver number            |                                        |
	| fatfstest R {DISK_NUMBER}{file_name}   +------------------------------------------------+                                        |
	|                                        | {file_name}:file to story read data            | read specified length of data          |
	| {length}                               +------------------------------------------------+                                        |
	|                                        | {length}: length to be read                    | from the file                          |
	+----------------------------------------+------------------------------------------------+----------------------------------------+
	|                                        | {DISK_NUMBER}:logical driver number            |                                        |
	| fatfstest W {DISK_NUMBER}{file_name}   +------------------------------------------------+                                        |
	|                                        | {file_name}:file to be written                 | write data to a file                   |
	| {content_p}{content_len}               +------------------------------------------------+                                        |
	|                                        | {content_p}: pointer to the data to be written |                                        |
	|                                        +------------------------------------------------+                                        |
	|                                        | {content_len}:length to be written             |                                        |
	+----------------------------------------+------------------------------------------------+----------------------------------------+
	|                                        | {DISK_NUMBER}:logical driver number            |                                        |
	| fatfstest D {DISK_NUMBER}{file_name}   +------------------------------------------------+                                        |
	|                                        | {file_name}:file to be written                 | read the specified length of data from |
	| {start_addr}{content_len}              +------------------------------------------------+ the specified address and write it     |
	|                                        | {start_addr}: start address for reading        | to the specified file                  |
	|                                        +------------------------------------------------+                                        |
	|                                        | {content_len}:length to be written             |                                        |
	+----------------------------------------+------------------------------------------------+----------------------------------------+
	|                                        | {DISK_NUMBER}:logical driver number            |  auto test,write the data to the file  |
	| fatfstest A {DISK_NUMBER}{file_name}   +------------------------------------------------+  and then read it, and compare the     |
	|                                        | {file_name}:file to be written                 |  result                                |
	| {content_len}{test_cnt} {start_addr}   +------------------------------------------------+                                        |
	|                                        | {content_len}: length of comparison            | note: the data written to the SD card  |
	|                                        +------------------------------------------------+ is read from the specified start_addr  |
	|                                        | {test_cnt}:number of cycle tests               |                                        |
	|                                        +------------------------------------------------+                                        |
	|                                        | {start_addr}:start address for reading         |                                        |
	+----------------------------------------+------------------------------------------------+----------------------------------------+

	disk_number definition：

   ::

		typedef enum
	{
	    DISK_NUMBER_RAM  = 0,
	    DISK_NUMBER_SDIO_SD = 1,
	    DISK_NUMBER_UDISK   = 2,
	    DISK_NUMBER_FLASH   = 3,
	    DISK_NUMBER_COUNT,
	} DISK_NUMBER;



5 Sample demonstration 
-------------------------------------
	The demo steps to be executed are as follows：

	1、Insert the SD card into the development board, and the GPIO connection method is as follows (due to GPIO multiplexing, the SDIO configuration in this demo is single line mode)

	::

		SD_CLK----GPIO14
		SD_CMD----GPIO15
		SD_D0-----GPIO16
		SD_D1-----GPIO17
		SD_D2-----GPIO18
		SD_D3-----GPIO19

	2、SD card operation

fatfstest M 1    //mount the SD card

   ::

	[16:06:10.103]→◇fatfstest M 1
	[16:06:10.108]←◆fatfstest M 1
	error file name,use defaultfilename.txt
	sd_card:I(203942):sd card has inited
	fmt=2
	fmt2=0
	Fatfs:I(203944):f_mount OK!
	Fatfs:I(203944):----- test_mount 1 over  -----


fatfstest S 1   //scan the SD card

   ::

		[16:11:39.041]→◇fatfstest S 1
		[16:11:39.046]←◆fatfstest S 1
		error file name,use defaultfilename.txt
		Fatfs:I(532878):
		----- scan_file_system 1 start -----
		Fatfs:I(532878):1:/
		Fatfs:I(532880):1:/autotest_400.txt
		Fatfs:I(532882):1:/dump_1.txt
		Fatfs:I(532884):scan_files OK!
		Fatfs:I(532886):----- scan_file_system 1 over  -----

fatfstest W 1 test.txt acl_bk7236_write_to_test 24   //Write a string "acl_bk7236_write_to_test" to the file "test.txt"

   ::

		[16:15:02.687]→◇fatfstest W 1 test.txt acl_bk7236_write_to_test 24

		[16:15:02.696]←◆fatfstest W 1 test.txt acl_bkFatfs:I(736530):
		----- test_fatfs 1 start -----
		Fatfs:I(736530):f_open "1:/test.txt"
		Fatfs:I(736530):.7236_write_to_test 24

		[16:15:02.837]←◆TODO:FATFS sync feature
		Fatfs:I(736678):f_close OK
		Fatfs:I(736678):----- test_fatfs 1 over  -----

		append and write:test.txt
		[16:15:02.866]←◆,acl_bk7236_write_to_test

fatfstest R 1 test.txt 32     //Read 32 bytes of data from the file test.txt

  ::

	[16:18:30.473]→◇fatfstest R 1 test.txt 32
	[16:18:30.478]←◆fatfstest R 1 test.txt 32
	Fatfs:I(944312):
	----- test_fatfs 1 start -----
	Fatfs:I(944312):f_open "1:/test.txt"
	Fatfs:I(944314):will read left_len = 24
	Fatfs:I(944314):f_read start:24 bytes

	====== f_read one cycle - dump(len=24) begin ======
	61 63 6c 5f 62 6b 37 32 35 36 5f 77 72 69 74 65
	5f 74 6f 5f 74 65 73 74
	====== f_read one cycle - dump(len=24)   end ======

	Fatfs:I(944328):f_read one cycle finish:left_len = 0
	Fatfs:I(944332):f_read: read total byte = 24
	Fatfs:I(944336):f_close OK
	Fatfs:I(944338):----- test_fatfs 1 over  -----

	read test.txt, len_h = 0, len_l = 32

fatfstest A 1 autotest.txt 2222 1 0   //Read 2222 bytes of data from the flash address 0x0, and save it into file autotest.txt of SD card, then read the data from autotest.txt for comparison; Perform this operation once

   ::

	[16:31:11.077]→◇fatfstest A 1 autotest.txt 2222 1 0
	[16:31:11.083]←◆fatfstest A 1 autotest.txt 2222 1 0

	[16:31:11.143]←◆TODO:FATFS sync feature
	Fatfs:I(195362):auto test succ

fatfstest D 1 dump.txt 0 10240    //Read 1024 bytes of data from the flash address 0x0, and save it into file dump.txt

   ::

	[16:33:15.934]→◇fatfstest D 1 dump.txt 0 10240
	[16:33:15.939]←◆fatfstest D 1 dump.txt 0 10240
	Fatfs:I(320154):
	----- test_fatfs_dump 1 start -----
	Fatfs:I(320154):file_name=dump.txt,start_addr=0x0,len=10240
	Fatfs:I(320154):f_open start "1:/dump.txt"
	Fatfs:I(320154):f_write start
	Fatfs:I(320158):f_write end len = 10240
	Fatfs:I(320158):f_lseek start
	Fatfs:I(320158):f_close start
	TODO:FATFS sync feature

fatfstest F 1   //Format SD card

   ::

	[17:43:49.985]→◇fatfstest F 1
	[17:43:49.990]←◆fatfstest F 1
	error file name,use defaultfilename.txt
	sd_card:I(327564):sd card has inited
	part=0
	sd_card:I(327564):card ver=2.0,size:0x01dacc00 sector(sector=512bytes)
	sdcard sector cnt=31116288

	[17:43:55.480]←◆TODO:FATFS sync feature
	Fatfs:I(333054):f_mkfs OK!
	format :1

