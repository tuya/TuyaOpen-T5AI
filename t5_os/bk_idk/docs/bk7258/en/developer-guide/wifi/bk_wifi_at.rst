.. _wifi-AT:

**Wi-Fi AT Commands**
=======================

:link_to_translation:`zh_CN:[zh_CN]`


.. important::

    *By default, the BK-IDK supports all the AT commands described in this section*

    *Unless specified,otherwise the timeout boundary for Wi-Fi AT is limited to 15s*


- :ref:`AT+WLMAC<wifi-mac>` :Get the MAC address of STA identity and SAP identity, you can also configure the MAC address

- :ref:`AT+STASTART<wifi-stastart>` :Connect to the router with the specified SSID as STA

- :ref:`AT+STASTOP<wifi-stastop>` :Disconnect from the router that is currently connected as STA 

- :ref:`AT+WIFISTATUS<wifi-getstatus>` :It is used to view the current mode status of the wifi device or specify parameter information

- :ref:`AT+WIFISCAN<wifi-scan>` :Scan for neighbor AP information,or scan according to specified parameters

- :ref:`AT+SAPSTART<wifi-sapstart>` :Create SoftAP as SAP

- :ref:`AT+SAPSTOP<wifi-stastop>` :Stop current SoftAP

- :ref:`AT+STAPING<wifi-pingstart>` :PING packets with a specified IP address, a specified duration, and a specified packet size

- :ref:`AT+STAPINGSTOP<wifi-pingstop>` :Stop the PING packet that is being pinged

.. _wifi-mac:

:ref:`AT+WLMAC<wifi-AT>` **:Get the MAC address of STA identity and SAP identity,also configure the MAC address**
--------------------------------------------------------------------------------------------------------------------------

**Query command** ::    
    
    None

**Setup command** ::

    AT+WLMAC 
    AT+WLMAC=mac_addr(Optional)

**Params** ::

    No param：Gets the STA/SAP MAC address of the current device
    
    mac_addr(Optional):configure the MAC address of this device

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

**Attributes** :
    +---------------+----------------+
    |  Timeout      |  Time-out time |
    +===============+================+
    |     Yes       |     15s        |     
    +---------------+----------------+

.. _wifi-stastart:

:ref:`AT+STASTART<wifi-AT>`  **:Connect to the router with the specified SSID as STA**
----------------------------------------------------------------------------------------

**Query command** ::    
    
    None

**Setup command** ::

     AT+STASTART=ssid, pwd(Optional)

**Params** ::

    SSID：SSID of the AP router to be connected as STA
    
    pwd:  The AP password is ignored if the AP is OPEN

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

**Attributes** :
    +---------------+----------------+
    |  Timeout      |  Time-out time |
    +===============+================+
    |     Yes       |     20s        |     
    +---------------+----------------+

**AT Event** ::

    EVT:WLAN STA CONNECTED

    CONNECTED SSID: xxxx(AP SSID)

    EVT:GOT-IP

    sta got ip:192.168.xx.xx


.. _wifi-stastop:

:ref:`AT+STASTOP<wifi-at>` **:Disconnect from the router that is currently connected as STA**
-----------------------------------------------------------------------------------------------

**Query command** ::    
    
    None

**Setup command** ::

    AT+STASTOP

**Params** ::

    None

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

**Attributes** :
    +---------------+----------------+
    |  Timeout      |  Time-out time |
    +===============+================+
    |     Yes       |     15s        |     
    +---------------+----------------+

**AT Event** ::

    EVT:WLAN STA DISCONNECTED

    EVT:DISCONNECT REASON:xxx(REASON CODE:xx)(Reasons in 802.11 protocol)

    local generated:yes/no(Indicate whether the disconnection was initiated by STA itself)

.. _wifi-getstatus:

:ref:`AT+WIFISTATUS<wifi-at>` **:view the current mode status of the wifi device or specify parameter information**
----------------------------------------------------------------------------------------------------------------------

**Query command** ::    
    
    None

**Setup command** ::

   AT+WIFISTATUS
   AT+WIFISTATUS=STA/AP
   AT+WIFISTATUS=STA/AP,param

**Params** ::

    No param：View the current mode status of the wifi device
    
    RSSI\BSSID\SSID\IP\CHANNEL:Query current status by a specified type.

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

**Attributes** :
    +---------------+----------------+
    |  Timeout      |  Time-out time |
    +===============+================+
    |     Yes       |     15s        |     
    +---------------+----------------+

**AT Event** ::

    No Param:
            EVT:'sta: x, ap: x (1:The corresponding identity has been enabled；0:The corresponding identity has not been enabled)
            EVT: sta:rssi=xxx,aid=xx,ssid=xxxx,bssid=xx:xx:xx:xx:xx:xx,channel=xx,cipher_type=xxx(STA is started)
            EVT: ap_ip=xxx,ap_gate=xxx,ap_mask=xxx,ap_dns=xxx(SAP is started)


        One Param：
            STA Status：
            CMDRSP:STA_WIFI_CONNECT
            CMDRSP:STA_WIFI_DISCONNECT

            AP Status：
            CMDRSP:AP_WIFI_START
            CMDRSP:AP_WIFI_CLOSE

         Two Params：
            STA：
                RSSI:    CMDRSP:sta_rssi:xxx
                CHANNEL: CMDRSP:sta_chnl:xx
                BSSID：  CMDRSP:sta_bssid:xx:xx:xx:xx:xx:xx
                SSID：   CMDRSP:sta_ssid::xxx
                IP：     CMDRSP:STA_IP=192.168.xx.xx,GATE=192.168.xx.xx,MASK=xxx.xxx.xxx.x,DNS=xxx.xx.xx.

            AP：
                SSID:    CMDRSP:ap_ssid:xxx
                CHANNEL: CMDRSP:ap_chnl:xx
                SECURITY:CMDRSP:ap_security:xxx


.. _wifi-scan:

:ref:`AT+WIFISCAN<wifi-at>` **:Scan for neighbor AP information,or scan according to specified parameters**
--------------------------------------------------------------------------------------------------------------

**Query command** ::    
    
    None

**Setup command** ::

    AT+WIFISCAN
    AT+WIFISCAN=param1
    AT+WIFISCAN=param1<,param2,param3,param4,param5,param6>(param2~param6 are all optional parameters)
    AT+WIFISCAN=SET_SSID,SSID,TYPE,DURATION,CNT,NUMBER

**Params** ::

    No Param：The device will scan and dump all surrounding AP information
    
    Only param1:ssid,SCAN will be conducted based on the specified SSID 

    Other situations：

        a) param1:SET_SSID,whether the SCAN is specified by SSID,specified:1; non-specified:0

        b) param2:SSID,if param1 is 1,fill in the target SSID, otherwise,fill in 0

        c) param3:TYPE,SCAN Type，Active: **0**,PASSIVE: **1** [If not specified,fill in 0]

        d) param4:DURATION,duration of single-channel SCAN in ms [If not specified,fill in 0]

        e) param5:CNT,the number of the specified channels [If not specified,fill in 0]

        f) param6:NUMBER,if the specified number of scanning channels in CNT is not 0,fill in the scan channel number [Optional,if CNT is zero,ignored]

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

**Attributes** :
    +---------------+----------------+
    |  Timeout      |  Time-out time |
    +===============+================+
    |     Yes       |      5s        |     
    +---------------+----------------+

**AT Event** ::

    EVT:WLAN STA SCAN_DONE

**Examples** ::

    1.AT+WIFISCAN [The device will scan and dump all surrounding AP information using the default settings]

    2.AT+WIFISCAN=aclsemi [Perform a full-channel scan of AP named aclsemi with the specified SSID using the default settings]

    3.AT+WIFISCAN=1,aclsemi,0,0,0 [Scan router named aclsemimreset is using the default settings]

    4.AT+WIFISCAN=0,0,0,0,3,1,6,11 [Scan all APs on CHANNLE 1/6/11,reset is using the default settings]
    
    5.AT+WIFISCAN=0,0,1,0,0 [Passive SCAN,reset is using the default settings]
    
    6.AT+WIFISCAN=0,0,0,70,0 [Set the specified channel scan duration to 70ms,reset is using the default settings]
    
    7.AT+WIFISCAN=1,aclsemi,1,70,3,1,6,11 [Scan router named aclsemi,PASSIVE SCAN,the specified channel scan duration is 70ms,Scan on CHANNLE 1/6/11]

.. note::

    When the configured *duration* time is between 0 and 120ms(120ms is included),it will be considered a failure if the scanning time exceeds 1.6 seconds

    **This condition only takes effect when the duration is configured.**


.. _wifi-sapstart:

:ref:`AT+SAPSTART<wifi-at>` **:Create SoftAP as SAP**
--------------------------------------------------------

**Query command** ::    
    
    None

**Setup command** ::

    AT+SAPSTART=param1
    AT+SAPSTART=param1,param2(Optional)
    AT+SAPSTART=param1,param2,param3

**Params** ::

    param1：The SSID string you want to create SoftAP
    
    param2 :pwd,     The password for the SoftAP you want to create, ignore this option if you want to set SAP to OPEN
            Channel, when you do not set a password or set a password length of less than 3 characters,
                     will be considered as a channel parameter processing.

    param3:channel,  when you set the SSID and password, the third parameter is the set channel value.

.. note::

    SSID length should not exceed 32 characters, exceeding 32 characters will result in a failure response!

    The password should be set between 8 and 64 bytes in length. If it is less than 8 bytes, it will be treated as OPEN. If it exceeds 64 bytes, the creation will fail!


**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

**Attributes** :
    +---------------+----------------+
    |  Timeout      |  Time-out time |
    +===============+================+
    |     Yes       |     15s        |     
    +---------------+----------------+

**AT Event** ::

    Print when a STA is connected to this SAP
    EVT:WLAN SAP CONNECTED + MAC Address which belongs to STA


.. _wifi-sapstop:

:ref:`AT+SAPSTOP<wifi-at>` **:Stop current SoftAP**
--------------------------------------------------------------

**Query command** ::    
    
    None

**Setup command** ::

    AT+SAPSTOP

**Params** ::

    None

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

**Attributes** :
    +---------------+----------------+
    |  Timeout      |  Time-out time |
    +===============+================+
    |     Yes       |     15s        |     
    +---------------+----------------+


.. _wifi-pingstart:

:ref:`AT+STAPING<wifi-at>` **:PING packets with a specified IP address, a specified duration, and a specified packet size**
------------------------------------------------------------------------------------------------------------------------------

**Query command** ::    
    
    None

**Setup command** ::

    AT+STAPING=param1,param2,param3

**Params** ::

    param1:Target IP Address
    param2:Packet numbers
    param3:size of each packet

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

**Attributes** :
    +---------------+----------------+
    |  Timeout      |  Time-out time |
    +===============+================+
    |     Yes       |     15s        |     
    +---------------+----------------+

.. _wifi-pingstop:

:ref:`AT+STAPINGSTOP<wifi-at>` **:Stop the PING packet that is being pinged**
-----------------------------------------------------------------------------

**Query command** ::    
    
    None

**Setup command** ::

    AT+STAPINGSTOP

**Params** ::

    None

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

**Attributes** :
    +---------------+----------------+
    |  Timeout      |  Time-out time |
    +===============+================+
    |     Yes       |     15s        |     
    +---------------+----------------+
