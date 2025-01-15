#include <stdint.h>
#include "decompress_bl2_reset_test.h"
#include "bootutil/bootutil_log.h"
#include "wdt_hal.h"

#if CONFIG_OVERWRITE_RESET_TEST
static bool s_test_overwrite_reset_en = false;
uint32_t s_total_block = 0;

const char* TEST_OVERWRITE_RESET_POINT(uint32_t point)
{
	switch (point){
	case POINT_AFTER_WRITE_BEFORE_BACKUP_RESUME:
		return "After overwrite block, but before backup resume data";
	case POINT_AFTER_BACKUP_RESUME:
		return "After backup resume data";
	case POINT_BEFORE_WRITE_LAST_BLOCK:
		return "Before write last block";
	case POINT_ERASE_RESUME:
		return "After write last block, before erase resume";
	case POINT_WRONG_RESUME_CRC8:
		return "Reset after Write wrong CRC8 into resume";
	default:
		return "Unknown point";
	}
}

void TEST_OVERWRITE_RESET_START(uint32_t resume_block_idx, uint32_t total_block)
{
	s_total_block = total_block;
	if (resume_block_idx == 0) {
		s_test_overwrite_reset_en = true;
		BOOT_LOG_WRN("@@@ enable overwrite WDT reset test");
	}
}

void TEST_OVERWRITE_RESET(uint32_t block_idx, uint32_t point)
{
	if (s_test_overwrite_reset_en == false) {
		return;
	}

	if ((CONFIG_OVERWRITE_RESET_TEST_POINT != POINT_BEFORE_WRITE_LAST_BLOCK) && (CONFIG_OVERWRITE_RESET_TEST_POINT != POINT_ERASE_RESUME)) {
		if (block_idx != CONFIG_OVERWRITE_RESET_TEST_BLOCK_IDX) {
			return;
		}
	}

	if (point != CONFIG_OVERWRITE_RESET_TEST_POINT) {
		return;
	}

	BOOT_LOG_WRN("@@@ block: %d", block_idx);
	BOOT_LOG_WRN("@@@ point: %s", TEST_OVERWRITE_RESET_POINT(point));
	wdt_reboot();
}

void TEST_OVERWRITE_RESET_MODIFY_DATA(uint32_t block_idx, uint32_t point, uint8_t *data)
{
	if ((CONFIG_OVERWRITE_RESET_TEST_POINT == point) && (block_idx == CONFIG_OVERWRITE_RESET_TEST_BLOCK_IDX)) {
		*data = 0xFF;	
		BOOT_LOG_WRN("@@@ modify block%d, resume CRC8 to 0xFF", block_idx);
	}
}

#endif

