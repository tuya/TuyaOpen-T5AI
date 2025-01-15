二．l_bootloader简介
----------------------------

:link_to_translation:`en:[English]`

l_bootloader：主要是uart下载功能，流程图如上图2所示。

- 1.在romcode执行完成之后会跳转到flash的0x0地址执行；
- 2.此时会判断uart rx_buf中有是否有数据，如果有数据，抢占到uart总线进入数据下载状态，下载完成之后会reboot；
- 3.如果此时uart rx_buf中没有数据，会循环检测一定次数后会跳转到指定地址（此为逻辑地址）处执行up_boot。
- 4.系统正常启动后运行在app业务中，uart接收到下载握手信息后会主动reboot进入l_bootloader下载程序，用户也可以取消此功能。

