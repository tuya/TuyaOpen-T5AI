.. _wifi-AT:

**Wi-Fi AT命令**
=======================

:link_to_translation:`en:[English]`


.. important::

    *BK-IDK 默认支持本节介绍的所有AT命令*

    *除非特定指定，Wi-Fi AT命令的超时时间均为15s*


- :ref:`AT+WLMAC<wifi-mac>` :获取STA身份和SAP身份的MAC地址，也可以配置MAC地址

- :ref:`AT+STASTART<wifi-stastart>` :以STA身份连接至指定SSID的路由器上

- :ref:`AT+STASTOP<wifi-stastop>` :以STA身份从目前正在连接的路由器上断开

- :ref:`AT+WIFISTATUS<wifi-getstatus>` :用来查看wifi设备当前的模式状态或指定参数信息

- :ref:`AT+WIFISCAN<wifi-scan>` :扫描周围AP信息,或者按照指定参数进行扫描

- :ref:`AT+SAPSTART<wifi-sapstart>` :以SAP身份创建SoftAP

- :ref:`AT+SAPSTOP<wifi-stastop>` :停止当前开启的SoftAP

- :ref:`AT+STAPING<wifi-pingstart>` :进行指定IP地址和指定时长和每包大小的PING包操作

- :ref:`AT+STAPINGSTOP<wifi-pingstop>` :停止当前正在进行的PING包工作


.. _wifi-mac:

:ref:`AT+WLMAC<wifi-AT>` **:获取或配置STA身份和SAP身份的MAC地址**
------------------------------------------------------------------

**查询命令** ::    
    
    无

**执行命令** ::

    AT+WLMAC 
    AT+WLMAC=mac_addr(可选)

**参数** ::

    无参数：获取当前设备的STA/SAP的MAC地址
    
    mac_addr: 配置STA身份的MAC地址(可选)

**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR

**属性** :
    +-------------+-----------+
    |  是否超时   |  超时时间 |
    +=============+===========+
    |     是      |     15s   |
    +-------------+-----------+

.. _wifi-stastart:

:ref:`AT+STASTART<wifi-AT>`  **:以STA身份连接至指定SSID的路由器上**
--------------------------------------------------------------------

**查询命令** ::    
    
    无

**执行命令** ::

    AT+STASTART=ssid, pwd(可选)

**参数** ::

    SSID：要以STA身份连接的AP路由器的SSID

    pwd：该AP的密码，如果该AP为OPEN则忽略此选项

**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR

**属性** :
    +-------------+-----------+
    |  是否超时   |  超时时间 |
    +=============+===========+
    |     是      |     20s   |
    +-------------+-----------+

**AT消息** ::

    EVT:WLAN STA CONNECTED

    CONNECTED SSID: xxxx(该AP的SSID)

    EVT:GOT-IP

    sta got ip:192.168.xx.xx所获得的IP地址


.. _wifi-stastop:

:ref:`AT+STASTOP<wifi-at>` **:以STA身份从目前正在连接的路由器上断开**
-----------------------------------------------------------------------

**查询命令** ::    
    
    无

**执行命令** ::

    AT+STASTOP

**参数** ::

    无

**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR

**属性** :
    +-------------+-----------+
    |  是否超时   |  超时时间 |
    +=============+===========+
    |     是      |     15s   |
    +-------------+-----------+

**AT消息** ::

    EVT:WLAN STA DISCONNECTED

    EVT:DISCONNECT REASON:xxx(REASON CODE:xx)(断连原因及其所对应的802.11协议断连原因值)

    local generated:yes/no(指示该断连是否由STA自身发起)

.. _wifi-getstatus:

:ref:`AT+WIFISTATUS<wifi-at>` **:用来查看wifi设备当前的模式状态或指定参数信息**
----------------------------------------------------------------------------------------------

**查询命令** ::    
    
    无

**执行命令** ::

   AT+WIFISTATUS
   AT+WIFISTATUS=STA/AP
   AT+WIFISTATUS=STA/AP,param

**参数** ::

    无参数：查看wifi设备当前的模式状态
    
    param:RSSI\BSSID\SSID\IP\CHANNEL
    可以返按照指定某个类型进行连接。

**属性** :
    +-------------+-----------+
    |  是否超时   |  超时时间 |
    +=============+===========+
    |     是      |     15s   |
    +-------------+-----------+

**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR

**AT消息** ::

    无参数时:
        EVT:'sta: x, ap: x (1:对应身份已启动；0:对应身份未启动)
        EVT:sta:rssi=xxx,aid=xx,ssid=xxxx,bssid=xx:xx:xx:xx:xx:xx,channel=xx,cipher_type=xxx(STA开启)
        EVT:ap_ip=xxx,ap_gate=xxx,ap_mask=xxx,ap_dns=xxx(SAP开启)

    单一参数：
        STA状态：
        CMDRSP:STA_WIFI_CONNECT
        CMDRSP:STA_WIFI_DISCONNECT

        AP状态：
        CMDRSP:AP_WIFI_START
        CMDRSP:AP_WIFI_CLOSE

    两个参数：
        STA身份：
            RSSI:    CMDRSP:sta_rssi:xxx
            CHANNEL: CMDRSP:sta_chnl:xx
            BSSID：  CMDRSP:sta_bssid:xx:xx:xx:xx:xx:xx
            SSID：   CMDRSP:sta_ssid::xxx
            IP：     CMDRSP:STA_IP=192.168.xx.xx,GATE=192.168.xx.xx,MASK=xxx.xxx.xxx.x,DNS=xxx.xx.xx.

        AP身份：
            SSID:    CMDRSP:ap_ssid:xxx
            CHANNEL: CMDRSP:ap_chnl:xx
            SECURITY:CMDRSP:ap_security:xxx
            
.. _wifi-scan:

:ref:`AT+WIFISCAN<wifi-at>` **:扫描周围AP信息，或者按照指定参数进行扫描**
----------------------------------------------------------------------------------------------

**查询命令** ::    
    
    无

**执行命令** ::

    AT+WIFISCAN
    AT+WIFISCAN=param1
    AT+WIFISCAN=param1<,param2,param3,param4,param5,param6>(param2~param6均为可选参数)
    AT+WIFISCAN=SET_SSID,SSID,TYPE,DURATION,CNT,NUMBER

**参数** ::

    无参数：设备会扫描周围所有的AP信息并打印
    
    只有param1:ssid 可以返按照指定ssid进行扫描。

    其他情况：

        a) param1:SET_SSID,是否指定ssid扫描,指定ssid:1; 不指定ssid:0；

        b) param2:SSID,若参数a为1，则填写目标ssid，否则此值为0；

        c) param3:TYPE,扫描类型，主动扫描:0，被动扫描:1；【若不指定则填写0】

        d) param4:DURATION,单信道扫描时长，单位为ms;【若不指定则填写0】

        e) param5:CNT,指定扫描信道的数量；【若不指定则填写0】

        f) param6:NUMBER,若e中指定扫描信道个数不为0 ，则f填写扫描的信道编号；【可选，若e为0不填写】

**属性** :
    +-------------+-----------+
    |  是否超时   |  超时时间 |
    +=============+===========+
    |     是      |      5s   |
    +-------------+-----------+

**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR

**AT消息** ::

    EVT:WLAN STA SCAN_DONE

**示例** ::

    1.AT+WIFISCAN【使用默认方式全信道扫描全部AP】

    2.AT+WIFISCAN=aclsemi【使用默认方式全信道扫描指定ssid的AP】

    3.AT+WIFISCAN=1,aclsemi,0,0,0【扫描aclsemi路由器，其他默认】

    4.AT+WIFISCAN=0,0,0,0,3,1,6,11【扫描1/6/11信道全部AP，其他默认】
    
    5.AT+WIFISCAN=0,0,1,0,0【被动扫描，其他默认】
    
    6.AT+WIFISCAN=0,0,0,70,0【指定信道扫描时长70ms，其他默认】
    
    7.AT+WIFISCAN=1,aclsemi,1,70,3,1,6,11【扫描指定ssid路由器，被动扫描，信道扫描时长70ms，仅扫描1/6/11信道】
    
.. note::

    当配置的 *duration* 时间在0~120ms(包括120ms)时，当扫描时间超过1.6s时会认定为失败

    **只有在配置了duration时此情况才生效**

.. _wifi-sapstart:

:ref:`AT+SAPSTART<wifi-at>` **:以SAP身份创建SoftAP**
--------------------------------------------------------

**查询命令** ::    
    
    无

**执行命令** ::

    AT+SAPSTART

**参数** ::

    AT+SAPSTART=param1
    AT+SAPSTART=param1,param2(可选)
    AT+SAPSTART=param1,param2,param3

**参数含义** ::

    param1:SSID,想要创建的SoftAP的SSID字符串

    param2:pwd,想要创建的SoftAP的密码，如果想将SAP设为OPEN则忽视此选项
           channel,当您未设置密码或者设置的密码长度小于3个字符时会被认为是信道参数处理

    param3:channel,当您设置ssid和pwd之后，第三个参数为设置的信道值

.. note::

    SSID设置长度请不要超过32个字符，超过32个字符会返回失败！

    密码设置长度请置于8~64个字节之间，小于8个字节则按照OPEN处理，超过64个字节将创建失败！


**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR

**属性** :
    +-------------+-----------+
    |  是否超时   |  超时时间 |
    +=============+===========+
    |     是      |     15s   |
    +-------------+-----------+

**AT消息** ::

    当有STA连接至本SAP时打印
    EVT:WLAN SAP CONNECTED + 连接上的STA的MAC地址


.. _wifi-sapstop:

:ref:`AT+SAPSTOP<wifi-at>` **:停止当前开启的SoftAP**
--------------------------------------------------------------

**查询命令** ::    
    
    无

**执行命令** ::

    AT+SAPSTOP

**参数** ::

    无

**参数含义** ::

    无

**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR

**属性** :
    +-------------+-----------+
    |  是否超时   |  超时时间 |
    +=============+===========+
    |     是      |     15s   |
    +-------------+-----------+


.. _wifi-pingstart:

:ref:`AT+STAPING<wifi-at>` **:进行指定IP地址和指定时长和每包大小的PING包操作**
-------------------------------------------------------------------------------

**查询命令** ::    
    
    无

**执行命令** ::

    AT+STAPING=param1,param2,param3

**参数** ::

    param1:想要PING的目标IP地址
    param2:想要PING的包的个数
    param3:想要PING的包的大小

**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR

**属性** :
    +-------------+-----------+
    |  是否超时   |  超时时间 |
    +=============+===========+
    |     是      |     15s   |
    +-------------+-----------+


.. _wifi-pingstop:

:ref:`AT+STAPINGSTOP<wifi-at>` **:停止当前正在进行的PING包工作**
------------------------------------------------------------------------

**查询命令** ::    
    
    无

**执行命令** ::

    AT+STAPINGSTOP

**参数** ::

    无

**响应** ::

    CMDRSP:OK

    CMDRSP:ERROR

**属性** :
    +-------------+-----------+
    |  是否超时   |  超时时间 |
    +=============+===========+
    |     是      |     15s   |
    +-------------+-----------+
