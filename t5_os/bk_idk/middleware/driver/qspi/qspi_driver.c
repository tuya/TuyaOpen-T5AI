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

#include <driver/int.h>
#include <os/mem.h>
#include <driver/qspi.h>
#include "bk_drv_model.h"
#include "bk_sys_ctrl.h"
#include "clock_driver.h"
#include "gpio_driver.h"
#include "power_driver.h"
#include "qspi_driver.h"
#include "qspi_statis.h"
#include "sys_driver.h"
#include <driver/gpio.h>

static qspi_driver_t s_qspi[SOC_QSPI_UNIT_NUM] = {
	{
		.hal.hw = (qspi_hw_t *)(SOC_QSPI0_REG_BASE),
	},
#if (SOC_QSPI_UNIT_NUM > 1)
	{
		.hal.hw = (qspi_hw_t *)(SOC_QSPI1_REG_BASE),
	}
#endif
};

#define QSPI_RETURN_ON_NOT_INIT() do {\
		if (!s_qspi_driver_is_init) {\
			QSPI_LOGE("QSPI driver not init\r\n");\
			return BK_ERR_QSPI_NOT_INIT;\
		}\
	} while(0)

#define QSPI_RETURN_ON_ID_NOT_INIT(id) do {\
		if (!s_qspi[id].id_init_bits) {\
			QSPI_LOGE("QSPI not init\r\n");\
			return BK_ERR_QSPI_ID_NOT_INIT;\
		}\
	} while(0)


static bool s_qspi_driver_is_init = false;
static qspi_callback_t s_qspi_tx_isr = {NULL};
static qspi_callback_t s_qspi_rx_isr = {NULL};


#define QSPI_SET_PIN(id) do {\
	gpio_dev_unmap(QSPI##id##_LL_CSN_PIN);\
	gpio_dev_unmap(QSPI##id##_LL_CLK_PIN);\
	gpio_dev_unmap(QSPI##id##_LL_IO0_PIN);\
	gpio_dev_unmap(QSPI##id##_LL_IO1_PIN);\
	gpio_dev_unmap(QSPI##id##_LL_IO2_PIN);\
	gpio_dev_unmap(QSPI##id##_LL_IO3_PIN);\
	gpio_dev_map(QSPI##id##_LL_CSN_PIN, GPIO_DEV_QSPI##id##_CSN);\
	gpio_dev_map(QSPI##id##_LL_CLK_PIN, GPIO_DEV_QSPI##id##_CLK);\
	gpio_dev_map(QSPI##id##_LL_IO0_PIN, GPIO_DEV_QSPI##id##_IO0);\
	gpio_dev_map(QSPI##id##_LL_IO1_PIN, GPIO_DEV_QSPI##id##_IO1);\
	gpio_dev_map(QSPI##id##_LL_IO2_PIN, GPIO_DEV_QSPI##id##_IO2);\
	gpio_dev_map(QSPI##id##_LL_IO3_PIN, GPIO_DEV_QSPI##id##_IO3);\
	bk_gpio_pull_up(QSPI##id##_LL_CSN_PIN);\
	bk_gpio_pull_up(QSPI##id##_LL_IO0_PIN);\
	bk_gpio_pull_up(QSPI##id##_LL_IO1_PIN);\
	bk_gpio_pull_up(QSPI##id##_LL_IO2_PIN);\
	bk_gpio_pull_up(QSPI##id##_LL_IO3_PIN);\
	bk_gpio_set_capacity(QSPI##id##_LL_CSN_PIN, 3);\
	bk_gpio_set_capacity(QSPI##id##_LL_CLK_PIN, 3);\
	bk_gpio_set_capacity(QSPI##id##_LL_IO0_PIN, 3);\
	bk_gpio_set_capacity(QSPI##id##_LL_IO1_PIN, 3);\
	bk_gpio_set_capacity(QSPI##id##_LL_IO2_PIN, 3);\
	bk_gpio_set_capacity(QSPI##id##_LL_IO3_PIN, 3);\
} while(0)


static void qspi_init_gpio(qspi_id_t id)
{
	switch (id) {
	case QSPI_ID_0:
		QSPI_SET_PIN(0);
		break;
#if (SOC_QSPI_UNIT_NUM > 1)
	case QSPI_ID_1:
		QSPI_SET_PIN(1);
		break;
#endif
#if (SOC_QSPI_UNIT_NUM > 2)
	case QSPI_ID_2:
		QSPI_SET_PIN(2);
		break;
#endif
	default:
		break;
	}
}

static void qspi_clock_enable(qspi_id_t id)
{
	switch(id)
	{
		case QSPI_ID_0:
			sys_drv_dev_clk_pwr_up(CLK_PWR_ID_QSPI_1, CLK_PWR_CTRL_PWR_UP);
			break;
#if (SOC_QSPI_UNIT_NUM > 1)
		case QSPI_ID_1:
			sys_drv_dev_clk_pwr_up(CLK_PWR_ID_QSPI_2, CLK_PWR_CTRL_PWR_UP);
			break;
#endif
		default:
			break;
	}
}

static void qspi_clock_disable(qspi_id_t id)
{
	switch(id)
	{
		case QSPI_ID_0:
			sys_drv_dev_clk_pwr_up(CLK_PWR_ID_QSPI_1, CLK_PWR_CTRL_PWR_DOWN);
			break;
#if (SOC_QSPI_UNIT_NUM > 1)
		case QSPI_ID_1:
			sys_drv_dev_clk_pwr_up(CLK_PWR_ID_QSPI_2, CLK_PWR_CTRL_PWR_DOWN);
			break;
#endif
		default:
			break;
	}
}

static void qspi_interrupt_enable(qspi_id_t id)
{
	switch(id)
	{
		case QSPI_ID_0:
			sys_drv_int_enable(QSPI0_INTERRUPT_CTRL_BIT);
			break;
#if (SOC_QSPI_UNIT_NUM > 1)
		case QSPI_ID_1:
			sys_drv_int_enable(QSPI1_INTERRUPT_CTRL_BIT);
			break;
#endif
		default:
			break;
	}
}

static void qspi_interrupt_disable(qspi_id_t id)
{
	switch(id)
	{
		case QSPI_ID_0:
			sys_drv_int_disable(QSPI0_INTERRUPT_CTRL_BIT);
			break;
#if (SOC_QSPI_UNIT_NUM > 1)
		case QSPI_ID_1:
			sys_drv_int_disable(QSPI1_INTERRUPT_CTRL_BIT);
			break;
#endif
		default:
			break;
	}
}

/* 
 * 1. set clock
 * 2. set gpio as qspi
 * 3. enable interrupt(fiq_int_enable)
 */
static void qspi_id_init_common(qspi_id_t id)
{
	qspi_clock_enable(id);
	qspi_init_gpio(id);
	qspi_interrupt_enable(id);

	qspi_hal_init_common(&s_qspi[id].hal);
}

static void qspi_id_deinit_common(qspi_id_t id)
{
	qspi_hal_deinit_common(&s_qspi[id].hal);
	qspi_interrupt_disable(id);
	qspi_clock_disable(id);
}

#if (!CONFIG_SYSTEM_CTRL)
static void qspi_isr(void);
#endif

bk_err_t bk_qspi_driver_init(void)
{
	if (s_qspi_driver_is_init) {
		return BK_OK;
	}

	os_memset(&s_qspi, 0, sizeof(s_qspi));
	for (int id = QSPI_ID_0; id < QSPI_ID_MAX; id++) {
		s_qspi[id].hal.id = id;
		qspi_hal_init(&s_qspi[id].hal);
	}

	qspi_statis_init();
	s_qspi_driver_is_init = true;

	return BK_OK;
}

bk_err_t bk_qspi_driver_deinit(void)
{
	if (!s_qspi_driver_is_init) {
		return BK_OK;
	}

	for (int id = QSPI_ID_0; id < QSPI_ID_MAX; id++) {
		qspi_id_deinit_common(id);
	}
	s_qspi_driver_is_init = false;

	return BK_OK;
}

bk_err_t bk_qspi_init(qspi_id_t id, const qspi_config_t *config)
{
	BK_RETURN_ON_NULL(config);
	QSPI_RETURN_ON_NOT_INIT();

	qspi_id_init_common(id);
	switch (config->src_clk) {
	case QSPI_SCLK_320M:
		sys_drv_qspi_clk_sel(id, QSPI_CLK_320M);
		break;
	case QSPI_SCLK_480M:
		sys_drv_qspi_clk_sel(id, QSPI_CLK_480M);
		break;
	default:
		sys_drv_qspi_clk_sel(id, QSPI_CLK_480M);
		break;
	}
	sys_drv_qspi_set_src_clk_div(id, config->src_clk_div);
	qspi_hal_set_clk_div(&s_qspi[id].hal, config->clk_div);
	s_qspi[id].id_init_bits |= BIT(0);
	return BK_OK;
}

bk_err_t bk_qspi_deinit(qspi_id_t id)
{
	qspi_id_deinit_common(id);
	s_qspi[id].id_init_bits &= ~BIT(0);
	return BK_OK;
}

bk_err_t bk_qspi_command(qspi_id_t id, const qspi_cmd_t *cmd)
{
	BK_RETURN_ON_NULL(cmd);
	QSPI_RETURN_ON_ID_NOT_INIT(id);
	qspi_hal_command(&s_qspi[id].hal, cmd);
	return BK_OK;
}

bk_err_t bk_qspi_write(qspi_id_t id, const void *data, uint32_t size)
{
	BK_RETURN_ON_NULL(data);

	qspi_hal_io_write(&s_qspi[id].hal, data, size);

	return BK_OK;
}

void bk_qspi_write_cmd(qspi_id_t id, uint8_t cmd)
{
	qspi_hal_set_cmd_c_l(&s_qspi[id].hal, 0);
    qspi_hal_set_cmd_c_h(&s_qspi[id].hal, 0);
    qspi_hal_set_cmd_c_cfg1(&s_qspi[id].hal, 0);
    qspi_hal_set_cmd_c_cfg2(&s_qspi[id].hal, 0);

    qspi_hal_set_cmd_c_h(&s_qspi[id].hal, cmd);
    qspi_hal_set_cmd_c_cfg1(&s_qspi[id].hal, 0xC);
    qspi_hal_cmd_c_start(&s_qspi[id].hal);
    qspi_hal_wait_cmd_done(&s_qspi[id].hal);
}

static void bk_qspi_write_word_direct(qspi_id_t id, uint8_t *data, uint32_t data_len)
{
	uint32_t value = 0;

	if(data_len > 4) {
		bk_printf("word length %d is too long\r\n", data_len);
		return;
	}

    qspi_hal_set_cmd_c_l(&s_qspi[id].hal, 0);
    qspi_hal_set_cmd_c_h(&s_qspi[id].hal, 0);
    qspi_hal_set_cmd_c_cfg1(&s_qspi[id].hal, 0);
    qspi_hal_set_cmd_c_cfg2(&s_qspi[id].hal, 0);

    for (uint8_t i = 0; i < data_len; i++) {
        value |= (data[i] << (i * 8));
    }

    qspi_hal_set_cmd_c_h(&s_qspi[id].hal, value);
    qspi_hal_set_cmd_c_cfg1(&s_qspi[id].hal, 0x3 << (data_len * 2));
    qspi_hal_cmd_c_start(&s_qspi[id].hal);
    qspi_hal_wait_cmd_done(&s_qspi[id].hal);
}

void bk_qspi_write_data_indirect_mode(qspi_id_t id, uint8_t *data, uint32_t data_len)
{
    uint32_t value = 0;
    uint32_t send_len = 0;
    uint32_t remain_len = data_len;
    uint8_t *data_tmp = data;

    qspi_hal_set_cmd_c_l(&s_qspi[id].hal, 0);
    qspi_hal_set_cmd_c_h(&s_qspi[id].hal, 0);
    qspi_hal_set_cmd_c_cfg1(&s_qspi[id].hal, 0);
    qspi_hal_set_cmd_c_cfg2(&s_qspi[id].hal, 0);

    while (remain_len > 0) {
        if (remain_len <= 4) {
            bk_qspi_write_word_direct(id, data, remain_len);
            break;
        }

        value = (data_tmp[3] << 24) | (data_tmp[2] << 16) | (data_tmp[1] << 8) | data_tmp[0];
        remain_len -= 4;
        send_len = remain_len < 0x100 ? remain_len : 0x100;
        qspi_hal_set_cmd_c_h(&s_qspi[id].hal, value);
        qspi_hal_set_cmd_c_cfg1(&s_qspi[id].hal, 0x300);
        qspi_hal_set_cmd_c_cfg2(&s_qspi[id].hal, send_len << 2);
        data_tmp += 4;
        bk_qspi_write(id, data_tmp, send_len);
        qspi_hal_cmd_c_start(&s_qspi[id].hal);
        qspi_hal_wait_cmd_done(&s_qspi[id].hal);
        remain_len -= send_len;
        data_tmp += send_len;
    }
}


bk_err_t bk_qspi_read(qspi_id_t id, void *data, uint32_t size)
{
	BK_RETURN_ON_NULL(data);

	qspi_hal_io_read(&s_qspi[id].hal, data, size);

	return BK_OK;
}

bk_err_t bk_qspi_register_tx_isr(qspi_isr_t isr, void *param)
{
	uint32_t int_level = rtos_enter_critical();
	s_qspi_tx_isr.callback = isr;
	s_qspi_tx_isr.param = param;
	rtos_exit_critical(int_level);
	return BK_OK;
}

bk_err_t bk_qspi_register_rx_isr(qspi_isr_t isr, void *param)
{
	uint32_t int_level = rtos_enter_critical();
	s_qspi_rx_isr.callback = isr;
	s_qspi_rx_isr.param = param;
	rtos_exit_critical(int_level);
	return BK_OK;
}

#if (!CONFIG_SYSTEM_CTRL)
static void qspi_isr(void)
{
	qspi_hal_t *hal = &s_qspi[id].hal;
	uint32_t int_status = 0;
	QSPI_STATIS_DEC();
	QSPI_STATIS_GET(qspi_statis);
	QSPI_STATIS_INC(qspi_statis->qspi_int_cnt);

	int_status = qspi_hal_get_interrupt_status_before_mask(hal);
	QSPI_LOGD("int_status before mask:%x\r\n", int_status);

	int_status = qspi_hal_get_interrupt_status_after_mask(hal);
	QSPI_LOGD("int_status after mask:%x\r\n", int_status);
	qspi_hal_clear_interrupt_status(hal, int_status);

	if (qspi_hal_is_sw_op_int_triggered(hal, int_status)) {
		QSPI_STATIS_INC(qspi_statis->sw_op_int_cnt);
		QSPI_LOGD("sw op int triggered\r\n");
		qspi_hal_clear_sw_op_int(hal);
		qspi_hal_stop_sw_op(hal);
		if (qspi_hal_is_cur_sw_op_write_data()) {
			qspi_hal_disable_ge0_tx(hal);
			if (s_qspi_tx_isr.callback) {
				s_qspi_tx_isr.callback(0, s_qspi_tx_isr.param);
			}
		}

		if (qspi_hal_is_cur_sw_op_read_data()) {
			qspi_hal_disable_ge1_rx(hal);
			if (s_qspi_rx_isr.callback) {
				s_qspi_rx_isr.callback(0, s_qspi_rx_isr.param);
			}
		}
	}
}
#endif
