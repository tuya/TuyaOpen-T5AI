系统调试
===================

:link_to_translation:`en:[English]`

Armino平台BK7258系统调试命令
--------------------------------------


 - BK7258 CPU0的log通过串口DL_UART0输出(默认波特率为115200)
 - BK7258 CPU1的log通过mailbox转发到CPU0串口DL_UART0输出(默认波特率为115200)
 - BK7258 CPU2的log通过串口UART2输出(默认波特率为115200)
 - CPU1 log带cpu1标签（异常log除外）
 - 由于内存缓冲区的限制，每条log数据的字节数，要小于128字节。超过这个大小的log都会被shell 模块丢弃，并输出一条 !!some LOGs discarded!! 如果log数量太多，来不及输出导致log堆积，缓冲区用完，也会输出这条提示字符串。
 - 通过串口输入log命令查看当前log配置
 - 通过串口输入help命令查看当前支持的调试命令：

  ::

    #
    #log
    log: echo 1, level 3, sync 0.

    #
    #help

    ====Build-in Commands====
    help
    log: log [echo(0,1)] [level(0~5)] [sync(0,1)] [Whitelist(0,1)]
    debug: debug cmd [param] (ex:debug help)
    cpu1: cpu1 cmd (ex:cpu1 help)
    loginfo: log statistics.
    modlog: modlog tag_name on/off

    ====User Commands====
    2bd_master_test: 2bd_master_test {start|stop}
    2bd_slave_test: 2bd_slave_test {start|stop}
    AT: AT
    AT+ATVERSION: AT+ATVERSION
    AT+BLE: AT+TYPE_CMD=CMD_name,param1,...,paramn
    AT+MAC: mac <mac>, get/set mac. e.g. mac c89346000001
    AT+RST: AT+RST
    AT+VERSION: AT+VERSION
    AT+WIFI: AT+TYPE_CMD=CMD_name,param1,...,paramn, refer to the at guide.
    aon_rtc_auto_test: {id} {start|stop}
    aon_rtc_clock_src: aon_rtc_clock_src {26m|rosc}
    aon_rtc_driver: aon_rtc_driver {init|deinit}
    aon_rtc_dump: {id}
    aon_rtc_get_time: aon_rtc_get_time {id}
    aon_rtc_register: aon_rtc_register {id} {name} {period_tick} {period_cnt}, {callback}
    aon_rtc_rosc_cal: aon_rtc_rosc_cal interval
    aon_rtc_rosc_test: aon_rtc_rosc_test
    aon_rtc_time_of_day: aon_rtc_time_of_day {get|set} {sec|usec}
    aon_rtc_unregister: aon_rtc_unregister {id} {name}
    ap: ap ssid [password] [channel[1:14]]
    assert: asset and dump system information
    aud_adc_dma_test: aud_adc_dma_test {start|stop sample_rate}
    aud_adc_loop_test: aud_adc_loop_test {start|stop sample_rate}
    aud_adc_mcp_test: aud_adc_mcp_test {start|stop sample_rate}
    aud_dac_dma_test: aud_dac_dma_test {start|stop 8000|16000|44100|48000}
    aud_dac_eq_test: aud_dac_eq_test {start|stop}
    aud_dac_mcp_test: aud_dac_mcp_test {8000|16000|44100|48000}
    aud_dmic_dma_test: aud_adc_dma_test {start|stop sample_rate}
    aud_dmic_loop_test: aud_adc_loop_test {start|stop sample_rate}
    aud_dmic_mcp_test: aud_adc_mcp_test {start|stop sample_rate}
    aud_dtmf_loop_test: aud_dtmf_loop_test {start|stop}
    aud_dtmf_mcp_test: aud_dtmf_mcp_test {start|stop}
    backtrace: show task backtrace
    bcn_loss_intv: bcn_loss_intv interval repeat_num}
    blacklist: Set ssid blacklist
    ble: ble arg1 arg2
    bootcore: bootcore [core id] [mode: 1:start,0:stop]
    cali: cali auto_test
    capa: wifi capability config
    cca: cca open\close\show
    channel: channel {1~13} - set monitor channel
    ckmn: ckmn ckeck32k {32|other}
    ckmn_driver: ckmn_driver {init|deinit}
    ckmn_show_reg: ckmn show_reg
    ckmnautosw: ckmnautosw {26m|32k}
    ckmncorr: ckmncorr {26m|32k} {0|1|2|3}
    cpuload: show task cpu load
    cputest: cputest [count]
    dhcpc: dhcpc
    dma: dma {id} {init|deinit|start|stop|set_tran_len|get_remain_len}
    dma_chnl: dma_chnl alloc
    dma_chnl_free: dma_chnl_free {id}
    dma_chnl_test: {start|stop} {uart1|uart2|uart3} {wait_ms}
    dma_config: dma_config {mode|priority|pasue|src|dst}{mode value/priority value/dev,width,increase_en,loop_en,start_addr,end_addr}\0dma_copy: copy {src} {dst} {len}
    dma_driver: dma_driver {init|deinit}
    dma_int: dma_int {id} {reg|enable_hf_fini|disable_hf_fini|enable_fini|disable_fini|pause}
    dma_memcopy_test: copy {count|in_number1|in_number2|out_number1|out_number2}
    dtm_master_test: dtm_master_test {start|stop}
    dtm_slave_test: dtm_slave_test {start|stop}
    dvfs: dvfs [cksel_core] [ckdiv_core] [ckdiv_bus] [ckdiv_cpu0] [ckdiv_cpu1]
    dvfs_auto_test: dvfs_auto_test [period]
    efuse: efuse [-r addr] [-w addr data]
    efuse_driver: efuse_driver {init|deinit}
    efuse_test: efuse_test {write|read}
    efusemac: efusemac [-r] [-w] [mac]
    event: event {reg|unreg|post} {mod_id} {event_id}
    exception: {undefine|dabort|illegal|irq|fiq}
    fatfs_idle_test: fatfs_idle_test {start|stop|clean}
    fatfstest: fatfstest <cmd>
    filter: filter <bits> - bit0/d, 1/preq, 2/prsp, 3/b, 4/a
    flash: flash {erase|read|write} [start_addr] [len]
    flash_erase_test: cli_flash_erase_test with ble connecting
    flash_partition: flash_partition {show}
    flash_test: flash_test <cmd(R/W/E/N)>
    fmap_test: flash_test memory map
    get: get wifi status
    gpio: gpio     [set_mode/output_low/output_high/input/spi_mode]      [id]     [mode]
    gpio_driver: gpio_driver    [init/deinit]}
    gpio_int: gpio_int    [index]     [inttype/start/stop]     [low/high_level/rising/falling edge]
    gpio_kpsta: gpio_kpsta [register/unregister][index][io_mode][pull_mode][func_mode]
    gpio_low_power: gpio_low_power [simulate][param]
    gpio_map: gpio_map     [sdio_map/spi_map]
    gpio_wake: gpio_wake [index][low/high_level/rising/falling edge][enable/disable wakeup]
    http_ota: http_ota url
    httplog: httplog [1|0].
    i2c: i2c {init|write|read}
    i2c_driver: i2c_driver {init|deinit}
    i2s_master_test: i2s_master_test {start|stop}
    i2s_slave_test: i2s_slave_test {start|stop}
    id
    int: retarget {int_group0} {int_group1}
    ip: ip [sta|ap][{ip}{mask}{gate}{dns}]
    ipconfig: ipconfig [sta|ap][{ip}{mask}{gate}{dns}]
    ipdbg: ipdbg [function][value]
    iperf: iperf help
    iplog: iplog [modle][type]
    la: la rf_adc_40M/rf_adc[_80M]/fe_adc/rf_dac/fe_dac
    lwip_mem: print lwip memory information
    lwip_pbuf: print lwip pbuf information
    lwip_stats: print lwip protocal statistics
    mac: mac <mac>, get/set mac. e.g. mac c89346000001
    mem_apply: mem_apply [module][value]
    memdump: <addr> <length>
    memleak: [show memleak
    memset: <addr> <value 1> [<value 2> ... <value n>]
    memshow: show free heap
    memstack: show stack memory usage
    memtest: <addr> <length>
    memtest_r: <src> <dest> <size>
    memtest_wr: <addr> <count>
    memtime: <addr> <count> <0:write,1:read>
    micodebug: micodebug on/off
    monitor: monitor {1~13|15|99}
    mpucfg: <rnr> <rbar> <rlar>
    mpuclr: <rnr>
    mpudump: dump mpu config
    mqttali: ali mqtt test
    mqttsend: mqttsend [topic] [msg]
    net: net {sta/ap} ... - wifi net config
    osinfo: show os runtime information
    otp_test: otp_test {read}
    pcm_master_test: pcm_master_test {start|stop}
    pcm_slave_test: pcm_slave_test {start|stop}
    per_packet_info: per_packet_info [per_packet_info_output_bitmap(base 16)]
    ping: ping <ip>
    pkt_dbg: packet debug config
    pm: pm [sleep_mode] [wake_source] [vote1] [vote2] [vote3] [param1] [param2] [param3]
    pm_ana: pm_ana [1/0]
    pm_auto_vote: pm_auto_vote [auto_vote_value]
    pm_boot_cp1: pm_boot_cp1 [module_name] [ctrl_state:0x0:bootup; 0x1:shutdowm]
    pm_boot_cp2: pm_boot_cp2 [module_name] [ctrl_state:0x0:bootup; 0x1:shutdowm]
    pm_clk: pm_clk [module_name][clk_state]
    pm_cp1_ctrl: pm_cp1_ctrl [cp1_auto_pw_ctrl]
    pm_ctrl: pm_ctrl [ctrl_value]
    pm_debug: pm_debug [debug_en_value]
    pm_freq: pm_freq [module_name][ frequency]
    pm_gpio: pm_gpio [1/0]
    pm_ldo: pm_ldo[module_name][gpio id][gpio_output_state:0x0->low voltage, 0x1->high voltage]
    pm_lpo: pm_lpo [lpo_type]
    pm_power: pm_power [module_name][ power state]
    pm_psram: pm_psram[module_name][ctrl_state:0x0:power&clk on; 0x1:power&clk off]
    pm_pwr_state: pm_pwr_state [pwr_state]
    pm_rosc: pm_rosc [rosc_accuracy_count_interval]
    pm_rosc_cali: pm_rosc_cali [cali_mode][cal_intval]
    pm_rosc_pin: pm_rosc_pin [lpo_clk:0:ana;1:dig]
    pm_rosc_ppm: pm_rosc_ppm [interval] [count]
    pm_vcore: pm_vcore [value]
    pm_vol: pm_vol [vol_value]
    pm_vote: pm_vote [pm_sleep_mode] [pm_vote] [pm_vote_value] [pm_sleep_time]
    pm_wakeup_source: pm_wakeup_source [pm_sleep_mode]
    ps: ps enable and debug info config
    psram_cache: psram_cache <addr> <size>
    psram_free: psram_free <addr>
    psram_malloc: psram_malloc <length>
    psram_state: psram_state
    psram_task_create: create task on psram
    psram_task_delete: delete task on psram
    psram_test: start|stop
    psram_test_ext: init|byte|word|rewirte|deinit
    puf: puf {version|enrollment|read_uid}
    qspi: qspi {init|write|read}
    qspi_driver: qspi_driver {init|deinit}
    qspi_flash: qspi_flash {write|read}
    rc: wifi rate control config
    reboot: reboot system
    regdump: regdump {module}
    regshow: regshow -w/r addr [value]
    rfcali_cfg_mode: 1:manual, 0:auto
    rfcali_cfg_rate_dist: b g n40 ble (0-31)
    rfcali_cfg_tssi_b: 0-255
    rfcali_cfg_tssi_g: 0-255
    rfcali_show_data: 
    rfconfig: rfconfig bt_polar|bt_btpll|bt_wifipll|wifi_btpll|wifi_wifipll
    rxsens: rxsens [-m] [-d] [-c] [-l]
    scan: scan [ssid]
    sd_card: sd_card {init|deinit|read|write|erase|cmp|}
    sdio: sdio {init|deinit|send_cmd|config_data}
    sdio_host_driver: sdio_host_driver {init|deinit}
    sdmadc: sdmadc_test
    sdtest: sdtest <cmd>
    set_interval: set listen interval}
    setclock: set clock freq, 0: PM_LPO_SRC_DIVD, 1: PM_LPO_SRC_X32K
    setcpufreq: setcpufreq [ckdiv_core] [ckdiv_bus] [ckdiv_cpu0] [ckdiv_cpu1]
    setjtagmode: set jtag mode {cpu0|cpu1|cpu2} {group1|group2}
    setprintport: set log/shell uart port 0/1/2
    spi: spi {init|write|read}
    spi_config: spi_config {id} {mode|baud_rate} [...]
    spi_data_test: spi_data_test {id} {master|slave} {baud_rate|send}[...]
    spi_driver: spi_driver {init|deinit}
    spi_flash: spi_flash {id} {readid|read|write|erase} {addr} {len}[...]
    spi_int: spi_int {id} {reg} {tx|rx}
    sta: sta ssid [password][bssid][channel]
    stackguard: stackguard <override_len>
    start_hidden_softap: start_hidden_softap ssid [password] [channel[1:14]]
    starttype: show start reason type
    state: state - show STA/AP state
    stop: stop {sta|ap}
    tasklist: list tasks
    tempd: tempd [init|deinit|stop|start|update]
    time: system time
    timer: timer {chan} {start|stop|read} [...]
    touch_multi_channel_cyclic_calib_test: touch_multi_channel_cyclic_calib_test {start|stop} {0|1|2|3}
    touch_multi_channel_scan_mode_test: touch_multi_channel_scan_mode_test {start|stop} {0|1|2|3}
    touch_single_channel_calib_mode_test: touch_single_channel_calib_mode_test {0|1|...|15} {0|1|2|3}
    touch_single_channel_manul_mode_test: touch_single_channel_manul_mode_test {0|1|...|15} {calibration_value}
    touch_single_channel_multi_calib_test: touch_single_channel_multi_calib_test {0|1|...|15} {0|1|2|3}
    trace: test trace information
    trng: trng {start|stop|get}
    trng_driver: {init|deinit}
    txevm: txevm [-m] [-c] [-l] [-r] [-w]
    uart: uart {id} {init|deinit|write|read|write_string|dump_statis} [...]
    uart_config: uart_config {id} {baud_rate|data_bits} [...]
    uart_driver: {init|deinit}
    uart_int: uart_int {id} {enable|disable|reg} {tx|rx}
    version
    wdt: wdt {start|stop|feed} [...]
    wdt_driver: {init|deinit}

    $wifi_diag: Wi-Fi HW diagnostics config


- 可以通过bootcore 1 1命令手动启动cpu1:
  
  ::

    #bootcore 1 1
    (102144):reset_cpu1_core at: 021b0000, start=1
    (102152):cpu0 receive the cpu1 boot success event [1]
    cli:I(102152):boot_cpu_core end.
    $wakeup

- 可以通过bootcore 2 1命令手动启动cpu2

- 可以通过cpu1 help的命令格式，输入cpu1的调试命令：

  ::

    #cpu1 version

    $cpu1:cli:I(18046):get_version
    cpu1:cli:I(18046):firmware version : Jan 10 2024 17:18:44
    cpu1:cli:I(18046):chip id : 72360101 
    cpu1:cli:I(18046):soc: bk7258_cp1
    #

Armino平台BK7258系统jtag调试
--------------------------------------

 - JLink环境通过Eclipse集成JLink gdb server + gdb 工具

 - Jlink和BK7258连线::

    1# VTref  ---- VREF
    7# SWDIO  ---- SWDIO
    9# SWCLK  ---- SWCLK
    20# GND   ---- GND

 - JLink软件版本
   https://www.segger.com/downloads/jlink/JLink_Windows_V768_x86_64.exe

 - Arm工具链版本
   https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-win32.exe

 - Eclipse版本
   eclipse-embedcpp-2020-12-R-win32-x86_64.zip

 - Eclipse工程配置

  .. figure:: ../../../_static/bk7236_jlink_config.png
      :align: center
      :alt: BK7258 JLink configuration
      :figclass: align-center

  .. figure:: ../../../_static/bk7236_jlink_config2.png
      :align: center
      :alt: BK7258 JLink configuration
      :figclass: align-center
  
  .. figure:: ../../../_static/bk7236_jlink_config3.png
    :align: center
    :alt: BK7258 JLink configuration
    :figclass: align-center


 - 默认jtag连接cpu0，BK7258有两个Jtag口(grou1/group2)
 - 可以通过setjtagmode cpu0 group1命令设置jtag连接cpu0
 - 可以通过setjtagmode cpu1 group1设置jtag连接cpu1
 - 可以通过jtagmode命令查看当前jtag状态


Armino平台BK7258 异常dump一键恢复现场工具
------------------------------------------------

 - 请参考发布工具中使用文档:
   https://dl.bekencorp.com/tools/Debug_tool/BK7258-debug.zip

 - BK7258 dump工具常见问题:

   + 默认Release版本dump功能是关闭的, 可以通过CONFIG_DUMP_ENABLE配置打开
   + BK7258有3个CM33 cpu, 可以通过三个cpu的config文件修改打开dump功能
   + Dump工具恢复现场的原理是脚本通过分析log,解析出regs,itcm,dtcm,sram内容,然后通过gdb将这些内容恢复到cm33的qemu虚拟机中
   + Log文件的后缀支持txt, log, DAT
   + Log文件的编码当前只支持utf-8, 其他编码格式可用通过notepad++手动转换为utf-8编码格式
   + 如果工具目录下有多份Log, 或者Log中有多次Dump, 工具会分析最后一次Dump, 需要保证工具目录下只有一份Log, 且Log中只有一份dump
   + Dump工具可以自动去掉日志里规则的时间戳: [2024-02-03 14:35:13.375193], 如果遇到不规则的时间戳, 需要手动去除
   + Dump过程中如果出现2次异常, 常见的如检测内存越界时, 遇到Assert, 会多打印一次寄存器, 解析时需要删掉第二次寄存器打印
   + BK7258 任一个cpu Dump都会将当前cpu的寄存器, itcm, dtcm, 以及640k sram全部dump出来 
   + 默认BK7258 cpu0的Log和Dump通过UART0输出
   + 默认cpu1的Log和Dump通过MAILBOX到cpu0再通过UART0输出
   + 默认cpu2的Log和Dump通过UART2输出
   + Dump过程中如果遇到两个cpu同时dump, 需要将Log拆分成两份dump文件,分别用cpu0和cpu1的elf来恢复现场
   + 每个cpu需要当前cpu的寄存器, itcm, dtcm, sram加上elf就可以恢复现场
   
       寄存器格式::

        CPU1 Current regs: =========> CPU1 表示当前寄存器是cpu1出现异常的寄存器
        0 r0 x 0x0
        1 r1 x 0x28061ca0
        2 r2 x 0x0
        3 r3 x 0x8061ca0
        4 r4 x 0x28061d74
        5 r5 x 0x28061d70
        6 r6 x 0x28085a90
        7 r7 x 0x28061de4
        8 r8 x 0x8080808
        9 r9 x 0x9090909
        10 r10 x 0x10101010
        11 r11 x 0x11111111
        12 r12 x 0x1
        14 sp x 0x20000928
        15 lr x 0x21ec909
        16 pc x 0x21ec8fa
        17 xpsr x 0x61000000
        18 msp x 0x2808ff48
        19 psp x 0x20000908
        20 primask x 0x0
        21 basepri x 0x0
        22 faultmask x 0x0
        23 fpscr x 0x0
        30 CPU1 xPSR x 0x4
        31 LR x 0xfffffffd
        32 control x 0xc
        40 MMFAR x 0x8061ca0
        41 BFAR x 0x8061ca0
        42 CFSR x 0x82
        43 HFSR x 0x0
        MemFault              =========> 初步异常原因是内存访问异常

      dtcm格式::

        >>>>stack mem dump begin, stack_top=20000000, stack end=20004000
        <<<<stack mem dump end. stack_top=20000000, stack end=20004000

      itcm格式::

        >>>>stack mem dump begin, stack_top=00000020, stack end=00004000
        <<<<stack mem dump end. stack_top=00000020, stack end=00004000

      sram格式::

        >>>>stack mem dump begin, stack_top=28040000, stack end=28060000
        <<<<stack mem dump end. stack_top=28040000, stack end=28060000

        >>>>stack mem dump begin, stack_top=28060000, stack end=280a0000
        <<<<stack mem dump end. stack_top=28060000, stack end=280a0000

        >>>>stack mem dump begin, stack_top=28000000, stack end=28010000
        <<<<stack mem dump end. stack_top=28000000, stack end=28010000

        >>>>stack mem dump begin, stack_top=28010000, stack end=28020000
        <<<<stack mem dump end. stack_top=28010000, stack end=28020000

        >>>>stack mem dump begin, stack_top=28020000, stack end=28040000
        <<<<stack mem dump end. stack_top=28020000, stack end=28040000

   + 当系统打开CONFIG_MEM_DEBUG时, Dump过程会将当前系统正在使用的Heap内存全部打印出来, 并检查是否有内存越界::

      tick       addr         size   line    func                               task            
      --------   ----------   ----   -----   --------------------------------   ----------------
      6976       0x28064b68   80     425     xQueueGenericCreate                media_ui_task   
      6976       0x28064be0   80     425     xQueueGenericCreate                media_ui_task   
      6976       0x28064c58   160    425     xQueueGenericCreate                media_ui_task   
      6976       0x28064d20   1024   863     xTaskCreate_ex                     media_ui_task   
      6976       0x28065148   104    868     xTaskCreate_ex                     media_ui_task   
      6976       0x2807d098   80     425     xQueueGenericCreate                transfer_major_task
      6976       0x2807d110   80     425     xQueueGenericCreate                transfer_major_task

   + 正常情况下也会将task相关信息dump到日志, 供问题分析时参考
