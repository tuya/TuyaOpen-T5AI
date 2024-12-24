// Copyright 2023-2024 Beken
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
#include <driver/gpio.h>
#include <driver/int.h>
#include <driver/lin.h>
#include <driver/lin_types.h>
#include "clock_driver.h"
#include "gpio_driver.h"
#include "power_driver.h"
#include "lin_statis.h"
#include "lin_driver.h"
#include "lin_hal.h"
#include "sys_driver.h"
#include "FreeRTOS.h"
#include "task.h"
#if CONFIG_LIN_PM_CB_SUPPORT
#include <modules/pm.h>
#endif

#if CONFIG_LIN_DEVICE_MASTER
#define	LIN_DEV              LIN_MASTER
#define LIN_DEV_ID           LIN_IDENT0
#define LIN_CHN              LIN_CHAN_2
#else
#define	LIN_DEV              LIN_SLAVE
#define LIN_DEV_ID           LIN_IDENT0
#define LIN_CHN              LIN_CHAN_2
#endif

#define LIN_TAG             "lin"
#define LIN_BAUD_RATE       (2000)

#define LIN_RETURN_ON_DEVICE_NOT_INIT() do { \
		if (!s_lin_driver_is_init) { \
			LIN_LOGE("lin driver not init\r\n"); \
			return BK_ERR_LIN_NOT_INIT; \
		} \
	} while(0)

#define LIN_RETURN_ON_INVALID_INT(type) do { \
		if ((type) >= LIN_INT_MAX) { \
			return BK_ERR_LIN_INT_TYPE; \
		} \
	} while(0)

static lin_callback_t s_lin_isr[LIN_INT_MAX] = {NULL};
static bool s_lin_driver_is_init = false;
static lin_buf_t s_lin_buf;
static TaskHandle_t lin_xBlockTasks;
static lin_gpio_t s_lin_gpio[LIN_CHAN_MAX] = LIN_GPIO_MAP;
static lin_config_t s_lin_def_cfg = {
	.chn = LIN_CHN,
	.dev = LIN_DEV,
	.length = LIN_DATA_LEN_8BYTES,
	.checksum = LIN_CLASSIC,
	.rate = LIN_BAUD_RATE,
	.bus_inactiv_time = LIN_BUS_INACTIVITY_10S,
	.wup_repeat_time = LIN_WUP_REPEAT_240MS,
	};

#if (CONFIG_LIN_PM_CB_SUPPORT)
#define LIN_PM_CHECK_RESTORE() do {\
	if (bk_pm_module_lv_sleep_state_get(PM_DEV_ID_LIN)) {\
		bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_LIN, PM_POWER_MODULE_STATE_ON);\
		lin_pm_restore(0, NULL);\
		bk_pm_module_lv_sleep_state_clear(PM_DEV_ID_LIN);\
	}\
} while(0)

static uint32_t s_lin_pm_backup[LIN_PM_BACKUP_REG_NUM];
static uint8_t s_lin_pm_backup_is_valid;
static int lin_pm_backup(uint64_t sleep_time, void *args)
{
	LIN_RETURN_ON_DEVICE_NOT_INIT();
	if (!s_lin_pm_backup_is_valid)
	{
		s_lin_pm_backup_is_valid = 1;
		lin_hal_backup(&s_lin_pm_backup[0]);
		sys_drv_dev_clk_pwr_up(CLK_PWR_ID_LIN, CLK_PWR_CTRL_PWR_DOWN);
	}

	return BK_OK;
}

static int lin_pm_restore(uint64_t sleep_time, void *args)
{

	LIN_RETURN_ON_DEVICE_NOT_INIT();

	if (s_lin_pm_backup_is_valid)
	{
		sys_drv_dev_clk_pwr_up(CLK_PWR_ID_LIN, CLK_PWR_CTRL_PWR_UP);
		lin_hal_restore(&s_lin_pm_backup[0]);
		s_lin_pm_backup_is_valid = 0;
	}

	return BK_OK;
}
#else
#define LIN_PM_CHECK_RESTORE()
#endif

bk_err_t bk_lin_gpio_init(lin_channel_t chn)
{
	if (chn >= LIN_CHAN_MAX || chn < LIN_CHAN_0) {
			LIN_LOGD("unsupported lin chnnal\r\n");
			return BK_ERR_PARAM;
	}
	BK_LOG_ON_ERR(gpio_dev_unmap(s_lin_gpio[chn].tx_gpio.id));
	BK_LOG_ON_ERR(gpio_dev_map(s_lin_gpio[chn].tx_gpio.id, s_lin_gpio[chn].tx_gpio.dev));
	BK_LOG_ON_ERR(gpio_dev_unmap(s_lin_gpio[chn].rx_gpio.id));
	BK_LOG_ON_ERR(gpio_dev_map(s_lin_gpio[chn].rx_gpio.id, s_lin_gpio[chn].rx_gpio.dev));
	BK_LOG_ON_ERR(gpio_dev_unmap(s_lin_gpio[chn].sleep_gpio.id));
	BK_LOG_ON_ERR(gpio_dev_map(s_lin_gpio[chn].sleep_gpio.id, s_lin_gpio[chn].sleep_gpio.dev));

	return BK_OK;
}

static bk_err_t bk_lin_gpio_deinit(lin_channel_t chn)
{
	if (chn >= LIN_CHAN_MAX || chn < LIN_CHAN_0) {
			LIN_LOGD("unsupported lin chnnal\r\n");
			return BK_ERR_PARAM;
	}
	BK_LOG_ON_ERR(gpio_dev_unmap(s_lin_gpio[chn].tx_gpio.id));
	BK_LOG_ON_ERR(gpio_dev_unmap(s_lin_gpio[chn].rx_gpio.id));
	BK_LOG_ON_ERR(gpio_dev_unmap(s_lin_gpio[chn].sleep_gpio.id));

	return BK_OK;
}

bk_err_t bk_lin_clock_enable(void)
{
	sys_drv_dev_clk_pwr_up(CLK_PWR_ID_LIN, CLK_PWR_CTRL_PWR_UP);

	return BK_OK;
}

bk_err_t bk_lin_clock_disable(void)
{
	sys_drv_dev_clk_pwr_up(CLK_PWR_ID_LIN, CLK_PWR_CTRL_PWR_DOWN);

	return BK_OK;
}

bk_err_t bk_lin_interrupt_enable(void)
{
	sys_drv_int_group2_enable(LIN_INTERRUPT_CTRL_BIT);

	return BK_OK;
}

bk_err_t bk_lin_interrupt_disable(void)
{
	sys_drv_int_group2_disable(LIN_INTERRUPT_CTRL_BIT);

	return BK_OK;
}

void lin_error(void)
{
	__attribute__((__unused__)) lin_statis_t *lin_statis = lin_statis_get_statis(lin_hal_get_ident_id());

	if (lin_hal_get_error_bit() != 0) {
		LIN_LOGD("bit error occurs during data transmission!!!\n");
		if (s_lin_isr[LIN_INT_BIT_ERR].isr) {
			s_lin_isr[LIN_INT_BIT_ERR].isr(s_lin_isr[LIN_INT_BIT_ERR].arg);
		}
		LIN_STATIS_INC(lin_statis->bit_err_cnt);
	} else if (lin_hal_get_error_chk() != 0 ) {
		LIN_LOGD("checksum error occurs!!!\n");
		if (s_lin_isr[LIN_INT_CHK_ERR].isr) {
			s_lin_isr[LIN_INT_CHK_ERR].isr(s_lin_isr[LIN_INT_CHK_ERR].arg);
		}
		LIN_STATIS_INC(lin_statis->chk_err_cnt);
	} else if (lin_hal_get_error_timeout() != 0) {
		LIN_LOGD("timeout error occurs!!!\n");
		if (s_lin_isr[LIN_INT_TIMEOUT_ERR].isr) {
			s_lin_isr[LIN_INT_TIMEOUT_ERR].isr(s_lin_isr[LIN_INT_TIMEOUT_ERR].arg);
		}
		LIN_STATIS_INC(lin_statis->timeout_err_cnt);
	} else if (lin_hal_get_error_parity() != 0) {

		LIN_LOGD("parity occurs!!!\n");
		if (s_lin_isr[LIN_INT_PARITY_ERR].isr) {
			s_lin_isr[LIN_INT_PARITY_ERR].isr(s_lin_isr[LIN_INT_PARITY_ERR].arg);
		}
		LIN_STATIS_INC(lin_statis->parity_err_cnt);
	} else {
		LIN_LOGD("lin unknown interrupt error!!!\n");
	}
}

static void lin_isr_master(void)
{
	__attribute__((__unused__)) lin_statis_t *lin_statis = lin_statis_get_statis(lin_hal_get_ident_id());

	if (lin_hal_get_status_error() != 0) {
		lin_error();
		LIN_STATIS_INC(lin_statis->error_cnt);
	} else if (lin_hal_get_status_wake_up() != 0) {
		/* TODO, after waking up, you can do corresponding operations */
		LIN_LOGD("The device has sent or received a wakeup signal!!!\n");
		LIN_STATIS_INC(lin_statis->parity_err_cnt);
		if (s_lin_isr[LIN_INT_WAKEUP].isr) {
			s_lin_isr[LIN_INT_WAKEUP].isr(s_lin_isr[LIN_INT_WAKEUP].arg);
		}
		LIN_STATIS_INC(lin_statis->wakeup_cnt);
	} else if (lin_hal_get_status_complete() != 0) {
		/* TODO, you can do corresponding operations */
		LIN_LOGD("master transmit completed\n");
		if (s_lin_isr[LIN_INT_COMPLETE].isr) {
			s_lin_isr[LIN_INT_COMPLETE].isr(s_lin_isr[LIN_INT_COMPLETE].arg);
		}

		LIN_STATIS_INC(lin_statis->completed_cnt);
	} else {
		LIN_LOGD("lin unknown interrupt!!!\n");
	}
	lin_hal_set_ctrl_reset_error();
	lin_hal_set_ctrl_reset_int();
	LIN_STATIS_INC(lin_statis->isr_cnt);
}

static void lin_isr_slave(void)
{
	__attribute__((__unused__)) lin_statis_t *lin_statis = lin_statis_get_statis(lin_hal_get_ident_id());

	if (lin_hal_get_status_error() != 0) {
		lin_error();
		LIN_STATIS_INC(lin_statis->error_cnt);
	} else if (lin_hal_get_status_data_req() != 0) {
		LIN_LOGD("data_req occures!!!\n");
		if (s_lin_isr[LIN_INT_DATA_REQ].isr) {
			s_lin_isr[LIN_INT_DATA_REQ].isr(s_lin_isr[LIN_INT_DATA_REQ].arg);
		}
		LIN_STATIS_INC(lin_statis->data_req_cnt);
	} else if (lin_hal_get_status_aborted() != 0) {
		LIN_LOGD("transmission is aborted!!!\n");
		if (s_lin_isr[LIN_INT_ABORTED].isr) {
			s_lin_isr[LIN_INT_ABORTED].isr(s_lin_isr[LIN_INT_ABORTED].arg);
		}
		LIN_STATIS_INC(lin_statis->aborted_cnt);
	} else if (lin_hal_get_status_bus_idle_timeout() != 0) {
		LIN_LOGD("bus idle timeout occurs,go to sleep!!!\n");
		if (s_lin_isr[LIN_INT_IDLE_TIMEOUT].isr) {
			s_lin_isr[LIN_INT_IDLE_TIMEOUT].isr(s_lin_isr[LIN_INT_IDLE_TIMEOUT].arg);
		}
		bk_lin_set_sleep();
		LIN_STATIS_INC(lin_statis->bus_idle_timeout_cnt);
	} else if (lin_hal_get_status_wake_up() != 0) {
		/* TODO, after waking up, you can do corresponding operations */
		LIN_LOGD("The device has sent or received a wakeup signal!!!\n");
		if (s_lin_isr[LIN_INT_WAKEUP].isr) {
			s_lin_isr[LIN_INT_WAKEUP].isr(s_lin_isr[LIN_INT_WAKEUP].arg);
		}
		// bk_lin_set_wakeup();
		LIN_STATIS_INC(lin_statis->wakeup_cnt);
	} else if (lin_hal_get_status_complete() != 0) {
		/* TODO, you can do corresponding operations */
		LIN_LOGD("transmission is completed!!!\n");
		if (s_lin_isr[LIN_INT_COMPLETE].isr) {
			s_lin_isr[LIN_INT_COMPLETE].isr(s_lin_isr[LIN_INT_COMPLETE].arg);
		}
		LIN_STATIS_INC(lin_statis->completed_cnt);
	} else {
		LIN_LOGD("lin unknown interrupt!!!\n");
	}
	lin_hal_set_ctrl_reset_error();
	lin_hal_set_ctrl_reset_int();
}

static void lin_isr(void)
{
	if (lin_hal_get_master() == LIN_MASTER) {
		lin_isr_master();
	} else {
		lin_isr_slave();
	}
}

bk_err_t bk_lin_set_dev(lin_dev_t dev)
{
	LIN_RETURN_ON_DEVICE_NOT_INIT();
	LIN_PM_CHECK_RESTORE();
	lin_hal_master_cfg(dev);

	return BK_OK;
}

bk_err_t bk_lin_get_dev(void)
{
	LIN_RETURN_ON_DEVICE_NOT_INIT();
	LIN_PM_CHECK_RESTORE();
	return lin_hal_get_master();
}

bk_err_t bk_lin_set_ident(uint32_t ident)
{
	LIN_RETURN_ON_DEVICE_NOT_INIT();
	LIN_PM_CHECK_RESTORE();
	lin_hal_set_ident_value(ident & LIN_IDENT_MASK);

	return BK_OK;
}

bk_err_t bk_lin_get_ident(void)
{
	LIN_RETURN_ON_DEVICE_NOT_INIT();
	LIN_PM_CHECK_RESTORE();
	return lin_hal_get_ident_value();
}

bk_err_t bk_lin_set_ident_id(uint32_t id)
{
	LIN_RETURN_ON_DEVICE_NOT_INIT();
	LIN_PM_CHECK_RESTORE();
	lin_hal_set_ident_id(id);

	return BK_OK;
}

uint32_t bk_lin_get_ident_id(void)
{
	LIN_RETURN_ON_DEVICE_NOT_INIT();
	LIN_PM_CHECK_RESTORE();
	return lin_hal_get_ident_id();
}

bk_err_t bk_lin_set_rate(double rate)
{
	LIN_RETURN_ON_DEVICE_NOT_INIT();
	LIN_PM_CHECK_RESTORE();
	lin_hal_set_rate(rate);

	return BK_OK;
}

double bk_lin_get_rate(void)
{
	LIN_RETURN_ON_DEVICE_NOT_INIT();
	LIN_PM_CHECK_RESTORE();
	return lin_hal_get_rate();
}

bk_err_t bk_lin_set_data_length(lin_data_len_t len)
{
	LIN_RETURN_ON_DEVICE_NOT_INIT();
	LIN_PM_CHECK_RESTORE();
	if (len > LIN_DATA_LEN_8BYTES) {
		len = LIN_DATA_LEN_8BYTES;
	}
	lin_hal_set_type_data_length(len);

	return BK_OK;
}

bk_err_t bk_lin_get_data_length(void)
{
	LIN_RETURN_ON_DEVICE_NOT_INIT();
	LIN_PM_CHECK_RESTORE();
	return lin_hal_get_type_data_length();
}

bk_err_t bk_lin_set_enh_check(lin_checksum_t enh_check)
{
	LIN_RETURN_ON_DEVICE_NOT_INIT();
	LIN_PM_CHECK_RESTORE();
	lin_hal_set_type_enh_check(enh_check);

	return BK_OK;
}

bk_err_t bk_lin_get_enh_check(void)
{
	LIN_RETURN_ON_DEVICE_NOT_INIT();
	LIN_PM_CHECK_RESTORE();
	return lin_hal_get_type_enh_check();
}

bk_err_t bk_lin_set_wup_repeat_time(lin_wup_repeat_time_t time)
{
	LIN_RETURN_ON_DEVICE_NOT_INIT();
	LIN_PM_CHECK_RESTORE();
	lin_hal_set_tcfg_wup_repeat_time(time);

	return BK_OK;
}

bk_err_t bk_lin_get_wup_repeat_time(void)
{
	LIN_RETURN_ON_DEVICE_NOT_INIT();
	LIN_PM_CHECK_RESTORE();
	return lin_hal_get_tcfg_wup_repeat_time();
}

bk_err_t bk_lin_set_bus_inactivity_time(lin_bus_inactivity_time_t time)
{
	LIN_RETURN_ON_DEVICE_NOT_INIT();
	LIN_PM_CHECK_RESTORE();
	lin_hal_set_tcfg_bus_inactivity_time(time);

	return BK_OK;
}

bk_err_t bk_lin_get_bus_inactivity_time(void)
{
	LIN_RETURN_ON_DEVICE_NOT_INIT();
	LIN_PM_CHECK_RESTORE();
	return lin_hal_get_tcfg_bus_inactivity_time();
}

bk_err_t bk_lin_set_transmit(lin_trans_t trans)
{
	LIN_RETURN_ON_DEVICE_NOT_INIT();
	LIN_PM_CHECK_RESTORE();
	lin_hal_set_ctrl_transmit(trans);

	return BK_OK;
}

bk_err_t bk_lin_get_transmit(void)
{
	LIN_RETURN_ON_DEVICE_NOT_INIT();
	LIN_PM_CHECK_RESTORE();
	return lin_hal_get_ctrl_transmit();
}

bk_err_t bk_lin_set_start_req(void)
{
	LIN_RETURN_ON_DEVICE_NOT_INIT();
	LIN_PM_CHECK_RESTORE();
	lin_hal_set_ctrl_start_req();

	return BK_OK;
}

bk_err_t bk_lin_get_start_req(void)
{
	LIN_RETURN_ON_DEVICE_NOT_INIT();
	LIN_PM_CHECK_RESTORE();
	return lin_hal_get_ctrl_start_req();
}

bk_err_t bk_lin_set_sleep(void)
{
	LIN_RETURN_ON_DEVICE_NOT_INIT();
	LIN_PM_CHECK_RESTORE();
	lin_hal_set_ctrl_sleep();

	return BK_OK;
}

bk_err_t bk_lin_get_sleep(void)
{
	LIN_RETURN_ON_DEVICE_NOT_INIT();
	LIN_PM_CHECK_RESTORE();
	return lin_hal_get_ctrl_sleep();
}

bk_err_t bk_lin_set_wakeup(void)
{
	LIN_RETURN_ON_DEVICE_NOT_INIT();
	LIN_PM_CHECK_RESTORE();
	lin_hal_set_ctrl_wakeup();

	return BK_OK;
}

static void bk_lin_complete_func(void *arg)
{
	if (lin_hal_get_ctrl_transmit() == LIN_RECV) {

		if (lin_xBlockTasks != NULL) {
			vTaskNotifyGiveFromISR(lin_xBlockTasks, NULL);
			lin_xBlockTasks = NULL;
		}
		LIN_LOGD("recevie completed\r\n");
	} else {
		LIN_LOGD("send completed\r\n");
	}
}

static void bk_lin_data_req_func(void *arg)
{
	uint32_t ident, id, dir;
	lin_buf_t *lin_buf = (lin_buf_t *)arg;

	if (!lin_buf) {
		LIN_LOGE("data_req arg is NULL!!!\n");
		return;
	}

	if (lin_buf->id >= LIN_IDENT_MAX) {
		LIN_LOGE("error: unknown id!!!\n");
		return;
	}
	/* a. Load the identifier from the field "id" from register IDENT and process it */
	ident = bk_lin_get_ident();
	id = ident & LIN_IDENT_ID_MASK;
	dir = (ident >> LIN_IDENT_DIR_POS) & LIN_IDENT_DIR_MASK;

	if (id == lin_buf->id) {
		/* b. Adjust the bit "transmit" in register CTRL */
		/* c. Load the "data length" and "set enh_check" in register TYPE */
		/* d. Load the data to transmit into the data buffer (for transmit operation only) */
		/* e. Set the bit "data ack" in register CTRL. */
		bk_lin_set_data_length(lin_buf->len);
		if (dir == LIN_SEND) {
			lin_hal_set_ctrl_transmit(LIN_RECV);

		} else {
			lin_hal_set_ctrl_transmit(LIN_SEND);
			bk_lin_send(lin_buf->tx_buf, lin_buf->len);
		}

		lin_hal_set_ctrl_data_ack();
	} else {
		lin_hal_set_ctrl_stop();
	}

}

static void bk_lin_wakeup_func(void *arg)
{
	/* TODO, you can do corresponding operations */
	LIN_LOGD("The device has sent or received a wakeup signal!!!\n");
}

static void bk_lin_idle_timeout_func(void *arg)
{
	/* TODO, you can do corresponding operations */
	LIN_LOGD("bus idle timeout occurs!!!\n");

}

static void bk_lin_aborted_func(void *arg)
{
	/* TODO, you can do corresponding operations */
	LIN_LOGD("transmission is aborted!!!\n");
}

static void bk_lin_bit_error_func(void *arg)
{
	/* TODO, you can do corresponding operations */
	LIN_LOGD("bit error occurs during data transmission!!!\n");
}
static void bk_lin_chk_error_func(void *arg)
{
	/* TODO, you can do corresponding operations */
	LIN_LOGD("checksum error occurs!!!\n");
}

static void bk_lin_timeout_error_func(void *arg)
{
	/* TODO, you can do corresponding operations */
	LIN_LOGD("timeout error occurs!!!\n");
}

static void bk_lin_parity_error_func(void *arg)
{
	/* TODO, you can do corresponding operations */
	LIN_LOGD("parity occurs!!!\n");
}

bk_err_t bk_lin_send(uint8_t *buf, uint32_t len)
{
	int i;
	uint32_t size = len;

	LIN_RETURN_ON_DEVICE_NOT_INIT();
	LIN_PM_CHECK_RESTORE();
	BK_RETURN_ON_NULL(buf);

	if (len >= LIN_DATA_LEN_MAX) {
		LIN_LOGE("length exceeds max\r\n");
		size = LIN_DATA_LEN_8BYTES;
	}

	for (i = 0; i < size; i++) {
		lin_hal_set_data_byte(i, buf[i]);
	}

	return BK_OK;
}

bk_err_t bk_lin_recv(uint8_t *buf, uint32_t len)
{
	int i;
	uint32_t size = len;
	LIN_RETURN_ON_DEVICE_NOT_INIT();
	LIN_PM_CHECK_RESTORE();
	BK_RETURN_ON_NULL(buf);

	if (len >= LIN_DATA_LEN_MAX) {
		LIN_LOGE("length exceeds max\r\n");
		size = LIN_DATA_LEN_8BYTES;
	}
	os_memset(buf, 0, len);

	for (i = 0; i < size; i++) {
		buf[i] = lin_hal_get_data_byte(i);
	}

	return BK_OK;
}

bk_err_t bk_lin_tx(lin_id_t id, uint8_t *tx, uint32_t len)
{
	uint32_t size = len;
	uint32_t ident = id | BIT(4);
	LIN_RETURN_ON_DEVICE_NOT_INIT();
	LIN_PM_CHECK_RESTORE();
	BK_RETURN_ON_NULL(tx);

	if (id >= LIN_IDENT_MAX) {
		LIN_LOGE("id exceeds max\r\n");
		return BK_FAIL;
	}

	if (len >= LIN_DATA_LEN_MAX) {
		LIN_LOGE("length exceeds max\r\n");
		size = LIN_DATA_LEN_8BYTES;
	}

	lin_hal_set_ctrl_reset_int();
	bk_lin_interrupt_enable();
	if (bk_lin_get_sleep() != 0) {
		bk_lin_set_wakeup();
	}
	if (lin_hal_get_master() == LIN_MASTER) {
		BK_RETURN_ON_ERR(bk_lin_get_start_req());
		bk_lin_set_ident(ident);
		bk_lin_set_data_length(size);
		bk_lin_set_transmit(LIN_SEND);
		bk_lin_send(tx, size);
		if (bk_lin_get_sleep() != 0) {
			bk_lin_set_wakeup();
		}
		bk_lin_set_start_req();
	} else {
		s_lin_buf.id = id;
		os_memset(s_lin_buf.tx_buf, 0, size);
		os_memcpy(s_lin_buf.tx_buf, tx, size);
		s_lin_buf.len = size;
		bk_lin_register_isr(LIN_INT_DATA_REQ, bk_lin_data_req_func, (void *)&s_lin_buf);
	}
	return BK_OK;
}

bk_err_t bk_lin_rx(lin_id_t id, uint8_t *rx, uint32_t len, uint32_t timeout)
{
	uint32_t size = len;
	int ret;
	uint32_t ident = id & ~BIT(4);

	LIN_RETURN_ON_DEVICE_NOT_INIT();
	LIN_PM_CHECK_RESTORE();
	BK_RETURN_ON_NULL(rx);

	if (id >= LIN_IDENT_MAX) {
		LIN_LOGE("id exceeds max\r\n");
		return BK_FAIL;
}

	if (len >= LIN_DATA_LEN_MAX) {
		LIN_LOGE("length exceeds max\r\n");
		size = LIN_DATA_LEN_8BYTES;
	}

	bk_lin_interrupt_enable();
	if (bk_lin_get_sleep() != 0) {
		bk_lin_set_wakeup();
	}

	if (lin_hal_get_master() == LIN_MASTER) {
		bk_lin_set_ident(ident);
		bk_lin_set_data_length(size);
		bk_lin_set_transmit(LIN_RECV);
		bk_lin_set_start_req();
	} else {
		s_lin_buf.id = id;
		os_memset(s_lin_buf.tx_buf, 0, size);
		os_memset(s_lin_buf.rx_buf, 0, size);
		s_lin_buf.len = size;
		bk_lin_register_isr(LIN_INT_DATA_REQ, bk_lin_data_req_func, (void *)&s_lin_buf);
	}

	lin_xBlockTasks = xTaskGetCurrentTaskHandle();
	xTaskNotifyStateClear(NULL);
	ret = ulTaskNotifyTake(pdTRUE, timeout);
	if(ret < 0) {
		LIN_LOGE("error occurred!!!\r\n");
		lin_hal_set_ctrl_reset_int();
		return ret;
	} else if (ret == 0) {
		LIN_LOGE("TaskNotify timeout occurred!!!\r\n");
	}
	bk_lin_recv(rx, size);
	return ret;
}

bk_err_t bk_lin_register_isr(lin_int_type_t type, lin_isr_t isr, void *arg)
{
	LIN_RETURN_ON_DEVICE_NOT_INIT();
	LIN_PM_CHECK_RESTORE();
	LIN_RETURN_ON_INVALID_INT(type);
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	s_lin_isr[type].isr = isr;
	s_lin_isr[type].arg = arg;
	GLOBAL_INT_RESTORE();

	return BK_OK;
}

bk_err_t bk_lin_cfg(lin_config_t *cfg)
{
	BK_RETURN_ON_NULL(cfg);
	LIN_RETURN_ON_DEVICE_NOT_INIT();
	LIN_PM_CHECK_RESTORE();

	BK_LOG_ON_ERR(bk_lin_gpio_init(cfg->chn));
	lin_hal_master_cfg(cfg->dev);
	lin_hal_set_rate(cfg->rate);
	lin_hal_set_type_enh_check(cfg->checksum);
	lin_hal_set_tcfg_bus_inactivity_time(cfg->bus_inactiv_time);
	lin_hal_set_tcfg_wup_repeat_time(cfg->wup_repeat_time);

	return BK_OK;
}

bk_err_t bk_lin_driver_init(void)
{
	if (s_lin_driver_is_init) {
		return BK_OK;
	}

	os_memset(&s_lin_isr, 0, sizeof(s_lin_isr));
	s_lin_buf.id = LIN_DEV_ID;
	s_lin_buf.len = LIN_DATA_LEN_8BYTES;

#if (CONFIG_LIN_PM_CB_SUPPORT)
	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_LIN, PM_POWER_MODULE_STATE_ON);
	pm_cb_conf_t enter_config = {lin_pm_backup, NULL};
	bk_pm_sleep_register_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_LIN, &enter_config, NULL);
	bk_pm_module_lv_sleep_state_clear(PM_DEV_ID_LIN);
#endif

	bk_lin_clock_enable();
	bk_lin_interrupt_disable();
	lin_hal_init();
	s_lin_driver_is_init = true;
	if (bk_lin_cfg(&s_lin_def_cfg) != BK_OK) {
		LIN_LOGE("lin config failed\r\n");
		bk_lin_driver_deinit();
		return BK_ERR_NOT_INIT;
	}
	lin_statis_init();
	bk_lin_register_isr(LIN_INT_WAKEUP, bk_lin_wakeup_func, NULL);
	bk_lin_register_isr(LIN_INT_BIT_ERR, bk_lin_bit_error_func, NULL);
	bk_lin_register_isr(LIN_INT_CHK_ERR, bk_lin_chk_error_func, NULL);
	bk_lin_register_isr(LIN_INT_TIMEOUT_ERR, bk_lin_timeout_error_func, NULL);
	bk_lin_register_isr(LIN_INT_PARITY_ERR, bk_lin_parity_error_func, NULL);
	bk_lin_register_isr(LIN_INT_COMPLETE, bk_lin_complete_func, NULL);
	if (lin_hal_get_master() == LIN_SLAVE) {
		bk_lin_register_isr(LIN_INT_DATA_REQ, bk_lin_data_req_func, (void *)&s_lin_buf);
		bk_lin_register_isr(LIN_INT_IDLE_TIMEOUT, bk_lin_idle_timeout_func, NULL);
		bk_lin_register_isr(LIN_INT_ABORTED, bk_lin_aborted_func, NULL);

	}
	bk_int_isr_register(INT_SRC_LIN, lin_isr, NULL);

	lin_hal_set_ctrl_reset_error();
	lin_hal_set_ctrl_reset_int();
	bk_lin_interrupt_enable();
	return BK_OK;
}

bk_err_t bk_lin_driver_deinit(void)
{
	if (!s_lin_driver_is_init) {
		return BK_OK;
	}
	bk_lin_interrupt_disable();
	bk_lin_gpio_deinit(LIN_CHAN_2);
	lin_hal_deinit();
	bk_int_isr_unregister(INT_SRC_LIN);
	bk_lin_clock_disable();
#if (CONFIG_LIN_PM_CB_SUPPORT)
	bk_pm_sleep_unregister_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_LIN, true, true);
	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_LIN, PM_POWER_MODULE_STATE_OFF);
#endif
	s_lin_driver_is_init = false;

	return BK_OK;
}
