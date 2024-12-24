// Copyright 2020-2021 Beken
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


#include <os/os.h>
#include <os/mem.h>
#include <driver/spi.h>
#include <driver/dma.h>
#include <driver/gpio.h>
#include "gpio_driver.h"
#include <driver/lcd.h>
#include <driver/lcd_types.h>
#include <driver/lcd_qspi.h>
#include <driver/qspi.h>
#include <driver/qspi_types.h>


#define LCD_SPI_TAG "lcd_spi"

#define LCD_SPI_LOGI(...) BK_LOGI(LCD_SPI_TAG, ##__VA_ARGS__)
#define LCD_SPI_LOGW(...) BK_LOGW(LCD_SPI_TAG, ##__VA_ARGS__)
#define LCD_SPI_LOGE(...) BK_LOGE(LCD_SPI_TAG, ##__VA_ARGS__)
#define LCD_SPI_LOGD(...) BK_LOGD(LCD_SPI_TAG, ##__VA_ARGS__)


#define LCD_SPI_REFRESH_WITH_QSPI   1


#define LCD_SPI_RESET_PIN           GPIO_28
#define LCD_SPI_BACKLIGHT_PIN       GPIO_26
#define LCD_SPI_RS_PIN              GPIO_9
#define LCD_SPI_DEVICE_CASET        0x2A
#define LCD_SPI_DEVICE_RASET        0x2B
#define LCD_SPI_DEVICE_RAMWR        0x2C

#if LCD_SPI_REFRESH_WITH_QSPI
#define LCD_SPI_ID                  QSPI_ID_0
#else
#define LCD_SPI_ID                  SPI_ID_0

spi_config_t config = {0};
#endif


static void lcd_spi_device_gpio_init(void)
{
    BK_LOG_ON_ERR(bk_gpio_driver_init());

    BK_LOG_ON_ERR(gpio_dev_unmap(LCD_SPI_BACKLIGHT_PIN));
    BK_LOG_ON_ERR(gpio_dev_unmap(LCD_SPI_RESET_PIN));
    BK_LOG_ON_ERR(gpio_dev_unmap(LCD_SPI_RS_PIN));

    gpio_config_t config;
    config.io_mode = GPIO_OUTPUT_ENABLE;
    config.pull_mode = GPIO_PULL_DISABLE;
    config.func_mode = GPIO_SECOND_FUNC_DISABLE;

    bk_gpio_set_config(LCD_SPI_RESET_PIN, &config);
    bk_gpio_set_config(LCD_SPI_BACKLIGHT_PIN, &config);
    bk_gpio_set_config(LCD_SPI_RS_PIN, &config);

    BK_LOG_ON_ERR(bk_gpio_set_output_low(LCD_SPI_BACKLIGHT_PIN));
    BK_LOG_ON_ERR(bk_gpio_set_output_high(LCD_SPI_RESET_PIN));
    BK_LOG_ON_ERR(bk_gpio_set_output_high(LCD_SPI_RS_PIN));
}

static void lcd_spi_device_gpio_deinit(void)
{
    BK_LOG_ON_ERR(gpio_dev_unmap(LCD_SPI_BACKLIGHT_PIN));
    BK_LOG_ON_ERR(gpio_dev_unmap(LCD_SPI_RESET_PIN));
    BK_LOG_ON_ERR(gpio_dev_unmap(LCD_SPI_RS_PIN));
}


#if LCD_SPI_REFRESH_WITH_QSPI

static qspi_driver_t s_lcd_spi[SOC_QSPI_UNIT_NUM] = {
    {
        .hal.hw = (qspi_hw_t *)(SOC_QSPI0_REG_BASE),
    },
#if (SOC_QSPI_UNIT_NUM > 1)
    {
        .hal.hw = (qspi_hw_t *)(SOC_QSPI1_REG_BASE),
    }
#endif
};

static void lcd_spi_driver_init_with_qspi(qspi_id_t qspi_id, lcd_qspi_clk_t clk)
{
    qspi_config_t lcd_qspi_config;
    os_memset(&lcd_qspi_config, 0, sizeof(lcd_qspi_config));

    os_memset(&s_lcd_spi, 0, sizeof(s_lcd_spi));
    s_lcd_spi[qspi_id].hal.id = qspi_id;
    qspi_hal_init(&s_lcd_spi[qspi_id].hal);

    switch (clk) {
        case LCD_QSPI_80M:
            lcd_qspi_config.src_clk = QSPI_SCLK_480M;
            lcd_qspi_config.src_clk_div = 5;
            BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &lcd_qspi_config));
            break;

        case LCD_QSPI_64M:
            lcd_qspi_config.src_clk = QSPI_SCLK_320M;
            lcd_qspi_config.src_clk_div = 4;
            BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &lcd_qspi_config));
            break;

        case LCD_QSPI_60M:
            lcd_qspi_config.src_clk = QSPI_SCLK_480M;
            lcd_qspi_config.src_clk_div = 7;
            BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &lcd_qspi_config));
            break;

        case LCD_QSPI_53M:
            lcd_qspi_config.src_clk = QSPI_SCLK_480M;
            lcd_qspi_config.src_clk_div = 8;
            BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &lcd_qspi_config));
            break;

        case LCD_QSPI_48M:
            lcd_qspi_config.src_clk = QSPI_SCLK_480M;
            lcd_qspi_config.src_clk_div = 9;
            BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &lcd_qspi_config));
            break;

        case LCD_QSPI_40M:
            lcd_qspi_config.src_clk = QSPI_SCLK_480M;
            lcd_qspi_config.src_clk_div = 11;
            BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &lcd_qspi_config));
            break;

        case LCD_QSPI_32M:
            lcd_qspi_config.src_clk = QSPI_SCLK_320M;
            lcd_qspi_config.src_clk_div = 9;
            BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &lcd_qspi_config));
            break;

        case LCD_QSPI_30M:
            lcd_qspi_config.src_clk = QSPI_SCLK_480M;
            lcd_qspi_config.src_clk_div = 15;
            BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &lcd_qspi_config));
            break;

        default:
            lcd_qspi_config.src_clk = QSPI_SCLK_480M;
            lcd_qspi_config.src_clk_div = 11;
            BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &lcd_qspi_config));
            break;
    }
}

static void lcd_spi_driver_deinit_with_qspi(qspi_id_t qspi_id)
{
    BK_LOG_ON_ERR(bk_qspi_deinit(qspi_id));
}

static void lcd_spi_send_cmd_with_qspi(qspi_id_t qspi_id, uint8_t cmd)
{
    qspi_hal_set_cmd_c_l(&s_lcd_spi[qspi_id].hal, 0);
    qspi_hal_set_cmd_c_h(&s_lcd_spi[qspi_id].hal, 0);
    qspi_hal_set_cmd_c_cfg1(&s_lcd_spi[qspi_id].hal, 0);
    qspi_hal_set_cmd_c_cfg2(&s_lcd_spi[qspi_id].hal, 0);

    qspi_hal_set_cmd_c_h(&s_lcd_spi[qspi_id].hal, cmd);
    qspi_hal_set_cmd_c_cfg1(&s_lcd_spi[qspi_id].hal, 0xC);
    qspi_hal_cmd_c_start(&s_lcd_spi[qspi_id].hal);
    qspi_hal_wait_cmd_done(&s_lcd_spi[qspi_id].hal);
}

static void lcd_spi_send_data_with_qspi_direct_mode(qspi_id_t qspi_id, uint8_t *data, uint32_t data_len)
{
    uint32_t value = 0;

    qspi_hal_set_cmd_c_l(&s_lcd_spi[qspi_id].hal, 0);
    qspi_hal_set_cmd_c_h(&s_lcd_spi[qspi_id].hal, 0);
    qspi_hal_set_cmd_c_cfg1(&s_lcd_spi[qspi_id].hal, 0);
    qspi_hal_set_cmd_c_cfg2(&s_lcd_spi[qspi_id].hal, 0);

    for (uint8_t i = 0; i < data_len; i++) {
        value |= (data[i] << (i * 8));
    }

    qspi_hal_set_cmd_c_h(&s_lcd_spi[qspi_id].hal, value);
    qspi_hal_set_cmd_c_cfg1(&s_lcd_spi[qspi_id].hal, 0x3 << (data_len * 2));
    qspi_hal_cmd_c_start(&s_lcd_spi[qspi_id].hal);
    qspi_hal_wait_cmd_done(&s_lcd_spi[qspi_id].hal);
}

void lcd_spi_send_data_with_qspi_indirect_mode(qspi_id_t qspi_id, uint8_t *data, uint32_t data_len)
{
    uint32_t value = 0;
    uint32_t send_len = 0;
    uint32_t remain_len = data_len;
    uint8_t *data_tmp = data;

    qspi_hal_set_cmd_c_l(&s_lcd_spi[qspi_id].hal, 0);
    qspi_hal_set_cmd_c_h(&s_lcd_spi[qspi_id].hal, 0);
    qspi_hal_set_cmd_c_cfg1(&s_lcd_spi[qspi_id].hal, 0);
    qspi_hal_set_cmd_c_cfg2(&s_lcd_spi[qspi_id].hal, 0);

    while (remain_len > 0) {
        if (remain_len <= 4) {
            lcd_spi_send_data_with_qspi_direct_mode(qspi_id, data, remain_len);
            break;
        }

        value = (data_tmp[3] << 24) | (data_tmp[2] << 16) | (data_tmp[1] << 8) | data_tmp[0];
        remain_len -= 4;
        send_len = remain_len < 0x100 ? remain_len : 0x100;
        qspi_hal_set_cmd_c_h(&s_lcd_spi[qspi_id].hal, value);
        qspi_hal_set_cmd_c_cfg1(&s_lcd_spi[qspi_id].hal, 0x300);
        qspi_hal_set_cmd_c_cfg2(&s_lcd_spi[qspi_id].hal, send_len << 2);
        data_tmp += 4;
        bk_qspi_write(qspi_id, data_tmp, send_len);
        qspi_hal_cmd_c_start(&s_lcd_spi[qspi_id].hal);
        qspi_hal_wait_cmd_done(&s_lcd_spi[qspi_id].hal);
        remain_len -= send_len;
        data_tmp += send_len;
    }
}
#endif

static void lcd_spi_send_cmd(uint8_t cmd)
{
    BK_LOG_ON_ERR(bk_gpio_set_output_low(LCD_SPI_RS_PIN));

#if LCD_SPI_REFRESH_WITH_QSPI
    lcd_spi_send_cmd_with_qspi(LCD_SPI_ID, cmd);
#else
    bk_spi_write_bytes(LCD_SPI_ID, &cmd, 1);
#endif
}

static void lcd_spi_send_data(uint8_t *data, uint32_t data_len)
{
    BK_LOG_ON_ERR(bk_gpio_set_output_high(LCD_SPI_RS_PIN));

#if LCD_SPI_REFRESH_WITH_QSPI
    lcd_spi_send_data_with_qspi_indirect_mode(LCD_SPI_ID, data, data_len);
#else

#if CONFIG_SPI_DMA
    if (data_len > 32) {
        bk_spi_dma_write_bytes(LCD_SPI_ID, data, data_len);
    } else {
        bk_spi_write_bytes(LCD_SPI_ID, data, data_len);
    }
#else
    bk_spi_write_bytes(LCD_SPI_ID, data, data_len);
#endif
#endif
}

#if (!LCD_SPI_REFRESH_WITH_QSPI)
static void lcd_spi_driver_init(void)
{
    bk_spi_driver_init();

    config.role = SPI_ROLE_MASTER;
    config.bit_width = SPI_BIT_WIDTH_8BITS;
    config.polarity = SPI_POLARITY_HIGH;
    config.phase = SPI_PHASE_2ND_EDGE;
    config.wire_mode = SPI_4WIRE_MODE;
    config.baud_rate = 30000000;
    config.bit_order = SPI_MSB_FIRST;

#if CONFIG_SPI_DMA
    config.dma_mode = SPI_DMA_MODE_ENABLE;
    config.spi_tx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI0);
    config.spi_rx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI0_RX);
    config.spi_tx_dma_width = DMA_DATA_WIDTH_8BITS;
    config.spi_rx_dma_width = DMA_DATA_WIDTH_8BITS;
#else
    config.dma_mode = SPI_DMA_MODE_DISABLE;
#endif

    BK_LOG_ON_ERR(bk_spi_init(LCD_SPI_ID, &config));
}

static void lcd_spi_driver_deinit(void)
{
    BK_LOG_ON_ERR(bk_spi_deinit(LCD_SPI_ID));

#if CONFIG_SPI_DMA
    bk_dma_free(DMA_DEV_GSPI0, config.spi_tx_dma_chan);
    bk_dma_free(DMA_DEV_GSPI0_RX, config.spi_rx_dma_chan);
#endif
}
#endif

void lcd_spi_backlight_open(void)
{
    BK_LOG_ON_ERR(bk_gpio_set_output_high(LCD_SPI_BACKLIGHT_PIN));
}

void lcd_spi_backlight_close(void)
{
    BK_LOG_ON_ERR(bk_gpio_set_output_low(LCD_SPI_BACKLIGHT_PIN));
}

void lcd_spi_init(const lcd_device_t *device)
{
    if (device == NULL) {
        LCD_SPI_LOGE("lcd spi device not found\r\n");
        return;
    }

    lcd_spi_device_gpio_init();

#if LCD_SPI_REFRESH_WITH_QSPI
    lcd_spi_driver_init_with_qspi(LCD_SPI_ID, device->spi->clk);
#else
    lcd_spi_driver_init();
#endif

    BK_LOG_ON_ERR(bk_gpio_set_output_low(LCD_SPI_RESET_PIN));
    rtos_delay_milliseconds(100);
    BK_LOG_ON_ERR(bk_gpio_set_output_high(LCD_SPI_RESET_PIN));
    rtos_delay_milliseconds(120);

    if (device->spi->init_cmd != NULL) {
        const lcd_qspi_init_cmd_t *init = device->spi->init_cmd;
        for (uint32_t i = 0; i < device->spi->device_init_cmd_len; i++) {
            if (init->data_len == 0xFF) {
                rtos_delay_milliseconds(init->data[0]);
            } else {
                lcd_spi_send_cmd(init->cmd);
                if (init->data_len != 0) {
                    lcd_spi_send_data((uint8_t *)init->data, init->data_len);
                }
            }
            init++;
        }
    } else {
        LCD_SPI_LOGE("lcd spi device init cmd is null\r\n");
    }

    lcd_spi_backlight_open();
}

void lcd_spi_deinit(void)
{
    lcd_spi_backlight_close();

    BK_LOG_ON_ERR(bk_gpio_set_output_low(LCD_SPI_RESET_PIN));

#if LCD_SPI_REFRESH_WITH_QSPI
    lcd_spi_driver_deinit_with_qspi(LCD_SPI_ID);
#else
    lcd_spi_driver_deinit();
#endif

    lcd_spi_device_gpio_deinit();
}

void lcd_spi_display_frame(uint8_t *frame_buffer, uint32_t width, uint32_t height)
{
    uint8_t column_value[4] = {0};
    uint8_t row_value[4] = {0};
    column_value[2] = (width >> 8) & 0xFF,
    column_value[3] = (width & 0xFF) - 1,
    row_value[2] = (height >> 8) & 0xFF;
    row_value[3] = (height & 0xFF) - 1;

    lcd_spi_send_cmd(LCD_SPI_DEVICE_CASET);
    lcd_spi_send_data(column_value, 4);
    lcd_spi_send_cmd(LCD_SPI_DEVICE_RASET);
    lcd_spi_send_data(row_value, 4);

    lcd_spi_send_cmd(LCD_SPI_DEVICE_RAMWR);
    lcd_spi_send_data(frame_buffer, width * height * 2);
}

