set(incs)
set(srcs)

if (CONFIG_AT_CMD)

if (NOT CONFIG_BTDM_CONTROLLER_ONLY)
	list(APPEND incs
		ble_boarding
		)

	list(APPEND srcs
		ble_boarding/ble_boarding.c
		ble_boarding/dm_ble_boarding.c
		)
endif()
endif()

if (CONFIG_AT)
if (NOT CONFIG_BTDM_CONTROLLER_ONLY)
	list(APPEND incs
		ble_boarding
		)

	list(APPEND srcs
		ble_boarding/ble_boarding.c
		ble_boarding/dm_ble_boarding.c
		)
endif()
endif()

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES bk_common at bk_bluetooth mbedtls
	)
