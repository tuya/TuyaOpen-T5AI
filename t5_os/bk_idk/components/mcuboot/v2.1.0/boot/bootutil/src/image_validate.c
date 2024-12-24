/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2017-2019 Linaro LTD
 * Copyright (c) 2016-2019 JUUL Labs
 * Copyright (c) 2019-2024 Arm Limited
 * Copyright (c) 2024 Beken
 *
 * Original license:
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

#include <flash_map_backend/flash_map_backend.h>

#include "bootutil/image.h"
#include "bootutil/crypto/sha.h"
#include "bootutil/sign_key.h"
#include "bootutil/security_cnt.h"
#include "bootutil/bootutil_public.h"
#include "bootutil/fault_injection_hardening.h"
#include "flash_partition.h"

#include "mcuboot_config/mcuboot_config.h"

#ifdef MCUBOOT_ENC_IMAGES
#include "bootutil/enc_key.h"
#endif
#if defined(MCUBOOT_SIGN_RSA)
#include "mbedtls/rsa.h"
#endif
#if defined(MCUBOOT_SIGN_EC256)
#include "mbedtls/ecdsa.h"
#endif
#if defined(MCUBOOT_ENC_IMAGES) || defined(MCUBOOT_SIGN_RSA) || \
    defined(MCUBOOT_SIGN_EC256)
#include "mbedtls/asn1.h"
#endif

#include "bootutil_priv.h"
#include <driver/flash.h>
#include "bootutil/bootutil_log.h"
#include "aon_pmu_hal.h"
#include "components/system.h"

#if CONFIG_DBUS_CHECK_CRC
uint16_t crc16_table[256];

typedef struct flash_data {
    uint8_t data[32];
    uint16_t crc16;
} flash_data_t;

void generate_crc16_table(uint16_t *table, uint16_t polynomial)
{
    for (int i = 0; i< 256; i++) {
        uint16_t crc = i << 8;
        for (int j = 0; j < 8; j++) {
            if ( crc & 0x8000) {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc = crc << 1;
            }
        }
        table[i] = crc;
    }
}

uint16_t calculate_crc16(const uint8_t *data, size_t length) 
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        uint8_t index = (crc >> 8) ^ data[i];
        crc = (crc << 8) ^ crc16_table[index];
    }
    return ((crc >> 8) & 0xFF) | ((crc & 0xFF) << 8);
}

bool check_crc(uint32_t start, size_t size)
{
    flash_data_t enc_data;
    uint16_t polynomial = 0x8005;

    size = (size + 32 - 1) & ~(32 - 1); // 32 align_up
    size = (size >> 5) *34;

    generate_crc16_table(crc16_table, polynomial);

    for (size_t i = 0; i < size / 34; i++) {
#if CONFIG_BL2_WDT
        aon_wdt_feed();
#endif
        bk_flash_read_bytes(start + i*34, &enc_data, 34);
        if (enc_data.crc16 != calculate_crc16(enc_data.data, 32)) {
            BOOT_LOG_ERR("read crc = %#x,cal crc =%#x, index=%d\r\n",enc_data.crc16, calculate_crc16(enc_data.data, 32), i);
            return false;
        }
    }
    return true;
}

#endif

/*
 * Compute SHA hash over the image.
 * (SHA384 if ECDSA-P384 is being used,
 *  SHA256 otherwise).
 */
static int
bootutil_img_hash(struct enc_key_data *enc_state, int image_index,
                  struct image_header *hdr, const struct flash_area *fap,
                  uint8_t *tmp_buf, uint32_t tmp_buf_sz, uint8_t *hash_result,
                  uint8_t *seed, int seed_len)
{
    bootutil_sha_context sha_ctx;
    uint32_t blk_sz;
    uint32_t size;
    uint16_t hdr_size;
    uint32_t off;
    int rc;
    uint32_t blk_off;
    uint32_t tlv_off;

#if (BOOT_IMAGE_NUMBER == 1) || !defined(MCUBOOT_ENC_IMAGES) || \
    defined(MCUBOOT_RAM_LOAD)
    (void)enc_state;
    (void)image_index;
    (void)hdr_size;
    (void)blk_off;
    (void)tlv_off;
#ifdef MCUBOOT_RAM_LOAD
    (void)blk_sz;
    (void)off;
    (void)rc;
    (void)fap;
    (void)tmp_buf;
    (void)tmp_buf_sz;
#endif
#endif

#ifdef MCUBOOT_ENC_IMAGES
    /* Encrypted images only exist in the secondary slot */
    if (MUST_DECRYPT(fap, image_index, hdr) &&
            !boot_enc_valid(enc_state, image_index, fap)) {
        return -1;
    }
#endif

    bootutil_sha_init(&sha_ctx);

    /* in some cases (split image) the hash is seeded with data from
     * the loader image */
    if (seed && (seed_len > 0)) {
        bootutil_sha_update(&sha_ctx, seed, seed_len);
    }

    /* Hash is computed over image header and image itself. */
    size = hdr_size = hdr->ih_hdr_size;
    size += hdr->ih_img_size;
    tlv_off = size;

    /* If protected TLVs are present they are also hashed. */
    size += hdr->ih_protect_tlv_size;

#ifdef MCUBOOT_RAM_LOAD
    bootutil_sha_update(&sha_ctx,
                        (void*)(IMAGE_RAM_BASE + hdr->ih_load_addr),
                        size);
#else
#if CONFIG_OTA_OVERWRITE
    if( fap->fa_off == partition_get_phy_offset(PARTITION_OTA )) {
        for (off = 0; off < size; off += blk_sz) {
            blk_sz = size - off;
            if (blk_sz > tmp_buf_sz) {
                blk_sz = tmp_buf_sz;
            }
#ifdef MCUBOOT_ENC_IMAGES
        /* The only data that is encrypted in an image is the payload;
         * both header and TLVs (when protected) are not.
         */
        if ((off < hdr_size) && ((off + blk_sz) > hdr_size)) {
            /* read only the header */
            blk_sz = hdr_size - off;
        }
        if ((off < tlv_off) && ((off + blk_sz) > tlv_off)) {
            /* read only up to the end of the image payload */
            blk_sz = tlv_off - off;
        }
#endif
        rc = flash_area_read(fap, off, tmp_buf, blk_sz);
        if (rc) {
            bootutil_sha_drop(&sha_ctx);
            return rc;
        }
#ifdef MCUBOOT_ENC_IMAGES
        if (MUST_DECRYPT(fap, image_index, hdr)) {
            /* Only payload is encrypted (area between header and TLVs) */
            if (off >= hdr_size && off < tlv_off) {
                blk_off = (off - hdr_size) & 0xf;
                boot_encrypt(enc_state, image_index, fap, off - hdr_size,
                        blk_sz, blk_off, tmp_buf);
            }
        }
#endif
        bootutil_sha_update(&sha_ctx, tmp_buf, blk_sz);
        }
    }
    else 
#endif /* CONFIG_OTA_OVERWRITE */
    {
        uint32_t addr = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(fap->fa_off));
#if CONFIG_DIRECT_XIP
        uint32_t sec_addr = partition_get_phy_offset(PARTITION_SECONDARY_ALL);
        if(fap->fa_off == sec_addr){
            flash_set_excute_enable(1);
            addr = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(partition_get_phy_offset(PARTITION_PRIMARY_ALL)));
        }
#endif
#if CONFIG_DBUS_CHECK_CRC
        if ((addr, size) != true) {
            BOOT_LOG_ERR("app crc16 error");
            return -1;
        } else {
            BOOT_LOG_INF("app crc16 right");
        }
#endif
        bootutil_sha_update(&sha_ctx,
                            (void*)(addr + SOC_FLASH_BASE_ADDR),
                            size);
#if CONFIG_DIRECT_XIP
        flash_set_excute_enable(0);
#endif
    }
#endif /* MCUBOOT_RAM_LOAD */
    bootutil_sha_finish(&sha_ctx, hash_result);
    bootutil_sha_drop(&sha_ctx);

    return 0;
}

static int
bootutil_hash(uint8_t *data, uint32_t data_sz, uint8_t *hash_result)
{
    bootutil_sha256_context sha256_ctx;
    bootutil_sha256_init(&sha256_ctx);
    bootutil_sha256_update(&sha256_ctx, data, data_sz);
    bootutil_sha256_finish(&sha256_ctx, hash_result);
    bootutil_sha256_drop(&sha256_ctx);
    return 0;
}

/*
 * Currently, we only support being able to verify one type of
 * signature, because there is a single verification function that we
 * call.  List the type of TLV we are expecting.  If we aren't
 * configured for any signature, don't define this macro.
 */
#if (defined(MCUBOOT_SIGN_RSA)      + \
     defined(MCUBOOT_SIGN_EC256)    + \
     defined(MCUBOOT_SIGN_EC384)    + \
     defined(MCUBOOT_SIGN_ED25519)) > 1
#error "Only a single signature type is supported!"
#endif

#if defined(MCUBOOT_SIGN_RSA)
#    if MCUBOOT_SIGN_RSA_LEN == 2048
#        define EXPECTED_SIG_TLV IMAGE_TLV_RSA2048_PSS
#    elif MCUBOOT_SIGN_RSA_LEN == 3072
#        define EXPECTED_SIG_TLV IMAGE_TLV_RSA3072_PSS
#    else
#        error "Unsupported RSA signature length"
#    endif
#    define SIG_BUF_SIZE (MCUBOOT_SIGN_RSA_LEN / 8)
#    define EXPECTED_SIG_LEN(x) ((x) == SIG_BUF_SIZE) /* 2048 bits */
#elif defined(MCUBOOT_SIGN_EC256) || \
      defined(MCUBOOT_SIGN_EC384) || \
      defined(MCUBOOT_SIGN_EC)
#    define EXPECTED_SIG_TLV IMAGE_TLV_ECDSA_SIG
#    define SIG_BUF_SIZE 128
#    define EXPECTED_SIG_LEN(x) (1) /* always true, ASN.1 will validate */
#elif defined(MCUBOOT_SIGN_ED25519)
#    define EXPECTED_SIG_TLV IMAGE_TLV_ED25519
#    define SIG_BUF_SIZE 64
#    define EXPECTED_SIG_LEN(x) ((x) == SIG_BUF_SIZE)
#else
#    define SIG_BUF_SIZE 32 /* no signing, sha256 digest only */
#endif

#if (defined(MCUBOOT_HW_KEY)       + \
     defined(MCUBOOT_BUILTIN_KEY)) > 1
#error "Please use either MCUBOOT_HW_KEY or the MCUBOOT_BUILTIN_KEY feature."
#endif

#ifdef EXPECTED_SIG_TLV

#if !defined(MCUBOOT_BUILTIN_KEY)
#if !defined(MCUBOOT_HW_KEY)
/* The key TLV contains the hash of the public key. */
#   define EXPECTED_KEY_TLV     IMAGE_TLV_KEYHASH
#   define KEY_BUF_SIZE         IMAGE_HASH_SIZE
#else
/* The key TLV contains the whole public key.
 * Add a few extra bytes to the key buffer size for encoding and
 * for public exponent.
 */
#   define EXPECTED_KEY_TLV     IMAGE_TLV_PUBKEY
#   define KEY_BUF_SIZE         (SIG_BUF_SIZE + 24)
#endif /* !MCUBOOT_HW_KEY */

#if !defined(MCUBOOT_HW_KEY)
static int
bootutil_find_key(uint8_t *keyhash, uint8_t keyhash_len)
{
    bootutil_sha_context sha_ctx;
    int i;
    const struct bootutil_key *key;
    uint8_t hash[IMAGE_HASH_SIZE];

    if (keyhash_len > IMAGE_HASH_SIZE) {
        return -1;
    }

    for (i = 0; i < bootutil_key_cnt; i++) {
        key = &bootutil_keys[i];
        bootutil_sha_init(&sha_ctx);
        bootutil_sha_update(&sha_ctx, key->key, *key->len);
        bootutil_sha_finish(&sha_ctx, hash);
        if (!memcmp(hash, keyhash, keyhash_len)) {
            bootutil_sha_drop(&sha_ctx);
            return i;
        }
    }
    bootutil_sha_drop(&sha_ctx);
    return -1;
}
#else /* !MCUBOOT_HW_KEY */
#if CONFIG_OTA_UPDATE_PUBKEY
typedef struct {
	uint8_t crc;
} CRC8_Context;

static uint8_t UpdateCRC8(uint8_t crcIn, uint8_t byte)
{
	uint8_t crc = crcIn;
	uint8_t i;

	crc ^= byte;

	for (i = 0; i < 8; i++) {
		if (crc & 0x01) {
			crc = (crc >> 1) ^ 0x8C;
		} else {
			crc >>= 1;
		}
	}
	return crc;
}

static void CRC8_Init( CRC8_Context *inContext )
{
    inContext->crc = 0;
}

static void CRC8_Update( CRC8_Context *inContext, const void *inSrc, size_t inLen )
{
	const uint8_t *src = (const uint8_t *) inSrc;
	const uint8_t *srcEnd = src + inLen;
	while ( src < srcEnd ) {
		inContext->crc = UpdateCRC8(inContext->crc, *src++);
	}
}

extern unsigned int pub_key_len;
static int
bootutil_find_key(uint8_t image_index, uint8_t *key, uint16_t key_len)
{
    uint32_t phy_off = partition_get_phy_offset(PARTITION_PUBLIC_KEY);
    uint32_t vir_off = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(phy_off + 3)); 
    uint16_t vir_length;
    uint8_t* check_key;
    CRC8_Context crc8;
    CRC8_Context check_crc;
    fih_int fih_rc;

    bk_flash_read_bytes(phy_off, (uint8_t *)&vir_length, 2); // vir_length = key_size
    if (key_len != vir_length) {
        BOOT_LOG_ERR("pubkey size mismatch: back=%d, ota-outer=%d", vir_length, key_len);
        return -1;
    }

    bk_flash_read_bytes(phy_off + 2, (uint8_t *)&check_crc, 1);
    uint16_t phy_length = ((vir_length + 31) >> 5) * 34; //ceil align 34
    uint8_t *enc_buf = (uint8_t*)malloc(phy_length);
    bk_flash_read_bytes(phy_off + 3, enc_buf, phy_length);
    CRC8_Init(&crc8);
    CRC8_Update(&crc8, enc_buf, phy_length);

    if (crc8.crc != check_crc.crc) {
        free(enc_buf);
        BOOT_LOG_ERR("back pubkey corrupted, crc8 expected=%#x, actual=%#x", check_crc.crc, crc8.crc);
        return -1;
    }

    check_key = (uint8_t *)malloc(key_len);
    bk_flash_read_cbus(vir_off, check_key, key_len);

    bootutil_hash(key, key_len, hash);
    bootutil_hash(check_key, key_len, check_hash);

    FIH_CALL(boot_fih_memequal, fih_rc, key, check_key, key_len);
    if (fih_eq(fih_rc, FIH_SUCCESS)) {
        bootutil_keys[0].key = key;
        pub_key_len = key_len;
        free(enc_buf);
        free(check_key);
        BOOT_LOG_INF("pubkey ok!");
        return 0;
    }

    BOOT_LOG_ERR("pubkey of back and OTA-outer not equal!");
    return -1;
}
#else
extern unsigned int pub_key_len;
static int
bootutil_find_key(uint8_t image_index, uint8_t *key, uint16_t key_len)
{
    bootutil_sha_context sha_ctx;
    uint8_t hash[IMAGE_HASH_SIZE];
    uint8_t key_hash[IMAGE_HASH_SIZE];
    size_t key_hash_size = sizeof(key_hash);
    int rc;
    FIH_DECLARE(fih_rc, FIH_FAILURE);

    bootutil_sha_init(&sha_ctx);
    bootutil_sha_update(&sha_ctx, key, key_len);
    bootutil_sha_finish(&sha_ctx, hash);
    bootutil_sha_drop(&sha_ctx);

#if CONFIG_OTA_UPDATE_PUBKEY
    uint8_t primary_key[key_len];
    rc = bk_read_pubkey_from_primary(primary_key, key_len);
    bootutil_sha256_init(&sha256_ctx);
    bootutil_sha256_update(&sha256_ctx, primary_key, key_len);
    bootutil_sha256_finish(&sha256_ctx, key_hash);
    bootutil_sha256_drop(&sha256_ctx);
#else
    rc = boot_retrieve_public_key_hash(image_index, key_hash, &key_hash_size);
#endif /*CONFIG_OTA_UPDATE_PUBKEY*/
    if (rc) {
        return -1;
    }

    /* Adding hardening to avoid this potential attack:
     *  - Image is signed with an arbitrary key and the corresponding public
     *    key is added as a TLV field.
     * - During public key validation (comparing against key-hash read from
     *   HW) a fault is injected to accept the public key as valid one.
     */
    FIH_CALL(boot_fih_memequal, fih_rc, hash, key_hash, key_hash_size);
    if (FIH_EQ(fih_rc, FIH_SUCCESS)) {
        bootutil_keys[0].key = key;
        pub_key_len = key_len;
        return 0;
    }
    BOOT_LOG_ERR("invalid pub key\r\n");
    return -1;
}
#endif /*CONFIG_OTA_UPDATE_PUBKEY*/
#endif /* !MCUBOOT_HW_KEY */
#endif /* !MCUBOOT_BUILTIN_KEY */
#endif /* EXPECTED_SIG_TLV */

/**
 * Reads the value of an image's security counter.
 *
 * @param hdr           Pointer to the image header structure.
 * @param fap           Pointer to a description structure of the image's
 *                      flash area.
 * @param security_cnt  Pointer to store the security counter value.
 *
 * @return              0 on success; nonzero on failure.
 */
int32_t
bootutil_get_img_security_cnt(struct image_header *hdr,
                              const struct flash_area *fap,
                              uint32_t *img_security_cnt)
{
    struct image_tlv_iter it;
    uint32_t off;
    uint16_t len;
    int32_t rc;

    if ((hdr == NULL) ||
        (fap == NULL) ||
        (img_security_cnt == NULL)) {
        /* Invalid parameter. */
        return BOOT_EBADARGS;
    }

    /* The security counter TLV is in the protected part of the TLV area. */
    if (hdr->ih_protect_tlv_size == 0) {
        return BOOT_EBADIMAGE;
    }

    rc = bootutil_tlv_iter_begin(&it, hdr, fap, IMAGE_TLV_SEC_CNT, true);
    if (rc) {
        return rc;
    }

    /* Traverse through the protected TLV area to find
     * the security counter TLV.
     */

    rc = bootutil_tlv_iter_next(&it, &off, &len, NULL);
    if (rc != 0) {
        /* Security counter TLV has not been found. */
        return -1;
    }

    if (len != sizeof(*img_security_cnt)) {
        /* Security counter is not valid. */
        return BOOT_EBADIMAGE;
    }

    rc = LOAD_IMAGE_DATA(hdr, fap, off, img_security_cnt, len);
    if (rc != 0) {
        return BOOT_EFLASH;
    }

    return 0;
}

#ifndef ALLOW_ROGUE_TLVS
/*
 * The following list of TLVs are the only entries allowed in the unprotected
 * TLV section.  All other TLV entries must be in the protected section.
 */
static const uint16_t allowed_unprot_tlvs[] = {
     IMAGE_TLV_KEYHASH,
     IMAGE_TLV_PUBKEY,
     IMAGE_TLV_SHA256,
     IMAGE_TLV_SHA384,
     IMAGE_TLV_RSA2048_PSS,
     IMAGE_TLV_ECDSA224,
     IMAGE_TLV_ECDSA_SIG,
     IMAGE_TLV_RSA3072_PSS,
     IMAGE_TLV_ED25519,
     IMAGE_TLV_ENC_RSA2048,
     IMAGE_TLV_ENC_KW,
     IMAGE_TLV_ENC_EC256,
     IMAGE_TLV_ENC_X25519,
     /* Mark end with ANY. */
     IMAGE_TLV_ANY,
};
#endif

/*
 * Verify the integrity of the image.
 * Return non-zero if image could not be validated/does not validate.
 */
fih_ret
bootutil_img_validate(struct enc_key_data *enc_state, int image_index,
                      struct image_header *hdr, const struct flash_area *fap,
                      uint8_t *tmp_buf, uint32_t tmp_buf_sz, uint8_t *seed,
                      int seed_len, uint8_t *out_hash)
{
    BOOT_LOG_INF("starting validate slot %d",fap->fa_id);
#if CONFIG_BL2_SKIP_VALIDATE
    uint32_t reset_reason = aon_pmu_hal_get_reset_reason_from_sram();
    if ((bk_boot_read_ota_confirm(0)) && (reset_reason == RESET_SOURCE_POWERON)) {
        BOOT_LOG_INF("reset resaon %#x, skip validate",reset_reason);
        return 0;
    }
    BOOT_LOG_INF("reset resaon %#x, do validate",reset_reason);
#endif

    uint32_t off;
    uint16_t len;
    uint16_t type;
    int image_hash_valid = 0;
#ifdef EXPECTED_SIG_TLV
    FIH_DECLARE(valid_signature, FIH_FAILURE);
#ifndef MCUBOOT_BUILTIN_KEY
    int key_id = -1;
#else
    /* Pass a key ID equal to the image index, the underlying crypto library
     * is responsible for mapping the image index to a builtin key ID.
     */
    int key_id = image_index;
#endif /* !MCUBOOT_BUILTIN_KEY */
#ifdef MCUBOOT_HW_KEY
    uint8_t key_buf[KEY_BUF_SIZE];
#endif
#endif /* EXPECTED_SIG_TLV */
    struct image_tlv_iter it;
    uint8_t buf[SIG_BUF_SIZE];
    uint8_t hash[IMAGE_HASH_SIZE];
    int rc = 0;
    FIH_DECLARE(fih_rc, FIH_FAILURE);
#ifdef MCUBOOT_HW_ROLLBACK_PROT
    fih_int security_cnt = fih_int_encode(INT_MAX);
    uint32_t img_security_cnt = 0;
    FIH_DECLARE(security_counter_valid, FIH_FAILURE);
#endif

    rc = bootutil_img_hash(enc_state, image_index, hdr, fap, tmp_buf,
            tmp_buf_sz, hash, seed, seed_len);
    if (rc) {
        goto out;
    }

    if (out_hash) {
        memcpy(out_hash, hash, IMAGE_HASH_SIZE);
    }

    rc = bootutil_tlv_iter_begin(&it, hdr, fap, IMAGE_TLV_ANY, false);
    if (rc) {
        goto out;
    }

    if (it.tlv_end > bootutil_max_image_size(fap)) {
        rc = -1;
        goto out;
    }

    /*
     * Traverse through all of the TLVs, performing any checks we know
     * and are able to do.
     */
    while (true) {
        rc = bootutil_tlv_iter_next(&it, &off, &len, &type);
        if (rc < 0) {
            goto out;
        } else if (rc > 0) {
            break;
        }

#ifndef ALLOW_ROGUE_TLVS
        /*
         * Ensure that the non-protected TLV only has entries necessary to hold
         * the signature.  We also allow encryption related keys to be in the
         * unprotected area.
         */
        if (!bootutil_tlv_iter_is_prot(&it, off)) {
             bool found = false;
             for (const uint16_t *p = allowed_unprot_tlvs; *p != IMAGE_TLV_ANY; p++) {
                  if (type == *p) {
                       found = true;
                       break;
                  }
             }
             if (!found) {
                  FIH_SET(fih_rc, FIH_FAILURE);
                  goto out;
             }
        }
#endif

        if (type == EXPECTED_HASH_TLV) {
            /* Verify the image hash. This must always be present. */
            if (len != sizeof(hash)) {
                rc = -1;
                goto out;
            }
            rc = LOAD_IMAGE_DATA(hdr, fap, off, buf, sizeof(hash));
            if (rc) {
                goto out;
            }

            FIH_CALL(boot_fih_memequal, fih_rc, hash, buf, sizeof(hash));
            if (FIH_NOT_EQ(fih_rc, FIH_SUCCESS)) {
                BOOT_LOG_INF("slot %d hash fail",fap->fa_id);
                FIH_SET(fih_rc, FIH_FAILURE);
                goto out;
            }

            image_hash_valid = 1;

#if CONFIG_BL2_SKIP_VALIDATE
            if (image_hash_valid == 1 && fap->fa_id == 0) {
#if CONFIG_OTA_CONFIRM_UPDATE
                bk_boot_write_ota_confirm(0);
#endif
                return 0;
            }
#endif

#ifdef EXPECTED_KEY_TLV
        } else if (type == EXPECTED_KEY_TLV) {
            /*
             * Determine which key we should be checking.
             */
            if (len > KEY_BUF_SIZE) {
                rc = -1;
                goto out;
            }
#ifndef MCUBOOT_HW_KEY
            rc = LOAD_IMAGE_DATA(hdr, fap, off, buf, len);
            if (rc) {
                goto out;
            }
            key_id = bootutil_find_key(buf, len);
#else
            rc = LOAD_IMAGE_DATA(hdr, fap, off, key_buf, len);
            if (rc) {
                goto out;
            }
            key_id = bootutil_find_key(image_index, key_buf, len);
#endif /* !MCUBOOT_HW_KEY */
            /*
             * The key may not be found, which is acceptable.  There
             * can be multiple signatures, each preceded by a key.
             */
#endif /* EXPECTED_KEY_TLV */
#ifdef EXPECTED_SIG_TLV
        } else if (type == EXPECTED_SIG_TLV) {
            if (!efuse_is_secureboot_enabled()) {
                BOOT_LOG_INF("not enable security efuse, skip signature validate");
                valid_signature = FIH_SUCCESS;
                continue;
            }
            /* Ignore this signature if it is out of bounds. */
            if (key_id < 0 || key_id >= bootutil_key_cnt) {
                key_id = -1;
                continue;
            }
            if (!EXPECTED_SIG_LEN(len) || len > sizeof(buf)) {
                rc = -1;
                goto out;
            }
            rc = LOAD_IMAGE_DATA(hdr, fap, off, buf, len);
            if (rc) {
                goto out;
            }
            FIH_CALL(bootutil_verify_sig, valid_signature, hash, sizeof(hash), buf, len, key_id);
            BOOT_LOG_INF("slot %d sig result %d",fap->fa_id, valid_signature);
            key_id = -1;
#endif /* EXPECTED_SIG_TLV */
#if (defined(MCUBOOT_HW_ROLLBACK_PROT) && CONFIG_ANTI_ROLLBACK)
        } else if (type == IMAGE_TLV_SEC_CNT) {
            /*
             * Verify the image's security counter.
             * This must always be present.
             */
            if (len != sizeof(img_security_cnt)) {
                /* Security counter is not valid. */
                rc = -1;
                goto out;
            }

            rc = LOAD_IMAGE_DATA(hdr, fap, off, &img_security_cnt, len);
            if (rc) {
                goto out;
            }

            FIH_CALL(boot_nv_security_counter_get, fih_rc, image_index,
                                                           &security_cnt);
            if (FIH_NOT_EQ(fih_rc, FIH_SUCCESS)) {
                FIH_SET(fih_rc, FIH_FAILURE);
                goto out;
            }

            /* Compare the new image's security counter value against the
             * stored security counter value.
             */
            fih_rc = fih_ret_encode_zero_equality(img_security_cnt <
                                   (uint32_t)fih_int_decode(security_cnt));
            if (FIH_NOT_EQ(fih_rc, FIH_SUCCESS)) {
                FIH_SET(fih_rc, FIH_FAILURE);
                goto out;
            }

            /* The image's security counter has been successfully verified. */
            security_counter_valid = fih_rc;
#endif /* MCUBOOT_HW_ROLLBACK_PROT */
        }
    }

    rc = !image_hash_valid;
    if (rc) {
        goto out;
    }
#ifdef EXPECTED_SIG_TLV
    FIH_SET(fih_rc, valid_signature);
#endif
#if (defined(MCUBOOT_HW_ROLLBACK_PROT) && CONFIG_ANTI_ROLLBACK)
    if (FIH_NOT_EQ(security_counter_valid, FIH_SUCCESS)) {
        rc = -1;
        goto out;
    }
#endif

out:
    if (rc) {
        FIH_SET(fih_rc, FIH_FAILURE);
    }

    FIH_RET(fih_rc);
}
