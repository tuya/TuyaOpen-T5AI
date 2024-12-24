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
#include <string.h>
#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include <modules/pm.h>
#include "crypto_test.h"

#define APP_SUCCESS             (0)
#define APP_ERROR               (-1)
#define APP_SUCCESS_MESSAGE "Example finished successfully!"
#define APP_ERROR_MESSAGE "Example exited with error!"

#define TAG "mbedtls"

#define CRYPTO_EXAMPLE_AES_MAX_TEXT_SIZE (100)
#define CRYPTO_EXAMPLE_AES_BLOCK_SIZE (16)
#define CRYPTO_EXAMPLE_AES_IV_SIZE (12)
#define CRYPTO_EXAMPLE_AES_ADDITIONAL_SIZE (35)
#define CRYPTO_EXAMPLE_AES_GCM_TAG_LENGTH (16)

/* AES sample IV, DO NOT USE IN PRODUCTION */
static uint8_t m_iv[CRYPTO_EXAMPLE_AES_IV_SIZE];

/* Below text is used as plaintext for encryption/decryption */
static uint8_t m_plain_text[CRYPTO_EXAMPLE_AES_MAX_TEXT_SIZE] = {
	"Example string to demonstrate basic usage of AES GCM mode."
};

/* Below text is used as additional data for authentication */
static uint8_t m_additional_data[CRYPTO_EXAMPLE_AES_ADDITIONAL_SIZE] = {
	"Example string of additional data"
};

static uint8_t *s_plain_text_p = 0;
static uint8_t *s_encrypted_text_p = 0;
static uint8_t *s_decrypted_text_p = 0;

static uint8_t m_encrypted_text[CRYPTO_EXAMPLE_AES_MAX_TEXT_SIZE + CRYPTO_EXAMPLE_AES_GCM_TAG_LENGTH];
static uint8_t m_decrypted_text[CRYPTO_EXAMPLE_AES_MAX_TEXT_SIZE];
static psa_key_id_t key_id;

static int test_init(void)
{
	s_plain_text_p = os_malloc(8192);
	s_encrypted_text_p = os_malloc(8192);
	s_decrypted_text_p = os_malloc(8192 + 32);

	if (!s_plain_text_p || !s_encrypted_text_p || !s_decrypted_text_p) {
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

	if (s_encrypted_text_p) {
		os_free(s_encrypted_text_p);
		s_encrypted_text_p = NULL;
	}

	if (s_decrypted_text_p) {
		os_free(s_decrypted_text_p);
		s_decrypted_text_p = NULL;
	}
}

static int crypto_init(void)
{
	psa_status_t status;

	/* Initialize PSA Crypto */
	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		return APP_ERROR;
	}

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

static int generate_key(uint32_t key_len)
{
	psa_status_t status;

	BK_LOGI(TAG, "Generating random AES key...\r\n");

	/* Configure the key attributes */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_GCM);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_AES);
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

	BK_LOGI(TAG, "AES key generated successfully!\r\n");

	return 0;
}

static int encrypt_aes_gcm(void)
{
	uint32_t output_len;
	psa_status_t status;

	BK_LOGI(TAG, "Encrypting using AES GCM MODE...\r\n");

	/* Generate a random IV */
	status = psa_generate_random(m_iv, CRYPTO_EXAMPLE_AES_IV_SIZE);
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_generate_random failed! (Error: %d)\r\n", status);
		return APP_ERROR;
	}

	/* Encrypt the plaintext and create the authentication tag */
	status = psa_aead_encrypt(key_id,
				  PSA_ALG_GCM,
				  m_iv,
				  sizeof(m_iv),
				  m_additional_data,
				  sizeof(m_additional_data),
				  m_plain_text,
				  sizeof(m_plain_text),
				  m_encrypted_text,
				  sizeof(m_encrypted_text),
				  (size_t *)&output_len);
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_aead_encrypt failed! (Error: %d)\r\n", status);
		return APP_ERROR;
	}

	BK_LOGI(TAG, "Encryption successful!\r\n");

	return APP_SUCCESS;
}

static int decrypt_aes_gcm(void)
{
	uint32_t output_len;
	psa_status_t status;

	BK_LOGI(TAG, "Decrypting using AES GCM MODE...\r\n");

	/* Decrypt and authenticate the encrypted data */
	status = psa_aead_decrypt(key_id,
				  PSA_ALG_GCM,
				  m_iv,
				  sizeof(m_iv),
				  m_additional_data,
				  sizeof(m_additional_data),
				  m_encrypted_text,
				  sizeof(m_encrypted_text),
				  m_decrypted_text,
				  sizeof(m_decrypted_text),
				  (size_t *)&output_len);
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_aead_decrypt failed! (Error: %d)\r\n", status);
		return APP_ERROR;
	}

	/* Check the validity of the decryption */
	if (memcmp(m_decrypted_text, m_plain_text, CRYPTO_EXAMPLE_AES_MAX_TEXT_SIZE) != 0) {
		BK_LOGI(TAG, "Error: Decrypted text doesn't match the plaintext\r\n");
		return APP_ERROR;
	}

	BK_LOGI(TAG, "Decryption and authentication successful!\r\n");

	return APP_SUCCESS;
}

int aes_gcm_main(void)
{
	int status;

	BK_LOGI(TAG, "Starting AES-GCM example...\r\n");

	status = crypto_init();
	if (status != APP_SUCCESS) {
		BK_LOGI(TAG, APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = generate_key(128);
	if (status != APP_SUCCESS) {
		BK_LOGI(TAG, APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = encrypt_aes_gcm();
	if (status != APP_SUCCESS) {
		BK_LOGI(TAG, APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = decrypt_aes_gcm();
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

static int encrypt_aes_gcm_perf(uint32_t key_len, uint32_t data_len)
{
	uint32_t output_len;
	psa_status_t status;
	uint32_t encrypted_len = data_len + 16;
	uint64_t start, end;

	BK_LOGI(TAG, "Encrypting using AES GCM MODE...\r\n");

	crypto_lock();
	start = crypto_get_time();
	/* Generate a random IV */
	status = psa_generate_random(m_iv, CRYPTO_EXAMPLE_AES_IV_SIZE);
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_generate_random failed! (Error: %d)\r\n", status);
		goto _error;
	}

	/* Encrypt the plaintext and create the authentication tag */
	status = psa_aead_encrypt(key_id,
				  PSA_ALG_GCM,
				  m_iv,
				  sizeof(m_iv),
				  m_additional_data,
				  sizeof(m_additional_data),
				  s_plain_text_p,
				  data_len,
				  s_encrypted_text_p,
				  encrypted_len,
				  (size_t *)&output_len);
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_aead_encrypt failed! (Error: %d)\r\n", status);
		goto _error;
	}
	end = crypto_get_time();
	crypto_unlock();
	crypto_perf_log("AES_GCM_ENC", "120M", key_len, data_len, end - start);

	BK_LOGI(TAG, "Encryption successful!\r\n");

	return APP_SUCCESS;

_error:
	crypto_unlock();
	return APP_ERROR;
}

static int decrypt_aes_gcm_perf(uint32_t key_len, uint32_t data_len)
{
	uint32_t output_len;
	psa_status_t status;
	uint32_t encrypted_len = data_len + 16;
	uint32_t decrypted_len = data_len;
	uint64_t start, end;

	BK_LOGI(TAG, "Decrypting using AES GCM MODE...\r\n");

	crypto_lock();
	start = crypto_get_time();
	/* Decrypt and authenticate the encrypted data */
	status = psa_aead_decrypt(key_id,
				  PSA_ALG_GCM,
				  m_iv,
				  sizeof(m_iv),
				  m_additional_data,
				  sizeof(m_additional_data),
				  s_encrypted_text_p,
				  encrypted_len,
				  s_decrypted_text_p,
				  decrypted_len,
				  (size_t *)&output_len);
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_aead_decrypt failed! (Error: %d)\r\n", status);
		goto _error;
	}

	end = crypto_get_time();
	crypto_unlock();
	crypto_perf_log("AES_GCM_DEC", "120M", key_len, data_len, end - start);


	/* Check the validity of the decryption */
	if (memcmp(s_decrypted_text_p, s_plain_text_p, data_len) != 0) {
		BK_LOGI(TAG, "Error: Decrypted text doesn't match the plaintext\r\n");
		goto _error;
	}

	BK_LOGI(TAG, "Decryption and authentication successful!\r\n");

	return APP_SUCCESS;

_error:
	crypto_unlock();
	return APP_ERROR;

}

int aes_gcm_perf_main(void)
{
	uint32_t key_len_list[] = {128, 192, 256};
	uint32_t data_len_list[] = {256, 512, 1024, 2048, 4096};
	uint32_t cpu_freq_list[] = {PM_CPU_FRQ_120M, PM_CPU_FRQ_240M};
	uint32_t key;
	uint32_t data;
	uint32_t cpu;
	int status;

	BK_LOGI(TAG, "AES-GCM perf test\r\n");

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
			
				status = encrypt_aes_gcm_perf(key_len_list[key], data_len_list[data]);
				if (status != APP_SUCCESS) {
					goto _error;
				}
			
				status = decrypt_aes_gcm_perf(key_len_list[key], data_len_list[data]);
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
	BK_LOGI(TAG, "AES-GCM perf test\r\n");
	test_deinit();
	return APP_SUCCESS;

_error:
	test_deinit();
	BK_LOGI(TAG, APP_ERROR_MESSAGE);
	return APP_ERROR;
}
