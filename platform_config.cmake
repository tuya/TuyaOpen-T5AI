list_subdirectories(PLATFORM_PUBINC_1 ${PLATFORM_PATH}/tuyaos/tuyaos_adapter)

# Type headers: the platform copy of tuya_cloud_types.h lacks INT_T / VOID_T /
# CHAR_T; put the tuyaopen/include/common copy first.
list(INSERT PLATFORM_PUBINC_1 0 "${CMAKE_CURRENT_LIST_DIR}/tuyaos/tuyaos_adapter/tuyaopen/include/common")

# lwIP headers used by core_network.c

set(PLATFORM_PUBINC_2
    ${PLATFORM_PATH}/t5_os/ap/include
    ${PLATFORM_PATH}/t5_os/build/bk7258/tuya_app/config
    ${PLATFORM_PATH}/t5_os/ap/components/lwip_intf_v2_1/lwip-2.1.2/src/include
)

# Ops contract headers shared by core and the port.
list(APPEND PLATFORM_PUBINC_1 "${TOP_SOURCE_DIR}/tuyaopen-core/port/tuyaopen/interface")

set(PLATFORM_PUBINC
    ${PLATFORM_PUBINC_1}
    ${PLATFORM_PUBINC_2})
