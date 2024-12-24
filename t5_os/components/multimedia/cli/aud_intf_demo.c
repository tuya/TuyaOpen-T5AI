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

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <stdio.h>
#include <stdlib.h>
#include "sys_driver.h"
//#include "aud_driver.h"
#include "aud_intf.h"
#include "aud_intf_types.h"
#if CONFIG_FATFS
#include "ff.h"
#include "diskio.h"
#endif
#include <driver/uart.h>
#include "gpio_driver.h"
#include <modules/g711.h>

#include "cli.h"
#include "media_cli.h"

//#include "aud_debug_tcp.h"

//#define UART_REPLACE_SDCARD_DEBUG

/* 8K mono, at least 320 bytes(>= one frame size) */
static const uint16_t in_data[] = {
	0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
	0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
	0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
	0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
	0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
	0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
	0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
	0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
	0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
	0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
	0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
	0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
	0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
	0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
	0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
	0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
	0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
	0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
	0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
	0x0000, 0x5A81, 0x7FFF, 0x5A83, 0x0000, 0xA57E, 0x8002, 0xA57E,
};

#if CONFIG_FATFS
static FIL spk_file;
#ifndef UART_REPLACE_SDCARD_DEBUG
static FIL mic_file;
static char mic_file_name[50];
#endif

static FIL voc_red_file;
static FIL voc_ply_file;
static char spk_file_name[50];
static char voc_record_file_name[50];
static char voc_play_file_name[50];
static int32_t *temp_spk_addr = NULL;     //存放从SDcard中取的pcm信号
static bool spk_file_empty = false;
#endif

static uint32_t const_data_count = 0;

extern void bk_delay(int num);

static void cli_aud_intf_help(void)
{
#if CONFIG_FATFS
	os_printf("aud_intf_record_test {init|start|pause|stop|set_chl|deinit xxx.pcm|mic1|dual} \r\n");
	os_printf("aud_intf_play_test {init|start|pause|stop|set_chl|deinit xxx.pcm|left|dual} \r\n");
	os_printf("aud_intf_sd_voc_test {start|stop xx.pcm, xx.pcm} \r\n");
#endif
	os_printf("aud_intf_set_voc_param_test {param value} \r\n");
	os_printf("aud_intf_set_aec_param_test {param value} \r\n");
	os_printf("aud_intf_get_aec_param_test \r\n");
	os_printf("aud_intf_set_samp_rate_test {param value} \r\n");
	os_printf("aud_intf_doorbell_test {start|stop} \r\n");
	os_printf("aud_intf_aud_debug_test {aud_debug|tx|rx|aec on|off} \r\n");
	os_printf("aud_intf_loop_test {start|stop} \r\n");
	os_printf("aud_intf_play_const_data_test {start|stop} \r\n");
}

#ifdef UART_REPLACE_SDCARD_DEBUG
static void uart_dump_mic_data(uart_id_t id, uint32_t baud_rate)
{
	uart_config_t config = {0};
	os_memset(&config, 0, sizeof(uart_config_t));
	if (id == 0) {
		gpio_dev_unmap(GPIO_10);
		gpio_dev_map(GPIO_10, GPIO_DEV_UART1_RXD);
		gpio_dev_unmap(GPIO_11);
		gpio_dev_map(GPIO_11, GPIO_DEV_UART1_TXD);
	} else if (id == 2) {
		gpio_dev_unmap(GPIO_40);
		gpio_dev_map(GPIO_40, GPIO_DEV_UART3_RXD);
		gpio_dev_unmap(GPIO_41);
		gpio_dev_map(GPIO_41, GPIO_DEV_UART3_TXD);
	} else {
		gpio_dev_unmap(GPIO_0);
		gpio_dev_map(GPIO_0, GPIO_DEV_UART2_TXD);
		gpio_dev_unmap(GPIO_1);
		gpio_dev_map(GPIO_1, GPIO_DEV_UART2_RXD);
	}

	config.baud_rate = baud_rate;
	config.data_bits = UART_DATA_8_BITS;
	config.parity = UART_PARITY_NONE;
	config.stop_bits = UART_STOP_BITS_1;
	config.flow_ctrl = UART_FLOWCTRL_DISABLE;
	config.src_clk = UART_SCLK_XTAL_26M;

	if (bk_uart_init(id, &config) != BK_OK) {
		os_printf("init uart fail \r\n");
	} else {
		os_printf("init uart ok \r\n");
	}
}
#endif

static bk_err_t read_spk_data_from_const_data(unsigned int size)
{
	bk_err_t ret = BK_OK;

	uint32_t remain = sizeof(in_data) - const_data_count;
	if (remain >= size) {
		/* write a fram speaker data to speaker_ring_buff */
		ret = bk_aud_intf_write_spk_data((uint8_t *)&in_data[const_data_count/2], size);
		const_data_count += size;
		if (const_data_count == sizeof(in_data)) {
			const_data_count = 0;
		}
		if (ret != BK_OK) {
			os_printf("write spk data fail \r\n");
			return ret;
		}
	} else {
		uint8_t *temp_data = (uint8_t *)os_malloc(size);
		if (!temp_data) {
			/* skip tail data */
			/* write a fram speaker data to speaker_ring_buff */
			ret = bk_aud_intf_write_spk_data((uint8_t *)in_data, size);
			const_data_count = size;
		} else {
			os_memcpy(temp_data, (uint8_t *)&in_data[const_data_count/2], remain);
			os_memcpy(temp_data + remain, (uint8_t *)in_data, size - remain);
			const_data_count = size - remain;
			ret = bk_aud_intf_write_spk_data((uint8_t *)temp_data, size);
			os_free(temp_data);
			temp_data = NULL;
		}
		if (ret != BK_OK) {
			os_printf("write spk data fail \r\n");
			return ret;
		}
	}

	return ret;
}

#if 0
static void uac_connect_state_cb_handle(uint8_t state)
{
	os_printf("[--%s--] state: %d \n", __func__, state);
}
#endif

static int send_mic_data_to_spk(uint8_t *data, unsigned int len)
{
	bk_err_t ret = BK_OK;

	int16_t *pcm_data = (int16_t *)data;
	uint8_t *g711a_data = NULL;
	g711a_data = os_malloc(len/2);

	/* pcm -> g711a */
	for (int i = 0; i < len/2; i++) {
		g711a_data[i] = linear2alaw(pcm_data[i]);
	}

	/* g711a -> pcm */
	for (int i = 0; i< len/2; i++) {
		pcm_data[i] = alaw2linear(g711a_data[i]);
	}

	/* write a fram speaker data to speaker_ring_buff */
	ret = bk_aud_intf_write_spk_data((uint8 *)pcm_data, len);
	if (ret != BK_OK) {
		os_printf("write mic spk data fail \r\n");
		return ret;
	}

	os_free(g711a_data);
	return len;
}


#if CONFIG_FATFS
static int send_mic_data_to_sd(uint8_t *data, unsigned int len)
{

#ifdef UART_REPLACE_SDCARD_DEBUG
	bk_uart_write_bytes(UART_ID_1, data, len);
	return len;
#else
	FRESULT fr;
	uint32 uiTemp = 0;

	//os_printf("%s, data: %p, len: %d \n", __func__, data, len);

	/* write data to file */
	fr = f_write(&mic_file, (void *)data, len, &uiTemp);
	if (fr != FR_OK) {
		os_printf("write %s fail.\r\n", mic_file_name);
	}

	return uiTemp;
#endif
}

void cli_aud_intf_record_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
#ifndef UART_REPLACE_SDCARD_DEBUG
	FRESULT fr;
#endif

	aud_intf_drv_setup_t aud_intf_drv_setup = DEFAULT_AUD_INTF_DRV_SETUP_CONFIG();
	aud_intf_mic_setup_t aud_intf_mic_setup = DEFAULT_AUD_INTF_MIC_SETUP_CONFIG();

	if (argc != 3) {
		cli_aud_intf_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		os_printf("init record test \r\n");

#ifdef UART_REPLACE_SDCARD_DEBUG
		uart_dump_mic_data(1, 2000000);
#else
		/*open file to save pcm data */
		sprintf(mic_file_name, "1:/%s", argv[2]);
		fr = f_open(&mic_file, mic_file_name, FA_CREATE_ALWAYS | FA_WRITE);
		if (fr != FR_OK) {
			os_printf("open %s fail.\r\n", mic_file_name);
			return;
		}
#endif
		//aud_intf_drv_setup.work_mode = AUD_INTF_WORK_MODE_NULL;
		//aud_intf_drv_setup.task_config.priority = 3;
		//aud_intf_drv_setup.aud_intf_rx_spk_data = NULL;
		aud_intf_drv_setup.aud_intf_tx_mic_data = send_mic_data_to_sd;
		ret = bk_aud_intf_drv_init(&aud_intf_drv_setup);
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_drv_init fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_drv_init complete \r\n");
		}

		ret = bk_aud_intf_set_mode(AUD_INTF_WORK_MODE_GENERAL);
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_set_mode fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_set_mode complete \r\n");
		}

		//aud_intf_mic_setup.mic_chl = AUD_INTF_MIC_CHL_MIC1;
		//aud_intf_mic_setup.samp_rate = AUD_ADC_SAMP_RATE_8K;
		aud_intf_mic_setup.mic_type = AUD_INTF_MIC_TYPE_UAC;
		aud_intf_mic_setup.frame_size = 1920;
		aud_intf_mic_setup.mic_gain = 0x3f;
		ret = bk_aud_intf_mic_init(&aud_intf_mic_setup);
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_mic_init fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_mic_init complete \r\n");
		}

#if 0
		/* register uac connect state callback */
		if (aud_intf_spk_setup.spk_type == AUD_INTF_SPK_TYPE_UAC) {
			ret = bk_aud_intf_register_uac_connect_state_cb(uac_connect_state_cb_handle);
			if (ret != BK_ERR_AUD_INTF_OK)
			{
				os_printf("bk_aud_intf_register_uac_connect_state_cb fail, ret:%d\n", ret);
			}
		}

		ret = bk_aud_intf_uac_auto_connect_ctrl(false);
		if (ret != BK_ERR_AUD_INTF_OK)
		{
			os_printf("aud_tras_uac_auto_connect_ctrl fail, ret:%d\n", ret);
		}
#endif
		os_printf("init mic record complete \r\n");
	} else if (os_strcmp(argv[1], "start") == 0) {
		ret = bk_aud_intf_mic_start();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_mic_start fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_mic_start complete \r\n");
		}
		os_printf("start mic record \r\n");
	} else if (os_strcmp(argv[1], "pause") == 0) {
		ret = bk_aud_intf_mic_pause();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_mic_start fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_mic_start complete \r\n");
		}
		os_printf("pause mic record \r\n");
	} else if (os_strcmp(argv[1], "stop") == 0) {
		ret = bk_aud_intf_mic_stop();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_mic_stop fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_mic_stop complete \r\n");
		}

		os_printf("stop mic record \r\n");
	} else if (os_strcmp(argv[1], "set_chl") == 0) {
		if (os_strcmp(argv[2], "mic1") == 0) {
			ret = bk_aud_intf_set_mic_chl(AUD_INTF_MIC_CHL_MIC1);
		} else if (os_strcmp(argv[2], "dual") == 0) {
			ret = bk_aud_intf_set_mic_chl(AUD_INTF_MIC_CHL_DUAL);
		} else {
			os_printf("mic channel is error \r\n");
			return;
		}

		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_set_mic_chl fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_set_mic_chl complete \r\n");
		}

		os_printf("change mic channel \r\n");
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		ret = bk_aud_intf_mic_deinit();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_mic_deinit fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_mic_deinit complete \r\n");
		}

		ret = bk_aud_intf_set_mode(AUD_INTF_WORK_MODE_NULL);
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_set_mode fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_set_mode complete \r\n");
		}

		ret = bk_aud_intf_drv_deinit();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_drv_deinit fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_drv_deinit complete \r\n");
		}

#ifndef UART_REPLACE_SDCARD_DEBUG
		/* close mic file */
		fr = f_close(&mic_file);
		if (fr != FR_OK) {
			os_printf("close %s fail!\r\n", mic_file_name);
			return;
		}
#endif
		os_printf("stop mic record \r\n");

		os_printf("stop record test \r\n");
	} else {
		cli_aud_intf_help();
		return;
	}

}

static bk_err_t read_spk_data_from_sd(unsigned int size)
{
	bk_err_t ret = BK_OK;
	FRESULT fr;
	uint32 uiTemp = 0;

	if (spk_file_empty)
		return BK_FAIL;

	temp_spk_addr = os_malloc(size);

	/* read data from file */
	fr = f_read(&spk_file, (void *)temp_spk_addr, size, &uiTemp);
	if (fr != FR_OK) {
		os_printf("write %s fail.\r\n", spk_file_name);
	}

	if (uiTemp != size) {
		spk_file_empty = true;
		os_printf("the %s is empty \r\n", spk_file_name);
	}

	/* write a fram speaker data to speaker_ring_buff */
	ret = bk_aud_intf_write_spk_data((uint8_t*)temp_spk_addr, size);
	if (ret != BK_OK) {
		os_printf("write spk data fail \r\n");
		return ret;
	}

	os_free(temp_spk_addr);

	return ret;
}

void cli_aud_intf_play_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_ERR_AUD_INTF_OK;
	FRESULT fr;

	aud_intf_drv_setup_t aud_intf_drv_setup = DEFAULT_AUD_INTF_DRV_SETUP_CONFIG();
	aud_intf_spk_setup_t aud_intf_spk_setup = DEFAULT_AUD_INTF_SPK_SETUP_CONFIG();

	if (argc != 3) {
		cli_aud_intf_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		os_printf("init play test \r\n");

		/*open file to read pcm data */
		sprintf(spk_file_name, "1:/%s", argv[2]);
		fr = f_open(&spk_file, spk_file_name, FA_OPEN_EXISTING | FA_READ);
		if (fr != FR_OK) {
			os_printf("open %s fail.\r\n", spk_file_name);
			return;
		}

		//aud_intf_drv_setup.work_mode = AUD_INTF_WORK_MODE_NULL;
		//aud_intf_drv_setup.task_config.priority = 3;
		aud_intf_drv_setup.aud_intf_rx_spk_data = read_spk_data_from_sd;
		//aud_intf_drv_setup.aud_intf_tx_mic_data = NULL;
		ret = bk_aud_intf_drv_init(&aud_intf_drv_setup);
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_drv_init fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_drv_init complete \r\n");
		}

		ret = bk_aud_intf_set_mode(AUD_INTF_WORK_MODE_GENERAL);
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_set_mode fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_set_mode complete \r\n");
		}

		//aud_intf_spk_setup.spk_chl = AUD_INTF_SPK_CHL_LEFT;
		//aud_intf_spk_setup.samp_rate = AUD_DAC_SAMP_RATE_8K;
		aud_intf_spk_setup.frame_size = 640;
		//aud_intf_spk_setup.spk_gain = 0x2d;
		//aud_intf_spk_setup.work_mode = AUD_DAC_WORK_MODE_SIGNAL_END;
		//aud_intf_spk_setup.spk_type = AUD_INTF_SPK_TYPE_UAC;
		ret = bk_aud_intf_spk_init(&aud_intf_spk_setup);
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_spk_init fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_spk_init complete \r\n");
		}

#if 0
		/* register uac connect state callback */
		if (aud_intf_spk_setup.spk_type == AUD_INTF_SPK_TYPE_UAC) {
			ret = bk_aud_intf_register_uac_connect_state_cb(uac_connect_state_cb_handle);
			if (ret != BK_ERR_AUD_INTF_OK)
			{
				os_printf("bk_aud_intf_register_uac_connect_state_cb fail, ret:%d\n", ret);
			}
		}

		ret = bk_aud_intf_uac_auto_connect_ctrl(false);
		if (ret != BK_ERR_AUD_INTF_OK)
		{
			os_printf("aud_tras_uac_auto_connect_ctrl fail, ret:%d\n", ret);
		}
#endif
		os_printf("init play \r\n");
	} else if (os_strcmp(argv[1], "start") == 0) {
		ret = bk_aud_intf_spk_start();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_spk_start fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_spk_start complete \r\n");
		}

		os_printf("start play \r\n");
	} else if (os_strcmp(argv[1], "pause") == 0) {
		ret = bk_aud_intf_spk_pause();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_spk_pause fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_spk_pause complete \r\n");
		}

		os_printf("pause play \r\n");
	} else if (os_strcmp(argv[1], "set_chl") == 0) {
		if (os_strcmp(argv[2], "left") == 0) {
			ret = bk_aud_intf_set_spk_chl(AUD_INTF_SPK_CHL_LEFT);
		} else if (os_strcmp(argv[2], "dual") == 0) {
			ret = bk_aud_intf_set_spk_chl(AUD_INTF_SPK_CHL_DUAL);
		} else {
			os_printf("spk channel is error \r\n");
			return;
		}

		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_set_spk_chl fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_set_spk_chl complete \r\n");
		}

		os_printf("change spk channel \r\n");
	} else if (os_strcmp(argv[1], "stop") == 0) {
		ret = bk_aud_intf_spk_stop();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_spk_stop fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_spk_stop complete \r\n");
		}

		spk_file_empty = false;

		/* close spk file */
		fr = f_close(&spk_file);
		if (fr != FR_OK) {
			os_printf("close %s fail!\r\n", spk_file_name);
			return;
		}

		fr = f_open(&spk_file, spk_file_name, FA_OPEN_EXISTING | FA_READ);
		if (fr != FR_OK) {
			os_printf("open %s fail.\r\n", spk_file_name);
			return;
		}

		os_printf("stop play \r\n");
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		ret = bk_aud_intf_spk_deinit();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_spk_deinit fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_spk_deinit complete \r\n");
		}

		ret = bk_aud_intf_drv_deinit();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_drv_deinit fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_drv_deinit complete \r\n");
		}

		spk_file_empty = false;

		/* close spk file */
		fr = f_close(&spk_file);
		if (fr != FR_OK) {
			os_printf("close %s fail!\r\n", spk_file_name);
			return;
		}

		os_printf("deinit play \r\n");
		os_printf("deinit play test \r\n");
	} else {
		cli_aud_intf_help();
		return;
	}

}


static bk_err_t aud_write_sd_data_to_spk(unsigned int size)
{
	bk_err_t ret = BK_OK;
	FRESULT fr;
	uint32 uiTemp = 0;

	//os_printf("enter %s \r\n", __func__);

	if (spk_file_empty)
		return BK_FAIL;

	temp_spk_addr = os_malloc(size);

	/* read data from file */
	fr = f_read(&voc_ply_file, (void *)temp_spk_addr, size, &uiTemp);
	if (fr != FR_OK) {
		os_printf("read %s fail.\r\n", voc_play_file_name);
	}

	if (uiTemp != size) {
		spk_file_empty = true;
		os_printf("the %s is empty \r\n", voc_play_file_name);
		//TODO
	}

	/* write a fram speaker data to speaker_ring_buff */
	ret = bk_aud_intf_write_spk_data((uint8_t*)temp_spk_addr, size);
	if (ret != BK_OK) {
		os_printf("write voc spk data fail \r\n");
		return ret;
	}

	os_free(temp_spk_addr);

	return ret;
}

static int aud_voc_write_mic_to_sd(UINT8 *data, UINT32 len)
{
	FRESULT fr;
	uint32 uiTemp = 0;

	//os_printf("[send_mic_data_to_sd] \r\n");

	/* write data to file */
	fr = f_write(&voc_red_file, (void *)data, len, &uiTemp);
	if (fr != FR_OK) {
		os_printf("write %s fail.\r\n", voc_record_file_name);
	}

	return uiTemp;
}

/* audio voice transfer pcm data by sdcard */
void cli_aud_intf_sd_voc_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	FRESULT fr;
	bk_err_t ret = BK_OK;

	aud_intf_drv_setup_t aud_intf_drv_setup = DEFAULT_AUD_INTF_DRV_SETUP_CONFIG();
	aud_intf_voc_setup_t aud_intf_voc_setup = DEFAULT_AUD_INTF_VOC_SETUP_CONFIG();

	if (argc != 4) {
		cli_aud_intf_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		os_printf("start voice transfer test \r\n");

		/*open file to save voice mic data */
		sprintf(voc_record_file_name, "1:/%s", argv[2]);
		fr = f_open(&voc_red_file, voc_record_file_name, FA_CREATE_ALWAYS | FA_WRITE);
		if (fr != FR_OK) {
			os_printf("open %s fail.\r\n", voc_record_file_name);
			return;
		}

		/*open file to read and write voice spk data */
		sprintf(voc_play_file_name, "1:/%s", argv[3]);
		fr = f_open(&voc_ply_file, voc_play_file_name, FA_OPEN_EXISTING | FA_READ);
		if (fr != FR_OK) {
			os_printf("open %s fail.\r\n", voc_play_file_name);
			return;
		}

		//aud_intf_drv_setup.work_mode = AUD_INTF_WORK_MODE_NULL;
		//aud_intf_drv_setup.task_config.priority = 3;
		aud_intf_drv_setup.aud_intf_rx_spk_data = aud_write_sd_data_to_spk;
		aud_intf_drv_setup.aud_intf_tx_mic_data = aud_voc_write_mic_to_sd;
		bk_aud_intf_drv_init(&aud_intf_drv_setup);
		bk_aud_intf_set_mode(AUD_INTF_WORK_MODE_VOICE);
		//aud_intf_voc_setup.aec_enable = true;
		//aud_intf_voc_setup.samp_rate = 8000;
		//aud_intf_voc_setup.data_type = AUD_INTF_VOC_DATA_TYPE_G711A;
		aud_intf_voc_setup.data_type = AUD_INTF_VOC_DATA_TYPE_PCM;
		//aud_intf_voc_setup.spk_mode = AUD_DAC_WORK_MODE_SIGNAL_END;
		//aud_intf_voc_setup.mic_gain = 0x2d;
		//aud_intf_voc_setup.spk_gain = 0x2d;
		//aud_intf_voc_setup.aec_cfg.ec_depth = 20;
		//aud_intf_voc_setup.aec_cfg.TxRxThr = 30;
		//aud_intf_voc_setup.aec_cfg.TxRxFlr = 6;
		//aud_intf_voc_setup.aec_cfg.ns_level = 2;
		//aud_intf_voc_setup.aec_cfg.ns_para = 1;
		bk_aud_intf_voc_init(aud_intf_voc_setup);

		temp_spk_addr = os_malloc(640);
		os_memset(temp_spk_addr, 0, 640);
		rtos_delay_milliseconds(2000);
		os_printf("start write spk data \r\n");
		for (int i = 0; i < 10; i++) {
			/* write a fram speaker data to speaker_ring_buff */
			ret = bk_aud_intf_write_spk_data((uint8_t*)temp_spk_addr, 640);
			if (ret != BK_OK) {
				os_printf("write voc spk data fail \r\n");
				return;
			}
		}
		os_free(temp_spk_addr);

		os_printf("start voice \r\n");
	} else if (os_strcmp(argv[1], "stop") == 0) {
		bk_aud_intf_voc_deinit();
		bk_aud_intf_set_mode(AUD_INTF_WORK_MODE_NULL);
		bk_aud_intf_drv_deinit();

		/* close voice record file */
		fr = f_close(&voc_red_file);
		if (fr != FR_OK) {
			os_printf("close %s fail!\r\n", voc_record_file_name);
			return;
		}

		/* close voice play file */
		fr = f_close(&voc_ply_file);
		if (fr != FR_OK) {
			os_printf("close %s fail!\r\n", voc_play_file_name);
			return;
		}

		os_printf("stop voice \r\n");
		os_printf("stop voice test \r\n");
	} else {
		cli_aud_intf_help();
		return;
	}

}
#endif	//#if CONFIG_FATFS

void cli_aud_intf_set_voc_param_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint8_t value = 0;

	if (argc != 3) {
		cli_aud_intf_help();
		return;
	}

	value = strtoul(argv[2], NULL, 0);
	if (os_strcmp(argv[1], "mic_gain") == 0) {
		bk_aud_intf_set_mic_gain(value);
		os_printf("set voc mic gain:%d \r\n", value);
	} else if (os_strcmp(argv[1], "spk_gain") == 0) {
		bk_aud_intf_set_spk_gain(value);
		os_printf("set voc spk gain:%d \r\n", value);
	} else if (os_strcmp(argv[1], "gain") == 0) {
		os_printf("stop play \r\n");
	} else {
		cli_aud_intf_help();
		return;
	}

}

void cli_aud_intf_set_aec_param_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	uint8_t value = 0;
	aud_intf_voc_aec_para_t op;

	if (argc != 3) {
		cli_aud_intf_help();
		return;
	}

	value = strtoul(argv[2], NULL, 0);
	if (os_strcmp(argv[1], "init_flags") == 0) {
		op = AUD_INTF_VOC_AEC_INIT_FLAG;
		os_printf("set aec init_flags:%d \r\n", value);
	} else if (os_strcmp(argv[1], "mic_delay") == 0) {
		op = AUD_INTF_VOC_AEC_MIC_DELAY;
		os_printf("set aec mic_delay:%d \r\n", value);
	} else if (os_strcmp(argv[1], "ec_depth") == 0) {
		op = AUD_INTF_VOC_AEC_EC_DEPTH;
		os_printf("set aec ec_depth:%d \r\n", value);
	} else if (os_strcmp(argv[1], "ref_scale") == 0) {
		op = AUD_INTF_VOC_AEC_REF_SCALE;
		os_printf("set aec ref_scale:%d \r\n", value);
	} else if (os_strcmp(argv[1], "voice_vol") == 0) {
		op = AUD_INTF_VOC_AEC_VOICE_VOL;
		os_printf("set aec voice_vol:%d \r\n", value);
	} else if (os_strcmp(argv[1], "TxRxThr") == 0) {
		op = AUD_INTF_VOC_AEC_TXRX_THR;
		os_printf("set aec TxRxThr:%d \r\n", value);
	} else if (os_strcmp(argv[1], "TxRxFlr") == 0) {
		op = AUD_INTF_VOC_AEC_TXRX_FLR;
		os_printf("set aec TxRxFlr:%d \r\n", value);
	} else if (os_strcmp(argv[1], "ns_level") == 0) {
		op = AUD_INTF_VOC_AEC_NS_LEVEL;
		os_printf("set aec ns_level:%d \r\n", value);
	} else if (os_strcmp(argv[1], "ns_para") == 0) {
		op = AUD_INTF_VOC_AEC_NS_PARA;
		os_printf("set aec ns_para:%d \r\n", value);
	} else if (os_strcmp(argv[1], "drc") == 0) {
		op = AUD_INTF_VOC_AEC_DRC;
		os_printf("set aec drc:%d \r\n", value);
	} else {
		cli_aud_intf_help();
		return;
	}
	ret = bk_aud_intf_set_aec_para(op, value);
	if (ret != BK_OK)
		os_printf("test fail \r\n");

	os_printf("set aec parameters complete \r\n");
}

void cli_aud_intf_get_aec_param_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;

	if (argc != 1) {
		cli_aud_intf_help();
		return;
	}

	ret = bk_aud_intf_get_aec_para();
	if (ret != BK_OK)
		os_printf("test fail \r\n");

	os_printf("get aec parameters complete \r\n");
}

/* set mic and speaker sample rate */
void cli_aud_intf_set_samp_rate_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	uint32_t adc_samp_rate;
	uint32_t dac_samp_rate;

	if (argc != 3) {
		cli_aud_intf_help();
		return;
	}

	if (os_strcmp(argv[1], "mic") == 0) {
		adc_samp_rate = strtoul(argv[2], NULL, 10);
		ret = bk_aud_intf_set_mic_samp_rate(adc_samp_rate);
		if (ret != BK_ERR_AUD_INTF_OK)
			os_printf("set mic samp rate:%d fail \r\n", adc_samp_rate);
		os_printf("set mic samp rate:%d complete \r\n", adc_samp_rate);
	} else if (os_strcmp(argv[1], "spk") == 0) {
		dac_samp_rate = strtoul(argv[2], NULL, 10);
		ret = bk_aud_intf_set_spk_samp_rate(dac_samp_rate);
		if (ret != BK_ERR_AUD_INTF_OK)
			os_printf("set spk samp rate:%d fail, ret:%d \r\n", dac_samp_rate, ret);
		os_printf("set spk samp rate:%d complete \r\n", dac_samp_rate);

	} else {
		cli_aud_intf_help();
		return;
	}

	if (ret != BK_OK)
		os_printf("test fail \r\n");

	os_printf("set samp rate complete \r\n");
}

/* set mic and speaker sample rate */
void cli_aud_intf_doorbell_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;

	if (argc != 2 && argc != 3) {
		cli_aud_intf_help();
		return;
	}

	if (os_strcmp(argv[1], "stop") == 0) {
		ret = bk_aud_intf_voc_stop();
		if (ret != BK_ERR_AUD_INTF_OK)
			os_printf("stop doorbell fail \r\n");
		os_printf("stop doorbell complete \r\n");
	} else if (os_strcmp(argv[1], "ctrl_mic") == 0) {
		aud_intf_voc_mic_ctrl_t mic_en = AUD_INTF_VOC_MIC_MAX;
		if (os_strcmp(argv[2], "open") == 0)
			mic_en = AUD_INTF_VOC_MIC_OPEN;
		else if (os_strcmp(argv[2], "close") == 0)
			mic_en = AUD_INTF_VOC_MIC_CLOSE;
		else {
			cli_aud_intf_help();
			return;
		}

		ret = bk_aud_intf_voc_mic_ctrl(mic_en);
		if (ret != BK_ERR_AUD_INTF_OK)
			os_printf("ctrl voc mic fail: %d \r\n", mic_en);
		os_printf("ctrl voc mic complete \r\n");
	} else if (os_strcmp(argv[1], "ctrl_spk") == 0) {
		aud_intf_voc_spk_ctrl_t spk_en = AUD_INTF_VOC_SPK_MAX;
		if (os_strcmp(argv[2], "open") == 0)
			spk_en = AUD_INTF_VOC_SPK_OPEN;
		else if (os_strcmp(argv[2], "close") == 0)
			spk_en = AUD_INTF_VOC_SPK_CLOSE;
		else {
			cli_aud_intf_help();
			return;
		}

		ret = bk_aud_intf_voc_spk_ctrl(spk_en);
		if (ret != BK_ERR_AUD_INTF_OK)
			os_printf("ctrl voc spk fail: %d \r\n", spk_en);
		os_printf("ctrl voc spk complete \r\n");
	} else if (os_strcmp(argv[1], "ctrl_aec") == 0) {
		bool aec_en = false;
		if (os_strcmp(argv[2], "open") == 0)
			aec_en = true;
		else if (os_strcmp(argv[2], "close") == 0)
			aec_en = false;
		else {
			cli_aud_intf_help();
			return;
		}

		ret = bk_aud_intf_voc_aec_ctrl(aec_en);
		if (ret != BK_ERR_AUD_INTF_OK)
			os_printf("ctrl voc aec fail: %d \r\n", aec_en);
		os_printf("ctrl voc aec complete \r\n");
	} else if (os_strcmp(argv[1], "uac_vol") == 0) {
		uint32_t value = strtoul(argv[2], NULL, 0);

		ret = bk_aud_intf_set_spk_gain(value);
		if (ret != BK_ERR_AUD_INTF_OK)
		{
			os_printf("aud_tras_uac_auto_connect_ctrl fail, ret:%d\n", ret);
			//break;
		}
		os_printf("set uac volume: %d \r\n", value);
		os_printf("set uac volume complete \r\n");
	} else if (os_strcmp(argv[1], "start") == 0) {
		ret = bk_aud_intf_voc_start();
		if (ret != BK_ERR_AUD_INTF_OK)
			os_printf("start doorbell fail \r\n");
		os_printf("start doorbell complete \r\n");
	} else {
		cli_aud_intf_help();
		return;
	}
}

#if CONFIG_AUD_TRAS_DAC_DEBUG
void cli_aud_intf_debug_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc != 2) {
		cli_aud_intf_help();
		return;
	}

	if (os_strcmp(argv[1], "on") == 0) {
		os_printf("turn on voc dac debug \r\n");
		bk_aud_intf_set_voc_dac_debug(true);
	} else if (os_strcmp(argv[1], "off") == 0) {
		bk_aud_intf_set_voc_dac_debug(false);
		os_printf("turn off voc dac debug \r\n");
	} else {
		cli_aud_intf_help();
		return;
	}
}
#endif

void cli_aud_intf_aud_debug_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc != 3) {
		cli_aud_intf_help();
		return;
	}
#if 0
	if (os_strcmp(argv[1], "aud_debug") == 0) {
		if (os_strcmp(argv[2], "on") == 0) {
			os_printf("open audio debug \r\n");
			bk_aud_debug_tcp_init();
		} else if (os_strcmp(argv[2], "off") == 0) {
			os_printf("close audio debug \r\n");
			bk_aud_debug_tcp_deinit();
		} else {
			os_printf("param is error \r\n");
		}
	} else if (os_strcmp(argv[1], "tx") == 0) {
		if (os_strcmp(argv[2], "on") == 0) {
			os_printf("open audio tx debug \r\n");
			bk_aud_intf_voc_tx_debug(bk_aud_debug_voc_tx_tcp_send_packet);
		} else if (os_strcmp(argv[2], "off") == 0) {
			os_printf("close audio tx debug \r\n");
			bk_aud_intf_voc_tx_debug(NULL);
		} else {
			os_printf("param is error \r\n");
		}
	} else if (os_strcmp(argv[1], "rx") == 0) {
		if (os_strcmp(argv[2], "on") == 0) {
			os_printf("open audio rx debug \r\n");
			bk_aud_intf_voc_rx_debug(bk_aud_debug_voc_rx_tcp_send_packet);
		} else if (os_strcmp(argv[2], "off") == 0) {
			os_printf("close audio rx debug \r\n");
			bk_aud_intf_voc_rx_debug(NULL);
		} else {
			os_printf("param is error \r\n");
		}
	} else if (os_strcmp(argv[1], "aec") == 0) {
		if (os_strcmp(argv[2], "on") == 0) {
			os_printf("open audio aec debug \r\n");
			bk_aud_intf_voc_aec_debug(bk_aud_debug_voc_aec_tcp_send_packet);
		} else if (os_strcmp(argv[2], "off") == 0) {
			os_printf("close audio aec debug \r\n");
			bk_aud_intf_voc_aec_debug(NULL);
		} else {
			os_printf("param is error \r\n");
		}
	} else {
		cli_aud_intf_help();
		return;
	}
#endif
}

void cli_aud_intf_loop_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;

	aud_intf_drv_setup_t aud_intf_drv_setup = DEFAULT_AUD_INTF_DRV_SETUP_CONFIG();
	aud_intf_mic_setup_t aud_intf_mic_setup = DEFAULT_AUD_INTF_MIC_SETUP_CONFIG();
	aud_intf_spk_setup_t aud_intf_spk_setup = DEFAULT_AUD_INTF_SPK_SETUP_CONFIG();

	if (argc != 2) {
		cli_aud_intf_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		//aud_intf_drv_setup.work_mode = AUD_INTF_WORK_MODE_NULL;
		//aud_intf_drv_setup.task_config.priority = 3;
		//aud_intf_drv_setup.aud_intf_rx_spk_data = NULL;
		aud_intf_drv_setup.aud_intf_tx_mic_data = send_mic_data_to_spk;
		ret = bk_aud_intf_drv_init(&aud_intf_drv_setup);
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_drv_init fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_drv_init complete \r\n");
		}

		ret = bk_aud_intf_set_mode(AUD_INTF_WORK_MODE_GENERAL);
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_set_mode fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_set_mode complete \r\n");
		}

		//aud_intf_mic_setup.mic_chl = AUD_INTF_MIC_CHL_MIC1;
		//aud_intf_mic_setup.samp_rate = AUD_ADC_SAMP_RATE_8K;
		//aud_intf_mic_setup.frame_size = 320;
		//aud_intf_mic_setup.mic_gain = 0x2d;
		ret = bk_aud_intf_mic_init(&aud_intf_mic_setup);
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_mic_init fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_mic_init complete \r\n");
		}

		//aud_intf_spk_setup.spk_chl = AUD_INTF_SPK_CHL_LEFT;
		//aud_intf_spk_setup.samp_rate = AUD_DAC_SAMP_RATE_8K;
		//aud_intf_spk_setup.frame_size = 320;
		//aud_intf_spk_setup.spk_gain = 0x2d;
		//aud_intf_spk_setup.work_mode = AUD_DAC_WORK_MODE_SIGNAL_END;
		//aud_intf_spk_setup.spk_type = AUD_INTF_SPK_TYPE_BOARD;
		ret = bk_aud_intf_spk_init(&aud_intf_spk_setup);
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_spk_init fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_spk_init complete \r\n");
		}

		ret = bk_aud_intf_mic_start();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_mic_start fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_mic_start complete \r\n");
		}

		ret = bk_aud_intf_spk_start();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_spk_start fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_spk_start complete \r\n");
		}

		os_printf("start audio loop test \r\n");
	} else if (os_strcmp(argv[1], "stop") == 0) {
		/* stop mic and spk */
		ret = bk_aud_intf_mic_stop();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_mic_stop fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_mic_stop complete \r\n");
		}

		ret = bk_aud_intf_spk_stop();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_spk_stop fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_spk_stop complete \r\n");
		}

		/* deinit mic and spk */
		ret = bk_aud_intf_mic_deinit();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_mic_deinit fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_mic_deinit complete \r\n");
		}

		ret = bk_aud_intf_spk_stop();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_spk_stop fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_spk_stop complete \r\n");
		}

		/* deinit aud_intf drv */
		ret = bk_aud_intf_set_mode(AUD_INTF_WORK_MODE_NULL);
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_set_mode fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_set_mode complete \r\n");
		}

		ret = bk_aud_intf_drv_deinit();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_drv_deinit fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_drv_deinit complete \r\n");
		}

		os_printf("stop audio loop test \r\n");
	} else {
		cli_aud_intf_help();
		return;
	}
}

void cli_aud_intf_play_const_data_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_ERR_AUD_INTF_OK;

	aud_intf_drv_setup_t aud_intf_drv_setup = DEFAULT_AUD_INTF_DRV_SETUP_CONFIG();
	aud_intf_spk_setup_t aud_intf_spk_setup = DEFAULT_AUD_INTF_SPK_SETUP_CONFIG();

	if (argc != 2) {
		cli_aud_intf_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		os_printf("init play test \r\n");

		const_data_count = 0;

		//aud_intf_drv_setup.work_mode = AUD_INTF_WORK_MODE_NULL;
		//aud_intf_drv_setup.task_config.priority = 3;
		aud_intf_drv_setup.aud_intf_rx_spk_data = read_spk_data_from_const_data;
		//aud_intf_drv_setup.aud_intf_tx_mic_data = NULL;
		ret = bk_aud_intf_drv_init(&aud_intf_drv_setup);
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_drv_init fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_drv_init complete \r\n");
		}

		ret = bk_aud_intf_set_mode(AUD_INTF_WORK_MODE_GENERAL);
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_set_mode fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_set_mode complete \r\n");
		}

		aud_intf_spk_setup.spk_chl = AUD_INTF_SPK_CHL_LEFT;
		//aud_intf_spk_setup.samp_rate = AUD_DAC_SAMP_RATE_8K;
		//aud_intf_spk_setup.frame_size = 320;
		aud_intf_spk_setup.spk_gain = 0x1d;
		//aud_intf_spk_setup.work_mode = AUD_DAC_WORK_MODE_SIGNAL_END;
		//aud_intf_spk_setup.spk_type = AUD_INTF_SPK_TYPE_UAC;
		ret = bk_aud_intf_spk_init(&aud_intf_spk_setup);
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_spk_init fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_spk_init complete \r\n");
		}
		ret = bk_aud_intf_spk_start();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_spk_start fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_spk_start complete \r\n");
		}

		os_printf("start play const audio data \r\n");
	}else if (os_strcmp(argv[1], "stop") == 0) {
		ret = bk_aud_intf_spk_deinit();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_spk_deinit fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_spk_deinit complete \r\n");
		}

		ret = bk_aud_intf_drv_deinit();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_drv_deinit fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_drv_deinit complete \r\n");
		}

		const_data_count = 0;

		os_printf("stop play const audio data \r\n");
	} else {
		cli_aud_intf_help();
		return;
	}

}

#define AUD_INTF_CMD_CNT (sizeof(s_aud_intf_commands) / sizeof(struct cli_command))
static const struct cli_command s_aud_intf_commands[] = {
#if CONFIG_FATFS
	{"aud_intf_record_test", "aud_intf_record_test {init|start|pause|stop|set_chl|deinit xxx.pcm|mic1|dual}", cli_aud_intf_record_cmd},
	{"aud_intf_play_test", "{init|start|pause|stop|set_chl|deinit xxx.pcm|left|dual}", cli_aud_intf_play_cmd},
	{"aud_intf_sd_voc_test", "aud_intf_sd_voc_test {start|stop xx.pcm, xx.pcm}", cli_aud_intf_sd_voc_cmd},
#endif
	{"aud_intf_set_voc_param_test", "aud_intf_set_voc_param_test {param value}", cli_aud_intf_set_voc_param_cmd},
	{"aud_intf_set_aec_param_test", "aud_intf_set_aec_param_test {param value}", cli_aud_intf_set_aec_param_cmd},
	{"aud_intf_get_aec_param_test", "aud_intf_get_aec_param_test", cli_aud_intf_get_aec_param_cmd},
	{"aud_intf_set_samp_rate_test", "aud_intf_set_samp_rate_test {param value}", cli_aud_intf_set_samp_rate_cmd},
	{"aud_intf_doorbell_test", "aud_intf_doorbell_test {start|stop}", cli_aud_intf_doorbell_cmd},
	/* debug cmd */
	{"aud_intf_aud_debug_test", "aud_intf_aud_debug_test {aud_debug|tx|rx|aec on|off}", cli_aud_intf_aud_debug_cmd},
	{"aud_intf_loop_test", "aud_intf_loop_test {start|stop}", cli_aud_intf_loop_test_cmd},
	{"aud_intf_play_const_data_test", "aud_intf_play_const_data_test {start|stop}", cli_aud_intf_play_const_data_cmd},
};

int cli_aud_intf_init(void)
{
	return cli_register_commands(s_aud_intf_commands, AUD_INTF_CMD_CNT);
}

