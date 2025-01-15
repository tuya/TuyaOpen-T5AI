// Copyright 2020-2025 Beken
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

#pragma once

#include "sdkconfig.h"
#include "CheckSumUtils.h"
#include "bk_https.h"

#if (CONFIG_SECURITY_OTA)

#define BK_ERR_OTA_HDR_MAGIC   (BK_ERR_OTA_BASE - 1)
#define BK_ERR_OTA_OOM         (BK_ERR_OTA_BASE - 2)
#define BK_ERR_OTA_IMG_HDR_CRC (BK_ERR_OTA_BASE - 3)
#define BK_ERR_OTA_IMG_ID      (BK_ERR_OTA_BASE - 4)
#define BK_ERR_OTA_IMG_CRC     (BK_ERR_OTA_BASE - 5)

#if CONFIG_VALIDATE_BEFORE_REBOOT
#include <mbedtls/sha256.h>
#include "mbedtls/ecdsa.h"
#include <driver/otp.h>
#include "mbedtls/oid.h"
#include "mbedtls/asn1.h"

#define BK_ERR_OTA_VALIDATE_TLV_FAIL                    (BK_ERR_OTA_BASE - 6)
#define BK_ERR_OTA_VALIDATE_IMG_HASH_FAIL               (BK_ERR_OTA_BASE - 7)
#define BK_ERR_OTA_VALIDATE_SEC_COUNTER_FAIL            (BK_ERR_OTA_BASE - 8)
#define BK_ERR_OTA_VALIDATE_PUB_KEY_FAIL                (BK_ERR_OTA_BASE - 9)
#define BK_ERR_OTA_VALIDATE_SIGNATURE_FAIL              (BK_ERR_OTA_BASE - 10)
#define BK_ERR_OTA_CBUS_READ_CRC_FAIL                       (BK_ERR_OTA_BASE - 11)

#define TLV_TOTAL_SIZE 512
#define NUM_ECC_BYTES (256 / 8)
#define IMAGE_TLV_INFO_MAGIC        0x6907
#define IMAGE_TLV_PROT_INFO_MAGIC   0x6908

#define IMAGE_TLV_PUBKEY            0x02   /* public key */
#define IMAGE_TLV_SHA256            0x10   /* SHA256 of image hdr and body */
#define IMAGE_TLV_ECDSA256          0x22   /* ECDSA of hash output */
#define IMAGE_TLV_SEC_CNT           0x50   /* security counter */
#define IMAGE_TLV_CUSTOM            0xA0

#define MAX_PUBKEY_LEN              128

typedef struct image_version {
    uint8_t iv_major;
    uint8_t iv_minor;
    uint16_t iv_revision;
    uint32_t iv_build_num;
} image_version_t;
typedef struct image_header {
    uint32_t ih_magic;
    uint32_t ih_load_addr;
    uint16_t ih_hdr_size;           /* Size of image header (bytes). */
    uint16_t ih_protect_tlv_size;   /* Size of protected TLV area (bytes). */
    uint32_t ih_img_size;           /* Does not include header. */
    uint32_t ih_flags;              /* IMAGE_F_[...]. */
    image_version_t ih_ver;
    uint32_t _pad1;
} image_header_t;

typedef struct image_tlv_info {
    uint16_t it_magic;
    uint16_t it_tlv_tot;  /* size of TLV area (including tlv_info header) */
} image_tlv_info_t;

/** Image trailer TLV format. All fields in little endian. */
typedef struct image_tlv {
    uint16_t it_type;   /* IMAGE_TLV_[...]. */
    uint16_t it_len;    /* Data length (not including TLV header). */
} image_tlv_t;

static const uint8_t ec_pubkey_oid[] = MBEDTLS_OID_EC_ALG_UNRESTRICTED;
static const uint8_t ec_secp256r1_oid[] = MBEDTLS_OID_EC_GRP_SECP256R1;

#endif
typedef struct ota_header_s {
        UINT64 magic;
        UINT32 crc;
        UINT32 version;
        UINT16 header_len;
        UINT16 image_num;
        UINT32 flags;
        UINT32 reserved[2];
} ota_header_t;

typedef struct ota_image_header_s {
        UINT32 image_len;
        UINT32 image_offset;
        UINT32 flash_offset;
        UINT32 checksum;
        UINT32 version;
        UINT32 flags;
        UINT32 reserved[2];
} ota_image_header_t;

typedef enum {
	OTA_PARSE_HEADER = 0,
	OTA_PARSE_IMG_HEADER,
	OTA_PARSE_IMG,
} ota_parse_type;

typedef struct ota_parse_s {
	ota_parse_type phase;
	ota_header_t ota_header;
	ota_image_header_t *ota_image_header;
	UINT32 offset;
	UINT32 index;
	UINT32 write_offset;
	CRC32_Context ota_crc;
	uint32_t total_rx_len;
	uint32_t percent;
#if CONFIG_VALIDATE_BEFORE_REBOOT
	mbedtls_sha256_context ctx;
	uint32_t validate_offset;
	uint32_t hash_size;
	uint8_t hash_result[32];
	struct image_header hdr;
	uint8_t tlv_total[TLV_TOTAL_SIZE];
#endif
} ota_parse_t;

typedef struct ota_image_info_s {
	uint32_t partition_offset;
	uint32_t partition_size;
	uint8_t fwu_image_id;
} ota_partition_info_t;

typedef enum {
    SECURITY_OTA_START,
    SECURITY_OTA_ERROR,
    SECURITY_OTA_SUCCESS,
    SECURITY_OTA_RESTART,
} security_ota_event_id_t;

typedef struct {
	security_ota_event_id_t event_id;
	void *data;
	int data_len;
} security_ota_event_t;

typedef void (*security_ota_cb_t)(security_ota_event_t);
typedef bk_err_t (*http_client_init_cb_t)(bk_http_client_handle_t);

typedef struct {
    bk_http_input_t *http_config;
    security_ota_cb_t ota_event_handler;
} security_ota_config_t;

void bk_ota_accept_image(void);
void bk_ota_clear_flag(void);
uint32_t bk_ota_get_flag(void);
void bk_ota_set_flag(uint32_t flag);
int security_ota_finish(void);

void security_ota_init(const char* https_url);
int security_ota_deinit(void);
int security_ota_parse_data(char *data, int len);
void security_ota_dispatch_event(security_ota_event_id_t event_id, void* event_data, int event_data_len);
uint32_t security_ota_get_restart(void);

bk_err_t update_back_pubkey_from_primary(void);
bk_err_t bk_ota_confirm_update();
bk_err_t bk_ota_cancel_update(uint8_t erase_ota);
bk_err_t bk_read_sign_version_from_primary(uint8_t* version);

#endif

