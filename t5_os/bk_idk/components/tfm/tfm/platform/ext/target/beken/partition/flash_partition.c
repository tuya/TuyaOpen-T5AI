#include "flash_map/flash_map.h"
#include "target.h"
#include "Driver_Flash.h"
#include "flash_partition.h"
#include "flash.h"
#include <components/log.h>
#include <ctype.h>
#include <string.h>

#define TAG "partition"

typedef struct {
	uint32_t phy_offset;
	uint32_t phy_size;
	uint32_t phy_flags;
} partition_config_t;
static partition_config_t s_partition_config[PARTITION_CNT] = {0};

const char *s_partition_name[PARTITION_CNT] = {
	"primary_all",
	"secondary_all",
	"ota",
	"partition",
	"primary_tfm_s",
	"primary_tfm_ns",
	"primary_cpu0_app",
	"sys_otp_nv",
	"sys_ps",
	"sys_its",
	"ota_control",
	"public_key"
};

static int is_alpha(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

uint32_t piece_address(uint8_t *array,uint32_t index)
{
	return ((uint32_t)(array[index]) << 24 | (uint32_t)(array[index+1])  << 16 | (uint32_t)(array[index+2])  << 8 | (uint32_t)((array[index+3])));
}

uint16_t short_address(uint8_t *array,uint16_t index)
{
	return ((uint16_t)(array[index]) << 8 | (uint16_t)(array[index+1]));
}

uint32_t partition_get_phy_offset(uint32_t id)
{
	if (id >= PARTITION_CNT) {
		return 0;
	}
	return s_partition_config[id].phy_offset;
}

uint32_t partition_get_phy_size(uint32_t id)
{
	if (id >= PARTITION_CNT) {
		return 0;
	}
	return s_partition_config[id].phy_size;
}

static uint8_t s_partition_buf[PARTITION_ENTRY_LEN + 1] = {0};

int partition_init(void)
{
	char name[PARTITION_NAME_LEN + 1];
	uint32_t i;
	int ret;

	uint32_t partition_start = PARTITION_PARTITION_PHY_OFFSET + PARTITION_PPC_OFFSET;
	for(i=0;i<PARTITION_AMOUNT;++i){
		ret = bk_flash_read_bytes(partition_start + PARTITION_ENTRY_LEN*i, s_partition_buf, PARTITION_ENTRY_LEN);
		if (0 != ret) {
			return 1;
		}

		if(is_alpha(s_partition_buf[0]) == 0){
			break;
		} 

		int j;
		for(j=0; j<PARTITION_NAME_LEN; ++j){
			if(s_partition_buf[j] == 0xFF){
				break;
			}
			name[j] = s_partition_buf[j];
		}
		name[j] = '\0';

		for (int k = 0; k < PARTITION_CNT; k++) {
			if(strcmp(s_partition_name[k],name) == 0){
				s_partition_config[k].phy_offset = piece_address(s_partition_buf, PARTITION_OFFSET_OFFSET);
				s_partition_config[k].phy_size = piece_address(s_partition_buf, PARTITION_SIZE_OFFSET);
				s_partition_config[k].phy_flags = short_address(s_partition_buf, PARTITION_FLAGS_OFFSET);
			}
		}
	}

	return 0;
}

void dump_partition(void)
{
	for (int k = 0; k < PARTITION_CNT; k++) {
		BK_LOGI(TAG, "%s offset=%x size=%x flags=%x\r\n", s_partition_name[k], \
		s_partition_config[k].phy_offset, s_partition_config[k].phy_size, s_partition_config[k].phy_flags);
	}
}
