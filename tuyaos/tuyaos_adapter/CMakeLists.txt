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
    "include/wired"
    "include/ipc"
    "include/lcd"
)

string(REPLACE " " ";" TUYA_HEADER_DIR_LIST $ENV{TUYA_HEADER_DIR})

foreach(head ${TUYA_HEADER_DIR_LIST})
	list(APPEND incs ${head})
endforeach()

# if (CONFIG_TUYA_LCD)
#     list(APPEND srcs "src/driver/tkl_display.c")
# endif()

#add tuya adapter src
if ("${CONFIG_CPU_INDEX}" STREQUAL "0")
    list(APPEND srcs
        src/driver/tkl_wakeup.c
        src/driver/tkl_lwip_0.c
        src/driver/tkl_wifi_0.c
        src/driver/tkl_hci_0.c
    )
endif()

if (CONFIG_SYS_CPU1)
    list(APPEND srcs
        src/driver/tkl_lwip_1.c
        src/driver/tkl_wifi_1.c
        src/driver/tkl_hci_1.c
        src/system/tkl_fs.c
        src/misc/fs_init.c
        src/misc/dhcpserver.c
        src/driver/tkl_video_enc.c
        src/driver/tkl_video_in.c
        src/driver/tkl_audio.c
        src/system/tkl_ota.c
    )

file(GLOB_RECURSE display_srcs "$ENV{ARMINO_PATH}/../../tuyaos/tuyaos_adapter/src/driver/tkl_display/*.c")
list(APPEND srcs ${display_srcs})

endif() # CONFIG_SYS_CPU1

if (NOT "${CONFIG_CPU_INDEX}" STREQUAL "2")
list(APPEND srcs
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
    src/system/tkl_output.c
    src/system/tkl_queue.c
    src/system/tkl_semaphore.c
    src/system/tkl_sleep.c
    src/system/tkl_system.c
    src/system/tkl_thread.c
    src/driver/tkl_pinmux.c
    src/misc/bk_adapter.c
    src/misc/qflash.c
    src/misc/sflash.c
    src/misc/tuya_tkl_ipc.c
    src/driver/tkl_adc.c
    src/driver/tkl_pwm.c
    src/driver/tkl_uart.c
    src/driver/tkl_watchdog.c
    src/driver/encrypt.c
    src/driver/tkl_wired.c
)

file(GLOB_RECURSE utilities_srcs "$ENV{ARMINO_PATH}/../../tuyaos/tuyaos_adapter/include/utilities/src/*.c")
list(APPEND srcs ${utilities_srcs})

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" REQUIRES bk_common bk_log multimedia bk_usb bk_draw_blend media_service lvgl bk_vfs bk_cli lwip_intf_v2_1)

#add tuya lib
if (CONFIG_SYS_CPU1)
set(tuya_libs "$ENV{TUYA_LIBS}")
string(REPLACE " " ";" tuya_libs "${tuya_libs}")
foreach(lib ${tuya_libs})	
	set(lib_name "lib${lib}.a")
    add_prebuilt_library(${lib_name} "$ENV{TUYA_LIBS_DIR}/${lib_name}")
    target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
endforeach()
endif() # CONFIG_SYS_CPU1