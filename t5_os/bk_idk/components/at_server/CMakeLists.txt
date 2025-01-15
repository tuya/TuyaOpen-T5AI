set(incs "")
set(srcs "")

if (CONFIG_AT)
	list(APPEND incs
		.
		_at_server
		_at_server_port
		at_server_func
		atsvr_cmd
		)
set(bk_inc ../../include)
list(APPEND incs
		${bk_inc}/os
	)
set(components ../../components)
list(APPEND incs
		${components}/bk_cli
	)
if(CONFIG_WIFI_AT_ENABLE)
list(APPEND incs
		${components}/bk_wifi/include
		${components}/bk_wifi/include/bk_private
		${components}/bk_wifi
	)
endif()
if(CONFIG_BT_AT_ENABLE)
list(APPEND incs
		${components}/bk_bluetooth/include

	)
endif()
if(CONFIG_BLE_AT_ENABLE)
list(APPEND incs
		${components}/bk_bluetooth/include

	)
endif()
if(CONFIG_NETIF_AT_ENABLE)
list(APPEND incs
		${components}/bk_netif/include

	)
endif()
if(CONFIG_PHY_AT_ENABLE)
list(APPEND incs
		${components}/bk_phy/include

	)
endif()
if(CONFIG_PM_AT_ENABLE)
list(APPEND incs
		${components}/bk_pm

	)
endif()
if(CONFIG_HTTPS_CLIENT_AT_ENABLE)
list(APPEND incs
		${components}/webclient

	)
endif()
if(CONFIG_MQTT_CLIENT_AT_ENABLE)
list(APPEND incs
		${components}/ali_mqtt/src

	)
endif()
if(CONFIG_WEBSOCKET_CLIENT_AT_ENABLE)
list(APPEND incs
		${components}/bk_websocket/include

	)
endif()
if(CONFIG_MISC_AT_ENABLE)

list(APPEND incs
		${components}/bk_system
	)
endif()

if(CONFIG_AT_DATA_MODE)
set(sdk_components ../../../components)
list(APPEND incs
		${components}/at/inc
	)
endif()

	list(APPEND srcs
		"_at_server_port/atsvr_core.c"
		"_at_server_port/atsvr_port.c"
		"_at_server/_at_server.c"
		"at_server_func/_atsvr_func.c"
		"at_server.c"
		"at_task.c"
		)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)
