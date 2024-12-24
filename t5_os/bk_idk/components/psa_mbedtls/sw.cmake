set(sw_srcs)
set(sw_srcs)
if (CONFIG_SW_CRYPTO)
	list(APPEND sw_srcs
		mbedtls_port/mbedtls/library/ecp_curves.c

		mbedtls/library/bignum.c
		mbedtls/library/cipher.c
		mbedtls/library/cipher_wrap.c
		mbedtls/library/ctr_drbg.c
		mbedtls/library/dhm.c
		mbedtls/library/ecdh.c
		mbedtls/library/ecdsa.c
		mbedtls/library/ecp.c
		mbedtls/library/entropy_poll.c
		mbedtls/library/md.c
		mbedtls/library/oid.c
		mbedtls/library/rsa.c
		mbedtls/library/pkparse.c
		mbedtls/library/psa_crypto.c
		mbedtls/library/psa_crypto_client.c
	)
endif()
