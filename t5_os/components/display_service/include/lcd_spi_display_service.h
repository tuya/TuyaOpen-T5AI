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

#include <driver/lcd_types.h>
#include <driver/spi.h>


#ifdef __cplusplus
extern "C" {
#endif

void lcd_spi_backlight_open(void);

void lcd_spi_backlight_close(void);

void lcd_spi_init(const lcd_device_t *device);

void lcd_spi_deinit(void);

void lcd_spi_display_frame(uint8_t *frame_buffer, uint32_t width, uint32_t height);

#ifdef __cplusplus
}
#endif

