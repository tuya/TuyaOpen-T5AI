/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdbool.h>
#include "flash_map/flash_map.h"
#include "target.h"
#include "Driver_Flash.h"
#include "flash_partition.h"

/* When undefined FLASH_DEV_NAME_0 or FLASH_DEVICE_ID_0 , default */
#if !defined(FLASH_DEV_NAME_0) || !defined(FLASH_DEVICE_ID_0)
#define FLASH_DEV_NAME_0  FLASH_DEV_NAME
#define FLASH_DEVICE_ID_0 FLASH_DEVICE_ID
#endif

/* When undefined FLASH_DEV_NAME_1 or FLASH_DEVICE_ID_1 , default */
#if !defined(FLASH_DEV_NAME_1) || !defined(FLASH_DEVICE_ID_1)
#define FLASH_DEV_NAME_1  FLASH_DEV_NAME
#define FLASH_DEVICE_ID_1 FLASH_DEVICE_ID
#endif

/* When undefined FLASH_DEV_NAME_2 or FLASH_DEVICE_ID_2 , default */
#if !defined(FLASH_DEV_NAME_2) || !defined(FLASH_DEVICE_ID_2)
#define FLASH_DEV_NAME_2  FLASH_DEV_NAME
#define FLASH_DEVICE_ID_2 FLASH_DEVICE_ID
#endif

/* When undefined FLASH_DEV_NAME_3 or FLASH_DEVICE_ID_3 , default */
#if !defined(FLASH_DEV_NAME_3) || !defined(FLASH_DEVICE_ID_3)
#define FLASH_DEV_NAME_3  FLASH_DEV_NAME
#define FLASH_DEVICE_ID_3 FLASH_DEVICE_ID
#endif

#if defined(MCUBOOT_SWAP_USING_SCRATCH)
/* When undefined FLASH_DEV_NAME_SCRATCH or FLASH_DEVICE_ID_SCRATCH , default */
#if !defined(FLASH_DEV_NAME_SCRATCH) || !defined(FLASH_DEVICE_ID_SCRATCH)
#define FLASH_DEV_NAME_SCRATCH  FLASH_DEV_NAME
#define FLASH_DEVICE_ID_SCRATCH FLASH_DEVICE_ID
#endif
#endif  /* defined(MCUBOOT_SWAP_USING_SCRATCH) */

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))

/* Flash device names must be specified by target */
extern ARM_DRIVER_FLASH FLASH_DEV_NAME_0;
extern ARM_DRIVER_FLASH FLASH_DEV_NAME_1;
extern ARM_DRIVER_FLASH FLASH_DEV_NAME_2;
extern ARM_DRIVER_FLASH FLASH_DEV_NAME_3;
extern ARM_DRIVER_FLASH FLASH_DEV_NAME_SCRATCH;

struct flash_area flash_map[];

uint32_t get_flash_map_offset(uint32_t index)
{
	return flash_map[index].fa_off;
}

uint32_t get_flash_map_size(uint32_t index)
{
	return flash_map[index].fa_size;
}

uint32_t get_flash_map_phy_size(uint32_t index)
{
	return flash_map[index].fa_phy_size;
}

int flash_map_init(void)
{
	uint32_t size, id;

	size = partition_get_phy_size(PARTITION_PRIMARY_ALL);
	flash_map[0].fa_phy_size = size;
	flash_map[0].fa_off = partition_get_phy_offset(PARTITION_PRIMARY_ALL);
	flash_map[0].fa_size = FLASH_PHY2VIRTUAL(size);
	flash_map[0].fa_size &= ~(0xFFF);

#if CONFIG_OTA_OVERWRITE
	id = PARTITION_OTA;
#else
	id = PARTITION_SECONDARY_ALL;
#endif
	flash_map[1].fa_off = partition_get_phy_offset(id);
	flash_map[1].fa_size = partition_get_phy_size(id);
	flash_map[1].fa_phy_size = partition_get_phy_size(id);
#if CONFIG_DIRECT_XIP
	size = partition_get_phy_size(PARTITION_SECONDARY_ALL);
	flash_map[1].fa_phy_size = size;
	flash_map[1].fa_size = FLASH_PHY2VIRTUAL(size);
	flash_map[1].fa_size &= ~(0xFFF);
	uint32_t primary_vir_start = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(flash_map[0].fa_off));
	uint32_t secondary_vir_start = FLASH_PHY2VIRTUAL(CEIL_ALIGN_34(flash_map[1].fa_off));
	flash_set_xip_offset(primary_vir_start,secondary_vir_start,flash_map[0].fa_size);
#endif
	return 0;
}

struct flash_area flash_map[] = {
	{
		.fa_id = FLASH_AREA_PRIMARY_ALL_ID,
		.fa_device_id = FLASH_DEVICE_ID,
		.fa_driver = &FLASH_DEV_NAME,
	},
	{
		.fa_id = FLASH_AREA_SECONDARY_ALL_ID,
		.fa_device_id = FLASH_DEVICE_ID,
		.fa_driver = &FLASH_DEV_NAME,
	}
};

const int flash_map_entry_num = ARRAY_SIZE(flash_map);
