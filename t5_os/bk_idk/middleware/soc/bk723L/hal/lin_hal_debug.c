// Copyright 2022-2023 Beken
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

// This is a generated file, if you need to modify it, use the script to
// generate and modify all the struct.h, ll.h, reg.h, debug_dump.c files!

#include "hal_config.h"
#include "lin_hw.h"
#include "lin_hal.h"

#if CFG_HAL_DEBUG_LIN
typedef void (*lin_dump_fn_t)(void);
typedef struct {
	uint32_t start;
	uint32_t end;
	lin_dump_fn_t fn;
} lin_reg_fn_map_t;

static void lin_dump_deviceid(void)
{
	SOC_LOGI("deviceid: %8x\r\n", REG_READ(SOC_LIN_REG_BASE + (0x0 << 2)));
}

static void lin_dump_versionid(void)
{
	SOC_LOGI("versionid: %8x\r\n", REG_READ(SOC_LIN_REG_BASE + (0x1 << 2)));
}

static void lin_dump_global_ctrl(void)
{
	lin_global_ctrl_t *r = (lin_global_ctrl_t *)(SOC_LIN_REG_BASE + (0x2 << 2));

	SOC_LOGI("global_ctrl: %8x\r\n", REG_READ(SOC_LIN_REG_BASE + (0x2 << 2)));
	SOC_LOGI("	soft_rst: %8x\r\n", r->soft_rst);
	SOC_LOGI("	bypass_cfg: %8x\r\n", r->bypass_cfg);
	SOC_LOGI("	lin_master: %8x\r\n", r->lin_master);
	SOC_LOGI("	reserved_bit_3_31: %8x\r\n", r->reserved_bit_3_31);
}

static void lin_dump_global_status(void)
{
	SOC_LOGI("global_status: %8x\r\n", REG_READ(SOC_LIN_REG_BASE + (0x3 << 2)));
}

static void lin_dump_rsv_4_1f(void)
{
	for (uint32_t idx = 0; idx < 28; idx++) {
		SOC_LOGI("rsv_4_1f: %8x\r\n", REG_READ(SOC_LIN_REG_BASE + ((0x4 + idx) << 2)));
	}
}

static void lin_dump_data(void)
{
	lin_data_t *r;
	int i;
	for (i = 0; i < 8; i++) {
 		r = (lin_data_t *)(SOC_LIN_REG_BASE + ((0x20 + i) << 2));

		SOC_LOGI("data0: %8x\r\n", REG_READ(SOC_LIN_REG_BASE + (0x20 << 2)));
		SOC_LOGI("	byte: %8x\r\n", r->byte);
		SOC_LOGI("	reserved_bit_8_31: %8x\r\n", r->reserved_bit_8_31);
	}
}

static void lin_dump_ctrl(void)
{
	lin_ctrl_t *r = (lin_ctrl_t *)(SOC_LIN_REG_BASE + (0x28 << 2));

	SOC_LOGI("ctrl: %8x\r\n", REG_READ(SOC_LIN_REG_BASE + (0x28 << 2)));
	SOC_LOGI("	start_req: %8x\r\n", r->start_req);
	SOC_LOGI("	wakeup: %8x\r\n", r->wakeup);
	SOC_LOGI("	reset_error: %8x\r\n", r->reset_error);
	SOC_LOGI("	reset_int: %8x\r\n", r->reset_int);
	SOC_LOGI("	data_ack: %8x\r\n", r->data_ack);
	SOC_LOGI("	transmit: %8x\r\n", r->transmit);
	SOC_LOGI("	sleep: %8x\r\n", r->sleep);
	SOC_LOGI("	stop: %8x\r\n", r->stop);
	SOC_LOGI("	reserved_8_31: %8x\r\n", r->reserved_8_31);
}

static void lin_dump_status(void)
{
	lin_status_t *r = (lin_status_t *)(SOC_LIN_REG_BASE + (0x29 << 2));

	SOC_LOGI("status: %8x\r\n", REG_READ(SOC_LIN_REG_BASE + (0x29 << 2)));
	SOC_LOGI("	complete: %8x\r\n", r->complete);
	SOC_LOGI("	wake_up: %8x\r\n", r->wake_up);
	SOC_LOGI("	error: %8x\r\n", r->error);
	SOC_LOGI("	int: %8x\r\n", r->intr);
	SOC_LOGI("	data_req: %8x\r\n", r->data_req);
	SOC_LOGI("	aborted: %8x\r\n", r->aborted);
	SOC_LOGI("	bus_idle_timeout: %8x\r\n", r->bus_idle_timeout);
	SOC_LOGI("	lin_active: %8x\r\n", r->lin_active);
	SOC_LOGI("	reserved_8_31: %8x\r\n", r->reserved_8_31);
}

static void lin_dump_err(void)
{
	lin_err_t *r = (lin_err_t *)(SOC_LIN_REG_BASE + (0x2a << 2));

	SOC_LOGI("err: %8x\r\n", REG_READ(SOC_LIN_REG_BASE + (0x2a << 2)));
	SOC_LOGI("	bit: %8x\r\n", r->bit);
	SOC_LOGI("	chk: %8x\r\n", r->chk);
	SOC_LOGI("	timeout: %8x\r\n", r->timeout);
	SOC_LOGI("	parity: %8x\r\n", r->parity);
	SOC_LOGI("	reserved_4_31: %8x\r\n", r->reserved_4_31);
}

static void lin_dump_type(void)
{
	lin_type_t *r = (lin_type_t *)(SOC_LIN_REG_BASE + (0x2b << 2));

	SOC_LOGI("type: %8x\r\n", REG_READ(SOC_LIN_REG_BASE + (0x2b << 2)));
	SOC_LOGI("	data_length: %8x\r\n", r->data_length);
	SOC_LOGI("	reserved_4_6: %8x\r\n", r->reserved_4_6);
	SOC_LOGI("	enh_check: %8x\r\n", r->enh_check);
	SOC_LOGI("	reserved_8_31: %8x\r\n", r->reserved_8_31);
}

static void lin_dump_btcfg0(void)
{
	lin_btcfg0_t *r = (lin_btcfg0_t *)(SOC_LIN_REG_BASE + (0x2c << 2));

	SOC_LOGI("btcfg0: %8x\r\n", REG_READ(SOC_LIN_REG_BASE + (0x2c << 2)));
	SOC_LOGI("	bt_div1: %8x\r\n", r->bt_div1);
	SOC_LOGI("	reserved_bit_8_31: %8x\r\n", r->reserved_bit_8_31);
}

static void lin_dump_btcfg1(void)
{
	lin_btcfg1_t *r = (lin_btcfg1_t *)(SOC_LIN_REG_BASE + (0x2d << 2));

	SOC_LOGI("btcfg1: %8x\r\n", REG_READ(SOC_LIN_REG_BASE + (0x2d << 2)));
	SOC_LOGI("	bt_div2: %8x\r\n", r->bt_div2);
	SOC_LOGI("	bt_mul: %8x\r\n", r->bt_mul);
	SOC_LOGI("	prescl1: %8x\r\n", r->prescl1);
	SOC_LOGI("	reserved_8_31: %8x\r\n", r->reserved_8_31);
}

static void lin_dump_ident(void)
{
	lin_ident_t *r = (lin_ident_t *)(SOC_LIN_REG_BASE + (0x2e << 2));

	SOC_LOGI("ident: %8x\r\n", REG_READ(SOC_LIN_REG_BASE + (0x2e << 2)));
	SOC_LOGI("	id: %8x\r\n", r->id);
	SOC_LOGI("	reserved_6_31: %8x\r\n", r->reserved_6_31);
}

static void lin_dump_tcfg(void)
{
	lin_tcfg_t *r = (lin_tcfg_t *)(SOC_LIN_REG_BASE + (0x2f << 2));

	SOC_LOGI("tcfg: %8x\r\n", REG_READ(SOC_LIN_REG_BASE + (0x2f << 2)));
	SOC_LOGI("	wup_repeat_time: %8x\r\n", r->wup_repeat_time);
	SOC_LOGI("	bus_inactivity_time: %8x\r\n", r->bus_inactivity_time);
	SOC_LOGI("	reserved_4_5: %8x\r\n", r->reserved_4_5);
	SOC_LOGI("	prescl2: %8x\r\n", r->prescl2);
	SOC_LOGI("	reserved_7_31: %8x\r\n", r->reserved_7_31);
}

static lin_reg_fn_map_t s_lin_fn[] =
{
	{0x0, 0x0, lin_dump_deviceid},
	{0x1, 0x1, lin_dump_versionid},
	{0x2, 0x2, lin_dump_global_ctrl},
	{0x3, 0x3, lin_dump_global_status},
	{0x4, 0x20, lin_dump_rsv_4_1f},
	{0x20, 0x27, lin_dump_data},
	{0x28, 0x28, lin_dump_ctrl},
	{0x29, 0x29, lin_dump_status},
	{0x2a, 0x2a, lin_dump_err},
	{0x2b, 0x2b, lin_dump_type},
	{0x2c, 0x2c, lin_dump_btcfg0},
	{0x2d, 0x2d, lin_dump_btcfg1},
	{0x2e, 0x2e, lin_dump_ident},
	{0x2f, 0x2f, lin_dump_tcfg},
	{-1, -1, 0}
};

void lin_struct_dump(uint32_t start, uint32_t end)
{
	uint32_t dump_fn_cnt = sizeof(s_lin_fn)/sizeof(s_lin_fn[0]) - 1;

	for (uint32_t idx = 0; idx < dump_fn_cnt; idx++) {
		if ((start <= s_lin_fn[idx].start) && (end >= s_lin_fn[idx].end)) {
			s_lin_fn[idx].fn();
		}
	}
}
#endif