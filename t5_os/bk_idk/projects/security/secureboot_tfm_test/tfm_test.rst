**Support Board: BK7236**

Secure Boot and Secure OTA
=======================================

Project Overview
--------------------------

This project is used for demonstrating [Security Boot](https://docs.bekencorp.com/armino/bk7236/en/latest/developer-guide/security/bk_security_boot.html), [Security Upgrade](https://docs.bekencorp.com/armino/bk7236/en/latest/developer-guide/ota/bk_security_ota.html).

**The security function of BK7236 is still under development and improvement, please note the following restrictions**

  - This project only supports running on 8M FLASH.
  - The generated otp_efuse_config.json only enables secure boot and FLASH AES encryption, you can enable more security features through EFUSE.
  - In the sample log, some logs are enabled for BL1/BL2 and Secure World (TF-M), and you can choose to disable them through EFUSE or compile options.

Hardware Requirements
----------------------------

BK7236 development board with 8MB FLASH.

Configure and Build
--------------------------------

Configure the Project
+++++++++++++++++++++++++++++++

Configure Security Counters
************************************

The security counter can be configured through app_security_counter in ota.csv under the project, which is used to prevent version downgrade during upgrade.
Note that the app_security_counter in ota.csv cannot be less than the app_security_counter value of the currently running program on the BK7236 development board,
Otherwise, the upgrade cannot be successful. If the secure boot program has never been run on a development build, the app_security_counter value should be 0.

About app_security_counter, please refer to [Security Configuration](https://docs.bekencorp.com/armino/bk7236/en/latest/developer-guide/config_tools/bk_config_security.html)

Configure BL2 Log Level
************************************

The BL2 log level can be configured by modifying CONFIG_TFM_BL2_LOG_LEVEL under config/bk7236/config under the project.


Build
+++++++++++++++++++++++++++++++

Enter the following command in the Armino root directory to compile:

> make bk7236 PROJECT=security/secureboot APP_VERSION="v1.0"

For preparations such as toolchain installation, please refer to [Quick Start](https://docs.bekencorp.com/armino/bk7236/en/latest/get-started/index.html).

Build different APP version
*****************************************

If you want to run a secure upgrade, it is recommended to compile twice with different APP_VERSION, for example:

  - When compiling for the first time, APP_VERSION="v1.0", the generated all-app.bin is used for burning.
  - When compiling for the second time, APP_VERSION="v2.0", the generated ota.bin is used for upgrade.

You can determine the currently running version by checking the APP Version in the startup log. The version used can also be judged by printing time:

```
bk_init:I(0):armino app init: Jul 21 2023 10:12:47
bk_init:I(2):APP Version: v1.0
```

Connect UART
++++++++++++++++++++++++++++++++++

UART0 is used for:

 - BKFIL to download images
 - Logging and CLI of Non-secure world

UART1 is used for logging of secure world.

Flash all-app.bin
++++++++++++++++++++++++++++++++++

Burn Armino/build/secureboot/bk7236/all-app.bin to the board using BKFIL.

Configure OTP/EFUSE
++++++++++++++++++++++++++++++++++

Burn the Armino/build/secureboot/bk7236/otp_efuse_config.json file to the development board. For the burning steps, please refer to [OTP/EFUSE Configuration](https://docs.bekencorp.com/armino/bk7236/zh_CN/latest/ developer-guide/config_tools/bk_config_otp_efuse.html).

**Important: Please burn all-app.bin first, then configure OTP/EFUSE, avoid power failure during configuration**

Generated otp_efuse_config.json
*****************************************

.. code-block:: json

  {
     "User_Operate_Enable": "false",
     "Security_Ctrl_Enable": "true",
     "Security_Data_Enable": "true",
     "eFuse_Enable": "false",
     "User_Operate": [],
     "Security_Ctrl": [
         {
             "secure_boot_enable": "0,0,1",
             "secure_boot_debug_disable": "0,3,0",
             "fast_boot_disable": "0,2,0",
             "boot_mode": "0,3,1",
             "secure_boot_clock_select": "0,4,1",
             "random_delay_enable": "0,5,0",
             "direct_jump_enable": "0,6,0",
             "security_boot_critical_error": "0,7,0",
             "attack_nmi_enable": "2,4,0",
             "spi_to_ahb_disable": "2,5,0",
             "auto_reset_enable[0]": "2,6,0",
             "auto_reset_enable[1]": "2,7,0",
             "flash_aes_enable": "3,5,0",
             "spi_download_disable": "3,6,0",
             "swd_disable": "3,7,0"
         }
     ],

     "Security_Data": [
         {
             "name": "bl1_rotpk_hash",
             "mode": "write",
             "permission": "WR",
             "start_addr": "0x4B100528",
             "last_valid_addr": "0x4B100548",
             "byte_len": "0x20",
             "data": null,
             "data_type": "hex",
             "status": "true"
         },
         {
             "name": "bl2_rotpk_hash",
             "mode": "write",
             "permission": "WR",
             "start_addr": "0x4B100548",
             "last_valid_addr": "0x4B100568",
             "byte_len": "0x20",
             "data": "ca68e9b3e991433ca9a6a2d6a74fc9dd5ccf559a4ae179537bde2772e0393ccf",
             "data_type": "hex",
             "status": "true"
         }
     ]
  }

Running and Output
----------------------------------

Case 1 - Secure Boot
+++++++++++++++++++++++++++++++++++

Secure Boot CLI Commands
***********************************

After configuring OTP/EFUSE, restart the development board to observe the secure boot process.

Security World Output
***********************************

::

 I (0) flash: id=0xc84017
 I (0) flash: write status reg: 38, status_reg_size: 1
 I (0) flash: write status reg: 0, status_reg_size: 1
 [INF] Starting bootloader
 I (0) otp: init
 [INF] step#1: open primary/secondary image area
 [INF] step#1: determine swap type
 [INF] boot_read_sectors 362
 [INF] step1: read image header of each slots
 [INF] boot_read_image_headers 96
 [INF] step1: slot compatible checking
 [INF] step1: slot is compatible
 [INF] read swap state: flash_off=45000, off=2d7ff0
 [INF] read swap info: flash_off=45000, off=2d7fd8
 [INF] read copy done: off=2d7fd8
 [INF] Primary image: magic=good, swap_type=0x1, copy_done=0x3, image_ok=0x3
 [INF] read swap state: flash_off=31d000, off=2d7ff0
 [INF] read swap info: flash_off=31d000, off=2d7fd8
 [INF] read copy done: off=2d7fd8
 [INF] Secondary image: magic=unset, swap_type=0x1, copy_done=0x3, image_ok=0x3
 [INF] Boot source: primary slot
 [INF] boot_read_image_headers 96
 [INF] step1: partial swap was NOT detected
 [INF] read swap state: flash_off=45000, off=2d7ff0
 [INF] read swap info: flash_off=45000, off=2d7fd8
 [INF] read copy done: off=2d7fd8
 [INF] read swap state: flash_off=31d000, off=2d7ff0
 [INF] read swap info: flash_off=31d000, off=2d7fd8
 [INF] read copy done: off=2d7fd8
 [INF] Swap type: none
 [INF] boot state=1
 
 image calculated hash:
 3f 15 52 eb a4 18 9d c5 72 4a 95 ea 5033 93 ed 81 b9 38 ee cd 7c c3 66 ec fa a2 31 79 af d8 00 
 [INF] tlv_iter: magic=6908
 [INF] tlv_iter: magic is ok
 [INF] tlv iter: off=1d4144 len=4 type=50, rc=0
 I (0) otp: dump otp, id=14, size=32
 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
 I (0) otp: read, id=14, out_len=32
 [INF] tlv iter: off=1d414c len=5b type=60, rc=0
 [INF] tlv iter: off=1d41af len=20 type=10, rc=0
 [INF] IMAGE_TLV_SHA256
 image tlv hash
 3f 15 52 eb a4 18 9d c5 72 4a 95 ea 50 33 93 ed 81 b9 38 ee cd 7c c3 66 ec fa a2 31 79 af d8 00 
 [INF] Image hash OK
 [INF] tlv iter: off=1d41d3 len=5b type=2, rc=0
 [INF] IMAGE_TLV_PUBKEY
 [INF] find key, image0, key_len=91
 Try to find key:
 30 59 30 13 06 07 2a 86 48 ce 3d 02 01 06 08 2a 86 48 ce 3d 03 01 07 03 42 00 04 c7 03 95 cd 21 
 3d a2 e8 6d af 37 34 12 21 2e ae a0 d5 d8 76 68 a7 b5 5e dc c3 fd c1 95 f9 ed 75 f3 5b d3 7b 7a 
 c7 a0 d0 78 ed cc 96 25 58 36 d7 8e 9c a4 58 01 b8 b8 a7 04 7c 12 d0 3e 0d ff db 
 Key hash is:
 b3 e9 68 ca 3c 43 91 e9 d6 a2 a6 a9 dd c9 4f a7 9a 55 cf 5c 53 79 e1 4a 72 27 de 7b cf 3c 39 e0 
 I (0) otp: dump otp, id=11, size=32
 b3 e9 68 ca 3c 43 91 e9 d6 a2 a6 a9 dd c9 4f a7 9a 55 cf 5c 53 79 e1 4a 72 27 de 7b cf 3c 39 e0 
 I (0) otp: read, id=11, out_len=32
 [INF] Retried key hash len=32
 
 Retrieved key hash is:
 b3 e9 68 ca 3c 43 91 e9 d6 a2 a6 a9 dd c9 4f a7 9a 55 cf 5c 53 79 e1 4a 72 27 de 7b cf 3c 39 e0 
 [INF] Key hash is equal
 [INF] tlv iter: off=1d4232 len=47 type=22, rc=0
 [INF] verify sig
 [INF] tlv iter: off=1d4232 len=47 type=22, rc=1
 [INF] tlv_iter: magic=6908
 [INF] tlv_iter: magic is ok
 I (0) otp: write, id=14, out_len=32
 [INF] tlv_iter: magic=6908
 [INF] tlv_iter: magic is ok
 [INF] Bootloader chainload address offset: 0x45000
 [INF] Jumping to the first image slot
 Jump to 1st app, msp=0x28000800 vector=0x204ece9
 I (0) otp: init
 [Sec Thread] Secure image initializing!
 TF-M isolation level is:  0x00000002
 TF-M FP mode: Hardware
 Lazy stacking enabled
 Booting TF-M Dev2.0.3+6662014
 I (0) flash: id=0xc84017
 I (0) flash: write status reg:38, status_reg_s 
 I (0) flash: write status reg:0, status_reg_s
 E (0) otp: otp read stub, id=0
 SCB->AIRCR:0xfa056408

Non-Secure World Output
***************************************

::

 os:I(0):
 os:I(0):mem_type start    end      size    
 os:I(0):-------- -------- -------- --------
 os:I(0):itcm     0x10000000 0x10003c70 15472   
 os:I(0):dtcm     0x30000000 0x300018f4 6388    
 os:I(0):ram      0x38010000 0x3807ebf8 453624  
 os:I(0):non_heap 0x38010000 0x3803f028 192552  
 os:I(0):iram     0x38010000 0x38010814 2068    
 os:I(0):data     0x38010814 0x3801b790 44924   
 os:I(0):bss      0x3801b7a0 0x3803f024 145540  
 os:I(0):non_cache 0x3802c67c 0x3803f022 76198   
 os:I(0):heap     0x3803f028 0x3807ebf8 261072  
 flash:I(0):id=0xc84017
 flash:I(0):write status reg:4004, status_reg_size:1
 ate enabled is 1
 driver_init end
 init:I(0):reason - power on
 init:I(0):regs - 0, 0, 0
 init:I(0):armino rev: 
 init:I(0):armino soc id:53434647_72360001
 init:I(0):trng enable
 mailbox:I(0):mailbox_config
 mailbox:I(0):bk_mailbox_recv_callback_register
 mailbox:I(0):bk_mailbox_recv_callback_register
 init:I(0):Intialize random stack guard.
 os:I(0):create main, tcb=380408d0, stack=[3803f8a8-380408a8:4096], prio=5
 start user app thread.
 os:I(0):create app, tcb=38041978, stack=[38040950-38041950:4096], prio=2
 os:I(0):os time(0ms).
 os:I(0):base aon rtc time: 0:46
 os:I(0):create IDLE, tcb=30000000, stack=[3000005c-3000065c:1536], prio=0
 os:I(0):create Tmr Svc, tcb=3000065c, stack=[300006b8-30000cb8:1536], prio=5
 bk_init:I(0):armino app init: Jul 21 2023 10:12:47
 bk_init:I(2):APP Version: v1.0
 os:I(4):create media_major_thread, tcb=38042af0, stack=[38041cc8-38042ac8:3584], prio=3
 media_app:W(6):media app thread startup complete
 media0:I(8):media major thread startup complete
 os:I(10):create event, tcb=380434c0, stack=[38042c98-38043498:2048], prio=1
 lwip:I(12):init TCP/IP
 os:I(16):create tcp/ip, tcb=38043fc8, stack=[380437a0-38043fa0:2048], prio=7
 os:I(18):create storage_task_thread, tcb=38045270, stack=[38044648-38045248:3072], prio=3
 I (20) wifi: wifi initing
 os:I(22):create syswq, tcb=38045808, stack=[380453e0-380457e0:1024], prio=6
 I (24) wifid: IP Rev: 802.11ax
 I (26) wifid: mm_env_init,interval = 0,bcn_loss_int 0,repeat 0.
 I (28) wifid: init td info :61440 pck:3
 
 I (30) cal: get rfcali_mode:1
 I (36) cal: cali bias(e),t1:21,t2:2548
 I (200) cal: cali bias(e),t1:21,t2:2548
 I (200) cal: calibration_main over
 I (200) cal: *********** finally result BK7236 **********
 I (200) cal: gbias_after_cal        : 0x0
 I (204) cal: gtx_q_dc_comp          : 0x7fa
 I (208) cal: gtx_i_dc_comp          : 0x80d
 I (212) cal: gtx_i_gain_comp        : 0xfff
 I (216) cal: gtx_q_gain_comp        : 0xfee
 I (220) cal: gtx_phase_comp         : 0x7fe
 I (224) cal: gtx_phase_ty2          : 0x800
 I (228) cal: gtx_ifilter_corner over: 0x8a
 I (232) cal: gtx_qfilter_corner over: 0x88
 I (236) cal: const_iqcal_p over     : 0x400
 I (240) cal: const_iqcal_idx over   : 0x1d0
 I (244) cal: g_rx_dc_gain_tab 0 over: 0x807c807c
 I (250) cal: g_rx_dc_gain_tab 1 over: 0x7c7c8078
 I (254) cal: g_rx_dc_gain_tab 2 over: 0x7c787878
 I (258) cal: g_rx_dc_gain_tab 3 over: 0x78747878
 I (262) cal: g_rx_dc_gain_tab 4 over: 0x78707870
 I (268) cal: g_rx_dc_gain_tab 5 over: 0x78707870
 I (272) cal: g_rx_dc_gain_tab 6 over: 0x79707970
 I (276) cal: g_rx_dc_gain_tab 7 over: 0x79717970
 I (280) cal: grx_amp_err_wr : 0x200
 I (286) cal: grx_phase_err_wr : 0x000
 I (290) cal: *****************************************
 I (294) cal: NO TXPWR_TAB_TAB found in flash
 I (298) cal: Load default txpwr for b: 0x120ff1ec
 I (302) cal: Load default txpwr for g: 0x120ff222
 I (308) cal: fit n20 table with dist:4
 I (310) cal: Load default txpwr for n40:0x120ff230
 I (316) cal: Load default txpwr for ble: 0x120ff1fa
 E (320) cal: NO TXID found in flash, use def temp: 474
 E (324) cal: temp in flash is: 474
 tempd:I(328):init temperature 474
 os:I(332):create tempd, tcb=38046620, stack=[380461f8-380465f8:1024], prio=5
 E (338) tempd: I (340): xtal init cal: NO TXID found final: 474, 70, 80
   flash, use lpf i&q:138, 136
 E (348) cal: NO TXID found in flash, use def xtal:59
 E (352) cal: xtal in flash is: 48
 I (356) cal: xtal_cali:48
 I (358) cal: --init_xtal = 48
 I (360) cal: NO TLV tag in flash
 gtx_dcorPA=0x6
 I (364) cal: pwr_gain_base default=0x19637c00 11b=0x19637c00 11g=0x99637c00 ble=0x99628c00
 I (374) cal: -----pwr_gain:40, rate=54, g_idx:40, shift_b:0, shift_g:0
 I (380) cal: idx:40=40+(0),r:54,xtal:48,pwr_gain:99637128
 os:I(386):create kmsgbk, tcb=38047a88, stack=[38046a60-38047a60:4096], prio=6
 os:I(392):create core_thread, tcb=380489a8, stack=[38048180-38048980:2048], prio=7
 os:I(400):create rf_arbitrate, tcb=38049368, stack=[38048b40-38049340:2048], prio=8
 I (408) coex: coex_init ok
 os:I(410):create wpas_thread, tcb=3804a888, stack=[38049860-3804a860:4096], prio=4
 I (418) wifi: wifi inited(1) ret(0)
 I (420) wifid: trace_mem_apply : length 28
 I (424) wifid: dbg_mem_apply : length 96
 I (428) wifid: hal_machw_mem_apply : length 228
 user app entry(0x0)
 gpio:I(436):bk_gpio_driver_init:has inited
 os:I(442):create cli, tcb=3804b960, stack=[3804b138-3804b938:2048], prio=5
 ATE enabled = 1

Case 2 - Security Upgrade
+++++++++++++++++++++++++++++++++++

On the basis of secure boot, security upgrade can be further exemplified, including the following steps:

  - Start the HTTP server
  - connect router
  - trigger upgrade

Start the HTTP server
*************************************

Use HFS(Http File Server) to start an HTTP server, you can also use other HTTP servers.
Copy Armino/build/secureboot/bk7236/ota.bin to the HFS server, as shown below:

![HFS Server](hfs.png)

Connect Router
*************************************

When testing security upgrades, the computer running the HTTP server and the development board need to be in the same local network.
In this example, the router/password is Tenda_062C88/1234568, use the following command to connect the development board to the router:

> sta Tenda_062C88 12345678

Trigger OTA
*************************************

Run OTA with the following command:

> http_ota http://192.168.5.100:8080/ota.bin

When the OTA download is complete and TF-M completes the integrity check of the downloaded IMAGE and then reboot. After reboot, BL2 will verify the signature of the downloaded IMAGE, and the verification has passed
Then move the IMAGE to the main partition.

Non-secure World Log
**************************************

::

 sta Tenda_062C88 12345678
 
 os:I(741246):create shell_handle, tcb=3803fda0, stack=[3804cd30-3804e130:5120], prio=5
 I (741246) wifi: ssid:Tenda_062C88 key:12345678
 I (741248) wifi: sta configuring
 I (741252) wifi: sta disconnecting
 lwip:I(741254):sta ip down
 I (741258) wpa: cancel scan request timer
 I (741262) wifid: sm external auth not in correct state state:0
 W (741266) wifid: sm_disconnect_req_handler: reason_code 3, vif type 0, active 1
 I (741274) wifid: sm_disconnect_continue: host 1,0.
 I (741278) wifid: sm_deauth_send: reason code 3
 I (741298) wifid: sm_frame_tx_cfm_handler: success tx, state 10, fctl 0xc0, tx_rlr 0x80800000
 I (741298) wifid: sm_disconnect_finish
 I (741298) wifid: sm_delete_resources
 I (741300) wifid: mm_hw_cfg: msgid = 30, mm = 1, hw = 3
 W (741306) wifid: [KW:] mm_set_vif_state, vif = 0, vif_type = 0, is_active = 0
 W (741312) wifid: chan_unregister_fix_req: VIF0
 W (741318) wifid: chan_ctxt_unlink: CTXT0, VIF0, status=1, nb_vif=0, nb_ctxt=0
 W (741324) wifid: chan_ctxt_del: CTXT0
 I (741328) wifid: mm_set_idle: req = 1, mm = 1, hw = 3
 I (741334) wifid: mm_set_idle: req = 1, mm = 0, hw = 0
 I (741338) wifid: me_set_active_cfm_handler: TASK_SM state 10
 I (741344) rwnx: disconnect
 I (741346) wpa: Event DEAUTH (11) received
 I (741350) hitf: del hw key idx=1
 I (741354) hitf: del hw key idx=0
 I (741356) wpa: State: COMPLETED -> DISCONNECTED
 I (741360) wpa: State: DISCONNECTED -> DISCONNECTED
 I (741366) wifi: sta disconnected (7)
 I (741370) wpa: Event DEAUTH (11) received
 I (741374) wifi: sta configured (7)
 I (741376) wifi: sta connecting
 I (741380) wifi: sta disconnecting
 I (741382) wifi: sta disconnected (7)
 I (741386) wpa: Setting scan request: 0.000000 sec
 I (741390) wifi: sta connected(f)
 I (741394) wifi: sta starting
 I (741396) wifi: sta already started, ignored!
 I (741402) wpa: wpa_supplicant_scan
 I (741404) wpa: State: DISCONNECTED -> SCANNING
 I (741408) wpa: scan work start, deinit=0
 I (741412) wpa: req driver to scan
 I (741416) wpa: wpa_driver_scan2: Scan specified BSSID ff:ff:ff:ff:ff:ff
 I (741422) hitf: send scan req to driver
 I (741426) wifid: ht in scan
 W (741430) wifid: [KW:]scanu_start_req: src_id = 14, dur = 0, chan_cnt = 13, ssid_len = 12, ssid = Tenda_062C88, bssid = 0xffff-ffff-ffff
 W (741442) wifid: [KW:]scan_start_req, vif = 0, chan_cnt =13
 I (741448) wpa: Event SCAN_STARTED (47) received
 
 #I (741492) wifid: scan_chan_end, scan_idx=0, freq=2412MHz, recv_frame 4
 
 #I (741536) wifid: scan_chan_end, scan_idx=1, freq=2417MHz, recv_frame 5
 I (741582) wifid: scan_chan_end, scan_idx=2, freq=2422MHz, recv_frame 5
 I (741628) wifid: scan_chan_end, scan_idx=3, freq=2427MHz, recv_frame 7
 I (741674) wifid: scan_chan_end, scan_idx=4, freq=2432MHz, recv_frame 14
 I (741718) wifid: scan_chan_end, scan_idx=5, freq=2437MHz, recv_frame 20
 I (741764) wifid: scan_chan_end, scan_idx=6, freq=2442MHz, recv_frame 23
 I (741810) wifid: scan_chan_end, scan_idx=7, freq=2447MHz, recv_frame 25
 I (741856) wifid: scan_chan_end, scan_idx=8, freq=2452MHz, recv_frame 28
 I (741900) wifid: scan_chan_end, scan_idx=9, freq=2457MHz, recv_frame 29
 I (741946) wifid: scan_chan_end, scan_idx=10, freq=2462MHz, recv_frame 36
 I (741992) wifid: scan_chan_end, scan_idx=11, freq=2467MHz, recv_frame 37
 I (742038) wifid: scan_chan_end, scan_idx=12, freq=2472MHz, recv_frame 40
 W (742038) wifid: [KW:]scan_chan_end,scan end
 W (742038) wifid: [KW:]scanu_confirm:status=0, req_type=0, upload_cnt=4, recv_cnt=40, time=586752us, result=1
 I (742046) wpa: Event SCAN_RESULTS (3) received
 I (742050) wpa: Scan completed in 0.598000 seconds
 I (742056) hitf: get scan result: 1
 I (742060) wpa: cipher2security 2 2 16 16
 I (742062) wpa: cipher2security 2 2 16 16
 I (742066) wpa: wpa_supplicant_connect
 I (742070) wpa: cancel scan request timer
 I (742074) wpa: State: SCANNING -> ASSOCIATING
 I (742078) wpa: wpa_driver_associate: auth_alg 0x1
 W (742084) wifid: [KW:]conn vif 0,active:0,vif_type:0,auth:0,state:0.
 W (742090) wifid: [KW:]conn bssid 2b50-673-882c, ssid:Tenda_062C88, is encryp:8.
 W (742096) wifid: sm_fast_connect start, bcn_len 97
 I (742102) wifid: me_check_special_device flag: 0
 W (742106) wifid: chan_ctxt_add: CTXT1, freq=2472MHz, bw=20MHz, pwr=0dBm
 W (742112) wifid: chan_register_fix_req: VIF0, CTXT1, type=3
 W (742118) wifid: chan_ctxt_link: CTXT1, VIF0, status=0, nb_vif=0
 I (742126) wifid: rc_init: station_id=1 format_mod=0 pre_type=0 short_gi=0 max_bw=0
 I (742132) wifid: rc_init: nss_max=0 mcs_max=255 r_idx_min=4 r_idx_max=11 no_samples=8
 I (742140) wifid: rc rates: [4] [5] [9] [8] [6] [7] [a] [b]
 I (742146) wifid: mm_hw_cfg: msgid = 24, mm = 1, hw = 3
 I (742150) wifid: mm_ap_drift: prevent sleep vif_bit 0x4010, ps_bit 0x4, cnt0
 I (742158) wifid: mm_compute_ap_drift:tsf_start_local 0x0-2487e157, hal_time 0x24882c59, tsf 0x24882c5a.
 I (742168) wifid: mm_hw_cfg: msgid = 22, mm = 1, hw = 3
 I (742172) wifid: mm_hw_cfg: msgid = 20, mm = 1, hw = 3
 I (742176) wifid: mm_set_idle: req = 0, mm = 1, hw = 3
 I (742182) wifid: me_set_active_cfm_handler: TASK_SM state 4
 W (742186) wifid: [KW:]sm_auth_send: seq 1, txtype 1, auth_type 0, seq 88
 I (742194) wifid: sm_frame_tx_cfm_handler: success tx, state 5, fctl 0xb0, tx_rlr 0x80800000
 I (742202) wifid: [KW:]sm_auth_handler: status code 0
 W (742206) wifid: me_comp_int, listen_int = 20, dtim = 1, listen_set = 0
 I (742220) wifid: sm_frame_tx_cfm_handler: success tx, state 8, fctl 0x0, tx_rlr 0x80800000
 W (742232) wifid: [KW:]sm_assoc_rsp_handler: status code 0
 W (742232) wifid: sm_assoc_done: aid 2
 I (742232) wifid: mm_hw_cfg: msgid = 30, mm = 1, hw = 3
 W (742234) wifid: [KW:] mm_set_vif_state, vif = 0, vif_type = 0, is_active = 1
 W (742242) wifid: chan_bcn_detect_start: CTXT1, VIF0
 W (742246) wifid: chan_register_once_req: VIF0, type=21, dur=102400us, freq=2472MHz, pwr=0dBm
 I (742254) wifid: VIF-0 Connection status 0
 I (742258) wifid: sm_check_keepa :28.
 W (742262) wifid: chan_update_req: VIF0, type=1
 I (742268) wifid: mm_set_ps_op: listen_interval = 20
 I (742272) rwnx: connect ok
 I (742274) wpa: Not associated - Delay processing of received EAPOL frame (state=ASSOCIATING bssid=00:00:00:00:00:00)
 I (742286) wpa: Event ASSOC (0) received
 I (742290) wpa: State: ASSOCIATING -> ASSOCIATED
 I (742294) wpa: cancel scan request timer
 I (742298) wpa: State: ASSOCIATED -> 4WAY_HANDSHAKE
 I (742304) wpa: WPA: TK 1b40e930b38db7a41559e8f15ee648ec
 I (742320) wpa: State: 4WAY_HANDSHAKE -> 4WAY_HANDSHAKE
 I (742332) hitf: add CCMP
 I (742332) hitf: add sta_mgmt_get_sta
 I (742332) hitf: add key, sta=1, vif=0, key=0
 I (742332) wifid: sta_mgmt_add_key: cipher 2
 I (742334) hitf: add hw key idx=9
 I (742338) wpa: State: 4WAY_HANDSHAKE -> GROUP_HANDSHAKE
 I (742342) wpa: WPA: GTK 75ef3cebe0701cec81fcdb3cbf14105b
 I (742348) hitf: adW (742350) wifid: chan_update_req: VIF0, type=5
 I (742354) wifid: mm_ap_drift: prevent sleep vif_bit 0x4011, ps_bit 0x200, cnt0
 I (742362) wifid: mm_compute_ap_drift:tsf_start_local 0x0-248b0150, hal_time 0x248b2717, tsf 0x248b2716.
 W (742370) wifid: mm_tbtt_compute: frame 1304, tim 744, tim_len 6, after_tim 110
 I (742378) wifid: mm_tbtt_compute: rx_tsfhi 0x0, rx_tsflo 0x248b04e8, hal_time 0x248b64a1, tsf 0x248b64a1.
 d CCMP
 I (742390) hitf: add is_broadcast_ether_addr
 I (742392) hitf: add key, sta=255, vif=0, key=1
 I (742396) hitf: add hw key idx=1
 I (742400) wpa: State: GROUP_HANDSHAKE -> COMPLETED
 W (742404) wifid: [KW:]ctrl_port_hdl:1
 lwip:I(742408): sta ip start
 lwip:I(742410):configuring interface sta (with DHCP client)
 lwip:I(742416):dhcp status timer:20000
 [KW:]dhcp_discover()
 cli:I(742422):BK STA connected Tenda_062C88
 I (742460) wifid: mm_ap_drift: prevent sleep vif_bit 0x5011, ps_bit 0x0, cnt1
 I (742460) wifid: mm_compute_ap_drift:tsf_start_local 0x0-248c9739, hal_time 0x248c9c2c, tsf 0x248c9c2e.
 I (742566) wifid: mm_ap_drift: prevent sleep vif_bit 0x4011, ps_bit 0x0, cnt2
 I (742566) wifid: mm_compute_ap_drift:tsf_start_local 0x0-248e2151, hal_time 0x248e261c, tsf 0x248e261c.
 I (742570) wifid: mm_ap_drift: clear ps, vif_bit 0x11, ps_bit 0x0
 [KW:]dhcp_discover()
 [KW:]DHCP_OFFER received in DHCP_STATE_SELECTING state
 [KW:]dhcp_select(netif=0x3801f640) en 1
 [KW:]DHCP_ACK received
 lwip:I(744510):ip_addr: 192.168.5.122
 lwip:I(744510):STA assoc delta:898, eapol delta:114, dhcp delta:2102, total:3114
 W (744512) wifid: [KW:]sm dhcp done vif:0
 W (744514) wifid: chan_update_req: VIF0, type=2
 cli:I(744520):BK STA got ip
 
 #
 #http_ota http://192.168.5.100:8080/ota.bin
 
 os:I(752292):create shell_handle, tcb=3803fda0, stack=[3804c340-3804d740:5120], prio=5
 cli:I(752292):http_ota_download :0x12161285 offset size fwu_id
      3000 1000 4
     34000 1000 5
      4000 41000 6
     35000 10000 7
    31d000 2d8000 3
 downloading OTA global header...
 crc 1863c392, version 0, header_len 20, image_num 1
 downloading OTA image header...
 image[0], image_len=2d8000, image_offset=40, flash_offset=31d000
 downloading OTA image0, expected data len=2d8000...
 downloaded OTA image0
 
 cli:I(762780):sucess.
 checking fwu image3...
 query status -132
 check fwu image3 success
 reboot

Secure World Log After Reboot
**************************************

After the download is complete, restart the development board. BL2 will verify the signature of the downloaded IMAGE. After the signature is passed, it will be swapped to the main partition. It can be observed that BL2 will display Swap type: test and
Starting swap using move algorithm, means using

The full log is as follows:

::

 I (0) otp: init
 [INF] Primary image: magic=good, swap_type=0x1, copy_done=0x3, image_ok=0x1
 [INF] Secondary image: magic=good, swap_type=0x1, copy_done=0x3, image_ok=0x3
 [INF] Boot source: none
 [INF] Swap type: test
 I (0) otp: dump otp, id=14, size=32
 I (0) otp: read, id=14, out_len=32
 I (0) otp: dump otp, id=11, size=32
 I (0) otp: read, id=11, out_len=32
 [INF] Starting swap using move algorithm.
 I (0) otp: dump otp, id=14, size=32
 I (0) otp: read, id=14, out_len=32
 I (0) otp: dump otp, id=11, size=32
 I (0) otp: read, id=11, out_len=32

Non-secure World Log After Reboot
**************************************

Similar to pre-upgrade startup log, but compile time and version number changed

::

 bk_init:I(0):armino app init: Jul 21 2023 10:18:48
 bk_init:I(2):APP Version: v2.0
