#pragma once

#define CONFIG_SAU_RECONFIG           1
#define CONFIG_RESTORE_VIA_EXC_RETURN 1

#if CONFIG_DEEP_LOW_VOLTAGE_SLEEP
extern unsigned char _deep_lv_start;
#define DLV_START_ADDR (uint32_t)&_deep_lv_start

#define DLV_SEC          __attribute__((section(".deep_lv")))
#else
#define DLV_SEC
#endif

#if 1
#define DLV_STATIC 
#else
#define DLV_STATIC static inline
#endif

typedef struct _dlv_scb_
{
	uint32_t vtor_val;
	uint32_t cpacr_val;
	uint32_t ccr_val;
	uint32_t aircr_val;
	uint32_t nsacr_val;
}dlv_scb_t;

typedef struct _dlv_nvic_
{
	uint32_t iser_val[2];
	uint32_t icer_val[2];
	uint32_t itns_val[2];
	uint32_t ipr_val[2];
}dlv_nvic_t;

typedef struct _dlv_core_
{
	uint32_t msplim_val;
	uint32_t psp_val;
	uint32_t psplim_val;

	uint32_t control_s_val;
	uint32_t fault_mask_s_val;
	uint32_t basepri_s_val;
	uint32_t primask_s_val;

	uint32_t control_ns_val;
	uint32_t fault_mask_ns_val;
	uint32_t basepri_ns_val;
	uint32_t primask_ns_val;
}dlv_core_t;

typedef struct _dlv_sau_
{
	uint32_t is_enable;
	uint32_t ctrl_val;
	uint32_t rnr_val;
	uint32_t rbar_val[8];
	uint32_t rlar_val[8];
}dlv_sau_t;

typedef struct _dlv_mpu_
{
	uint32_t is_enable;
	uint32_t ctrl_val;
	uint32_t mair0_val;
	uint32_t mair1_val;
	uint32_t rnr_val;
	uint32_t rbar_val[16];
	uint32_t rlar_val[16];
}dlv_mpu_t;

typedef struct _dlv_itcm_
{
	uint32_t tcmcr_val;
	uint32_t tgu_ctrl_val;
	uint32_t tgu_lut_val;
}dlv_itcm_t;

typedef struct _dlv_dtcm_
{
	uint32_t tcmcr_val;
	uint32_t tgu_ctrl_val;
	uint32_t tgu_lut_val;
}dlv_dtcm_t;

typedef struct _dlv_fpu_
{
	uint32_t fpccr_val;
	uint32_t fpcar_val;
	uint32_t fpdscr_val;
}dlv_fpu_t;

typedef struct _dlv_debug_
{
	uint32_t dauthctrl_val;
	/*debug components*/
	/*fpb module*/
	/*dwt module*/
}dlv_debug_t;

typedef struct _dlv_stk_frame_
{
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r12;
	uint32_t lr;
	uint32_t ret_pc;
	uint32_t xpsr;
}dlv_stack_frame_t;

typedef struct _dlv_context_
{
	dlv_scb_t sys_ctrl;
	dlv_nvic_t nvic;

	dlv_sau_t sau;
	dlv_mpu_t mpu;
	dlv_fpu_t fpu;
	dlv_debug_t debug;

	dlv_itcm_t itcm;
	dlv_dtcm_t dtcm;

	dlv_core_t core;
	dlv_stack_frame_t stk_frame;
}dlv_context_t;

extern void dlv_context_restore(void);
// eof

