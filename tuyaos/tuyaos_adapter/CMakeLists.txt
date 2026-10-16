set(incs)
set(srcs)

# Read TuyaOpen SDK configuration without overwriting platform settings.
# Export additional SDK options here as platform integrations need them.
function(tuyaopen_read_config)
    set(CONFIG_ENABLE_TUYAOPEN_OPUS OFF)
    if (DEFINED ENV{BUILD_PARAM_DIR}
        AND NOT "$ENV{BUILD_PARAM_DIR}" STREQUAL "")
        include("$ENV{BUILD_PARAM_DIR}/build_param.cmake")
    endif()
    set(CONFIG_ENABLE_TUYAOPEN_OPUS "${CONFIG_ENABLE_TUYAOPEN_OPUS}" PARENT_SCOPE)
endfunction()

macro(FIND_INCLUDE_DIR result curdir)
    file(GLOB_RECURSE children "${curdir}/*.h")
    set(dirlist "")
    foreach(child ${children})
        string(REGEX REPLACE "(.*)/.*" "\\1" DIR_NAME ${child})
        if((IS_DIRECTORY ${DIR_NAME}) AND (NOT (${DIR_NAME} IN_LIST dirlist)))
            list(APPEND dirlist ${DIR_NAME})
        endif()
    endforeach()
    set(${result} ${dirlist})
    #message(STATUS "result=${dirlist}")
endmacro()

#add tuya include dir
set(incs
    "include"
    "src/misc"
    "include/adc"
    "include/atomic"
    "include/bluetooth"
    "include/cellular"
    "include/flash"
    "include/gpio"
    "include/hci"
    "include/i2c"
    "include/i2s"
    "include/init/include"
    "include/media"
    "include/network"
    "include/pinmux"
    "include/pwm"
    "include/rtc/include"
    "include/security"
    "include/spi/include"
    "include/qspi"
    "include/system"
    "include/timer"
    "include/touch"
    "include/uart"
    "include/utilities/include"
    "include/vad"
    "include/vad/lib"
    "include/wakeup"
    "include/watchdog"
    "include/wifi"

    "include/wired"
    "include/ipc"
    "include/kws"
    "include/kws/libtutuClear"
    "include/rgb"
    "include/mcu8080"
    "include/dvp"
    "include/dma2d"
    "include/jpeg_codec"
    "include/audio"
    "tuyaopen"
)

string(REPLACE " " ";" TUYA_HEADER_DIR_LIST $ENV{TUYA_HEADER_DIR})

foreach(head ${TUYA_HEADER_DIR_LIST})
	list(APPEND incs ${head})
endforeach()

set(srcs
    src/system/tkl_memory.c
    src/system/tkl_mutex.c
    src/system/tkl_network.c
    src/system/tkl_output.c
    src/system/tkl_queue.c
    src/system/tkl_semaphore.c
    src/system/tkl_system.c
    src/system/tkl_thread.c
    src/system/tkl_atomic.c
    src/system/tkl_task_notify.c
    src/driver/tkl_watchdog.c
    # src/driver/tkl_ipc.c
    # src/misc/tuya_tkl_ipc.c
    include/init/src/tkl_init_system.c
)

if ("${CONFIG_CPU_INDEX}" STREQUAL "0")
    # list(APPEND srcs src/driver/tkl_hci_0.c)
    if (CONFIG_TUYA_TEST_CLI)
        list(APPEND srcs src/test/cli_tuya_test_0.c)
    endif() # CONFIG_TUYA_TEST_CLI
else()
    list(APPEND srcs
        src/driver/tkl_wifi.c
        src/driver/tkl_wired.c
        # src/driver/tkl_hci_1.c
        src/driver/tkl_hci.c
        src/driver/tkl_video_enc.c
        src/driver/tkl_video_in.c
        src/driver/tkl_cellular.c
        src/driver/tkl_vad.c
        src/driver/tkl_kws.c
        src/driver/tkl_pinmux.c
        src/driver/tkl_rgb.c
        src/driver/tkl_8080.c
        src/driver/tkl_adc.c
        src/driver/tkl_pwm.c
        src/driver/tkl_uart.c
        src/driver/tkl_flash.c
        # src/driver/tkl_otp_flash.c
        tuyaopen/tuyaopen_license.c
        src/driver/tkl_rtc.c
        src/driver/tkl_spi.c
        src/driver/tkl_qspi.c
        src/driver/tkl_gpio.c
        src/driver/tkl_i2c.c
        src/driver/tkl_timer.c
        src/driver/tkl_touch.c
        # src/driver/tkl_wired.c
        # src/driver/tkl_i2s.c
        src/driver/tkl_i2s_v2.c
        src/driver/tkl_dma2d.c
        src/driver/tkl_dvp.c
        src/driver/tkl_jpeg_codec.c

        src/misc/tuya_sdio.c
        src/driver/tkl_audio.c
        src/driver/tkl_aud_adc.c
        src/driver/tkl_aud_dac.c
        src/driver/tkl_aud_dmic.c
        src/misc/encrypt.c
        src/misc/fs_init.c
        src/misc/dhcpserver.c
        src/misc/qflash.c
        src/misc/sflash.c
        src/misc/tkl_aud_pm.c
        src/misc/bk_adapter.c
        src/misc/tuya_dma.c
        # src/misc/tkl_mftest.c
        # src/misc/mftest/mt_audio.c
        # src/misc/mftest/mt_fft.c

        src/system/tkl_fs.c
        src/system/tkl_ota.c
        src/system/tkl_sleep.c
        src/system/ota_head.c

        src/driver/tkl_wakeup.c
        # src/driver/tkl_asr.c
        # src/misc/ty_vad_app.c

        include/init/src/tkl_init_common.c
        include/init/src/tkl_init_network.c
        include/init/src/tkl_init_wifi.c
        include/init/src/tkl_init_wired.c
        include/init/src/tkl_init_cellular.c
        include/utilities/src/tuya_hashmap.c
        include/utilities/src/tuya_ringbuf.c
        include/utilities/src/tuya_smartpointer.c
        include/utilities/src/tuya_queue.c
        include/utilities/src/tuya_mem_heap.c
        include/utilities/src/tuya_list.c
        include/utilities/src/tuya_tools.c
    )

    file(GLOB_RECURSE utilities_srcs "$ENV{ARMINO_PATH}/../../tuyaos/tuyaos_adapter/include/utilities/src/*.c")
    list(APPEND srcs ${utilities_srcs})

    if (CONFIG_TUYA_TEST_CLI)
        list(APPEND incs
            src/test/test_media
            src/test/coremark
            src/test/coremark/posix
        )
        list(APPEND srcs
            # src/test/test_adc.c
            # src/test/test_audio.c
            # src/test/test_lcd.c
            # src/test/test_pwm.c
            # src/test/test_qspi.c
            # src/test/test_mp3.c
            # src/test/test_timer.c
            # src/test/test_usb.c
            # src/test/test_spi.c
            # src/test/test_ringbuffer/rb.c
            # src/test/test_media.c
            # src/test/test_mic.c
            # src/test/test_speaker.c
            # src/test/test_i2s.c
            # src/test/test_mtd.c
            # src/test/test_i2s.c
            # src/test/test_mtd/qspi_flash_GD5F1G.c
            # src/test/test_media/test_media_audio.c
            # src/test/test_media/test_media_camera.c
            # src/test/test_media/test_media_h264.c
            # src/test/test_media/test_media_lcd.c
            # src/test/test_eth.c
            # src/test/test_iperf.c
            # src/test/test_mem.c
            # src/test/test_dwt.c
            # src/test/test_wifi.c
            # src/test/test_dvp/dvp_gc2145.c
            # src/test/test_dvp/test_dvp.c
            # src/test/coremark/core_list_join.c
            # src/test/coremark/core_main.c
            # src/test/coremark/core_matrix.c
            # src/test/coremark/core_state.c
            # src/test/coremark/core_util.c
            # src/test/coremark/posix/core_portme.c
            # src/test/test_sdcard.c
            # src/test/audio_input.c
            src/test/test_common.c
            src/test/test_lp.c
            src/test/test_gpio.c
            src/test/test_littlefs.c
            src/test/test_fs.c
            src/test/cli_tuya_test_1.c
        )
    endif() # CONFIG_TUYA_TEST_CLI

endif() 

if ("$ENV{ARMINO_SOC}" STREQUAL "bk7258")
    set(priv_requires_list bk_common bk_log bk_cli lwip_intf_v2_1 os_source)
elseif ("$ENV{ARMINO_SOC}" STREQUAL "bk7258_ap")
    set(priv_requires_list bk_common bk_system bk_modem bk_log bk_cli lwip_intf_v2_1 os_source lvgl display_service bk_vfs multimedia media_service bk_draw_blend bk_usb audio_record audio_play)
else()
    message(FATAL_ERROR soc:"$ENV{ARMINO_SOC}" is not valid!)
endif() # CONFIG_CPU_INDEX

armino_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS ${incs}
    PRIV_REQUIRES ${priv_requires_list}
)

#add tuya lib
if ("${CONFIG_CPU_INDEX}" STREQUAL "1")

# platform/t5ai/tuyaos/tuyaos_adapter/include/vad/lib/libaudio_subsys.a
target_link_libraries(${COMPONENT_LIB} INTERFACE "$ENV{ARMINO_PATH}/../../tuyaos/tuyaos_adapter/include/vad/lib/libaudio_subsys.a")

tuyaopen_read_config()
# TuyaOpen Opus objects are already included in the SDK libtuyaos.a.
if (NOT CONFIG_ENABLE_TUYAOPEN_OPUS)
    target_link_libraries(${COMPONENT_LIB} INTERFACE
        "$ENV{ARMINO_PATH}/../../tuyaos/tuyaos_adapter/include/opus/libopus.a")
endif()

# platform/t5ai/tuyaos/tuyaos_adapter/include/kws/libtutuClear/libtutuClear_wakeup_nihaotuya_xiaozhitongxue_heytuya_hituya_20251024_small_model.a
target_link_libraries(${COMPONENT_LIB} INTERFACE "$ENV{ARMINO_PATH}/../../tuyaos/tuyaos_adapter/include/kws/libtutuClear/libtutuClear_wakeup_nihaotuya_xiaozhitongxue_heytuya_hituya_20251024_small_model.a")
# platform/t5ai/tuyaos/tuyaos_adapter/src/misc/mftest/libtal_mftest.a
target_link_libraries(${COMPONENT_LIB} INTERFACE "$ENV{ARMINO_PATH}/../../tuyaos/tuyaos_adapter/src/misc/mftest/libtal_mftest.a")

set(tuya_libs "$ENV{TUYA_LIBS}")
string(REPLACE " " ";" tuya_libs "${tuya_libs}")
foreach(lib ${tuya_libs})	
	set(lib_name "lib${lib}.a")
    add_prebuilt_library(${lib_name} "$ENV{TUYA_LIBS_DIR}/${lib_name}")
    target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
endforeach()
endif() # CONFIG_SYS_CPU1