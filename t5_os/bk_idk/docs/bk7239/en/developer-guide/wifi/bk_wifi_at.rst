.. _WiFi-AT:

Wi-Fi AT命令集
====================================================
Armino Wi-Fi AT命令集方便客户熟悉BEKEN Wi-Fi产品基本功能，能够快速了解Wi-Fi的基本使用方法，同时也为自动化脚本提供了标准接口

-  :ref:`介绍 <cmd-wifi-intro>`
-  :ref:`AT+WIFI=SCAN <cmd-SCAN>`：扫描热点
-  :ref:`AT+WIFI=STACONNECT <cmd-STACONNECT>`：STA连接
-  :ref:`AT+WIFI=APSTART <cmd-STARTAP>`：开启SoftAP
-  :ref:`AT+WIFI=STATE <cmd-STATE>`：查看STA/SoftAP参数
-  :ref:`AT+WIFI=STOP <cmd-STOP>`：关闭SoftAP/STA
-  :ref:`AT+WIFI=PING <cmd-PING>`：ping
-  :ref:`AT+MAC <cmd-MAC>`：查看/修改设备MAC地址
-  :ref:`AT+RST <cmd-RST>`：重启设备

.. _cmd-wifi-intro:

介绍
-----------

.. important::
    Armino SDK默认支持上述所有的AT命令，未来还将支持更多的AT命令

.. _cmd-SCAN:

:ref:`AT+WIFI=SCAN <WiFi-AT>`：**扫描热点**
---------------------------------------------

**执行命令**
^^^^^^^^^^^^^^^

**功能：**

扫描周边热点，并返回扫描结果

**命令：**
::

    AT+WIFI=SCAN,<ssid>

**响应：**
::

    CMDRSP:OK
    或者
    <error reason>
    CMDRSP:ERROR

**参数**
^^^^^^^^^^

-  <ssid>: 指定SSID扫描，参数为空表示执行全信道扫描

**示例**
^^^^^^^^^^^^

::

    //全信道扫描
    AT+WIFI=SCAN
    CMDRSP:OK
    scan found 32 AP
    SSID                     BSSID                RSSI chan security

    TP-Link_F49B             68:ff:7b:ae:f4:9b    -40   6   WPA2-MIX
    Xiaomi_1DBC              d4:da:21:a4:1d:bd    -36   6   WPA2-AES
    Redmi_253C               24:cf:24:3a:25:3e    -42  11   WPA2-AES
    ...

    //指定SSID扫描
    AT+WIFI=SCAN,aclsemi
    CMDRSP:OK
    scan found 3 AP
    SSID                     BSSID               RSSI chan  security

    aclsemi                  58:c7:ac:7e:2d:f0    -50  11   WPA2-AES
    aclsemi                  58:c7:ac:7e:32:a0    -59  1    WPA2-AES
    aclsemi                  58:c7:ac:7e:2a:30    -65  6    WPA2-AES

.. _cmd-STACONNECT:

:ref:`AT+WIFI=STACONNECT <WiFi-AT>`：**STA连接**
--------------------------------------------------------

**执行命令**
^^^^^^^^^^^^^^^

**功能：**

执行Wi-Fi Station连接目标AP操作

**命令：**
::

    AT+WIFI=STACONNECT,<ssid>,<pwd>

**响应：**
::

    CMDRSP:OK
    或者
    <error reason>
    CMDRSP:ERROR

**参数**
^^^^^^^^^^^

-  <ssid>：目标AP的SSID
    - 如果 SSID 和密码中有 ``,``、``"``、``\\`` 等特殊字符，需转义
    - 支持连接SSID为中文的AP，但是某些路由器或者热点的中文SSID不是UTF-8编码格式。您可以先扫描SSID，然后使用扫描到的SSID进行连接
-  <pwd>：目标AP的PASSWORD，如果热点不加密，则无需输入
    - 密码最长63字节

**示例**
^^^^^^^^^^^^

::

    AT+WIFI=STACONNECT,aclsemi,ACL8semi
    STA connected aclsemi
    STA got ip
    CMDRSP:OK

.. _cmd-STARTAP:

:ref:`AT+WIFI=APSTART <WiFi-AT>`：**开启SoftAP**
------------------------------------------------------

**执行命令**
^^^^^^^^^^^^^^^

**功能：**

开启SoftAP，支持OPEN或者加密模式，支持指定信道

**命令：**
::

    AT+WIFI=APSTART,<ssid>,<pwd>,<channel>

**响应：**
::

    CMDRSP:OK
    或者
    <error reason>
    CMDRSP:ERROR

**参数**
^^^^^^^^^^^

-  <ssid>：字符串参数，接入点名称
-  <pwd>：字符串参数，密码，范围8~63字节ASCII，为空表示不加密
-  <channel>：信道号，缺省为信道11

**示例**
^^^^^^^^^^^^

::

    AT+WIFI=APSTART,test321,12345678,10
    chan_ctxt_add: CTXT1, freq=2457MHz, bw=20MHz, pwr=0dBm
    beacon_int_set:100 TU
    ap started
    CMDRSP:OK

.. _cmd-STATE:

:ref:`AT+WIFI=STATE <WiFi-AT>`：**查看STA/SoftAP参数**
----------------------------------------------------------

**执行命令**
^^^^^^^^^^^^^^^^

**功能：**

用于查看当前设备的模式状态，支持sta/ap查看ip地址、信道、ssid、bssid、rssi等信息

**命令：**
::

    AT+WIFI=STATE,<mode>,<rssi>,<channel>,<bssid>,<ssid>,<ip>

**响应：**
::

    CMDRSP:parameters
    CMDRSP:OK
    或者
    bad parameters
    CMDRSP:ERROR

**参数**
^^^^^^^^^^^^

-  <mode>：STA/AP，支持查询STA/AP身份的参数
-  <rssi>：信号强度，一般为负数
-  <channel>：信道号，数值在1-14之间
-  <ssid>：接入点名称
-  <bssid>：接入点AP的MAC地址
-  <ip>：设备的IP地址，SoftAP的默认地址为192.168.0.1

**示例**
^^^^^^^^^^

::

    //查询SoftAP信道
    AT+WIFI=STATE,AP,CHANNEL
    CMDRSP::11
    CMDRSP:OK
    //查询STA连接的RSSI值
    AT+WIFI=STATE,STA,RSSI
    CMDRSP::-40
    CMDRSP:OK

**说明**
^^^^^^^^^^^

-  STA支持查询：SSID、CHANNEL、IP、**RSSI**、**BSSID**
-  AP支持查询：SSID、CHANNEL、IP、**SECURITY**

.. _cmd-STOP:

:ref:`AT+WIFI=STOP <WiFi-AT>`：**关闭STA/SoftAP**
------------------------------------------------------

**执行命令**
^^^^^^^^^^^^^^^

**功能：**

用于停止当前的SoftAP/STA身份

**命令：**
::

    AT+WIFI=STOP,<mode>

**响应：**
::

    CMDRSP:OK
    或者
    <error reason>
    CMDRSP:ERROR

**参数**
^^^^^^^^^^^^^^

-  <mode>：STA/AP

**示例**
^^^^^^^^^^^

::

    //停止STA
    AT+WIFI=STOP,STA
    sta stopped
    CMDRSP:OK
    //停止SoftAP
    AT+WIFI=STOP,AP
    ap stopped
    CMDRSP:OK

.. _cmd-PING:

:ref:`AT+WIFI=PING <WiFi-AT>`：**Ping**
------------------------------------------------------

**执行命令**
^^^^^^^^^^^^^^^^

**功能：**

向目标主机发送ICMP Echo请求报文，测试目的站是否可达及了解其有关状态

**命令：**
::

    AT+WIFI=PING,<host>,<num>,<bytes>

**响应：**
::

    CMDRSP:OK
    或者
    <error reason>
    CMDRSP:ERROR

**参数**
^^^^^^^^^^^

-  <host>：目的站点的IP地址，需在同一网段内
-  <num>：Ping包个数，默认为4
-  <bytes>：设置数据包大小，为空则为缺省值

**示例**
^^^^^^^^^

::

    AT+WIFI=PING,192.168.1.1,10
    ping: size:32 times:10
    CMDRSP:OK

.. _cmd-MAC:

:ref:`AT+MAC <WiFi-AT>`：**查询/设置设备MAC地址**
-----------------------------------------------------

**执行命令**
^^^^^^^^^^^^^^^^

**功能：**

用于查询/修改当前设备的MAC地址

**命令：**
::

    AT+MAC //查询设备MAC地址
    AT+MAC=<param> //修改设备MAC地址

**响应：**
::

    CMDRSP:OK
    或者
    <error reason>
    CMDRSP:ERROR

**示例**
^^^^^^^^^^^^^

::

    //查询
    AT+MAC
    base mac: c8:47:8c:11:22:34
    sta mac: c8:47:8c:11:22:34
    ap mac: c8:47:8c:11:22:35
    CMDRSP:OK
    //设置
    AT+MAC=c8478c334455
    set base mac: c8:47:8c:33:44:55
    CMDRSP:OK

.. _cmd-RST:

:ref:`AT+RST <WiFi-AT>`：**重启设备**
-----------------------------------------------------

**执行命令**
^^^^^^^^^^^^^^^^^^^

**功能：**

重启设备

**命令：**
::

    AT+RST

**响应：**
::

    CMDRSP:OK
    或者
    <error reason>
    CMDRSP:ERROR

**示例**
^^^^^^^^^^^^^

::

    AT+RST
    bk_reboot
    CMDRSP:OK


:link_to_translation:`en:[English]`

