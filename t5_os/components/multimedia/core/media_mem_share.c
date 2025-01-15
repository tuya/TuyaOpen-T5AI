// Copyright 2024-2025 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>

#ifdef CONFIG_VIDEO_DTCM
__attribute__((section(".dtcm_sec_data "), aligned(0x4))) uint8_t share_buff[1024 * 10] = {0};
uint8_t *media_dtcm_share_buff = &share_buff[0];
#else
uint8_t *media_dtcm_share_buff = NULL;
#endif

#if (CONFIG_MEDIA_MAJOR)

#ifdef CONFIG_BT_REUSE_MEDIA_MEMORY
__attribute__((section(".bt_spec_data"), aligned(0x10))) uint8_t media_share_buf = 0;
uint8_t *media_bt_share_buffer = &media_share_buf;
#else
uint8_t *media_bt_share_buffer = NULL;
#endif

#endif