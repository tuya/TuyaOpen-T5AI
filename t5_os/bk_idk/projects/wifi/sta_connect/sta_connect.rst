STA Connect
=======================================

Project Overview
--------------------------

This project is used for demonstrating [STA connect](https://docs.bekencorp.com/armino/bk7236/en/latest/developer-guide/wifi/bk_wifi_mode.html)

  - This project shows the process of connecting BEKEN Wi-Fi module as STA to AP.
  - By configuring ssid and password in advance, sta_connect could achieve connect automatically after startup.

Hardware Requirements
----------------------------

All Beken development board.

Configure and Build
--------------------------------

Configure the Project
+++++++++++++++++++++++++++++++

You can configure ssid and password in projects/examples/wifi/sta_connect/main/Kconfig.projbuild

::

 menu "Example Configuration"

    config EXAMPLE_WIFI_SSID
        string "WiFi SSID"
        default "TP-LINK_599F"
        help
            SSID (network name) for the example to connect to.

    config EXAMPLE_WIFI_PASSWORD
        string "WiFi Password"
        default "12345678"
        help
            WiFi password (WPA or WPA2) for the example to use.
 endmenu

Build
+++++++++++++++++++++++++++++++

Enter the following command in the Armino root directory to compile:

> make bk7236 PROJECT=examples/wifi/sta_connect

For preparations such as toolchain installation, please refer to [Quick Start](https://docs.bekencorp.com/armino/bk7236/en/latest/get-started/index.html).

Connect UART
++++++++++++++++++++++++++++++++++

UART0 is used for:

 - BKFIL to download images
 - Logging and CLI of Non-secure world

UART1 is used for logging of secure world.

Flash all-app.bin
++++++++++++++++++++++++++++++++++

Burn Armino/build/sta_connect/bk7236/all-app.bin to the board using BKFIL.

Running and Output
----------------------------------

Restart the development board to observe the sta_connect process.

Output
+++++++++++++++++++++++++++++++

::

 sta TP-LINK_599F 12345678
 os:I(3070):create shell_handle, tcb=28040a38, stack=[2803f610-28040a10:5120], prio=5
 bk_wifi:I(3070):ssid:TP-LINK_599F key:12345678
 wifid I:(3072): phy_s_ch_w_r err,band:0,cf:255,pf:255,type:5
 wifid I:(3080): mm_add_if_req: vif_idx=0, type=0, p2p=0, status=0
 wpa I:(3086): State: DISCONNECTED -> INACTIVE
 wpa I:(3086): State: INACTIVE -> DISCONNECTED
 lwip:I(3086):mac c8:47:8c:10:a5:75
 os:I(3086):create pskc, tcb=2804dd58, stack=[2804d530-2804dd30:2048], prio=6
 wpa I:(3086): PSKC: ssid TP-LINK_599F, passphrase 12345678
 wpa I:(3122): auto_reconnect: count 0, timeout 0, disable_auto_reconnect_after_disconnect 0

 wpa I:(3156): Setting scan request: 0.000000 sec
 wpa I:(3156): State: DISCONNECTED -> SCANNING
 wifid I:(3160): [KW:]scanu_start_req:src_id=14, vif=0, dur=0, chan_cnt=13, ssid_len=9,ssid=TP-LINK_599F, bssid=0xffff-ffff-ffff
 wpa I:(3164): Event SCAN_STARTED (47) received

 cal I:(3356): temp_code:-7 - adc_code:452 - adc_trend:[6]:464->[7]:444

 wifid I:(3730): [KW:]scanu_confirm:status=0,req_type=0,upload_cnt=4,recv_cnt=43,time=570543us,result=1
 wpa I:(3734): Event SCAN_RESULTS (3) received
 wpa I:(3734): Scan completed in 0.568000 seconds
 hitf I:(3734): get scan result:1

 cal I:(4420): temp_code:-10 - adc_code:467 - adc_trend:[7]:444->[6]:464

 wpa I:(5946): PSKC: end

 wpa I:(6034): State: SCANNING -> ASSOCIATING
 wifid I:(6036): [KW:]conn vif0-0,auth_type:3,bssid:4634-3cec-7809,ssid:TP-LINK_599F,is encryp:8.
 wifid I:(6036): chan_ctxt_add: CTXT0,freq2437MHz,bw20MHz,pwr0dBm
 wifid I:(6036): chan_reg_fix:VIF0,CTXT0,type3,ctxt_s0,nb_vif0
 wifid I:(6038): rc_init: station_id=0 format_mod=5 pre_type=0 short_gi=1 max_bw=0
 wifid I:(6038): rc_init: nss_max=0 mcs_max=9 r_idx_min=0 r_idx_max=3 no_samples=10
 wifid I:(6040): rc rates:[400] [2801] [2a02] [2c00] [2c05] [2c09] [2a05] [2a04] [2803] [2807] 
 wifid I:(6040): txp inc,rssi:-52,rssi_b:-52,rssi_s:112,rate:0,fmt:0,idx_o:24,idx_c:24,shift:0,idx:24
 wpa I:(6044): Event EXTERNAL_AUTH (55) received
 wpa I:(6044): sae_prepare_commit enter

 wpa I:(6250): sae_derive_commit enter

 wifid I:(6452): mm_ap_drift: clear ps, vif_bit 0x10,ps_bit 0x0

 wpa I:(6970): sae_derive_commit exit
 wpa I:(6970): sae_prepare_commit exit
 wpa I:(6990): Event TX_STATUS (16) received

 wpa I:(7054): Event RX_MGMT (18) received
 wpa I:(7054): SME: SAE authentication transaction 1 status code 0
 wpa I:(7056): sae_process_commit enter

 wpa I:(8534): sae_process_commit exit
 cal I:(8542): temp_code:-7 - adc_code:452 - adc_trend:[6]:464->[7]:444
 wpa I:(8552): Event TX_STATUS (16) received
 wpa I:(8570): Event RX_MGMT (18) received
 wpa I:(8570): SME: SAE authentication transaction 2 status code 0
 wifid I:(8572): [KW:]assoc_req_send:is ht, seq_num:29
 wifid I:(8582): [KW:]assoc_rsp:status0,tx_s0x80800000
 wifid I:(8594): [KW:]mm_set_vif_state,vif=0,vif_type=0,is_active=1, aid=0x8
 wpa I:(8594): Event ASSOC (0) received
 wpa I:(8596): State: ASSOCIATING -> ASSOCIATED
 wifid I:(8604): mm_tbtt_compute:frame 3248,tim 720,tim_len 6,after_tim 2078
 wpa I:(8628): State: ASSOCIATED -> 4WAY_HANDSHAKE
 wpa I:(8630): WPA: TK
 wpa I:(8668): State: 4WAY_HANDSHAKE -> 4WAY_HANDSHAKE
 hitf I:(8692): add CCMP
 wpa I:(8692): State: 4WAY_HANDSHAKE -> GROUP_HANDSHAKE
 hitf I:(8692): add CCMP
 hitf I:(8694): add BIP
 wpa I:(8694): State: GROUP_HANDSHAKE -> COMPLETED
 wifid W:(8696): hitfcli:I(8698):BK STA disconnected, reason(257), local_generated
 lwip:I(8700):configuring interface sta (with DHCP client)
 lwip:I(8700):[KW:]sta:DHCP_DISCOVER()
 cli:I(8700):BK STA connected TP-LINK_599F

 lwip:I(9022):[KW:]sta:DHCP_DISCOVER()
 lwip:I(10022):[KW:]sta:DHCP_DISCOVER()

 lwip:I(10186):[KW:]sta:DHCP_OFFE
 R received in DHCP_STATE_SELECTING state
 lwip:I(10186):[KW:]sta:DHCP_REQUEST(netif=0x2800fbac) en   1

 lwip:I(10570):[KW:]sta:DHCP_ACK received
 cli:I(10572):BK STA got ip
