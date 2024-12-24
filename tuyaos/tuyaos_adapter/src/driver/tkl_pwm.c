#include "tkl_pwm.h"
#include <driver/pwm.h>
#include <driver/pwm_types.h>

#define __PWM_FREQ2PERIOD(x) ((unsigned int)((26000000.0/x)))

pwm_chan_t ty_to_bk_pwm(TUYA_PWM_NUM_E ch_id)
{
    pwm_chan_t pwm = PWM_ID_MAX;
    switch(ch_id) {
        case TUYA_PWM_NUM_0:
            pwm = PWM_ID_0;
        break;
        case TUYA_PWM_NUM_1:
            pwm = PWM_ID_4;
        break;
        case TUYA_PWM_NUM_2:
            pwm = PWM_ID_6;
        break;
        case TUYA_PWM_NUM_3:
            pwm = PWM_ID_8;
        break;
        case TUYA_PWM_NUM_4:
            pwm = PWM_ID_10;
        break;
        default:
        break;
    }

    return  pwm;
}

/**
 * @brief pwm init
 *
 * @param[in] ch_id: pwm channal id
 * @param[in] cfg: pwm config
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_init(TUYA_PWM_NUM_E ch_id, const TUYA_PWM_BASE_CFG_T *cfg)
{
    unsigned int period;
    unsigned int duty;
    pwm_chan_t pwm_port;
    pwm_init_config_t config = {0};

    period = __PWM_FREQ2PERIOD(cfg->frequency);
    duty = (unsigned int)(cfg->duty / 10000.0 * period);
    if(cfg->duty && duty == 0) {
        duty = 1;
    }

    config.period_cycle = period;
    config.duty_cycle = duty;
    config.duty2_cycle = 0;
    config.duty3_cycle = 0;

    pwm_port = ty_to_bk_pwm(ch_id);

    if(bk_pwm_init(pwm_port, &config))
        return OPRT_COM_ERROR;

    switch (cfg->polarity) {
        case TUYA_PWM_NEGATIVE:
            bk_pwm_set_init_signal_low(pwm_port);
            break;

        case TUYA_PWM_POSITIVE:
            bk_pwm_set_init_signal_high(pwm_port);
            break;

        default:
            return OPRT_COM_ERROR;
            break;
    }

    return OPRT_OK;
}

/**
 * @brief pwm deinit
 *
 * @param[in] ch_id: pwm channal id
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_deinit(TUYA_PWM_NUM_E ch_id)
{
    pwm_chan_t pwm_port;

    pwm_port = ty_to_bk_pwm(ch_id);
    bk_pwm_deinit(pwm_port);
    return OPRT_OK;
}

/**
 * @brief pwm start
 *
 * @param[in] ch_id: pwm channal id
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_start(TUYA_PWM_NUM_E ch_id)
{
    pwm_chan_t pwm_port;

    pwm_port = ty_to_bk_pwm(ch_id);
    bk_pwm_start(pwm_port);

    return OPRT_OK;
}

/**
 * @brief pwm stop
 *
 * @param[in] ch_id: pwm channal id
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_stop(TUYA_PWM_NUM_E ch_id)
{
    pwm_chan_t pwm_port;

    pwm_port = ty_to_bk_pwm(ch_id);
    bk_pwm_stop(pwm_port);

    return OPRT_OK;
}

/**
 * @brief set pwm info
 *
 * @param[in] ch_id: pwm channal id
 * @param[in] info: pwm info
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_info_set(TUYA_PWM_NUM_E ch_id, const TUYA_PWM_BASE_CFG_T *info)
{
    unsigned int period;
    unsigned int duty;
    pwm_chan_t pwm_port;
    pwm_period_duty_config_t config = {0};

    period = __PWM_FREQ2PERIOD(info->frequency);
    duty = (unsigned int)(info->duty / 10000.0 * period);
 
    if(info->duty && duty == 0) {
        duty = 1;
    }

    config.period_cycle = period;
    config.duty_cycle = duty;
    config.duty2_cycle = 0;
    config.duty3_cycle = 0;

    pwm_port = ty_to_bk_pwm(ch_id);

    if(bk_pwm_set_period_duty(pwm_port, &config))
        return OPRT_COM_ERROR;

    switch (info->polarity) {
        case TUYA_PWM_NEGATIVE:
            bk_pwm_set_init_signal_low(pwm_port);
            break;

        case TUYA_PWM_POSITIVE:
            bk_pwm_set_init_signal_high(pwm_port);
            break;

        default:
            return OPRT_COM_ERROR;
            break;
    }

    return OPRT_OK;
}

/**
 * @brief get pwm info
 *
 * @param[in] ch_id: pwm channal id
 * @param[out] info: pwm info
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_info_get(TUYA_PWM_NUM_E ch_id, TUYA_PWM_BASE_CFG_T *info)
{
    //pwm_chan_t pwm_port;
    //pwm_port = ty_to_bk_pwm(ch_id);
    return OPRT_NOT_SUPPORTED;
}
