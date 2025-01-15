/**
 * @if copyright_display
 *      Copyright (C), 2018-2021, Arm Technology (China) Co., Ltd.
 *      All rights reserved
 *
 *      The content of this file or document is CONFIDENTIAL and PROPRIETARY
 *      to Arm Technology (China) Co., Ltd. It is subject to the terms of a
 *      License Agreement between Licensee and Arm Technology (China) Co., Ltd
 *      restricting among other things, the use, reproduction, distribution
 *      and transfer.  Each of the embodiments, including this information and
 *      any derivative work shall retain this copyright notice.
 * @endif
 */

#include <stdint.h>
#include "pal_log.h"
#include "dubhe_event.h"
#include "dubhe_regs.h"
#include "dubhe_aca.h"
#include "dubhe_sca.h"
#include "dubhe_hash.h"
#include "dubhe_trng.h"
#include "dubhe_otp.h"
#include "dubhe_driver.h"
#include "system_hw.h"
#include <modules/pm.h>
#include "bk_misc.h"

unsigned long _g_Dubhe_RegBase;
static int do_dubhe_driver_init( unsigned long dbh_base_addr );

static void dubhe_dma_disable(void)
{
    uint32_t value = 0;

    value = DBH_READ_REGISTER( TOP_CTRL, CLK_CTRL );
    DBH_REG_FLD_SET( CLK_CTRL, DMA_AHB_EN, value, 0 );
    DBH_WRITE_REGISTER( TOP_CTRL, CLK_CTRL, value );
}

int dubhe_clk_enable( dubhe_module_type_t type )
{
    int ret        = 0;
    uint32_t value = 0;

    value = DBH_READ_REGISTER( TOP_CTRL, CLK_CTRL );
    switch ( type ) {
    case DBH_MODULE_HASH:
        DBH_REG_FLD_SET( CLK_CTRL, HASH_EN, value, 1 );
        break;
    case DBH_MODULE_SCA:
        DBH_REG_FLD_SET( CLK_CTRL, SCA_EN, value, 1 );
        break;
    case DBH_MODULE_ACA:
        DBH_REG_FLD_SET( CLK_CTRL, ACA_EN, value, 1 );
        break;
    case DBH_MODULE_OTP:
        DBH_REG_FLD_SET( CLK_CTRL, OTP_EN, value, 1 );
        break;
    case DBH_MODULE_TRNG:
        DBH_REG_FLD_SET( CLK_CTRL, TRNG_EN, value, 1 );
        break;
    default:
        PAL_LOG_ERR( "invalid dubhe module %d\n", type );
        ret = -1;
        return ret;
    }

    DBH_WRITE_REGISTER( TOP_CTRL, CLK_CTRL, value );

    return ret;
}

int dubhe_clk_disable( dubhe_module_type_t type )
{
    int ret        = 0;
    uint32_t value = 0;

    value = DBH_READ_REGISTER( TOP_CTRL, CLK_CTRL );
    switch ( type ) {
    case DBH_MODULE_HASH:
        DBH_REG_FLD_SET( CLK_CTRL, HASH_EN, value, 0 );
        break;
    case DBH_MODULE_SCA:
        DBH_REG_FLD_SET( CLK_CTRL, SCA_EN, value, 0 );
        break;
    case DBH_MODULE_ACA:
        DBH_REG_FLD_SET( CLK_CTRL, ACA_EN, value, 0 );
        break;
    case DBH_MODULE_OTP:
        DBH_REG_FLD_SET( CLK_CTRL, OTP_EN, value, 0 );
        break;
    case DBH_MODULE_TRNG:
        DBH_REG_FLD_SET( CLK_CTRL, TRNG_EN, value, 0 );
        break;
    default:
        PAL_LOG_ERR( "invalid dubhe module %d\n", type );
        ret = -1;
        return ret;
    }

    DBH_WRITE_REGISTER( TOP_CTRL, CLK_CTRL, value );

    return ret;
}

static int dubhe_lv_enter(uint64_t sleep_time, void *args)
{
    dubhe_driver_cleanup();
    return 0;
}

static int dubhe_lv_exit(uint64_t sleep_time, void *args)
{
    do_dubhe_driver_init(SOC_SHANHAI_BASE);
    return 0;
}

static void dubhe_lv_init(void)
{
    pm_cb_conf_t enter = {dubhe_lv_enter, NULL};
    pm_cb_conf_t exit = {dubhe_lv_exit, NULL};
    bk_pm_sleep_register_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_SECURE_WORLD, &enter, &exit);
}

static int do_dubhe_driver_init( unsigned long dbh_base_addr )
{
    uint32_t int_level = rtos_disable_int();

    if (sys_ll_get_cpu_power_sleep_wakeup_pwd_encp() != 0) {
        sys_ll_set_cpu_power_sleep_wakeup_pwd_encp(0);
        delay_us(100);
    }

    _g_Dubhe_RegBase = dbh_base_addr;

#if DUBHE_SECURE
    dubhe_dma_disable();
#endif

#if defined( ARM_CE_DUBHE_ACA )
    dubhe_aca_driver_init( );
#endif
#if defined( ARM_CE_DUBHE_HASH )
    dubhe_clk_enable( DBH_MODULE_HASH );
    arm_ce_hash_driver_init( );
#endif
#if defined( ARM_CE_DUBHE_SCA )
    arm_ce_sca_driver_init( );
#endif
#if defined( ARM_CE_DUBHE_TRNG )
    arm_ce_trng_driver_init( );
#endif
#if defined( ARM_CE_DUBHE_OTP )
    arm_ce_otp_driver_init( );
#endif

#if defined( DUBHE_FOR_RUNTIME )
    dubhe_event_init( );
#endif

    rtos_enable_int(int_level);
    return 0;
}

int dubhe_driver_init( unsigned long dbh_base_addr )
{
    static bool dubhe_inited = false;
    bool need_init_driver = false;
    int ret = 0;

    if (dubhe_inited == false) {
        dubhe_inited = true;
        need_init_driver = true;

    	dubhe_lv_init();
        if (sys_ll_get_cpu_power_sleep_wakeup_pwd_encp() != 0) {
            sys_ll_set_cpu_power_sleep_wakeup_pwd_encp(0);
            delay_us(500); //for power-on init, bk7239n need more delay before dubhe can work correctly
        }
    }

    if ((need_init_driver == true) || (sys_ll_get_cpu_power_sleep_wakeup_pwd_encp() != 0)) {
        ret = do_dubhe_driver_init(dbh_base_addr);
    }

    return ret;
}

void dubhe_driver_cleanup( void )
{
#if defined( ARM_CE_DUBHE_ACA )
    dubhe_aca_driver_cleanup( );
    extern void mbedtls_dubhe_cleanup( );
    mbedtls_dubhe_cleanup( );
#endif

    dubhe_event_cleanup( );
    delay_us(100);
}

/*************************** The End Of File*****************************/
