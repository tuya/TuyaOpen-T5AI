#include <common/bk_include.h>
#include <common/bk_err.h>
#if ((CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7239XX) || (CONFIG_SOC_BK7286XX)) && (CONFIG_OTP)
#include <driver/otp.h>
#elif (CONFIG_SOC_BK7256XX)
#include <driver/efuse.h>
#include "modules/chip_support.h"
#endif
#include <os/os.h>
#include <math.h>
#include <stdlib.h>

#include "temp_detect_pub.h"
#include "temp_detect.h"
#include "volt_detect.h"

#include "bk_saradc.h"
#include "sdmadc/sdmadc_driver.h"

#define TAG "sensor"
#define ADC_SWITCH_DELT                   2

//TODO: move daemon code in temp_detect.c to this file

#if (CONFIG_TEMP_DETECT || CONFIG_VOLT_DETECT)
typedef struct {
	beken_mutex_t lock;
	float         temperature;
	float         voltage;
} MCU_SENSOR_INFO;
MCU_SENSOR_INFO g_sensor_info;
#endif

#if ((CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7239XX) || (CONFIG_SOC_BK7286XX)) && (CONFIG_OTP)
static bk_err_t bk_sensor_load_adc_cali_value(void)
{
    uint16_t values[2];
    bk_err_t result;

    result = bk_otp_apb_read(OTP_GADC_CALIBRATION, (uint8_t *)&values, sizeof(values));
    if ((result != BK_OK) || (values[0] == 0) || (values[1] == 0)) {
        BK_LOGW(TAG, "uncali saradc value:[%x %x]\r\n", values[0], values[1]);
        goto LOAD_SDMADC;
    }

    BK_LOGI(TAG, "saradc low value:[%x]\r\n", values[0]);
    BK_LOGI(TAG, "saradc high value:[%x]\r\n", values[1]);
    saradc_set_calibrate_val(&values[0], SARADC_CALIBRATE_LOW);
    saradc_set_calibrate_val(&values[1], SARADC_CALIBRATE_HIGH);

LOAD_SDMADC:
    result = bk_otp_apb_read(OTP_SDMADC_CALIBRATION, (uint8_t *)&values, sizeof(values));
    if ((result != BK_OK) || (values[0] == 0) || (values[1] == 0)) {
        BK_LOGW(TAG, "uncali sdmadc value:[%x %x]\r\n", values[0], values[1]);
        goto FAILURE;
    }

    BK_LOGI(TAG, "sdmadc low value:[%x]\r\n", values[0]);
    BK_LOGI(TAG, "sdmadc high value:[%x]\r\n", values[1]);
    bk_sdmadc_set_calibrate_val(values[0], SARADC_CALIBRATE_LOW);
    bk_sdmadc_set_calibrate_val(values[1], SARADC_CALIBRATE_HIGH);

    return BK_OK;

FAILURE:
    return BK_FAIL;
}
#elif (CONFIG_SOC_BK7256XX)
static bk_err_t bk_sensor_load_adc_cali_value(void)
{
    UINT8 values[5];
    float data = 0.0;
    saradc_calibrate_val saradc_val;

    bk_efuse_read_byte(6, &values[0]);
    bk_efuse_read_byte(7, &values[1]);
    bk_efuse_read_byte(8, &values[2]);
    bk_efuse_read_byte(9, &values[3]);
    if(((values[0] == 0xFF) && (values[2] == 0xFF))
        ||((values[0] == 0x00) && (values[1] == 0x00))
        ||(((values[1] == 0xFF) && (values[3] == 0xFF)) ||((values[1] == 0x00) && (values[3] == 0x00))))
    {
        BK_LOGW(TAG, "uncali adc value efuse 6,7,8,9:[%2x %02x %02x %02x]\r\n", values[0], values[1], values[2], values[3]);
        bk_efuse_read_byte(18, &values[0]);
        bk_efuse_read_byte(19, &values[1]);
        bk_efuse_read_byte(20, &values[2]);
        bk_efuse_read_byte(21, &values[3]);
        if(((values[0] == 0xFF) && (values[2] == 0xFF))
            ||((values[0] == 0x00) && (values[1] == 0x00))
            ||(((values[1] == 0xFF) && (values[3] == 0xFF)) ||((values[1] == 0x00) && (values[3] == 0x00))))
        {
            BK_LOGW(TAG, "uncali adc value efuse 18,19,20,21:[%2x %02x %02x %02x]\r\n", values[0], values[1], values[2], values[3]);
            bk_efuse_read_byte(12, &values[0]);
            bk_efuse_read_byte(13, &values[1]);
            bk_efuse_read_byte(14, &values[2]);
            bk_efuse_read_byte(15, &values[3]);
            if(((values[0] == 0xFF) && (values[2] == 0xFF))
            ||((values[0] == 0x00) && (values[1] == 0x00))
            ||(((values[1] == 0xFF) && (values[3] == 0xFF)) ||((values[1] == 0x00) && (values[3] == 0x00))))
            {
                BK_LOGW(TAG, "uncali adc value efuse 12,13,14,15:[%2x %02x %02x %02x]\r\n", values[0], values[1], values[2], values[3]);
                goto FAILURE;
            }
            else
            {
                bk_efuse_read_byte(11, &values[4]);
                if(CHIP_VERSION_C > bk_get_hardware_chip_id_version())
                {
                    if((values[4]&0xC0) != 0x0)
                       goto FAILURE;
                }
                else
                {
                    if((values[4]&0xC0) != 0xC0)
                       goto FAILURE;
                }
            }
        }
    }

    saradc_val.low = values[1] & 0xFF;
    saradc_val.low = (saradc_val.low << 8) | values[0];

    saradc_val.high = values[3] & 0xFF;
    saradc_val.high = (saradc_val.high << 8) | values[2];
    data = (float)(saradc_val.high) / saradc_val.low;
    if(abs(data - ADC_SWITCH_DELT) <= 0.02)
    {
        g_saradc_flag = 0x1;
    }
    else
        g_saradc_flag = 0x0;

    BK_LOGI(TAG, "saradc low value:[%x]\r\n", saradc_val.low);
    BK_LOGI(TAG, "saradc high value:[%x]\r\n", saradc_val.high);
    saradc_set_calibrate_val(&saradc_val.low, SARADC_CALIBRATE_LOW);
    saradc_set_calibrate_val(&saradc_val.high, SARADC_CALIBRATE_HIGH);

    return BK_OK;

FAILURE:
    return BK_FAIL;
}
#else
static bk_err_t bk_sensor_load_adc_cali_value(void)
{
    return BK_FAIL;
}
#endif

bk_err_t bk_sensor_init(void)
{
	bk_err_t ret = BK_OK;

	bk_sensor_load_adc_cali_value();
#if (CONFIG_TEMP_DETECT || CONFIG_VOLT_DETECT)
	if (NULL == g_sensor_info.lock)
	{
		ret = rtos_init_mutex(&g_sensor_info.lock);
		g_sensor_info.voltage    = NAN;
		g_sensor_info.temperature = NAN;
	}
#endif
	return ret;
}

bk_err_t bk_sensor_deinit(void)
{
	bk_err_t ret = BK_OK;

#if (CONFIG_TEMP_DETECT || CONFIG_VOLT_DETECT)
	if (NULL != g_sensor_info.lock)
	{
		ret = rtos_deinit_mutex(&g_sensor_info.lock);
		g_sensor_info.lock       = NULL;
		g_sensor_info.voltage    = NAN;
		g_sensor_info.temperature = NAN;
	}
#endif
	return ret;
}

#if (CONFIG_TEMP_DETECT)
bk_err_t bk_sensor_set_current_temperature(float temperature)
{
	if (NULL == g_sensor_info.lock)
	{
		return BK_ERR_NOT_INIT;
	}

	rtos_lock_mutex(&g_sensor_info.lock);
	g_sensor_info.temperature = temperature;
	rtos_unlock_mutex(&g_sensor_info.lock);

	return BK_OK;
}

bk_err_t bk_sensor_get_current_temperature(float *temperature)
{
	if (NULL == g_sensor_info.lock)
	{
		return BK_ERR_NOT_INIT;
	}

	if (isnan(g_sensor_info.temperature))
	{
		return BK_ERR_TRY_AGAIN;
	}

	if (NULL == temperature)
	{
		return BK_ERR_PARAM;
	}

	rtos_lock_mutex(&g_sensor_info.lock);
	*temperature = g_sensor_info.temperature;
	rtos_unlock_mutex(&g_sensor_info.lock);

	return BK_OK;
}
#endif

#if (CONFIG_VOLT_DETECT)
bk_err_t bk_sensor_set_current_voltage(float voltage)
{
	if (NULL == g_sensor_info.lock)
	{
		return BK_ERR_NOT_INIT;
	}

	rtos_lock_mutex(&g_sensor_info.lock);
	g_sensor_info.voltage = voltage;
	rtos_unlock_mutex(&g_sensor_info.lock);

	return BK_OK;
}

bk_err_t bk_sensor_get_current_voltage(float *voltage)
{
	if (NULL == g_sensor_info.lock)
	{
		return BK_ERR_NOT_INIT;
	}

	if (isnan(g_sensor_info.voltage))
	{
		return BK_ERR_TRY_AGAIN;
	}

	if (NULL == voltage)
	{
		return BK_ERR_PARAM;
	}

	rtos_lock_mutex(&g_sensor_info.lock);
	*voltage = g_sensor_info.voltage;
	rtos_unlock_mutex(&g_sensor_info.lock);

	return BK_OK;
}

#endif
