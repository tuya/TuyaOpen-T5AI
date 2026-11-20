list_subdirectories(PLATFORM_PUBINC_1 ${PLATFORM_PATH}/tuyaos/tuyaos_adapter)

# Type headers: the platform copy of tuya_cloud_types.h lacks INT_T / VOID_T /
# CHAR_T; put the tuyaopen/include/common copy first.
list(INSERT PLATFORM_PUBINC_1 0 "${CMAKE_CURRENT_LIST_DIR}/tuyaos/tuyaos_adapter/tuyaopen/include/common")
# Core/P2P compile view: libre embeds struct sockaddr_in6. The T5AI lwIP port
# itself stays on its own (IPv4) configuration; this view is only consumed by
# the root build (tuyaopen-headers), never by the platform lwIP build.
set(PLATFORM_CORE_LWIP_VIEW "${CMAKE_CURRENT_LIST_DIR}/tuyaos/tuyaos_adapter/include/lwip_view")

# lwIP headers used by core_network.c

set(PLATFORM_PUBINC_2
    ${PLATFORM_PATH}/t5_os/build/bk7258/tuya_app/bk7258_ap/config
    ${PLATFORM_PATH}/t5_os/ap/include
    ${PLATFORM_PATH}/t5_os/ap/middleware/driver/include/bk_private
    ${PLATFORM_PATH}/t5_os/build/bk7258/tuya_app/config
    ${PLATFORM_PATH}/t5_os/ap/components/lwip_intf_v2_1/lwip-2.1.2/port
    ${PLATFORM_PATH}/t5_os/ap/components/lwip_intf_v2_1/lwip-2.1.2/src/include
)

# This platform implements the TKL file system API used by core's FS ops card.
add_definitions(-DTUYAOPEN_HAS_TKL_FS=1)

# POSIX-name shims over newlib/lwIP used by the P2P/library sources:
# <netinet/in.h>, <netdb.h>, <sys/un.h>, <netinet/tcp.h>, <arch/sys_arch.h>.
# lwIP's own compat/posix supplies <arpa/inet.h> and <sys/socket.h>.
list(INSERT PLATFORM_PUBINC_1 0
    "${CMAKE_CURRENT_LIST_DIR}/tuyaos/tuyaos_adapter/include/posix_compat")
list(APPEND PLATFORM_PUBINC_2
    "${PLATFORM_PATH}/t5_os/ap/components/lwip_intf_v2_1/lwip-2.1.2/src/include/compat/posix")

set(PLATFORM_PUBINC
    ${PLATFORM_PUBINC_1}
    ${PLATFORM_PUBINC_2})
