// Copyright 2022-2023 Beken
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

#define DEC_TAG "decomp"

#define DEC_LOGI(...) BK_LOGI(DEC_TAG, ##__VA_ARGS__)
#define DEC_LOGW(...) BK_LOGW(DEC_TAG, ##__VA_ARGS__)
#define DEC_LOGE(...) BK_LOGE(DEC_TAG, ##__VA_ARGS__)
#define DEC_LOGD(...) BK_LOGD(DEC_TAG, ##__VA_ARGS__)

typedef enum {
	DECOMPRESS_BY_LZMA = 0,
	UNKNOWN_DECOMPRESSPOR,
} decompress_type_t;

uint8_t *decompress_in_memory(uint8_t *src, uint8_t *dest, uint32_t src_len, decompress_type_t decompressor);
