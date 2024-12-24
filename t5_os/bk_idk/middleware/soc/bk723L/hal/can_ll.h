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

#pragma once

#include <soc/soc.h>
#include "hal_port.h"
#include "can_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CAN_LL_REG_BASE   SOC_CAN_REG_BASE
#define CAN_GPIO_MAP \
{ \
	{{GPIO_44, GPIO_DEV_CAN_TX}, {GPIO_45, GPIO_DEV_CAN_RX}, {GPIO_46, GPIO_DEV_CAN_STANDBY}}, \
}

#define CAN_BR_MAP \
{ \
    {CAN_BR_250K, 0x0F02020b}, \
    {CAN_BR_500K, 0x0702020b}, \
    {CAN_BR_800K, 0x0303030e}, \
    {CAN_BR_1M, 0x01050518}, \
    {CAN_BR_2M, 0x00050518}, \
    {CAN_BR_4M, 0x0002020b}, \
    {CAN_BR_5M, 0x00020208}, \
}

//reg :rid

static inline uint32_t can_ll_get_rid_esi_value(void) {
	can_id_esi_t *r = (can_id_esi_t*)(SOC_CAN_REG_BASE + (0x0 << 2));
	return r->v;
}

static inline uint32_t can_ll_get_rid(void) {
	can_id_esi_t *r = (can_id_esi_t*)(SOC_CAN_REG_BASE + (0x0 << 2));
	return r->id;
}


static inline uint32_t can_ll_get_rid_esi(void) {
	can_id_esi_t *r = (can_id_esi_t*)(SOC_CAN_REG_BASE + (0x0 << 2));
	return r->esi;
}

//reg :rbuf_ctrl


static inline uint32_t can_ll_get_rbuf_ctrl_value(void) {
	can_buf_ctrl_t *r = (can_buf_ctrl_t*)(SOC_CAN_REG_BASE + (0x1 << 2));
	return r->v;
}

static inline uint32_t can_ll_get_rbuf_ctrl_dlc(void) {
	can_buf_ctrl_t *r = (can_buf_ctrl_t*)(SOC_CAN_REG_BASE + (0x1 << 2));
	return r->dlc;
}

static inline uint32_t can_ll_get_rbuf_ctrl_brs(void) {
	can_buf_ctrl_t *r = (can_buf_ctrl_t*)(SOC_CAN_REG_BASE + (0x1 << 2));
	return r->brs;
}

static inline uint32_t can_ll_get_rbuf_ctrl_fdf(void) {
	can_buf_ctrl_t *r = (can_buf_ctrl_t*)(SOC_CAN_REG_BASE + (0x1 << 2));
	return r->fdf;
}

static inline uint32_t can_ll_get_rbuf_ctrl_rtr(void) {
	can_buf_ctrl_t *r = (can_buf_ctrl_t*)(SOC_CAN_REG_BASE + (0x1 << 2));
	return r->rtr;
}

static inline uint32_t can_ll_get_rbuf_ctrl_ide(void) {
	can_buf_ctrl_t *r = (can_buf_ctrl_t*)(SOC_CAN_REG_BASE + (0x1 << 2));
	return r->ide;
}

//reg :rdata

static inline uint32_t can_ll_get_rdata_value(uint32_t i) {
	can_data_t *r = (can_data_t*)(SOC_CAN_REG_BASE + ((0x2 + i) << 2));
	return r->v;
}

//reg :tid

static inline void can_ll_set_tid_esi_value(uint32_t v) {
	can_id_esi_t *r = (can_id_esi_t*)(SOC_CAN_REG_BASE + (0x14<< 2));
	r->id = v;
}

static inline uint32_t can_ll_get_tid_esi_value(void) {
	can_id_esi_t *r = (can_id_esi_t*)(SOC_CAN_REG_BASE + (0x14<< 2));
	return r->v;
}

//reg :tid_esi

static inline void can_ll_set_tid(uint32_t v) {
	can_id_esi_t *r = (can_id_esi_t*)(SOC_CAN_REG_BASE + (0x14 << 2));
	r->id = v;
}

static inline uint32_t can_ll_get_tid(void) {
	can_id_esi_t *r = (can_id_esi_t*)(SOC_CAN_REG_BASE + (0x14 << 2));
	return r->id;
}

static inline void can_ll_set_tid_esi(uint32_t v) {
	can_id_esi_t *r = (can_id_esi_t*)(SOC_CAN_REG_BASE + (0x14 << 2));
	r->esi = v;
}

static inline uint32_t can_ll_get_tid_esi(void) {
	can_id_esi_t *r = (can_id_esi_t*)(SOC_CAN_REG_BASE + (0x14 << 2));
	return r->esi;
}

//reg :tbuf_ctrl

static inline void can_ll_set_tbuf_ctrl_value(uint32_t v) {
	can_buf_ctrl_t *r = (can_buf_ctrl_t*)(SOC_CAN_REG_BASE + (0x15 << 2));
	r->v = v;
}

static inline uint32_t can_ll_get_tbuf_ctrl_value(void) {
	can_buf_ctrl_t *r = (can_buf_ctrl_t*)(SOC_CAN_REG_BASE + (0x15 << 2));
	return r->v;
}

static inline void can_ll_set_tbuf_ctrl_dlc(uint32_t v) {
	can_buf_ctrl_t *r = (can_buf_ctrl_t*)(SOC_CAN_REG_BASE + (0x15 << 2));
	r->dlc = v;
}

static inline uint32_t can_ll_get_tbuf_ctrl_dlc(void) {
	can_buf_ctrl_t *r = (can_buf_ctrl_t*)(SOC_CAN_REG_BASE + (0x15 << 2));
	return r->dlc;
}

static inline void can_ll_set_tbuf_ctrl_brs(uint32_t v) {
	can_buf_ctrl_t *r = (can_buf_ctrl_t*)(SOC_CAN_REG_BASE + (0x15 << 2));
	r->brs = v;
}

static inline uint32_t can_ll_get_tbuf_ctrl_brs(void) {
	can_buf_ctrl_t *r = (can_buf_ctrl_t*)(SOC_CAN_REG_BASE + (0x15 << 2));
	return r->brs;
}

static inline void can_ll_set_tbuf_ctrl_fdf(uint32_t v) {
	can_buf_ctrl_t *r = (can_buf_ctrl_t*)(SOC_CAN_REG_BASE + (0x15 << 2));
	r->fdf = v;
}

static inline uint32_t can_ll_get_tbuf_ctrl_fdf(void) {
	can_buf_ctrl_t *r = (can_buf_ctrl_t*)(SOC_CAN_REG_BASE + (0x15 << 2));
	return r->fdf;
}

static inline void can_ll_set_tbuf_ctrl_rtr(uint32_t v) {
	can_buf_ctrl_t *r = (can_buf_ctrl_t*)(SOC_CAN_REG_BASE + (0x15 << 2));
	r->rtr = v;
}

static inline uint32_t can_ll_get_tbuf_ctrl_rtr(void) {
	can_buf_ctrl_t *r = (can_buf_ctrl_t*)(SOC_CAN_REG_BASE + (0x15 << 2));
	return r->rtr;
}

static inline void can_ll_set_tbuf_ctrl_ide(uint32_t v) {
	can_buf_ctrl_t *r = (can_buf_ctrl_t*)(SOC_CAN_REG_BASE + (0x15 << 2));
	r->ide = v;
}

static inline uint32_t can_ll_get_tbuf_ctrl_ide(void) {
	can_buf_ctrl_t *r = (can_buf_ctrl_t*)(SOC_CAN_REG_BASE + (0x15 << 2));
	return r->ide;
}

//reg :tdata

static inline void can_ll_set_tdata(uint32_t i, uint32_t v) {
	can_data_t *r = (can_data_t*)(SOC_CAN_REG_BASE + ((0x16 + i) << 2));
	r->data = v;
}


static inline uint32_t can_ll_get_tdata(uint32_t i) {
	can_data_t *r = (can_data_t*)(SOC_CAN_REG_BASE + ((0x16 + i) << 2));
	return r->data;
}

//reg :tts

static inline uint32_t can_ll_get_tts_l_value(void) {
	can_tts_t *r = (can_tts_t*)(SOC_CAN_REG_BASE + (0x26 << 2));
	return r->v;
}

static inline uint32_t can_ll_get_tts_h_value(void) {
	can_tts_t *r = (can_tts_t*)(SOC_CAN_REG_BASE + (0x27 << 2));
	return r->v;
}

//reg :cfg_stat

static inline void can_ll_set_cfg_value(uint32_t v) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	r->v = v;
}

static inline uint32_t can_ll_get_cfg_value(void) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	return r->v;
}

static inline void can_ll_set_busoff(uint32_t v) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	r->busoff  = v;
}

static inline uint32_t can_ll_get_busoff(void) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	return r->busoff ;
}

static inline uint32_t can_ll_get_tactive(void) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	return r->tactive ;
}

static inline uint32_t can_ll_get_ractive(void) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	return r->ractive ;
}

static inline void can_ll_set_tsss(uint32_t v) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	r->tsss  = v;
}

static inline uint32_t can_ll_get_tsss(void) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	return r->tsss ;
}

static inline void can_ll_set_tpss(uint32_t v) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	r->tpss  = v;
}

static inline uint32_t can_ll_get_tpss(void) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	return r->tpss ;
}

static inline void can_ll_set_lbmi(uint32_t v) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	r->lbmi = v;
}

static inline uint32_t can_ll_get_lbmi(void) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	return r->lbmi;
}

static inline void can_ll_set_lbme(uint32_t v) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	r->lbme = v;
}

static inline uint32_t can_ll_get_lbme(void) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	return r->lbme;
}

static inline void can_ll_set_reset(uint32_t v) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	r->reset = v;
}

static inline uint32_t can_ll_get_reset(void) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	return r->reset;
}

//reg :tcmd

static inline void can_ll_set_tsa(uint32_t v) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	r->tsa  = v;
}

static inline uint32_t can_ll_get_tsa(void) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	return r->tsa ;
}

static inline void can_ll_set_tsall(uint32_t v) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	r->tsall  = v;
}

static inline uint32_t can_ll_get_tsall(void) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	return r->tsall ;
}

static inline void can_ll_set_tsone(uint32_t v) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	r->tsone  = v;
}

static inline uint32_t can_ll_get_tsone(void) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	return r->tsone ;
}

static inline void can_ll_set_tpa(uint32_t v) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	r->tpa  = v;
}

static inline uint32_t can_ll_get_tpa(void) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	return r->tpa ;
}

static inline void can_ll_set_tpe(uint32_t v) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	r->tpe  = v;
}

static inline uint32_t can_ll_get_tpe(void) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	return r->tpe ;
}

static inline void can_ll_set_stby(uint32_t v) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	r->stby  = v;
}

static inline uint32_t can_ll_get_stby(void) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	return r->stby ;
}

static inline void can_ll_set_lom(uint32_t v) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	r->lom  = v;
}

static inline uint32_t can_ll_get_lom(void) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	return r->lom ;
}

static inline void can_ll_set_tbsel(uint32_t v) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	r->tbsel  = v;
}

static inline uint32_t can_ll_get_tbsel(void) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	return r->tbsel ;
}

//reg :tctrl

static inline uint32_t can_ll_get_tsstat(void) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	return r->tsstat ;
}

static inline void can_ll_set_tttbm(uint32_t v) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	r->tttbm  = v;
}

static inline uint32_t can_ll_get_tttbm(void) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	return r->tttbm ;
}

static inline void can_ll_set_tsmode(uint32_t v) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	r->tsmode  = v;
}

static inline uint32_t can_ll_get_tsmode(void) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	return r->tsmode ;
}

static inline void can_ll_set_tsnext(uint32_t v) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	r->tsnext  = v;
}

static inline uint32_t can_ll_get_tsnext(void) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	return r->tsnext ;
}

static inline void can_ll_set_fd_iso(uint32_t v) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	r->fd_iso  = v;
}

static inline uint32_t can_ll_get_fd_iso(void) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	return r->fd_iso ;
}

//reg :rctrl

static inline uint32_t can_ll_get_rstat(void) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	return r->rstat ;
}

static inline void can_ll_set_rball(uint32_t v) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	r->rball  = v;
}

static inline uint32_t can_ll_get_rball(void) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	return r->rball ;
}

static inline void can_ll_set_rrel(uint32_t v) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	r->rrel  = v;
}

static inline uint32_t can_ll_get_rrel(void) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	return r->rrel ;
}

static inline uint32_t can_ll_get_rov(void) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	return r->rov ;
}

static inline void can_ll_set_rom(uint32_t v) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	r->rom  = v;
}

static inline uint32_t can_ll_get_rom(void) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	return r->rom ;
}

static inline void can_ll_set_sack(uint32_t v) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	r->sack  = v;
}

static inline uint32_t can_ll_get_sack(void) {
	can_cfg_t *r = (can_cfg_t*)(SOC_CAN_REG_BASE + (0x28 << 2));
	return r->sack ;
}

//reg :trie

static inline void can_ll_set_ie_value(uint32_t v) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	r->v = v;
}

static inline uint32_t can_ll_get_ie_value(void) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	return r->v;
}

static inline uint32_t can_ll_get_tsff(void) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	return r->tsff ;
}

static inline void can_ll_set_eie(uint32_t v) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	r->eie  = v;
}

static inline uint32_t can_ll_get_eie(void) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	return r->eie ;
}

static inline void can_ll_set_tsie(uint32_t v) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	r->tsie  = v;
}

static inline uint32_t can_ll_get_tsie(void) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	return r->tsie ;
}

static inline void can_ll_set_tpie(uint32_t v) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	r->tpie  = v;
}

static inline uint32_t can_ll_get_tpie(void) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	return r->tpie ;
}

static inline void can_ll_set_rafie(uint32_t v) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	r->rafie  = v;
}

static inline uint32_t can_ll_get_rafie(void) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	return r->rafie ;
}

static inline void can_ll_set_rfie(uint32_t v) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	r->rfie  = v;
}

static inline uint32_t can_ll_get_rfie(void) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	return r->rfie ;
}

static inline void can_ll_set_roie(uint32_t v) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	r->roie  = v;
}

static inline uint32_t can_ll_get_roie(void) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	return r->roie ;
}

static inline void can_ll_set_rie(uint32_t v) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	r->rie  = v;
}

static inline uint32_t can_ll_get_rie(void) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	return r->rie ;
}

//reg :trif

static inline void can_ll_set_aif(uint32_t v) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	r->aif  = v;
}

static inline uint32_t can_ll_get_aif(void) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	return r->aif ;
}

static inline void can_ll_set_eif(uint32_t v) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	r->eif  = v;
}

static inline uint32_t can_ll_get_eif(void) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	return r->eif ;
}

static inline void can_ll_set_tsif(uint32_t v) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	r->tsif  = v;
}

static inline uint32_t can_ll_get_tsif(void) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	return r->tsif ;
}

static inline void can_ll_set_tpif(uint32_t v) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	r->tpif  = v;
}

static inline uint32_t can_ll_get_tpif(void) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	return r->tpif ;
}

static inline void can_ll_set_rafif(uint32_t v) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	r->rafif  = v;
}

static inline uint32_t can_ll_get_rafif(void) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	return r->rafif ;
}

static inline void can_ll_set_rfif(uint32_t v) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	r->rfif  = v;
}

static inline uint32_t can_ll_get_rfif(void) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	return r->rfif ;
}

static inline void can_ll_set_roif(uint32_t v) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	r->roif  = v;
}

static inline uint32_t can_ll_get_roif(void) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	return r->roif ;
}

static inline void can_ll_set_rif(uint32_t v) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	r->rif  = v;
}

static inline uint32_t can_ll_get_rif(void) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	return r->rif ;
}

//reg :errint

static inline void can_ll_set_beif(uint32_t v) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	r->beif  = v;
}

static inline uint32_t can_ll_beif(void) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	return r->beif ;
}

static inline void can_ll_set_beie(uint32_t v) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	r->beie  = v;
}

static inline uint32_t can_ll_get_beie(void) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	return r->beie ;
}

static inline void can_ll_set_alif(uint32_t v) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	r->alif  = v;
}

static inline uint32_t can_ll_get_alif(void) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	return r->alif ;
}

static inline void can_ll_set_alie(uint32_t v) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	r->alie  = v;
}

static inline uint32_t can_ll_get_alie(void) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	return r->alie ;
}

static inline void can_ll_set_epif(uint32_t v) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	r->epif  = v;
}

static inline uint32_t can_ll_get_epif(void) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	return r->epif ;
}

static inline void can_ll_set_epie(uint32_t v) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	r->epie  = v;
}

static inline uint32_t can_ll_get_epie(void) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	return r->epie ;
}

static inline uint32_t can_ll_get_epass(void) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	return r->epass ;
}

static inline uint32_t can_ll_get_ewarn(void) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	return r->ewarn ;
}

//reg :limit

static inline void can_ll_set_ewl(uint32_t v) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	r->ewl = v;
}

static inline uint32_t can_ll_get_ewl(void) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	return r->ewl;
}

static inline void can_ll_set_afwl(uint32_t v) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	r->afwl = v;
}

static inline uint32_t can_ll_get_afwl(void) {
	can_ie_t *r = (can_ie_t*)(SOC_CAN_REG_BASE + (0x29 << 2));
	return r->afwl;
}

//reg :s_seg_1

static inline void can_ll_set_sseg_value(uint32_t v) {
	can_sseg_t *r = (can_sseg_t*)(SOC_CAN_REG_BASE + (0x2a<< 2));
	r->v = v;
}

static inline uint32_t can_ll_get_sseg_value(void) {
	can_sseg_t *r = (can_sseg_t*)(SOC_CAN_REG_BASE + (0x2a<< 2));
	return r->v;
}

static inline void can_ll_set_sseg1(uint32_t v) {
	can_sseg_t *r = (can_sseg_t*)(SOC_CAN_REG_BASE + (0x2a<< 2));
	r->s_seg_1 = v;
}

static inline uint32_t can_ll_get_sseg1(void) {
	can_sseg_t *r = (can_sseg_t*)(SOC_CAN_REG_BASE + (0x2a<< 2));
	return r->s_seg_1;
}

//reg :s_seg_2

static inline void can_ll_set_sseg2(uint32_t v) {
	can_sseg_t *r = (can_sseg_t*)(SOC_CAN_REG_BASE + (0x2a << 2));
	r->s_seg_2 = v;
}

static inline uint32_t can_ll_get_sseg2(void) {
	can_sseg_t *r = (can_sseg_t*)(SOC_CAN_REG_BASE + (0x2a << 2));
	return r->s_seg_2;
}

//reg :ssjw

static inline void can_ll_set_ssjw(uint32_t v) {
	can_sseg_t *r = (can_sseg_t*)(SOC_CAN_REG_BASE + (0x2a << 2));
	r->s_sjw = v;
}

static inline uint32_t can_ll_get_ssjw(void) {
	can_sseg_t *r = (can_sseg_t*)(SOC_CAN_REG_BASE + (0x2a << 2));
	return r->s_sjw;
}

//reg :spresc

static inline void can_ll_set_spresc(uint32_t v) {
	can_sseg_t *r = (can_sseg_t*)(SOC_CAN_REG_BASE + (0x2a << 2));
	r->s_presc = v;
}

static inline uint32_t can_ll_get_spresc(void) {
	can_sseg_t *r = (can_sseg_t*)(SOC_CAN_REG_BASE + (0x2a << 2));
	return r->s_presc;
}

//reg :f_seg_2

static inline void can_ll_set_fseg_value(uint32_t v) {
	can_fseg_t *r = (can_fseg_t*)(SOC_CAN_REG_BASE + (0x2b << 2));
	r->v = v;
}

static inline uint32_t can_ll_get_fseg_value(void) {
	can_fseg_t *r = (can_fseg_t*)(SOC_CAN_REG_BASE + (0x2b << 2));
	return r->v;
}

static inline void can_ll_set_fseg1(uint32_t v) {
	can_fseg_t *r = (can_fseg_t*)(SOC_CAN_REG_BASE + (0x2b << 2));
	r->f_seg_1 = v;
}

static inline uint32_t can_ll_get_fseg1(void) {
	can_fseg_t *r = (can_fseg_t*)(SOC_CAN_REG_BASE + (0x2b << 2));
	return r->f_seg_1;
}

//reg :f_seg_2

static inline void can_ll_set_fseg2(uint32_t v) {
	can_fseg_t *r = (can_fseg_t*)(SOC_CAN_REG_BASE + (0x2b << 2));
	r->f_seg_2 = v;
}

static inline uint32_t can_ll_get_fseg2(void) {
	can_fseg_t *r = (can_fseg_t*)(SOC_CAN_REG_BASE + (0x2b << 2));
	return r->f_seg_2;
}

//reg :f_sjw

static inline void can_ll_set_fsjw(uint32_t v) {
	can_fseg_t *r = (can_fseg_t*)(SOC_CAN_REG_BASE + (0x2b << 2));
	r->f_sjw = v;
}

static inline uint32_t can_ll_get_fsjw(void) {
	can_fseg_t *r = (can_fseg_t*)(SOC_CAN_REG_BASE + (0x2b << 2));
	return r->f_sjw;
}

//reg :f_presc


static inline void can_ll_set_fpresc(uint32_t v) {
	can_fseg_t *r = (can_fseg_t*)(SOC_CAN_REG_BASE + (0x2b << 2));
	r->f_presc = v;
}

static inline uint32_t can_ll_get_fpresc(void) {
	can_fseg_t *r = (can_fseg_t*)(SOC_CAN_REG_BASE + (0x2b << 2));
	return r->f_presc;
}

//reg :ealcap

static inline void can_ll_set_cap_value(uint32_t v) {
	can_cap_t *r = (can_cap_t*)(SOC_CAN_REG_BASE + (0x2c << 2));
	r->v = v;
}

static inline uint32_t can_ll_get_cap_value(void) {
	can_cap_t *r = (can_cap_t*)(SOC_CAN_REG_BASE + (0x2c << 2));
	return r->v;
}

static inline uint32_t can_ll_get_alc(void) {
	can_cap_t *r = (can_cap_t*)(SOC_CAN_REG_BASE + (0x2c << 2));
	return r->alc;
}

static inline uint32_t can_ll_get_koer(void) {
	can_cap_t *r = (can_cap_t*)(SOC_CAN_REG_BASE + (0x2c << 2));
	return r->koer;
}

//reg :tdc

static inline void can_ll_set_sspoff(uint32_t v) {
	can_cap_t *r = (can_cap_t*)(SOC_CAN_REG_BASE + (0x2c << 2));
	r->sspoff = v;
}

static inline uint32_t can_ll_get_sspoff(void) {
	can_cap_t *r = (can_cap_t*)(SOC_CAN_REG_BASE + (0x2c << 2));
	return r->sspoff;
}

static inline void can_ll_set_tdcen(uint32_t v) {
	can_cap_t *r = (can_cap_t*)(SOC_CAN_REG_BASE + (0x2c << 2));
	r->tdcen = v;
}

static inline uint32_t can_ll_get_tdcen(void) {
	can_cap_t *r = (can_cap_t*)(SOC_CAN_REG_BASE + (0x2c << 2));
	return r->tdcen;
}

//reg :recnt

static inline uint32_t can_ll_get_recnt(void) {
	can_cap_t *r = (can_cap_t*)(SOC_CAN_REG_BASE + (0x2c << 2));
	return r->recnt;
}

//reg :

static inline uint32_t can_ll_get_tecnt(void) {
	can_cap_t *r = (can_cap_t*)(SOC_CAN_REG_BASE + (0x2c << 2));
	return r->tecnt;
}

//reg :acfctrl

static inline void can_ll_set_acf_value(uint32_t v) {
	can_acf_t *r = (can_acf_t*)(SOC_CAN_REG_BASE + (0x2d << 2));
	r->v = v;
}

static inline uint32_t can_ll_get_acf_value(void) {
	can_acf_t *r = (can_acf_t*)(SOC_CAN_REG_BASE + (0x2d << 2));
	return r->v;
}

static inline void can_ll_set_acfadr(uint32_t v) {
	can_acf_t *r = (can_acf_t*)(SOC_CAN_REG_BASE + (0x2d << 2));
	r->acfadr  = v;
}

static inline uint32_t can_ll_get_acfadr(void) {
	can_acf_t *r = (can_acf_t*)(SOC_CAN_REG_BASE + (0x2d << 2));
	return r->acfadr ;
}

static inline void can_ll_set_selmask(uint32_t v) {
	can_acf_t *r = (can_acf_t*)(SOC_CAN_REG_BASE + (0x2d << 2));
	r->selmask  = v;
}

static inline uint32_t can_ll_get_selmask(void) {
	can_acf_t *r = (can_acf_t*)(SOC_CAN_REG_BASE + (0x2d << 2));
	return r->selmask ;
}

//reg :timecfg

static inline void can_ll_set_timeen(uint32_t v) {
	can_acf_t *r = (can_acf_t*)(SOC_CAN_REG_BASE + (0x2d << 2));
	r->timeen = v;
}

static inline uint32_t can_ll_get_timeen(void) {
	can_acf_t *r = (can_acf_t*)(SOC_CAN_REG_BASE + (0x2d << 2));
	return r->timeen;
}

static inline void can_ll_set_timepos(uint32_t v) {
	can_acf_t *r = (can_acf_t*)(SOC_CAN_REG_BASE + (0x2d << 2));
	r->timepos = v;
}

static inline uint32_t can_ll_get_timepos(void) {
	can_acf_t *r = (can_acf_t*)(SOC_CAN_REG_BASE + (0x2d << 2));
	return r->timepos;
}

static inline void can_ll_set_acf_en(uint32_t v) {
	can_acf_t *r = (can_acf_t*)(SOC_CAN_REG_BASE + (0x2d << 2));
	r->acf_en = v;
}

static inline uint32_t can_ll_get_acf_en(void) {
	can_acf_t *r = (can_acf_t*)(SOC_CAN_REG_BASE + (0x2d << 2));
	return r->acf_en;
}

//reg :acode_x or amask_x 

static inline void can_ll_set_aid_value(uint32_t v) {
	can_aid_t *r = (can_aid_t*)(SOC_CAN_REG_BASE + (0x2e << 2));
	r->v = v;
}

static inline uint32_t can_ll_get_aid_value(void) {
	can_aid_t *r = (can_aid_t*)(SOC_CAN_REG_BASE + (0x2e << 2));
	return r->v;
}

static inline void can_ll_set_acode_or_amask(uint32_t v) {
	can_aid_t *r = (can_aid_t*)(SOC_CAN_REG_BASE + (0x2e << 2));
	r->acode_or_amask = v;
}

static inline uint32_t can_ll_get_acode_or_amask(void) {
	can_aid_t *r = (can_aid_t*)(SOC_CAN_REG_BASE + (0x2e << 2));
	return r->acode_or_amask;
}

static inline void can_ll_set_aide(uint32_t v) {
	can_aid_t *r = (can_aid_t*)(SOC_CAN_REG_BASE + (0x2e << 2));
	r->aide = v;
}

static inline uint32_t can_ll_get_aide(void) {
	can_aid_t *r = (can_aid_t*)(SOC_CAN_REG_BASE + (0x2e << 2));
	return r->aide;
}

static inline void can_ll_set_aidee(uint32_t v) {
	can_aid_t *r = (can_aid_t*)(SOC_CAN_REG_BASE + (0x2e << 2));
	r->aidee = v;
}

static inline uint32_t can_ll_get_aidee(void) {
	can_aid_t *r = (can_aid_t*)(SOC_CAN_REG_BASE + (0x2e << 2));
	return r->aidee;
}

//reg :ver_0

static inline void can_ll_set_ttcfg_value(uint32_t v) {
	can_ttcfg_t *r = (can_ttcfg_t*)(SOC_CAN_REG_BASE + (0x2f << 2));
	r->v = v;
}

static inline uint32_t can_ll_get_ttcfg_value(void) {
	can_ttcfg_t *r = (can_ttcfg_t*)(SOC_CAN_REG_BASE + (0x2f << 2));
	return r->v;
}

static inline uint32_t can_ll_get_ver_0(void) {
	can_ttcfg_t *r = (can_ttcfg_t*)(SOC_CAN_REG_BASE + (0x2f << 2));
	return r->ver_0;
}

//reg :ver_1

static inline uint32_t can_ll_get_ver_1(void) {
	can_ttcfg_t *r = (can_ttcfg_t*)(SOC_CAN_REG_BASE + (0x2f << 2));
	return r->ver_1;
}

//reg :tbslot

static inline void can_ll_set_tbptr(uint32_t v) {
	can_ttcfg_t *r = (can_ttcfg_t*)(SOC_CAN_REG_BASE + (0x2f << 2));
	r->tbptr  = v;
}

static inline uint32_t can_ll_get_tbptr(void) {
	can_ttcfg_t *r = (can_ttcfg_t*)(SOC_CAN_REG_BASE + (0x2f << 2));
	return r->tbptr ;
}

static inline void can_ll_set_tbf(uint32_t v) {
	can_ttcfg_t *r = (can_ttcfg_t*)(SOC_CAN_REG_BASE + (0x2f << 2));
	r->tbf  = v;
}

static inline uint32_t can_ll_get_tbf(void) {
	can_ttcfg_t *r = (can_ttcfg_t*)(SOC_CAN_REG_BASE + (0x2f << 2));
	return r->tbf ;
}

static inline void can_ll_set_tbe(uint32_t v) {
	can_ttcfg_t *r = (can_ttcfg_t*)(SOC_CAN_REG_BASE + (0x2f << 2));
	r->tbe  = v;
}

static inline uint32_t can_ll_get_tbe(void) {
	can_ttcfg_t *r = (can_ttcfg_t*)(SOC_CAN_REG_BASE + (0x2f << 2));
	return r->tbe ;
}

//reg :ttcfg

static inline void can_ll_set_ttcfg_tten(uint32_t v) {
	can_ttcfg_t *r = (can_ttcfg_t*)(SOC_CAN_REG_BASE + (0x2f << 2));
	r->tten  = v;
}

static inline uint32_t can_ll_get_ttcfg_tten(void) {
	can_ttcfg_t *r = (can_ttcfg_t*)(SOC_CAN_REG_BASE + (0x2f << 2));
	return r->tten ;
}

static inline void can_ll_set_ttcfg_t_presc(uint32_t v) {
	can_ttcfg_t *r = (can_ttcfg_t*)(SOC_CAN_REG_BASE + (0x2f << 2));
	r->t_presc  = v;
}

static inline uint32_t can_ll_get_ttcfg_t_presc(void) {
	can_ttcfg_t *r = (can_ttcfg_t*)(SOC_CAN_REG_BASE + (0x2f << 2));
	return r->t_presc ;
}

static inline void can_ll_set_ttcfg_ttif(uint32_t v) {
	can_ttcfg_t *r = (can_ttcfg_t*)(SOC_CAN_REG_BASE + (0x2f << 2));
	r->ttif  = v;
}

static inline uint32_t can_ll_get_ttcfg_ttif(void) {
	can_ttcfg_t *r = (can_ttcfg_t*)(SOC_CAN_REG_BASE + (0x2f << 2));
	return r->ttif ;
}

static inline void can_ll_set_ttcfg_ttie(uint32_t v) {
	can_ttcfg_t *r = (can_ttcfg_t*)(SOC_CAN_REG_BASE + (0x2f << 2));
	r->ttie  = v;
}

static inline uint32_t can_ll_get_ttcfg_ttie(void) {
	can_ttcfg_t *r = (can_ttcfg_t*)(SOC_CAN_REG_BASE + (0x2f << 2));
	return r->ttie ;
}

static inline void can_ll_set_ttcfg_teif(uint32_t v) {
	can_ttcfg_t *r = (can_ttcfg_t*)(SOC_CAN_REG_BASE + (0x2f << 2));
	r->teif  = v;
}

static inline uint32_t can_ll_get_ttcfg_teif(void) {
	can_ttcfg_t *r = (can_ttcfg_t*)(SOC_CAN_REG_BASE + (0x2f << 2));
	return r->teif ;
}

static inline void can_ll_set_ttcfg_wtif(uint32_t v) {
	can_ttcfg_t *r = (can_ttcfg_t*)(SOC_CAN_REG_BASE + (0x2f << 2));
	r->wtif  = v;
}

static inline uint32_t can_ll_get_ttcfg_wtif(void) {
	can_ttcfg_t *r = (can_ttcfg_t*)(SOC_CAN_REG_BASE + (0x2f << 2));
	return r->wtif ;
}

static inline void can_ll_set_ttcfg_wtie(uint32_t v) {
	can_ttcfg_t *r = (can_ttcfg_t*)(SOC_CAN_REG_BASE + (0x2f << 2));
	r->wtie  = v;
}

static inline uint32_t can_ll_get_ttcfg_wtie(void) {
	can_ttcfg_t *r = (can_ttcfg_t*)(SOC_CAN_REG_BASE + (0x2f << 2));
	return r->wtie ;
}

//reg :ref_msg

static inline void can_ll_set_ref_msg_value(uint32_t v) {
	can_ref_msg_t *r = (can_ref_msg_t*)(SOC_CAN_REG_BASE + (0x30 << 2));
	r->v = v;
}

static inline uint32_t can_ll_get_ref_msg_value(void) {
	can_ref_msg_t *r = (can_ref_msg_t*)(SOC_CAN_REG_BASE + (0x30 << 2));
	return r->v;
}

static inline void can_ll_set_ref_msg_id(uint32_t v) {
	can_ref_msg_t *r = (can_ref_msg_t*)(SOC_CAN_REG_BASE + (0x30 << 2));
	r->ref_id = v;
}

static inline uint32_t can_ll_get_ref_msg_id(void) {
	can_ref_msg_t *r = (can_ref_msg_t*)(SOC_CAN_REG_BASE + (0x30 << 2));
	return r->ref_id;
}

static inline void can_ll_set_ref_msg_ide(uint32_t v) {
	can_ref_msg_t *r = (can_ref_msg_t*)(SOC_CAN_REG_BASE + (0x30 << 2));
	r->ref_ide = v;
}

static inline uint32_t can_ll_get_ref_msg_ide(void) {
	can_ref_msg_t *r = (can_ref_msg_t*)(SOC_CAN_REG_BASE + (0x30 << 2));
	return r->ref_ide;
}

//reg :trig_cfg0

static inline void can_ll_set_trig_cfg_value(uint32_t v) {
	can_trig_cfg_t *r = (can_trig_cfg_t*)(SOC_CAN_REG_BASE + (0x31 << 2));
	r->v = v;
}

static inline uint32_t can_ll_get_trig_cfg_value(void) {
	can_trig_cfg_t *r = (can_trig_cfg_t*)(SOC_CAN_REG_BASE + (0x31 << 2));
	return r->v;
}

static inline void can_ll_set_ttptr(uint32_t v) {
	can_trig_cfg_t *r = (can_trig_cfg_t*)(SOC_CAN_REG_BASE + (0x31 << 2));
	r->ttptr  = v;
}

static inline uint32_t can_ll_get__ttptr(void) {
	can_trig_cfg_t *r = (can_trig_cfg_t*)(SOC_CAN_REG_BASE + (0x31 << 2));
	return r->ttptr ;
}

//reg :

static inline void can_ll_set_ttype(uint32_t v) {
	can_trig_cfg_t *r = (can_trig_cfg_t*)(SOC_CAN_REG_BASE + (0x31 << 2));
	r->ttype = v;
}

static inline uint32_t can_ll_get_ttype(void) {
	can_trig_cfg_t *r = (can_trig_cfg_t*)(SOC_CAN_REG_BASE + (0x31 << 2));
	return r->ttype;
}

static inline void can_ll_set_tew(uint32_t v) {
	can_trig_cfg_t *r = (can_trig_cfg_t*)(SOC_CAN_REG_BASE + (0x31 << 2));
	r->tew = v;
}

static inline uint32_t can_ll_get_tew(void) {
	can_trig_cfg_t *r = (can_trig_cfg_t*)(SOC_CAN_REG_BASE + (0x31 << 2));
	return r->tew;
}

//reg :

static inline void can_ll_set_tt_trig(uint32_t v) {
	can_trig_cfg_t *r = (can_trig_cfg_t*)(SOC_CAN_REG_BASE + (0x31 << 2));
	r->tttrig  = v;
}

static inline uint32_t can_ll_get_tt_trig(void) {
	can_trig_cfg_t *r = (can_trig_cfg_t*)(SOC_CAN_REG_BASE + (0x31 << 2));
	return r->tttrig ;
}

//reg : mem_port

static inline void can_ll_set_mem_stat_value(uint32_t v) {
	can_mem_stat_t *r = (can_mem_stat_t*)(SOC_CAN_REG_BASE + (0x32 << 2));
	r->v = v;
}

static inline uint32_t can_ll_get_mem_stat_value(void) {
	can_mem_stat_t *r = (can_mem_stat_t*)(SOC_CAN_REG_BASE + (0x32 << 2));
	return r->v;
}

static inline void can_ll_set_tt_wtrig(uint32_t v) {
	can_mem_stat_t *r = (can_mem_stat_t*)(SOC_CAN_REG_BASE + (0x32 << 2));
	r->ttwtrig = v;
}

static inline uint32_t can_ll_get_tt_wtrig(void) {
	can_mem_stat_t *r = (can_mem_stat_t*)(SOC_CAN_REG_BASE + (0x32 << 2));
	return r->ttwtrig;
}

static inline void can_ll_set_mpen(uint32_t v) {
	can_mem_stat_t *r = (can_mem_stat_t*)(SOC_CAN_REG_BASE + (0x32 << 2));
	r->mpen  = v;
}

static inline uint32_t can_ll_get_mpen(void) {
	can_mem_stat_t *r = (can_mem_stat_t*)(SOC_CAN_REG_BASE + (0x32 << 2));
	return r->mpen ;
}

static inline void can_ll_set_mdwie(uint32_t v) {
	can_mem_stat_t *r = (can_mem_stat_t*)(SOC_CAN_REG_BASE + (0x32 << 2));
	r->mdwie  = v;
}

static inline uint32_t can_ll_get_mdwie(void) {
	can_mem_stat_t *r = (can_mem_stat_t*)(SOC_CAN_REG_BASE + (0x32 << 2));
	return r->mdwie ;
}

static inline void can_ll_set_mdwif(uint32_t v) {
	can_mem_stat_t *r = (can_mem_stat_t*)(SOC_CAN_REG_BASE + (0x32 << 2));
	r->mdwif  = v;
}

static inline uint32_t can_ll_get_mdwif(void) {
	can_mem_stat_t *r = (can_mem_stat_t*)(SOC_CAN_REG_BASE + (0x32 << 2));
	return r->mdwif ;
}

static inline void can_ll_set_mdeif(uint32_t v) {
	can_mem_stat_t *r = (can_mem_stat_t*)(SOC_CAN_REG_BASE + (0x32 << 2));
	r->mdeif  = v;
}

static inline uint32_t can_ll_get_mdeif(void) {
	can_mem_stat_t *r = (can_mem_stat_t*)(SOC_CAN_REG_BASE + (0x32 << 2));
	return r->mdeif ;
}

static inline void can_ll_set_maeif(uint32_t v) {
	can_mem_stat_t *r = (can_mem_stat_t*)(SOC_CAN_REG_BASE + (0x32 << 2));
	r->maeif  = v;
}

static inline uint32_t can_ll_get_maeif(void) {
	can_mem_stat_t *r = (can_mem_stat_t*)(SOC_CAN_REG_BASE + (0x32 << 2));
	return r->maeif ;
}

static inline void can_ll_set_acfa(uint32_t v) {
	can_mem_stat_t *r = (can_mem_stat_t*)(SOC_CAN_REG_BASE + (0x32 << 2));
	r->acfa  = v;
}

static inline uint32_t can_ll_get_acfa(void) {
	can_mem_stat_t *r = (can_mem_stat_t*)(SOC_CAN_REG_BASE + (0x32 << 2));
	return r->acfa ;
}

static inline uint32_t can_ll_get_txs(void) {
	can_mem_stat_t *r = (can_mem_stat_t*)(SOC_CAN_REG_BASE + (0x32 << 2));
	return r->txs ;
}

static inline uint32_t can_ll_get_txb(void) {
	can_mem_stat_t *r = (can_mem_stat_t*)(SOC_CAN_REG_BASE + (0x32 << 2));
	return r->txb ;
}

static inline uint32_t can_ll_get_heloc(void) {
	can_mem_stat_t *r = (can_mem_stat_t*)(SOC_CAN_REG_BASE + (0x32 << 2));
	return r->heloc ;
}

//reg :mem_es0

static inline void can_ll_set_mem_es_value(uint32_t v) {
	can_mem_es_t *r = (can_mem_es_t*)(SOC_CAN_REG_BASE + (0x33 << 2));
	r->v = v;
}

static inline uint32_t can_ll_get_mem_es_value(void) {
	can_mem_es_t *r = (can_mem_es_t*)(SOC_CAN_REG_BASE + (0x33 << 2));
	return r->v;
}

static inline void can_ll_set_mebp1(uint32_t v) {
	can_mem_es_t *r = (can_mem_es_t*)(SOC_CAN_REG_BASE + (0x33 << 2));
	r->mebp1  = v;
}

static inline uint32_t can_ll_get_mebp1(void) {
	can_mem_es_t *r = (can_mem_es_t*)(SOC_CAN_REG_BASE + (0x33 << 2));
	return r->mebp1 ;
}

static inline void can_ll_set_me1ee(uint32_t v) {
	can_mem_es_t *r = (can_mem_es_t*)(SOC_CAN_REG_BASE + (0x33 << 2));
	r->me1ee  = v;
}

static inline uint32_t can_ll_get_me1ee(void) {
	can_mem_es_t *r = (can_mem_es_t*)(SOC_CAN_REG_BASE + (0x33 << 2));
	return r->me1ee ;
}

static inline void can_ll_set_meaee(uint32_t v) {
	can_mem_es_t *r = (can_mem_es_t*)(SOC_CAN_REG_BASE + (0x33 << 2));
	r->meaee  = v;
}

static inline uint32_t can_ll_get_meaee(void) {
	can_mem_es_t *r = (can_mem_es_t*)(SOC_CAN_REG_BASE + (0x33 << 2));
	return r->meaee ;
}

//reg :

static inline void can_ll_set_mebp2(uint32_t v) {
	can_mem_es_t *r = (can_mem_es_t*)(SOC_CAN_REG_BASE + (0x33 << 2));
	r->mebp2  = v;
}

static inline uint32_t can_ll_get_mebp2(void) {
	can_mem_es_t *r = (can_mem_es_t*)(SOC_CAN_REG_BASE + (0x33 << 2));
	return r->mebp2 ;
}

static inline void can_ll_set_me2ee(uint32_t v) {
	can_mem_es_t *r = (can_mem_es_t*)(SOC_CAN_REG_BASE + (0x33 << 2));
	r->me2ee  = v;
}

static inline uint32_t can_ll_get_me2ee(void) {
	can_mem_es_t *r = (can_mem_es_t*)(SOC_CAN_REG_BASE + (0x33 << 2));
	return r->me2ee ;
}

//reg :mem_es1

static inline void can_ll_set_meeec(uint32_t v) {
	can_mem_es_t *r = (can_mem_es_t*)(SOC_CAN_REG_BASE + (0x33 << 2));
	r->meeec  = v;
}

static inline uint32_t can_ll_get_meeec(void) {
	can_mem_es_t *r = (can_mem_es_t*)(SOC_CAN_REG_BASE + (0x33 << 2));
	return r->meeec ;
}

static inline void can_ll_set_menec(uint32_t v) {
	can_mem_es_t *r = (can_mem_es_t*)(SOC_CAN_REG_BASE + (0x33 << 2));
	r->menec  = v;
}

static inline uint32_t can_ll_get_menec(void) {
	can_mem_es_t *r = (can_mem_es_t*)(SOC_CAN_REG_BASE + (0x33 << 2));
	return r->menec ;
}

//reg :mem_es3

static inline void can_ll_set_mel(uint32_t v) {
	can_mem_es_t *r = (can_mem_es_t*)(SOC_CAN_REG_BASE + (0x33 << 2));
	r->mel  = v;
}

static inline uint32_t can_ll_get_mel(void) {
	can_mem_es_t *r = (can_mem_es_t*)(SOC_CAN_REG_BASE + (0x33 << 2));
	return r->mel ;
}

static inline void can_ll_set_mes(uint32_t v) {
	can_mem_es_t *r = (can_mem_es_t*)(SOC_CAN_REG_BASE + (0x33 << 2));
	r->mes  = v;
}

static inline uint32_t can_ll_get_mes(void) {
	can_mem_es_t *r = (can_mem_es_t*)(SOC_CAN_REG_BASE + (0x33 << 2));
	return r->mes ;
}

//reg :scfg

static inline void can_ll_set_scfg_value(uint32_t v) {
	can_scfg_t *r = (can_scfg_t*)(SOC_CAN_REG_BASE + (0x34 << 2));
	r->v = v;
}

static inline uint32_t can_ll_get_scfg_value(void) {
	can_scfg_t *r = (can_scfg_t*)(SOC_CAN_REG_BASE + (0x34 << 2));
	return r->v;
}

static inline void can_ll_set_xmren(uint32_t v) {
	can_scfg_t *r = (can_scfg_t*)(SOC_CAN_REG_BASE + (0x34 << 2));
	r->xmren  = v;
}

static inline uint32_t can_ll_get_xmren(void) {
	can_scfg_t *r = (can_scfg_t*)(SOC_CAN_REG_BASE + (0x34 << 2));
	return r->xmren ;
}

static inline void can_ll_set_seif(uint32_t v) {
	can_scfg_t *r = (can_scfg_t*)(SOC_CAN_REG_BASE + (0x34 << 2));
	r->seif  = v;
}

static inline uint32_t can_ll_get_seif(void) {
	can_scfg_t *r = (can_scfg_t*)(SOC_CAN_REG_BASE + (0x34 << 2));
	return r->seif ;
}

static inline void can_ll_set_swie(uint32_t v) {
	can_scfg_t *r = (can_scfg_t*)(SOC_CAN_REG_BASE + (0x34 << 2));
	r->swie  = v;
}

static inline uint32_t can_ll_get_swie(void) {
	can_scfg_t *r = (can_scfg_t*)(SOC_CAN_REG_BASE + (0x34 << 2));
	return r->swie ;
}

static inline void can_ll_set_swif(uint32_t v) {
	can_scfg_t *r = (can_scfg_t*)(SOC_CAN_REG_BASE + (0x34 << 2));
	r->swif  = v;
}

static inline uint32_t can_ll_get_swif(void) {
	can_scfg_t *r = (can_scfg_t*)(SOC_CAN_REG_BASE + (0x34 << 2));
	return r->swif;
}

static inline void can_ll_set_fstim(uint32_t v) {
	can_scfg_t *r = (can_scfg_t*)(SOC_CAN_REG_BASE + (0x34 << 2));
	r->fstim  = v;
}

static inline uint32_t can_ll_get_fstim(void) {
	can_scfg_t *r = (can_scfg_t*)(SOC_CAN_REG_BASE + (0x34 << 2));
	return r->fstim;
}

//reg 0x00000800:

static inline void can_ll_set_fd_enable(uint32_t v) {
	can_fd_t *r = (can_fd_t*)(SOC_CAN_REG_BASE + 0x800 );
	r->can_fd_enable = v;
}

static inline uint32_t can_ll_get_fd_enable(void) {
	can_fd_t *r = (can_fd_t*)(SOC_CAN_REG_BASE + 0x800);
	return r->can_fd_enable;
}
#ifdef __cplusplus
}
#endif
