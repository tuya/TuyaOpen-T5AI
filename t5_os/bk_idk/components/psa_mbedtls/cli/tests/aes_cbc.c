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

#include <string.h>
#include <os/os.h>
#include <os/mem.h>
#include "components/log.h"
#include "common/bk_include.h"
#include <modules/pm.h>
#include "psa/crypto.h"
#include "psa/crypto_extra.h"
#include "crypto_test.h"

#define APP_SUCCESS    (0)
#define APP_ERROR      (-1)
#define APP_SUCCESS_MESSAGE "Example finished successfully!"
#define APP_ERROR_MESSAGE "Example exited with error!"

#define TAG "mbedtls"

#define CRYPTO_EXAMPLE_AES_MAX_TEXT_SIZE (64)
#define CRYPTO_EXAMPLE_AES_BLOCK_SIZE (16)

static uint8_t m_iv[CRYPTO_EXAMPLE_AES_BLOCK_SIZE];

/* Below text is used as plaintext for encryption/decryption */
static uint8_t m_plain_text[CRYPTO_EXAMPLE_AES_MAX_TEXT_SIZE] = {
	"Example string to demonstrate basic usage of AES CBC mode."
};

static uint8_t m_encrypted_text[CRYPTO_EXAMPLE_AES_MAX_TEXT_SIZE];
static uint8_t m_decrypted_text[CRYPTO_EXAMPLE_AES_MAX_TEXT_SIZE];

static uint8_t *s_plain_text_p = 0;
static uint8_t *s_encrypted_text_p = 0;
static uint8_t *s_decrypted_text_p = 0;
static uint8_t *s_iv_p = 0;

static psa_key_id_t key_id;

static int test_init(void)
{
	s_plain_text_p = os_malloc(8192);
	s_encrypted_text_p = os_malloc(8192);
	s_decrypted_text_p = os_malloc(8192);
	s_iv_p = os_malloc(32);

	if (!s_plain_text_p || !s_encrypted_text_p || !s_decrypted_text_p || !s_iv_p) {
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

	if (s_iv_p) {
		os_free(s_iv_p);
		s_iv_p = NULL;
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

static int generate_key(uint32_t key_len)
{
	psa_status_t status;

	BK_LOGI(TAG, "Generating random AES key...\r\n");

	/* Configure the key attributes */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_CBC_NO_PADDING);
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

	return APP_SUCCESS;
}

static int encrypt_cbc_aes(void)
{
	uint32_t olen;
	psa_status_t status;
	psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;

	BK_LOGI(TAG, "Encrypting using AES CBC MODE...\r\n");

	/* Setup the encryption operation */
	status = psa_cipher_encrypt_setup(&operation, key_id, PSA_ALG_CBC_NO_PADDING);
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_cipher_encrypt_setup failed! (Error: %d)\r\n", status);
		return APP_ERROR;
	}

	/* Generate an IV */
	status = psa_cipher_generate_iv(&operation, m_iv, sizeof(m_iv), (size_t *)&olen);
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_cipher_generate_iv failed! (Error: %d)\r\n", status);
		return APP_ERROR;
	}

	/* Perform the encryption */
	status = psa_cipher_update(&operation, m_plain_text,
				   sizeof(m_plain_text), m_encrypted_text,
				   sizeof(m_encrypted_text), (size_t *)&olen);
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_cipher_update failed! (Error: %d)\r\n", status);
		return APP_ERROR;
	}

	/* Finalize the encryption */
	status = psa_cipher_finish(&operation, m_encrypted_text + olen,
				   sizeof(m_encrypted_text) - olen,
				   (size_t *)&olen);
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_cipher_finish failed! (Error: %d)\r\n", status);
		return APP_ERROR;
	}

	BK_LOGI(TAG, "Encryption successful!\r\n");

	/* Clean up cipher operation context */
	psa_cipher_abort(&operation);

	return APP_SUCCESS;
}

static int decrypt_cbc_aes(void)
{
	uint32_t olen;
	psa_status_t status;
	psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;

	BK_LOGI(TAG, "Decrypting using AES CBC MODE...\r\n");

	/* Setup the decryption operation */
	status = psa_cipher_decrypt_setup(&operation, key_id, PSA_ALG_CBC_NO_PADDING);
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_cipher_decrypt_setup failed! (Error: %d)\r\n", status);
		return APP_ERROR;
	}

	/* Set the IV generated in encryption */
	status = psa_cipher_set_iv(&operation, m_iv, sizeof(m_iv));
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_cipher_set_iv failed! (Error: %d)\r\n", status);
		return APP_ERROR;
	}

	/* Perform the decryption */
	status = psa_cipher_update(&operation, m_encrypted_text,
				   sizeof(m_encrypted_text), m_decrypted_text,
				   sizeof(m_decrypted_text), (size_t *)&olen);
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_cipher_update failed! (Error: %d)\r\n", status);
		return APP_ERROR;
	}

	/* Finalize the decryption */
	status = psa_cipher_finish(&operation, m_decrypted_text + olen,
				   sizeof(m_decrypted_text) - olen,
				   (size_t *)&olen);
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_cipher_finish failed! (Error: %d)\r\n", status);
		return APP_ERROR;
	}

	/* Check the validity of the decryption */
	if (memcmp(m_decrypted_text,
				m_plain_text,
				CRYPTO_EXAMPLE_AES_MAX_TEXT_SIZE) != 0){

		BK_LOGI(TAG, "Error: Decrypted text doesn't match the plaintext\r\n");
		return APP_ERROR;
	}

	BK_LOGI(TAG, "Decryption successful!\r\n");

	/*  Clean up cipher operation context */
	psa_cipher_abort(&operation);

	return APP_SUCCESS;
}

int aes_cbc_main(void)
{
	int status;

	BK_LOGI(TAG, "Starting AES-CBC-NO-PADDING example...\r\n");

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

	status = encrypt_cbc_aes();
	if (status != APP_SUCCESS) {
		BK_LOGI(TAG, APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = decrypt_cbc_aes();
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

static int encrypt_cbc_aes_perf(uint32_t key_len, uint32_t data_len)
{
	uint32_t olen;
	psa_status_t status;
	psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;
	uint64_t start, end;
	uint32_t iv_len = 16;

	BK_LOGI(TAG, "Encrypting using AES CBC MODE...\r\n");

	crypto_lock();
	start = crypto_get_time();
	/* Setup the encryption operation */
	status = psa_cipher_encrypt_setup(&operation, key_id, PSA_ALG_CBC_NO_PADDING);
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "psa_cipher_encrypt_setup failed! (Error: %d)\r\n", status);
		goto _error;
	}

	/* Generate an IV */
	status = psa_cipher_generate_iv(&operation, s_iv_p, iv_len, (size_t *)&olen);
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "psa_cipher_generate_iv failed! (Error: %d)\r\n", status);
		goto _error;
	}

	/* Perform the encryption */
	status = psa_cipher_update(&operation, s_plain_text_p,
				   data_len, s_encrypted_text_p,
				   data_len, (size_t *)&olen);
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "psa_cipher_update failed! (Error: %d)\r\n", status);
		goto _error;
	}

	/* Finalize the encryption */
	status = psa_cipher_finish(&operation, s_encrypted_text_p + olen,
				   data_len - olen,
				   (size_t *)&olen);
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "psa_cipher_finish failed! (Error: %d)\r\n", status);
		goto _error;
	}
	end = crypto_get_time();
	crypto_unlock();
	crypto_perf_log("AES_CBC_ENC", "120M", key_len, data_len, end - start);

	BK_LOGI(TAG, "Encryption successful!\r\n");

	/* Clean up cipher operation context */
	psa_cipher_abort(&operation);
	return APP_SUCCESS;

_error:
	crypto_unlock();
	return APP_ERROR;
}

static int decrypt_cbc_aes_perf(uint32_t key_len, uint32_t data_len)
{
	uint64_t start, end;
	uint32_t olen;
	psa_status_t status;
	psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;
	uint32_t iv_len = 16;

	BK_LOGD(TAG, "Decrypting using AES CBC MODE...\r\n");

	crypto_lock();
	start = crypto_get_time();

	/* Setup the decryption operation */
	status = psa_cipher_decrypt_setup(&operation, key_id, PSA_ALG_CBC_NO_PADDING);
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "psa_cipher_decrypt_setup failed! (Error: %d)\r\n", status);
		goto _error;
	}

	/* Set the IV generated in encryption */
	status = psa_cipher_set_iv(&operation, s_iv_p, iv_len);
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "psa_cipher_set_iv failed! (Error: %d)\r\n", status);
		goto _error;
	}

	/* Perform the decryption */
	status = psa_cipher_update(&operation, s_encrypted_text_p,
				   data_len, s_decrypted_text_p,
				   data_len, (size_t *)&olen);
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "psa_cipher_update failed! (Error: %d)\r\n", status);
		goto _error;
	}

	/* Finalize the decryption */
	status = psa_cipher_finish(&operation, s_decrypted_text_p + olen,
				   data_len - olen,
				   (size_t *)&olen);
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "psa_cipher_finish failed! (Error: %d)\r\n", status);
		goto _error;
	}

	end = crypto_get_time();
	crypto_unlock();
	crypto_perf_log("AES_CBC_DEC", "120M", key_len, data_len, end - start);

	/* Check the validity of the decryption */
	if (memcmp(s_decrypted_text_p, s_plain_text_p, data_len) != 0){
		BK_LOGE(TAG, "Error: Decrypted text doesn't match the plaintext\r\n");
		goto _error;
	}

	BK_LOGI(TAG, "Decryption successful!\r\n");

	/*  Clean up cipher operation context */
	psa_cipher_abort(&operation);

	return APP_SUCCESS;

_error:
	crypto_unlock();
	return APP_ERROR;
}

int aes_cbc_perf_main(void)
{
	uint32_t key_len_list[] = {128, 192, 256};
	uint32_t data_len_list[] = {256, 512, 1024, 2048, 4096};
	uint32_t cpu_freq_list[] = {PM_CPU_FRQ_120M, PM_CPU_FRQ_240M};
	uint32_t key;
	uint32_t data;
	uint32_t cpu;
	int status;

	BK_LOGI(TAG, "Starting AES-CBC-NO-PADDING perf test\r\n");

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
	
				status = encrypt_cbc_aes_perf(key_len_list[key], data_len_list[data]);
				if (status != APP_SUCCESS) {
					goto _error;
				}
			
				status = decrypt_cbc_aes_perf(key_len_list[key], data_len_list[data]);
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
	BK_LOGI(TAG, "AES-CBC-NO-PADDING perf test end\r\n");
	test_deinit();
	return APP_SUCCESS;

_error:
	test_deinit();
	BK_LOGI(TAG, APP_ERROR_MESSAGE);
	return APP_ERROR;
}
