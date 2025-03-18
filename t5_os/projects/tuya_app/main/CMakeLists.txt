set(incs . include)
set(srcs app_main.c)

list(APPEND incs
    $ENV{ARMINO_PATH}/../../../../adapter/utilities/include
    $ENV{ARMINO_PATH}/../../../../include/base/include
    $ENV{ARMINO_PATH}/../../../../include/base/include/tal_lwip/include
)



set(priv_req bk_init bt media_service lwip_intf_v2_1)

if ("${ARMINO_SOC}" STREQUAL "bk7258")
list(APPEND priv_req multimedia)
endif()

if (CONFIG_OVERRIDE_FLASH_PARTITION)
	list(APPEND srcs
		vendor_flash.c
	)
endif()

armino_component_register(
	SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
    REQUIRES tuyaos_adapter
	PRIV_REQUIRES "${priv_req}" lwip_intf_v2_1 multimedia
)
