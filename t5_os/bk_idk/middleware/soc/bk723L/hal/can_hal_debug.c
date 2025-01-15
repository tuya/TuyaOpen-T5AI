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
#include "can_hw.h"
// #include "can_hal.h"

typedef void (*can_dump_fn_t)(void);
typedef struct {
	uint32_t start;
	uint32_t end;
	can_dump_fn_t fn;
} can_reg_fn_map_t;

static void can_dump_rid_esi(void)
{
	can_id_esi_t *r = (can_id_esi_t *)(SOC_CAN_REG_BASE + (0x0 << 2));

	SOC_LOGI("rid_esi: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x0 << 2)));
	SOC_LOGI("	id: %8x\r\n", r->id);
	SOC_LOGI("	reserved_29_30: %8x\r\n", r->reserved_29_30);
	SOC_LOGI("	esi: %8x\r\n", r->esi);
}

static void can_dump_rbuf_ctrl(void)
{
	can_buf_ctrl_t *r = (can_buf_ctrl_t *)(SOC_CAN_REG_BASE + (0x1 << 2));

	SOC_LOGI("rbuf_ctrl: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x1 << 2)));
	SOC_LOGI("	dlc: %8x\r\n", r->dlc);
	SOC_LOGI("	brs: %8x\r\n", r->brs);
	SOC_LOGI("	fdf: %8x\r\n", r->fdf);
	SOC_LOGI("	rtr: %8x\r\n", r->rtr);
	SOC_LOGI("	ide: %8x\r\n", r->ide);
	SOC_LOGI("	reserved_8_31: %8x\r\n", r->reserved_8_31);
}

static void can_dump_rdata(void)
{
	for (uint32_t idx = 0; idx < 16; idx++) {
		SOC_LOGI("rdata: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + ((0x2 + idx) << 2)));
	}
}

static void can_dump_tid_esi(void)
{
	can_id_esi_t *r = (can_id_esi_t *)(SOC_CAN_REG_BASE + (0x14 << 2));

	SOC_LOGI("tid_esi: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x14 << 2)));
	SOC_LOGI("	tid: %8x\r\n", r->id);
	SOC_LOGI("	reserved_29_30: %8x\r\n", r->reserved_29_30);
	SOC_LOGI("	esi: %8x\r\n", r->esi);
}

static void can_dump_tbuf_ctrl(void)
{
	can_buf_ctrl_t *r = (can_buf_ctrl_t *)(SOC_CAN_REG_BASE + (0x15 << 2));

	SOC_LOGI("tbuf_ctrl: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x15 << 2)));
	SOC_LOGI("	dlc: %8x\r\n", r->dlc);
	SOC_LOGI("	brs: %8x\r\n", r->brs);
	SOC_LOGI("	fdf: %8x\r\n", r->fdf);
	SOC_LOGI("	rtr: %8x\r\n", r->rtr);
	SOC_LOGI("	ide: %8x\r\n", r->ide);
	SOC_LOGI("	reserved_8_31: %8x\r\n", r->reserved_8_31);
}

static void can_dump_tdata(void)
{
	for (uint32_t idx = 0; idx < 16; idx++) {
		SOC_LOGI("tdata: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + ((0x16 + idx) << 2)));
	}
}

static void can_dump_tts_l(void)
{
	SOC_LOGI("tts_l: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x26 << 2)));
}

static void can_dump_tts_h(void)
{
	SOC_LOGI("tts_H: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x27 << 2)));
}

static void can_dump_cfg(void)
{
	can_cfg_t *r = (can_cfg_t *)(SOC_CAN_REG_BASE + (0x28 << 2));
	SOC_LOGI("cfg: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x28 << 2)));
	SOC_LOGI("	busoff : %8x\r\n", r->busoff );
	SOC_LOGI("	tactive : %8x\r\n", r->tactive );
	SOC_LOGI("	ractive : %8x\r\n", r->ractive );
	SOC_LOGI("	tsss : %8x\r\n", r->tsss );
	SOC_LOGI("	tpss : %8x\r\n", r->tpss );
	SOC_LOGI("	lbmi: %8x\r\n", r->lbmi);
	SOC_LOGI("	lbme: %8x\r\n", r->lbme);
	SOC_LOGI("	reset: %8x\r\n", r->reset);
	SOC_LOGI("	tsa : %8x\r\n", r->tsa );
	SOC_LOGI("	tsall : %8x\r\n", r->tsall );
	SOC_LOGI("	tsone : %8x\r\n", r->tsone );
	SOC_LOGI("	tpa : %8x\r\n", r->tpa );
	SOC_LOGI("	tpe : %8x\r\n", r->tpe );
	SOC_LOGI("	stby : %8x\r\n", r->stby );
	SOC_LOGI("	lom : %8x\r\n", r->lom );
	SOC_LOGI("	tbsel : %8x\r\n", r->tbsel );
	SOC_LOGI("	tsstat : %8x\r\n", r->tsstat );
	SOC_LOGI("	reserved_18_19: %8x\r\n", r->reserved_18_19);
	SOC_LOGI("	tttbm : %8x\r\n", r->tttbm );
	SOC_LOGI("	tsmode : %8x\r\n", r->tsmode );
	SOC_LOGI("	tsnext : %8x\r\n", r->tsnext );
	SOC_LOGI("	fd_iso : %8x\r\n", r->fd_iso );
	SOC_LOGI("	rstat : %8x\r\n", r->rstat );
	SOC_LOGI("	reserved_26_26: %8x\r\n", r->reserved_26_26);
	SOC_LOGI("	rball : %8x\r\n", r->rball );
	SOC_LOGI("	rrel : %8x\r\n", r->rrel );
	SOC_LOGI("	rov : %8x\r\n", r->rov );
	SOC_LOGI("	rom : %8x\r\n", r->rom );
	SOC_LOGI("	sack : %8x\r\n", r->sack );
}


static void can_dump_ie(void)
{
	can_ie_t *r = (can_ie_t *)(SOC_CAN_REG_BASE + (0x29 << 2));

	SOC_LOGI("ie: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x29 << 2)));
	SOC_LOGI("	tsff : %8x\r\n", r->tsff );
	SOC_LOGI("	eie : %8x\r\n", r->eie );
	SOC_LOGI("	tsie : %8x\r\n", r->tsie );
	SOC_LOGI("	tpie : %8x\r\n", r->tpie );
	SOC_LOGI("	rafie : %8x\r\n", r->rafie );
	SOC_LOGI("	rfie : %8x\r\n", r->rfie );
	SOC_LOGI("	roie : %8x\r\n", r->roie );
	SOC_LOGI("	rie : %8x\r\n", r->rie );
	SOC_LOGI("	aif : %8x\r\n", r->aif );
	SOC_LOGI("	eif : %8x\r\n", r->eif );
	SOC_LOGI("	tsif : %8x\r\n", r->tsif );
	SOC_LOGI("	tpif : %8x\r\n", r->tpif );
	SOC_LOGI("	rafif : %8x\r\n", r->rafif );
	SOC_LOGI("	rfif : %8x\r\n", r->rfif );
	SOC_LOGI("	roif : %8x\r\n", r->roif );
	SOC_LOGI("	rif : %8x\r\n", r->rif );
	SOC_LOGI("	beif : %8x\r\n", r->beif );
	SOC_LOGI("	beie : %8x\r\n", r->beie );
	SOC_LOGI("	alif : %8x\r\n", r->alif );
	SOC_LOGI("	alie : %8x\r\n", r->alie );
	SOC_LOGI("	epif : %8x\r\n", r->epif );
	SOC_LOGI("	epie : %8x\r\n", r->epie );
	SOC_LOGI("	epass : %8x\r\n", r->epass );
	SOC_LOGI("	ewarn : %8x\r\n", r->ewarn );
	SOC_LOGI("	ewl: %8x\r\n", r->ewl);
	SOC_LOGI("	afwl: %8x\r\n", r->afwl);
}


static void can_dump_sseg(void)
{
	can_sseg_t *r = (can_sseg_t *)(SOC_CAN_REG_BASE + (0x2a << 2));

	SOC_LOGI("sseg: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x2a << 2)));
	SOC_LOGI("	s_seg_1: %8x\r\n", r->s_seg_1);
	SOC_LOGI("	s_seg_2: %8x\r\n", r->s_seg_2);
	SOC_LOGI("	reserved_15_15: %8x\r\n", r->reserved_15_15);
	SOC_LOGI("	s_sjw: %8x\r\n", r->s_sjw);
	SOC_LOGI("	reserved_23_23: %8x\r\n", r->reserved_23_23);
	SOC_LOGI("	s_presc: %8x\r\n", r->s_presc);
}

static void can_dump_fseg(void)
{
	can_fseg_t *r = (can_fseg_t *)(SOC_CAN_REG_BASE + (0x2b << 2));

	SOC_LOGI("fseg: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x2b << 2)));
	SOC_LOGI("	f_seg_1: %8x\r\n", r->f_seg_1);
	SOC_LOGI("	reserved_5_7: %8x\r\n", r->reserved_5_7);
	SOC_LOGI("	f_seg_2: %8x\r\n", r->f_seg_2);
	SOC_LOGI("	reserved_12_15: %8x\r\n", r->reserved_12_15);
	SOC_LOGI("	f_sjw: %8x\r\n", r->f_sjw);
	SOC_LOGI("	reserved_20_23: %8x\r\n", r->reserved_20_23);
	SOC_LOGI("	_presc: %8x\r\n", r->f_presc);
}

static void can_dump_cap(void)
{
	can_cap_t *r = (can_cap_t *)(SOC_CAN_REG_BASE + (0x2c << 2));

	SOC_LOGI("ealcap: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x2c << 2)));
	SOC_LOGI("	alc: %8x\r\n", r->alc);
	SOC_LOGI("	koer: %8x\r\n", r->koer);
	SOC_LOGI("	sspoff: %8x\r\n", r->sspoff);
	SOC_LOGI("	tdcen: %8x\r\n", r->tdcen);
	SOC_LOGI("	recnt: %8x\r\n", r->recnt);
	SOC_LOGI("	tecnt: %8x\r\n", r->tecnt);
}

static void can_dump_acf(void)
{
	can_acf_t *r = (can_acf_t *)(SOC_CAN_REG_BASE + (0x2d << 2));

	SOC_LOGI("acf: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x2d << 2)));
	SOC_LOGI("	acfadr : %8x\r\n", r->acfadr );
	SOC_LOGI("	reserved_4_4: %8x\r\n", r->reserved_4_4);
	SOC_LOGI("	selmask : %8x\r\n", r->selmask );
	SOC_LOGI("	reserved_6_7: %8x\r\n", r->reserved_6_7);
	SOC_LOGI("	timeen: %8x\r\n", r->timeen);
	SOC_LOGI("	timepos: %8x\r\n", r->timepos);
	SOC_LOGI("	reserved_10_15: %8x\r\n", r->reserved_10_15);
	SOC_LOGI("	ae_x: %8x\r\n", r->acf_en);
}



static void can_dump_aid(void)
{
	can_aid_t *r = (can_aid_t *)(SOC_CAN_REG_BASE + (0x2e << 2));

	SOC_LOGI("aid: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x2e << 2)));
	SOC_LOGI("	acode_or_amask: %8x\r\n", r->acode_or_amask);
	SOC_LOGI("	aide: %8x\r\n", r->aide);
	SOC_LOGI("	aidee: %8x\r\n", r->aidee);
	SOC_LOGI("	reserved_31_31: %8x\r\n", r->reserved_31_31);
}

static void can_dump_ttcfg(void)
{
	can_ttcfg_t *r = (can_ttcfg_t *)(SOC_CAN_REG_BASE + (0x2f << 2));

	SOC_LOGI("ttcfg: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x2f << 2)));
	SOC_LOGI("	ver_0: %8x\r\n", r->ver_0);
	SOC_LOGI("	ver_1: %8x\r\n", r->ver_1);
	SOC_LOGI("	tbptr : %8x\r\n", r->tbptr );
	SOC_LOGI("	tbf : %8x\r\n", r->tbf );
	SOC_LOGI("	tbe : %8x\r\n", r->tbe );
	SOC_LOGI("	tten : %8x\r\n", r->tten );
	SOC_LOGI("	t_presc : %8x\r\n", r->t_presc );
	SOC_LOGI("	ttif : %8x\r\n", r->ttif );
	SOC_LOGI("	ttie : %8x\r\n", r->ttie );
	SOC_LOGI("	teif : %8x\r\n", r->teif );
	SOC_LOGI("	wtif : %8x\r\n", r->wtif );
	SOC_LOGI("	wtie : %8x\r\n", r->wtie );
}

static void can_dump_ref_msg(void)
{
	can_ref_msg_t *r = (can_ref_msg_t *)(SOC_CAN_REG_BASE + (0x30 << 2));

	SOC_LOGI("ref_msg: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x30 << 2)));
	SOC_LOGI("	ref_id: %8x\r\n", r->ref_id);
	SOC_LOGI("	reserved_29_30: %8x\r\n", r->reserved_29_30);
	SOC_LOGI("	ref_ide: %8x\r\n", r->ref_ide);
}

static void can_dump_trig_cfg(void)
{
	can_trig_cfg_t *r = (can_trig_cfg_t *)(SOC_CAN_REG_BASE + (0x31 << 2));

	SOC_LOGI("trig_cfg: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x31 << 2)));
	SOC_LOGI("	ttptr : %8x\r\n", r->ttptr );
	SOC_LOGI("	reserved_6_7: %8x\r\n", r->reserved_6_7);
	SOC_LOGI("	ttype: %8x\r\n", r->ttype);
	SOC_LOGI("	reserved_11_11: %8x\r\n", r->reserved_11_11);
	SOC_LOGI("	tew: %8x\r\n", r->tew);
	SOC_LOGI("	tttrig : %8x\r\n", r->tttrig );
}

static void can_dump_mem_stat(void)
{
	can_mem_stat_t *r = (can_mem_stat_t *)(SOC_CAN_REG_BASE + (0x32 << 2));

	SOC_LOGI("mem_stat: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x32 << 2)));
	SOC_LOGI("	tt_wtrig: %8x\r\n", r->ttwtrig);
	SOC_LOGI("	mpen : %8x\r\n", r->mpen );
	SOC_LOGI("	mdwie : %8x\r\n", r->mdwie );
	SOC_LOGI("	mdwif : %8x\r\n", r->mdwif );
	SOC_LOGI("	mdeif : %8x\r\n", r->mdeif );
	SOC_LOGI("	maeif : %8x\r\n", r->maeif );
	SOC_LOGI("	reserved_21_23: %8x\r\n", r->reserved_21_23);
	SOC_LOGI("	acfa : %8x\r\n", r->acfa );
	SOC_LOGI("	txs : %8x\r\n", r->txs );
	SOC_LOGI("	txb : %8x\r\n", r->txb );
	SOC_LOGI("	heloc : %8x\r\n", r->heloc );
	SOC_LOGI("	reserved_29_31: %8x\r\n", r->reserved_29_31);
}

static void can_dump_mem_es(void)
{
	can_mem_es_t *r = (can_mem_es_t *)(SOC_CAN_REG_BASE + (0x33 << 2));

	SOC_LOGI("mem_es: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x33 << 2)));
	SOC_LOGI("	mebp1 : %8x\r\n", r->mebp1 );
	SOC_LOGI("	me1ee : %8x\r\n", r->me1ee );
	SOC_LOGI("	meaee : %8x\r\n", r->meaee );
	SOC_LOGI("	mebp2 : %8x\r\n", r->mebp2 );
	SOC_LOGI("	me2ee : %8x\r\n", r->me2ee );
	SOC_LOGI("	reserved_15_15: %8x\r\n", r->reserved_15_15);
	SOC_LOGI("	meeec : %8x\r\n", r->meeec );
	SOC_LOGI("	menec : %8x\r\n", r->menec );
	SOC_LOGI("	mel : %8x\r\n", r->mel );
	SOC_LOGI("	mes : %8x\r\n", r->mes );
	SOC_LOGI("	reserved_27_31: %8x\r\n", r->reserved_27_31);
}

static void can_dump_scfg(void)
{
	can_scfg_t *r = (can_scfg_t *)(SOC_CAN_REG_BASE + (0x34 << 2));

	SOC_LOGI("scfg: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x34 << 2)));
	SOC_LOGI("	xmren : %8x\r\n", r->xmren );
	SOC_LOGI("	seif : %8x\r\n", r->seif );
	SOC_LOGI("	swie : %8x\r\n", r->swie );
	SOC_LOGI("	swif : %8x\r\n", r->swif );
	SOC_LOGI("	fstim : %8x\r\n", r->fstim );
	SOC_LOGI("	reserved_7_31: %8x\r\n", r->reserved_7_31);
}

static void can_dump_fd(void)
{
	can_fd_t *r = (can_fd_t *)(SOC_CAN_REG_BASE + (0x200 << 2));

	SOC_LOGI("fd: %8x\r\n", REG_READ(SOC_CAN_REG_BASE + (0x200 << 2)));
	SOC_LOGI("	can_fd_enable: %8x\r\n", r->can_fd_enable);
	SOC_LOGI("	reserved_1_31: %8x\r\n", r->reserved_1_31);
}

static can_reg_fn_map_t s_can_fn[] =
{
	{0x0, 0x0, can_dump_rid_esi},
	{0x1, 0x1, can_dump_rbuf_ctrl},
	{0x2, 0x11, can_dump_rdata},
	{0x14, 0x14, can_dump_tid_esi},
	{0x15, 0x15, can_dump_tbuf_ctrl},
	{0x16, 0x25, can_dump_tdata},
	{0x26, 0x26, can_dump_tts_l},
	{0x27, 0x27, can_dump_tts_h},
	{0x28, 0x28, can_dump_cfg},
	{0x29, 0x29, can_dump_ie},
	{0x2a, 0x2a, can_dump_sseg},
	{0x2b, 0x2b, can_dump_fseg},
	{0x2c, 0x2c, can_dump_cap},
	{0x2d, 0x2d, can_dump_acf},
	{0x2e, 0x2e, can_dump_aid},
	{0x2f, 0x2f, can_dump_ttcfg},
	{0x30, 0x30, can_dump_ref_msg},
	{0x31, 0x31, can_dump_trig_cfg},
	{0x32, 0x32, can_dump_mem_stat},
	{0x33, 0x33, can_dump_mem_es},
	{0x34, 0x34, can_dump_scfg},
	{0x200, 0x200, can_dump_fd},
	{-1, -1, 0}
};

void can_struct_dump(uint32_t start, uint32_t end)
{
	uint32_t dump_fn_cnt = sizeof(s_can_fn)/sizeof(s_can_fn[0]) - 1;

	for (uint32_t idx = 0; idx < dump_fn_cnt; idx++) {
		if ((start <= s_can_fn[idx].start) && (end >= s_can_fn[idx].end)) {
			s_can_fn[idx].fn();
		}
	}
}
