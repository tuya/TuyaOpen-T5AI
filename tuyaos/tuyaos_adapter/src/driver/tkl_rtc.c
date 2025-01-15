 /*============================================================================
 *                                                                            *
 * Copyright (C) by Tuya Inc                                                  *
 * All rights reserved                                                        *
 *                                                                            *
 *                                                                            *
 =============================================================================*/

/*============================ INCLUDES ======================================*/
#include "tkl_rtc.h"

#include <driver/aon_rtc.h>

/*============================ MACROS ========================================*/
/*============================ TYPES =========================================*/
/*============================ PROTOTYPES ====================================*/

/*============================ LOCAL VARIABLES ===============================*/

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ IMPLEMENTATION ================================*/


#define TICK_CNT_PER_MS    (32)

OPERATE_RET tkl_rtc_init(void)
{
    bk_aon_rtc_tick_init();

    return OPRT_OK;
}


OPERATE_RET tkl_rtc_time_get(TIME_T *time_sec)
{
    uint32_t tick = 0;

    tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
    *time_sec = tick / (TICK_CNT_PER_MS * 1000) ;

    return OPRT_OK;
}

OPERATE_RET tkl_rtc_time_set(TIME_T time_sec)
{
    //uint32_t id = 0;
    //uint32_t tick = 0;
    //bool period = true;

    //tick = time_sec * TICK_CNT_PER_MS * 1000;

    bk_aon_rtc_driver_init();
//    bk_aon_rtc_create(id, tick, period);

    return OPRT_OK;
}

OPERATE_RET tkl_rtc_deinit(void)
{
    bk_aon_rtc_driver_deinit();

//    uint32_t id = 0;
//    bk_aon_rtc_destroy(id);

    return OPRT_OK;
}


