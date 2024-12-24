Armino Wi-Fi Instruction
=======================================================

Armino Wi-Fi Feature List
-------------------------------------------------------
- Compatible with 802.11 b/g/n/ax 2.4GHz Standard
- Support both HT20 and HT40
- Support 802.11N MCS0-7
- Support STA、SoftAP and Direct Mode
- Support AP+STA Concurrent
- Support WPA、WPA2 and WPA3 Encryption
- Support AMPDU、QoS
- Suuport Low-Power in STA Mode

Armino Wi-Fi Programming Model
-------------------------------------------------------
.. image:: ../../../_static/program.png

Wi-Fi Driver is an isolated component from upper-layer code(such as TCP/IP、Applicant Task..etc). Usually, Customer applicantion program is responsible for Wi-Fi initialization and necessary configuration. Wi-Fi Driver accepts and processes data requested by API, and nofitys the application to process related events.

Armino Wi-Fi Events
-------------------------------------------------------
EVENT_WIFI_SCAN_DONE
+++++++++++++++++++++++++++++++++++++++++++++++++++++++
The scanning completion event is reported when the full channels scan finshed. After receiving this event, the application's event callback function could call bk_wifi_scan_get_result to get scan results.

EVENT_WIFI_STA_CONNECTED
+++++++++++++++++++++++++++++++++++++++++++++++++++++++
The event indicates STA connecting with AP successfully. When received this event, Wi-Fi Driver will start DHCP server to get ip address or use static one according to the application's response.

EVENT_WIFI_STA_DISCONNECTED
+++++++++++++++++++++++++++++++++++++++++++++++++++++++
Wi-Fi_STA_DISCONNECTED event usually indicates the disconection from AP. The Client application needs to close sockets when receiving this event. If this event is not expected, it could start the reconnect process.

Armino Wi-Fi Station Connect Scenario
-------------------------------------------------------
.. image:: ../../../_static/connect_procedure_en.png

Wi-Fi Initialization
+++++++++++++++++++++++++++++++++++++++++++++++++++++++
- App Task calls app_wifi_init interface to initialize bk_event_init、bk_netif_init、bk_wifi_init

Wi-Fi Configuration
+++++++++++++++++++++++++++++++++++++++++++++++++++++++
- After Wi-Fi initialization, it will configure workqueue、Wi-Fi MAC、PHY...etc

Wi-Fi Starting Up
+++++++++++++++++++++++++++++++++++++++++++++++++++++++
- Wi-Fi Driver starts with interface ``bk_wifi_sta_start``
- Customer could do some confgurations such as channel、modes、data rate..etc
- Initialize Lwip stack and wpa_supplicant

Wi-Fi Connecting Phase
+++++++++++++++++++++++++++++++++++++++++++++++++++++++
- Wi-Fi Driver enters corresponding mode according to the latest config, default is STA mode
- Excute scan and connect commands
- Report EVENT_WIFI_SCAN_DONE based on the scanning results
- After connecting successfully, Wi-Fi driver will return EVENT_WIFI_STA_CONNECTED
- The callback function will transmit the connected event to application task

Wi-Fi Obtain IP
+++++++++++++++++++++++++++++++++++++++++++++++++++++++
- Start DHCP client to obtain IP address
- SM_DHCP_DONE_IND will be sent after IP address was obtained
- The application will creat TCP/UDP socket operation with this indication

Wi-Fi Station Connect Example
+++++++++++++++++++++++++++++++++++++++++++++++++++++++
::

    net sta enable
    net sta scan once
    net sta scan result
    net sta config SSID PW              //keep PW NULL in OPEN Mode
    net sta connect

Armino Wi-Fi Station Disconnect Example
-------------------------------------------------------
.. image:: ../../../_static/sta_disconnect_en.png

- Application could call function ``bk_wifi_sta_disconnect`` to deactivate from currently connected router
- When Wi-Fi connection is disconnected due to active disconnection, offline router, or weak RSSI signal, the Wi-Fi event callback function will send ``EVENT_WIFI_STA_DISCONNECTED``
- After receiving the network disconnection event, the application will clear all the network applications, for example, TCP/UDP client socket
- Usually for unexpected Wi-Fi disconnection, the application will initialize a reconnection

Armino Wi-Fi shutdown Scenario
-------------------------------------------------------
It is similar to Station disconnection case, please refer to previous chapter.

Armino Wi-Fi SoftAP Mode
-------------------------------------------------------
The usage of BEKEN SoftAP mode could read ``bk_wifi_ap_start`` from API develop guide, stations list connected with AP comes from ``bk_wifi_ap_get_sta_list`` .

Armino Wi-Fi Failure Reason Code
-------------------------------------------------------
.. image:: ../../../_static/reason_codes.png

Armino Wi-Fi MAC Address Configuration
-------------------------------------------------------
The configuration of Wi-Fi MAC address is in ``bk_system\mac.c``,it contains base_mac、sta_mac、ap_mac.The overall architecture of BEKEN Wi-Fi MAC address is as follows:

- It contains three functional macros:CONFIG_NEW_MAC_POLICY、CONFIG_RANDOM_MAC_ADDR、CONFIG_BK_MAC_ADDR_CHECK,all of them are configured to yes by default
- CONFIG_NEW_MAC_POLICY defines the MAC address are stored in the first 6 bytes of Net_param partition;if this area is empty,use a random MAC address
- When first 6 bytes of Net_param are empty and CONFIG_RANDOM_MAC_ADDR is enabled,the MAC address is randomly generated then will be stored in Net_param partition.Certainly,the MAC address generated randomly will follow BEKEN MAC address rules: C8:47:8C
- CONFIG_BK_MAC_ADDR_CHECK is used to determine whether the MAC address configured by the customer complies with the BEKEN rule C8:47:8C .If not required,set it to N in the defconfig file
- All above three macro definitions are located in ``middleware\soc\bk7236\bk7236.defconfig`` ,with default values of y
- Using the MAC Address Configuration Tool: bk_writer writes the MAC address to flash.If the flash is fully erased,it will cause the MAC address to be lost,resulting in the use of random MAC address
- use command ``mac`` for temporary test,the example is as follows:

::

    mac c8478caabbcc


Armino Wi-Fi Fragmentation
=======================================================
Wi-Fi receive fragmentation is supported, but Wi-Fi transmit fragmentation is not supported.



Armino Wi-Fi Buffer Usage
=======================================================
**Introduction to Wi-Fi buffer memory and LwIP buffer memory**
-------------------------------------------------------------------
- Wi-Fi memory block usage: mainly hardware receive buffer, software transmit descriptor. The hardware receive buffer is designed for the hardware side of the chip, with a default configuration size that cannot be changed by the software side. The memory used for software transmit descriptor and SKB memory block are requested from SRAM in "dynamic" way.
- LWIP Memory Block Management: LwIP memory is allocated by the software side, and a certain size of MEM is "dynamically" taken out from the SRAM HEAP for LwIP use by means of macro definition.


Importance of LwIP/Wi-Fi Buffer Memory Configuration
-------------------------------------------------------
In order to achieve a robust, high-performance system, we need to consider memory usage or configuration very carefully because:

- Given the limited overall memory on Armino, the Wi-Fi and LWIP sides cannot request unlimited memory for normal system operation.
- In order to get better and better performance from Wi-Fi, the default TRX memory usage in the LwIP driver is dynamically configured proportionally to achieve the highest Mem utilization to alleviate the problem of tight Mem usage during TX or RX.
- Wi-Fi throughput depends heavily on memory-related configurations such as TCP window size, number of LwIP Wi-Fi receive/transmit data buffers, etc. The Wi-Fi throughput is also dependent on the number of memory buffers available in the LwIP.
- The peak total memory that may be used in Armino LwIP depends on factors such as scenario requirements, e.g., extreme throughput acquisition, maximum TCP/UDP connections that the application may have, relaying, and other scenarios.
- The total memory required by the application is also an important factor when considering memory configuration.

For these reasons, there is no one parameter configuration that fits all applications. Instead, we must take into account that different memory configurations are considered for each different application.

Armino Data Flow and Memory Usage Patterns
-------------------------------------------------------

.. figure:: ../../../_static/Armino_wifi_data_flow.png
    :align: center
    :alt: Armino_wifi_data_flow
    :figclass: align-center

    Armino wifi data flow

The Armino data flow path is roughly divided into three layers, namely, the application layer, the WIFI layer and the hardware layer, of which the WIFI layer includes the LWIP layer and the MAC layer as well as the related interface layer.

- RX Direction: During the data receiving process, the hardware transfers the received packets to the receive data buffer of Wi-Fi, the receive data buffer of LWIP for relevant protocol processing, and finally uploads them to the application layer. the receive data buffer of Wi-Fi and the receive data buffer of LWIP share the same buffer by default. the receive data buffer of Wi-Fi and the receive data buffer of LWIP share the same buffer by default. the receive data buffer of Wi-Fi and the receive data buffer of LWIP share the same buffer by default. Both Wi-Fi receive data buffer and LWIP receive data buffer size are requested from SRAM HEAP. the LWIP receive data buffer size (LWIP_MEM_SIZE) can be configured according to the requirements of different scenarios.
- TX Direction: During the data sending process, the application first copies the message to be sent to the MEM (dynamically requested memory) of LWIP through the LWIP data interface for TCP/IP encapsulation, and then sends the message to the send data buffer of Wi-Fi layer for MAC encapsulation processing, and finally waits for sending. In order to save SRAM HEAP space, Armino uses PSRAM space for the data transfer copy in the upper layer application, while relieving the pressure on SRAM memory without reducing the available memory for the application.


How to Improve Armino Wi-Fi Performance
------------------------------------------------
The performance of Armino Wi-Fi is affected by a number of parameters that are related and constrained by each configuration parameter. If the configuration parameters are reasonable and optimal, it will not only improve the performance, but also increase the available memory for the application and improve the stability.

In this section, we will briefly introduce the operating modes of the Wi-Fi/LWIP stack and explain the role of each parameter. We will recommend the corresponding parameter configurations so that you can choose the most appropriate configuration according to your usage scenario.The Wi-Fi performance can be improved by adjusting the size or number of MEMs in each of the above layers. Below we will introduce the parameters you need to configure.

**Receive data direction (RX) related parameters**
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++

CONFIG_LWIP_MEM_MAX_RX_SIZE
************************************

This parameter indicates the size of the receive data buffer configured by the lwip layer. Increasing this parameter can enhance packet reception performance. This parameter needs to match the mac layer processing receive data buffer size. The maximum size of this parameter cannot exceed the size of LWIP_MEM_SIZE, and the default configuration is (LWIP_MEM_SIZE*3)/4.

CONFIG_LWIP_UDP_RECVMBOX_SIZE
************************************

This parameter indicates that for UDP RX, the LWIP kernel will first send the packet to the UDP receive mailbox, and then the application will get the packet from the mailbox. This means that LWIP can cache the maximum UDP_RECCVMBOX_SIZE packets for each UDP socket, so the maximum number of UDP packets that may be cached for all UDP sockets is a multiple of UDP_RECCVMBOX_SIZE and the maximum number of UDP sockets. In other words, a larger UDP_RECVMBOX_SIZE means more memory.
On the other hand, if the incoming mailbox is too small, the mailbox may be full. Therefore, in general, we need to make sure that the UDP receive mailbox is large enough to avoid packet loss between the LWIP core and the application.

CONFIG_LWIP_TCP_WND
************************************

This parameter indicates the size of the receive data buffer used by the LWIP layer for each TCP flow, the default configuration is 29200(B) (the value of 20 * default MSS), setting a smaller default receive window size can save some memory, but will greatly reduce performance. Also, in the case of multiple streams, the value of this parameter should be reduced accordingly.

**Transmit Data Direction (TX) related parameters**
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++

CONFIG_LWIP_MEM_MAX_TX_SIZE
************************************

This parameter indicates the size of the receive data buffer configured by the lwip layer, the maximum size of this parameter should not exceed the size of LWIP_MEM_SIZE, the default configuration is (LWIP_MEM_SIZE*5)/6. If the program should send a large amount of data that needs to be copied, increasing this parameter can enhance the performance of packet sending.

CONFIG_LWIP_TCP_SND_BUF
************************************

This parameter indicates the size of the send data buffer used by the LWIP layer for each TCP stream. This value must be at least 2 times the size of the MSS; the default value on Armino is 20 times the size of the MSS. Setting a smaller default SND BUF size saves some RAM, but reduces performance.

**Receive Data and Transmit Data Direction (TRX) highly relevant parameters**
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

CONFIG_LWIP_MEM_SIZE
************************************

This parameter indicates the LWIP layer heap memory size, which is requested from the system heap memory. The default configuration size for this parameter is 51200 (50K). If a large amount of data needs to be sent or received, increasing this parameter can improve the overall throughput performance.

CONFIG_LWIP_TCP_MSS
************************************

This parameter represents the TCP maximum message length, which is an option defined by the TCP protocol. The MSS (Maximum Segment Size) option is used to negotiate the maximum length of data that can be carried by each message segment when a TCP connection is established and both the sender and receiver are communicating.
This parameter can be set lower to save RAM and defaults to 1460 (ipv4)/1440 (ipv6) for optimal throughput.
IPv4 TCP_MSS range: 576 <= TCP_mSS <= 1460
IPv6 TCP_MSS range: 1220 <= TCP_mSS <= 1440

CONFIG_LWIP_MEMP_NUM_NETBUF
************************************

This parameter indicates the number of NetBuf structures. Setting this value too small when programming with netconn and socket may result in a failure to allocate memory when receiving data, thus not serving data sending and receiving for several connections at the same time. The minimum configuration on Armino is 16 and the maximum is 32. Here it is recommended to configure the maximum of 32 by default.

CONFIG_LWIP_PBUF_POOL_SIZE
************************************

This parameter indicates the number of buffers in the pbuf pool, the parameter is configurable. Maximum configuration is 20, on Armino it is recommended that the default configuration size is 10.

CONFIG_LWIP_TCP_SND_QUEUELEN
************************************

This parameter indicates the TCP send buffer space, this parameter limits the number of pbufs in the send buffer. This parameter is set to 40 by default and must be at least equal to (2 * TCP_SND_BUF/TCP_MSS) to work properly.

CONFIG_LWIP_MEMP_NUM_TCP_SEG
************************************

This parameter indicates the number of segments of TCP that can be in the queue at the same time, the default parameter sets the size to 80 and must be equal to at least (2 * LWIP_TCP_SND_QUEUELEN) to work properly.

