Armino Wi-Fi BK-RLK User Guide
=======================================================

:link_to_translation:`zh_CN:[中文]`

BK-RLK Overview
-------------------------------------------------------

BK-RLK is kind of wireless Wi-Fi communication protocol that is defined by Beken.
In BK-RLK communication, The application layer communicates between devices directly through the data link layer.
BK-RLK has the advantages of ultra-low power consumption,ultra-low delay,high bandwidth,more flexible in development and carry a lighter contrl system, which can be widely used in various intelligent electronic products supporting Wi-Fi connnunication.

BK-RLK Information Description
-------------------------------------------------------

BK-RLK simplifies the communication process between devices, and its communication identification core moves directly from TCP/IP layer to data link layer. To facilitate the usage of the application layer, BK-RLK maintains a list of peer devices information on the MAC layer. The specific maintenance of device information is shown in the table below:

+------------------+-------------------------------------------------------------------------+
| param            | Description                                                             |
+==================+=========================================================================+
| MAC Address      | match devices MAC address, length in 6 bytes                            |
+------------------+-------------------------------------------------------------------------+
| Channel          | match device communication channel,range 1 to 14                        |
+------------------+-------------------------------------------------------------------------+
| Encrypt          | whether to encrypt communication with matching devices                  |
+------------------+-------------------------------------------------------------------------+
| State            | the application layer is used to add connection status information of   |
|                  | matching devices,etc.                                                   |
+------------------+-------------------------------------------------------------------------+

BK-RLK Instructions
-------------------------------------------------------
BK-RLK basic communication process are as follows：

 - BK-RLK initialization
 - BK-RLK add peer devices
 - BK-RLK data transmission and reception
 - BK-RLK low power consumption configuration(optional)
 - BK-RLK other communication parameter configuration(optional)

BK-RLK initialization：

User of BK-RLK by application layer needs to be initialized：
 - call bk_rlk_init() to initialize BK-RLK to register BK-RLK；
 - call bk_rlk_register_send_cb() configure send callback function interface，which is used to judge whether the date is send successfully on the MAC layer；
 - call bk_rlk_register_recv_cb() configure receive callback funtion interface，which is used to receive data from the bottom to application layer；
 - call bk_rlk_set_channel() configure BK-RLK communication channel；

BK-RLK add peer devices：

BK-RLK needs to call bk_rlk_add_peer() add peer devices information before transmission data. Configuratin peer devices MAC address as ff:ff:ff:ff:ff:ff if send broadcast frame or multicast frame. Others set peer devices MAC address.

There is no limit to the number of paried devices that can be added to BK-RLK. The more number,the more memory it takes up.
It is suggested that add peer information reasonably according to the actual situation. Call bk_rlk_del_peer() interface to delete un-used peer information in time. The channel of local devices and paired devices must be consistent befor communication. It is recommended to check whether the channel is consistent before sending data.

Note: Communication cannot be performed without adding the peer devices information to bk_rlk_add_peer().

BK-RLK data transmission and reception：

BK-RLK has two transmission interface，bk_rlk_send() and bk_rlk_send_ex().
It is suitable for any packet sending requirement to bk_rlk_send(). The send callback function registered by bk_rlk_register_send_cb() is used to feedback whether the packet send by bk_rlk_send() is successfully.
It is special send interface provided for the application layer to bk_rlk_send_ex()，which is configure the required parameters directly and register an independent sending callback function. The configuration parameters are shown as follows:

+------------------+-------------------------------------------------------------------------+
| param            | Description                                                             |
+==================+=========================================================================+
| data             | transmission data                                                       |
+------------------+-------------------------------------------------------------------------+
| len              | data length                                                             |
+------------------+-------------------------------------------------------------------------+
|                  | send callback function, feedback if data send successfully at bottom    |
|                  | layer, cb has two arguments:                                            |
| cb               | 1) args, user-defined parameters of application layer;                  |
|                  | 2) status, feedback status flag whether the bottom layer data is sent   |
|                  | successfully;                                                           |
+------------------+-------------------------------------------------------------------------+
| args             | cb arguments, which is used to identify the sent data, and user-defined |
|                  | by the application layer;                                               |
+------------------+-------------------------------------------------------------------------+
| tx_rate          | data transmission rate                                                  |
+------------------+-------------------------------------------------------------------------+
| tx_power         | data transmission power                                                 |
+------------------+-------------------------------------------------------------------------+
| tx_retry_cnt     | data transmission retry count                                           |
+------------------+-------------------------------------------------------------------------+

BK-RLK provided data receive callback function interface bk_rlk_register_recv_cb(). The application layer needs to register receive callback function before using BK-RLK to receive data. Parameters feedback by BK-RLK receiving interface are shown in the following table :

+------------------+-------------------------------------------------------------------------+
| param            | Description                                                             |
+==================+=========================================================================+
| data             | transmission data                                                       |
+------------------+-------------------------------------------------------------------------+
| len              | data length                                                             |
+------------------+-------------------------------------------------------------------------+
| src_address      | source address of received BK-RLK packet，length in 6 bytes             |
+------------------+-------------------------------------------------------------------------+
| des_address      | destination address of received BK-RLK packet，length in 6 bytes        |
+------------------+-------------------------------------------------------------------------+
| rssi             | RX RSSI of BK-RLK packet                                                |
+------------------+-------------------------------------------------------------------------+

BK-RLK low power consumption configuration：
call bk_rlk_sleep() open BK-RLK power save mode，and call bk_rlk_wakeup() close BK-RLK power save mode。

BK-RLK other communication parameter configuration：
In order to facilitate the development and use of application layer, BK-RLK provides a wider interface for data transmission parameter configuration:

 - Call bk_rlk_set_tx_ac() to configure data transmission priority;
 - Call bk_rlk_set_tx_timeout_ms() to configure data transmission timeout;
 - Call bk_rlk_set_tx_power() to configure data transmission power;
 - Call bk_rlk_set_tx_rate() to configure data transmission rate;

By default，BK-RLK has given a reasonable parameter configuration. If it is not satisfied with the development requirements, the application layer can adjust the parameters through the above interfaces.

BK-RLK Reference Materials
-------------------------------------------------------
  BK-RLK specific API interface,reference demo and operation can refer to the following path：

    `API reference: <../../api-reference/wifi/bk_wifi.html>`_ introduce BK-RLK API interface

    `BK-RLK projects: <../../projects_work/wifi/bk_rlk_media.html>`_ introduce BK-RLK projects