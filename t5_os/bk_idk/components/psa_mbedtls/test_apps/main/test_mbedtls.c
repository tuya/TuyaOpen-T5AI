#include "unity_fixture.h"
#include "unity.h"
#include <os/os.h>
#include <os/mem.h>
#include "components/log.h"
#include "common/bk_include.h"
#include <modules/pm.h>
#include "psa/crypto.h"
#include "psa/crypto_extra.h"
#include <string.h>

#define TAG "mbedtls"

#define CRYPTO_EXAMPLE_AES_BLOCK_SIZE (16)
#define CRYPTO_EXAMPLE_AES_IV_SIZE (12)
static uint8_t m_iv[CRYPTO_EXAMPLE_AES_BLOCK_SIZE];

#define CRYPTO_EXAMPLE_AES_CBC_MAX_TEXT_SIZE (64)
static uint8_t m_plain_text_aes_cbc[CRYPTO_EXAMPLE_AES_CBC_MAX_TEXT_SIZE] = { "Example string to demonstrate basic usage of AES CBC mode." };
static uint8_t m_encrypted_text_aes_cbc[CRYPTO_EXAMPLE_AES_CBC_MAX_TEXT_SIZE];
static uint8_t m_decrypted_text_aes_cbc[CRYPTO_EXAMPLE_AES_CBC_MAX_TEXT_SIZE];

#define CRYPTO_EXAMPLE_AES_GCM_MAX_TEXT_SIZE (100)
#define CRYPTO_EXAMPLE_AES_ADDITIONAL_SIZE (35)
#define CRYPTO_EXAMPLE_AES_GCM_TAG_LENGTH (16)
static uint8_t m_plain_text_aes_gcm[CRYPTO_EXAMPLE_AES_GCM_MAX_TEXT_SIZE] = { "Example string to demonstrate basic usage of AES GCM mode." };
static uint8_t m_additional_data[CRYPTO_EXAMPLE_AES_ADDITIONAL_SIZE] = { "Example string of additional data" };
static uint8_t m_encrypted_text_aes_gcm[CRYPTO_EXAMPLE_AES_GCM_MAX_TEXT_SIZE + CRYPTO_EXAMPLE_AES_GCM_TAG_LENGTH];
static uint8_t m_decrypted_text_aes_gcm[CRYPTO_EXAMPLE_AES_GCM_MAX_TEXT_SIZE];

#define CRYPTO_EXAMPLE_HMAC_TEXT_SIZE (100)
#define CRYPTO_EXAMPLE_HMAC_KEY_SIZE (32)
static uint8_t hmac[CRYPTO_EXAMPLE_HMAC_KEY_SIZE];
static uint8_t m_plain_text_hmac[CRYPTO_EXAMPLE_HMAC_TEXT_SIZE] = { "Example string to demonstrate basic usage of HMAC signing/verification." };

#define CRYPTO_EXAMPLE_SHA256_TEXT_SIZE (150)
#define CRYPTO_EXAMPLE_SHA256_SIZE (32)
static uint8_t m_plain_text_sha256[CRYPTO_EXAMPLE_SHA256_TEXT_SIZE] = {
	"Example string to demonstrate basic usage of SHA256."
	"That uses single and multi-part PSA crypto API's to "
	"perform a SHA-256 hashing operation."
};
static uint8_t m_hash[CRYPTO_EXAMPLE_SHA256_SIZE];

#define CRYPTO_EXAMPLE_ECDH_KEY_BITS (256)
#define CRYPTO_EXAMPLE_ECDH_PUBLIC_KEY_SIZE (65)
static uint8_t m_pub_key_bob[CRYPTO_EXAMPLE_ECDH_PUBLIC_KEY_SIZE];
static uint8_t m_pub_key_alice[CRYPTO_EXAMPLE_ECDH_PUBLIC_KEY_SIZE];
static uint8_t m_secret_alice[32];
static uint8_t m_secret_bob[32];
psa_key_id_t key_id_alice;
psa_key_id_t key_id_bob;

#define CRYPTO_EXAMPLE_ECDSA_TEXT_SIZE (100)
#define CRYPTO_EXAMPLE_ECDSA_PUBLIC_KEY_SIZE (65)
#define CRYPTO_EXAMPLE_ECDSA_SIGNATURE_SIZE (64)
#define CRYPTO_EXAMPLE_ECDSA_HASH_SIZE (32)
static uint8_t m_plain_text_ecdsa[CRYPTO_EXAMPLE_ECDSA_TEXT_SIZE] = { "Example string to demonstrate basic usage of ECDSA." };
static uint8_t m_pub_key[CRYPTO_EXAMPLE_ECDSA_PUBLIC_KEY_SIZE];
static uint8_t m_signature[CRYPTO_EXAMPLE_ECDSA_SIGNATURE_SIZE];
static uint8_t m_hash_ecdsa[CRYPTO_EXAMPLE_ECDSA_HASH_SIZE];

static psa_key_id_t keypair_id;
static psa_key_id_t pub_key_id;
static psa_key_id_t key_id;



TEST_CASE("aes_cbc", "[crypto]")
{
	psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	uint32_t olen;

	TEST_BK_OK(psa_crypto_init());

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_CBC_NO_PADDING);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&key_attributes, 128);
	TEST_BK_OK(psa_generate_key(&key_attributes, &key_id));
	psa_reset_key_attributes(&key_attributes);


	TEST_BK_OK(psa_cipher_encrypt_setup(&operation, key_id, PSA_ALG_CBC_NO_PADDING));
	TEST_BK_OK(psa_cipher_generate_iv(&operation, m_iv, sizeof(m_iv), (size_t *)&olen));
	TEST_BK_OK(psa_cipher_update(&operation, m_plain_text_aes_cbc, sizeof(m_plain_text_aes_cbc), m_encrypted_text_aes_cbc, sizeof(m_encrypted_text_aes_cbc), (size_t *)&olen));
	TEST_BK_OK(psa_cipher_finish(&operation, m_encrypted_text_aes_cbc + olen, sizeof(m_encrypted_text_aes_cbc) - olen, (size_t *)&olen));
	TEST_BK_OK(psa_cipher_abort(&operation));
	
	TEST_BK_OK(psa_cipher_decrypt_setup(&operation, key_id, PSA_ALG_CBC_NO_PADDING));
	TEST_BK_OK(psa_cipher_set_iv(&operation, m_iv, sizeof(m_iv)));
	TEST_BK_OK(psa_cipher_update(&operation, m_encrypted_text_aes_cbc, sizeof(m_encrypted_text_aes_cbc), m_decrypted_text_aes_cbc, sizeof(m_decrypted_text_aes_cbc), (size_t *)&olen));
	TEST_BK_OK(psa_cipher_finish(&operation, m_decrypted_text_aes_cbc + olen, sizeof(m_decrypted_text_aes_cbc) - olen, (size_t *)&olen));
	TEST_ASSERT_EQUAL(memcmp(m_decrypted_text_aes_cbc, m_plain_text_aes_cbc, CRYPTO_EXAMPLE_AES_CBC_MAX_TEXT_SIZE), 0);

	TEST_BK_OK(psa_cipher_abort(&operation));
	TEST_BK_OK(psa_destroy_key(key_id));
}

TEST_CASE("aes_gcm", "[crypto]")
{
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	uint32_t output_len;

	TEST_BK_OK(psa_crypto_init());

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_GCM);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&key_attributes, 128);
	TEST_BK_OK(psa_generate_key(&key_attributes, &key_id));
	psa_reset_key_attributes(&key_attributes);

	TEST_BK_OK(psa_generate_random(m_iv, CRYPTO_EXAMPLE_AES_IV_SIZE));
	TEST_BK_OK(psa_aead_encrypt(key_id, PSA_ALG_GCM, m_iv, sizeof(m_iv), m_additional_data, sizeof(m_additional_data), m_plain_text_aes_gcm, sizeof(m_plain_text_aes_gcm), m_encrypted_text_aes_gcm, sizeof(m_encrypted_text_aes_gcm), (size_t *)&output_len));

	TEST_BK_OK(psa_aead_decrypt(key_id, PSA_ALG_GCM, m_iv, sizeof(m_iv), m_additional_data, sizeof(m_additional_data), m_encrypted_text_aes_gcm, sizeof(m_encrypted_text_aes_gcm), m_decrypted_text_aes_gcm, sizeof(m_decrypted_text_aes_gcm), (size_t *)&output_len));

	TEST_ASSERT_EQUAL(memcmp(m_decrypted_text_aes_gcm, m_plain_text_aes_gcm, CRYPTO_EXAMPLE_AES_GCM_MAX_TEXT_SIZE), 0);

	TEST_BK_OK(psa_destroy_key(key_id));
}

TEST_CASE("hmac", "[crypto]")
{
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_mac_operation_t operation = PSA_MAC_OPERATION_INIT;
	uint32_t olen;

	TEST_BK_OK(psa_crypto_init());

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_VERIFY_HASH | PSA_KEY_USAGE_SIGN_HASH);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_HMAC(PSA_ALG_SHA_256));
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_HMAC);
	psa_set_key_bits(&key_attributes, 256);

	TEST_BK_OK(psa_generate_key(&key_attributes, &key_id));
	psa_reset_key_attributes(&key_attributes);

	TEST_BK_OK(psa_mac_sign_setup(&operation, key_id, PSA_ALG_HMAC(PSA_ALG_SHA_256)));
	TEST_BK_OK(psa_mac_update(&operation, m_plain_text_hmac, sizeof(m_plain_text_hmac)));
	TEST_BK_OK(psa_mac_sign_finish(&operation, hmac, sizeof(hmac), (size_t *)&olen));

	TEST_BK_OK(psa_mac_verify_setup(&operation, key_id, PSA_ALG_HMAC(PSA_ALG_SHA_256)));
	TEST_BK_OK(psa_mac_update(&operation, m_plain_text_hmac, sizeof(m_plain_text_hmac)));
	TEST_BK_OK(psa_mac_verify_finish(&operation, hmac, sizeof(hmac)));

	TEST_BK_OK(psa_destroy_key(key_id));
}

TEST_CASE("sha256_single", "[crypto]")
{
	uint32_t olen;

	TEST_BK_OK(psa_crypto_init());
	memset(m_hash, 0, sizeof(m_hash));
	TEST_BK_OK(psa_hash_compute(PSA_ALG_SHA_256, m_plain_text_sha256, sizeof(m_plain_text_sha256), m_hash, sizeof(m_hash), (size_t *)&olen));
	TEST_BK_OK(psa_hash_compare(PSA_ALG_SHA_256, m_plain_text_sha256, sizeof(m_plain_text_sha256), m_hash, sizeof(m_hash)));
}

TEST_CASE("sha256_multiple", "[crypto]")
{
	psa_hash_operation_t hash_operation = {0};
	uint8_t *input_ptr = m_plain_text_sha256;
	uint32_t olen;

	TEST_BK_OK(psa_crypto_init());
	memset(m_hash, 0, sizeof(m_hash));
	TEST_BK_OK(psa_hash_setup(&hash_operation, PSA_ALG_SHA_256));
	TEST_BK_OK(psa_hash_update(&hash_operation, input_ptr, 42));
	input_ptr += 42;
	TEST_BK_OK(psa_hash_update(&hash_operation, input_ptr, 58));
	input_ptr += 58;
	TEST_BK_OK(psa_hash_update(&hash_operation, input_ptr, 50));
	TEST_BK_OK(psa_hash_finish(&hash_operation, m_hash, sizeof(m_hash), (size_t *)&olen));
	TEST_BK_OK(psa_hash_compare(PSA_ALG_SHA_256, m_plain_text_sha256, sizeof(m_plain_text_sha256), m_hash, sizeof(m_hash)));
}

TEST_CASE("ecdh", "[crypto]")
{
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	uint32_t output_len;
	size_t olen;

	TEST_BK_OK(psa_crypto_init());

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_DERIVE);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDH);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&key_attributes, 256);
	TEST_BK_OK(psa_generate_key(&key_attributes, &key_id_alice));
	TEST_BK_OK(psa_generate_key(&key_attributes, &key_id_bob));
	psa_reset_key_attributes(&key_attributes);

	TEST_BK_OK(psa_export_public_key(key_id_alice, m_pub_key_alice, sizeof(m_pub_key_alice), &olen));

	TEST_BK_OK(psa_export_public_key(key_id_bob, m_pub_key_bob, sizeof(m_pub_key_bob), &olen));

	TEST_BK_OK(psa_raw_key_agreement(PSA_ALG_ECDH, key_id_alice, m_pub_key_bob, sizeof(m_pub_key_bob), m_secret_alice, sizeof(m_secret_alice), (size_t *)&output_len));
	TEST_BK_OK(psa_raw_key_agreement(PSA_ALG_ECDH, key_id_bob, m_pub_key_alice, sizeof(m_pub_key_alice), m_secret_bob, sizeof(m_secret_bob), (size_t *)&output_len));

	TEST_ASSERT_EQUAL(memcmp(m_secret_bob, m_secret_alice, sizeof(m_secret_alice)), 0);

	TEST_BK_OK(psa_destroy_key(key_id_alice));
	TEST_BK_OK(psa_destroy_key(key_id_bob));
}

TEST_CASE("ecdsa", "[crypto]")
{
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	uint32_t output_len;
	size_t olen;

	TEST_BK_OK(psa_crypto_init());

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_SIGN_HASH);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&key_attributes, 256);
	TEST_BK_OK(psa_generate_key(&key_attributes, &keypair_id));
	TEST_BK_OK(psa_export_public_key(keypair_id, m_pub_key, sizeof(m_pub_key), &olen));
	psa_reset_key_attributes(&key_attributes);

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_VERIFY_HASH);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&key_attributes, 256);
	TEST_BK_OK(psa_import_key(&key_attributes, m_pub_key, sizeof(m_pub_key), &pub_key_id));
	psa_reset_key_attributes(&key_attributes);

	TEST_BK_OK(psa_hash_compute(PSA_ALG_SHA_256, m_plain_text_ecdsa, sizeof(m_plain_text_ecdsa), m_hash_ecdsa, sizeof(m_hash_ecdsa), (size_t *)&output_len));
	TEST_BK_OK(psa_sign_hash(keypair_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256), m_hash_ecdsa, sizeof(m_hash_ecdsa), m_signature, sizeof(m_signature), (size_t *)&output_len));

	TEST_BK_OK(psa_verify_hash(pub_key_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256), m_hash_ecdsa, sizeof(m_hash_ecdsa), m_signature, sizeof(m_signature)));
	TEST_BK_OK(psa_destroy_key(keypair_id));
	TEST_BK_OK(psa_destroy_key(pub_key_id));
}

void mbedtls_ut_stub(void)
{
}
