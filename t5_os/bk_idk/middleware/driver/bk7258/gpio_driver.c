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

#include <common/bk_include.h>
#include "bk_intc.h"
#include "bk_icu.h"
#include <os/mem.h>
#include "gpio_hal.h"
#include "gpio_map.h"
#include "gpio_driver.h"
#include "gpio_driver_base.h"
#include "icu_driver.h"
#include "amp_lock_api.h"
#include <driver/gpio.h>
#include "aon_pmu_hal.h"
#include "sys_hal.h"

extern gpio_driver_t s_gpio;

#define GPIO_REG_LOCK_WAIT_TIME_MS  6
#define GPIO_RETURN_ON_INVALID_PERIAL_MODE(mode, mode_max) do {\
				if ((mode) >= (mode_max)) {\
					return BK_ERR_GPIO_SET_INVALID_FUNC_MODE;\
				}\
			} while(0)
#if CONFIG_GPIO_RETENTION_SUPPORT
#define GPIO_RETENTION_MAP_DUMP            (0)
#define GPIO_RETENTION_MAP_SIZE            (8)
#define GPIO_RETENTION_MAP                 {GPIO_24, GPIO_NUM_MAX, GPIO_NUM_MAX, GPIO_NUM_MAX, GPIO_NUM_MAX, GPIO_NUM_MAX, GPIO_NUM_MAX, GPIO_NUM_MAX}
#define GPIO_RETENTION_EN_CHECK(id, i, f)  ((id < GPIO_NUM_MAX) && (s_gpio_retention_en_bitmap & BIT(i) || f))

static uint32_t s_gpio_retention_map[GPIO_RETENTION_MAP_SIZE] = GPIO_RETENTION_MAP;
static uint32_t s_gpio_retention_en_bitmap = 0;
#endif

bk_err_t gpio_dev_map(gpio_id_t gpio_id, gpio_dev_t dev)
{
#if (CONFIG_CPU_CNT > 1) && CONFIG_MAILBOX
	uint32_t ret_val = 1;

	ret_val = amp_res_acquire(AMP_RES_ID_GPIO, GPIO_REG_LOCK_WAIT_TIME_MS);
	GPIO_LOGD("amp_res_acquire:ret=%d\r\n", ret_val);
	if(ret_val != BK_OK)
		return ret_val;
#endif

	/* Restore a configuration that is not a secondary function to its initial state. */
	gpio_hal_output_enable(&s_gpio.hal, gpio_id, 0);
	gpio_hal_input_enable(&s_gpio.hal, gpio_id, 0);
	gpio_hal_pull_enable(&s_gpio.hal, gpio_id, 0);
	gpio_hal_disable_interrupt(&s_gpio.hal, gpio_id);
	gpio_hal_func_map(&s_gpio.hal, gpio_id, dev);

#if (CONFIG_CPU_CNT > 1) && CONFIG_MAILBOX
	ret_val = amp_res_release(AMP_RES_ID_GPIO);
	GPIO_LOGD("amp res release:ret=%d\r\n", ret_val);
	if(ret_val != BK_OK)
		return ret_val;
#endif
	return BK_OK;
}

bk_err_t gpio_dev_unmap(gpio_id_t gpio_id)
{
#if (CONFIG_CPU_CNT > 1) && CONFIG_MAILBOX
	uint32_t ret_val = 1;

	ret_val = amp_res_acquire(AMP_RES_ID_GPIO, GPIO_REG_LOCK_WAIT_TIME_MS);
	GPIO_LOGD("amp_res_acquire:ret=%d\r\n", ret_val);
	if(ret_val != BK_OK)
		return ret_val;
#endif

	/* Restore a configuration that is not a secondary function to its initial state. */
	gpio_hal_output_enable(&s_gpio.hal, gpio_id, 0);
	gpio_hal_input_enable(&s_gpio.hal, gpio_id, 0);
	gpio_hal_pull_enable(&s_gpio.hal, gpio_id, 0);
	gpio_hal_disable_interrupt(&s_gpio.hal, gpio_id);
	gpio_hal_func_unmap(&s_gpio.hal, gpio_id);

#if (CONFIG_CPU_CNT > 1) && CONFIG_MAILBOX
	ret_val = amp_res_release(AMP_RES_ID_GPIO);
	GPIO_LOGD("amp res release:ret=%d\r\n", ret_val);
	if(ret_val != BK_OK)
		return ret_val;
#endif

	return BK_OK;
}

/* Here doesn't check the GPIO id is whether used by another CPU-CORE, but checked current CPU-CORE */
bk_err_t gpio_dev_unprotect_map(gpio_id_t gpio_id, gpio_dev_t dev)
{
	GPIO_LOGI("%s:id=%d, dev=%d\r\n", __func__, gpio_id, dev);

	/* Restore a configuration that is not a secondary function to its initial state. */
	gpio_hal_output_enable(&s_gpio.hal, gpio_id, 0);
	gpio_hal_input_enable(&s_gpio.hal, gpio_id, 0);
	gpio_hal_pull_enable(&s_gpio.hal, gpio_id, 0);
	gpio_hal_disable_interrupt(&s_gpio.hal, gpio_id);
	gpio_hal_func_map(&s_gpio.hal, gpio_id, dev);

	return BK_OK;
}

/* Here doesn't check the GPIO id is whether used by another CPU-CORE */
bk_err_t gpio_dev_unprotect_unmap(gpio_id_t gpio_id)
{
	/* Restore a configuration that is not a secondary function to its initial state. */
	gpio_hal_output_enable(&s_gpio.hal, gpio_id, 0);
	gpio_hal_input_enable(&s_gpio.hal, gpio_id, 0);
	gpio_hal_pull_enable(&s_gpio.hal, gpio_id, 0);
	gpio_hal_disable_interrupt(&s_gpio.hal, gpio_id);
	gpio_hal_func_unmap(&s_gpio.hal, gpio_id);

	return BK_OK;
}

bk_err_t gpio_i2c1_sel(gpio_i2c1_map_mode_t mode)
{
	GPIO_RETURN_ON_INVALID_PERIAL_MODE(mode, GPIO_I2C1_MAP_MODE_MAX);

	GPIO_MAP_TABLE(GPIO_I2C1_USED_GPIO_NUM, GPIO_I2C1_MAP_MODE_MAX, i2c1_gpio_map) = GPIO_I2C1_MAP_TABLE;

	gpio_hal_devs_map(&s_gpio.hal, i2c1_gpio_map[mode].gpio_bits, i2c1_gpio_map[mode].devs, GPIO_I2C1_USED_GPIO_NUM);

	return BK_OK;
}

bk_err_t gpio_i2s_sel(gpio_i2s_map_mode_t mode)
{
	GPIO_RETURN_ON_INVALID_PERIAL_MODE(mode, GPIO_I2S_MAP_MODE_MAX);

	GPIO_MAP_TABLE(GPIO_I2S_USED_GPIO_NUM, GPIO_I2S_MAP_MODE_MAX, i2s_gpio_map) = GPIO_I2S_MAP_TABLE;

	gpio_hal_devs_map(&s_gpio.hal, i2s_gpio_map[mode].gpio_bits, i2s_gpio_map[mode].devs, GPIO_I2S_USED_GPIO_NUM);

	return BK_OK;
}

bk_err_t gpio_spi_sel(gpio_spi1_map_mode_t mode)
{
	GPIO_RETURN_ON_INVALID_PERIAL_MODE(mode, GPIO_SPI_MAP_MODE_MAX);

	GPIO_MAP_TABLE(GPIO_SPI0_USED_GPIO_NUM, GPIO_SPI_MAP_MODE_MAX, spi_gpio_map) = GPIO_SPI0_MAP_TABLE;

	gpio_hal_devs_map(&s_gpio.hal, spi_gpio_map[mode].gpio_bits, spi_gpio_map[mode].devs, GPIO_SPI0_USED_GPIO_NUM);

	return BK_OK;
}

bk_err_t gpio_sdio_sel(gpio_sdio_map_mode_t mode)
{
	GPIO_RETURN_ON_INVALID_PERIAL_MODE(mode, GPIO_SDIO_MAP_MODE_MAX);

	GPIO_MAP_TABLE(GPIO_SDIO_USED_GPIO_NUM, GPIO_SDIO_MAP_MODE_MAX, sdio_gpio_map) = GPIO_SDIO_MAP_TABLE;

	gpio_hal_devs_map(&s_gpio.hal, sdio_gpio_map[mode].gpio_bits, sdio_gpio_map[mode].devs, GPIO_SDIO_USED_GPIO_NUM);

	return BK_OK;
}

bk_err_t gpio_sdio_one_line_sel(gpio_sdio_map_mode_t mode)
{
	GPIO_RETURN_ON_INVALID_PERIAL_MODE(mode, GPIO_SDIO_MAP_MODE_MAX);

	GPIO_MAP_TABLE(GPIO_SDIO_ONE_LINE_USED_GPIO_NUM, GPIO_SDIO_MAP_MODE_MAX, sdio_gpio_map) = GPIO_SDIO_ONE_LINE_MAP_TABLE;

	gpio_hal_devs_map(&s_gpio.hal, sdio_gpio_map[mode].gpio_bits, sdio_gpio_map[mode].devs, GPIO_SDIO_ONE_LINE_USED_GPIO_NUM);

	return BK_OK;
}


bk_err_t gpio_jtag_sel(gpio_jtag_map_group_t group_id)
{
	gpio_dev_unprotect_unmap(GPIO_20);
	gpio_dev_unprotect_unmap(GPIO_21);
	gpio_dev_unprotect_unmap(GPIO_0);
	gpio_dev_unprotect_unmap(GPIO_1);

	if (group_id == GPIO_JTAG_MAP_GROUP0) {
		gpio_dev_unprotect_map(GPIO_20, GPIO_DEV_JTAG_TCK);
		gpio_dev_unprotect_map(GPIO_21, GPIO_DEV_JTAG_TMS);
	} else if (group_id == GPIO_JTAG_MAP_GROUP1) {
		gpio_dev_unprotect_map(GPIO_0, GPIO_DEV_JTAG_TCK);
		gpio_dev_unprotect_map(GPIO_1, GPIO_DEV_JTAG_TMS);
	} else {
		GPIO_LOGI("Unsupported group id(%d).\r\n", group_id);
		return BK_FAIL;
	}

	return BK_OK;
}

#if CONFIG_GPIO_RETENTION_SUPPORT
bk_err_t bk_gpio_retention_set(gpio_id_t gpio_id, gpio_output_state_e gpio_output_state)
{
	bk_err_t ret;

	ret = gpio_retention_map_set(gpio_id, gpio_output_state);

	if (BK_OK != ret)
	{
		GPIO_LOGW("gpio retention set fail: gpio_%d type: %d\r\n", gpio_id, gpio_output_state);
	}

#if GPIO_RETENTION_MAP_DUMP
	GPIO_LOGI("gpio retention map:\r\n");
	uint32_t bitmap = aon_pmu_hal_gpio_retention_bitmap_get();
	for (uint32_t i = 0; i < GPIO_RETENTION_MAP_SIZE; i++)
	{
		gpio_id = s_gpio_retention_map[i];
		gpio_output_state = !!(bitmap & BIT(i));
		if (GPIO_RETENTION_EN_CHECK(gpio_id, i, false))
		{
			GPIO_LOGI("gpio_%d type: %d\r\n", gpio_id, gpio_output_state);
		}
	}
#endif

	return ret;
}

bk_err_t bk_gpio_retention_clr(gpio_id_t gpio_id)
{
	bk_err_t ret;

	ret = gpio_retention_map_clr(gpio_id);

	if (BK_OK != ret)
	{
		GPIO_LOGW("gpio retention set fail: gpio_%d\r\n", gpio_id);
	}

#if GPIO_RETENTION_MAP_DUMP
	GPIO_LOGI("gpio retention map:\r\n");
	uint32_t bitmap = aon_pmu_hal_gpio_retention_bitmap_get();
	gpio_output_state_e gpio_output_state;
	for (uint32_t i = 0; i < GPIO_RETENTION_MAP_SIZE; i++)
	{
		gpio_id = s_gpio_retention_map[i];
		gpio_output_state = !!(bitmap & BIT(i));
		if (GPIO_RETENTION_EN_CHECK(gpio_id, i, false))
		{
			GPIO_LOGI("gpio_%d type: %d\r\n", gpio_id, gpio_output_state);
		}
	}
#endif

	return ret;
}

bk_err_t gpio_retention_map_set(gpio_id_t id, gpio_output_state_e gpio_output_state)
{
	bk_err_t ret = BK_FAIL;
	gpio_id_t gpio_id;
	uint32_t bitmap = aon_pmu_hal_gpio_retention_bitmap_get();

	for (uint32_t i = 0; i < GPIO_RETENTION_MAP_SIZE; i++)
	{
		gpio_id = s_gpio_retention_map[i];
		if (id == gpio_id && gpio_id < GPIO_NUM_MAX)
		{
			s_gpio_retention_en_bitmap |= BIT(i);
			if (GPIO_OUTPUT_STATE_HIGH == gpio_output_state) {
				bitmap |= BIT(i);
			} else if (GPIO_OUTPUT_STATE_LOW == gpio_output_state) {
				bitmap &= ~BIT(i);
			}
			ret = BK_OK;
		}
	}

	aon_pmu_hal_gpio_retention_bitmap_set(bitmap);

	return ret;
}

bk_err_t gpio_retention_map_clr(gpio_id_t id)
{
	bk_err_t ret = BK_FAIL;
	gpio_id_t gpio_id;
	uint32_t bitmap = aon_pmu_hal_gpio_retention_bitmap_get();

	for (uint32_t i = 0; i < GPIO_RETENTION_MAP_SIZE; i++)
	{
		gpio_id = s_gpio_retention_map[i];
		if (id == gpio_id && gpio_id < GPIO_NUM_MAX)
		{
			s_gpio_retention_en_bitmap &= ~BIT(i);
			bitmap &= ~BIT(i);
			ret = BK_OK;
		}
	}

	aon_pmu_hal_gpio_retention_bitmap_set(bitmap);

	return ret;
}

uint64_t gpio_retention_map_get(void)
{
	gpio_id_t gpio_id;
	uint64_t gpio_bitmap = 0;

	for (uint32_t i = 0; i < GPIO_RETENTION_MAP_SIZE; i++)
	{
		gpio_id = s_gpio_retention_map[i];
		if (GPIO_RETENTION_EN_CHECK(gpio_id, i, false))
		{
			gpio_bitmap |= BIT64(gpio_id);
		}
	}

	return gpio_bitmap;
}

void gpio_retention_sync(bool force_flag)
{
	gpio_id_t gpio_id;
	uint32_t bitmap = aon_pmu_hal_gpio_retention_bitmap_get();

	for (uint32_t i = 0; i < GPIO_RETENTION_MAP_SIZE; i++)
	{
		gpio_id = s_gpio_retention_map[i];
		if (GPIO_RETENTION_EN_CHECK(gpio_id, i, force_flag))
		{
			if (bitmap & BIT(i)) {
				GPIO_UP(gpio_id);
			} else {
				GPIO_DOWN(gpio_id);
			}
		}
		if (force_flag)
		{
#if CONFIG_GPIO_RETENTION_DISPOSABLE
			gpio_retention_map_clr(gpio_id);
#else
			s_gpio_retention_en_bitmap |= BIT64(i);
#endif
		}
	}
}
#endif

