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
#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include "crypto_test.h"
#include <modules/pm.h>

#define APP_SUCCESS		(0)
#define APP_ERROR		(-1)
#define APP_SUCCESS_MESSAGE "Example finished successfully!"
#define APP_ERROR_MESSAGE "Example exited with error!"

#define TAG "mbedtls"

#define CRYPTO_EXAMPLE_SHA384_TEXT_SIZE (150)
#define CRYPTO_EXAMPLE_SHA384_SIZE (48)

/* Below text is used as plaintext for computing/verifying the hash. */
static uint8_t m_plain_text[CRYPTO_EXAMPLE_SHA384_TEXT_SIZE] = {
	"Example string to demonstrate basic usage of SHA384."
	"That uses single and multi-part PSA crypto API's to "
	"perform a SHA-384 hashing operation."
};

static uint8_t m_hash[CRYPTO_EXAMPLE_SHA384_SIZE];
static uint8_t *s_plain_text_p = NULL;
#define MAX_TEXT_SIZE 8192

static int test_init(void)
{
	s_plain_text_p = os_malloc(MAX_TEXT_SIZE);

	if (!s_plain_text_p) {
		BK_LOGE(TAG, "Failed to alloc memory...\r\n");
		return -1;
	}

	os_memset(s_plain_text_p, 0xaa, MAX_TEXT_SIZE);

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

static int hash_singlepart_sha384(void)
{
	uint32_t olen;
	psa_status_t status;

	BK_LOGI(TAG, "Hashing using SHA384...\r\n");

	/* Calculate the SHA256 hash */
	status = psa_hash_compute(
		PSA_ALG_SHA_384, m_plain_text, sizeof(m_plain_text), m_hash, sizeof(m_hash), (size_t *)&olen);
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_hash_compute failed! (Error: %d)\r\n", status);
		return APP_ERROR;
	}

	BK_LOGI(TAG, "Hashing successful!\r\n");

	return APP_SUCCESS;
}

static int hash_multipart_sha384(void)
{
	uint32_t olen;
	psa_status_t status;
	uint8_t *input_ptr = m_plain_text;
	psa_hash_operation_t hash_operation = {0};

	BK_LOGI(TAG, "Hashing using multi-part SHA384...\r\n");

	/* Setup a multipart hash operation */
	status = psa_hash_setup(&hash_operation, PSA_ALG_SHA_384);
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "Could not setup the hash operation! Error %d\r\n", status);
		return APP_ERROR;
	}

	/* Feed the chunks of the input data to the PSA driver */
	status = psa_hash_update(&hash_operation, input_ptr, 42);
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "Could not hash the next chunk! Error %d\r\n", status);
		return APP_ERROR;

	}
	BK_LOGI(TAG, "Added %d bytes\r\n", 42);
	input_ptr += 42;

	status = psa_hash_update(&hash_operation, input_ptr, 58);
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "Could not hash the next chunk! Error %d\r\n", status);
		return APP_ERROR;

	}
	BK_LOGI(TAG, "Added %d bytes\r\n", 58);
	input_ptr += 58;

	status = psa_hash_update(&hash_operation, input_ptr, 50);
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "Could not hash the next chunk! Error %d\r\n", status);
		return APP_ERROR;
	}
	BK_LOGI(TAG, "Added %d bytes\r\n", 50);

	status = psa_hash_finish(&hash_operation, m_hash, sizeof(m_hash), (size_t *)&olen);
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "Could not finish the hash operation! Error %d\r\n", status);
		return APP_ERROR;
	}

	BK_LOGI(TAG, "Hashing successful!\r\n");

	return APP_SUCCESS;
}

static int verify_sha384(void)
{
	psa_status_t status;

	BK_LOGI(TAG, "Verifying the SHA384 hash...\r\n");

	/* Verify the hash */
	status = psa_hash_compare(
		PSA_ALG_SHA_384, m_plain_text, sizeof(m_plain_text), m_hash, sizeof(m_hash));
	if (status != PSA_SUCCESS) {
		BK_LOGI(TAG, "psa_hash_compare failed! (Error: %d)\r\n", status);
		return APP_ERROR;
	}

	BK_LOGI(TAG, "SHA384 verification successful!\r\n");

	return APP_SUCCESS;
}

int sha384_main(void)
{
	int status;

	BK_LOGI(TAG, "Starting SHA384 example...\r\n");

	status = crypto_init();
	if (status != APP_SUCCESS) {
		BK_LOGI(TAG, APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = hash_singlepart_sha384();
	if (status != APP_SUCCESS) {
		BK_LOGI(TAG, APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = verify_sha384();
	if (status != APP_SUCCESS) {
		BK_LOGI(TAG, APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	/* Reset the hash */
	memset(m_hash, 0, sizeof(m_hash));

	status = hash_multipart_sha384();
	if (status != APP_SUCCESS) {
		BK_LOGI(TAG, APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = verify_sha384();
	if (status != APP_SUCCESS) {
		BK_LOGI(TAG, APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	BK_LOGI(TAG, APP_SUCCESS_MESSAGE);

	return APP_SUCCESS;
}

static int hash_singlepart_sha384_perf(uint32_t data_len)
{
	uint32_t olen;
	psa_status_t status;
	uint64_t start, end;

	BK_LOGI(TAG, "Hashing using SHA384...\r\n");
	crypto_lock();
	start = crypto_get_time();

	/* Calculate the SHA256 hash */
	status = psa_hash_compute(PSA_ALG_SHA_384, s_plain_text_p, data_len, m_hash, sizeof(m_hash), (size_t *)&olen);
	if (status != PSA_SUCCESS) {
		crypto_unlock();
		BK_LOGI(TAG, "psa_hash_compute failed! (Error: %d)\r\n", status);
		return APP_ERROR;
	}
	end = crypto_get_time();
	crypto_unlock();
	crypto_perf_log("SHA384", "120M", 0, data_len, end - start);
	BK_LOGI(TAG, "Hashing successful!\r\n");

	return APP_SUCCESS;
}

static int verify_sha384_perf(uint32_t data_len)
{
	psa_status_t status;
	uint64_t start, end;

	BK_LOGI(TAG, "Verifying the SHA384 hash...\r\n");
	crypto_lock();
	start = crypto_get_time();

	/* Verify the hash */
	status = psa_hash_compare(
		PSA_ALG_SHA_384, s_plain_text_p, data_len, m_hash, sizeof(m_hash));
	if (status != PSA_SUCCESS) {
		crypto_unlock();
		BK_LOGI(TAG, "psa_hash_compare failed! (Error: %d)\r\n", status);
		return APP_ERROR;
	}

	end = crypto_get_time();
	crypto_unlock();
	crypto_perf_log("SHA384_VERIFY", "120M", 0, data_len, end - start);
	BK_LOGI(TAG, "SHA384 verification successful!\r\n");

	return APP_SUCCESS;
}

int sha384_perf_main(void)
{
	uint32_t cpu_freq_list[] = {PM_CPU_FRQ_120M, PM_CPU_FRQ_240M};
	uint32_t data_len_list[] = {32, 1024, 4096};
	uint32_t data;
	uint32_t cpu;
	int status;

	BK_LOGI(TAG, "SHA384 perf test\r\n");

	if (test_init() != 0) {
		goto _error;
	}

	status = crypto_init();
	if (status != APP_SUCCESS) {
		goto _error;
	}

	for (data = 0; data < sizeof(data_len_list)/sizeof(uint32_t); data++) {
		for (cpu = 0; cpu < sizeof(cpu_freq_list)/sizeof(uint32_t); cpu++) {
			crypto_set_cpu_freq(cpu_freq_list[cpu]);
			status = hash_singlepart_sha384_perf(data_len_list[data]);
			if (status != APP_SUCCESS) {
				goto _error;
			}
		
			status = verify_sha384_perf(data_len_list[data]);
			if (status != APP_SUCCESS) {
				goto _error;
			}
		}
	}

	BK_LOGI(TAG, APP_SUCCESS_MESSAGE);
	BK_LOGI(TAG, "SHA384 perf test OK\r\n");
	test_deinit();

	return APP_SUCCESS;

_error:
	test_deinit();
	BK_LOGI(TAG, APP_ERROR_MESSAGE);
	return APP_ERROR;
}
