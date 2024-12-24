set(incs include include/bk_private)
set(srcs)

if (CONFIG_OTA_HASH_FUNCTION)
	list(APPEND srcs ota_hash.c)
endif()

if (CONFIG_SECURITY_OTA)
	list(APPEND srcs security_ota.c)
	list(APPEND srcs CheckSumUtils.c)

	if (CONFIG_OTA_HTTPS)
		list(APPEND srcs security_ota_https.c)
	endif()

else()
	if (CONFIG_OTA_HTTPS)
		list(APPEND srcs ota_https.c)
	endif()

	if (CONFIG_OTA_HTTP)
		list(APPEND srcs ota.c)
	endif()
endif()

if (CONFIG_OTA_TEST)
	list(APPEND srcs cli_ota.c)
endif()

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES bk_common bk_netif bk_ps fatfs bk_init at bk_wifi compal http bk_https mbedtls tfm psa_mbedtls
	)
