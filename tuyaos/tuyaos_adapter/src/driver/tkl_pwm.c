#include "tkl_pwm.h"
#include <driver/pwm.h>
#include <driver/pwm_types.h>

#define TUYA_PWM_ID_MAX 5
#define __PWM_FREQ2PERIOD(x) ((unsigned int)((26000000.0/x)))

static pwm_group_t pwm_group = PWM_GROUP_ID_INVALID;
static TUYA_PWM_BASE_CFG_T pwm_cfg[TUYA_PWM_ID_MAX] = {{0}};
static TUYA_PWM_NUM_E mult_chid[2] = {0};

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

    if(ch_id >= TUYA_PWM_ID_MAX) {
        return OPRT_COM_ERROR;
    }

    memcpy(&pwm_cfg[ch_id], cfg, sizeof(TUYA_PWM_BASE_CFG_T));
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

    if(ch_id >= TUYA_PWM_ID_MAX) {
        return OPRT_COM_ERROR;
    }

    pwm_port = ty_to_bk_pwm(ch_id);
    bk_pwm_deinit(pwm_port);

    memset(&pwm_cfg[ch_id], 0, sizeof(TUYA_PWM_BASE_CFG_T));
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

    if(ch_id >= TUYA_PWM_ID_MAX) {
        return OPRT_COM_ERROR;
    }

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

    if(ch_id >= TUYA_PWM_ID_MAX) {
        return OPRT_COM_ERROR;
    }

    pwm_port = ty_to_bk_pwm(ch_id);
    bk_pwm_stop(pwm_port);

    return OPRT_OK;
}

/**
 * @brief pwm duty set
 *
 * @param[in] ch_id: pwm channal id, id index starts at 0
 * @param[in] duty:  pwm duty cycle
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_duty_set(TUYA_PWM_NUM_E ch_id, uint32_t duty)
{
    unsigned int period;
    unsigned int curr_duty;
    pwm_chan_t pwm_port;
    pwm_period_duty_config_t config = {0};

    if(ch_id >= TUYA_PWM_ID_MAX) {
        return OPRT_COM_ERROR;
    }

    if(duty == pwm_cfg[ch_id].duty) {
        return OPRT_OK;
    }

    period = __PWM_FREQ2PERIOD(pwm_cfg[ch_id].frequency);
    curr_duty = (unsigned int)(duty / 10000.0 * period);
    if(pwm_cfg[ch_id].duty && curr_duty == 0) {
        curr_duty = 1;
    }

    config.period_cycle = period;
    config.duty_cycle = curr_duty;
    config.duty2_cycle = 0;
    config.duty3_cycle = 0;

    pwm_port = ty_to_bk_pwm(ch_id);
    if(bk_pwm_set_period_duty(pwm_port, &config)) {
        return OPRT_COM_ERROR;
    }

    pwm_cfg[ch_id].duty = duty;

    return OPRT_OK;
}

/**
 * @brief pwm frequency set
 *
 * @param[in] ch_id: pwm channal id, id index starts at 0
 * @param[in] frequency: pwm frequency
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_frequency_set(TUYA_PWM_NUM_E ch_id, uint32_t frequency)
{
    unsigned int period;
    unsigned int curr_duty;
    pwm_chan_t pwm_port;
    pwm_period_duty_config_t config = {0};

    if(ch_id >= TUYA_PWM_ID_MAX) {
        return OPRT_COM_ERROR;
    }

    if(frequency == pwm_cfg[ch_id].frequency) {
        return OPRT_OK;
    }

    period = __PWM_FREQ2PERIOD(frequency);
    curr_duty = (unsigned int)(pwm_cfg[ch_id].duty / 10000.0 * period);
    if(pwm_cfg[ch_id].duty && curr_duty == 0) {
        curr_duty = 1;
    }

    config.period_cycle = period;
    config.duty_cycle = curr_duty;
    config.duty2_cycle = 0;
    config.duty3_cycle = 0;

    pwm_port = ty_to_bk_pwm(ch_id);
    if(bk_pwm_set_period_duty(pwm_port, &config)) {
        return OPRT_COM_ERROR;
    }

    pwm_cfg[ch_id].frequency = frequency;

    return OPRT_OK;
}

/**
 * @brief pwm polarity set
 *
 * @param[in] ch_id: pwm channal id, id index starts at 0
 * @param[in] polarity: pwm polarity
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_polarity_set(TUYA_PWM_NUM_E ch_id, TUYA_PWM_POLARITY_E polarity)
{
    pwm_chan_t pwm_port;

    if (ch_id >= TUYA_PWM_ID_MAX) {
        return OPRT_INVALID_PARM;
    }

    pwm_port = ty_to_bk_pwm(ch_id);
    switch (polarity) {
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

    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief multiple pwm channel start
 *
 * @param[in] ch_id: pwm channal id list
 * @param[in] num  : num of pwm channal to start
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_multichannel_start(TUYA_PWM_NUM_E *ch_id, uint8_t num)
{
    unsigned int period;
    unsigned int duty[2] = {0};

    if(num != 2) {
        bk_printf("pwm param num is not 2 !!!");
        return OPRT_COM_ERROR;
    }

    if (ch_id[0] >= TUYA_PWM_ID_MAX || ch_id[1] >= TUYA_PWM_ID_MAX) {
        return OPRT_INVALID_PARM;
    }

    if(PWM_GROUP_ID_INVALID == pwm_group) {
        pwm_group_init_config_t group_init_cfg;

        mult_chid[0] = ch_id[0];
        mult_chid[1] = ch_id[1];

        if(pwm_cfg[ch_id[0]].frequency != pwm_cfg[ch_id[1]].frequency) {
            bk_printf("frequency is diff !!!");
            return OPRT_COM_ERROR;
        }
        period = __PWM_FREQ2PERIOD(pwm_cfg[ch_id[0]].frequency);
        duty[0] = (unsigned int)(pwm_cfg[ch_id[0]].duty / 10000.0 * period);
        if(pwm_cfg[ch_id[0]].duty && duty[0] == 0) {
            duty[0] = 1;
        }
        duty[1] = (unsigned int)(pwm_cfg[ch_id[1]].duty / 10000.0 * period);
        if(pwm_cfg[ch_id[1]].duty && duty[1] == 0) {
            duty[1] = 1;
        }

        group_init_cfg.chan1 = ty_to_bk_pwm(ch_id[0]);
        group_init_cfg.chan2 = ty_to_bk_pwm(ch_id[1]);
        group_init_cfg.period_cycle = period;
        group_init_cfg.chan1_duty_cycle = duty[0];
        group_init_cfg.chan2_duty_cycle = duty[1];
        //bk_printf("chan1:%d, chan2:%d, period_cycle:%d, chan1_duty_cycle:%d, chan2_duty_cycle:%d\r\n",
        //    group_init_cfg.chan1, group_init_cfg.chan2, group_init_cfg.period_cycle, group_init_cfg.chan1_duty_cycle, group_init_cfg.chan2_duty_cycle);

        bk_pwm_group_init(&group_init_cfg, &pwm_group);
        bk_pwm_group_start(pwm_group);
    } else {
        pwm_group_config_t group_cfg;

        if(pwm_cfg[ch_id[0]].frequency != pwm_cfg[ch_id[1]].frequency) {
            bk_printf("frequency is diff !!!");
            return OPRT_COM_ERROR;
        }
        period = __PWM_FREQ2PERIOD(pwm_cfg[ch_id[0]].frequency);
        duty[0] = (unsigned int)(pwm_cfg[ch_id[0]].duty / 10000.0 * period);
        if(pwm_cfg[ch_id[0]].duty && duty[0] == 0) {
            duty[0] = 1;
        }
        duty[1] = (unsigned int)(pwm_cfg[ch_id[1]].duty / 10000.0 * period);
        if(pwm_cfg[ch_id[1]].duty && duty[1] == 0) {
            duty[1] = 1;
        }

        group_cfg.period_cycle = period;
        group_cfg.chan1_duty_cycle = duty[0];
        group_cfg.chan2_duty_cycle = duty[1];
        bk_pwm_group_set_config(pwm_group, &group_cfg);
    }

    return OPRT_OK;
}

/**
 * @brief multiple pwm channel stop
 *
 * @param[in] ch_id: pwm channal id list
 * @param[in] num  : num of pwm channal to stop
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_multichannel_stop(TUYA_PWM_NUM_E *ch_id, uint8_t num)
{
    if (ch_id[0] != mult_chid[0] || ch_id[1] != mult_chid[1]) {
        return OPRT_INVALID_PARM;
    }

    bk_pwm_group_stop(pwm_group);

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

    if (ch_id >= TUYA_PWM_ID_MAX) {
        return OPRT_INVALID_PARM;
    }

    memcpy(&pwm_cfg[ch_id], info, sizeof(TUYA_PWM_BASE_CFG_T));
    if(PWM_GROUP_ID_INVALID != pwm_group) {
        if(ch_id == mult_chid[0] || ch_id == mult_chid[1]) {
            return  OPRT_OK;
        }
    }

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
    if (ch_id >= TUYA_PWM_ID_MAX) {
        return OPRT_INVALID_PARM;
    }

    info->duty = pwm_cfg[ch_id].duty;
    info->frequency = pwm_cfg[ch_id].frequency;
    info->polarity = pwm_cfg[ch_id].polarity;
    return OPRT_NOT_SUPPORTED;
}
