#include <inttypes.h>
#include <driver/flash.h>
#include "flash_partition.h"
#include <stdlib.h>
#include <string.h>
#include "region_defs.h"
#include "bootutil/bootutil_log.h"
#include "flash_map/flash_map.h"
#include "decompress_bl2_reset_test.h"

#define TOVIRTURE(addr) ((addr)%34+(addr)/34*32)
#define TOPHY(addr) ((addr)%32+(addr)/32*34)
#define CEIL_ALIGN_34(addr) (((addr) + 34 - 1) / 34 * 34)
#define ALIGN_4096(addr) (((addr) + 4096 - 1) / 4096 * 4096)
#define COMPRESS_BLOCK_SIZE (64*1024)
#define MAX_BLOCK_NUM 100

extern uint8_t *decompress_in_memory();
extern uint32_t get_flash_map_offset(uint32_t index);
extern uint32_t get_flash_map_size(uint32_t index);
extern uint32_t get_flash_map_phy_size(uint32_t index);

typedef struct {
	uint8_t crc;
} CRC8_Context;

#if CONFIG_DECOMPRESS_RESUME
typedef struct {
	uint8_t index;
	uint8_t data[33];
	CRC8_Context crc8;
} resume_block_t;

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

static void CRC8_Final( CRC8_Context *inContext, uint8_t *outResult )
{
	*outResult = inContext->crc & 0xffu;
}

static uint32_t get_resume_base_address(void)
{
	uint32_t back_address = partition_get_phy_offset(PARTITION_OTA_CONTROL);
	return back_address;
}


static uint8_t read_resume_block(uint32_t back_address, uint8_t* resume_data, size_t resume_data_size)
{
	resume_block_t resume_block[2];
	memset(resume_block, 0xFF, sizeof(resume_block));
	CRC8_Context crc_8;
	uint8_t idx = 0;

	for(; idx < MAX_BLOCK_NUM; ++idx) {
		CRC8_Init(&crc_8);
		resume_block_t* curr = &resume_block[idx % 2];
		memset(curr,0xFF,sizeof(resume_block_t));
		bk_flash_read_bytes(back_address + idx * sizeof(resume_block_t), (uint8_t*)curr, sizeof(resume_block_t));
		if(curr->index != 0xFF){
			CRC8_Update(&crc_8, &curr->index, sizeof(curr->index));
			CRC8_Update(&crc_8, curr->data, sizeof(curr->data));
			if(crc_8.crc != curr->crc8.crc){
				BOOT_LOG_ERR("resume block=%d crc8 error!", idx);
				return 0xffu;
			}
		} else {
			break;
		}
	}
	memcpy(resume_data,resume_block[(idx + 1) % 2].data,resume_data_size);
	return idx;
}

static uint8_t write_resume_block(uint8_t idx, uint32_t back_address, uint8_t* resume_data, size_t resume_data_size)
{
	resume_block_t resume_block;
	memset(&resume_block,0xFF,sizeof(resume_block));
	memcpy(resume_block.data,resume_data,resume_data_size);
	CRC8_Context crc_8;
	CRC8_Init(&crc_8);
	CRC8_Update(&crc_8, &idx, sizeof(idx));
	CRC8_Update(&crc_8, resume_block.data, sizeof(resume_block.data));
	resume_block.index = idx;
	resume_block.crc8 = crc_8;
	TEST_OVERWRITE_RESET_MODIFY_DATA(idx, POINT_WRONG_RESUME_CRC8, &resume_block.crc8);
	bk_flash_write_bytes(back_address + idx * sizeof(resume_block), (uint8_t*)&resume_block, sizeof(resume_block));
	TEST_OVERWRITE_RESET(idx, POINT_WRONG_RESUME_CRC8);
}

static uint32_t resume_flash(uint32_t block_num)
{
	uint32_t primary_all_phy_offset = get_flash_map_offset(0);
	uint32_t area_size = get_flash_map_phy_size(0);

	uint32_t back_address = get_resume_base_address();
	uint32_t back_data_size = CEIL_ALIGN_34(primary_all_phy_offset) - primary_all_phy_offset;
	uint8_t back_data[back_data_size];
	uint8_t restart_block_idx = read_resume_block(back_address, back_data, back_data_size);

	BOOT_LOG_INF("total block=%d, resume block=%d", block_num, restart_block_idx);

	if((restart_block_idx == 0) || (restart_block_idx == 0xffu) || (restart_block_idx > block_num)) {
		BOOT_LOG_INF("Erasing primary and resume");
		flash_area_erase_fast(primary_all_phy_offset, area_size);
		flash_area_erase_fast(get_resume_base_address(), 4096);
		restart_block_idx = 0;
		return restart_block_idx;
	}

	uint32_t restart_block_offset = primary_all_phy_offset + TOPHY(COMPRESS_BLOCK_SIZE) * restart_block_idx;
	uint32_t erase_size = TOPHY(COMPRESS_BLOCK_SIZE) + 4 * 1024;

	if(restart_block_idx < block_num) {
		erase_size = TOPHY(COMPRESS_BLOCK_SIZE) + 4 * 1024;
	} else if(restart_block_idx == block_num) {
		erase_size = primary_all_phy_offset + area_size - restart_block_offset;
	}
	BOOT_LOG_INF("Erasing primary off=0x%x, size=0x%x", restart_block_offset, erase_size);
	flash_area_erase_fast(restart_block_offset, erase_size);
	bk_flash_write_bytes(restart_block_offset, back_data, back_data_size);

	return restart_block_idx;
}

static void back_flash(uint32_t restart_block_idx)
{
	uint32_t primary_all_phy_offset = get_flash_map_offset(0);

	uint32_t back_address = get_resume_base_address();
	uint32_t back_data_size = CEIL_ALIGN_34(primary_all_phy_offset) - primary_all_phy_offset; // COMPRESS_BLOCK = 68k
	uint8_t back_data[back_data_size];
	uint32_t restart_block_offset = primary_all_phy_offset + TOPHY(COMPRESS_BLOCK_SIZE) * (restart_block_idx + 1);
	bk_flash_read_bytes(restart_block_offset, back_data, back_data_size);
	write_resume_block(restart_block_idx, back_address, back_data, back_data_size);
}

#endif

#if CONFIG_STATIC_ARRARY
static uint8_t s_compressed_buf[COMPRESS_BLOCK_SIZE + 64];
static uint8_t s_decompressed_buf[COMPRESS_BLOCK_SIZE + 64];
#else
//Use fixed address to optimize loader time
static uint8_t *s_compressed_buf = (uint8_t*)BL2_FREE_SRAM;
static uint8_t *s_decompressed_buf = (uint8_t*)(BL2_FREE_SRAM + COMPRESS_BLOCK_SIZE + 128);
#endif

static uint32_t idx_sum(uint16_t* buffer, size_t idx)
{
	uint32_t sum = 0;
	for( size_t i = 0; i < idx; ++i){
		sum += buffer[i];
	}
	return sum;
}

static void clean_buf(void)
{
	os_memset(s_decompressed_buf, 0, COMPRESS_BLOCK_SIZE);
	os_memset(s_compressed_buf, 0, COMPRESS_BLOCK_SIZE);
}

int
boot_copy_region(struct boot_loader_state *state, const struct flash_area *fap_src,
	const struct flash_area *fap_dst, uint32_t off_src, uint32_t off_dst, uint32_t sz)
{
	uint32_t bytes_copied = 0;
	uint32_t primary_all_vir_size = get_flash_map_size(0);
	uint32_t primary_all_phy_offset = get_flash_map_offset(0);
	uint32_t block_num = (primary_all_vir_size) / COMPRESS_BLOCK_SIZE;
	uint16_t block_list[block_num + 2];

	flash_protect_type_t protect = bk_flash_get_protect_type();
	bk_flash_set_protect_type(FLASH_PROTECT_NONE);

	uint8_t restart_block_idx;
#if CONFIG_DECOMPRESS_RESUME
	restart_block_idx = resume_flash(block_num);
	TEST_OVERWRITE_RESET_START(restart_block_idx, block_num);
#else
	restart_block_idx = 0;
	uint32_t area_size = get_flash_map_phy_size(0);
	flash_area_erase_fast(primary_all_phy_offset, area_size);
#endif
	int rate_process = block_num / 5;
	uint32_t vir_primary_all_start_address = TOVIRTURE(CEIL_ALIGN_34(primary_all_phy_offset));

	uint8_t block_idx = 0;

	bytes_copied = BL2_HEADER_SIZE;
	flash_area_read(fap_src, off_src + bytes_copied, block_list, 2 * (block_num + 2));
	bytes_copied += 2 * (block_num + 2);

	bytes_copied += idx_sum(block_list, restart_block_idx);

	for (block_idx = restart_block_idx; block_idx < block_num; block_idx++){
#if CONFIG_AON_WDT
		aon_wdt_feed();
#endif
		clean_buf();
		flash_area_read(fap_src, off_src + bytes_copied, s_compressed_buf, block_list[block_idx]);
		decompress_in_memory(s_compressed_buf, s_decompressed_buf, COMPRESS_BLOCK_SIZE, 0);
		bk_flash_write_cbus(vir_primary_all_start_address + COMPRESS_BLOCK_SIZE * block_idx, (s_decompressed_buf), COMPRESS_BLOCK_SIZE);
#if CONFIG_DECOMPRESS_RESUME
		TEST_OVERWRITE_RESET(block_idx, POINT_AFTER_WRITE_BEFORE_BACKUP_RESUME);
		back_flash(restart_block_idx);
		TEST_OVERWRITE_RESET(block_idx, POINT_AFTER_BACKUP_RESUME);
#endif
		restart_block_idx += 1;
		bytes_copied += block_list[block_idx];

		if((block_idx + 1) % rate_process == 0){
			BOOT_LOG_INF("OTA %d%%",(block_idx / rate_process + 1) * 20);
		}
	}

	uint16_t last_block_before_size = block_list[block_idx+1];
	uint16_t last_block_after_size  = block_list[block_idx];

	if (last_block_before_size > 0) {
		clean_buf();
		flash_area_read(fap_src, off_src + bytes_copied, s_compressed_buf, last_block_after_size);
		decompress_in_memory(s_compressed_buf, s_decompressed_buf, last_block_before_size, 0);

		uint16_t write_size = (last_block_before_size + 31) / 32 * 32;
		TEST_OVERWRITE_RESET(block_idx, POINT_BEFORE_WRITE_LAST_BLOCK);
		bk_flash_write_cbus(vir_primary_all_start_address + COMPRESS_BLOCK_SIZE * block_idx, s_decompressed_buf, write_size);
	}

#if CONFIG_DECOMPRESS_RESUME
	TEST_OVERWRITE_RESET(block_idx, POINT_ERASE_RESUME);
	BOOT_LOG_INF("erasing resume");
	flash_area_erase_fast(get_resume_base_address(), 4096);
#endif
	bk_flash_set_protect_type(protect);
	return 0;
}
