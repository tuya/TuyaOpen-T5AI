**Wi-Fi Problem Locating Guide**
=====================================

:link_to_translation:`zh_CN:[中文]`

**Overview**
---------------
This chapter mainly introduces the methods of locating Wi-Fi common problem and capturing logs.

**Analysis of Transceiver Path Problems**
-------------------------------------------
Common problems in the transceiver path include: low data transmission bandwidth, data transmission packet loss, and high data transmission delay.
This type of problem generally requires checking some statistical information through CLI commands, and even requires analyzing the transceiver path log and Wi-Fi air packets.
Armino SDK supports LWIP and Wi-Fi bottom layer output sending and receiving related logs by default. These logs are not enabled by default and can be enabled through the API interface when needed to view them.
For details, please refer to cli_pkt_debug_cmd function implementation.

.. important::
    *LWIP and Wi-Fi logs will affect system performance, please make sure to enable them only when locating problems*

**LWIP Related Log Introduction**
************************************

The LWIP module currently supports ICMP, IP and TCP sub-modules to output sending and receiving key logs. If you need to view the relevant logs, you need to follow the steps below.

1. First, you need to enable LWIP debug log code compilation through the CONFIG_LWIP_DEBUG_LOG function macro in the corresponding project, for example, in the /project/app/bk****/config file
CONFIG_LWIP_DEBUG_LOG=y, you can enable LWIP debug log code compilation in the bk*** chip app project.

2. You can specifically enable the log of the LWIP submodule through the void lwip_set_pkt_trx_dbg_cfg (uint32_t cfg_bit) interface.

The LWIP submodule log configuration is completed by cfg_bit. The meaning of cfg_bit is as shown in the following table:

+-------------------------------+-----------+--------------------------------------+
| **Macro Definition**          | **Value** | **Description**                      |
+===============================+===========+======================================+
| LWIP_TX_DBG_LOG_SOCKET        |   1<<16   | Enable log in lwip_send function     |
+-------------------------------+-----------+--------------------------------------+
| LWIP_TX_DBG_LOG_IP            |   1<<17   | Eanble IP TX log                     |
+-------------------------------+-----------+--------------------------------------+
| LWIP_TX_DBG_LOG_TCP           |   1<<18   | Eanble TCP TX log                    |
+-------------------------------+-----------+--------------------------------------+
| LWIP_RX_DBG_LOG_SOCKET        |   1<<20   | Eanble log in lwip_recvfrom function |
+-------------------------------+-----------+--------------------------------------+
| LWIP_RX_DBG_LOG_IP            |   1<<21   | Eanble IP RX log                     |
+-------------------------------+-----------+--------------------------------------+
| LWIP_RX_DBG_LOG_TCP           |   1<<22   | Eanble TCP RX log                    |
+-------------------------------+-----------+--------------------------------------+
| LWIP_TX_DBG_LOG_PING          |   1<<24   | Eanble ICMP TX log                   |
+-------------------------------+-----------+--------------------------------------+
| LWIP_RX_DBG_LOG_PING          |   1<<25   | Eanble ICMP RX log                   |
+-------------------------------+-----------+--------------------------------------+

**LWIP Related Debug Commands**
*********************************
You can use lwip_mem and lwip_stats CLI commands to assist in locating TCP/IP related issues.

lwip_mem - View memory usage related to lwip, this command returns the following:

::

    $lwip_mem

    (76916):Name             total used addr       size  err
    (76916):----------------------------------------------------
    (76916):RAW_PCB          4     0    0x2804ebbf 28    0
    (76916):UDP_PCB          10    1    0x2804fc07 40    0
    (76918):TCP_PCB          8     0    0x2804f0a1 164   0
    (76918):TCP_PCB_LISTEN   4     0    0x2804f01e 32    0
    (76918):TCP_SEG          80    0    0x2804f5c4 20    0
    (76918):REASSDATA        5     0    0x2804ec32 32    0
    (76918):FRAG_PBUF        15    0    0x28049c2a 24    0
    (76918):NETBUF           32    0    0x28049e18 16    0
    (76918):NETCONN          20    0    0x2804a01b 52    0
    (76918):TCPIP_MSG_API    8     0    0x2804ed98 16    0
    (76918):TCPIP_MSG_INPKT  32    0    0x2804ee1b 16    0
    (76918):ARP_QUEUE        4     0    0x28049c07 8     0
    (76918):IGMP_GROUP       8     1    0x28049d95 16    0
    (76920):SYS_TIMEOUT      12    6    0x2804ecd5 16    0
    (76920):NETDB            4     0    0x2804a42e 308   0
    (76920):PBUF_REF/ROM     10    0    0x2804eb1c 16    0
    (76920):PBUF_POOL        10    0    0x2804a901 1692  0
    (76920):===== MEMP_PBUF_POOL ======
    (76920):avail 10, used 0, max 0, err 0
    (76920):========== MEM ============
    (76920):avail 51200, used 60, max 1124, err 0
    (76920):tx avail 42666, rx avail 38400, tx used 60, rx used 0, tx max 544, rx max 1064, tx err 0, rx err 0

lwip_stats - View lwip sending and receiving statistics. This command returns the following:

::

    $lwip_stats
    (444938):
    ETHARP
    (444938):xmit:     2
    (444938):recv:     5
    (444938):fw:       0
    (444938):drop:     0
    
    $(444940):chkerr:   0
    (444940):lenerr:   0
    (444940):memerr:   0
    (444940):rterr:    0
    (444940):proterr:  0
    (444942):opterr:   0
    (444942):err:      0
    (444942):cachehit: 5
    (444942):
    IP
    (444942):xmit:     10
    (444942):recv:     16
    (444942):fw:       0
    (444942):drop:     5
    (444942):chkerr:   0
    (444942):lenerr:   0
    (444942):memerr:   0
    (444942):rterr:    0
    (444942):proterr:  0
    (444942):opterr:   0
    (444942):err:      0
    (444942):cachehit: 0
    (444942):
    UDP
    (444942):xmit:     4
    (444942):recv:     5
    (444944):fw:       0
    (444944):drop:     0
    (444944):chkerr:   0
    (444944):lenerr:   0
    (444944):memerr:   0
    (444944):rterr:    0
    (444944):proterr:  0
    (444944):opterr:   0
    (444944):err:      0
    (444944):cachehit: 4
    (444944):
    TCP
    (444944):xmit:     0
    (444944):recv:     0
    (444944):fw:       0
    (444944):drop:     0
    (444944):chkerr:   0
    (444944):lenerr:   0
    (444944):memerr:   0
    (444944):rterr:    0
    (444944):proterr:  0
    (444944):opterr:   0
    (444944):err:      0
    (444944):cachehit: 0

.. Note:: To view the lwip CLI command, you need to ensure that the cli_lwip.c module is involved in compilation

**Wi-Fi Related log Introduction**
************************************

The Wi-Fi module currently supports the TX/RX channel to output key logs. The log can be enabled through void bk_wifi_set_pkt_trx_dbg_cfg(uint32_t cfg_bit)
API interface implementation, the meaning of cfg_bit is as shown in the following table:

+-------------------------------+-----------+-----------------------------------+
| **Macro Definition**          | **Value** | **Description**                   |
+===============================+===========+===================================+
| WIFI_TX_DBG_LOG_DATA_PKT      |   1<<0    | Enable Wi-Fi Data Frame TX log    |
+-------------------------------+-----------+-----------------------------------+
| WIFI_TX_DBG_LOG_MGMT_PKT      |   1<<1    | Enable Wi-Fi MGMT Frame TX log    |
+-------------------------------+-----------+-----------------------------------+
| WIFI_TX_DBG_LOG_PUSH_PKT      |   1<<2    | Enable Wi-Fi TX Push log          |
+-------------------------------+-----------+-----------------------------------+
| WIFI_RX_DBG_LOG_DATA_PKT      |   1<<8    | Enable Wi-Fi Data Frame RX log    |
+-------------------------------+-----------+-----------------------------------+
| WIFI_RX_DBG_LOG_MGMT_PKT      |   1<<9    | Enable Wi-Fi Mgmt Frame RX log    |
+-------------------------------+-----------+-----------------------------------+

.. important::
    *After the log is enabled, as the amount of transmitted data increases, the amount of log will increase dramatically. In order to capture the complete log, the serial port baudrate needs to be increased*

**Wi-Fi Path Debug Command**
*********************************

The Wi-Fi module also provides sending and receiving statistical information. This information can be queried through the get mac_trx status command. The command returns the following:

::

    $get mac_trx status
    
    $wifid:I(1077306):======================= MAC RX INFO ========================
    wifid:I(1077308):rx data cnt:31, upload cnt:36, forward cnt:36, memerr cnt:0
    wifid:I(1077308):======================= MAC TX INFO ========================
    wifid:I(1077308):tx data dld cnt:12, retry cnt:0, single cnt:12, agg cnt:5, discard cnt:0, success cnt:12
    wifid:I(1077308):tx data tx time:269, avg time:22, max time:152
    CMDRSP:OK

.. Note:: To view Wi-Fi CLI commands, you need to ensure that the cli_wifi.c module is involved in compilation

**Analysis Method of Common Issues about Wi-Fi Transmission**
***********************************************************************
- Low transmission bandwidth: If the TCP transmission bandwidth is low, you need to pay attention to whether the upper-layer application frequently sends data and encounters a situation where the TCP buffer is full. If similar problems exist, you need to open the logs of LWIP and Wi-Fi and cooperate with the Wi-Fi air interface for analysis.
- Transmission packet loss: First, confirm whether the transmission and reception is normal through lwip log. If there is no abnormality, you need to open lwip log and Wi-Fi log and analyze the Wi-Fi air interface packet.
- High transmission delay: This type of problem is generally related to the environment. You can first use the ping program to confirm whether the delay with the target communication address is as expected. If the ping delay is normal, you need to enable lwip log and Wi-Fi log and analyze the Wi-Fi air interface packets.