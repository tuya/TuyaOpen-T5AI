**Wi-Fi Getting_started**
==================================

:link_to_translation:`en:[English]`

在 `example <https://gitlab.bekencorp.com/armino/bk_idk/-/tree/release/v2.0.1/projects/wifi>`_ 中提供了几个独立的 Wi-Fi 驱动程序的 Demo，用户可以根据指导单独编译烧录后直接运行，后续还将继续补充更多的应用程序 Demo

.. _Wi-Fi Scan Demo:

**Wi-Fi Scan Demo**
-----------------------

`scan_Kconfig.projbuild <https://gitlab.bekencorp.com/armino/bk_idk/-/tree/release/v2.0.1/projects/wifi/scan/main>`_ 中定义了 Wi-Fi Scan Demo 的参数：
::

    menu "Example Configuration"
        config EXAMPLE_SCAN_ALL_SSID
            bool "Scan All SSID"
            default y                                    //全信道扫描

        config EXAMPLE_WIFI_SSID
            string "WiFi SSID"
        depends on !EXAMPLE_SCAN_ALL_SSID
            default "aclsemi"                            //指定 SSID 扫描
    endmenu

按照上述配置项说明，客户根据自己的应用进行相应的配置、保存，执行编译命令、烧录版本，开机即可实现开机自动扫描。编译命令如下：
::

    make bk7258 PROJECT=wifi/scan

编译成功后生成的烧录文件路径：
::

    build/scan/bk7258/all-app.bin

执行 Log 参考如下：
::

    wifi:I(1134):scan found 4 AP                   //指定 SSID 扫描
    SSID                             BSSID               RSSI chan security
    -------------------------------- -----------------   ---- ---- ---------
    aclsemi                          74:50:4e:3f:48:b0    -29  6 WPA2-AES
    aclsemi                          74:50:4e:3f:84:d0    -32  1 WPA2-AES
    aclsemi                          74:50:4e:3f:99:50    -33  6 WPA2-AES
    aclsemi                          74:50:4e:3f:82:50    -58 11 WPA2-AES
    wifi:I(1136):scan found 32 AP                  //全信道扫描
    SSID                             BSSID               RSSI chan security
    -------------------------------- -----------------   ---- ---- ---------
    Redmi_253C                       24:cf:24:3a:25:3e    -24  4 WPA2-MIX
    TP-LINK-1                        18:f2:2c:74:1d:dd    -30  1 WPA2-AES
    aclsemi                          74:50:4e:3f:48:b0    -31  6 WPA2-AES
    aclsemi                          74:50:4e:3f:99:50    -33  6 WPA2-AES
    TP-LINK_6C28                     50:fa:84:8f:6c:28    -34  1 WPA2-AES
    Xiaomi_0296                      8c:de:f9:b6:07:c2    -35  6 WPA2-MIX
    aclsemi                          74:50:4e:3f:84:d0    -36  1 WPA2-AES
    PowerTest                        c8:3a:35:7f:7e:e0    -38 11 WPA2-AES
    MP                               c0:61:18:2b:82:b4    -44 11 WPA2-AES
    ChinaNet-HeLg                    cc:c2:e0:6e:0f:a9    -45 13 WPA2-MIX


.. _Wi-Fi Sta_connect Demo:

**Wi-Fi Sta_connect Demo**
----------------------------

Wi-Fi Station Demo 是通过配置好目标 AP 的 SSID 和 Password，编译并烧录版本后，开机后 Armino Wi-Fi 驱动程序会自动扫描并连接至配置的目标 AP。`sta_connect_Kconfig.projbuild <https://gitlab.bekencorp.com/armino/bk_idk/-/tree/release/v2.0.1/projects/wifi/sta_connect/main>`_ 配置如下：
::

    menu "Example Configuration"
        config EXAMPLE_WIFI_SSID
            string "Wi-Fi SSID"
            default "aclsemi"                            //目标 SSID

        config EXAMPLE_WIFI_PASSWORD
            string "Wi-Fi Password"
            default "ACL8semi"                           //目标 SSID 密码
    endmenu

编译命令：
::

    make bk7258 PROJECT=wifi/sta_connect

编译成功后生成的烧录文件路径：
::

    build/sta_connect/bk7258/all-app.bin

执行 Log 参考如下：
::

    wifi:I(308):wifi inited(1) ret(0)              //开机后初始化完成
    example:I(310):ssid:aclsemi password:ACL8semi  //连接指定 SSID、Password
    example:I(3618):STA connected to aclsemi       //连接成功
    example:I(3658):STA got ip                     //获取 IP 地址


.. _Wi-Fi Softap Demo:

**Wi-Fi AP Demo**
----------------------------

Armino Wi-Fi 开启热点的 Demo 跟之前的 scan、sta_connect 类似，都是配置好参数后，编译并烧录版本。
`softap_Kconfig.projbuild <https://gitlab.bekencorp.com/armino/bk_idk/-/tree/release/v2.0.1/projects/wifi/ap/main>`_ 参数说明如下：

::

    menu "Example Configuration"
        config EXAMPLE_WIFI_SSID
            string "WiFi SSID"
            default "myssid"                             //SoftAP SSID
        config EXAMPLE_WIFI_PASSWORD
            string "WiFi Password"
            default "12345678"                           //SoftAP Password
        config EXAMPLE_IP
            string "IP4 Address of AP"
            default "192.168.10.1"                       //Default IP Address
        config EXAMPLE_MASK
            string "Network mask of AP"
            default "255.255.255.0"                      //Default 参数可以自定义
        config EXAMPLE_GW
            string "Gateway address of AP"
            default "192.168.10.1"
        config EXAMPLE_DNS
            string "DNS address of AP"
            default "192.168.10.1"
    endmenu

编译命令：
::

    make bk7258 PROJECT=wifi/ap

编译成功后生成的烧录文件路径：
::

    build/ap/bk7258/all-app.bin

执行 Log 参考如下：
::

    wifi:I(310):wifi inited(1) ret(0)
    example:I(312):ssid:myssid  key:mypassword
    wifi:I(314):ap configuring
    wifi:I(316):ap configured
    wifi:I(318):ap starting
    wifi:I(320):init 1st vif
    hitf:I(544):start apm success, vif0, channel0, bcmc2
    os:I(6070):create dhcp-server, tcb=30029de0, stack=[30030280-30030880:1536], prio=2
    wifi:I(6072):ap started