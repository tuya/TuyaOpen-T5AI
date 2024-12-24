#include "sdkconfig.h"
#include <string.h>
#include "cli.h"
#include <components/system.h>
#include "driver/flash.h"
#include "modules/ota.h"
#include "_ota.h"
#include "utils_httpc.h"
#include "modules/wifi.h"

#include <driver/flash.h>
#include "partitions.h"
#include "CheckSumUtils.h"
#include "security_ota.h"
#include <driver/wdt.h>
#include <components/log.h>

#define OTA_MAGIC_WORD "\x42\x4B\x37\x32\x33\x36\x35\x38"
#define MANIFEST_SIZE  (4 * 1024)
static uint32_t s_restart = 0;
extern void vPortEnableTimerInterrupt( void );
extern void vPortDisableTimerInterrupt( void );

static ota_parse_t ota_parse = {0};
static bool s_ota_finish_flag = true;
static bool s_ota_running_flag = false;

#define TAG "ota"

uint32_t security_ota_get_restart(void)
{
	return s_restart;
}

static void security_ota_set_flash_protect_type(int type)
{
	bk_flash_set_protect_type(type);
	if (type == FLASH_PROTECT_NONE) {
		// bk_flash_display_config_info("ota unprotect flash");
	} else {
		// bk_flash_display_config_info("ota protect flash");
	}
}

#if (CONFIG_TFM_FWU)
int bk_ota_check(psa_image_id_t ota_image);

#if CONFIG_INT_WDT
#include <driver/wdt.h>
#include <bk_wdt.h>
#endif


#include "sys_ctrl/sys_driver.h"

void wdt_init(void);
static uint32_t ota_image_flag = 0;

const ota_partition_info_t s_ota_partition_info[] = {
	{CONFIG_PRIMARY_MANIFEST_PHY_PARTITION_OFFSET,   CONFIG_PRIMARY_MANIFEST_PHY_PARTITION_SIZE,   FWU_IMAGE_TYPE_PRIMARY_MANIFEST},
	{CONFIG_SECONDARY_MANIFEST_PHY_PARTITION_OFFSET, CONFIG_SECONDARY_MANIFEST_PHY_PARTITION_SIZE, FWU_IMAGE_TYPE_SECONDARY_MANIFEST},
	{CONFIG_PRIMARY_BL2_PHY_PARTITION_OFFSET,        CONFIG_PRIMARY_BL2_PHY_PARTITION_SIZE,        FWU_IMAGE_TYPE_PRIMARY_BL2},
	{CONFIG_SECONDARY_BL2_PHY_PARTITION_OFFSET,      CONFIG_SECONDARY_BL2_PHY_PARTITION_SIZE,      FWU_IMAGE_TYPE_SECONDARY_BL2},
#if CONFIG_DIRECT_XIP
	{CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET,        CONFIG_PRIMARY_ALL_PHY_PARTITION_SIZE,        FWU_IMAGE_TYPE_FULL},
#endif
#if CONFIG_OTA_OVERWRITE
	{CONFIG_OTA_PHY_PARTITION_OFFSET,                CONFIG_OTA_PHY_PARTITION_SIZE,                FWU_IMAGE_TYPE_FULL},
#endif
	{CONFIG_SECONDARY_ALL_PHY_PARTITION_OFFSET,      CONFIG_SECONDARY_ALL_PHY_PARTITION_SIZE,      FWU_IMAGE_TYPE_FULL}
};

static void security_ota_dump_partition_info(void)
{
	BK_LOGI(TAG, "%8s  %8s  %8s\r\n", "offset", "size", "fwu_id");
	for (uint32_t partition_id = 0; partition_id < sizeof(s_ota_partition_info)/sizeof(ota_partition_info_t); partition_id++) {
		const ota_partition_info_t *p = &s_ota_partition_info[partition_id];
		BK_LOGI(TAG, "%8x  %8x  %-6d\r\n", p->partition_offset, p->partition_size, p->fwu_image_id);
	}
}

static const ota_partition_info_t* security_ota_get_partition_info(void)
{
	uint32_t flash_offset;

	if (ota_parse.phase != OTA_PARSE_IMG
		|| ota_parse.index >= ota_parse.ota_header.image_num) {
		return NULL;
	}

	flash_offset = ota_parse.ota_image_header[ota_parse.index].flash_offset;

	for (uint32_t partition_id = 0; partition_id < sizeof(s_ota_partition_info)/sizeof(ota_partition_info_t); partition_id++) {
		if (flash_offset == s_ota_partition_info[partition_id].partition_offset) {
			return &s_ota_partition_info[partition_id];
		}
	}

	return NULL;
}

static uint32_t security_ota_get_fwu_image_id(void)
{
	const ota_partition_info_t *partition_info = security_ota_get_partition_info();

	if (!partition_info) {
		return FWU_IMAGE_TYPE_INVALID;
	}

	//TODO for BL2, update image_id per boot_flag
	return partition_info->fwu_image_id;
}

static psa_image_id_t security_ota_fwu2psa_image_id(uint32_t fwu_image_id)
{
	return (psa_image_id_t)FWU_CALCULATE_IMAGE_ID(FWU_IMAGE_ID_SLOT_STAGE, fwu_image_id, 0);
}

int bk_ota_check(psa_image_id_t ota_image)
{
	psa_status_t status;
	psa_image_id_t dependency_uuid;
	psa_image_version_t dependency_version;
	psa_image_info_t info;

#if CONFIG_INT_WDT
	bk_wdt_stop();
	bk_task_wdt_stop();
#endif 

	status = psa_fwu_query(ota_image, &info);
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "query status %d\r\n", status);
		goto _ret_fail;
	}
	if (info.state != PSA_IMAGE_CANDIDATE) {
		BK_LOGE(TAG, "info state %d\r\n", info.state);
		goto _ret_fail;
	}

	status = psa_fwu_install(ota_image, &dependency_uuid, &dependency_version);
	if (status != PSA_SUCCESS_REBOOT) {
		BK_LOGE(TAG, "install fail %d\r\n", status);
		goto _ret_fail;
	}

	status = psa_fwu_query(ota_image, &info);
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "query fail %d\r\n", status);
		goto _ret_fail;
	}
	if (info.state != PSA_IMAGE_REBOOT_NEEDED) {
		BK_LOGE(TAG, "info fail %d\r\n", info.state);
		goto _ret_fail;
	}

#if CONFIG_INT_WDT
	bk_wdt_start(CONFIG_INT_WDT_PERIOD_MS);
#endif
	return 0;

_ret_fail:
#if CONFIG_INT_WDT
	wdt_init();
#endif
	return -1;
}

void bk_ota_set_flag(uint32_t flag)
{
	ota_image_flag |= flag;
}

uint32_t bk_ota_get_flag(void)
{
	return ota_image_flag;
}

void bk_ota_clear_flag(void)
{
	ota_image_flag = 0;
}

#endif // (CONFIG_TFM_FWU)

// extern void sys_hal_set_ota_finish();
void bk_ota_accept_image(void)
{
#if CONFIG_TFM_FWU
	int32_t ns_interface_lock_init(void);
	psa_image_id_t psa_image_id = (psa_image_id_t)FWU_CALCULATE_IMAGE_ID(FWU_IMAGE_ID_SLOT_ACTIVE, FWU_IMAGE_TYPE_FULL, 0);
	BK_LOGI(TAG, "accept image\r\n");
	ns_interface_lock_init();
	psa_fwu_accept(psa_image_id);
	sys_hal_set_ota_finish(0);
#elif (!defined(CONFIG_TFM_FWU)) && (CONFIG_DIRECT_XIP)
	extern uint32_t flash_get_excute_enable();
	extern void bk_flash_write_xip_status(uint32_t fa_id, uint32_t type, uint32_t status);
	uint32_t update_id = flash_get_excute_enable();
	bk_flash_write_xip_status(update_id,XIP_IMAGE_OK_TYPE, XIP_ACTIVE);
	update_id ^= 1;
	bk_flash_write_xip_status(update_id,XIP_IMAGE_OK_TYPE,XIP_BACK);
	sys_hal_set_ota_finish(0);
#endif
}

static int security_ota_parse_header(uint8_t **data, int *len);
static int security_ota_parse_image_header(uint8_t **data, int *len);

static int security_ota_parse_header(uint8_t **data, int *len)
{
	uint32_t data_len, offset;
	uint8_t *tmp;

	if (*len == 0) return 0;

	if (ota_parse.offset == 0) {
		BK_LOGI(TAG, "downloading OTA global header...\r\n");
	}

	tmp = (uint8_t *)&ota_parse.ota_header;
	data_len = sizeof(ota_header_t) - ota_parse.offset;
	if (*len < data_len) {
		os_memcpy(tmp + ota_parse.offset, *data, *len);
		ota_parse.offset += *len;
		s_restart += *len;
		return 0;
	} else {
		os_memcpy(tmp + ota_parse.offset, *data, data_len);
		*data += data_len;
		*len -= data_len;
		s_restart += data_len;

		//check global header magic code!
		if(os_memcmp(OTA_MAGIC_WORD,tmp,8) != 0){
			BK_LOGE(TAG, "magic error\r\n");
			return BK_ERR_OTA_HDR_MAGIC;
		}

		/*calculate global header crc*/
		offset = sizeof(ota_parse.ota_header.magic) + sizeof(ota_parse.ota_header.crc);
		tmp += offset;
		CRC32_Update(&ota_parse.ota_crc, tmp, sizeof(ota_header_t) - offset);

		/*to next parse*/
		ota_parse.phase = OTA_PARSE_IMG_HEADER;
		ota_parse.offset = 0;
		if (ota_parse.ota_image_header) {
			os_free(ota_parse.ota_image_header);
		}
		offset = ota_parse.ota_header.image_num * sizeof(ota_image_header_t);
		ota_parse.ota_image_header = (ota_image_header_t *)os_malloc(offset);
		if (!ota_parse.ota_image_header) {
			BK_LOGE(TAG, "ota parse image header: oom\r\n");
			return BK_ERR_OTA_OOM;
		}
		BK_LOGI(TAG, "crc %x, version %x, header_len %x, image_num %x\r\n",
			ota_parse.ota_header.crc, ota_parse.ota_header.version, ota_parse.ota_header.header_len, ota_parse.ota_header.image_num);
	}

	return 0;
}

static int security_ota_parse_image_header(uint8_t **data, int *len)
{
	int i;
	uint32_t data_len, offset, crc_control;
	uint8_t *tmp;

	if (*len == 0) return 0;

	if (ota_parse.offset == 0) {
		BK_LOGI(TAG, "downloading OTA image header...\r\n");
	}

	tmp = (uint8_t *)ota_parse.ota_image_header;
	data_len = ota_parse.ota_header.image_num * sizeof(ota_image_header_t) - ota_parse.offset;
	if (*len < data_len) {
		os_memcpy(tmp + ota_parse.offset, *data, *len);
		ota_parse.offset += *len;
		s_restart += *len;
		return 0;
	} else {
		os_memcpy(tmp + ota_parse.offset, *data, data_len);
		*data += data_len;
		*len -= data_len;
		s_restart += data_len;

		/*calculate header crc*/
		offset = ota_parse.ota_header.image_num * sizeof(ota_image_header_t);
		CRC32_Update(&ota_parse.ota_crc, tmp, offset);

		//TODO check image CRC!
		CRC32_Final(&ota_parse.ota_crc,&crc_control);
		if(crc_control != ota_parse.ota_header.crc){
			BK_LOGE(TAG, "crc error\r\n");
			return BK_ERR_OTA_IMG_HDR_CRC;
		}

		/*to next parse*/
		ota_parse.phase = OTA_PARSE_IMG;
		ota_parse.offset = 0;
		for (i = 0; i < ota_parse.ota_header.image_num; i++) {
			BK_LOGI(TAG, "image[%d], image_len=%x, image_offset=%x, flash_offset=%x\r\n", i,
				ota_parse.ota_image_header[i].image_len,
				ota_parse.ota_image_header[i].image_offset,
				ota_parse.ota_image_header[i].flash_offset);
		}
	}

	return 0;
}

uint32_t get_http_flash_wr_buf_max(void);
static void security_ota_write_flash(uint8_t **data, uint32_t len, uint32_t psa_image_id)
{
	uint32_t copy_len;
	uint32_t data_idx = 0;
	uint32_t http_flash_wr_buf_max = get_http_flash_wr_buf_max();

	while(data_idx < len){
		copy_len = min(len-(data_idx),http_flash_wr_buf_max - bk_http_ptr->wr_last_len);
		os_memcpy(bk_http_ptr->wr_buf + bk_http_ptr->wr_last_len, *data + data_idx, copy_len);
		data_idx += copy_len;
		bk_http_ptr->wr_last_len += copy_len;
		if(bk_http_ptr->wr_last_len >= http_flash_wr_buf_max){
#if CONFIG_TFM_FWU
			psa_fwu_write(psa_image_id, ota_parse.write_offset, (const void *)bk_http_ptr->wr_buf, http_flash_wr_buf_max);
#else
			extern void bk_flash_ota_update(uint32_t off, const void *src, uint32_t len);
			bk_flash_ota_update(ota_parse.write_offset, (const void *)bk_http_ptr->wr_buf, http_flash_wr_buf_max);
#endif
			ota_parse.write_offset += http_flash_wr_buf_max;
			bk_http_ptr->wr_last_len = 0;
		}
	}
}

#if CONFIG_VALIDATE_BEFORE_REBOOT
static uint32_t count_bit_one_in_byte(uint8_t byte)
{
	uint32_t counter = 0;
	while(byte) {
		counter += byte & 1;
		byte >>= 1;
	}
	return counter;
}

static uint32_t count_bit_one_in_buffer(uint8_t* buffer, size_t length)
{
	uint32_t counter = 0;
	uint32_t temp;
	for(size_t i=0; i < length; ++i){
		temp = count_bit_one_in_byte(buffer[i]);
		counter += temp;
		if(temp == 0)
			break;
	}
	return counter;
}

static void set_bit_one_in_buffer(uint8_t* buffer, size_t value)
{
	size_t byte_index = 0;
	while(value > 0){
		if(value > 8){
			buffer[byte_index] = 0xFF;
			value -= 8;
		} else {
			buffer[byte_index] |= (0xFF >> (8 - value));
			value = 0;
		}
		byte_index++;
	}
}

static int mbedtls_import_key(uint8_t **cp, uint8_t *end)
{
	size_t len;
	mbedtls_asn1_buf alg;
	mbedtls_asn1_buf param;
	if (mbedtls_asn1_get_tag(cp, end, &len,
		MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE)) {
		return -1;
	}
	end = *cp + len;

	/* ECParameters (RFC5480) */
	if (mbedtls_asn1_get_alg(cp, end, &alg, &param)) {
		return -2;
	}
	/* id-ecPublicKey (RFC5480) */
	if (alg.len != sizeof(ec_pubkey_oid) - 1 ||
		memcmp(alg.p, ec_pubkey_oid, sizeof(ec_pubkey_oid) - 1)) {
		return -3;
	}
	/* namedCurve (RFC5480) */
	if (param.len != sizeof(ec_secp256r1_oid) - 1 ||
		memcmp(param.p, ec_secp256r1_oid, sizeof(ec_secp256r1_oid) - 1)) {
		return -4;
	}
	/* ECPoint (RFC5480) */
	if (mbedtls_asn1_get_bitstring_null(cp, end, &len)) {
		return -6;
	}
	if (*cp + len != end) {
		return -7;
	}

	if (len != 2 * NUM_ECC_BYTES + 1) {
		return -8;
	}
	return 0;
}

static int mbedtls_ecdsa_p256_verify(mbedtls_ecdsa_context *ctx,
											uint8_t *pk, size_t pk_len,
											uint8_t *hash,
											uint8_t *sig, size_t sig_len)
{
	int rc = -1;

	(void)sig;
	(void)hash;
#if CONFIG_PSA_MBEDTLS
	rc = mbedtls_ecp_group_load(&ctx->MBEDTLS_PRIVATE(grp), MBEDTLS_ECP_DP_SECP256R1);
#elif CONFIG_MBEDTLS
	rc = mbedtls_ecp_group_load(&ctx->grp, MBEDTLS_ECP_DP_SECP256R1);
#endif

	if (rc) {
		return -1;
	}
#if CONFIG_PSA_MBEDTLS
	rc = mbedtls_ecp_point_read_binary(&ctx->MBEDTLS_PRIVATE(grp), &ctx->MBEDTLS_PRIVATE(Q), pk, pk_len);
#elif CONFIG_MBEDTLS
	rc = mbedtls_ecp_point_read_binary(&ctx->grp, &ctx->Q, pk, pk_len);
#endif
	if (rc) {
		return -1;
	}

#if CONFIG_PSA_MBEDTLS
	rc = mbedtls_ecp_check_pubkey(&ctx->MBEDTLS_PRIVATE(grp), &ctx->MBEDTLS_PRIVATE(Q));
#elif CONFIG_MBEDTLS
	rc = mbedtls_ecp_check_pubkey(&ctx->grp, &ctx->Q);
#endif

	if (rc) {
		return -1;
	}

	rc = mbedtls_ecdsa_read_signature(ctx, hash, 32,
									sig, sig_len);
	if (rc) {
		return -1;
	}

	return 0;
}

extern bk_err_t bk_flash_read_cbus(uint32_t address, void *user_buf, uint32_t size);
extern void bk_flash_write_cbus(uint32_t address, const uint8_t *user_buf, uint32_t size);

#define CHECK_READ_CBUS(call) \
	do { \
		if((call) != BK_OK) { \
			return BK_ERR_OTA_CBUS_READ_CRC_FAIL; \
		} \
	} while(0)

static bk_err_t read_pubkey_from_primary(uint8_t* pubkey, uint16_t* pubkey_size)
{
	uint32_t key_len = 0;
	image_header_t primary_hdr;

	uint32_t fa_off = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET));
	CHECK_READ_CBUS(bk_flash_read_cbus(fa_off, &primary_hdr, sizeof(image_header_t)));
	uint32_t tlv_begin = primary_hdr.ih_hdr_size + primary_hdr.ih_img_size;
	uint32_t offset = 0;
	image_tlv_t tlv;

	while(offset < TLV_TOTAL_SIZE) {
		CHECK_READ_CBUS(bk_flash_read_cbus(fa_off + tlv_begin + offset, (void *)&tlv, sizeof(image_tlv_t)));

		if (tlv.it_type == IMAGE_TLV_PROT_INFO_MAGIC || tlv.it_type == IMAGE_TLV_INFO_MAGIC) {
			offset += sizeof(image_tlv_t);
		} else if (tlv.it_type != IMAGE_TLV_CUSTOM) {
			offset += (sizeof(image_tlv_t) + tlv.it_len);
		} else {
			offset += sizeof(image_tlv_t);
			key_len = tlv.it_len;
			break;
		}
	}

	if (offset > TLV_TOTAL_SIZE || key_len > MAX_PUBKEY_LEN) {
		return BK_FAIL;
	}

	CHECK_READ_CBUS(bk_flash_read_cbus(fa_off + tlv_begin + offset, pubkey, key_len));

	if (pubkey_size != NULL) {
		*pubkey_size = key_len;
	}

	return BK_OK;
}

bk_err_t update_back_pubkey_from_primary(void)
{
	uint32_t phy_off = CONFIG_PUBLIC_KEY_PHY_PARTITION_OFFSET;
	uint32_t vir_off = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(phy_off + 3)); 
	uint16_t vir_length;
	uint8_t* back_key;
	uint8_t primary_key[MAX_PUBKEY_LEN] = {0};
	uint16_t primary_key_size = 0;
	CRC8_Context crc8;
	CRC8_Context check_crc;
	uint8_t *enc_buf;
	uint8_t check_erase_buf[64];
	uint8_t ff_buf[64];
	bk_err_t ret = BK_OK;

	ret = read_pubkey_from_primary(primary_key, &primary_key_size);
	if (ret != BK_OK) {
		return ret;
	}
	security_ota_set_flash_protect_type(FLASH_PROTECT_NONE);

	memset(ff_buf, 0xFF, 64);
	bk_flash_read_bytes(phy_off, check_erase_buf, 64);
	if (memcmp(check_erase_buf, ff_buf, 64) != 0) {
		bk_flash_read_bytes(phy_off, (uint8_t *)&vir_length, 2);
		bk_flash_read_bytes(phy_off + 2, (uint8_t *)&check_crc, 1);
		uint16_t phy_length = ((vir_length + 31) >> 5) * 34; //ceil align 34

		enc_buf = (uint8_t*)malloc(phy_length);
		if (!enc_buf) {
			security_ota_set_flash_protect_type(FLASH_PROTECT_ALL);
			return BK_ERR_OTA_OOM;
		}

		bk_flash_read_bytes(phy_off + 3, enc_buf, phy_length);
		CRC8_Init(&crc8);
		CRC8_Update(&crc8, enc_buf, phy_length);
		free(enc_buf);

		if (crc8.crc == check_crc.crc) {
			back_key = (uint8_t *)malloc(vir_length);
			if (!back_key) {
				security_ota_set_flash_protect_type(FLASH_PROTECT_ALL);
				return BK_ERR_OTA_OOM;
			}

			CHECK_READ_CBUS(bk_flash_read_cbus(vir_off, back_key, vir_length));
			if (memcmp(back_key, primary_key, primary_key_size) == 0) {
				free(back_key);
				security_ota_set_flash_protect_type(FLASH_PROTECT_ALL);
				return BK_OK;
			}
			free(back_key);
		}
	}

	bk_flash_erase_sector_ota(phy_off);
	uint16_t phy_key_size = ((primary_key_size + 31) >> 5) * 34; //ceil align 34
	uint16_t vir_key_size = FLASH_PHY2VIRTUAL(phy_key_size); // vir_key_size - keysize = data 0
	bk_flash_write_cbus(vir_off, primary_key, vir_key_size);
	enc_buf = (uint8_t*)malloc(phy_key_size);
	bk_flash_read_bytes(phy_off + 3, (uint8_t *)enc_buf, phy_key_size);
	CRC8_Init(&crc8);
	CRC8_Update(&crc8, enc_buf, phy_key_size);
	bk_flash_write_bytes_ota(phy_off + 2, (uint8_t *)&crc8, 1);
	bk_flash_write_bytes_ota(phy_off, (uint8_t *)&primary_key_size, 2);
	free(enc_buf);

	// re-read to ensure update success
	back_key = (uint8_t *)malloc(primary_key_size);
	CHECK_READ_CBUS(bk_flash_read_cbus(vir_off, back_key, primary_key_size));
	bk_flash_read_bytes(phy_off + 2, (uint8_t *)&check_crc, 1);
	security_ota_set_flash_protect_type(FLASH_PROTECT_ALL);

	if ((crc8.crc == check_crc.crc) && (memcmp(back_key, primary_key, primary_key_size) == 0)) {
		BK_LOGI(TAG, "update back pubkey ok\r\n");
		ret = BK_OK;
	} else {
		ret = BK_FAIL;
		BK_LOGE(TAG, "back pubkey failed\r\n");
	}

	free(back_key);
	return ret;
}

static void mbedtls_sha256_hash(uint8_t *digest, uint32_t digest_sz, uint8_t *hash_result)
{
	mbedtls_sha256_context sha256_ctx;
	mbedtls_sha256_init(&sha256_ctx);
	mbedtls_sha256_starts(&sha256_ctx,0);
	mbedtls_sha256_update(&sha256_ctx, digest, digest_sz);
	mbedtls_sha256_finish(&sha256_ctx, hash_result);
	mbedtls_sha256_free(&sha256_ctx);
}

static bk_err_t http_ota_handle_tlv()
{
	uint32_t tlv_off = 0;
	int ret = 0;
	bool image_hash_valid = false;
	bool valid_signature = false;
	bool valid_public_key = false;
	image_tlv_info_t tlv_pro_info,tlv_info;
	image_tlv_t tlv;
	uint8_t pubkey[MAX_PUBKEY_LEN] = {0};
	uint8_t *end = NULL;
	uint8_t* value = NULL;

#ifndef CONFIG_OTA_UPDATE_PUBKEY
	uint8_t key_hash[32];
#endif // !CONFIG_OTA_UPDATE_PUBKEY

	memcpy(&tlv_pro_info, ota_parse.tlv_total + tlv_off, sizeof(image_tlv_info_t));
	tlv_off += sizeof(image_tlv_info_t);
	memcpy(&tlv_info, ota_parse.tlv_total + tlv_pro_info.it_tlv_tot, sizeof(image_tlv_info_t));

	if(tlv_pro_info.it_magic != IMAGE_TLV_PROT_INFO_MAGIC || tlv_info.it_magic != IMAGE_TLV_INFO_MAGIC){
		BK_LOGE(TAG, "tlv magic error!\r\n");
		return BK_ERR_OTA_VALIDATE_TLV_FAIL;
	}

	if(tlv_pro_info.it_tlv_tot +  tlv_info.it_tlv_tot > TLV_TOTAL_SIZE){
		BK_LOGE(TAG, "tlv size error!\r\n");
		return BK_ERR_OTA_VALIDATE_TLV_FAIL;
	}

	while(tlv_off < tlv_pro_info.it_tlv_tot + tlv_info.it_tlv_tot) {
		if(tlv_off == tlv_pro_info.it_tlv_tot){
			tlv_off += sizeof(image_tlv_info_t);
		}
		memcpy(&tlv, ota_parse.tlv_total + tlv_off, sizeof(image_tlv_t));
		tlv_off += sizeof(image_tlv_t);
		value = (uint8_t*)os_malloc(tlv.it_len);
		if (value == NULL) {
			BK_LOGE(TAG, "OOM tlv_len=%d\r\n", tlv.it_len);
			ret =  BK_ERR_OTA_OOM;
			goto out;
		}
		memcpy(value, ota_parse.tlv_total + tlv_off, tlv.it_len);
		tlv_off += tlv.it_len;

		if(tlv.it_type == IMAGE_TLV_SEC_CNT){
#if CONFIG_ANTI_ROLLBACK
			uint32_t img_sec_counter;
			if(tlv.it_len != sizeof(img_sec_counter)){
				BK_LOGE(TAG, "security counter error!\r\n");
				ret = BK_ERR_OTA_VALIDATE_SEC_COUNTER_FAIL;
				goto out;
			}
			memcpy(&img_sec_counter,value,sizeof(img_sec_counter));
			uint8_t security_counter[64];
			bk_otp_apb_read(OTP_BL2_SECURITY_COUNTER, security_counter, 64);
			uint32_t store_sec_counter = count_bit_one_in_buffer(security_counter,64);
			if(img_sec_counter < store_sec_counter){
				BK_LOGE(TAG, "security counter error!\r\n");
				ret = BK_ERR_OTA_VALIDATE_SEC_COUNTER_FAIL;
				goto out;
			} else if(img_sec_counter > store_sec_counter){
				if(img_sec_counter > 64*8){
					BK_LOGE(TAG, "security counter overflow!\r\n");
					ret = BK_ERR_OTA_VALIDATE_SEC_COUNTER_FAIL;
					goto out;
				}
				set_bit_one_in_buffer(security_counter, img_sec_counter);
				bk_otp_apb_update(OTP_BL2_SECURITY_COUNTER, security_counter, 64);
			}
#endif

		} else if(tlv.it_type == IMAGE_TLV_SHA256){
			if(tlv.it_len != sizeof(ota_parse.hash_result)){
				ret = BK_ERR_OTA_VALIDATE_IMG_HASH_FAIL;
				goto out;
			}
			if(memcmp(value,ota_parse.hash_result,32) != 0){
				BK_LOGE(TAG, "hash error!\r\n");
				ret = BK_ERR_OTA_VALIDATE_IMG_HASH_FAIL;
				goto out;
			}
			image_hash_valid = true;
			BK_LOGI(TAG, "image hash ok\r\n");
		} else if(tlv.it_type == IMAGE_TLV_CUSTOM){
			memcpy(pubkey, value, tlv.it_len);
			end = pubkey + tlv.it_len;
#if CONFIG_OTA_UPDATE_PUBKEY
			uint8_t primary_pubkey[MAX_PUBKEY_LEN];
			ret = read_pubkey_from_primary(primary_pubkey, NULL);
			if (ret != BK_OK) {
				BK_LOGE(TAG, "primary read public key fail!\r\n");
				goto out;
			}

			if (memcmp(primary_pubkey, pubkey, tlv.it_len) != 0){
				BK_LOGE(TAG, "public key of OTA-outer and primary not equal!\r\n");
				ret = BK_ERR_OTA_VALIDATE_PUB_KEY_FAIL;
				goto out;
			}
#else
			mbedtls_sha256_hash(value,tlv.it_len,key_hash);
			uint8_t pubkey_hash[32];
			bk_otp_apb_read(OTP_BL2_BOOT_PUBLIC_KEY_HASH, pubkey_hash, 32);
			if(memcmp(key_hash, pubkey_hash, 32) != 0){
				BK_LOGE(TAG, "incorrect public key hash!\r\n");
				ret = BK_ERR_OTA_VALIDATE_PUB_KEY_FAIL;
				goto out;
			}
#endif
			valid_public_key = true;
			BK_LOGI(TAG, "public key ok\r\n");
		} else if(tlv.it_type == IMAGE_TLV_ECDSA256) {
			if (valid_public_key == false) {
				BK_LOGE(TAG, "OTA has no public key!\r\n");
				ret = BK_ERR_OTA_VALIDATE_PUB_KEY_FAIL;
				goto out;
			}

			mbedtls_ecdsa_context ctx;
			mbedtls_ecdsa_init(&ctx);
			uint8_t *ppubkey = pubkey;
			ret = mbedtls_import_key(&ppubkey, end);
			if (ret != 0){
				BK_LOGE(TAG, "import key signature error!\r\n");
				ret = BK_ERR_OTA_VALIDATE_SIGNATURE_FAIL;
				goto out;
			}

			ret = mbedtls_ecdsa_p256_verify(&ctx, ppubkey, end - ppubkey, ota_parse.hash_result, value, tlv.it_len);
			if (ret != 0){
				BK_LOGE(TAG, "verify signature error!\r\n");
				ret = BK_ERR_OTA_VALIDATE_SIGNATURE_FAIL;
				goto out;
			}
			mbedtls_ecdsa_free(&ctx);
			valid_signature = true;
			BK_LOGI(TAG, "sig ok\r\n");
		}
		os_free(value);
		value = NULL;
	}

	if(image_hash_valid == false) {
		BK_LOGE(TAG, "invalid image hash!\r\n");
		ret = BK_ERR_OTA_VALIDATE_IMG_HASH_FAIL;
	} else if(valid_signature == false) {
		BK_LOGE(TAG, "invalid sig!\r\n");
		ret = BK_ERR_OTA_VALIDATE_SIGNATURE_FAIL;
	}

out:
	if(value != NULL){
		os_free(value);
		value = NULL;
	}
	end = NULL;
	return ret;
}

#endif

static int security_ota_handle_image(uint8_t **data, int *len)
{
	uint32_t image_crc;

	vPortDisableTimerInterrupt();

	do {
#if CONFIG_INT_WDT
		bk_wdt_feed();
#endif
#if CONFIG_TASK_WDT
		extern void bk_task_wdt_feed(void);
		bk_task_wdt_feed();
#endif
		if (ota_parse.offset == 0) {
			BK_LOGI(TAG, "downloading OTA image%d, expected data len=%x...\r\n", ota_parse.index, ota_parse.ota_image_header[ota_parse.index].image_len);
			ota_parse.percent = 0;
			CRC32_Init(&ota_parse.ota_crc);
#if CONFIG_VALIDATE_BEFORE_REBOOT
			mbedtls_sha256_init(&ota_parse.ctx);
			mbedtls_sha256_starts(&ota_parse.ctx,0);
			ota_parse.hash_size = 0xFFFFFFFF;
			ota_parse.validate_offset = 0;
			memset(&ota_parse.tlv_total,0xFF,TLV_TOTAL_SIZE);
#endif
		}
#if CONFIG_TFM_FWU
		uint32_t fwu_image_id = security_ota_get_fwu_image_id();
		if (fwu_image_id == FWU_IMAGE_TYPE_INVALID) {
			if (*len) {
				BK_LOGE(TAG, "Invalid image ID, parse index=%d, parse offset=%x, len=%d, total_rx_len=%x\r\n",
					ota_parse.index, ota_parse.offset, *len, ota_parse.total_rx_len);
					vPortEnableTimerInterrupt();
				return BK_ERR_OTA_IMG_ID;
			}
			vPortEnableTimerInterrupt();
			return BK_OK;
		}

		bk_ota_set_flag(BIT(fwu_image_id));
		psa_image_id_t psa_image_id = security_ota_fwu2psa_image_id(fwu_image_id);
#else
		uint32_t psa_image_id = 0;
#endif
		uint32_t image_len = ota_parse.ota_image_header[ota_parse.index].image_len;
		uint32_t data_len = image_len - ota_parse.offset;

		if (ota_parse.offset >= (image_len/10 + image_len*ota_parse.percent/100)) {
			ota_parse.percent += 10;
			if (ota_parse.percent < 100) {
				BK_LOGI(TAG, "downloading %d%%\r\n", ota_parse.percent);
			}
		}

#if CONFIG_VALIDATE_BEFORE_REBOOT
		int head_left_len = sizeof(image_header_t) - ota_parse.validate_offset;
		if(head_left_len > 0){
			uint8_t * tmp = (uint8_t *)&ota_parse.hdr;
			if (*len < head_left_len) {
				os_memcpy(tmp + ota_parse.validate_offset, *data, *len);
			} else {
				os_memcpy(tmp + ota_parse.validate_offset, *data, head_left_len);
			}
		} else {
			ota_parse.hash_size = ota_parse.hdr.ih_hdr_size + ota_parse.hdr.ih_img_size + ota_parse.hdr.ih_protect_tlv_size;
			uint32_t tlv_info_begin = ota_parse.hdr.ih_hdr_size + ota_parse.hdr.ih_img_size;
			uint32_t tlv_info_end = tlv_info_begin + TLV_TOTAL_SIZE;
			if(ota_parse.validate_offset + *len >= tlv_info_begin && ota_parse.validate_offset < tlv_info_end) {
				if(ota_parse.validate_offset < tlv_info_begin){
					if(ota_parse.validate_offset + *len > tlv_info_end){
						os_memcpy(ota_parse.tlv_total, *data + (tlv_info_begin - ota_parse.validate_offset), TLV_TOTAL_SIZE);
					} else {
						os_memcpy(ota_parse.tlv_total, *data + (tlv_info_begin - ota_parse.validate_offset), ota_parse.validate_offset + *len - tlv_info_begin);
					}
				} else {
					os_memcpy(ota_parse.tlv_total + ota_parse.validate_offset - tlv_info_begin, *data, MIN(tlv_info_end-ota_parse.validate_offset,*len));
				}
			}
		}
		if (ota_parse.validate_offset + *len <= ota_parse.hash_size){
			mbedtls_sha256_update(&ota_parse.ctx, *data, *len);
		} else if(ota_parse.validate_offset < ota_parse.hash_size){
			mbedtls_sha256_update(&ota_parse.ctx, *data, ota_parse.hash_size - ota_parse.validate_offset);
		}
		ota_parse.validate_offset += *len;
#endif

		if (*len < data_len) {
			security_ota_write_flash(data, *len, psa_image_id);
			CRC32_Update(&ota_parse.ota_crc,*data,*len);
			ota_parse.offset += *len;
			s_restart += *len;
			*len = 0;
		} else {
			security_ota_write_flash(data, *len, psa_image_id);
			uint32_t align_len = (((bk_http_ptr->wr_last_len) + ((32) - 1)) & ~((32) - 1));  // align_up 32
			memset(bk_http_ptr->wr_buf + bk_http_ptr->wr_last_len,0xFF,align_len - bk_http_ptr->wr_last_len);
#if CONFIG_TFM_FWU
			psa_fwu_write(psa_image_id, ota_parse.write_offset, (const void *)bk_http_ptr->wr_buf, align_len);
#else
			extern void bk_flash_ota_update(uint32_t off, const void *src, uint32_t len);
			bk_flash_ota_update(ota_parse.write_offset, (const void *)bk_http_ptr->wr_buf, align_len);
#endif
			bk_http_ptr->wr_last_len = 0;
			CRC32_Update(&ota_parse.ota_crc,*data,*len);
			s_restart += *len;
			*data += data_len;
			*len = 0;

			BK_LOGI(TAG, "downloaded OTA image%d\r\n", ota_parse.index);
			//check image CRC, then we can abort quickly!
			CRC32_Final(&ota_parse.ota_crc,&image_crc);
			if(image_crc !=  ota_parse.ota_image_header[ota_parse.index].checksum){
				BK_LOGE(TAG, "image crc error!\r\n");
				vPortEnableTimerInterrupt();
				return BK_ERR_OTA_IMG_CRC;
			}

#if CONFIG_VALIDATE_BEFORE_REBOOT
			mbedtls_sha256_finish(&ota_parse.ctx, ota_parse.hash_result);
			mbedtls_sha256_free(&ota_parse.ctx);
			int ret = http_ota_handle_tlv();
			if (ret != 0) {
				BK_LOGE(TAG, "validate image fail!\r\n");
				vPortEnableTimerInterrupt();
				return ret;
			}
#endif
			/*to next image*/
			BK_LOGI(TAG, "\r\n");
			ota_parse.index++;
			ota_parse.offset = 0;
			ota_parse.write_offset = 0;
		}
	} while(*len);

	vPortEnableTimerInterrupt();

	return BK_OK;
}

int security_ota_parse_data(char *data, int len)
{
	int ret = BK_OK;

	ota_parse.total_rx_len += len;
	if (ota_parse.phase == OTA_PARSE_HEADER) {
		ret = security_ota_parse_header((uint8_t **)&data, &len);
		if (ret != BK_OK) {
			goto end;
		}
	}

	if (ota_parse.phase == OTA_PARSE_IMG_HEADER) {
		ret = security_ota_parse_image_header((uint8_t **)&data, &len);
		if (ret != BK_OK) {
			goto end;
		}
	}

	if (ota_parse.phase == OTA_PARSE_IMG) {
		if (len == 0) return BK_OK;

		ret = security_ota_handle_image((uint8_t **)&data, &len);
		if (ret != BK_OK) {
			goto end;
		}
	}

	return BK_OK;

end:
	s_ota_finish_flag = false;
#if CONFIG_OTA_HTTPS
	security_ota_dispatch_event(SECURITY_OTA_ERROR, &(ret), sizeof(int));
#endif
	return ret;
}

extern void bk_ota_erase(void);

static int security_ota_deinit_no_flash_protected(void)
{
	if (ota_parse.ota_image_header) {
		os_free(ota_parse.ota_image_header);
		ota_parse.ota_image_header = NULL;
	}

	os_free(bk_http_ptr->wr_buf);
	bk_http_ptr->wr_buf = NULL;
	s_restart = 0;
	s_ota_running_flag = false;
	if (s_ota_finish_flag == false) {
#if CONFIG_TFM_FWU
		psa_image_id_t id = security_ota_fwu2psa_image_id(FWU_IMAGE_TYPE_FULL);
		psa_fwu_abort(id);
#else
		bk_ota_erase();
#endif
		return BK_FAIL;
	}

	return 0;
}

int security_ota_deinit(void)
{
	int ret = security_ota_deinit_no_flash_protected();
	security_ota_set_flash_protect_type(FLASH_PROTECT_ALL);
	return ret;
}

void security_ota_init(const char* https_url)
{
	security_ota_set_flash_protect_type(FLASH_PROTECT_NONE);
	static char *last_https_url = NULL;
	if (last_https_url == NULL) {
		last_https_url = os_malloc(strlen(https_url) + 1);
		if (last_https_url != NULL) {
			strcpy(last_https_url, https_url);
		}
	}
	else {
		if(strcmp(last_https_url, https_url) != 0){
			s_ota_finish_flag = false;
			security_ota_deinit_no_flash_protected();
			os_free(last_https_url);
			last_https_url = os_malloc(strlen(https_url) + 1);
			if (last_https_url != NULL) {
				strcpy(last_https_url, https_url);
			}
		}
	}
	s_ota_running_flag = true;
	if (s_restart != 0) {
#if CONFIG_OTA_HTTPS
	security_ota_dispatch_event(SECURITY_OTA_RESTART,NULL,0);
#endif
	return ;
	}
	s_ota_finish_flag = true;
#if CONFIG_TFM_FWU
	security_ota_dump_partition_info();
	bk_ota_clear_flag();
#else
	bk_ota_erase();
#if CONFIG_DIRECT_XIP
	extern void bk_flash_ota_write_magic(void);
	bk_flash_ota_write_magic();
#endif
#if CONFIG_OTA_HTTPS
	security_ota_dispatch_event(SECURITY_OTA_START,NULL,0);
#endif
#endif
	os_memset(&ota_parse, 0, sizeof(ota_parse_t));
	CRC32_Init(&ota_parse.ota_crc);

	if (!bk_http_ptr->wr_buf) {
		bk_http_ptr->wr_buf = (uint8_t*)os_malloc(4096 * sizeof(char));
	}
	bk_http_ptr->wr_last_len = 0;
}

int security_ota_finish(void)
{
	if(security_ota_deinit() != 0){
		return BK_FAIL;
	}

#if (CONFIG_TFM_FWU)
	int ret;
	psa_image_id_t psa_image_id = 0;
	uint8_t fwu_image_id = 0;

	uint32_t ota_flags = bk_ota_get_flag();
	while (ota_flags) {
		if (ota_flags & 1) {
#if CONFIG_DIRECT_XIP
			/*when run in B cannot read A,so don't check A*/
			BK_LOGI(TAG, "reboot\r\n");
			sys_hal_set_ota_finish(1);
			psa_fwu_request_reboot();
#endif
			BK_LOGI(TAG, "checking fwu image%d...\r\n", fwu_image_id);
			psa_image_id = (psa_image_id_t)FWU_CALCULATE_IMAGE_ID(FWU_IMAGE_ID_SLOT_STAGE, fwu_image_id, 0);
			ret = bk_ota_check(psa_image_id);
			if (ret != BK_OK) {
				BK_LOGI(TAG, "check fwu image%d failed\r\n", fwu_image_id);
				return BK_FAIL;
			} else {
				BK_LOGI(TAG, "check fwu image%d success\r\n", fwu_image_id);
			}
		}
		ota_flags >>= 1;
		fwu_image_id++;
	}
	sys_hal_set_ota_finish(1);
#ifndef CONFIG_OTA_CONFIRM_UPDATE
	BK_LOGI(TAG, "reboot\r\n");
	psa_fwu_request_reboot();
#else
	security_ota_dispatch_event(SECURITY_OTA_SUCCESS,NULL,0);
#endif
#else // CONFIG_TFM_FWU
#if CONFIG_DIRECT_XIP
		sys_hal_set_ota_finish(1);
		extern uint32_t flash_get_excute_enable();
		extern void bk_flash_write_xip_status(uint32_t fa_id, uint32_t type, uint32_t status);
		uint32_t update_id = (flash_get_excute_enable() ^ 1);
		bk_flash_write_xip_status(update_id,XIP_COPY_DONE_TYPE,XIP_SET);
#endif // CONFIG_DIRECT_XIP
	security_ota_dispatch_event(SECURITY_OTA_SUCCESS,NULL,0);
#ifndef CONFIG_VALIDATE_BEFORE_REBOOT
	bk_reboot_ex(RESET_SOURCE_OTA_REBOOT);
#endif
#endif /* CONFIG_TFM_FWU*/
	return BK_OK;
}

#if CONFIG_OTA_CONFIRM_UPDATE
bk_err_t bk_ota_confirm_update()
{
	if (s_ota_running_flag){
		return BK_FAIL;
	}
#if CONFIG_TFM_FWU
	bk_flash_set_protect_type(FLASH_PROTECT_NONE);
	psa_fwu_confirm(1);
	bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
	return BK_OK;
#else
	sys_hal_set_ota_finish(1);
	extern bk_err_t bk_flash_ota_write_confirm(uint32_t status);
	bk_err_t ret = bk_flash_ota_write_confirm(OVERWRITE_CONFIRM);
	return ret;
#endif
}

bk_err_t bk_ota_cancel_update(uint8_t erase_ota)
{
	if (s_ota_running_flag){
		return BK_FAIL;
	}

#if CONFIG_TFM_FWU
	bk_flash_set_protect_type(FLASH_PROTECT_NONE);
	psa_fwu_confirm(0);
	bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
	return BK_OK;
#else
	extern bk_err_t bk_flash_ota_erase_confirm();
	bk_err_t ret = bk_flash_ota_erase_confirm();
	sys_hal_set_ota_finish(0);

	if (erase_ota == 1) {
		security_ota_set_flash_protect_type(FLASH_PROTECT_NONE);
		bk_ota_erase();
		security_ota_set_flash_protect_type(FLASH_PROTECT_ALL);
	} 
	return ret;
#endif
}
#endif /*CONFIG_OTA_CONFIRM_UPDATE*/
