set(incs)
set(priv_incs)
set(srcs)

if (CONFIG_PSA_MBEDTLS)
include(sw.cmake)
include(hw.cmake)
include(tfm.cmake)
include(cust.cmake)

list(APPEND incs
	# include te/cust/port dir first
	${hw_incs}
	${cust_incs}
	mbedtls_port/inc
	mbedtls_port/mbedtls/include
	mbedtls_port/mbedtls/library
	mbedtls_port/configs
	mbedtls/include
)

list(APPEND priv_incs
	mbedtls/library
)

list(APPEND srcs
	${sw_srcs}
	${hw_srcs}
	${tfm_srcs}
	${cust_srcs}

	mbedtls_port/src/tls_hardware.c
	mbedtls_port/src/tls_mem.c
	mbedtls_port/mbedtls/library/net_sockets.c
	mbedtls_port/mbedtls/library/platform.c

	# src_x509
	mbedtls/library/pkcs7.c
	mbedtls/library/x509.c
	mbedtls/library/x509_create.c
	mbedtls/library/x509_crl.c
	mbedtls/library/x509_crt.c
	mbedtls/library/x509_csr.c
	mbedtls/library/x509write.c
	mbedtls/library/x509write_crt.c
	mbedtls/library/x509write_csr.c

	# src_tls
	mbedtls/library/debug.c
	mbedtls/library/mps_reader.c
	mbedtls/library/mps_trace.c
	mbedtls/library/ssl_cache.c
	mbedtls/library/ssl_ciphersuites.c
	mbedtls/library/ssl_client.c
	mbedtls/library/ssl_cookie.c
	mbedtls/library/ssl_debug_helpers_generated.c
	mbedtls/library/ssl_msg.c
	mbedtls/library/ssl_ticket.c
	mbedtls/library/ssl_tls.c
	mbedtls/library/ssl_tls12_client.c
	mbedtls/library/ssl_tls12_server.c
	mbedtls/library/ssl_tls13_keys.c
	mbedtls/library/ssl_tls13_server.c
	mbedtls/library/ssl_tls13_client.c
	mbedtls/library/ssl_tls13_generic.c

	# src_crypto
	mbedtls/library/aes.c
	mbedtls/library/aesni.c
	mbedtls/library/aria.c
	mbedtls/library/asn1parse.c
	mbedtls/library/asn1write.c
	mbedtls/library/base64.c
	mbedtls/library/bignum_core.c
	mbedtls/library/camellia.c
	mbedtls/library/ccm.c
	mbedtls/library/chacha20.c
	mbedtls/library/chachapoly.c
	mbedtls/library/constant_time.c
	mbedtls/library/des.c
	mbedtls/library/ecjpake.c
	mbedtls/library/ecp_curves_new.c
	mbedtls/library/entropy.c
	mbedtls/library/error.c
	mbedtls/library/gcm.c
	mbedtls/library/hmac_drbg.c
	mbedtls/library/md5.c
	mbedtls/library/nist_kw.c
	mbedtls/library/ripemd160.c
	mbedtls/library/rsa_alt_helpers.c
	mbedtls/library/sha1.c
	mbedtls/library/sha256.c
	mbedtls/library/sha512.c
	mbedtls/library/sha3.c
	mbedtls/library/pem.c
	mbedtls/library/pk.c
	mbedtls/library/pk_wrap.c
	mbedtls/library/pkcs12.c
	mbedtls/library/pkcs5.c
	mbedtls/library/pkwrite.c
	mbedtls/library/platform_util.c
	mbedtls/library/poly1305.c
	mbedtls/library/psa_util.c
	mbedtls/library/cmac.c
	mbedtls/library/psa_crypto_cipher.c
	mbedtls/library/psa_crypto_slot_management.c
	mbedtls/library/psa_crypto_rsa.c
	mbedtls/library/psa_crypto_ffdh.c
	mbedtls/library/psa_crypto_ecp.c
	mbedtls/library/psa_crypto_driver_wrappers_no_static.c
	mbedtls/library/psa_crypto_hash.c
	mbedtls/library/psa_crypto_aead.c
	mbedtls/library/psa_crypto_mac.c
	mbedtls/library/psa_crypto_pake.c
	mbedtls/library/psa_crypto_se.c
	mbedtls/library/psa_crypto_storage.c	
)

if (CONFIG_PSA_MBEDTLS_TEST)
	list(APPEND incs
		cli/tests
		mbedtls/tests/src
		mbedtls/tests/include
	)
	
	list(APPEND srcs
		cli/tests/aes_cbc.c
		cli/tests/aes_gcm.c
		cli/tests/cli_crypto.c
		cli/tests/ecdh.c
		cli/tests/ecdsa.c
		cli/tests/hmac.c
		cli/tests/psa_tls_client.c
		cli/tests/sha256.c
		cli/tests/sha384.c
		cli/tests/key_manage.c
		cli/tests/platform_misc.c
		mbedtls/tests/src/certs.c
		mbedtls_port/src/tls_connect.c
	)
endif()
endif()

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_INCLUDE_DIRS "${priv_incs}"
	PRIV_REQUIRES bk_common tfm lwip_intf_v2_1
)

if (CONFIG_PSA_MBEDTLS)
set(PSA_MBEDTLS_CONFIG_FILE_PATH  "${COMPONENT_DIR}/mbedtls_port/configs/mbedtls_psa_crypto_config.h")
target_compile_definitions(${COMPONENT_LIB}
	PUBLIC
		MBEDTLS_CONFIG_FILE="${PSA_MBEDTLS_CONFIG_FILE_PATH}"
		${hw_defs}
)
endif()
