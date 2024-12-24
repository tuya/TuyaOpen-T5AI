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

// Mailbox for CrossCore
#include <common/bk_include.h>
#include <string.h>
#include "mailbox_driver_base.h"
#include "mailbox_driver.h"
#include "icu_driver.h"

// for master core
static mailbox_info_t s_mailbox_master;
// for slave core
static mailbox_info_t s_mailbox_slave;

void crosscore_mb_rx_isr(mailbox_data_t *data);

// Receive MBOX @hal Data to @data
static void mailbox_receive_data(mailbox_hal_t *hal, mailbox_data_t *data, mailbox_box_num_t box)
{
	data->param0 = mailbox_hal_read_param0(hal, box);
	data->param1 = mailbox_hal_read_param1(hal, box);
	data->param2 = mailbox_hal_read_param2(hal, box);
	data->param3 = mailbox_hal_read_param3(hal, box);
	MAILBOX_LOGD("[MAILBOX_RECEIVE_PARAM]: (%08x, 0x%08x, %d, %d)\r\n", data->param0,
				 data->param1, data->param2, data->param3);
}

// Write @data to MBOX @hal
static void mailbox_write_data(mailbox_hal_t *hal, mailbox_data_t *data, mailbox_box_num_t box)
{
	mailbox_hal_write_param0(hal, data->param0, box);
	mailbox_hal_write_param1(hal, data->param1, box);
	mailbox_hal_write_param2(hal, data->param2, box);
	mailbox_hal_write_param3(hal, data->param3, box);
	MAILBOX_LOGD("[MAILBOX_SEND_PARAM]: (%08x, 0x%08x, %d, %d)\r\n", data->param0,
				 data->param1, data->param2, data->param3);
}

static void mailbox_rx_isr(mailbox_hal_t *hal)
{
	mailbox_box_num_t box;
	mailbox_data_t data;

	MAILBOX_LOGD("%s hw %p\n", __func__, hal->hw);

	box = mailbox_hal_check_which_box_trigger(hal);
	if (box == MAILBOX_NONE) {
		MAILBOX_LOGD("%s BOX ERROR\n", __func__);
		return;
	}

	// Receive Data
	mailbox_receive_data(hal, &data, box);

	// Handle MBOX Data
	crosscore_mb_rx_isr(&data);

	// Clear Int, Sender MBOX will clear ready bit
	mailbox_hal_box_clear(hal, box);

	MAILBOX_LOGD("[RECIVE_R]: (%08x, 0x%08x, %d, %d)\r\n", data.param0,
				 data.param1, data.param2, data.param3);

	MAILBOX_LOGD("%s OUT\n\n", __func__);
}

static bk_err_t __bk_mailbox_send(mailbox_hal_t *hal, mailbox_data_t *data, mailbox_endpoint_t src, mailbox_endpoint_t dst)
{
	mailbox_box_num_t box;
	int wait_count = 0;

	MAILBOX_LOGD("%s\n", __func__);
	MAILBOX_LOGD("mailbox %p send to %d\n", hal->hw, dst);

	while (wait_count < MAILBOX_SEND_WAIT_COUNT) {
		box = mailbox_hal_check_which_box_ready(hal);

		if (box != MAILBOX_NONE)
			break;
		wait_count++;
	}

	if (box == MAILBOX_NONE) {// mailbox busy
		MAILBOX_LOGD("No MailBox left for %d\n", dst);
		return BK_ERR_MAILBOX_TIMEOUT;
	}

	MAILBOX_LOGD("BOX: %x param0: %x, param1: %x, param2: %x, param3: %x\r\n",
				 box, data->param0, data->param1, data->param2, data->param3);

	// mailbox_hal_box_clear_ready(hal, box);
	mailbox_write_data(hal, data, box);
	mailbox_hal_box_trigger(hal, box);

	return BK_OK;
}

static void mailbox_mbox0_isr(void)
{
	mailbox_rx_isr(&s_mailbox_master.hal);
}

static void mailbox_mbox1_isr(void)
{
	mailbox_rx_isr(&s_mailbox_slave.hal);
}

bk_err_t bk_mailbox_cc_init(void)
{
	static bool mailbox_cc_inited = false;

	if (mailbox_cc_inited) {
		os_printf("%s already inited\n", __func__);
		return 0;
	}

	// Initialize MBOX0 for CPU0
	memset(&s_mailbox_master, 0, sizeof(s_mailbox_master));
	// set hw mailbox index
	s_mailbox_master.hal.id = MAILBOX0;
	// set hw mailbox reg base
	mailbox_hal_addr_init(&s_mailbox_master.hal);

	// Initialize MBOX
	mailbox_hal_box_init(&s_mailbox_master.hal);
	mailbox_hal_set_identity(&s_mailbox_master.hal);

	// Enable INT
	// mailbox_interrupt_enable(MAILBOX_CPU1);  // ENABLE INT CORRENT CPU CORE

	// Initialize MBOX1 for CPU1
	memset(&s_mailbox_slave, 0, sizeof(s_mailbox_slave));
	// set hw mailbox index
	s_mailbox_slave.hal.id = MAILBOX1;
	// set hw mailbox reg base
	mailbox_hal_addr_init(&s_mailbox_slave.hal);

	// Initialize MBOX
	mailbox_hal_box_init(&s_mailbox_slave.hal);
	mailbox_hal_set_identity(&s_mailbox_slave.hal);

	// Register ISR to receive MBOX0/1's message
	bk_int_isr_register(INT_SRC_MAILBOX0, mailbox_mbox0_isr, NULL);
	bk_int_isr_register(INT_SRC_MAILBOX1, mailbox_mbox1_isr, NULL);

	mailbox_cc_inited = true;

	return BK_OK;
}

bk_err_t bk_mailbox_master_send(mailbox_data_t *data, mailbox_endpoint_t src, mailbox_endpoint_t dst)
{
	return __bk_mailbox_send(&s_mailbox_master.hal, data, src, dst);
}

bk_err_t bk_mailbox_slave_send(mailbox_data_t *data, mailbox_endpoint_t src, mailbox_endpoint_t dst)
{
	return __bk_mailbox_send(&s_mailbox_slave.hal, data, src, dst);
}

void bk_mailbox_enable_int(mailbox_endpoint_t mbox)
{
	mailbox_interrupt_enable(mbox);
}

void bk_core_mbox_enable_int(int core_id, mailbox_endpoint_t mbox)
{
#if CONFIG_SOC_BK7236_SMP_TEMP || CONFIG_SOC_BK7239_SMP_TEMP || CONFIG_SOC_BK7286_SMP_TEMP 
  core_mbox_interrupt_enable(core_id, mbox);
#endif
}
// eof
