// Copyright 2020-2021 Beken
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


#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <driver/touch.h>
#include <driver/touch_types.h>
#include "sys_driver.h"
#include "touch_driver.h"
#include "aon_pmu_driver.h"
#include <driver/timer.h>
#include "bk_saradc.h"
#include <driver/adc.h>


float iir_x[11][3] = {0};
float iir_y[11][2] = {0};
float iir_y_x[11][3] = {0};
float iir_y_y[11][2] = {0};
uint32_t g_gain_s = 0;
UINT8 g_num = 0;
UINT8 g_touch_capa_cali_flag = 0;
beken_timer_t touch_capa_cali_tmr = {0};
beken_timer_t touch_tmr = {0};

static beken_thread_t touch_digital_tube_disp_thread_hdl = NULL;
extern void bk_delay(int num);
extern uint32_t s_touch_channel;

static void cli_touch_help(void)
{
	TOUCH_LOGI("touch_single_channel_calib_mode_test {0|1|2|...|15} {0|1|2|3}\r\n");
	TOUCH_LOGI("touch_single_channel_manul_mode_test {0|1|...|15} {calibration_value}\r\n");
	TOUCH_LOGI("touch_multi_channel_scan_mode_test {start|stop} {0|1|2|3}\r\n");
	TOUCH_LOGI("touch_single_channel_multi_calib_test {0|1|...|15} {0|1|2|3}");
}

static void cli_touch_isr(void *param)
{
//	uint32_t int_status = 0;
//	int_status = bk_touch_get_int_status();
//	TOUCH_LOGI("interrupt status = %x\r\n", int_status);
}

static void touch_cyclic_calib_timer_isr(timer_id_t chan)
{
	uint32_t cap_out = 0;
	uint32_t touch_id = 0;
	uint32_t touch_crg[16] = {0};
	uint32_t touch_crg_max = 0;
	uint32_t multi_chann_value = 0xffff;
	touch_config_t touch_config;

	TOUCH_LOGI("multi_channel_cyclic_calib_test start!\r\n");

	bk_touch_clear_int(multi_chann_value);
	bk_touch_int_enable(multi_chann_value, 0);
	bk_touch_scan_mode_enable(0);
	bk_touch_scan_mode_multi_channl_set(0);
	bk_touch_enable(0);

	for(touch_id = 0; touch_id < 16; touch_id++)
	{
		bk_touch_enable(1 << touch_id);
	
		touch_config.sensitivity_level = g_gain_s;
		touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_6;
		touch_config.detect_range = TOUCH_DETECT_RANGE_8PF;
		bk_touch_config(&touch_config);
	
		bk_touch_calibration_start();
		cap_out = bk_touch_get_calib_value();
		if (cap_out >= 0x1F0) {
			touch_config.detect_range = TOUCH_DETECT_RANGE_12PF;
			bk_touch_config(&touch_config);
			bk_touch_calibration_start();
			cap_out = bk_touch_get_calib_value();
			if (cap_out >= 0x1F0) {
				touch_config.detect_range = TOUCH_DETECT_RANGE_19PF;
				bk_touch_config(&touch_config);
				bk_touch_calibration_start();
				cap_out = bk_touch_get_calib_value();
				if (cap_out >= 0x1F0) {
					touch_config.detect_range = TOUCH_DETECT_RANGE_27PF;
					bk_touch_config(&touch_config);
					bk_touch_calibration_start();
					cap_out = bk_touch_get_calib_value();
					if (cap_out >= 0x1F0) {
						TOUCH_LOGE("Calibration value is out of the detect range, the channel cannot be used, please select the other channel!\r\n");
						return;
					}
				}
			}
		}
		touch_crg[touch_id] = touch_config.detect_range;
		TOUCH_LOGI("touch[%d] crg = %d, calibration value = %x !\r\n", touch_id, touch_crg[touch_id], cap_out);
		bk_delay(1000);
	}

	for (touch_id = 0; touch_id < 16; touch_id++)
	{
		if (touch_crg_max < touch_crg[touch_id]) {
			touch_crg_max = touch_crg[touch_id];
		}
	}
	TOUCH_LOGI("touch_crg_max = %d\r\n", touch_crg_max);

	for (touch_id = 0; touch_id < 16; touch_id++)
	{
		if (touch_crg_max != touch_crg[touch_id]) {
			bk_touch_enable(1 << touch_id);
			touch_config.detect_range = touch_crg_max;
			bk_touch_config(&touch_config);
			bk_touch_calibration_start();
		}
	}
	bk_touch_scan_mode_multi_channl_set(multi_chann_value);
	bk_touch_scan_mode_enable(1);
	bk_touch_int_enable(multi_chann_value, 1);

}

static void touch_digital_tube_disp_main(void)
{
	bk_touch_digital_tube_init();

	while(1) {
		bk_touch_digital_tube_display(s_touch_channel);

	}
}

bk_err_t bk_touch_digital_tube_display_init(void)
{
	bk_err_t ret = BK_OK;

	ret = rtos_create_thread(&touch_digital_tube_disp_thread_hdl,
								BEKEN_DEFAULT_WORKER_PRIORITY,
								"touch_digital_tube_disp",
								(beken_thread_function_t)touch_digital_tube_disp_main,
								4096,
								NULL);
	if (ret != kNoErr) {
		os_printf("create touch digital tube disp task failed!\r\n");
		touch_digital_tube_disp_thread_hdl = NULL;
	}

	return ret;
}

void cli_touch_single_channel_calib_mode_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t touch_id = 0;
	uint32_t cap_out = 0;
	uint32_t gain_s = 0;
	touch_config_t touch_config;

	if (argc != 3) {
		cli_touch_help();
		return;
	}

	touch_id = os_strtoul(argv[1], NULL, 10) & 0xFF;
	gain_s = os_strtoul(argv[2], NULL, 10) & 0xFF;
	if(touch_id >= 0 && touch_id < 16) {
		TOUCH_LOGI("touch single channel calib mode test %d start!\r\n", touch_id);
		bk_touch_gpio_init(1 << touch_id);
		bk_touch_enable(1 << touch_id);
		bk_touch_register_touch_isr(1 << touch_id, cli_touch_isr, NULL);

		touch_config.sensitivity_level = gain_s;
		touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_6;
		touch_config.detect_range = TOUCH_DETECT_RANGE_8PF;
		bk_touch_config(&touch_config);
		#if (CONFIG_SOC_BK7236XX || CONFIG_SOC_BK7239XX || CONFIG_SOC_BK7286XX)
		bk_touch_set_test_mode(0, 0);
		bk_touch_set_calib_mode(0x3F, 9);
		#endif

		bk_touch_scan_mode_enable(0);
		bk_touch_calibration_start();
		cap_out = bk_touch_get_calib_value();
		TOUCH_LOGI("cap_out0 = %x\r\n", cap_out);
		if (cap_out >= 0x1F0) {
			touch_config.detect_range = TOUCH_DETECT_RANGE_12PF;
			bk_touch_config(&touch_config);
			bk_touch_calibration_start();
			cap_out = bk_touch_get_calib_value();
			TOUCH_LOGI("cap_out1 = %x\r\n", cap_out);
			if (cap_out >= 0x1F0) {
				touch_config.detect_range = TOUCH_DETECT_RANGE_19PF;
				bk_touch_config(&touch_config);
				bk_touch_calibration_start();
				cap_out = bk_touch_get_calib_value();
				TOUCH_LOGI("cap_out2 = %x\r\n", cap_out);
				if (cap_out >= 0x1F0) {
					touch_config.detect_range = TOUCH_DETECT_RANGE_27PF;
					bk_touch_config(&touch_config);
					bk_touch_calibration_start();
					cap_out = bk_touch_get_calib_value();
					TOUCH_LOGI("cap_out3 = %x\r\n", cap_out);
					if (cap_out >= 0x1F0) {
						TOUCH_LOGE("Calibration value is out of the detect range, the channel cannot be used, please select the other channel!\r\n");
						return;
					}
				}
			}
		}
		bk_touch_int_enable(1 << touch_id, 1);
	} else {
		TOUCH_LOGE("unsupported touch channel selection command!\r\n");
	}
}

void cli_touch_single_channel_manul_mode_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t calib_value = 0;
	uint32_t cap_out = 0;
	uint32_t touch_id = 0;
	touch_config_t touch_config;

	if (argc != 3) {
		cli_touch_help();
		return;
	}

	touch_id = os_strtoul(argv[1], NULL, 16) & 0xFF;
	if(touch_id >= 0 && touch_id < 16) {
		TOUCH_LOGI("touch single channel manul mode test %d start!\r\n", touch_id);
		bk_touch_gpio_init(1 << touch_id);
		bk_touch_enable(1 <<touch_id);
		bk_touch_register_touch_isr(1 << touch_id, cli_touch_isr, NULL);

		touch_config.sensitivity_level = TOUCH_SENSITIVITY_LEVLE_3;
		touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_6;
		touch_config.detect_range = TOUCH_DETECT_RANGE_27PF;
		bk_touch_config(&touch_config);
		bk_touch_scan_mode_enable(0);

		calib_value = os_strtoul(argv[2], NULL, 16) & 0xFFF;
		TOUCH_LOGI("calib_value = %x\r\n", calib_value);
		bk_touch_manul_mode_enable(calib_value);
		bk_touch_int_enable(1 << touch_id, 1);
		cap_out = bk_touch_get_calib_value();
		TOUCH_LOGI("cap_out = %x\r\n", cap_out);
		if(calib_value == cap_out) {
			TOUCH_LOGI("single channel manul mode test is successful!\r\n");
		} else {
			TOUCH_LOGE("single channel manul mode test is failed!\r\n");
			TOUCH_LOGE("please input larger calibration value!\r\n");
			bk_touch_manul_mode_disable();
			bk_touch_disable();
		}
	} else {
		TOUCH_LOGE("unsupported touch channel selection command!\r\n");
	} 
}

void cli_touch_multi_channel_scan_mode_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;

	uint32_t multi_chann_value = 0xCF3F;
	uint32_t touch_crg[16] = {0};
	uint32_t touch_crg_max = 0;
	uint32_t touch_id = 0;
	uint32_t cap_out = 0;
	uint32_t gain_s = 0;
	touch_config_t touch_config;

	if (argc != 3) {
		cli_touch_help();
		return;
	}

	ret = bk_touch_digital_tube_display_init();
	if (ret != BK_OK) {
		os_printf("init touch digital tube display task failed!\r\n");
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		TOUCH_LOGI("multi_channel_scan_mode_test start!\r\n");
		gain_s = os_strtoul(argv[2], NULL, 10) & 0xFF;
		for(touch_id = 0; touch_id < 16; touch_id++)
		{
			if (touch_id == 6 || touch_id == 7 || touch_id == 13 || touch_id == 12) {
				continue;
			}
			bk_touch_gpio_init(1 << touch_id);
			bk_touch_enable(1 << touch_id);
			bk_touch_register_touch_isr(1 << touch_id, cli_touch_isr, NULL);
	
			touch_config.sensitivity_level = gain_s;
			touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_6;
			touch_config.detect_range = TOUCH_DETECT_RANGE_8PF;
			bk_touch_config(&touch_config);
	
			bk_touch_scan_mode_enable(0);
			bk_touch_calibration_start();
			cap_out = bk_touch_get_calib_value();
			if (cap_out >= 0x1F0) {
				touch_config.detect_range = TOUCH_DETECT_RANGE_12PF;
				bk_touch_config(&touch_config);
				bk_touch_calibration_start();
				cap_out = bk_touch_get_calib_value();
				if (cap_out >= 0x1F0) {
					touch_config.detect_range = TOUCH_DETECT_RANGE_19PF;
					bk_touch_config(&touch_config);
					bk_touch_calibration_start();
					cap_out = bk_touch_get_calib_value();
					if (cap_out >= 0x1F0) {
						touch_config.detect_range = TOUCH_DETECT_RANGE_27PF;
						bk_touch_config(&touch_config);
						bk_touch_calibration_start();
						cap_out = bk_touch_get_calib_value();
						if (cap_out >= 0x1F0) {
							TOUCH_LOGE("Calibration value is out of the detect range, the channel cannot be used, please select the other channel!\r\n");
							return;
						}
					}
				}
			}
			touch_crg[touch_id] = touch_config.detect_range;
			TOUCH_LOGI("touch[%d] crg = %d, calibration value = %x !\r\n", touch_id, touch_crg[touch_id], cap_out);
			bk_delay(1000);
		}

		for (touch_id = 0; touch_id < 16; touch_id++)
		{
			if (touch_id == 6 || touch_id == 7 || touch_id == 13 || touch_id == 12) {
				continue;
			}

			if (touch_crg_max < touch_crg[touch_id]) {
				touch_crg_max = touch_crg[touch_id];
			}
		}
		TOUCH_LOGI("touch_crg_max = %d\r\n", touch_crg_max);

		for (touch_id = 0; touch_id < 16; touch_id++)
		{
			if (touch_id == 6 || touch_id == 7 || touch_id == 13 || touch_id == 12) {
				continue;
			}

			if (touch_crg_max != touch_crg[touch_id]) {
				bk_touch_enable(1 << touch_id);
				touch_config.detect_range = touch_crg_max;
				bk_touch_config(&touch_config);
				bk_touch_calibration_start();
			}
		}

		bk_touch_scan_mode_multi_channl_set(multi_chann_value);
		bk_touch_scan_mode_enable(1);
		bk_touch_int_enable(multi_chann_value, 1);
	} else if (os_strcmp(argv[1], "stop") == 0) {
		TOUCH_LOGI("multi_channel_scan_mode_test stop!\r\n");
		bk_touch_scan_mode_enable(0);
		bk_touch_disable();
	}
}

void cli_touch_single_channel_multi_calib_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t touch_id = 0;
	uint32_t cap_out = 0;
	uint32_t gain_s = 0;
	uint32_t count = 0;
	touch_config_t touch_config;

	if (argc != 3) {
		cli_touch_help();
		return;
	}

	touch_id = os_strtoul(argv[1], NULL, 10) & 0xFF;
	gain_s = os_strtoul(argv[2], NULL, 10) & 0xFF;
	if(touch_id >= 0 && touch_id < 16) {
		TOUCH_LOGI("touch single channel calib mode test %d start!\r\n", touch_id);
		bk_touch_gpio_init(1 << touch_id);
		bk_touch_enable(1 << touch_id);
		bk_touch_register_touch_isr(1 << touch_id, cli_touch_isr, NULL);

		touch_config.sensitivity_level = gain_s;
		touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_6;
		touch_config.detect_range = TOUCH_DETECT_RANGE_8PF;
		bk_touch_config(&touch_config);

		bk_touch_scan_mode_enable(0);
		bk_touch_calibration_start();
		cap_out = bk_touch_get_calib_value();
		TOUCH_LOGI("cap_out0 = %x\r\n", cap_out);
		if (cap_out >= 0x1F0) {
			touch_config.detect_range = TOUCH_DETECT_RANGE_12PF;
			bk_touch_config(&touch_config);
			bk_touch_calibration_start();
			cap_out = bk_touch_get_calib_value();
			TOUCH_LOGI("cap_out1 = %x\r\n", cap_out);
			if (cap_out >= 0x1F0) {
				touch_config.detect_range = TOUCH_DETECT_RANGE_19PF;
				bk_touch_config(&touch_config);
				bk_touch_calibration_start();
				cap_out = bk_touch_get_calib_value();
				TOUCH_LOGI("cap_out2 = %x\r\n", cap_out);
				if (cap_out >= 0x1F0) {
					touch_config.detect_range = TOUCH_DETECT_RANGE_27PF;
					bk_touch_config(&touch_config);
					bk_touch_calibration_start();
					cap_out = bk_touch_get_calib_value();
					TOUCH_LOGI("cap_out3 = %x\r\n", cap_out);
					if (cap_out >= 0x1F0) {
						TOUCH_LOGE("Calibration value is out of the detect range, the channel cannot be used, please select the other channel!\r\n");
						return;
					}
				}
			}
		}

		for (count = 0; count < 5000; count++)
		{
			bk_touch_calibration_start();
			cap_out = bk_touch_get_calib_value();
			TOUCH_LOGI("cap_out = %x\r\n", cap_out);
			bk_delay(10000);
		}

		bk_touch_int_enable(1 << touch_id, 1);
	} else {
		TOUCH_LOGE("unsupported touch channel selection command!\r\n");
	}
}

void cli_touch_multi_channel_cyclic_calib_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret = 0;
	uint32_t multi_chann_value = 0xffff;
	uint32_t touch_crg[16] = {0};
	uint32_t touch_crg_max = 0;
	uint32_t touch_id = 0;
	uint32_t cap_out = 0;
	touch_config_t touch_config;

	if (argc != 3) {
		cli_touch_help();
		return;
	}
	
	if (os_strcmp(argv[1], "start") == 0) {
		g_gain_s = os_strtoul(argv[2], NULL, 10) & 0xFF;
		for(touch_id = 0; touch_id < 16; touch_id++)
		{
			bk_touch_gpio_init(1 << touch_id);
			bk_touch_enable(1 << touch_id);
			bk_touch_register_touch_isr(1 << touch_id, cli_touch_isr, NULL);
	
			touch_config.sensitivity_level = g_gain_s;
			touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_6;
			touch_config.detect_range = TOUCH_DETECT_RANGE_8PF;
			bk_touch_config(&touch_config);
	
			bk_touch_scan_mode_enable(0);
			bk_touch_calibration_start();
			cap_out = bk_touch_get_calib_value();
			if (cap_out >= 0x1F0) {
				touch_config.detect_range = TOUCH_DETECT_RANGE_12PF;
				bk_touch_config(&touch_config);
				bk_touch_calibration_start();
				cap_out = bk_touch_get_calib_value();
				if (cap_out >= 0x1F0) {
					touch_config.detect_range = TOUCH_DETECT_RANGE_19PF;
					bk_touch_config(&touch_config);
					bk_touch_calibration_start();
					cap_out = bk_touch_get_calib_value();
					if (cap_out >= 0x1F0) {
						touch_config.detect_range = TOUCH_DETECT_RANGE_27PF;
						bk_touch_config(&touch_config);
						bk_touch_calibration_start();
						cap_out = bk_touch_get_calib_value();
						if (cap_out >= 0x1F0) {
							TOUCH_LOGE("Calibration value is out of the detect range, the channel cannot be used, please select the other channel!\r\n");
							return;
						}
					}
				}
			}
			touch_crg[touch_id] = touch_config.detect_range;
			TOUCH_LOGI("touch[%d] crg = %d, calibration value = %x !\r\n", touch_id, touch_crg[touch_id], cap_out);
			bk_delay(1000);
		}

		for (touch_id = 0; touch_id < 16; touch_id++)
		{
			if (touch_crg_max < touch_crg[touch_id]) {
				touch_crg_max = touch_crg[touch_id];
			}
		}
		TOUCH_LOGI("touch_crg_max = %d\r\n", touch_crg_max);

		for (touch_id = 0; touch_id < 16; touch_id++)
		{
			if (touch_crg_max != touch_crg[touch_id]) {
				bk_touch_enable(1 << touch_id);
				touch_config.detect_range = touch_crg_max;
				bk_touch_config(&touch_config);
				bk_touch_calibration_start();
			}
		}

		bk_touch_scan_mode_multi_channl_set(multi_chann_value);
		bk_touch_scan_mode_enable(1);
		bk_touch_int_enable(multi_chann_value, 1);


		ret = bk_timer_start(TIMER_ID1, 10000, touch_cyclic_calib_timer_isr);
		if (ret != BK_OK) {
			os_printf("Timer start failed\r\n");
		}
	}
}

#if CONFIG_SOC_BK7256XX
void touch_push(float *data_buff, float data, UINT8 num)
{
    UINT8 i;

    for(i=0;i<num;i++)
    {
        data_buff[i] = data_buff[i+1];
    }
    data_buff[num] = data;
}

void touch_saradc_iir_iir_fillter(UINT8 chan_idx)
{
    UINT16 value = 0;
    UINT32 sum = 0;
    float touch_delt[11] = {-300,-300,-300,-300,-300,-300,-300,-300,-300,-300,-300};
    uint32_t chan[11] = {2,3,4,5,8,9,10,11,12,14,15};
    float x=0.0,y1=0.0,y2=0.0,b1=1.0,b2=0.0,b3=-1.0,a1=1.0,a2=-1.8945,a3=0.9037,a4=-1.5515,a5=0.6755,s1=0.1588,s2=0.1588;

    aon_pmu_drv_touch_select(chan[chan_idx]);
    for(UINT8 i = 0; i<= 1; i++)
    {
        BK_LOG_ON_ERR(bk_adc_read(&value, ADC_READ_SEMAPHORE_WAIT_TIME));
        value = value << 1;
        sum += value;
    }

    x = ((float)sum)/2;
    //IIR1
    touch_push(iir_x[chan_idx], x, 2);
    y1 = (((b1*iir_x[chan_idx][2] + b2*iir_x[chan_idx][1] + b3*iir_x[chan_idx][0]) - a2 * iir_y[chan_idx][1] - a3 * iir_y[chan_idx][0])/a1);

    touch_push(iir_y[chan_idx], y1, 1);

    //IIR2
    y1 = y1*s1;
    touch_push(iir_y_x[chan_idx], y1, 2);
    y2 = (((b1*iir_y_x[chan_idx][2] + b2*iir_y_x[chan_idx][1] + b3*iir_y_x[chan_idx][0]) - a4 * iir_y_y[chan_idx][1] - a5 * iir_y_y[chan_idx][0])/a1);

    touch_push(iir_y_y[chan_idx], y2, 1);
    y2 = y2*s2;
    //TOUCH_LOGI("y2=%f,touch_chan=%d,num=%d\r\n",y2,chan_idx,num);

    //initial oscilation
    if(g_num >= 100)
    {
        //The y2 is negative
        if ((y2 < touch_delt[chan_idx]))
        {
            s_touch_channel = chan[chan_idx];
        }
    }
    rtos_delay_milliseconds(2);
}

void touch_capa_cali(void *param)
{
    uint32_t touch_chan = 2;
    UINT32 cap_out      = 0;
    int ret             = 0;
    uint32_t chan[11]   = {2,3,4,5,8,9,10,11,12,14,15};
    UINT8 j             = 0;
    touch_config_t touch_config;

    if(g_touch_capa_cali_flag == 0)
    {
        g_touch_capa_cali_flag = 1;
        for(j = 0; j < 11; j++)
        {
            touch_chan = chan[j];
            bk_touch_gpio_init(1 << touch_chan);
            bk_touch_enable(1 << touch_chan);
            bk_touch_scan_mode_enable(0);

            touch_config.sensitivity_level = TOUCH_SENSITIVITY_LEVLE_0;
            touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_6;
            touch_config.detect_range = TOUCH_DETECT_RANGE_8PF;
            bk_touch_config(&touch_config);

            bk_touch_calibration_start();
            cap_out = bk_touch_get_calib_value();
            //TOUCH_LOGI("cap_out=%d,touch_chan=%d\r\n",cap_out,chan[j]);
            bk_touch_manul_mode_enable(cap_out);
            bk_touch_manul_mode_disable();
        }
        if (touch_capa_cali_tmr.handle != NULL)
        {
            ret = rtos_reload_timer(&touch_capa_cali_tmr);
            BK_ASSERT(kNoErr == ret);
        }
        g_touch_capa_cali_flag = 0;
    }
    else
    {
        TOUCH_LOGI("touch saradc task executing!\r\n");
    }
}

void touch_adc_get(void *param)
{
    UINT8 j;
    int err;

    g_num += 1;
    if(g_touch_capa_cali_flag == 0)
    {
        g_touch_capa_cali_flag = 2;
        BK_LOG_ON_ERR(bk_adc_acquire());
        sys_drv_set_ana_pwd_gadc_buf(0);
        BK_LOG_ON_ERR(bk_adc_init(ADC_9));
        adc_config_t config = {0};

        config.chan = ADC_9;
        config.adc_mode = 3;
        config.src_clk = 1;
        config.clk = 0x31975;
        config.saturate_mode = 4;
        config.steady_ctrl= 7;
        config.adc_filter = 0;
        if(config.adc_mode == ADC_CONTINUOUS_MODE)
        {
            config.sample_rate = 0;
        }

        BK_LOG_ON_ERR(bk_adc_set_config(&config));
        BK_LOG_ON_ERR(bk_adc_enable_bypass_clalibration());
        BK_LOG_ON_ERR(bk_adc_start());

        for(j = 0; j < 11; j++)
        {
            touch_saradc_iir_iir_fillter(j);
        }
        if(g_num >= 100)
            g_num -= 1;
        bk_adc_stop();
        bk_adc_deinit(ADC_9);
        bk_adc_release();
        if (touch_tmr.handle != NULL)
        {
            err = rtos_reload_timer(&touch_tmr);
            BK_ASSERT(kNoErr == err);
        }
        g_touch_capa_cali_flag = 0;
    }
}

void cli_touch_adc_mode_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (os_strcmp(argv[1], "cali") == 0)
    {
        UINT32 t_ms = 5000;
        int err;

        if (touch_capa_cali_tmr.handle != NULL)
        {
            err = rtos_deinit_timer(&touch_capa_cali_tmr);
            BK_ASSERT(kNoErr == err);
            touch_capa_cali_tmr.handle = NULL;
        }

        err = rtos_init_timer(&touch_capa_cali_tmr,
                              t_ms,
                              touch_capa_cali,
                              (void *)0);
        BK_ASSERT(kNoErr == err);
        err = rtos_start_timer(&touch_capa_cali_tmr);
        BK_ASSERT(kNoErr == err);

        //enable LED
        err = bk_touch_digital_tube_display_init();
        if (err != BK_OK)
        {
            TOUCH_LOGI("init touch digital tube display task failed!\r\n");
            return;
        }
    }
    else if(os_strcmp(argv[1], "test") == 0)
    {
        UINT32 t_ms = 100;
        int err;

        if (touch_tmr.handle != NULL)
        {
            err = rtos_deinit_timer(&touch_tmr);
            BK_ASSERT(kNoErr == err);
            touch_tmr.handle = NULL;
        }

        err = rtos_init_timer(&touch_tmr,
                              t_ms,
                              touch_adc_get,
                              (void *)0);
        BK_ASSERT(kNoErr == err);
        err = rtos_start_timer(&touch_tmr);
        BK_ASSERT(kNoErr == err);
    }
}
#endif

