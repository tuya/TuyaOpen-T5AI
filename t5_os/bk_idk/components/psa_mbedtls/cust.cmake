set(cust_incs)
set(cust_srcs)

if (CONFIG_PSA_CUSTOMIZATION_TEST)
	list(APPEND cust_incs
		customization/tests
	)
	
	list(APPEND cust_srcs
		# tests
		customization/tests/tz_aes_cbc_test.c
		customization/tests/tz_aes_gcm_test.c
		#customization/tests/tz_ecdh_test.c
		#customization/tests/tz_ecdsa_test.c
		#customization/tests/tz_hmac_test.c
		#customization/tests/tz_psa_tls_client_test.c
		#customization/tests/tz_sha256_test.c
		customization/tests/cli_psa_custom.c
	)

if (CONFIG_TFM_PS)
	list(APPEND cust_srcs
		customization/tests/tz_ps_test.c
	)
endif()

if (CONFIG_TFM_DUBHE_KEY_LADDER_NSC)
	list(APPEND cust_srcs
		customization/tests/tz_key_ladder_test.c
	)
endif()
endif()

if (CONFIG_PSA_CUSTOMIZATION_TZ_L)
	list(APPEND cust_incs
		customization/tz/l
	)
	
	list(APPEND cust_srcs
		#customization
		customization/tz/l/tz_mbedtls.c
		customization/tz/l/hal_tz.c
	)

if (CONFIG_TFM_PS)
	list(APPEND cust_srcs
		customization/tz/l/tz_ps.c
	)
endif()
endif()


