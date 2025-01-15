#include <stdint.h>
#include "partitions.h"
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

enum {
	PARTITION_PRIMARY_ALL = 0,
	PARTITION_SECONDARY_ALL,
	PARTITION_OTA,
	PARTITION_PARTITION,
	PARTITION_SPE,
	PARTITION_TFM_NS,
	PARTITION_NSPE,
	PARTITION_OTP_NV,
	PARTITION_PS,
	PARTITION_ITS,
	PARTITION_OTA_CONTROL,
	PARTITION_PUBLIC_KEY,
	PARTITION_CNT,
};

#define PARTITION_PARTITION_PHY_OFFSET   CONFIG_PARTITION_PHY_PARTITION_OFFSET
#define PARTITION_PPC_OFFSET             0x400
#define PARTITION_NAME_LEN               20
#define PARTITION_ENTRY_LEN              32
#define PARTITION_OFFSET_OFFSET          22
#define PARTITION_SIZE_OFFSET            26
#define PARTITION_FLAGS_OFFSET           30

#define FLASH_PHY2VIRTUAL(phy_addr)      ((((phy_addr) / 34) << 5) + ((phy_addr) % 34))
#define SOC_FLASH_BASE_ADDR 0x02000000
#define HDR_SZ                  0x1000

#define BOOT_VERSION_OFFSET              (CONFIG_PRIMARY_MANIFEST_PHY_PARTITION_OFFSET + 0x4)

int partition_init(void);
uint32_t partition_get_phy_offset(uint32_t id);
uint32_t partition_get_phy_size(uint32_t id);
void dump_partition(void);
