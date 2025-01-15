list_subdirectories(PLATFORM_PUBINC_1 ${PLATFORM_PATH}/tuyaos/tuyaos_adapter)

set(PLATFORM_PUBINC_2 
    ${PLATFORM_PATH}/t5_os/bk_idk/components/lwip_intf_v2_1/lwip-2.1.2/src/include
    ${PLATFORM_PATH}/t5_os/bk_idk/components/lwip_intf_v2_1/lwip-2.1.2/port
    ${PLATFORM_PATH}/t5_os/bk_idk/include
    ${PLATFORM_PATH}/t5_os/build/bk7258/config
    ${PLATFORM_PATH}/t5_os/bk_idk/middleware/driver/include/bk_private
    ${PLATFORM_PATH}/t5_os/bk_idk/components/bk_rtos/include
)

set(PLATFORM_PUBINC 
    ${PLATFORM_PUBINC_1}
    ${PLATFORM_PUBINC_2})