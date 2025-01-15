// Copyright 2022-2023 Beken
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

#include "driver/efuse.h"

#define EFUSE_SECUREBOOT_ENABLED_BIT        (3)
#define EFUSE_SPE_DEBUG_DISABLED_BIT        (31)
#define EFUSE_INFO_DISABLED_BIT             (8)
#define EFUSE_ERR_DISABLED_BIT              (9)
#define EFUSE_SECURE_DOWNLOAD_DISABLED_BIT  (10)

#define TAG "efuse"

static uint32_t s_efuse_data = 0;

int bk_efuse_init(void)
{
	uint8_t *efuse_byte_p = (uint8_t*)&s_efuse_data;
	int offset;

	bk_efuse_driver_init();
	for (offset = 0; offset < 4; offset ++) {
		bk_efuse_read_byte(offset, efuse_byte_p);
		efuse_byte_p++;
	}
	return BK_OK;
}

bool efuse_is_secureboot_enabled(void)
{
#if CONFIG_BL2_VALIDATE_ENABLED_BY_EFUSE
	return !!(s_efuse_data & BIT(EFUSE_SECUREBOOT_ENABLED_BIT));
#else
	return true;
#endif
}

bool efuse_is_spe_debug_enabled(void)
{
	return !(s_efuse_data & BIT(EFUSE_SPE_DEBUG_DISABLED_BIT));
}

bool efuse_is_info_log_enabled(void)
{
	return !(s_efuse_data & BIT(EFUSE_INFO_DISABLED_BIT));
}

bool efuse_is_err_log_enabled(void)
{
	return !(s_efuse_data & BIT(EFUSE_ERR_DISABLED_BIT));
}

bool efuse_is_secure_download_enabled(void)
{
	return !(s_efuse_data & BIT(EFUSE_SECURE_DOWNLOAD_DISABLED_BIT));
}

void dump_efuse(void)
{
	BK_LOGI(TAG, "efuse=%x\r\n", s_efuse_data);
}
