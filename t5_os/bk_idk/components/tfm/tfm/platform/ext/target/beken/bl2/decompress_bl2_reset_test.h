#pragma once

enum {
	POINT_AFTER_WRITE_BEFORE_BACKUP_RESUME = 0,
	POINT_WRONG_RESUME_CRC8,
	POINT_AFTER_BACKUP_RESUME,
	POINT_BEFORE_WRITE_LAST_BLOCK,
	POINT_ERASE_RESUME,
};

#define CONFIG_OVERWRITE_RESET_TEST           0
#define CONFIG_OVERWRITE_RESET_TEST_BLOCK_IDX 10
#define CONFIG_OVERWRITE_RESET_TEST_POINT     POINT_ERASE_RESUME

#if CONFIG_OVERWRITE_RESET_TEST
void TEST_OVERWRITE_RESET_START(uint32_t resume_block_idx, uint32_t total_block);
void TEST_OVERWRITE_RESET(uint32_t block_idx, uint32_t point);
void TEST_OVERWRITE_RESET_MODIFY_DATA(uint32_t block_idx, uint32_t point, uint8_t *data);
#else
#define TEST_OVERWRITE_RESET_START(resume_block_idx, total_block)
#define TEST_OVERWRITE_RESET(block_idx, point)
#define TEST_OVERWRITE_RESET_MODIFY_DATA(block_idx, point, data);
#endif

