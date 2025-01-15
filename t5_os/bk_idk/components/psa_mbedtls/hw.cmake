set(hw_srcs)
set(hw_incs)
set(hw_defs)

if (CONFIG_TRUSTENGINE)
	set(TE_DIR ${COMPONENT_DIR}/../bk_trustengine)

	list(APPEND hw_incs
		${TE_DIR}/
		${TE_DIR}/dubhe_alt/inc
		${TE_DIR}/dubhe_driver/inc
		${TE_DIR}/dubhe_driver/inc/crypto
		${TE_DIR}/dubhe_driver/inc/common
		${TE_DIR}/spe/inc
		${TE_DIR}/spe/inc/pal
		${TE_DIR}/spe/inc/hal
	)

	list(APPEND hw_srcs
		${TE_DIR}/spe/src/misc.c

		${TE_DIR}/dubhe_alt/src/aes.c
		${TE_DIR}/dubhe_alt/src/bignum_wrap.c
		${TE_DIR}/dubhe_alt/src/ccm.c
		${TE_DIR}/dubhe_alt/src/cmac.c
		${TE_DIR}/dubhe_alt/src/gcm.c
		${TE_DIR}/dubhe_alt/src/otp.c
		${TE_DIR}/dubhe_alt/src/sha256.c

		${TE_DIR}/dubhe_driver/src/common/dubhe_driver.c
		${TE_DIR}/dubhe_driver/src/common/dubhe_event.c
		${TE_DIR}/dubhe_driver/src/common/dubhe_intr_handler.c
		${TE_DIR}/dubhe_driver/src/common/dubhe_sram_alloc.c
		${TE_DIR}/dubhe_driver/src/crypto/aca/dubhe_aca.c
		${TE_DIR}/dubhe_driver/src/crypto/aca/dubhe_aca_hw.c
		${TE_DIR}/dubhe_driver/src/crypto/aead/dubhe_ccm.c
		${TE_DIR}/dubhe_driver/src/crypto/aead/dubhe_gcm.c
		${TE_DIR}/dubhe_driver/src/crypto/hash/dubhe_hash.c
		${TE_DIR}/dubhe_driver/src/crypto/mac/dubhe_cbcmac.c
		${TE_DIR}/dubhe_driver/src/crypto/mac/dubhe_cmac.c
		${TE_DIR}/dubhe_driver/src/crypto/mac/dubhe_ghash.c
		${TE_DIR}/dubhe_driver/src/crypto/otp/dubhe_otp.c
		${TE_DIR}/dubhe_driver/src/crypto/sca/dubhe_sca.c
		${TE_DIR}/dubhe_driver/src/crypto/trng/dubhe_trng.c
		${TE_DIR}/dubhe_driver/src/hal_pal/pal_heap.c
		${TE_DIR}/dubhe_driver/src/hal_pal/pal_log.c
		${TE_DIR}/dubhe_driver/src/hal_pal/pal_mutex.c
		${TE_DIR}/dubhe_driver/src/hal_pal/pal_preempt.c
		${TE_DIR}/dubhe_driver/src/hal_pal/pal_semaphore.c
		${TE_DIR}/dubhe_driver/src/hal_pal/pal_signal.c
		${TE_DIR}/dubhe_driver/src/hal_pal/pal_string.c
		${TE_DIR}/dubhe_driver/src/hal_pal/pal_time.c

		${TE_DIR}/dubhe_lib/bignum.c
		${TE_DIR}/dubhe_lib/bignum_delta.c
		${TE_DIR}/dubhe_lib/cipher.c
		${TE_DIR}/dubhe_lib/cipher_wrap.c
		${TE_DIR}/dubhe_lib/ctr_drbg.c
		${TE_DIR}/dubhe_lib/dhm.c
		${TE_DIR}/dubhe_lib/ecdh.c
		${TE_DIR}/dubhe_lib/ecdsa.c
		${TE_DIR}/dubhe_lib/ecp.c
		${TE_DIR}/dubhe_lib/ecp_curves.c
		${TE_DIR}/dubhe_lib/entropy_poll.c
		${TE_DIR}/dubhe_lib/md.c
		${TE_DIR}/dubhe_lib/oid.c
		${TE_DIR}/dubhe_lib/pkparse.c
		${TE_DIR}/dubhe_lib/rsa.c
	)

	list(APPEND srcs
		mbedtls_port/mbedtls/programs/test/selftest.c

		mbedtls/library/psa_crypto.c
		mbedtls/library/psa_crypto_client.c
	)

if (CONFIG_TRUSTENGINE_TEST)
	list(APPEND hw_incs cli)

	list(APPEND hw_srcs
		${TE_DIR}/cli/te200_sha_test.c
		${TE_DIR}/cli/te200_aes_test.c
		${TE_DIR}/cli/cli_te200.c
		${TE_DIR}/cli/te200_ecdsa_test.c
		${TE_DIR}/cli/te200_rsa_test.c
	)
endif()

set(hw_defs
	CRYPTO_HW_ACCELERATOR
	CRYPTO_HW_ACCELERATOR_OTP_PROVISIONING
	TEE_M
	ARM_CE_DUBHE
	DUBHE_SECURE
	ARM_CE_DUBHE_SCA
	ARM_CE_DUBHE_TRNG
	ARM_CE_DUBHE_OTP
	ARM_CE_DUBHE_HASH
	ARM_CE_DUBHE_ACA
	DUBHE_FOR_RUNTIME
)
endif()
