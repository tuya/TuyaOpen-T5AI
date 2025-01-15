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

#include <os/os.h>
#include <os/mem.h>

#if (CONFIG_LZMA1900 | CONFIG_LZMA2301)
#include "lzma.h"
#endif

#include "decompress.h"


uint8_t *decompress_in_memory(uint8_t *src, uint8_t *dest, uint32_t src_len, decompress_type_t decompressor)
{
	switch (decompressor) {
		case DECOMPRESS_BY_LZMA:
#if (CONFIG_LZMA1900 | CONFIG_LZMA2301)
		{
			size_t uncomp_size = *((size_t *)(src + 5));

			int ret = bk_lzma_decode(src, src_len, dest, uncomp_size);
			if (ret) {
				DEC_LOGE("lzma error code : %d \r\n", ret);
			}

			return dest;
		}
#endif
			break;
		
		default:
			DEC_LOGE("unknown decompressor !!! \r\n");
			break;
	}

	return dest;
}
