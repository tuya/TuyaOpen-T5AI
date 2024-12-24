五.Bootloader升级固件相关info
-------------------------------

:link_to_translation:`en:[English]`

- 1.升级固件制作有两种方式：

  - 1）方式一：通过工具手动生成；

  - 2）方式二：由编译打包的时候自动化生成；（首选方式二）

- 2.升级固件路径为：

.. important::
   在build/app/bk7258/encrypt/app_pack.rbl

- 3.打开Everything-1.4.1.935.x64-Setup.exe，工具->选项->HTTP服务器，绑定本机ip。使用浏览器打开本机的ip地址，找到本地的升级文件 \*.rbl，拷贝url出来，用于cli命令。

 .. figure:: ../../../../_static/bootlaoder_everthing.png
    :align: center
    :alt: bootlaoder_everthing
    :figclass: align-center

    图6 Everything工具页面

- 4.使用串口发送cli命令，例如：
  http_ota http://192.168.21.101/D%3A/E/build/app_pack.rbl

