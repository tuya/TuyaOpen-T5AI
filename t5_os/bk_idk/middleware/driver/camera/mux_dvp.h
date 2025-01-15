// Copyright 2024-2025 Beken
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


#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_BLOCK_WIDTH     (80)
#define MAX_BLOCK_HEIGHT    (40)

#define DVP_YUV_PINGPANG_LENGTH  (864 * 32 * 2)
#define DVP_ENCODE_PINGPANG_LENGTH (480 * 32 * 2)
#define DVP_YUV_ROTATE_BLOCK_SIZE (MAX_BLOCK_WIDTH * MAX_BLOCK_HEIGHT * 2)

typedef struct {
	uint8_t yuv_pingpang[DVP_YUV_PINGPANG_LENGTH];
	uint8_t enc_pingpang[DVP_ENCODE_PINGPANG_LENGTH];
	uint8_t rot_rx[DVP_YUV_ROTATE_BLOCK_SIZE];
	uint8_t rot_tx[DVP_YUV_ROTATE_BLOCK_SIZE];
} dvp_sram_buffer_t;

#ifdef __cplusplus
}
#endif

