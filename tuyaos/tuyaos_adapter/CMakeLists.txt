set(incs)
set(srcs)

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
    "src/test"
    "src/misc"
    "src/test/test_lvgl"
    "include/adc"
    "include/bluetooth"
    "include/display"
    "include/flash"
    "include/gpio"
    "include/hci"
    "include/i2c"
    "include/init/include"
    "include/media"
    "include/network"
    "include/pinmux"
    "include/pwm"
    "include/rtc/include"
    "include/security"
    "include/spi/include"
    "include/system"
    "include/timer"
    "include/uart"
    "include/utilities/include"
    "include/wakeup"
    "include/watchdog"
    "include/wifi"
    "include/tal_compatible"
    "include/lvgl"
    "include/wired"
    "include/ipc"
    )

string(REPLACE " " ";" TUYA_HEADER_DIR_LIST $ENV{TUYA_HEADER_DIR})

foreach(head ${TUYA_HEADER_DIR_LIST})
	list(APPEND incs ${head})
endforeach()

if ("${CONFIG_CPU_INDEX}" STREQUAL "0")
    list(APPEND srcs
        src/driver/tkl_lwip.c
        src/driver/tkl_adc.c
        src/driver/tkl_hci.c
        src/driver/tkl_pwm.c
        src/driver/tkl_uart.c
        src/driver/tkl_watchdog.c
        src/driver/tkl_wifi.c
        src/driver/tkl_wakeup.c
        src/driver/encrypt.c
	src/driver/tkl_wired.c
        include/init/src/tkl_init_system.c
        include/init/src/tkl_init_common.c
        include/init/src/tkl_init_network.c
        include/init/src/tkl_init_wifi.c
	include/init/src/tkl_init_wired.c
        include/utilities/src/tuya_hashmap.c
        include/utilities/src/tuya_ringbuf.c
        include/utilities/src/tuya_smartpointer.c
        include/utilities/src/tuya_queue.c
        include/utilities/src/tuya_mem_heap.c
        include/utilities/src/tuya_list.c
        include/utilities/src/tuya_tools.c
    )
endif()

# if ("${CONFIG_CPU_INDEX}" STREQUAL "1")
if (CONFIG_SYS_CPU1)
    list(APPEND srcs
        src/test/test_lvgl/tuya_lvgl_demo.c
        src/test/test_lvgl/stress/lvgl_event_handle.c
        src/test/test_lvgl/stress/lvgl_stress_demo.c
        src/test/test_lvgl/widgets/lv_demo_widgets.c
        src/test/test_lvgl/widgets/assets/img_lvgl_logo.c
        src/test/test_lvgl/widgets/assets/img_clothes.c
        src/test/test_lvgl/widgets/assets/img_demo_widgets_avatar.c
    )
endif()

if (NOT "${CONFIG_CPU_INDEX}" STREQUAL "2")
    list(APPEND srcs
        src/driver/tkl_display.c
        src/driver/tkl_video_enc.c
        src/driver/tkl_video_in.c
        src/driver/tkl_audio.c
        src/driver/tkl_flash.c
        src/driver/tkl_rtc.c
        src/driver/tkl_spi.c
        src/driver/tkl_gpio.c
        src/driver/tkl_i2c.c
        src/driver/tkl_timer.c
        src/driver/tkl_ipc.c
        src/system/tkl_memory.c
        src/system/tkl_mutex.c
        src/system/tkl_network.c
        src/system/tkl_ota.c
        src/system/tkl_output.c
        src/system/tkl_queue.c
        src/system/tkl_semaphore.c
        src/system/tkl_sleep.c
        src/system/tkl_system.c
        src/system/tkl_thread.c
        src/system/tkl_lvgl.c
        src/system/tkl_fs.c
        src/driver/tkl_pinmux.c
        src/misc/bk_adapter.c
        src/misc/qflash.c
        src/misc/sflash.c
        src/misc/fs_init.c
        src/misc/tuya_tkl_ipc.c
    )
endif()

if (CONFIG_TUYA_TEST_CLI)
list(APPEND srcs
    src/test/cli_tuya_test.c
    src/test/test_adc.c
    src/test/test_audio.c
    src/test/test_gpio.c
    src/test/test_lcd.c
    src/test/test_pwm.c
    src/test/test_wifi.c
    src/test/test_startup_frame.c
    src/test/test_media.c
    src/test/test_qspi.c
    src/test/test_littlefs.c
    src/test/test_mp3.c
    src/test/test_timer.c
    )
endif() # CONFIG_TUYA_TEST_CLI

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" REQUIRES bk_common bk_log multimedia bk_usb bk_draw_blend media_service lvgl bk_vfs bk_cli lwip_intf_v2_1)

#add tuya lib
set(tuya_libs "$ENV{TUYA_LIBS}")
string(REPLACE " " ";" tuya_libs "${tuya_libs}")
foreach(lib ${tuya_libs})	
	set(lib_name "lib${lib}.a")
    add_prebuilt_library(${lib_name} "$ENV{TUYA_LIBS_DIR}/${lib_name}")
    target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
endforeach()