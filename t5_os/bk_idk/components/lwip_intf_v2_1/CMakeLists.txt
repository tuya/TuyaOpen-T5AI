
set(incs)
set(srcs)

list(APPEND incs
    $ENV{TUYA_PROJECT_DIR}/include/base/include
    $ENV{TUYA_PROJECT_DIR}/include/components/tal_lwip/include
    $ENV{TUYA_PROJECT_DIR}/include/components/tal_lwip/include/lwip
    $ENV{TUYA_PROJECT_DIR}/include/components/tal_lwip/include/netif
    $ENV{TUYA_PROJECT_DIR}/include/components/tal_lwip/include/compat
    $ENV{TUYA_PROJECT_DIR}/vendor/T5/tuyaos/tuyaos_adapter/include/lwip
    $ENV{TUYA_PROJECT_DIR}/vendor/T5/tuyaos/tuyaos_adapter/bk_extension/include
    dhcpd
    lwip-2.1.2/port
)

if (CONFIG_LWIP_V2_1)
list(APPEND srcs
    lwip-2.1.2/port/tuya_net.c
    lwip-2.1.2/port/wlanif.c
    lwip-2.1.2/port/ethernetif.c
    )

if (CONFIG_SYS_CPU0)
list(APPEND srcs
    lwip-2.1.2/src/core/ipv4/ip4_addr.c
    lwip-2.1.2/src/core/def.c
    lwip-2.1.2/src/core/pbuf.c
    lwip-2.1.2/port/tuya_sys_arch.c
    lwip-2.1.2/src/core/mem.c
    lwip-2.1.2/src/core/memp.c
    )
endif() # CONFIG_SYS_CPU0

else()

if (CONFIG_SYS_CPU1)
list(APPEND srcs
    dhcpd/dhcp-server.c
    dhcpd/dhcp-server-main.c
    lwip-2.1.2/src/core/disable_wifi.c
    )
endif() # CONFIG_SYS_CPU1

endif() # CONFIG_LWIP_V2_1

armino_component_register(
    SRCS "${srcs}"
    INCLUDE_DIRS "${incs}"
    REQUIRES driver
    PRIV_REQUIRES bk_common bk_wifi bk_rtos os_source tuyaos_adapter
)
