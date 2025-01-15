// Copyright 2024-2025 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <stdlib.h>
#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include "crypto_test.h"
#include <modules/pm.h>

#define APP_SUCCESS		(0)
#define APP_ERROR		(-1)
#define APP_SUCCESS_MESSAGE "Example finished successfully!"
#define APP_ERROR_MESSAGE "Example exited with error!"

#define CRYPTO_EXAMPLE_HMAC_TEXT_SIZE (100)
#define CRYPTO_EXAMPLE_HMAC_KEY_SIZE (32)

#define TAG "mbedtls"

/* Below text is used as plaintext for signing/verifiction */
static uint8_t m_plain_text[CRYPTO_EXAMPLE_HMAC_TEXT_SIZE] = {
	"Example string to demonstrate basic usage of HMAC signing/verification."
};

static uint8_t hmac[CRYPTO_EXAMPLE_HMAC_KEY_SIZE];
static psa_key_id_t key_id;
static uint8_t *s_plain_text_p = 0;

static int test_init(void)
{
	s_plain_text_p = os_malloc(8192);

	if (!s_plain_text_p) {
		BK_LOGE(TAG, "Failed to alloc memory...\r\n");
		return -1;
	}

	os_memset(s_plain_text_p, 0xaa, 8192);

	return 0;
}

static void test_deinit(void)
{
	if (s_plain_text_p) {
		os_free(s_plain_text_p);
		s_plain_text_p = NULL;
	}
}

static int crypto_init(void)
{
	psa_status_t status;

	/* Initialize PSA Crypto */
	status = psa_crypto_init();
	if (status != PSA_SUCCESS)
		return APP_ERROR;

	return APP_SUCCESS;
}

static int crypto_finish(void)
{
	psa_status_t status;

	/* Destroy the key handle */
	status = psa_destroy_key(key_id);
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_destroy_key failed! (Error: %d)\r\n", status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

int generate_key(uint32_t key_len)
{
	psa_status_t status;

	BK_LOGI(TAG, "Generating random HMAC key...\r\n");

	/* Configure the key attributes */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_usage_flags(&key_attributes,
				PSA_KEY_USAGE_VERIFY_HASH | PSA_KEY_USAGE_SIGN_HASH);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_HMAC(PSA_ALG_SHA_256));
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_HMAC);
	psa_set_key_bits(&key_attributes, key_len);

	/* Generate a random key. The key is not exposed to the application,
	 * we can use it to encrypt/decrypt using the key handle
	 */
	status = psa_generate_key(&key_attributes, &key_id);
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_generate_key failed! (Error: %d)\r\n", status);
		return APP_ERROR;
	}

	/* After the key handle is acquired the attributes are not needed */
	psa_reset_key_attributes(&key_attributes);

	BK_LOGI(TAG, "HMAC key generated successfully!\r\n");

	return APP_SUCCESS;
}

int hmac_sign(void)
{
	uint32_t olen;
	psa_status_t status;
	psa_mac_operation_t operation = PSA_MAC_OPERATION_INIT;

	BK_LOGI(TAG, "Signing using HMAC ...\r\n");

	/* Initialize the HMAC signing operation */
	status = psa_mac_sign_setup(&operation, key_id, PSA_ALG_HMAC(PSA_ALG_SHA_256));
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_mac_sign_setup failed! (Error: %d)\r\n", status);
		return APP_ERROR;
	}

	/* Perform the HMAC signing */
	status = psa_mac_update(&operation, m_plain_text, sizeof(m_plain_text));
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_mac_update failed! (Error: %d)\r\n", status);
		return APP_ERROR;
	}

	/* Finalize the HMAC signing */
	status = psa_mac_sign_finish(&operation, hmac, sizeof(hmac), (size_t *)&olen);
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_mac_sign_finish failed! (Error: %d)\r\n", status);
		return APP_ERROR;
	}

	BK_LOGI(TAG, "Signing successful!\r\n");

	return APP_SUCCESS;
}

int hmac_verify(void)
{
	psa_status_t status;
	psa_mac_operation_t operation = PSA_MAC_OPERATION_INIT;

	BK_LOGI(TAG, "Verifying the HMAC signature...\r\n");

	/* Initialize the HMAC verification operation */
	status = psa_mac_verify_setup(&operation, key_id, PSA_ALG_HMAC(PSA_ALG_SHA_256));
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_mac_verify_setup failed! (Error: %d)\r\n", status);
		return APP_ERROR;
	}

	/* Perform the HMAC verification */
	status = psa_mac_update(&operation, m_plain_text, sizeof(m_plain_text));
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_mac_update failed! (Error: %d)\r\n", status);
		return APP_ERROR;
	}

	/* Finalize the HMAC verification */
	status = psa_mac_verify_finish(&operation, hmac, sizeof(hmac));
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_mac_verify_finish failed! (Error: %d)\r\n", status);
		return APP_ERROR;
	}

	BK_LOGI(TAG, "HMAC verified successfully!\r\n");

	return APP_SUCCESS;
}

int hmac_main(void)
{
	int status;

	BK_LOGI(TAG, "Starting HMAC example...\r\n");

	status = crypto_init();
	if (status != APP_SUCCESS) {
		BK_LOGI(TAG, APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = generate_key(256);
	if (status != APP_SUCCESS) {
		BK_LOGI(TAG, APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = hmac_sign();
	if (status != APP_SUCCESS) {
		BK_LOGI(TAG, APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = hmac_verify();
	if (status != APP_SUCCESS) {
		BK_LOGI(TAG, APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = crypto_finish();
	if (status != APP_SUCCESS) {
		BK_LOGI(TAG, APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	BK_LOGI(TAG, APP_SUCCESS_MESSAGE);

	return APP_SUCCESS;
}

int hmac_sign_perf(uint32_t key_len, uint32_t data_len)
{
	uint32_t olen;
	psa_status_t status;
	psa_mac_operation_t operation = PSA_MAC_OPERATION_INIT;
	uint64_t start, end;

	BK_LOGI(TAG, "Signing using HMAC ...\r\n");
	crypto_lock();
	start = crypto_get_time();
	
	/* Initialize the HMAC signing operation */
	status = psa_mac_sign_setup(&operation, key_id, PSA_ALG_HMAC(PSA_ALG_SHA_256));
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_mac_sign_setup failed! (Error: %d)\r\n", status);
		goto _error;
	}

	/* Perform the HMAC signing */
	status = psa_mac_update(&operation, s_plain_text_p, data_len);
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_mac_update failed! (Error: %d)\r\n", status);
		goto _error;
	}

	/* Finalize the HMAC signing */
	status = psa_mac_sign_finish(&operation, hmac, sizeof(hmac), (size_t *)&olen);
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_mac_sign_finish failed! (Error: %d)\r\n", status);
		goto _error;
	}

	BK_LOGI(TAG, "Signing successful!\r\n");
	end = crypto_get_time();
	crypto_unlock();
	crypto_perf_log("HMAC_SIGN", "120M", key_len, data_len, end - start);
	return APP_SUCCESS;

_error:
	crypto_unlock();
	return APP_ERROR;
}

int hmac_verify_perf(uint32_t key_len, uint32_t data_len)
{
	uint64_t start, end;
	psa_status_t status;
	psa_mac_operation_t operation = PSA_MAC_OPERATION_INIT;

	BK_LOGI(TAG, "Verifying the HMAC signature...\r\n");

	crypto_lock();
	start = crypto_get_time();

	/* Initialize the HMAC verification operation */
	status = psa_mac_verify_setup(&operation, key_id, PSA_ALG_HMAC(PSA_ALG_SHA_256));
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_mac_verify_setup failed! (Error: %d)\r\n", status);
		goto _error;
	}

	/* Perform the HMAC verification */
	status = psa_mac_update(&operation, s_plain_text_p, data_len);
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_mac_update failed! (Error: %d)\r\n", status);
		goto _error;
	}

	/* Finalize the HMAC verification */
	status = psa_mac_verify_finish(&operation, hmac, sizeof(hmac));
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_mac_verify_finish failed! (Error: %d)\r\n", status);
		goto _error;
	}

	end = crypto_get_time();
	crypto_unlock();
	crypto_perf_log("HMAC_VERIFY", "120M", key_len, data_len, end - start);

	BK_LOGI(TAG, "HMAC verified successfully!\r\n");

	return APP_SUCCESS;

_error:
	crypto_unlock();
	return APP_ERROR;
}

int hmac_perf_main(void)
{
	uint32_t key_len_list[] = {128, 192, 256};
	uint32_t data_len_list[] = {256, 512, 1024, 2048, 4096};
	uint32_t cpu_freq_list[] = {PM_CPU_FRQ_120M, PM_CPU_FRQ_240M};
	uint32_t cpu;
	uint32_t key;
	uint32_t data;
	int status;

	BK_LOGI(TAG, "HMAC perf test\r\n");

	if (test_init() != 0) {
		goto _error;
	}

	status = crypto_init();
	if (status != APP_SUCCESS) {
		goto _error;
	}

	for (key = 0; key < sizeof(key_len_list)/sizeof(uint32_t); key++) {
		for (data = 0; data < sizeof(data_len_list)/sizeof(uint32_t); data++) {
			for (cpu = 0; cpu < sizeof(cpu_freq_list)/sizeof(uint32_t); cpu++) {
				crypto_set_cpu_freq(cpu_freq_list[cpu]);
				status = generate_key(key_len_list[key]);
				if (status != APP_SUCCESS) {
					goto _error;
				}
			
				status = hmac_sign_perf(key_len_list[key], data_len_list[data]);
				if (status != APP_SUCCESS) {
					goto _error;
				}
			
				status = hmac_verify_perf(key_len_list[key], data_len_list[data]);
				if (status != APP_SUCCESS) {
					goto _error;
				}
			
				status = crypto_finish();
				if (status != APP_SUCCESS) {
					goto _error;
				}
			}
		}
	}

	BK_LOGI(TAG, APP_SUCCESS_MESSAGE);
	BK_LOGI(TAG, "HMAC perf test end\r\n");
	test_deinit();
	return APP_SUCCESS;

_error:
	test_deinit();
	BK_LOGI(TAG, APP_ERROR_MESSAGE);
	return APP_ERROR;
}
