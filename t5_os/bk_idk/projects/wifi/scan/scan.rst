Wi-Fi Scan
=======================================

Project Overview
--------------------------

This project is used for demonstrating [Wi-Fi Scan](https://docs.bekencorp.com/armino/bk7236/en/latest/developer-guide/wifi/bk_wifi_scan.html)

  - This project shows Wi-Fi scan procedure, it scans the APs around and return the results.
  - This project not only supports global scanning, but also can scan by specific SSID.

Hardware Requirements
----------------------------

All Beken development board.

Configure and Build
--------------------------------

Configure the parameter
+++++++++++++++++++++++++++++++

You can configure the scan parameter in projects/examples/wifi/scan/main/Kconfig.projbuild

::

 menu "Example Configuration"

    config EXAMPLE_SCAN_ALL_SSID
        bool "Scan All SSID"
        default y

    config EXAMPLE_WIFI_SSID
        string "WiFi SSID"
	depends on !EXAMPLE_SCAN_ALL_SSID
        default "myssid"
        help
            Set the SSID to be scaned
 endmenu

Build
+++++++++++++++++++++++++++++++

Enter the following command in the Armino root directory to compile:

> make bk7236 PROJECT=examples/wifi/scan

For preparations such as toolchain installation, please refer to [Quick Start](https://docs.bekencorp.com/armino/bk7236/en/latest/get-started/index.html).

Connect UART
++++++++++++++++++++++++++++++++++

UART0 is used for:

 - BKFIL to download images
 - Logging and CLI of Non-secure world

UART1 is used for logging of secure world.

Flash all-app.bin
++++++++++++++++++++++++++++++++++

Burn Armino/build/scan/bk7236/all-app.bin to the board using BKFIL.

Running and Output
----------------------------------

Case 1 - Scan all SSIDs
+++++++++++++++++++++++++++++++++++

Restart the development board to observe the scan process.

Output
***********************************

::

 scan
 wifi I:(7480): scaning
 wifi I:(7496): scan all APs
 wpa I:(7496): Setting scan request: 0.100000 sec
 hitf I:(8168): get scan result:32
 cli:I(8174):rx event <1 0>
 wifi I:(8174): scan found 32 AP
              SSID                     BSSID         RSSI chan security
 -------------------------------- -----------------   ---- ---- ---------

                                  4e:6a:48:0d:d8:70    -38  1 WPA2-AES
 Tenda_062C88                     50:2b:73:06:2c:88    -40 13 WPA2-AES
 TP-LINK_D870                     3c:6a:48:0d:d8:70    -41  1 WPA2-AES
                                  74:45:2d:23:96:68    -44  4 WPA2-AES
 TP-LINK_599F                     9c:a6:15:ed:59:9f    -47 11 WPA2-AES
 OpenWrt                          20:76:93:47:54:7a    -47  6 WPA3-WPA2-MIX
 ASUS-RT-AX55                     50:eb:f6:14:fb:e0    -52  6 WPA3-WPA2-MIX
 Redmi_BC96                       c8:bf:4c:af:35:c7    -56  1 WPA2-AES
 Bekencorp-wifi                   00:dd:b6:15:ed:f1    -60  6 WPA2-MIX
 pengfei.zhang                    00:dd:b6:15:ed:f2    -60  6 WPA2-MIX
 Bekencorp-guest                  00:dd:b6:15:ed:f0    -60  6 WPA2-MIX
 wifi-team                        34:46:ec:3c:09:78    -62 11 WPA3-WPA2-MIX
 belon                            d4:ee:07:58:df:3c    -66  6 WPA2-AES
 siwifi-2894-2.4G                 a8:5a:f3:07:28:94    -68  7 WPA2-AES
 tlmap                            20:dc:e6:a2:3b:82    -68  6 WPA2-AES
 Redmi_BC96                       24:cf:24:0d:35:fc    -71  1 WPA2-AES
 pengfei.zhang                    74:50:4e:3f:84:d2    -77 11 WPA2-MIX
 Bekencorp-wifi                   74:50:4e:3f:84:d1    -77 11 WPA2-MIX
 Bekencorp-guest                  74:50:4e:3f:84:d0    -79 11 WPA2-MIX
 test_2022                        c8:3a:35:88:14:a1    -82  2 WPA2-AES
 NETGEAR66                        cc:08:fb:3b:d7:10    -85 11 WPA2-AES
 pengfei.zhang                    00:dd:b6:16:83:52    -99  1 WPA2-MIX
 TP-LINK_6C28                     50:fa:84:8f:6c:28    -53 12 WEP
 Broadcom                         00:90:4c:1f:d2:4d    -33  1 NONE
 ZQ_AUSU                          2c:4d:54:74:5a:28    -35  1 NONE
 FAST_3AC5                        f4:6a:92:a4:3a:c5    -36  6 NONE
                                  00:dd:b6:15:ed:f3    -58  6 NONE
 TP-LINK_236A                     74:05:a5:fe:23:6a    -60 11 NONE
 testrfefeef                      d4:a6:52:5b:12:02    -67 11 NONE
 Xiaomi_5510_AAEF                 44:df:65:a6:55:11    -71  3 NONE
                                  74:50:4e:3f:84:d3    -79 11 NONE
 midea_e2_0136                    fc:df:00:6a:58:14    -86  1 NONE

Case 2 - Scan specific SSID
+++++++++++++++++++++++++++++++++++

After setting EXAMPLE_SCAN_ALL_SSID to "n" and "myssid" to the specific SSID in Kconfig.projbuild, restart the development board to observe the scan process.

Output
***********************************

::

 scan wifi-team
 wifi I:(1379834): scaning
 wifi I:(1379834): scan wifi-team
 wpa I:(1379834): Setting scan request: 0.100000 sec
 wpa I:(1379934): use ssids from scan command
 hitf I:(1380506): get scan result:1
 cli:I(1380506):rx event <1 0>
 wifi I:(1380508): scan found 1 AP
              SSID                     BSSID         RSSI chan security
 -------------------------------- -----------------   ---- ---- ---------

 wifi-team                        34:46:ec:3c:09:78    -62 11 WPA3-WPA2-MIX

