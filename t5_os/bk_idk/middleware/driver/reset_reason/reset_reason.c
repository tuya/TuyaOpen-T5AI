#include <common/bk_include.h>
#include "bk_fake_clock.h"
#include "bk_icu.h"
#include "bk_drv_model.h"
#include "bk_uart.h"

#include "bk_arm_arch.h"
#include "sys_ctrl.h"
#include "bk_sys_ctrl.h"

#include "reset_reason.h"
#include <components/log.h>
#include "aon_pmu_hal.h"
#include "driver/gpio.h"

#define TAG "init"
#define DISPLAY_START_TYPE_STR 1



static uint32_t s_start_type;
static uint32_t s_misc_value_save;
static uint32_t s_mem_value_save;

uint32_t bk_misc_get_reset_reason(void)
{
	return s_start_type;
}

void persist_memory_init(void)
{
	*((volatile uint32_t *)(PERSIST_MEMORY_ADDR)) = (uint32_t)CRASH_ILLEGAL_JUMP_VALUE;
}

uint32_t persist_memory_get(void)
{
	return (*((volatile uint32_t *)(PERSIST_MEMORY_ADDR)));
}

bool persist_memory_is_lost(void)
{
	if ((uint32_t)CRASH_ILLEGAL_JUMP_VALUE == persist_memory_get())
		return false;
	else
		return true;
}

static char *misc_get_start_type_str(uint32_t start_type)
{
#if DISPLAY_START_TYPE_STR
	switch (start_type) {
	case RESET_SOURCE_POWERON:
		return "power on";

	case RESET_SOURCE_REBOOT:
		return "software reboot";

	case RESET_SOURCE_WATCHDOG:
		return "interrupt watchdog";

	case RESET_SOURCE_DEEPPS_GPIO:
		return "deep sleep gpio";

	case RESET_SOURCE_DEEPPS_RTC:
		return "deep sleep rtc";

	case RESET_SOURCE_DEEPPS_TOUCH:
		return "deep sleep touch";

	case RESET_SOURCE_CRASH_ILLEGAL_JUMP:
		return "illegal jump";

	case RESET_SOURCE_CRASH_UNDEFINED:
		return "undefined";

	case RESET_SOURCE_CRASH_PREFETCH_ABORT:
		return "prefetch abort";

	case RESET_SOURCE_CRASH_DATA_ABORT:
		return "data abort";

	case RESET_SOURCE_CRASH_UNUSED:
		return "unused";

	case RESET_SOURCE_CRASH_ILLEGAL_INSTRUCTION:
		return "illegal instruction";

	case RESET_SOURCE_CRASH_MISALIGNED:
		return "misaligned";

	case RESET_SOURCE_CRASH_ASSERT:
		return "assert";

	case RESET_SOURCE_DEEPPS_USB:
		return "deep sleep usb";

	case RESET_SOURCE_SUPER_DEEP:
		return "super deep sleep";

	case RESET_SOURCE_NMI_WDT:
		return "nmi watchdog";

	case RESET_SOURCE_HARD_FAULT:
		return "hard fault";

	case RESET_SOURCE_MPU_FAULT:
		return "mpu fault";

	case RESET_SOURCE_BUS_FAULT:
		return "bus fault";

	case RESET_SOURCE_USAGE_FAULT:
		return "usage fault";

	case RESET_SOURCE_SECURE_FAULT:
		return "secure fault";

	case RESET_SOURCE_DEFAULT_EXCEPTION:
		return "default exception";

	case RESET_SOURCE_OTA_REBOOT:
		return "ota reboot";

	case RESET_SOURCE_BROWN_OUT:
		return "brown out";

	case RESET_SOURCE_UNKNOWN:
		return "unknown";

	case RESET_SOURCE_BOOTLOADER_NMI_WDT:
		return "bl2 nmi watchdog";

	case RESET_SOURCE_BOOTLOADER_HARD_FAULT:
		return "bl2 hard fault";

	case RESET_SOURCE_BOOTLOADER_MPU_FAULT:
		return "bl2 mpu fault";

	case RESET_SOURCE_BOOTLOADER_BUS_FAULT:
		return "bl2 bus fault";

	case RESET_SOURCE_BOOTLOADER_REBOOT:
		return "bl2 reboot";

	case RESET_SOURCE_BOOTLOADER_UNKNOWN:
		return "bl2 unknown";

	default:
		// Chip power on the value of start address may not always be 0
		// return "unknown";
		return "power on";
	}
#else
	return "";
#endif
}

void show_reset_reason(void)
{
	// Modified by TUYA Start
	BK_LOGW(TAG, "reason - %s\r\n", misc_get_start_type_str(s_start_type));
	if(RESET_SOURCE_DEEPPS_GPIO == s_start_type)
	{
#if CONFIG_DEEP_PS
		BK_LOGW(TAG, "by gpio - %d\r\n", bk_misc_wakeup_get_gpio_num());
#else
#ifdef CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT
		BK_LOGW(TAG, "by gpio - %d\r\n", bk_gpio_get_wakeup_gpio_id());
#endif
#endif
	}
	BK_LOGW(TAG, "regs - %x, %x, %x\r\n", s_start_type, s_misc_value_save, s_mem_value_save);
	// Modified by TUYA End
}

#if (CONFIG_SOC_BK7231N) || (CONFIG_SOC_BK7236A)
//only can be do once
uint32_t reset_reason_init(void)
{
	uint32_t misc_value;
	sctrl_ctrl(CMD_GET_SCTRL_RETETION, &misc_value);

	if ((s_start_type = sctrl_get_deep_sleep_wake_soure()) == 0) {
		if (0 == (misc_value & SW_RETENTION_WDT_FLAG)) {
			if (persist_memory_is_lost())
				s_start_type = RESET_SOURCE_POWERON;
			else
				s_start_type = RESET_SOURCE_CRASH_ILLEGAL_JUMP;
		} else {
			switch (misc_value & SW_RETENTION_VAL_MASK) {
			case (RESET_SOURCE_REBOOT & SW_RETENTION_VAL_MASK):
				s_start_type = RESET_SOURCE_REBOOT;
				break;
			case (CRASH_UNDEFINED_VALUE & SW_RETENTION_VAL_MASK):
				s_start_type = RESET_SOURCE_CRASH_UNDEFINED;
				break;
			case (CRASH_PREFETCH_ABORT_VALUE & SW_RETENTION_VAL_MASK):
				s_start_type = RESET_SOURCE_CRASH_PREFETCH_ABORT;
				break;
			case (CRASH_DATA_ABORT_VALUE & SW_RETENTION_VAL_MASK):
				s_start_type = RESET_SOURCE_CRASH_DATA_ABORT;
				break;
			case (CRASH_UNUSED_VALUE & SW_RETENTION_VAL_MASK):
				s_start_type = RESET_SOURCE_CRASH_UNUSED;
				break;
			case (RESET_SOURCE_WATCHDOG & SW_RETENTION_VAL_MASK):
				s_start_type = RESET_SOURCE_WATCHDOG;
				break;
			default:
				s_start_type = RESET_SOURCE_UNKNOWN;
				break;
			}
		}
	}

	s_misc_value_save = misc_value;
	s_mem_value_save = persist_memory_get();
	persist_memory_init();

	//clear
	sctrl_ctrl(CMD_SET_SCTRL_RETETION, &misc_value);

	return s_start_type;
}

void bk_misc_set_reset_reason(uint32_t type)
{
	uint32_t misc_value = type & SW_RETENTION_VAL_MASK;
	sctrl_ctrl(CMD_SET_SCTRL_RETETION, &misc_value);
}

#elif (CONFIG_SOC_BK7256XX)

uint32_t reset_reason_init(void) {
#if (CONFIG_SYS_CPU0)
	uint32_t misc_value = REG_READ(START_TYPE_ADDR) >> 1;

	if (misc_value == (POWERON_INIT_MEM_TAG >> 1)) {
		s_start_type = RESET_SOURCE_POWERON;
	} else {
		s_start_type = misc_value;
	}

	s_misc_value_save = misc_value;
	s_mem_value_save = persist_memory_get();
	persist_memory_init();

#endif

	return s_start_type;
}

void bk_misc_set_reset_reason(uint32_t type)
{
#if (CONFIG_SYS_CPU0)
	uint32_t misc_value = (type << 1) | REG_GET_BIT(START_TYPE_ADDR, 0x1);
	REG_WRITE(START_TYPE_ADDR, misc_value);
#endif
}

#elif (CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7239XX) || (CONFIG_SOC_BK7286XX)

uint32_t reset_reason_init(void)
{
	uint32_t misc_value;

	if (s_start_type) {
		return s_start_type;
	}

	misc_value = aon_pmu_ll_get_r7b();
	misc_value = ((misc_value >> 24) & 0x7f);

	s_start_type = misc_value;
	s_misc_value_save = misc_value;
	bk_misc_set_reset_reason(RESET_SOURCE_UNKNOWN);

	return s_start_type;
}

void bk_misc_set_reset_reason(uint32_t type)
{
	/* use PMU_REG0 bit[24:30] for reset reason */
	uint32_t misc_value = aon_pmu_ll_get_r0();

	/* clear last reset reason */
	misc_value &= ~(0x7f << 24);

	misc_value |= ((type & 0x7f) << 24);
	aon_pmu_ll_set_r0(misc_value);

	/* pass PMU_REGO value to PMU_REG7B*/
	aon_pmu_ll_set_r25(0x424B55AA);
	aon_pmu_ll_set_r25(0xBDB4AA55);
}

#else
//only can be do once
uint32_t reset_reason_init(void)
{
	uint32_t misc_value = *((volatile uint32_t *)(START_TYPE_ADDR));

#if (!CONFIG_SOC_BK7271)
	if ((s_start_type = sctrl_get_deep_sleep_wake_soure()) == 0)
#else
	BK_LOGI(TAG, "reset_reason_init TODO\r\n");
#endif
	{
		switch (misc_value) {
		case RESET_SOURCE_REBOOT:
			s_start_type = misc_value;
			break;
		case CRASH_UNDEFINED_VALUE:
			s_start_type = RESET_SOURCE_CRASH_UNDEFINED;
			break;
		case CRASH_PREFETCH_ABORT_VALUE:
			s_start_type = RESET_SOURCE_CRASH_PREFETCH_ABORT;
			break;
		case CRASH_DATA_ABORT_VALUE:
			s_start_type = RESET_SOURCE_CRASH_DATA_ABORT;
			break;
		case CRASH_UNUSED_VALUE:
			s_start_type = RESET_SOURCE_CRASH_UNUSED;
			break;
		case CRASH_ILLEGAL_JUMP_VALUE:
			s_start_type = RESET_SOURCE_CRASH_ILLEGAL_JUMP;
			break;
		case RESET_SOURCE_WATCHDOG:
			if (persist_memory_is_lost())
				s_start_type = RESET_SOURCE_WATCHDOG;
			else
				s_start_type = RESET_SOURCE_CRASH_ILLEGAL_JUMP;
			break;

		default:
			if (persist_memory_is_lost())
				s_start_type = RESET_SOURCE_POWERON;
			else
				s_start_type = RESET_SOURCE_CRASH_ILLEGAL_JUMP;
			break;
		}
	}

	s_misc_value_save = misc_value;
	s_mem_value_save = persist_memory_get();
	persist_memory_init();

	return s_start_type;
}

void bk_misc_set_reset_reason(uint32_t type)
{
	*((volatile uint32_t *)(START_TYPE_ADDR)) = (uint32_t)type;
}
#endif
