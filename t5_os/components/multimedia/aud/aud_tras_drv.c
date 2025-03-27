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

//#include <common/bk_include.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <driver/aud_adc_types.h>
#include <driver/aud_adc.h>
#include <driver/aud_dac_types.h>
#include <driver/aud_dac.h>

#include <driver/dma.h>
#include <bk_general_dma.h>
#include "sys_driver.h"
#include "aud_intf_private.h"
#include "aud_tras_drv.h"
#include <driver/psram.h>
#include <modules/aec.h>
#include <driver/audio_ring_buff.h>
#include <modules/g711.h>
#include "gpio_driver.h"
#include <driver/gpio.h>


#include "bk_misc.h"
#include <soc/mapping.h>

#include "media_evt.h"
#include "media_mailbox_list_util.h"
#include <bk_general_dma.h>
#include <driver/uart.h>
#include "gpio_driver.h"

#include "psram_mem_slab.h"


#if (CONFIG_CACHE_ENABLE)
#include "cache.h"
#endif


#define AUD_TRAS_DRV_TAG "tras_drv"

#define LOGI(...) BK_LOGI(AUD_TRAS_DRV_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(AUD_TRAS_DRV_TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(AUD_TRAS_DRV_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(AUD_TRAS_DRV_TAG, ##__VA_ARGS__)


#define AUD_MEDIA_SEM_ENABLE    0


#define CONFIG_AUD_TRAS_AEC_MIC_DELAY_POINTS   53
//#define CONFIG_AUD_RING_BUFF_SAFE_INTERVAL    20

#define CONFIG_UAC_MIC_SPK_COUNT_DEBUG
#ifdef CONFIG_UAC_MIC_SPK_COUNT_DEBUG
#define UAC_MIC_SPK_COUNT_DEBUG_INTERVAL (1000 * 2)
#endif

#define TU_QITEM_COUNT      (120)
static beken_thread_t  aud_trs_drv_thread_hdl = NULL;
static beken_queue_t aud_trs_drv_int_msg_que = NULL;

aud_tras_drv_info_t aud_tras_drv_info = DEFAULT_AUD_TRAS_DRV_INFO();
static bool uac_mic_read_flag = false;
static bool uac_spk_write_flag = false;

media_mailbox_msg_t uac_connect_state_msg = {0};

media_mailbox_msg_t mic_to_media_app_msg = {0};
media_mailbox_msg_t spk_to_media_app_msg = {0};
#if AUD_MEDIA_SEM_ENABLE
beken_semaphore_t mailbox_media_aud_mic_sem = NULL;
#endif
aud_tras_drv_mic_notify_t mic_nofity = {0, 0};

static beken_semaphore_t aud_tras_drv_task_sem = NULL;

#if CONFIG_AUD_TRAS_AEC_MIC_DELAY_DEBUG
static uint8_t mic_delay_num = 0;
#endif

#define AUD_MIC_DEBUG       0


#if AUD_MIC_DEBUG
//#if 1
static const unsigned int PCM_8000[] = {
	0x0001, 0x5A82, 0x7FFF, 0x5A82, 0x0000, 0xA57F, 0x8001, 0xA57E,
	0x0001, 0x5A82, 0x7FFF, 0x5A82, 0x0000, 0xA57F, 0x8001, 0xA57E,
	0x0001, 0x5A82, 0x7FFF, 0x5A82, 0x0000, 0xA57F, 0x8001, 0xA57E,
	0x0001, 0x5A82, 0x7FFF, 0x5A82, 0x0000, 0xA57F, 0x8001, 0xA57E,
	0x0001, 0x5A82, 0x7FFF, 0x5A82, 0x0000, 0xA57F, 0x8001, 0xA57E,
	0x0001, 0x5A82, 0x7FFF, 0x5A82, 0x0000, 0xA57F, 0x8001, 0xA57E,
	0x0001, 0x5A82, 0x7FFF, 0x5A82, 0x0000, 0xA57F, 0x8001, 0xA57E,
	0x0001, 0x5A82, 0x7FFF, 0x5A82, 0x0000, 0xA57F, 0x8001, 0xA57E,
	0x0001, 0x5A82, 0x7FFF, 0x5A82, 0x0000, 0xA57F, 0x8001, 0xA57E,
	0x0001, 0x5A82, 0x7FFF, 0x5A82, 0x0000, 0xA57F, 0x8001, 0xA57E,
	0x0001, 0x5A82, 0x7FFF, 0x5A82, 0x0000, 0xA57F, 0x8001, 0xA57E,
	0x0001, 0x5A82, 0x7FFF, 0x5A82, 0x0000, 0xA57F, 0x8001, 0xA57E,
	0x0001, 0x5A82, 0x7FFF, 0x5A82, 0x0000, 0xA57F, 0x8001, 0xA57E,
	0x0001, 0x5A82, 0x7FFF, 0x5A82, 0x0000, 0xA57F, 0x8001, 0xA57E,
	0x0001, 0x5A82, 0x7FFF, 0x5A82, 0x0000, 0xA57F, 0x8001, 0xA57E,
	0x0001, 0x5A82, 0x7FFF, 0x5A82, 0x0000, 0xA57F, 0x8001, 0xA57E,
	0x0001, 0x5A82, 0x7FFF, 0x5A82, 0x0000, 0xA57F, 0x8001, 0xA57E,
	0x0001, 0x5A82, 0x7FFF, 0x5A82, 0x0000, 0xA57F, 0x8001, 0xA57E,
	0x0001, 0x5A82, 0x7FFF, 0x5A82, 0x0000, 0xA57F, 0x8001, 0xA57E,
	0x0001, 0x5A82, 0x7FFF, 0x5A82, 0x0000, 0xA57F, 0x8001, 0xA57E,
};
#endif

#ifdef CONFIG_UAC_MIC_SPK_COUNT_DEBUG
typedef struct {
	beken_timer_t timer;
	uint32_t mic_size;
	uint32_t spk_size;
} uac_mic_spk_count_debug_t;

static uac_mic_spk_count_debug_t uac_mic_spk_count = {0};
#endif


/* extern api */
bk_err_t aud_tras_drv_deinit(void);
static bk_err_t aud_tras_drv_voc_start(void);
static bk_err_t aud_tras_drv_set_spk_gain(uint16_t value);


void *audio_tras_drv_malloc(uint32_t size)
{
#if CONFIG_AUD_TRAS_USE_SRAM
    return os_malloc(size);
#else
    return psram_malloc(size);//bk_psram_frame_buffer_malloc(PSRAM_HEAP_AUDIO, size);
#endif
}

void audio_tras_drv_free(void *mem)
{
#if CONFIG_AUD_TRAS_USE_SRAM
    os_free(mem);
#else
    psram_free(mem);//bk_psram_frame_buffer_free(mem);
#endif
}

#ifdef CONFIG_UAC_MIC_SPK_COUNT_DEBUG
static void uac_mic_spk_count_dump(void *param)
{
	uac_mic_spk_count.mic_size = uac_mic_spk_count.mic_size / 1024 / (UAC_MIC_SPK_COUNT_DEBUG_INTERVAL / 1000);
	uac_mic_spk_count.spk_size = uac_mic_spk_count.spk_size / 1024 / (UAC_MIC_SPK_COUNT_DEBUG_INTERVAL / 1000);

	LOGI("[UAC] mic: %uKB/s, spk: %uKB/s \n", uac_mic_spk_count.mic_size, uac_mic_spk_count.spk_size);
	uac_mic_spk_count.mic_size  = 0;
	uac_mic_spk_count.spk_size  = 0;
}

static void uac_mic_spk_count_close(void)
{
	bk_err_t ret = BK_OK;

	if (uac_mic_spk_count.timer.handle) {
		ret = rtos_stop_timer(&uac_mic_spk_count.timer);
		if (ret != BK_OK) {
			LOGE("%s, %d, stop aud_tx_count timer fail \n", __func__, __LINE__);
		}
		ret = rtos_deinit_timer(&uac_mic_spk_count.timer);
		if (ret != BK_OK) {
			LOGE("%s, %d, deinit aud_tx_count timer fail \n", __func__, __LINE__);
		}
		uac_mic_spk_count.timer.handle = NULL;
	}
	uac_mic_spk_count.mic_size = 0;
	uac_mic_spk_count.spk_size = 0;
	LOGI("%s, %d, close uac count timer complete \n", __func__, __LINE__);
}

static void uac_mic_spk_count_open(void)
{
	bk_err_t ret = BK_OK;

	if (uac_mic_spk_count.timer.handle != NULL)
	{
		ret = rtos_deinit_timer(&uac_mic_spk_count.timer);
		if (BK_OK != ret)
		{
			LOGE("%s, %d, deinit uac_mic_spk_count time fail \n", __func__, __LINE__);
			goto exit;
		}
		uac_mic_spk_count.timer.handle = NULL;
	}

	uac_mic_spk_count.mic_size = 0;
	uac_mic_spk_count.spk_size = 0;

	ret = rtos_init_timer(&uac_mic_spk_count.timer, UAC_MIC_SPK_COUNT_DEBUG_INTERVAL, uac_mic_spk_count_dump, NULL);
	if (ret != BK_OK) {
		LOGE("%s, %d, rtos_init_timer fail \n", __func__, __LINE__);
		goto exit;
	}
	ret = rtos_start_timer(&uac_mic_spk_count.timer);
	if (ret != BK_OK) {
		LOGE("%s, %d, rtos_start_timer fail \n", __func__, __LINE__);
		goto exit;
	}
	LOGI("%s, %d, open uac count timer complete \n", __func__, __LINE__);

	return;
exit:

	uac_mic_spk_count_close();
}

static void uac_mic_spk_count_add_mic_size(uint32_t mic_size)
{
	uac_mic_spk_count.mic_size += mic_size;
}

static void uac_mic_spk_count_add_spk_size(uint32_t spk_size)
{
	uac_mic_spk_count.spk_size += spk_size;
}
#endif

static void aud_tras_dac_pa_ctrl(bool en)
{
	if (en) {
#if CONFIG_AUD_TRAS_DAC_PA_CTRL
	/* delay 2ms to avoid po audio data, and then open pa */
	delay_ms(2);
	/* open pa according to congfig */
	gpio_dev_unmap(AUD_DAC_PA_CTRL_GPIO);
	bk_gpio_enable_output(AUD_DAC_PA_CTRL_GPIO);
#if AUD_DAC_PA_ENABLE_LEVEL
	bk_gpio_set_output_high(AUD_DAC_PA_CTRL_GPIO);
#else
	bk_gpio_set_output_low(AUD_DAC_PA_CTRL_GPIO);
#endif
#endif
	} else {
#if CONFIG_AUD_TRAS_DAC_PA_CTRL
		/* open pa according to congfig */
		//gpio_dev_unmap(AUD_DAC_PA_CTRL_GPIO);
		//bk_gpio_enable_output(AUD_DAC_PA_CTRL_GPIO);
#if AUD_DAC_PA_ENABLE_LEVEL
		bk_gpio_set_output_low(AUD_DAC_PA_CTRL_GPIO);
#else
		bk_gpio_set_output_high(AUD_DAC_PA_CTRL_GPIO);
#endif
#endif
	}
}

bk_err_t aud_tras_drv_send_msg(aud_tras_drv_op_t op, void *param)
{
	bk_err_t ret;
	aud_tras_drv_msg_t msg;

	msg.op = op;
	msg.param = param;
	if (aud_trs_drv_int_msg_que) {
		ret = rtos_push_to_queue(&aud_trs_drv_int_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			LOGE("%s, %d, aud_tras_send_int_msg fail \n", __func__, __LINE__);
			return kOverrunErr;
		}

		return ret;
	}
	return kNoResourcesErr;
}

static bk_err_t aud_tras_adc_config(aud_adc_config_t *adc_config)
{
	bk_err_t ret = BK_OK;

	/* init audio driver and config adc */
	ret = bk_aud_driver_init();
	if (ret != BK_OK) {
		LOGE("%s, %d, init audio driver fail \n", __func__, __LINE__);
		goto aud_adc_exit;
	}

	ret = bk_aud_adc_init(adc_config);
	if (ret != BK_OK) {
		LOGE("%s, %d, init audio adc fail \n", __func__, __LINE__);
		goto aud_adc_exit;
	}

	return BK_OK;

aud_adc_exit:
	LOGE("%s, %d, audio adc config fail \n", __func__, __LINE__);
	bk_aud_driver_deinit();
	return BK_FAIL;
}

static bk_err_t aud_tras_dac_config(aud_dac_config_t *dac_config)
{
	bk_err_t ret = BK_OK;

	/* init audio driver and config dac */
	ret = bk_aud_driver_init();
	if (ret != BK_OK) {
		LOGE("%s, %d, init audio driver fail \n", __func__, __LINE__);
		goto aud_dac_exit;
	}

	ret = bk_aud_dac_init(dac_config);
	if (ret != BK_OK) {
		LOGE("%s, %d, init audio dac fail \n", __func__, __LINE__);
		goto aud_dac_exit;
	}

	return BK_OK;

aud_dac_exit:
	LOGE("%s, %d, audio dac config fail \n", __func__, __LINE__);
	bk_aud_driver_deinit();
	return BK_FAIL;
}
const uint16_t EQTAB[257] =
{
  8638,10022,10662,10996,11263,11527,11804,12099,12390,12624,12825,13007,13178,13344,13510,13763,
  13981,14146,14310,14471,14629,14783,14930,15069,15199,15319,15428,15527,15614,15688,15749,15810,
  15870,15929,15986,16040,16092,16139,16183,16222,16256,16285,16309,16327,16339,16345,16345,16345,
  16345,16345,16345,16345,16344,16343,16342,16339,16336,16331,16326,16319,16310,16300,16288,16275,
  16259,16241,16221,16199,16175,16148,16118,16086,16051,16014,15973,15930,15883,15834,15782,15727,
  15669,15608,15548,15488,15428,15368,15309,15249,15190,15130,15070,15009,14948,14886,14824,14761,
  14697,14632,14566,14499,14431,14361,14291,14219,14146,14071,13994,13916,13835,13753,13670,13582,
  13489,13392,13291,13190,13089,12987,12885,12780,12673,12563,12449,12330,12205,12076,11939,11796,
  11646,11487,11321,11144,10957,10761,10554,10337,10108,9868, 9616, 9351, 9074, 8783, 8481, 8165,
  7831, 7480, 7113, 6748, 6388, 6032, 5681, 5337, 4999, 4670, 4349, 4037, 3736, 3445, 3165, 2898,
  2643, 2401, 2173, 1960, 1761, 1577, 1408, 1256, 1120, 1001, 898,  812,  744,  692,  658,  642,
  624,  606,  587,  568,  549,  531,  512,  494,  476,  459,  442,  425,  409,  393,  378,  363,
  348,  334,  321,  308,  296,  284,  273,  263,  253,  244,  236,  228,  221,  214,  208,  203,
  197,  192,  186,  180,  174,  168,  162,  156,  151,  145,  140,  135,  130,  125,  121,  116,
  112,  108,  105,  101,  98,   95,   93,   90,   88,   86,   85,   84,   83,   82,   82,   82,
  82,   82,   82,   82,   82,   82,   82,   82,   82,   82,   82,   82,   82,   82,   82,   82,   82
 };

static bk_err_t aud_tras_drv_aec_cfg(void)
{
	uint32_t aec_context_size = 0;
	uint32_t val = 0;
	aec_info_t *temp_aec_info = aud_tras_drv_info.voc_info.aec_info;
	/* init aec context */
	aec_context_size = aec_size(2000);
	LOGI("%s, %d, malloc aec size: %d \n", __func__, __LINE__, aec_context_size);
	temp_aec_info->aec = (AECContext*)audio_tras_drv_malloc(aec_context_size);
//	temp_aec_info->aec = (AECContext*)psram_malloc(aec_context_size);
	LOGI("%s, %d, aec use malloc sram: %p \n", __func__, __LINE__, temp_aec_info->aec);
	if (temp_aec_info->aec == NULL) {
		LOGE("%s, %d, malloc aec fail, aec_context_size: %d \n", __func__, __LINE__, aec_context_size);
		return BK_FAIL;
	}

	/* config sample rate, default is 8K */
	if (temp_aec_info->samp_rate == 8000 || temp_aec_info->samp_rate == 16000) {
		aec_init(temp_aec_info->aec, temp_aec_info->samp_rate);
	}

	/* 获取处理帧长，16000采样率320点(640字节)，8000采样率160点(320字节)  (对应20毫秒数据) */
	aec_ctrl(temp_aec_info->aec, AEC_CTRL_CMD_GET_FRAME_SAMPLE, (uint32_t)(&(temp_aec_info->samp_rate_points)));

	/* 获取结构体内部可以复用的ram作为每帧tx,rx,out数据的临时buffer; ram很宽裕的话也可以在外部单独申请获取 */
	aec_ctrl(temp_aec_info->aec, AEC_CTRL_CMD_GET_TX_BUF, (uint32_t)(&val)); temp_aec_info->mic_addr = (int16_t*)val;
	aec_ctrl(temp_aec_info->aec, AEC_CTRL_CMD_GET_RX_BUF, (uint32_t)(&val)); temp_aec_info->ref_addr = (int16_t*)val;
	aec_ctrl(temp_aec_info->aec, AEC_CTRL_CMD_GET_OUT_BUF,(uint32_t)(&val)); temp_aec_info->out_addr = (int16_t*)val;

	/* 以下是参数调节示例,aec_init中都已经有默认值,可以直接先用默认值 */
	aec_ctrl(temp_aec_info->aec, AEC_CTRL_CMD_SET_FLAGS, temp_aec_info->aec_config->init_flags);							//库内各模块开关; aec_init内默认赋值0x1f;

	/* 回声消除相关 */
	temp_aec_info->aec_config->mic_delay = 16;//0x0
	temp_aec_info->aec_config->ec_depth = 0x2;//0x14
	temp_aec_info->aec_config->voice_vol =0x0d;//0xe
	temp_aec_info->aec_config->ns_level = 0x5;//0x2
	temp_aec_info->aec_config->ns_para = 0x02;//0x1
	temp_aec_info->aec_config->drc = 0x0;//0xf
	aec_ctrl(temp_aec_info->aec, AEC_CTRL_CMD_SET_MIC_DELAY, temp_aec_info->aec_config->mic_delay);						//设置参考信号延迟(采样点数，需要dump数据观察)
	aec_ctrl(temp_aec_info->aec, AEC_CTRL_CMD_SET_EC_DEPTH, temp_aec_info->aec_config->ec_depth);							//建议取值范围1~50; 后面几个参数建议先用aec_init内的默认值，具体需要根据实际情况调试; 总得来说回声越大需要调的越大
//	aec_ctrl(temp_aec_info->aec, AEC_CTRL_CMD_SET_TxRxThr, temp_aec_info->aec_config->TxRxThr);							//建议取值范围10~64
//	aec_ctrl(temp_aec_info->aec, AEC_CTRL_CMD_SET_TxRxFlr, temp_aec_info->aec_config->TxRxFlr);							//建议取值范围1~10

	aec_ctrl(temp_aec_info->aec, AEC_CTRL_CMD_SET_REF_SCALE, temp_aec_info->aec_config->ref_scale);						//取值0,1,2；rx数据如果幅值太小的话适当放大
	aec_ctrl(temp_aec_info->aec, AEC_CTRL_CMD_SET_VOL, temp_aec_info->aec_config->voice_vol);								//通话过程中如果需要经常调节喇叭音量就设置下当前音量等级
	/* 降噪相关 */
	aec_ctrl(temp_aec_info->aec, AEC_CTRL_CMD_SET_NS_LEVEL, temp_aec_info->aec_config->ns_level);							//建议取值范围1~8；值越小底噪越小
	aec_ctrl(temp_aec_info->aec, AEC_CTRL_CMD_SET_NS_PARA, temp_aec_info->aec_config->ns_para);							//只能取值0,1,2; 降噪由弱到强，建议默认值
	/* drc(输出音量相关) */
	aec_ctrl(temp_aec_info->aec, AEC_CTRL_CMD_SET_DRC, temp_aec_info->aec_config->drc);									//建议取值范围0x10~0x1f;   越大输出声音越大
	LOGI("aec config:0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",
		temp_aec_info->aec_config->init_flags, //0x1f
		temp_aec_info->aec_config->mic_delay,//0x0
		temp_aec_info->aec_config->ec_depth,//0x14
		temp_aec_info->aec_config->TxRxThr,//0x1e
		temp_aec_info->aec_config->TxRxFlr,//0x6
		temp_aec_info->aec_config->ref_scale,//0x0
		temp_aec_info->aec_config->voice_vol,//0xe
		temp_aec_info->aec_config->ns_level,//0x2
		temp_aec_info->aec_config->ns_para,//0x1
		temp_aec_info->aec_config->drc//0xf
		 );

	return BK_OK;
}

static bk_err_t aud_tras_drv_aec_buff_cfg(aec_info_t *aec_info)
{
	uint16_t samp_rate_points = aec_info->samp_rate_points;

	/* malloc aec ref ring buffer to save ref data */
	LOGI("%s, %d, ref_ring_buff size: %d \n", __func__, __LINE__, samp_rate_points*2*2);
	aec_info->aec_ref_ring_buff = (int16_t *)audio_tras_drv_malloc(samp_rate_points*2*2 + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL);
	if (aec_info->aec_ref_ring_buff == NULL) {
		LOGE("%s, %d, malloc ref ring buffer fail \n", __func__, __LINE__);
		goto aud_tras_drv_aec_buff_cfg_exit;
	}

	/* malloc aec out ring buffer to save mic data has been aec processed */
	aec_info->aec_out_ring_buff = (int16_t *)audio_tras_drv_malloc(samp_rate_points*2*2 + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL);
	if (aec_info->aec_out_ring_buff == NULL) {
		LOGE("%s, %d, malloc aec out ring buffer fail \n", __func__, __LINE__);
		goto aud_tras_drv_aec_buff_cfg_exit;
	}

	/* init ref_ring_buff */
	ring_buffer_init(&(aec_info->ref_rb), (uint8_t*)aec_info->aec_ref_ring_buff, samp_rate_points*2*2 + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL, DMA_ID_MAX, RB_DMA_TYPE_NULL);

	/* init aec_ring_buff */
	ring_buffer_init(&(aec_info->aec_rb), (uint8_t*)aec_info->aec_out_ring_buff, samp_rate_points*2*2 + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL, DMA_ID_MAX, RB_DMA_TYPE_NULL);

	return BK_OK;

aud_tras_drv_aec_buff_cfg_exit:
	if (aec_info->aec_ref_ring_buff != NULL) {
		audio_tras_drv_free(aec_info->aec_ref_ring_buff);
		aec_info->aec_ref_ring_buff = NULL;
	}
	if (aec_info->aec_out_ring_buff != NULL) {
		audio_tras_drv_free(aec_info->aec_out_ring_buff);
		aec_info->aec_out_ring_buff = NULL;
	}
	return BK_FAIL;
}

static void aud_tras_drv_aec_decfg(void)
{
	if (aud_tras_drv_info.voc_info.aec_enable) {
		if (aud_tras_drv_info.voc_info.aec_info->aec) {
			audio_tras_drv_free(aud_tras_drv_info.voc_info.aec_info->aec);
			aud_tras_drv_info.voc_info.aec_info->aec = NULL;
		}

		if (aud_tras_drv_info.voc_info.aec_info->aec_config) {
			audio_tras_drv_free(aud_tras_drv_info.voc_info.aec_info->aec_config);
			aud_tras_drv_info.voc_info.aec_info->aec_config = NULL;
		}

		ring_buffer_clear(&(aud_tras_drv_info.voc_info.aec_info->ref_rb));
		ring_buffer_clear(&(aud_tras_drv_info.voc_info.aec_info->aec_rb));

		if (aud_tras_drv_info.voc_info.aec_info->aec_ref_ring_buff) {
			audio_tras_drv_free(aud_tras_drv_info.voc_info.aec_info->aec_ref_ring_buff);
			aud_tras_drv_info.voc_info.aec_info->aec_ref_ring_buff = NULL;
		}

		if (aud_tras_drv_info.voc_info.aec_info->aec_out_ring_buff) {
			audio_tras_drv_free(aud_tras_drv_info.voc_info.aec_info->aec_out_ring_buff);
			aud_tras_drv_info.voc_info.aec_info->aec_out_ring_buff = NULL;
		}

		if (aud_tras_drv_info.voc_info.aec_info) {
			audio_tras_drv_free(aud_tras_drv_info.voc_info.aec_info);
			aud_tras_drv_info.voc_info.aec_info = NULL;
		}
	} else {
		aud_tras_drv_info.voc_info.aec_info = NULL;
	}

	aud_tras_drv_info.voc_info.aec_enable = false;
}

/* 搬运audio adc 采集到的一帧mic和ref信号后，触发中断通知AEC处理数据 */
static void aud_tras_adc_dma_finish_isr(void)
{
	bk_err_t ret = BK_OK;
//	GPIO_UP(3);

	if (aud_tras_drv_info.work_mode == AUD_INTF_WORK_MODE_GENERAL) {
		ret = aud_tras_drv_send_msg(AUD_TRAS_DRV_MIC_TX_DATA, NULL);
	} else {
		/* send msg to AEC or ENCODER to process mic data */
		if (aud_tras_drv_info.voc_info.aec_enable)
			ret = aud_tras_drv_send_msg(AUD_TRAS_DRV_AEC, NULL);
		else
			ret = aud_tras_drv_send_msg(AUD_TRAS_DRV_ENCODER, NULL);
	}
	if (ret != kNoErr) {
		LOGE("%s, %d, send msg: AUD_TRAS_DRV_AEC fail \n", __func__, __LINE__);
	}
//	GPIO_DOWN(3);
}

static bk_err_t aud_tras_adc_dma_config(dma_id_t dma_id, int32_t *ring_buff_addr, uint32_t ring_buff_size, uint32_t transfer_len, aud_intf_mic_chl_t mic_chl)
{
	bk_err_t ret = BK_OK;
	dma_config_t dma_config;
	uint32_t adc_port_addr;

	dma_config.mode = DMA_WORK_MODE_REPEAT;
	dma_config.chan_prio = 1;
	dma_config.src.dev = DMA_DEV_AUDIO_RX;
	dma_config.dst.dev = DMA_DEV_DTCM;
	switch (mic_chl) {
		case AUD_INTF_MIC_CHL_MIC1:
			dma_config.src.width = DMA_DATA_WIDTH_16BITS;
			break;

		case AUD_INTF_MIC_CHL_DUAL:
			dma_config.src.width = DMA_DATA_WIDTH_32BITS;
			break;

		default:
			break;
	}
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;

	/* get adc fifo address */
	if (bk_aud_adc_get_fifo_addr(&adc_port_addr) != BK_OK) {
		LOGE("%s, %d, get adc fifo address fail \n", __func__, __LINE__);
		return BK_ERR_AUD_INTF_ADC;
	} else {
		dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
		dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
		dma_config.src.start_addr = adc_port_addr;
		dma_config.src.end_addr = adc_port_addr + 4;
	}

	dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
	dma_config.dst.start_addr = (uint32_t)ring_buff_addr;
	dma_config.dst.end_addr = (uint32_t)ring_buff_addr + ring_buff_size;

	/* init dma channel */
	ret = bk_dma_init(dma_id, &dma_config);
	if (ret != BK_OK) {
		LOGE("%s, %d, audio adc dma channel init fail \n", __func__, __LINE__);
		return BK_ERR_AUD_INTF_DMA;
	}

	/* set dma transfer length */
	bk_dma_set_transfer_len(dma_id, transfer_len);

	//register isr
	bk_dma_register_isr(dma_id, NULL, (void *)aud_tras_adc_dma_finish_isr);
	bk_dma_enable_finish_interrupt(dma_id);
#if (CONFIG_SPE)
	BK_LOG_ON_ERR(bk_dma_set_dest_sec_attr(dma_id, DMA_ATTR_SEC));
	BK_LOG_ON_ERR(bk_dma_set_src_sec_attr(dma_id, DMA_ATTR_SEC));
#endif

	return BK_ERR_AUD_INTF_OK;
}

/* 搬运audio dac 一帧dac信号后，触发中断通知decoder处理数据 */
static void aud_tras_dac_dma_finish_isr(void)
{
	bk_err_t ret = BK_OK;

	if (aud_tras_drv_info.work_mode == AUD_INTF_WORK_MODE_GENERAL)
		/* send msg to notify app to write speaker data */
		ret = aud_tras_drv_send_msg(AUD_TRAS_DRV_SPK_REQ_DATA, NULL);
	else
		/* send msg to decoder to decoding recevied data */
		ret = aud_tras_drv_send_msg(AUD_TRAS_DRV_DECODER, NULL);
	if (ret != kNoErr) {
		LOGE("%s, %d, dac send msg: AUD_TRAS_DRV_DECODER fail \n", __func__, __LINE__);
	}
}

static bk_err_t aud_tras_dac_dma_config(dma_id_t dma_id, int32_t *ring_buff_addr, uint32_t ring_buff_size, uint32_t transfer_len, aud_intf_spk_chl_t spk_chl)
{
	bk_err_t ret = BK_OK;
	dma_config_t dma_config;
	uint32_t dac_port_addr;

	dma_config.mode = DMA_WORK_MODE_REPEAT;
	dma_config.chan_prio = 1;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.dst.dev = DMA_DEV_AUDIO;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	switch (spk_chl) {
		case AUD_INTF_SPK_CHL_LEFT:
			dma_config.dst.width = DMA_DATA_WIDTH_16BITS;
			break;

		case AUD_INTF_SPK_CHL_DUAL:
			dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
			break;

		default:
			break;
	}

	/* get dac fifo address */
	if (bk_aud_dac_get_fifo_addr(&dac_port_addr) != BK_OK) {
		LOGE("%s, %d, get dac fifo address fail \n", __func__, __LINE__);
		return BK_FAIL;
	} else {
		dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
		dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
		dma_config.dst.start_addr = dac_port_addr;
		dma_config.dst.end_addr = dac_port_addr + 4;
	}
	dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
	dma_config.src.start_addr = (uint32_t)ring_buff_addr;
	dma_config.src.end_addr = (uint32_t)(ring_buff_addr) + ring_buff_size;

	/* init dma channel */
	ret = bk_dma_init(dma_id, &dma_config);
	if (ret != BK_OK) {
		LOGE("%s, %d, audio dac dma channel init fail \n", __func__, __LINE__);
		return BK_FAIL;
	}

	/* set dma transfer length */
	bk_dma_set_transfer_len(dma_id, transfer_len);

	//register isr
	bk_dma_register_isr(dma_id, NULL, (void *)aud_tras_dac_dma_finish_isr);
	bk_dma_enable_finish_interrupt(dma_id);
#if (CONFIG_SPE)
	BK_LOG_ON_ERR(bk_dma_set_dest_sec_attr(dma_id, DMA_ATTR_SEC));
	BK_LOG_ON_ERR(bk_dma_set_src_sec_attr(dma_id, DMA_ATTR_SEC));
#endif

	return BK_OK;
}

static bk_err_t aud_tras_aec(void)
{
	bk_err_t ret = BK_OK;
	uint32_t size = 0;
	uint32 i=0;
	static int16_t temp_buf[640];
	static int16_t temp_ref_buf[640];

	if (aud_tras_drv_info.voc_info.status == AUD_TRAS_DRV_VOC_STA_NULL)
		return BK_OK;

	aec_info_t *aec_info_pr = aud_tras_drv_info.voc_info.aec_info;

#if (CONFIG_CACHE_ENABLE)
	flush_all_dcache();
#endif

	/* get a fram mic data from mic_ring_buff */
	if (ring_buffer_get_fill_size(&(aud_tras_drv_info.voc_info.mic_rb)) >= aec_info_pr->samp_rate_points*4) {
		size = ring_buffer_read(&(aud_tras_drv_info.voc_info.mic_rb), (uint8_t*)temp_buf, aec_info_pr->samp_rate_points*4);
		if (size != aec_info_pr->samp_rate_points*4) {
			LOGE("%s, %d, read mic_ring_buff fail, size:%d \n", __func__, __LINE__, size);
			//return BK_FAIL;
		}
		for(i=0;i<320;i++)
		{
			aec_info_pr->mic_addr[i]=temp_buf[2*i];
            // Modified by TUYA Start
            if (aud_tras_drv_info.voc_info.mic2_disable) {
                aec_info_pr->ref_addr[i] = 0x00;
            } else {
                aec_info_pr->ref_addr[i]=temp_buf[2*i+1];
            }
            // Modified by TUYA End
		}
	} else {
		LOGD("%s, %d, do not have mic data need to aec \n", __func__, __LINE__);
		return BK_OK;
	}


	if (aud_tras_drv_info.voc_info.spk_en == AUD_INTF_VOC_SPK_CLOSE || aud_tras_drv_info.voc_info.aec_enable == false) {
		/* save mic data after aec processed to aec_ring_buffer */
		if (ring_buffer_get_free_size(&(aec_info_pr->aec_rb)) >= aec_info_pr->samp_rate_points*2) {
			size = ring_buffer_write(&(aec_info_pr->aec_rb), (uint8_t*)aec_info_pr->mic_addr, aec_info_pr->samp_rate_points*2);
			if (size != aec_info_pr->samp_rate_points*2) {
				LOGE("%s, %d, the data write to aec_ring_buff is not a frame \n", __func__, __LINE__);
				//return BK_FAIL;
			}
		}

		/* send msg to encoder to encoding data */
		ret = aud_tras_drv_send_msg(AUD_TRAS_DRV_ENCODER, NULL);
		if (ret != kNoErr) {
			LOGE("%s, %d, send msg: AUD_TRAS_DRV_ENCODER fail \n", __func__, __LINE__);
			return BK_FAIL;
		}

		return BK_OK;
	}

	/* read ref data from ref_ring_buff */
	if (ring_buffer_get_fill_size(&(aec_info_pr->ref_rb)) >= aec_info_pr->samp_rate_points*2) {
		size = ring_buffer_read(&(aec_info_pr->ref_rb), (uint8_t*)temp_ref_buf, aec_info_pr->samp_rate_points*2);
		if (size != aec_info_pr->samp_rate_points*2) {
			LOGE("%s, %d, the ref data readed from ref_ring_buff is not a frame \n", __func__, __LINE__);
			//return BK_FAIL;
			//os_memset(ref_addr, 0, frame_sample*2);
		}
	} else {
		//LOGE("no ref data \n");
		os_memset((void *)aec_info_pr->ref_addr, 0, aec_info_pr->samp_rate_points*2);
	}
	//os_printf("ref_addr: ref_addr[0]= %02x, ref_addr[0]= %02x \r\n", ref_addr[0], ref_addr[1]);

	if (aud_tras_drv_info.voc_info.aud_tras_dump_aec_cb) {
		aud_tras_drv_info.voc_info.aud_tras_dump_aec_cb((uint8_t *)aec_info_pr->mic_addr, aec_info_pr->samp_rate_points*2);
		aud_tras_drv_info.voc_info.aud_tras_dump_aec_cb((uint8_t *)aec_info_pr->ref_addr, aec_info_pr->samp_rate_points*2);
	}

#if CONFIG_AUD_TRAS_AEC_DUMP_DEBUG
#if CONFIG_AUD_TRAS_AEC_DUMP_MODE_TF
	os_memcpy((void *)aud_tras_drv_info.voc_info.aec_dump.mic_dump_addr, aec_info_pr->mic_addr, aec_info_pr->samp_rate_points*2);
	os_memcpy((void *)aud_tras_drv_info.voc_info.aec_dump.ref_dump_addr, aec_info_pr->ref_addr, aec_info_pr->samp_rate_points*2);
	//os_printf("memcopy complete \r\n");
#endif //CONFIG_AUD_TRAS_AEC_DUMP_MODE_TF

#if CONFIG_AUD_TRAS_AEC_DUMP_MODE_UART
	bk_uart_write_bytes(CONFIG_AUD_TRAS_AEC_DUMP_UART_ID, aec_info_pr->mic_addr, aec_info_pr->samp_rate_points*2);
	bk_uart_write_bytes(CONFIG_AUD_TRAS_AEC_DUMP_UART_ID, aec_info_pr->ref_addr, aec_info_pr->samp_rate_points*2);
#endif

#if CONFIG_AUD_TRAS_AEC_DUMP_MODE_UDP
	size = bk_aud_debug_voc_udp_send_packet((unsigned char *)aec_info_pr->mic_addr, aec_info_pr->samp_rate_points*2);
	if (size != aec_info_pr->samp_rate_points*2)
		os_printf("%s, %d, udp dump mic fail, all:%d, complete:%d \n", __func__, __LINE__, aec_info_pr->samp_rate_points*2, size);
	size = bk_aud_debug_voc_udp_send_packet((unsigned char *)aec_info_pr->ref_addr, aec_info_pr->samp_rate_points*2);
	if (size != aec_info_pr->samp_rate_points*2)
		os_printf("%s, %d, udp dump ref fail, all:%d, complete:%d \n", __func__, __LINE__, aec_info_pr->samp_rate_points*2, size);
#endif
#if CONFIG_AUD_TRAS_AEC_DUMP_MODE_TCP
	size = bk_aud_debug_voc_tcp_send_packet((unsigned char *)aec_info_pr->mic_addr, aec_info_pr->samp_rate_points*2);
	if (size != aec_info_pr->samp_rate_points*2)
		os_printf("%s, %d, tcp dump mic fail, all:%d, complete:%d \n", __func__, __LINE__, aec_info_pr->samp_rate_points*2, size);
	size = bk_aud_debug_voc_tcp_send_packet((unsigned char *)aec_info_pr->ref_addr, aec_info_pr->samp_rate_points*2);
	if (size != aec_info_pr->samp_rate_points*2)
		os_printf("%s, %d, tcp dump ref fail, all:%d, complete:%d \n", __func__, __LINE__, aec_info_pr->samp_rate_points*2, size);
#endif

#endif //CONFIG_AUD_TRAS_AEC_DUMP_DEBUG

#if MIC_DEBUG
    int handle_len = aec_info_pr->samp_rate_points*2;
    if(g_mic_data_save_flag)
    {
        if((g_recv_len+handle_len) > PSRAM_BUFF_SIZE)
        {
            bk_printf("[%s]dialog_porting overflow\r\n",__func__);
        }
        else
        {
            os_memcpy(g_mic_psram_buff+g_recv_len, aec_info_pr->mic_addr, handle_len);
        }
    }
#endif
#if MIC_DEBUG
    if(g_mic_data_save_flag)
    {
        if((g_recv_len+handle_len) > PSRAM_BUFF_SIZE)
        {
            bk_printf("[%s]dialog_porting overflow\r\n",__func__);
        }
        else
        {
            os_memcpy(g_ref_psram_buff+g_recv_len, aec_info_pr->ref_addr, handle_len);
        }
    }
#endif
	/* aec process data */
	//os_printf("ref_addr:%p, mic_addr:%p, out_addr:%p \r\n", aec_context_pr->ref_addr, aec_context_pr->mic_addr, aec_context_pr->out_addr);
	aec_proc(aec_info_pr->aec, aec_info_pr->ref_addr, aec_info_pr->mic_addr, aec_info_pr->out_addr);
#if MIC_DEBUG
    if(g_mic_data_save_flag)
    {
        if((g_recv_len+handle_len) > PSRAM_BUFF_SIZE)
        {
            bk_printf("[%s]dialog_porting overflow\r\n",__func__);
        }
        else
        {
            os_memcpy(g_out_psram_buff+g_recv_len, aec_info_pr->out_addr, handle_len);
            g_recv_len += handle_len;
        }
    }
#endif

	if (aud_tras_drv_info.voc_info.aud_tras_dump_aec_cb) {
		aud_tras_drv_info.voc_info.aud_tras_dump_aec_cb((uint8_t *)aec_info_pr->out_addr, aec_info_pr->samp_rate_points*2);
	}

#if CONFIG_AUD_TRAS_AEC_DUMP_DEBUG
#if CONFIG_AUD_TRAS_AEC_DUMP_MODE_TF
	os_memcpy((void *)aud_tras_drv_info.voc_info.aec_dump.out_dump_addr, aec_info_pr->out_addr, aec_info_pr->samp_rate_points*2);
#endif //CONFIG_AUD_TRAS_AEC_DUMP_MODE_TF

#if CONFIG_AUD_TRAS_AEC_DUMP_MODE_UART
	bk_uart_write_bytes(CONFIG_AUD_TRAS_AEC_DUMP_UART_ID, aec_info_pr->out_addr, aec_info_pr->samp_rate_points*2);
#endif

#if CONFIG_AUD_TRAS_AEC_DUMP_MODE_UDP
	size = bk_aud_debug_voc_udp_send_packet((unsigned char *)aec_info_pr->out_addr, aec_info_pr->samp_rate_points*2);
	if (size != aec_info_pr->samp_rate_points*2)
		os_printf("%s, %d, udp dump aec out fail, all:%d, complete:%d \r\n", __func__, __LINE__, aec_info_pr->samp_rate_points*2, size);
#endif
#if CONFIG_AUD_TRAS_AEC_DUMP_MODE_TCP
	size = bk_aud_debug_voc_tcp_send_packet((unsigned char *)aec_info_pr->out_addr, aec_info_pr->samp_rate_points*2);
	if (size != aec_info_pr->samp_rate_points*2)
		os_printf("%s, %d, tcp dump aec out fail, all:%d, complete:%d \r\n", __func__, __LINE__, aec_info_pr->samp_rate_points*2, size);
#endif
#endif //CONFIG_AUD_TRAS_AEC_DUMP_DEBUG

	/* save mic data after aec processed to aec_ring_buffer */
	if (ring_buffer_get_free_size(&(aec_info_pr->aec_rb)) >= aec_info_pr->samp_rate_points*2) {
		size = ring_buffer_write(&(aec_info_pr->aec_rb), (uint8_t*)aec_info_pr->out_addr, aec_info_pr->samp_rate_points*2);
		if (size != aec_info_pr->samp_rate_points*2) {
			LOGE("%s, %d, the data writeten to aec_ring_buff is not a frame \n", __func__, __LINE__);
			//return BK_FAIL;
		}
	}

	/* send msg to encoder to encoding data */
	ret = aud_tras_drv_send_msg(AUD_TRAS_DRV_ENCODER, NULL);
	if (ret != kNoErr) {
		LOGE("%s, %d, send msg: AUD_TRAS_DRV_ENCODER fail \n", __func__, __LINE__);
		return BK_FAIL;
	}

	return ret;
}

static audio_packet_t *aud_uac_malloc_packet(uint32_t dev)
{
	audio_packet_t *packet = NULL;

	switch(dev)
	{
		case USB_UAC_SPEAKER_DEVICE:
			aud_tras_drv_info.spk_info.uac_spk_packet.dev = USB_UAC_MIC_DEVICE;
			aud_tras_drv_info.spk_info.uac_spk_packet.data_buffer = NULL;
			aud_tras_drv_info.spk_info.uac_spk_packet.num_packets = 2;
			aud_tras_drv_info.spk_info.uac_spk_packet.data_buffer_size = aud_tras_drv_info.spk_info.frame_size * 2;
			aud_tras_drv_info.spk_info.uac_spk_packet.state = NULL;
			aud_tras_drv_info.spk_info.uac_spk_packet.num_byte = NULL;
			aud_tras_drv_info.spk_info.uac_spk_packet.actual_num_byte = NULL;
			packet = &aud_tras_drv_info.spk_info.uac_spk_packet;
			break;
		case USB_UAC_MIC_DEVICE:
			aud_tras_drv_info.mic_info.uac_mic_packet.dev = USB_UAC_MIC_DEVICE;
			aud_tras_drv_info.mic_info.uac_mic_packet.data_buffer = NULL;
			aud_tras_drv_info.mic_info.uac_mic_packet.num_packets = 2;
			aud_tras_drv_info.mic_info.uac_mic_packet.data_buffer_size = aud_tras_drv_info.mic_info.frame_size * 2;
			aud_tras_drv_info.mic_info.uac_mic_packet.state = NULL;
			aud_tras_drv_info.mic_info.uac_mic_packet.num_byte = NULL;
			aud_tras_drv_info.mic_info.uac_mic_packet.actual_num_byte = NULL;
			packet = &aud_tras_drv_info.mic_info.uac_mic_packet;
			break;
		default:
			break;
	}

	return packet;
}

static void aud_uac_push_packet(audio_packet_t *packet)
{
	bk_err_t ret = BK_OK;

	if (aud_tras_drv_info.mic_info.status == AUD_TRAS_DRV_MIC_STA_START && packet->data_buffer_size > 0) {
		if (ring_buffer_get_free_size(&aud_tras_drv_info.mic_info.mic_rb) >= packet->data_buffer_size) {
			ring_buffer_write(&aud_tras_drv_info.mic_info.mic_rb, (uint8_t *)packet->data_buffer, packet->data_buffer_size);
		}
	}

	/* send msg to TX_DATA to process mic data */
	ret = aud_tras_drv_send_msg(AUD_TRAS_DRV_MIC_TX_DATA, NULL);
	if (ret != kNoErr) {
		LOGE("%s, %d, send msg: AUD_TRAS_DRV_MIC_TX_DATA fail \n", __func__, __LINE__);
	}

	return;
}

static void aud_uac_free_packet(audio_packet_t *packet)
{
	bk_err_t ret = BK_OK;

	if (packet->data_buffer_size == aud_tras_drv_info.spk_info.frame_size) {
		os_memcpy(packet->data_buffer, aud_tras_drv_info.spk_info.uac_spk_buff, packet->data_buffer_size);
		os_memset(aud_tras_drv_info.spk_info.uac_spk_buff, 0x00, aud_tras_drv_info.spk_info.frame_size);
	}

	/* send msg to notify app to write speaker data */
	ret = aud_tras_drv_send_msg(AUD_TRAS_DRV_SPK_REQ_DATA, (void *)packet);
	if (ret != kNoErr) {
		LOGE("%s, %d, send msg: AUD_TRAS_DRV_SPK_REQ_DATA fail \n", __func__, __LINE__);
	}

	return;
}

static const struct audio_packet_control_t aud_uac_buffer_ops_funcs = {
    ._uac_malloc = aud_uac_malloc_packet,
    ._uac_push = aud_uac_push_packet,
    ._uac_pop = NULL,
    ._uac_free = aud_uac_free_packet,
};

static audio_packet_t *voc_uac_malloc_packet(uint32_t dev)
{
	audio_packet_t *packet = NULL;

	switch(dev)
	{
		case USB_UAC_SPEAKER_DEVICE:
			aud_tras_drv_info.voc_info.uac_spk_packet.dev = USB_UAC_MIC_DEVICE;
			aud_tras_drv_info.voc_info.uac_spk_packet.data_buffer = NULL;
			aud_tras_drv_info.voc_info.uac_spk_packet.num_packets = 2;
			aud_tras_drv_info.voc_info.uac_spk_packet.data_buffer_size = aud_tras_drv_info.voc_info.speaker_samp_rate_points * 2 * 2;
			aud_tras_drv_info.voc_info.uac_spk_packet.state = NULL;
			aud_tras_drv_info.voc_info.uac_spk_packet.num_byte = NULL;
			aud_tras_drv_info.voc_info.uac_spk_packet.actual_num_byte = NULL;
			packet = &aud_tras_drv_info.voc_info.uac_spk_packet;
			break;
		case USB_UAC_MIC_DEVICE:
			aud_tras_drv_info.voc_info.uac_mic_packet.dev = USB_UAC_MIC_DEVICE;
			aud_tras_drv_info.voc_info.uac_mic_packet.data_buffer = NULL;
			aud_tras_drv_info.voc_info.uac_mic_packet.num_packets = 2;
			aud_tras_drv_info.voc_info.uac_mic_packet.data_buffer_size = aud_tras_drv_info.voc_info.mic_samp_rate_points * 2 * 2;
			aud_tras_drv_info.voc_info.uac_mic_packet.state = NULL;
			aud_tras_drv_info.voc_info.uac_mic_packet.num_byte = NULL;
			aud_tras_drv_info.voc_info.uac_mic_packet.actual_num_byte = NULL;
			packet = &aud_tras_drv_info.voc_info.uac_mic_packet;
			break;
		default:
			break;
	}

	return packet;
}

static void voc_uac_push_packet(audio_packet_t *packet)
{
//	LOGI("%s size %x\r\n",__func__, packet->data_buffer_size);
	bk_err_t ret = BK_OK;

#ifdef CONFIG_UAC_MIC_SPK_COUNT_DEBUG
	uac_mic_spk_count_add_mic_size(packet->data_buffer_size);
#endif

	if (aud_tras_drv_info.voc_info.status == AUD_TRAS_DRV_VOC_STA_START && packet->data_buffer_size > 0) {
		if (ring_buffer_get_free_size(&aud_tras_drv_info.voc_info.mic_rb) >= packet->data_buffer_size) {
			ring_buffer_write(&aud_tras_drv_info.voc_info.mic_rb, (uint8_t *)packet->data_buffer, packet->data_buffer_size);
		}
	}

	/* send msg to TX_DATA to process mic data */
	if (aud_tras_drv_info.voc_info.aec_enable) {
		ret = aud_tras_drv_send_msg(AUD_TRAS_DRV_AEC, NULL);
		//LOGI("%s AUD_TRAS_DRV_AEC\r\n",__func__);
	} else {
		ret = aud_tras_drv_send_msg(AUD_TRAS_DRV_ENCODER, NULL);
		//LOGI("%s AUD_TRAS_DRV_ENCODER\r\n",__func__);
	}
	if (ret != kNoErr) {
		LOGE("%s, %d, send msg: AUD_TRAS_DRV_ENCODER fail \n", __func__, __LINE__);
	}

	return;
}

static void voc_uac_free_packet(audio_packet_t *packet)
{
	bk_err_t ret = BK_OK;
//	LOGI("%s size %x\r\n",__func__, packet->data_buffer_size);

	/* check status and size */
	if (aud_tras_drv_info.voc_info.status == AUD_TRAS_DRV_VOC_STA_START && aud_tras_drv_info.voc_info.speaker_samp_rate_points*2 == packet->data_buffer_size) {
		os_memcpy(packet->data_buffer, aud_tras_drv_info.voc_info.uac_spk_buff, packet->data_buffer_size);

#if 0
		bk_uart_write_bytes(UART_ID_2, packet->data_buffer, packet->data_buffer_size);
#endif

		os_memset(aud_tras_drv_info.voc_info.uac_spk_buff, 0x00, aud_tras_drv_info.voc_info.speaker_samp_rate_points*2);

#ifdef CONFIG_UAC_MIC_SPK_COUNT_DEBUG
		uac_mic_spk_count_add_spk_size(packet->data_buffer_size);
#endif
	}

	/* send msg to notify app to write speaker data */
	ret = aud_tras_drv_send_msg(AUD_TRAS_DRV_DECODER, (void *)packet);
	if (ret != kNoErr) {
		LOGE("%s, %d, send msg: AUD_TRAS_DRV_DECODER fails \n", __func__, __LINE__);
	}

//	memset(packet->data_buffer, 0x11, packet->data_buffer_size);
	return;
}

static const struct audio_packet_control_t voc_uac_buffer_ops_funcs = {
    ._uac_malloc = voc_uac_malloc_packet,
    ._uac_push = voc_uac_push_packet,
    ._uac_pop = NULL,
    ._uac_free = voc_uac_free_packet,
};

static bk_err_t aud_tras_uac_auto_connect_ctrl(bool en)
{
	aud_tras_drv_info.uac_auto_connect = en;

	return BK_ERR_AUD_INTF_OK;
}

static bk_err_t aud_tras_uac_disconnect_handle(void)
{
//	LOGI("enter %s \n", __func__);

	/* notify app that uac disconnecting */
	if (aud_tras_drv_info.uac_connect_state_cb_exist && aud_tras_drv_info.uac_status == AUD_INTF_UAC_ABNORMAL_DISCONNECTED) {
		uac_connect_state_msg.event = EVENT_UAC_CONNECT_STATE_NOTIFY;
		uac_connect_state_msg.param = AUD_INTF_UAC_ABNORMAL_DISCONNECTED;
		uac_connect_state_msg.sem = NULL;
		msg_send_notify_to_media_major_mailbox(&uac_connect_state_msg, APP_MODULE);
	}

	/* reset mic and spk current status */
	aud_tras_drv_info.uac_mic_open_current = false;
	aud_tras_drv_info.uac_spk_open_current = false;

	return BK_OK;
}

static void aud_tras_uac_disconnect_cb(void)
{
	uint8_t count = 6;
	bk_err_t ret = BK_OK;

	LOGI("%s \n", __func__);

	if (aud_tras_drv_info.uac_status != AUD_INTF_UAC_NORMAL_DISCONNECTED) {
		aud_tras_drv_info.uac_status = AUD_INTF_UAC_ABNORMAL_DISCONNECTED;
		do {
			if (count == 0)
				break;
			count--;
			ret = aud_tras_drv_send_msg(AUD_TRAS_DRV_UAC_DISCONT, NULL);
			if (ret != BK_OK) {
				LOGE("%s, %d, send msg: AUD_TRAS_DRV_UAC_DISCONT fail: %d \n", __func__, __LINE__, count);
				rtos_delay_milliseconds(20);
			}
		} while (ret != BK_OK);
	}
}

static bk_err_t aud_tras_uac_connect_handle(void)
{
	bk_err_t ret = BK_OK;
	bk_err_t err = BK_OK;

	/* notify app that uac disconnecting */
	if (aud_tras_drv_info.uac_connect_state_cb_exist) {
		uac_connect_state_msg.event = EVENT_UAC_CONNECT_STATE_NOTIFY;
		uac_connect_state_msg.param = AUD_INTF_UAC_CONNECTED;
		uac_connect_state_msg.sem = NULL;
		msg_send_notify_to_media_major_mailbox(&uac_connect_state_msg, APP_MODULE);
	}

	/* check Automatic recover uac connect */
	if (aud_tras_drv_info.uac_status == AUD_INTF_UAC_ABNORMAL_DISCONNECTED) {
		/* uac automatically connect */
		if (!aud_tras_drv_info.uac_auto_connect) {
			LOGI("%s, %d, uac not automatically connect, need user todo \n", __func__, __LINE__);
			return BK_OK;
		}
	}

	if (aud_tras_drv_info.voc_info.status == AUD_TRAS_DRV_VOC_STA_NULL) {
		return BK_OK;
	}

	/* config uac */
	LOGI("%s, %d, config uac \n", __func__, __LINE__);
	if (aud_tras_drv_info.work_mode == AUD_INTF_WORK_MODE_GENERAL) {
		if (aud_tras_drv_info.mic_info.status != AUD_TRAS_DRV_MIC_STA_NULL && aud_tras_drv_info.mic_info.mic_type == AUD_INTF_MIC_TYPE_UAC) {
			ret = bk_aud_uac_mic_set_param(aud_tras_drv_info.mic_info.uac_mic_config);
			if (ret != BK_OK) {
				LOGE("%s, %d, uac set mic param fail \n", __func__, __LINE__);
				err = BK_ERR_AUD_INTF_UAC_MIC;
				goto fail;
			}
		}

		if (aud_tras_drv_info.spk_info.status != AUD_TRAS_DRV_SPK_STA_NULL && aud_tras_drv_info.spk_info.spk_type == AUD_INTF_SPK_TYPE_UAC) {
			ret = bk_aud_uac_spk_set_param(aud_tras_drv_info.spk_info.uac_spk_config);
			if (ret != BK_OK) {
				LOGE("%s, %d, uac set spk param fail \n", __func__, __LINE__);
				err = BK_ERR_AUD_INTF_UAC_SPK;
				goto fail;
			}
		}
	} else {
		ret = bk_aud_uac_set_param(aud_tras_drv_info.voc_info.uac_config);
		if (ret != BK_OK) {
			LOGE("%s, %d, uac set param fail \n", __func__, __LINE__);
			err = BK_ERR_AUD_INTF_UAC_DRV;
			goto fail;
		}
	}

	LOGI("%s, %d, voc_ops: %p, malloc: %p, push: %p, free: %p \n", __func__, __LINE__, &voc_uac_buffer_ops_funcs, voc_uac_buffer_ops_funcs._uac_malloc, voc_uac_buffer_ops_funcs._uac_push, voc_uac_buffer_ops_funcs._uac_free);

	if (aud_tras_drv_info.work_mode == AUD_INTF_WORK_MODE_GENERAL) {
		ret = bk_aud_uac_register_transfer_buffer_ops((void *)&aud_uac_buffer_ops_funcs);
	} else {
		ret = bk_aud_uac_register_transfer_buffer_ops((void *)&voc_uac_buffer_ops_funcs);
	}
	if (ret != BK_OK) {
		LOGE("%s, %d, register transfer buffer ops fail \n", __func__, __LINE__);
		err = BK_ERR_AUD_INTF_UAC_DRV;
		goto fail;
	}

	if (aud_tras_drv_info.uac_mic_open_status == true && aud_tras_drv_info.uac_mic_open_current == false) {
		ret = bk_aud_uac_start_mic();
		if (ret != BK_OK) {
			LOGE("%s, %d, start uac mic fail, ret:%d \n", __func__, __LINE__, ret);
			err = BK_ERR_AUD_INTF_UAC_MIC;
			goto fail;
		} else {
			aud_tras_drv_info.uac_mic_open_current = true;
		}
	}

	if (aud_tras_drv_info.uac_spk_open_status == true && aud_tras_drv_info.uac_spk_open_current == false) {
		ret = bk_aud_uac_start_spk();
		if (ret != BK_OK) {
			LOGE("%s, %d, start uac spk fail, ret:%d \n", __func__, __LINE__, ret);
			err = BK_ERR_AUD_INTF_UAC_SPK;
			goto fail;
		} else {
			aud_tras_drv_info.uac_spk_open_current = true;
		}
	}

	return ret;

fail:

	return err;
}


static void aud_tras_uac_connect_cb(void)
{
	uint8_t count = 6;
	bk_err_t ret = BK_OK;

	LOGI("%s \n", __func__);

	if (aud_tras_drv_info.uac_status != AUD_INTF_UAC_CONNECTED) {
		aud_tras_drv_info.uac_status = AUD_INTF_UAC_CONNECTED;
		do {
			if (count == 0)
				break;
			count--;
			ret = aud_tras_drv_send_msg(AUD_TRAS_DRV_UAC_CONT, NULL);
			if (ret != BK_OK) {
				LOGE("%s, %d, send msg: AUD_TRAS_DRV_UAC_CONT fail: %d \n", __func__, __LINE__, count);
				rtos_delay_milliseconds(20);
			}
		} while (ret != BK_OK);
	}
}

static bk_err_t aud_tras_enc(void)
{
	bk_err_t ret = BK_OK;
	uint32_t size = 0;
	uint32_t i = 0;
	uint16_t temp_mic_samp_rate_points = aud_tras_drv_info.voc_info.mic_samp_rate_points;
	tx_info_t temp_tx_info;
//	uint32_t fill_size = 0;

	if (aud_tras_drv_info.voc_info.status == AUD_TRAS_DRV_VOC_STA_NULL)
		return BK_OK;

	if (aud_tras_drv_info.voc_info.mic_type == AUD_INTF_MIC_TYPE_BOARD) {
		if (aud_tras_drv_info.voc_info.aec_enable) {
			/* get data from aec_ring_buff */
			size = ring_buffer_read(&(aud_tras_drv_info.voc_info.aec_info->aec_rb), (uint8_t *)aud_tras_drv_info.voc_info.encoder_temp.pcm_data, temp_mic_samp_rate_points*2);
			if (size != temp_mic_samp_rate_points*2) {
				LOGE("%s, %d, read aec_rb :%d \n", __func__, __LINE__, size);
				os_memset(aud_tras_drv_info.voc_info.encoder_temp.pcm_data, 0, temp_mic_samp_rate_points*2);
				//goto encoder_exit;
			}
		} else {
#if (CONFIG_CACHE_ENABLE)
			flush_all_dcache();
#endif
			/* get data from mic_ring_buff */
#if 0
			size = ring_buffer_read(&(aud_tras_drv_info.voc_info.mic_rb), (uint8_t *)aud_tras_drv_info.voc_info.encoder_temp.pcm_data, temp_mic_samp_rate_points*2);
			if (size != temp_mic_samp_rate_points*2) {
				LOGE("%s, %d, read mic_rb :%d \n", __func__, __LINE__, size);
				os_memset(aud_tras_drv_info.voc_info.encoder_temp.pcm_data, 0, temp_mic_samp_rate_points*2);
				//goto encoder_exit;
			}
#else
            if (ring_buffer_get_fill_size(&(aud_tras_drv_info.voc_info.mic_rb)) >= temp_mic_samp_rate_points*4) {
                int16_t temp_buf[640];
                size = ring_buffer_read(&(aud_tras_drv_info.voc_info.mic_rb), (uint8_t *)temp_buf, temp_mic_samp_rate_points*4);
                if (size != temp_mic_samp_rate_points*4) {
                    LOGE("%s, %d, read mic_rb :%d \n", __func__, __LINE__, size);
                    os_memset(aud_tras_drv_info.voc_info.encoder_temp.pcm_data, 0, temp_mic_samp_rate_points*4);
                    //goto encoder_exit;
                }
                for (int i = 0; i < 320; i++)
                {
                    aud_tras_drv_info.voc_info.encoder_temp.pcm_data[i] = temp_buf[2 * i];
                }
            }
#endif
#if AUD_MIC_DEBUG
			os_memcpy(aud_tras_drv_info.voc_info.encoder_temp.pcm_data, PCM_8000, 320);
#endif
		}
	} else {
		if (aud_tras_drv_info.voc_info.aec_enable) {
			/* get data from aec_ring_buff */
			size = ring_buffer_read(&(aud_tras_drv_info.voc_info.aec_info->aec_rb), (uint8_t *)aud_tras_drv_info.voc_info.encoder_temp.pcm_data, temp_mic_samp_rate_points*2);
			if (size != temp_mic_samp_rate_points*2) {
				LOGE("%s, %d, read aec_rb :%d \n", __func__, __LINE__, size);
				goto encoder_exit;
			}
		} else {
			/* get data from mic_ring_buff */
			size = ring_buffer_read(&(aud_tras_drv_info.voc_info.mic_rb), (uint8_t *)aud_tras_drv_info.voc_info.encoder_temp.pcm_data, temp_mic_samp_rate_points*2);
			if (size != temp_mic_samp_rate_points*2) {
				LOGE("%s, %d, the data readed from mic_ring_buff is not a frame, size:%d \n", __func__, __LINE__, size);
				goto encoder_exit;
			}
		}
	}

	switch (aud_tras_drv_info.voc_info.data_type) {
		case AUD_INTF_VOC_DATA_TYPE_G711A:
			/* G711A encoding pcm data to a-law data*/
			for (i=0; i<temp_mic_samp_rate_points; i++) {
				aud_tras_drv_info.voc_info.encoder_temp.law_data[i] = linear2alaw(aud_tras_drv_info.voc_info.encoder_temp.pcm_data[i]);
			}
			break;

		case AUD_INTF_VOC_DATA_TYPE_G711U:
			/* G711U encoding pcm data to u-law data*/
			for (i=0; i<temp_mic_samp_rate_points; i++) {
				aud_tras_drv_info.voc_info.encoder_temp.law_data[i] = linear2ulaw(aud_tras_drv_info.voc_info.encoder_temp.pcm_data[i]);
			}
			break;

		case AUD_INTF_VOC_DATA_TYPE_PCM:
			break;

		default:
			break;
	}

	temp_tx_info = aud_tras_drv_info.voc_info.tx_info;
	switch (aud_tras_drv_info.voc_info.data_type) {
		case AUD_INTF_VOC_DATA_TYPE_G711A:
		case AUD_INTF_VOC_DATA_TYPE_G711U:
			os_memcpy(temp_tx_info.ping.buff_addr, aud_tras_drv_info.voc_info.encoder_temp.law_data, temp_mic_samp_rate_points);
			break;

		case AUD_INTF_VOC_DATA_TYPE_PCM:
			os_memcpy(temp_tx_info.ping.buff_addr, aud_tras_drv_info.voc_info.encoder_temp.pcm_data, temp_mic_samp_rate_points * 2);
			break;

		default:
			break;
	}

	/* dump tx data */
	if (aud_tras_drv_info.voc_info.aud_tras_dump_tx_cb) {
		aud_tras_drv_info.voc_info.aud_tras_dump_tx_cb((uint8_t *)temp_tx_info.ping.buff_addr, (uint32_t)temp_tx_info.buff_length);
	}

#if (CONFIG_CACHE_ENABLE)
		flush_all_dcache();
#endif

	if (aud_tras_drv_info.voc_info.aud_tx_rb) {
		int free_size = ring_buffer_get_free_size(aud_tras_drv_info.voc_info.aud_tx_rb);
		if (free_size > temp_tx_info.buff_length) {
			//GPIO_UP(4);
			ring_buffer_write(aud_tras_drv_info.voc_info.aud_tx_rb, (uint8_t *)temp_tx_info.ping.buff_addr, temp_tx_info.buff_length);
			//GPIO_DOWN(4);
		} else {
			//LOGE("aud_tx_rb free_size: %d \n", free_size);
		}
	}

#if 0
	/* send mic notify mailbox msg to media app */
	if (aud_tras_drv_info.aud_tras_tx_mic_data != NULL) {
		uint32_t result = BK_OK;
		mic_nofity.ptr_data = (uint32_t)temp_tx_info.ping.buff_addr;
		mic_nofity.length = (uint32_t)temp_tx_info.buff_length;
		mic_to_media_app_msg.event = EVENT_AUD_MIC_DATA_NOTIFY;
		mic_to_media_app_msg.param = (uint32_t)&mic_nofity;
#if AUD_MEDIA_SEM_ENABLE
		mic_to_media_app_msg.sem = mailbox_media_aud_mic_sem;
#else
		mic_to_media_app_msg.sem = NULL;
#endif
		mic_to_media_app_msg.result = (uint32_t)&result;
		//msg_send_to_media_major_mailbox(&mic_to_media_app_msg, (uint32_t)&result, APP_MODULE);

#if AUD_MEDIA_SEM_ENABLE
		ret = rtos_get_semaphore(&mailbox_media_aud_mic_sem, BEKEN_WAIT_FOREVER);
		if (ret != BK_OK)
		{
			LOGE("%s, %d, rtos_get_semaphore \n", __func__, __LINE__);
			ret = BK_FAIL;
		}
		else
		{
			ret = result;
		}
#endif
	}
#endif

	return ret;

encoder_exit:

	return BK_FAIL;
}


#if CONFIG_AUD_TRAS_DAC_DEBUG
static bk_err_t aud_tras_voc_dac_debug(bool enable)
{
	os_printf("%s, enable:%d \r\n", __func__, enable);
	if (enable == aud_voc_dac_debug_flag)
		return BK_FAIL;

	//open dac debug
	FRESULT fr;
	if (enable) {
		/*open file to save data write to speaker */
		fr = f_open(&dac_debug_file, dac_debug_file_name, FA_CREATE_ALWAYS | FA_WRITE);
		if (fr != FR_OK) {
			LOGE("open %s fail.\r\n", dac_debug_file_name);
			return BK_FAIL;
		}
		aud_voc_dac_debug_flag = true;
		os_printf("start dac debug \r\n");
	} else {
		/*open file to save data write to speaker */
		fr = f_close(&dac_debug_file);
		if (fr != FR_OK) {
			LOGE("open %s fail.\r\n", dac_debug_file_name);
			return BK_FAIL;
		}
		aud_voc_dac_debug_flag = false;
		os_printf("stop dac debug \r\n");
	}
	return BK_OK;
}
#endif

static bk_err_t aud_tras_dec(void)
{
	uint32_t size = 0;
	uint32_t i = 0;

	if (aud_tras_drv_info.voc_info.status == AUD_TRAS_DRV_VOC_STA_NULL)
		return BK_OK;

#if (CONFIG_CACHE_ENABLE)
	flush_all_dcache();
#endif

	switch (aud_tras_drv_info.voc_info.data_type) {
		case AUD_INTF_VOC_DATA_TYPE_G711A:
		case AUD_INTF_VOC_DATA_TYPE_G711U:
			/* check the frame number in decoder_ring_buffer */
			if (aud_tras_drv_info.voc_info.spk_type == AUD_INTF_SPK_TYPE_BOARD) {
				if (ring_buffer_get_fill_size(aud_tras_drv_info.voc_info.rx_info.decoder_rb) >= aud_tras_drv_info.voc_info.speaker_samp_rate_points) {
					//os_printf("decoder process \r\n", size);
					/* get G711A data from decoder_ring_buff */
					size = ring_buffer_read(aud_tras_drv_info.voc_info.rx_info.decoder_rb, (uint8_t*)aud_tras_drv_info.voc_info.decoder_temp.law_data, aud_tras_drv_info.voc_info.speaker_samp_rate_points);
					if (size != aud_tras_drv_info.voc_info.speaker_samp_rate_points) {
						LOGE("%s, %d, read decoder_ring_buff G711A data fail \n", __func__, __LINE__);
						if (aud_tras_drv_info.voc_info.data_type == AUD_INTF_VOC_DATA_TYPE_G711U)
							os_memset(aud_tras_drv_info.voc_info.decoder_temp.law_data, 0xFF, aud_tras_drv_info.voc_info.speaker_samp_rate_points);
						else
							os_memset(aud_tras_drv_info.voc_info.decoder_temp.law_data, 0xD5, aud_tras_drv_info.voc_info.speaker_samp_rate_points);
					}
				} else {
					if (aud_tras_drv_info.voc_info.data_type == AUD_INTF_VOC_DATA_TYPE_G711U)
						os_memset(aud_tras_drv_info.voc_info.decoder_temp.law_data, 0xFF, aud_tras_drv_info.voc_info.speaker_samp_rate_points);
					else
						os_memset(aud_tras_drv_info.voc_info.decoder_temp.law_data, 0xD5, aud_tras_drv_info.voc_info.speaker_samp_rate_points);
				}

				/* dump rx data */
				if (aud_tras_drv_info.voc_info.aud_tras_dump_rx_cb) {
					aud_tras_drv_info.voc_info.aud_tras_dump_rx_cb(aud_tras_drv_info.voc_info.decoder_temp.law_data, aud_tras_drv_info.voc_info.speaker_samp_rate_points);
				}

				if (aud_tras_drv_info.voc_info.data_type == AUD_INTF_VOC_DATA_TYPE_G711U) {
					/* G711U decoding u-law data to pcm data*/
					for (i=0; i<aud_tras_drv_info.voc_info.speaker_samp_rate_points; i++) {
						aud_tras_drv_info.voc_info.decoder_temp.pcm_data[i] = ulaw2linear(aud_tras_drv_info.voc_info.decoder_temp.law_data[i]);
					}
				} else {
					/* G711A decoding a-law data to pcm data*/
					for (i=0; i<aud_tras_drv_info.voc_info.speaker_samp_rate_points; i++) {
						aud_tras_drv_info.voc_info.decoder_temp.pcm_data[i] = alaw2linear(aud_tras_drv_info.voc_info.decoder_temp.law_data[i]);
					}
				}
			} else {
				if (ring_buffer_get_free_size(&aud_tras_drv_info.voc_info.speaker_rb) > aud_tras_drv_info.voc_info.speaker_samp_rate_points * 2) {
					/* check the frame number in decoder_ring_buffer */
					if (ring_buffer_get_fill_size(aud_tras_drv_info.voc_info.rx_info.decoder_rb) >= aud_tras_drv_info.voc_info.speaker_samp_rate_points) {
						//os_printf("decoder process \r\n", size);
						/* get G711A data from decoder_ring_buff */
						//addAON_GPIO_Reg0x9 = 2;
						size = ring_buffer_read(aud_tras_drv_info.voc_info.rx_info.decoder_rb, (uint8_t*)aud_tras_drv_info.voc_info.decoder_temp.law_data, aud_tras_drv_info.voc_info.speaker_samp_rate_points);
						if (size != aud_tras_drv_info.voc_info.speaker_samp_rate_points) {
							LOGE("%s, %d, read decoder_ring_buff G711A data fail \n", __func__, __LINE__);
							if (aud_tras_drv_info.voc_info.data_type == AUD_INTF_VOC_DATA_TYPE_G711U)
								os_memset(aud_tras_drv_info.voc_info.decoder_temp.law_data, 0xFF, aud_tras_drv_info.voc_info.speaker_samp_rate_points);
							else
								os_memset(aud_tras_drv_info.voc_info.decoder_temp.law_data, 0xD5, aud_tras_drv_info.voc_info.speaker_samp_rate_points);
						}
						//addAON_GPIO_Reg0x9 = 0;
					} else {
						if (aud_tras_drv_info.voc_info.data_type == AUD_INTF_VOC_DATA_TYPE_G711U)
							os_memset(aud_tras_drv_info.voc_info.decoder_temp.law_data, 0xFF, aud_tras_drv_info.voc_info.speaker_samp_rate_points);
						else
							os_memset(aud_tras_drv_info.voc_info.decoder_temp.law_data, 0xD5, aud_tras_drv_info.voc_info.speaker_samp_rate_points);
					}

					/* dump rx data */
					if (aud_tras_drv_info.voc_info.aud_tras_dump_rx_cb) {
						aud_tras_drv_info.voc_info.aud_tras_dump_rx_cb(aud_tras_drv_info.voc_info.decoder_temp.law_data, aud_tras_drv_info.voc_info.speaker_samp_rate_points);
					}

					if (aud_tras_drv_info.voc_info.data_type == AUD_INTF_VOC_DATA_TYPE_G711U) {
						/* G711U decoding u-law data to pcm data*/
						for (i=0; i<aud_tras_drv_info.voc_info.speaker_samp_rate_points; i++) {
							aud_tras_drv_info.voc_info.decoder_temp.pcm_data[i] = ulaw2linear(aud_tras_drv_info.voc_info.decoder_temp.law_data[i]);
						}
					} else {
						/* G711A decoding a-law data to pcm data*/
						for (i=0; i<aud_tras_drv_info.voc_info.speaker_samp_rate_points; i++) {
							aud_tras_drv_info.voc_info.decoder_temp.pcm_data[i] = alaw2linear(aud_tras_drv_info.voc_info.decoder_temp.law_data[i]);
						}
					}
				}
			}
			break;

		case AUD_INTF_VOC_DATA_TYPE_PCM:
			if (aud_tras_drv_info.voc_info.spk_type == AUD_INTF_SPK_TYPE_BOARD) {
				if (ring_buffer_get_fill_size(aud_tras_drv_info.voc_info.rx_info.decoder_rb) >= aud_tras_drv_info.voc_info.speaker_samp_rate_points * 2) {
					//os_printf("decoder process \r\n", size);
					/* get pcm data from decoder_ring_buff */
					size = ring_buffer_read(aud_tras_drv_info.voc_info.rx_info.decoder_rb, (uint8_t*)aud_tras_drv_info.voc_info.decoder_temp.pcm_data, aud_tras_drv_info.voc_info.speaker_samp_rate_points * 2);
					if (size != aud_tras_drv_info.voc_info.speaker_samp_rate_points * 2) {
						LOGE("%s, %d, read decoder_ring_buff pcm data fail \n", __func__, __LINE__);
						os_memset(aud_tras_drv_info.voc_info.decoder_temp.pcm_data, 0x00, aud_tras_drv_info.voc_info.speaker_samp_rate_points * 2);
					}
				} else {
					os_memset(aud_tras_drv_info.voc_info.decoder_temp.pcm_data, 0x00, aud_tras_drv_info.voc_info.speaker_samp_rate_points * 2);
				}
				/* dump rx data */
				if (aud_tras_drv_info.voc_info.aud_tras_dump_rx_cb) {
					aud_tras_drv_info.voc_info.aud_tras_dump_rx_cb((uint8_t *)aud_tras_drv_info.voc_info.decoder_temp.pcm_data, aud_tras_drv_info.voc_info.speaker_samp_rate_points * 2);
				}
			} else {
				if (ring_buffer_get_free_size(&aud_tras_drv_info.voc_info.speaker_rb) > aud_tras_drv_info.voc_info.speaker_samp_rate_points * 2) {
					/* check the frame number in decoder_ring_buffer */
					if (ring_buffer_get_fill_size(aud_tras_drv_info.voc_info.rx_info.decoder_rb) >= aud_tras_drv_info.voc_info.speaker_samp_rate_points * 2) {
						//os_printf("decoder process \r\n", size);
						/* get pcm data from decoder_ring_buff */
						size = ring_buffer_read(aud_tras_drv_info.voc_info.rx_info.decoder_rb, (uint8_t*)aud_tras_drv_info.voc_info.decoder_temp.pcm_data, aud_tras_drv_info.voc_info.speaker_samp_rate_points * 2);
						if (size != aud_tras_drv_info.voc_info.speaker_samp_rate_points * 2) {
							LOGE("%s, %d, read decoder_ring_buff pcm data fail \n", __func__, __LINE__);
							os_memset(aud_tras_drv_info.voc_info.decoder_temp.pcm_data, 0x00, aud_tras_drv_info.voc_info.speaker_samp_rate_points * 2);
						}
					} else {
						os_memset(aud_tras_drv_info.voc_info.decoder_temp.pcm_data, 0x00, aud_tras_drv_info.voc_info.speaker_samp_rate_points * 2);
					}

					/* dump rx data */
					if (aud_tras_drv_info.voc_info.aud_tras_dump_rx_cb) {
						aud_tras_drv_info.voc_info.aud_tras_dump_rx_cb((uint8_t *)aud_tras_drv_info.voc_info.decoder_temp.pcm_data, aud_tras_drv_info.voc_info.speaker_samp_rate_points * 2);
					}
				}
			}
			break;

		default:
			break;
	}

#if CONFIG_AUD_TRAS_AEC_MIC_DELAY_DEBUG
	mic_delay_num++;
	os_memset(aud_tras_drv_info.voc_info.decoder_temp.pcm_data, 0, aud_tras_drv_info.voc_info.speaker_samp_rate_points*2);
	if (mic_delay_num == 50) {
		aud_tras_drv_info.voc_info.decoder_temp.pcm_data[0] = 0x2FFF;
		mic_delay_num = 0;
		LOGI("%s, %d, mic_delay_num \n", __func__, __LINE__);
	}
#endif

	if (aud_tras_drv_info.voc_info.aec_enable) {
		/* read mic fill data size */
		uint32_t mic_fill_size = ring_buffer_get_fill_size(&(aud_tras_drv_info.voc_info.mic_rb));
		//os_printf("mic_rb: fill_size=%d \r\n", mic_fill_size);
		uint32_t speaker_fill_size = ring_buffer_get_fill_size(&(aud_tras_drv_info.voc_info.speaker_rb));
		//os_printf("speaker_rb: fill_size=%d \r\n", speaker_fill_size);
		uint32_t ref_fill_size = ring_buffer_get_fill_size(&(aud_tras_drv_info.voc_info.aec_info->ref_rb));
		//os_printf("ref_rb: fill_size=%d \r\n", ref_fill_size);
		/* 设置参考信号延迟(采样点数，需要dump数据观察) */
#if CONFIG_AUD_TRAS_AEC_MIC_DELAY_DEBUG
		os_printf("%s, %d, MIC_DELAY: %d \n", __func__, __LINE__, (mic_fill_size + speaker_fill_size - ref_fill_size)/2);
#endif
		if ((mic_fill_size + speaker_fill_size - ref_fill_size)/2 < 0) {
			LOGE("%s, %d, MIC_DELAY is error, ref_fill_size: %d \n", __func__, __LINE__, ref_fill_size);
			aec_ctrl(aud_tras_drv_info.voc_info.aec_info->aec, AEC_CTRL_CMD_SET_MIC_DELAY, 0);
		} else {
			//aec_ctrl(aud_tras_drv_info.voc_info.aec_info->aec, AEC_CTRL_CMD_SET_MIC_DELAY, (mic_fill_size + speaker_fill_size - ref_fill_size)/2 + CONFIG_AUD_TRAS_AEC_MIC_DELAY_POINTS);
		}

		if (ring_buffer_get_free_size(&(aud_tras_drv_info.voc_info.aec_info->ref_rb)) > aud_tras_drv_info.voc_info.aec_info->samp_rate_points*2) {
			size = ring_buffer_write(&(aud_tras_drv_info.voc_info.aec_info->ref_rb), (uint8_t *)aud_tras_drv_info.voc_info.decoder_temp.pcm_data, aud_tras_drv_info.voc_info.aec_info->samp_rate_points*2);
			if (size != aud_tras_drv_info.voc_info.aec_info->samp_rate_points*2) {
				LOGE("%s, %d, write data to ref_ring_buff fail, size=%d \n", __func__, __LINE__, size);
				goto decoder_exit;
			}
		}


	}

#if CONFIG_AUD_TRAS_DAC_DEBUG
	if (aud_voc_dac_debug_flag) {
		//dump the data write to speaker
		FRESULT fr;
		uint32 uiTemp = 0;
		uint32_t i = 0, j = 0;
		/* write data to file */
		fr = f_write(&dac_debug_file, (uint32_t *)aud_tras_drv_info.voc_info.decoder_temp.pcm_data, aud_tras_drv_info.voc_info.speaker_samp_rate_points*2, &uiTemp);
		if (fr != FR_OK) {
			LOGE("%s, %d, write %s fail \n", __func__, __LINE__, dac_debug_file_name);
			return BK_FAIL;
		}

		//write 8K sin data
		for (i = 0; i < aud_tras_drv_info.voc_info.speaker_samp_rate_points*2; i++) {
			for (j = 0; j < 8; j++) {
				*(uint32_t *)0x47800048 = PCM_8000[j];
			}
			i += 8;
		}
	} else
#endif
	{
	/* save the data after G711A processed to encoder_ring_buffer */
		if (ring_buffer_get_free_size(&(aud_tras_drv_info.voc_info.speaker_rb)) > aud_tras_drv_info.voc_info.speaker_samp_rate_points*2) {
			size = ring_buffer_write(&(aud_tras_drv_info.voc_info.speaker_rb), (uint8_t *)aud_tras_drv_info.voc_info.decoder_temp.pcm_data, aud_tras_drv_info.voc_info.speaker_samp_rate_points*2);
			if (size != aud_tras_drv_info.voc_info.speaker_samp_rate_points*2) {
				LOGE("%s, %d, the data writeten to speaker_ring_buff is not a frame, size=%d \n", __func__, __LINE__, size);
				goto decoder_exit;
			}
			aud_tras_drv_info.voc_info.rx_info.aud_trs_read_seq++;
		}

		if (aud_tras_drv_info.voc_info.spk_type == AUD_INTF_SPK_TYPE_UAC) {
			size = ring_buffer_read(&aud_tras_drv_info.voc_info.speaker_rb, (uint8_t *)aud_tras_drv_info.voc_info.uac_spk_buff, aud_tras_drv_info.voc_info.speaker_samp_rate_points*2);
			if (size != aud_tras_drv_info.voc_info.speaker_samp_rate_points*2) {
				LOGE("%s, %d, read one frame pcm speaker data fail \n", __func__, __LINE__);
				os_memset(aud_tras_drv_info.voc_info.uac_spk_buff, 0x00, aud_tras_drv_info.voc_info.speaker_samp_rate_points*2);
			}
		}
	}
	/* call callback to notify app */
	if (aud_tras_drv_info.aud_tras_rx_spk_data)
		aud_tras_drv_info.aud_tras_rx_spk_data((unsigned int)aud_tras_drv_info.voc_info.speaker_samp_rate_points*2);

	return BK_OK;

decoder_exit:

	return BK_FAIL;
}


static bk_err_t aud_tras_drv_mic_tx_data(void)
{
	bk_err_t ret = BK_OK;
	uint32_t size = 0;

	if (aud_tras_drv_info.mic_info.mic_type == AUD_INTF_MIC_TYPE_BOARD) {
#if (CONFIG_CACHE_ENABLE)
		flush_all_dcache();
#endif
	}

	/* get a fram mic data from mic_ring_buff */
	if (ring_buffer_get_fill_size(&(aud_tras_drv_info.mic_info.mic_rb)) >= aud_tras_drv_info.mic_info.frame_size) {
		size = ring_buffer_read(&(aud_tras_drv_info.mic_info.mic_rb), (uint8_t*)aud_tras_drv_info.mic_info.temp_mic_addr, aud_tras_drv_info.mic_info.frame_size);
		if (size != aud_tras_drv_info.mic_info.frame_size) {
			LOGE("%s, %d, read mic_ring_buff fail, size:%d \n", __func__, __LINE__, size);
		}
	}

	/* call callback to notify app */
	if (aud_tras_drv_info.aud_tras_tx_mic_data)
	{
		mic_nofity.ptr_data = (uint32_t)aud_tras_drv_info.mic_info.temp_mic_addr;
		mic_nofity.length = (uint32_t)aud_tras_drv_info.mic_info.frame_size;
		mic_to_media_app_msg.event = EVENT_AUD_MIC_DATA_NOTIFY;
		mic_to_media_app_msg.param = (uint32_t)&mic_nofity;
//		LOGE("%s, size:%d \n", __func__, aud_tras_drv_info.mic_info.frame_size);
		msg_send_notify_to_media_major_mailbox(&mic_to_media_app_msg, APP_MODULE);
	}

	return ret;
}

static bk_err_t aud_tras_drv_spk_req_data(audio_packet_t * packet)
{
	bk_err_t ret = BK_OK;
	uint32_t size = 0;

#if (CONFIG_CACHE_ENABLE)
	flush_all_dcache();
#endif

	if (aud_tras_drv_info.spk_info.spk_type == AUD_INTF_SPK_TYPE_BOARD) {
		/* get speaker data from spk_rx_ring_buff */
		if (ring_buffer_get_fill_size(aud_tras_drv_info.spk_info.spk_rx_rb) >= aud_tras_drv_info.spk_info.frame_size) {
			size = ring_buffer_read(aud_tras_drv_info.spk_info.spk_rx_rb, (uint8_t*)aud_tras_drv_info.spk_info.temp_spk_addr, aud_tras_drv_info.spk_info.frame_size);
			if (size != aud_tras_drv_info.spk_info.frame_size) {
				LOGE("%s, %d, read spk_rx_ring_buff fail, size:%d \n", __func__, __LINE__, size);
				os_memset(aud_tras_drv_info.spk_info.temp_spk_addr, 0, aud_tras_drv_info.spk_info.frame_size);
			}
		} else {
			LOGD("%s, %d, spk_rx_rb is empty \n", __func__, __LINE__);
			os_memset(aud_tras_drv_info.spk_info.temp_spk_addr, 0, aud_tras_drv_info.spk_info.frame_size);
		}

		/* write spk_rx_data to audio dac */
		size = ring_buffer_write(&(aud_tras_drv_info.spk_info.spk_rb), (uint8_t*)aud_tras_drv_info.spk_info.temp_spk_addr, aud_tras_drv_info.spk_info.frame_size);
		if (size != aud_tras_drv_info.spk_info.frame_size) {
			LOGE("%s, %d, write spk_data to audio dac fail, size:%d \n", __func__, __LINE__, size);
			//TODO
			//add handle code
		}

		/* call callback to notify app */
		if (aud_tras_drv_info.aud_tras_rx_spk_data)
		{
			spk_to_media_app_msg.event = EVENT_AUD_SPK_DATA_NOTIFY;
			spk_to_media_app_msg.param = aud_tras_drv_info.spk_info.frame_size;
			msg_send_notify_to_media_major_mailbox(&spk_to_media_app_msg, APP_MODULE);
			//ret = aud_tras_drv_info.aud_tras_rx_spk_data((unsigned int)aud_tras_drv_info.spk_info.frame_size);
		}
	} else {
		if (ring_buffer_get_fill_size(&aud_tras_drv_info.spk_info.spk_rb) >= aud_tras_drv_info.spk_info.uac_spk_buff_size) {
			/* get pcm data from spk_rb */
			size = ring_buffer_read(&aud_tras_drv_info.spk_info.spk_rb, aud_tras_drv_info.spk_info.uac_spk_buff, aud_tras_drv_info.spk_info.uac_spk_buff_size);
			if (size != aud_tras_drv_info.spk_info.uac_spk_buff_size) {
				LOGE("%s, %d, read spk_rb uac spk data fail \n", __func__, __LINE__);
				os_memset(aud_tras_drv_info.spk_info.uac_spk_buff, 0x00, aud_tras_drv_info.spk_info.uac_spk_buff_size);
			}
		} else {
			os_memset(aud_tras_drv_info.spk_info.uac_spk_buff, 0x00, aud_tras_drv_info.spk_info.uac_spk_buff_size);
		}

		/* save the data after G711A processed to encoder_ring_buffer */
		if (ring_buffer_get_free_size(&(aud_tras_drv_info.spk_info.spk_rb)) > aud_tras_drv_info.spk_info.frame_size) {
			/* get speaker data from spk_rx_ring_buff */
			if (ring_buffer_get_fill_size(aud_tras_drv_info.spk_info.spk_rx_rb) >= aud_tras_drv_info.spk_info.frame_size) {
				size = ring_buffer_read(aud_tras_drv_info.spk_info.spk_rx_rb, (uint8_t*)aud_tras_drv_info.spk_info.temp_spk_addr, aud_tras_drv_info.spk_info.frame_size);
				if (size != aud_tras_drv_info.spk_info.frame_size) {
					LOGE("%s, %d, read spk_rx_ring_buff fail, size:%d \n", __func__, __LINE__, size);
					os_memset(aud_tras_drv_info.spk_info.temp_spk_addr, 0, aud_tras_drv_info.spk_info.frame_size);
				}
			} else {
				LOGD("%s, %d, spk_rx_rb is empty \n", __func__, __LINE__);
				os_memset(aud_tras_drv_info.spk_info.temp_spk_addr, 0, aud_tras_drv_info.spk_info.frame_size);
			}

			/* write spk_rx_data to audio dac */
			size = ring_buffer_write(&(aud_tras_drv_info.spk_info.spk_rb), (uint8_t*)aud_tras_drv_info.spk_info.temp_spk_addr, aud_tras_drv_info.spk_info.frame_size);
			if (size != aud_tras_drv_info.spk_info.frame_size) {
				LOGE("%s, %d, write spk_data to audio dac fail, size:%d \n", __func__, __LINE__, size);
				//TODO
				//add handle code
			}

			/* call callback to notify app */
			if (aud_tras_drv_info.aud_tras_rx_spk_data)
			{
				spk_to_media_app_msg.event = EVENT_AUD_SPK_DATA_NOTIFY;
				spk_to_media_app_msg.param = aud_tras_drv_info.spk_info.frame_size;
				msg_send_notify_to_media_major_mailbox(&spk_to_media_app_msg, APP_MODULE);
				//ret = aud_tras_drv_info.aud_tras_rx_spk_data((unsigned int)aud_tras_drv_info.spk_info.frame_size);
			}
		}
	}

	return ret;
}


static bk_err_t aud_tras_drv_spk_init(aud_intf_spk_config_t *spk_cfg)
{
	bk_err_t ret = BK_OK;
	bk_err_t err = BK_ERR_AUD_INTF_FAIL;

	LOGI("%s \n", __func__);

	aud_tras_drv_info.spk_info.spk_type = spk_cfg->spk_type;
	if (aud_tras_drv_info.spk_info.spk_type == AUD_INTF_SPK_TYPE_BOARD) {
		/* get audio dac config */
		aud_tras_drv_info.spk_info.dac_config = audio_tras_drv_malloc(sizeof(aud_dac_config_t));
		if (aud_tras_drv_info.spk_info.dac_config == NULL) {
			LOGE("%s, %d, adc_config os_malloc fail \n", __func__, __LINE__);
			err = BK_ERR_AUD_INTF_MEMY;
			goto aud_tras_drv_spk_init_exit;
		} else {
			aud_tras_drv_info.spk_info.dac_config->samp_rate = spk_cfg->samp_rate;
			if (spk_cfg->spk_chl == AUD_INTF_SPK_CHL_LEFT)
				aud_tras_drv_info.spk_info.dac_config->dac_chl = AUD_DAC_CHL_L;
			else
				aud_tras_drv_info.spk_info.dac_config->dac_chl = AUD_DAC_CHL_LR;
			aud_tras_drv_info.spk_info.dac_config->work_mode = spk_cfg->work_mode;
			aud_tras_drv_info.spk_info.dac_config->dac_gain = spk_cfg->spk_gain;	//default 2D  3F  15
			aud_tras_drv_info.spk_info.dac_config->dac_clk_invert = AUD_DAC_CLK_INVERT_RISING;
			aud_tras_drv_info.spk_info.dac_config->clk_src = AUD_CLK_XTAL;
		}
		aud_tras_drv_info.spk_info.spk_en = true;
	} else {
		/* set audio uac config */
		if (aud_tras_drv_info.spk_info.uac_spk_config == NULL) {
			aud_tras_drv_info.spk_info.uac_spk_config = audio_tras_drv_malloc(sizeof(aud_uac_spk_config_t));
			if (aud_tras_drv_info.spk_info.uac_spk_config == NULL) {
				LOGE("%s, %d, uac_config os_malloc fail \n", __func__, __LINE__);
				err = BK_ERR_AUD_INTF_MEMY;
				goto aud_tras_drv_spk_init_exit;
			} else {
				aud_tras_drv_info.spk_info.uac_spk_config->spk_format_tag = AUD_UAC_DATA_FORMAT_TYPE_PCM;
				aud_tras_drv_info.spk_info.uac_spk_config->spk_samp_rate = 8000;
				aud_tras_drv_info.spk_info.uac_spk_config->spk_volume = 0;
			}
			aud_tras_drv_info.spk_info.uac_spk_config->spk_samp_rate = spk_cfg->samp_rate;
			aud_tras_drv_info.spk_info.uac_spk_config->spk_volume = spk_cfg->spk_gain;
		}
	}

	aud_tras_drv_info.spk_info.spk_chl = spk_cfg->spk_chl;
	aud_tras_drv_info.spk_info.frame_size = spk_cfg->frame_size;
	aud_tras_drv_info.spk_info.fifo_frame_num = spk_cfg->fifo_frame_num;
	aud_tras_drv_info.spk_info.spk_rx_rb = spk_cfg->spk_rx_rb;
	aud_tras_drv_info.spk_info.spk_rx_ring_buff = spk_cfg->spk_rx_ring_buff;

	/* malloc spk_ring_buff to save audio data */
	aud_tras_drv_info.spk_info.spk_ring_buff = (int32_t *)audio_tras_drv_malloc(aud_tras_drv_info.spk_info.frame_size * 2 + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL);
	if (aud_tras_drv_info.spk_info.spk_ring_buff == NULL) {
		LOGE("%s, %d, malloc speaker ring buffer fail \n", __func__, __LINE__);
		err = BK_ERR_AUD_INTF_MEMY;
		goto aud_tras_drv_spk_init_exit;
	}

	if (aud_tras_drv_info.spk_info.spk_type == AUD_INTF_SPK_TYPE_BOARD) {
		/* audio dac config */
		ret = aud_tras_dac_config(aud_tras_drv_info.spk_info.dac_config);
		if (ret != BK_OK) {
			LOGE("%s, %d, audio dac init fail \n", __func__, __LINE__);
			err = BK_ERR_AUD_INTF_DAC;
			goto aud_tras_drv_spk_init_exit;
		}
		LOGI("step1: init audio and config DAC complete \n");

		/* init dma driver */
		ret = bk_dma_driver_init();
		if (ret != BK_OK) {
			LOGE("%s, %d, dma driver init fail \n", __func__, __LINE__);
			err = BK_ERR_AUD_INTF_DMA;
			goto aud_tras_drv_spk_init_exit;
		}

		/* allocate free DMA channel */
		aud_tras_drv_info.spk_info.spk_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
		if ((aud_tras_drv_info.spk_info.spk_dma_id < DMA_ID_0) || (aud_tras_drv_info.spk_info.spk_dma_id >= DMA_ID_MAX)) {
			LOGE("%s, %d, malloc dac dma fail \n", __func__, __LINE__);
			err = BK_ERR_AUD_INTF_DMA;
			goto aud_tras_drv_spk_init_exit;
		}

		/* config audio dac dma to carry speaker data to "spk_ring_buff" */
		ret = aud_tras_dac_dma_config(aud_tras_drv_info.spk_info.spk_dma_id, aud_tras_drv_info.spk_info.spk_ring_buff, aud_tras_drv_info.spk_info.frame_size * 2 + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL, aud_tras_drv_info.spk_info.frame_size, aud_tras_drv_info.spk_info.spk_chl);
		if (ret != BK_ERR_AUD_INTF_OK) {
			LOGE("%s, %d, config audio dac dma fail \n", __func__, __LINE__);
			ret = ret;
			goto aud_tras_drv_spk_init_exit;
		}

		/* init spk_ring_buff */
		ring_buffer_init(&(aud_tras_drv_info.spk_info.spk_rb), (uint8_t*)aud_tras_drv_info.spk_info.spk_ring_buff, aud_tras_drv_info.spk_info.frame_size * 2 + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL, aud_tras_drv_info.spk_info.spk_dma_id, RB_DMA_TYPE_READ);
		LOGI("step2: init dma:%d, and spk ring buff:%p, size:%d complete \n", aud_tras_drv_info.spk_info.spk_dma_id, aud_tras_drv_info.spk_info.spk_ring_buff, aud_tras_drv_info.spk_info.frame_size * 2 + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL);
	} else if (aud_tras_drv_info.spk_info.spk_type == AUD_INTF_SPK_TYPE_UAC) {
		/* init spk_ring_buff */
		ring_buffer_init(&(aud_tras_drv_info.spk_info.spk_rb), (uint8_t*)aud_tras_drv_info.spk_info.spk_ring_buff, aud_tras_drv_info.spk_info.frame_size*2 + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL, DMA_ID_MAX, RB_DMA_TYPE_NULL);

		/* close spk */
		aud_tras_drv_info.spk_info.spk_en = false;
		aud_tras_drv_info.spk_info.uac_spk_buff_size = aud_tras_drv_info.spk_info.frame_size;
		aud_tras_drv_info.spk_info.uac_spk_buff = (uint8_t *)audio_tras_drv_malloc(aud_tras_drv_info.spk_info.uac_spk_buff_size);

		/* register uac connect and disconnect callback */
		bk_aud_uac_register_disconnect_cb(aud_tras_uac_disconnect_cb);
		bk_aud_uac_register_connect_cb(aud_tras_uac_connect_cb);

		LOGI("%s, %d, init uac driver \n", __func__, __LINE__);
		ret = bk_aud_uac_driver_init();
		if (ret != BK_OK) {
			LOGE("%s, %d, init uac driver fail \n", __func__, __LINE__);
			err = BK_ERR_AUD_INTF_UAC_DRV;
			goto aud_tras_drv_spk_init_exit;
		}

		/* set uac speaker volume */
		//aud_tras_drv_set_spk_gain(aud_tras_drv_info.spk_info.uac_spk_config->spk_volume);

//		aud_tras_drv_info.uac_status = AUD_INTF_UAC_CONNECTED;

		LOGI("step2: init spk ring buff:%p, size:%d complete \n", aud_tras_drv_info.spk_info.spk_ring_buff, aud_tras_drv_info.spk_info.frame_size*2 + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL);
	} else {
		err = BK_ERR_AUD_INTF_PARAM;
		goto aud_tras_drv_spk_init_exit;
	}

	/* init spk temp buffer */
	aud_tras_drv_info.spk_info.temp_spk_addr = (int32_t *)audio_tras_drv_malloc(aud_tras_drv_info.spk_info.frame_size);
	if (aud_tras_drv_info.spk_info.temp_spk_addr == NULL) {
		LOGE("%s, %d, malloc temp spk ring buffer fail \n", __func__, __LINE__);
		err = BK_ERR_AUD_INTF_MEMY;
		goto aud_tras_drv_spk_init_exit;
	}
	LOGI("step3: init temp spk ring buff complete \n");

#ifdef CONFIG_UAC_MIC_SPK_COUNT_DEBUG
	if (aud_tras_drv_info.spk_info.spk_type == AUD_INTF_SPK_TYPE_UAC) {
		uac_mic_spk_count_open();
	}
#endif

	/* change status: AUD_TRAS_DRV_SPK_NULL --> AUD_TRAS_DRV_SPK_IDLE */
	aud_tras_drv_info.spk_info.status = AUD_TRAS_DRV_SPK_STA_IDLE;

	LOGI("step4: init spk complete \n");

	return BK_ERR_AUD_INTF_OK;

aud_tras_drv_spk_init_exit:
	aud_tras_drv_info.spk_info.spk_en = false;
	aud_tras_drv_info.spk_info.status = AUD_TRAS_DRV_MIC_STA_NULL;
	aud_tras_drv_info.spk_info.spk_chl = AUD_INTF_MIC_CHL_MIC1;

	if (aud_tras_drv_info.spk_info.dac_config)
		audio_tras_drv_free(aud_tras_drv_info.spk_info.dac_config);
	aud_tras_drv_info.spk_info.dac_config = NULL;

	aud_tras_drv_info.spk_info.frame_size = 0;
	if (aud_tras_drv_info.spk_info.spk_dma_id != DMA_ID_MAX) {
		bk_dma_deinit(aud_tras_drv_info.spk_info.spk_dma_id);
		bk_dma_free(DMA_DEV_AUDIO, aud_tras_drv_info.spk_info.spk_dma_id);
	}
	aud_tras_drv_info.spk_info.spk_dma_id = DMA_ID_MAX;

	if (aud_tras_drv_info.spk_info.spk_ring_buff != NULL) {
		ring_buffer_clear(&(aud_tras_drv_info.spk_info.spk_rb));
		audio_tras_drv_free(aud_tras_drv_info.spk_info.spk_ring_buff);
	}
	aud_tras_drv_info.spk_info.spk_ring_buff = NULL;

	if (aud_tras_drv_info.spk_info.temp_spk_addr != NULL)
		audio_tras_drv_free(aud_tras_drv_info.spk_info.temp_spk_addr);
	aud_tras_drv_info.spk_info.temp_spk_addr = NULL;

	if (aud_tras_drv_info.spk_info.uac_spk_config)
		audio_tras_drv_free(aud_tras_drv_info.spk_info.uac_spk_config);
	aud_tras_drv_info.spk_info.uac_spk_config = NULL;

	LOGE("%s, %d, init spk fail \n", __func__, __LINE__);

	return err;
}

static bk_err_t aud_tras_drv_spk_deinit(void)
{
	if (aud_tras_drv_info.spk_info.spk_type == AUD_INTF_SPK_TYPE_BOARD) {
		/* disable audio dac */
		bk_aud_dac_stop();
		aud_tras_dac_pa_ctrl(false);
		bk_dma_stop(aud_tras_drv_info.spk_info.spk_dma_id);
		bk_dma_deinit(aud_tras_drv_info.spk_info.spk_dma_id);
		bk_dma_free(DMA_DEV_AUDIO, aud_tras_drv_info.spk_info.spk_dma_id);
		bk_aud_dac_deinit();
		if (aud_tras_drv_info.mic_info.status == AUD_TRAS_DRV_MIC_STA_NULL)
			bk_aud_driver_deinit();
		audio_tras_drv_free(aud_tras_drv_info.spk_info.dac_config);
		aud_tras_drv_info.spk_info.dac_config = NULL;
	} else {
		aud_tras_drv_info.uac_status = AUD_INTF_UAC_NORMAL_DISCONNECTED;
		bk_aud_uac_stop_spk();
		bk_aud_uac_driver_deinit();

		audio_tras_drv_free(aud_tras_drv_info.spk_info.uac_spk_buff);
		aud_tras_drv_info.spk_info.uac_spk_buff = NULL;
		aud_tras_drv_info.spk_info.uac_spk_buff_size = 0;
	}

	aud_tras_drv_info.spk_info.spk_en = false;
	aud_tras_drv_info.spk_info.spk_chl = AUD_INTF_MIC_CHL_MIC1;
	aud_tras_drv_info.spk_info.frame_size = 0;
	aud_tras_drv_info.spk_info.spk_dma_id = DMA_ID_MAX;

	if (aud_tras_drv_info.spk_info.uac_spk_config)
		audio_tras_drv_free(aud_tras_drv_info.spk_info.uac_spk_config);
	aud_tras_drv_info.spk_info.uac_spk_config = NULL;

	ring_buffer_clear(&(aud_tras_drv_info.spk_info.spk_rb));
	audio_tras_drv_free(aud_tras_drv_info.spk_info.spk_ring_buff);
	aud_tras_drv_info.spk_info.spk_ring_buff = NULL;

	audio_tras_drv_free(aud_tras_drv_info.spk_info.temp_spk_addr);
	aud_tras_drv_info.spk_info.temp_spk_addr = NULL;

#ifdef CONFIG_UAC_MIC_SPK_COUNT_DEBUG
	if (aud_tras_drv_info.spk_info.spk_type == AUD_INTF_SPK_TYPE_UAC) {
		uac_mic_spk_count_close();
	}
#endif

	aud_tras_drv_info.spk_info.status = AUD_TRAS_DRV_SPK_STA_NULL;

	return BK_ERR_AUD_INTF_OK;
}

static bk_err_t aud_tras_drv_spk_start(void)
{
	bk_err_t ret = BK_OK;
	uint32_t size = 0;

	if ((aud_tras_drv_info.spk_info.status != AUD_TRAS_DRV_SPK_STA_IDLE) && (aud_tras_drv_info.spk_info.status != AUD_TRAS_DRV_SPK_STA_PAUSE)) {
		return BK_ERR_AUD_INTF_STA;
	}

	if (aud_tras_drv_info.spk_info.spk_type == AUD_INTF_SPK_TYPE_BOARD) {
		/* enable dac */
		bk_aud_dac_start();
		bk_aud_dac_start();

		aud_tras_dac_pa_ctrl(true);
		if (!bk_dma_get_enable_status(aud_tras_drv_info.spk_info.spk_dma_id)) {
			ret = bk_dma_start(aud_tras_drv_info.spk_info.spk_dma_id);
			if (ret != BK_OK) {
				LOGE("%s, %d, start speaker dma fail \n", __func__, __LINE__);
				return BK_ERR_AUD_INTF_DMA;
			}
		}
	} else {
		LOGI("%s, %d, start uac spk \n", __func__, __LINE__);
		if (aud_tras_drv_info.uac_status == AUD_INTF_UAC_CONNECTED) {
			ret = bk_aud_uac_start_spk();
			if (ret != BK_OK) {
				LOGE("%s, %d, start uac spk fail, ret: %d \n", __func__, __LINE__, ret);
				return BK_ERR_AUD_INTF_UAC_SPK;
			} else {
				aud_tras_drv_info.uac_spk_open_status = true;
				aud_tras_drv_info.uac_spk_open_current = true;
			}
		} else {
			aud_tras_drv_info.uac_spk_open_status = true;
		}
	}

	if (aud_tras_drv_info.spk_info.status == AUD_TRAS_DRV_SPK_STA_IDLE) {
		/* get speaker data from spk_rx_ring_buff */
		if (ring_buffer_get_fill_size(aud_tras_drv_info.spk_info.spk_rx_rb) >= aud_tras_drv_info.spk_info.frame_size) {
			size = ring_buffer_read(aud_tras_drv_info.spk_info.spk_rx_rb, (uint8_t*)aud_tras_drv_info.spk_info.temp_spk_addr, aud_tras_drv_info.spk_info.frame_size);
			if (size != aud_tras_drv_info.spk_info.frame_size) {
				LOGE("%s, %d, read spk_rx_ring_buff fail, size: %d \n", __func__, __LINE__, size);
			}
		} else {
			LOGD("%s, %d, spk_rx_rb is empty \n", __func__, __LINE__);
			os_memset(aud_tras_drv_info.spk_info.temp_spk_addr, 0, aud_tras_drv_info.spk_info.frame_size);
		}

		/* write spk_rx_data to audio dac */
		size = ring_buffer_write(&(aud_tras_drv_info.spk_info.spk_rb), (uint8_t*)aud_tras_drv_info.spk_info.temp_spk_addr, aud_tras_drv_info.spk_info.frame_size);
		if (size != aud_tras_drv_info.spk_info.frame_size) {
			LOGE("%s, %d, write spk_data to audio dac fail, size: %d \n", __func__, __LINE__, size);
			//return BK_FAIL;
		}

		if (ring_buffer_get_fill_size(aud_tras_drv_info.spk_info.spk_rx_rb) >= aud_tras_drv_info.spk_info.frame_size) {
			size = ring_buffer_read(aud_tras_drv_info.spk_info.spk_rx_rb, (uint8_t*)aud_tras_drv_info.spk_info.temp_spk_addr, aud_tras_drv_info.spk_info.frame_size);
			if (size != aud_tras_drv_info.spk_info.frame_size) {
				LOGE("%s, %d, read spk_rx_ring_buff fail, size:%d \n", __func__, __LINE__, size);
			}
		} else {
			LOGD("%s, %d, spk_rx_rb is empty \n", __func__, __LINE__);
			os_memset(aud_tras_drv_info.spk_info.temp_spk_addr, 0, aud_tras_drv_info.spk_info.frame_size);
		}

		/* write spk_rx_data to audio dac */
		size = ring_buffer_write(&(aud_tras_drv_info.spk_info.spk_rb), (uint8_t*)aud_tras_drv_info.spk_info.temp_spk_addr, aud_tras_drv_info.spk_info.frame_size);
		if (size != aud_tras_drv_info.spk_info.frame_size) {
			LOGE("%s, %d, write spk_data to audio dac fail, size: %d \n", __func__, __LINE__, size);
			//return BK_FAIL;
		}
	}

	aud_tras_drv_info.spk_info.status = AUD_TRAS_DRV_SPK_STA_START;

	return BK_ERR_AUD_INTF_OK;
}

/* not stop dma, only disable adc */
static bk_err_t aud_tras_drv_spk_pause(void)
{
	if (aud_tras_drv_info.spk_info.status != AUD_TRAS_DRV_SPK_STA_START) {
		return BK_ERR_AUD_INTF_STA;
	}

	if (aud_tras_drv_info.spk_info.spk_type == AUD_INTF_SPK_TYPE_BOARD) {
		/* disable adc */
		bk_aud_dac_stop();
		aud_tras_dac_pa_ctrl(false);
//		bk_dma_stop(aud_tras_drv_info.spk_info.spk_dma_id);
	} else {
		bk_aud_uac_stop_spk();
	}

	aud_tras_drv_info.spk_info.status = AUD_TRAS_DRV_SPK_STA_PAUSE;

	return BK_ERR_AUD_INTF_OK;
}

static bk_err_t aud_tras_drv_spk_stop(void)
{
	bk_err_t ret = BK_OK;

	if ((aud_tras_drv_info.spk_info.status != AUD_TRAS_DRV_SPK_STA_START) && (aud_tras_drv_info.spk_info.status != AUD_TRAS_DRV_SPK_STA_PAUSE)) {
		return BK_ERR_AUD_INTF_STA;
	}

	if (aud_tras_drv_info.spk_info.spk_type == AUD_INTF_SPK_TYPE_BOARD) {
		/* disable adc */
		bk_aud_dac_stop();
		aud_tras_dac_pa_ctrl(false);
		bk_dma_stop(aud_tras_drv_info.spk_info.spk_dma_id);
	} else {
		ret = bk_aud_uac_stop_spk();
		if (ret != BK_OK) {
			LOGE("%s, %d, stop uac spk fail \n", __func__, __LINE__);
			return BK_ERR_AUD_INTF_UAC_SPK;
		} else {
			aud_tras_drv_info.uac_spk_open_status = false;
			aud_tras_drv_info.uac_spk_open_current = false;
		}
	}

	ring_buffer_clear(&(aud_tras_drv_info.spk_info.spk_rb));
	aud_tras_drv_info.spk_info.status = AUD_TRAS_DRV_SPK_STA_IDLE;

	return BK_ERR_AUD_INTF_OK;
}

static bk_err_t aud_tras_drv_spk_set_chl(aud_intf_spk_chl_t spk_chl)
{
	bk_err_t ret = BK_ERR_AUD_INTF_OK;

	switch (spk_chl) {
		case AUD_INTF_SPK_CHL_LEFT:
			ret = bk_aud_dac_set_chl(AUD_DAC_CHL_L);
			if (ret == BK_OK)
				ret = bk_dma_set_dest_data_width(aud_tras_drv_info.mic_info.mic_dma_id, DMA_DATA_WIDTH_16BITS);
			break;

		case AUD_INTF_SPK_CHL_DUAL:
			ret = bk_aud_dac_set_chl(AUD_DAC_CHL_LR);
			if (ret == BK_OK)
				ret = bk_dma_set_dest_data_width(aud_tras_drv_info.mic_info.mic_dma_id, DMA_DATA_WIDTH_32BITS);
			break;

		default:
			break;
	}

	return ret;
}

static void aud_tras_drv_mic_uac_mic_cb(uint8_t *buff, uint32_t count)
{
	bk_err_t ret = BK_OK;
	uint32_t write_size = 0;
	uint32_t data = 0;

	if (aud_tras_drv_info.mic_info.status == AUD_TRAS_DRV_MIC_STA_START && count > 0) {
		if (ring_buffer_get_free_size(&aud_tras_drv_info.mic_info.mic_rb) >= count) {
			for (write_size = 0; write_size < count/4; write_size++) {
				data = *((uint32_t *)buff);
				ring_buffer_write(&aud_tras_drv_info.mic_info.mic_rb, (uint8_t *)&data, 4);
			}
		}
	}

	if (uac_mic_read_flag && (ring_buffer_get_fill_size(&aud_tras_drv_info.mic_info.mic_rb) < aud_tras_drv_info.mic_info.frame_size))
		uac_mic_read_flag = false;

	if ((ring_buffer_get_fill_size(&aud_tras_drv_info.mic_info.mic_rb) > aud_tras_drv_info.mic_info.frame_size) && (uac_mic_read_flag == false)) {
		uac_mic_read_flag = true;
		/* send msg to TX_DATA to process mic data */
		ret = aud_tras_drv_send_msg(AUD_TRAS_DRV_MIC_TX_DATA, NULL);
		if (ret != kNoErr) {
			LOGE("%s, %d, send msg: AUD_TRAS_DRV_MIC_TX_DATA fail \n", __func__, __LINE__);
			uac_mic_read_flag = false;
		}
	}
}


static bk_err_t aud_tras_drv_mic_init(aud_intf_mic_config_t *mic_cfg)
{
	bk_err_t ret = BK_OK;
	bk_err_t err = BK_ERR_AUD_INTF_FAIL;

	LOGD("%s, %d, mic_cfg->frame_size: %d \n", __func__, __LINE__, mic_cfg->frame_size);

//	aud_tras_drv_info.mic_info.aud_tras_drv_mic_event_cb = mic_cfg->aud_tras_drv_mic_event_cb;
	aud_tras_drv_info.mic_info.mic_type = mic_cfg->mic_type;
	if (aud_tras_drv_info.mic_info.mic_type == AUD_INTF_MIC_TYPE_BOARD) {
		/* get audio adc config */
		aud_tras_drv_info.mic_info.adc_config = audio_tras_drv_malloc(sizeof(aud_adc_config_t));
		if (aud_tras_drv_info.mic_info.adc_config == NULL) {
			LOGE("%s, %d, adc_config audio_tras_drv_malloc fail \n", __func__, __LINE__);
			err = BK_ERR_AUD_INTF_MEMY;
			goto aud_tras_drv_mic_init_exit;
		} else {
			if (mic_cfg->mic_chl == AUD_INTF_MIC_CHL_MIC1) {
				aud_tras_drv_info.mic_info.adc_config->adc_chl = AUD_ADC_CHL_L;
			} else if (mic_cfg->mic_chl == AUD_INTF_MIC_CHL_DUAL) {
				aud_tras_drv_info.mic_info.adc_config->adc_chl = AUD_ADC_CHL_LR;
			} else {
				LOGW("%s, %d, mic chl is error, set default: AUD_MIC_MIC1_ENABLE \n", __func__, __LINE__);
				aud_tras_drv_info.mic_info.adc_config->adc_chl = AUD_ADC_CHL_L;
			}
			aud_tras_drv_info.mic_info.adc_config->samp_rate = mic_cfg->samp_rate;
			aud_tras_drv_info.mic_info.adc_config->adc_gain = mic_cfg->mic_gain;	//default: 0x2d
			aud_tras_drv_info.mic_info.adc_config->adc_samp_edge = AUD_ADC_SAMP_EDGE_RISING;
			aud_tras_drv_info.mic_info.adc_config->adc_mode = AUD_ADC_MODE_DIFFEN;
			aud_tras_drv_info.mic_info.adc_config->clk_src = AUD_CLK_XTAL;
		}
		aud_tras_drv_info.mic_info.mic_en = true;
	} else {
		/* set audio uac config */
		if (aud_tras_drv_info.mic_info.uac_mic_config == NULL) {
			aud_tras_drv_info.mic_info.uac_mic_config = audio_tras_drv_malloc(sizeof(aud_uac_mic_config_t));
			if (aud_tras_drv_info.mic_info.uac_mic_config == NULL) {
				LOGE("%s, %d, uac_config audio_tras_drv_malloc fail \n", __func__, __LINE__);
				err = BK_ERR_AUD_INTF_MEMY;
				goto aud_tras_drv_mic_init_exit;
			} else {
				aud_tras_drv_info.mic_info.uac_mic_config->mic_format_tag = AUD_UAC_DATA_FORMAT_TYPE_PCM;
				aud_tras_drv_info.mic_info.uac_mic_config->mic_samp_rate = 8000;
			}
			aud_tras_drv_info.mic_info.uac_mic_config->mic_samp_rate = mic_cfg->samp_rate;
		}
	}

	aud_tras_drv_info.mic_info.mic_chl = mic_cfg->mic_chl;
	aud_tras_drv_info.mic_info.frame_size = mic_cfg->frame_size;

	/* config audio adc or uac and dma */
	LOGD("%s, %d, mic_ring_buff size: %d \n", __func__, __LINE__, aud_tras_drv_info.mic_info.frame_size * 2 + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL);
	aud_tras_drv_info.mic_info.mic_ring_buff = (int32_t *)audio_tras_drv_malloc(aud_tras_drv_info.mic_info.frame_size * 2 + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL);
	if (aud_tras_drv_info.mic_info.mic_ring_buff == NULL) {
		LOGE("%s, %d, malloc mic ring buffer fail \n", __func__, __LINE__);
		err = BK_ERR_AUD_INTF_MEMY;
		goto aud_tras_drv_mic_init_exit;
	}

	if (aud_tras_drv_info.mic_info.mic_type == AUD_INTF_MIC_TYPE_BOARD) {
		/* audio adc config */
		ret = aud_tras_adc_config(aud_tras_drv_info.mic_info.adc_config);
		if (ret != BK_OK) {
			LOGE("%s, %d, audio adc init fail \n", __func__, __LINE__);
			err = BK_ERR_AUD_INTF_ADC;
			goto aud_tras_drv_mic_init_exit;
		}
		LOGI("step1: init audio and config ADC complete \n");

		/* init dma driver */
		ret = bk_dma_driver_init();
		if (ret != BK_OK) {
			LOGE("%s, %d, dma driver init fail \n", __func__, __LINE__);
			err = BK_ERR_AUD_INTF_DMA;
			goto aud_tras_drv_mic_init_exit;
		}

		/* allocate free DMA channel */
		aud_tras_drv_info.mic_info.mic_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
		if ((aud_tras_drv_info.mic_info.mic_dma_id < DMA_ID_0) || (aud_tras_drv_info.mic_info.mic_dma_id >= DMA_ID_MAX)) {
			LOGE("%s, %d, malloc adc dma fail \n", __func__, __LINE__);
			err = BK_ERR_AUD_INTF_DMA;
			goto aud_tras_drv_mic_init_exit;
		}

		/* config audio adc dma to carry mic data to "mic_ring_buff" */
		ret = aud_tras_adc_dma_config(aud_tras_drv_info.mic_info.mic_dma_id, aud_tras_drv_info.mic_info.mic_ring_buff, aud_tras_drv_info.mic_info.frame_size * 2 + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL, aud_tras_drv_info.mic_info.frame_size, aud_tras_drv_info.mic_info.mic_chl);
		if (ret != BK_OK) {
			LOGE("%s, %d, config audio adc dma fail \n", __func__, __LINE__);
			err = ret;
			goto aud_tras_drv_mic_init_exit;
		}

		/* init mic_ring_buff */
		ring_buffer_init(&(aud_tras_drv_info.mic_info.mic_rb), (uint8_t*)aud_tras_drv_info.mic_info.mic_ring_buff, aud_tras_drv_info.mic_info.frame_size * 2 + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL, aud_tras_drv_info.mic_info.mic_dma_id, RB_DMA_TYPE_WRITE);
		LOGI("step2: init dma:%d, and mic ring buff:%p, size:%d complete \n", aud_tras_drv_info.mic_info.mic_dma_id, aud_tras_drv_info.mic_info.mic_ring_buff, aud_tras_drv_info.mic_info.frame_size * 2);
	} else if (aud_tras_drv_info.mic_info.mic_type == AUD_INTF_MIC_TYPE_UAC) {
		/* init mic_ring_buff */
		ring_buffer_init(&(aud_tras_drv_info.mic_info.mic_rb), (uint8_t*)aud_tras_drv_info.mic_info.mic_ring_buff, aud_tras_drv_info.mic_info.frame_size*2 + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL, DMA_ID_MAX, RB_DMA_TYPE_NULL);

		/* register uac connect and disconnect callback */
		bk_aud_uac_register_disconnect_cb(aud_tras_uac_disconnect_cb);
		bk_aud_uac_register_connect_cb(aud_tras_uac_connect_cb);

		LOGI("%s, %d, init uac driver \n", __func__, __LINE__);
		ret = bk_aud_uac_driver_init();
		if (ret != BK_OK) {
			LOGE("%s, %d, init uac driver fail \n", __func__, __LINE__);
			err = BK_ERR_AUD_INTF_UAC_DRV;
			goto aud_tras_drv_mic_init_exit;
		}
		LOGI("step2: init mic ring buff:%p, size:%d complete \n", aud_tras_drv_info.mic_info.mic_ring_buff, aud_tras_drv_info.voc_info.mic_samp_rate_points*2 + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL);
	} else {
		err = BK_ERR_AUD_INTF_PARAM;
		goto aud_tras_drv_mic_init_exit;
	}

	/* init mic temp buffer */
	aud_tras_drv_info.mic_info.temp_mic_addr = (int32_t *)audio_tras_drv_malloc(aud_tras_drv_info.mic_info.frame_size);
	if (aud_tras_drv_info.mic_info.temp_mic_addr == NULL) {
		LOGE("%s, %d, malloc temp mic ring buffer fail \n", __func__, __LINE__);
		err = BK_ERR_AUD_INTF_MEMY;
		goto aud_tras_drv_mic_init_exit;
	}
	LOGI("step3: init temp mic ring buff complete \n");

	/* change status: AUD_TRAS_DRV_MIC_NULL --> AUD_TRAS_DRV_MIC_IDLE */
	aud_tras_drv_info.mic_info.status = AUD_TRAS_DRV_MIC_STA_IDLE;

#ifdef CONFIG_UAC_MIC_SPK_COUNT_DEBUG
	if (aud_tras_drv_info.mic_info.mic_type == AUD_INTF_MIC_TYPE_UAC) {
		uac_mic_spk_count_open();
	}
#endif

	LOGI("step4: init mic complete \n");

	return BK_ERR_AUD_INTF_OK;

aud_tras_drv_mic_init_exit:
	aud_tras_drv_info.mic_info.mic_en = false;
	aud_tras_drv_info.mic_info.status = AUD_TRAS_DRV_MIC_STA_NULL;
	aud_tras_drv_info.mic_info.mic_chl = AUD_INTF_MIC_CHL_MIC1;

	if (aud_tras_drv_info.mic_info.adc_config)
		audio_tras_drv_free(aud_tras_drv_info.mic_info.adc_config);
	aud_tras_drv_info.mic_info.adc_config = NULL;

	if (aud_tras_drv_info.mic_info.uac_mic_config)
		audio_tras_drv_free(aud_tras_drv_info.mic_info.uac_mic_config);
	aud_tras_drv_info.mic_info.uac_mic_config = NULL;

	aud_tras_drv_info.mic_info.frame_size = 0;
	if (aud_tras_drv_info.mic_info.mic_dma_id != DMA_ID_MAX) {
		bk_dma_stop(aud_tras_drv_info.mic_info.mic_dma_id);
		bk_dma_deinit(aud_tras_drv_info.mic_info.mic_dma_id);
		bk_dma_free(DMA_DEV_AUDIO, aud_tras_drv_info.mic_info.mic_dma_id);
	}
	aud_tras_drv_info.mic_info.mic_dma_id = DMA_ID_MAX;

	if (aud_tras_drv_info.mic_info.mic_ring_buff != NULL) {
		ring_buffer_clear(&(aud_tras_drv_info.mic_info.mic_rb));
		audio_tras_drv_free(aud_tras_drv_info.mic_info.mic_ring_buff);
	}
	aud_tras_drv_info.mic_info.mic_ring_buff = NULL;

	if (aud_tras_drv_info.mic_info.temp_mic_addr != NULL)
		audio_tras_drv_free(aud_tras_drv_info.mic_info.temp_mic_addr);
	aud_tras_drv_info.mic_info.temp_mic_addr = NULL;

	aud_tras_drv_info.mic_info.mic_type = AUD_INTF_MIC_TYPE_MAX;

	return err;
}

static bk_err_t aud_tras_drv_mic_deinit(void)
{
	if (aud_tras_drv_info.mic_info.mic_type == AUD_INTF_MIC_TYPE_BOARD) {
		/* disable audio adc */
		bk_aud_adc_stop();
		bk_dma_stop(aud_tras_drv_info.mic_info.mic_dma_id);
		bk_dma_deinit(aud_tras_drv_info.mic_info.mic_dma_id);
		bk_dma_free(DMA_DEV_AUDIO, aud_tras_drv_info.mic_info.mic_dma_id);
		bk_aud_adc_deinit();
		if (aud_tras_drv_info.spk_info.status == AUD_TRAS_DRV_SPK_STA_NULL)
			bk_aud_driver_deinit();
		audio_tras_drv_free(aud_tras_drv_info.mic_info.adc_config);
		aud_tras_drv_info.mic_info.adc_config = NULL;
	} else {
		aud_tras_drv_info.uac_status = AUD_INTF_UAC_NORMAL_DISCONNECTED;
		bk_aud_uac_stop_mic();
//		bk_aud_uac_register_mic_callback(NULL);
		bk_aud_uac_driver_deinit();
	}

	aud_tras_drv_info.mic_info.mic_en = false;
	aud_tras_drv_info.mic_info.mic_chl = AUD_INTF_MIC_CHL_MIC1;
	aud_tras_drv_info.mic_info.frame_size = 0;
	aud_tras_drv_info.mic_info.mic_dma_id = DMA_ID_MAX;

	if (aud_tras_drv_info.mic_info.uac_mic_config)
		audio_tras_drv_free(aud_tras_drv_info.mic_info.uac_mic_config);
	aud_tras_drv_info.mic_info.uac_mic_config = NULL;

	ring_buffer_clear(&(aud_tras_drv_info.mic_info.mic_rb));
	audio_tras_drv_free(aud_tras_drv_info.mic_info.mic_ring_buff);
	aud_tras_drv_info.mic_info.mic_ring_buff = NULL;

	audio_tras_drv_free(aud_tras_drv_info.mic_info.temp_mic_addr);
	aud_tras_drv_info.mic_info.temp_mic_addr = NULL;

#ifdef CONFIG_UAC_MIC_SPK_COUNT_DEBUG
	if (aud_tras_drv_info.mic_info.mic_type == AUD_INTF_MIC_TYPE_UAC) {
		uac_mic_spk_count_close();
	}
#endif

	aud_tras_drv_info.mic_info.status = AUD_TRAS_DRV_MIC_STA_NULL;

	return BK_ERR_AUD_INTF_OK;
}

static bk_err_t aud_tras_drv_mic_start(void)
{
	bk_err_t ret = BK_OK;
	if ((aud_tras_drv_info.mic_info.status != AUD_TRAS_DRV_MIC_STA_IDLE) && (aud_tras_drv_info.mic_info.status != AUD_TRAS_DRV_MIC_STA_PAUSE)) {
		return BK_ERR_AUD_INTF_STA;
	}

	if (aud_tras_drv_info.mic_info.mic_type == AUD_INTF_MIC_TYPE_BOARD) {
		ret = bk_dma_start(aud_tras_drv_info.mic_info.mic_dma_id);
		if (ret != BK_OK) {
			LOGE("%s, %d, start mic dma fail \n", __func__, __LINE__);
			return BK_ERR_AUD_INTF_DMA;
		}

		/* enable adc */
		if (aud_tras_drv_info.mic_info.status == AUD_TRAS_DRV_MIC_STA_IDLE)
			bk_aud_adc_start();
	} else {
		LOGI("%s, %d, start uac mic \n", __func__, __LINE__);
		if (aud_tras_drv_info.uac_status == AUD_INTF_UAC_CONNECTED) {
			ret = bk_aud_uac_start_mic();
			if (ret != BK_OK) {
				LOGE("%s, %d, start uac mic fail, ret:%d \n", __func__, __LINE__, ret);
				return BK_ERR_AUD_INTF_UAC_MIC;
			} else {
				aud_tras_drv_info.uac_mic_open_status = true;
				aud_tras_drv_info.uac_mic_open_current = true;
			}
		} else {
			aud_tras_drv_info.uac_mic_open_status = true;
		}
	}

	aud_tras_drv_info.mic_info.status = AUD_TRAS_DRV_MIC_STA_START;

	return BK_ERR_AUD_INTF_OK;
}

/* not stop dma, only disable adc */
static bk_err_t aud_tras_drv_mic_pause(void)
{
	bk_err_t ret = BK_OK;
	if (aud_tras_drv_info.mic_info.status != AUD_TRAS_DRV_MIC_STA_START) {
		return BK_ERR_AUD_INTF_STA;
	}

	if (aud_tras_drv_info.mic_info.mic_type == AUD_INTF_MIC_TYPE_BOARD) {
		/* stop adc dma */
		//bk_aud_stop_adc();
		bk_dma_stop(aud_tras_drv_info.mic_info.mic_dma_id);
	} else {
		LOGI("%s, %d, stop uac mic \n", __func__, __LINE__);
		ret = bk_aud_uac_stop_mic();
		if (ret != BK_OK) {
			LOGE("%s, %d, stop uac mic fail \n", __func__, __LINE__);
			return BK_ERR_AUD_INTF_UAC_MIC;
		}
	}

	aud_tras_drv_info.mic_info.status = AUD_TRAS_DRV_MIC_STA_PAUSE;

	return BK_ERR_AUD_INTF_OK;
}

static bk_err_t aud_tras_drv_mic_stop(void)
{
	bk_err_t ret = BK_OK;
	if ((aud_tras_drv_info.mic_info.status != AUD_TRAS_DRV_MIC_STA_START) && (aud_tras_drv_info.mic_info.status != AUD_TRAS_DRV_MIC_STA_PAUSE)) {
		return BK_ERR_AUD_INTF_STA;
	}

	if (aud_tras_drv_info.mic_info.mic_type == AUD_INTF_MIC_TYPE_BOARD) {
		/* disable adc */
		bk_aud_adc_stop();
		bk_dma_stop(aud_tras_drv_info.mic_info.mic_dma_id);
		ring_buffer_clear(&(aud_tras_drv_info.mic_info.mic_rb));
	} else {
		LOGI("%s, %d, stop uac mic \n", __func__, __LINE__);
		ret = bk_aud_uac_stop_mic();
		if (ret != BK_OK) {
			LOGE("%s, %d, stop uac mic fail \n", __func__, __LINE__);
			return BK_ERR_AUD_INTF_UAC_MIC;
		} else {
			aud_tras_drv_info.uac_mic_open_status = false;
			aud_tras_drv_info.uac_mic_open_current = false;
		}
	}

	aud_tras_drv_info.mic_info.status = AUD_TRAS_DRV_MIC_STA_IDLE;

	return BK_ERR_AUD_INTF_OK;
}

/*
static bk_err_t aud_tras_drv_mic_get_gain(uint8_t *value)
{
	*value = aud_tras_drv_info.mic_info.adc_config->adc_set_gain;

	return BK_OK;
}
*/

static bk_err_t aud_tras_drv_mic_set_chl(aud_intf_mic_chl_t mic_chl)
{
	bk_err_t ret = BK_ERR_AUD_INTF_OK;
	switch (mic_chl) {
		case AUD_INTF_MIC_CHL_MIC1:
			ret = bk_aud_adc_set_chl(AUD_MIC_MIC1);
			if (ret == BK_OK)
				ret = bk_dma_set_src_data_width(aud_tras_drv_info.mic_info.mic_dma_id, DMA_DATA_WIDTH_16BITS);
			break;

		case AUD_INTF_MIC_CHL_DUAL:
			ret = bk_aud_adc_set_chl(AUD_MIC_BOTH);
			if (ret == BK_OK)
				ret = bk_dma_set_src_data_width(aud_tras_drv_info.mic_info.mic_dma_id, DMA_DATA_WIDTH_32BITS);
			break;

		default:
			LOGE("%s, %d, not support, mic_chl: %d \n", __func__, __LINE__, mic_chl);
			break;
	}

	return ret;
}

static bk_err_t aud_tras_drv_mic_set_samp_rate(uint32_t samp_rate)
{
	bk_err_t ret = BK_ERR_AUD_INTF_OK;
	ret = bk_aud_adc_set_samp_rate(samp_rate);

	return ret;
}


static bk_err_t aud_tras_drv_voc_deinit(void)
{
//	if (aud_tras_drv_info.voc_info.status == AUD_TRAS_DRV_VOC_STA_NULL)
//		return BK_ERR_AUD_INTF_OK;

	/* disable mic */
	if (aud_tras_drv_info.voc_info.mic_type == AUD_INTF_MIC_TYPE_BOARD) {
		bk_aud_adc_stop();
		bk_aud_adc_deinit();
		bk_dma_stop(aud_tras_drv_info.voc_info.adc_dma_id);
		bk_dma_deinit(aud_tras_drv_info.voc_info.adc_dma_id);
		bk_dma_free(DMA_DEV_AUDIO, aud_tras_drv_info.voc_info.adc_dma_id);
		if (aud_tras_drv_info.voc_info.adc_config) {
			audio_tras_drv_free(aud_tras_drv_info.voc_info.adc_config);
			aud_tras_drv_info.voc_info.adc_config = NULL;
		}
	} else {
		bk_aud_uac_stop_mic();
//		bk_aud_uac_unregister_mic_callback();
	}

	/* disable spk */
	if (aud_tras_drv_info.voc_info.spk_type == AUD_INTF_SPK_TYPE_BOARD) {
		bk_aud_dac_stop();
		aud_tras_dac_pa_ctrl(false);
		bk_aud_dac_deinit();
		if (aud_tras_drv_info.voc_info.dac_config) {
			audio_tras_drv_free(aud_tras_drv_info.voc_info.dac_config);
			aud_tras_drv_info.voc_info.dac_config = NULL;
		}
		/* stop dma */
		bk_dma_stop(aud_tras_drv_info.voc_info.dac_dma_id);
		bk_dma_deinit(aud_tras_drv_info.voc_info.dac_dma_id);
		bk_dma_free(DMA_DEV_AUDIO, aud_tras_drv_info.voc_info.dac_dma_id);
	} else {
		bk_aud_uac_stop_spk();
//		bk_aud_uac_unregister_spk_callback();
//		bk_aud_uac_register_spk_buff_ptr(NULL, 0);
	}

	if (aud_tras_drv_info.voc_info.mic_type == AUD_INTF_MIC_TYPE_BOARD || aud_tras_drv_info.voc_info.spk_type == AUD_INTF_SPK_TYPE_BOARD)
		bk_aud_driver_deinit();

	if (aud_tras_drv_info.voc_info.mic_type == AUD_INTF_MIC_TYPE_UAC || aud_tras_drv_info.voc_info.spk_type == AUD_INTF_SPK_TYPE_UAC) {
		aud_tras_drv_info.uac_status = AUD_INTF_UAC_NORMAL_DISCONNECTED;
		//bk_aud_uac_register_disconnect_cb(aud_tras_uac_disconnect_cb);
		bk_aud_uac_register_connect_cb(NULL);
		bk_aud_uac_driver_deinit();
	}

	/* disable AEC */
	aud_tras_drv_aec_decfg();

	if (aud_tras_drv_info.voc_info.uac_config) {
		audio_tras_drv_free(aud_tras_drv_info.voc_info.uac_config);
		aud_tras_drv_info.voc_info.uac_config = NULL;
	}

	/* free audio ring buffer */
	//mic deconfig
	ring_buffer_clear(&(aud_tras_drv_info.voc_info.mic_rb));
	if (aud_tras_drv_info.voc_info.mic_ring_buff) {
		audio_tras_drv_free(aud_tras_drv_info.voc_info.mic_ring_buff);
		aud_tras_drv_info.voc_info.mic_ring_buff = NULL;
	}
	aud_tras_drv_info.voc_info.mic_samp_rate_points = 0;
	aud_tras_drv_info.voc_info.mic_frame_number = 0;
	aud_tras_drv_info.voc_info.adc_dma_id = DMA_ID_MAX;

	//speaker deconfig
	ring_buffer_clear(&(aud_tras_drv_info.voc_info.speaker_rb));
	if (aud_tras_drv_info.voc_info.speaker_ring_buff) {
		audio_tras_drv_free(aud_tras_drv_info.voc_info.speaker_ring_buff);
		aud_tras_drv_info.voc_info.speaker_ring_buff = NULL;
	}
	aud_tras_drv_info.voc_info.speaker_samp_rate_points = 0;
	aud_tras_drv_info.voc_info.speaker_frame_number = 0;
	aud_tras_drv_info.voc_info.dac_dma_id = DMA_ID_MAX;

	/* tx and rx deconfig */
	//tx deconfig
	aud_tras_drv_info.voc_info.tx_info.tx_buff_status = false;
	aud_tras_drv_info.voc_info.tx_info.buff_length = 0;
	aud_tras_drv_info.voc_info.tx_info.ping.busy_status = false;
	aud_tras_drv_info.voc_info.tx_info.ping.buff_addr = NULL;
	//rx deconfig
	aud_tras_drv_info.voc_info.rx_info.rx_buff_status = false;
	aud_tras_drv_info.voc_info.rx_info.decoder_ring_buff = NULL;
	aud_tras_drv_info.voc_info.rx_info.decoder_rb = NULL;
	aud_tras_drv_info.voc_info.rx_info.frame_size = 0;
	aud_tras_drv_info.voc_info.rx_info.frame_num = 0;
	aud_tras_drv_info.voc_info.rx_info.rx_buff_seq_tail = 0;
	aud_tras_drv_info.voc_info.rx_info.aud_trs_read_seq = 0;
	aud_tras_drv_info.voc_info.rx_info.fifo_frame_num = 0;

	/* uac spk buffer */
	if (aud_tras_drv_info.voc_info.uac_spk_buff) {
		audio_tras_drv_free(aud_tras_drv_info.voc_info.uac_spk_buff);
		aud_tras_drv_info.voc_info.uac_spk_buff = NULL;
		aud_tras_drv_info.voc_info.uac_spk_buff_size = 0;
	}

	/* encoder_temp and decoder_temp deconfig*/
	if (aud_tras_drv_info.voc_info.encoder_temp.law_data) {
		audio_tras_drv_free(aud_tras_drv_info.voc_info.encoder_temp.law_data);
		aud_tras_drv_info.voc_info.encoder_temp.law_data = NULL;
	}
	if (aud_tras_drv_info.voc_info.decoder_temp.law_data) {
		audio_tras_drv_free(aud_tras_drv_info.voc_info.decoder_temp.law_data);
		aud_tras_drv_info.voc_info.decoder_temp.law_data = NULL;
	}
	if (aud_tras_drv_info.voc_info.encoder_temp.pcm_data) {
		audio_tras_drv_free(aud_tras_drv_info.voc_info.encoder_temp.pcm_data);
		aud_tras_drv_info.voc_info.encoder_temp.pcm_data = NULL;
	}
	if (aud_tras_drv_info.voc_info.decoder_temp.pcm_data) {
		audio_tras_drv_free(aud_tras_drv_info.voc_info.decoder_temp.pcm_data);
		aud_tras_drv_info.voc_info.decoder_temp.pcm_data = NULL;
	}
	aud_tras_drv_info.voc_info.aud_tx_rb = NULL;
	aud_tras_drv_info.voc_info.data_type = AUD_INTF_VOC_DATA_TYPE_PCM;

#ifdef CONFIG_UAC_MIC_SPK_COUNT_DEBUG
	if (aud_tras_drv_info.voc_info.mic_type == AUD_INTF_MIC_TYPE_UAC || aud_tras_drv_info.voc_info.spk_type == AUD_INTF_SPK_TYPE_UAC) {
		uac_mic_spk_count_close();
	}
#endif

	/* change status:
				AUD_TRAS_DRV_VOC_IDLE --> AUD_TRAS_DRV_VOC_NULL
				AUD_TRAS_DRV_VOC_START --> AUD_TRAS_DRV_VOC_NULL
				AUD_TRAS_DRV_VOC_STOP --> AUD_TRAS_DRV_VOC_NULL
	*/
	aud_tras_drv_info.voc_info.status = AUD_TRAS_DRV_VOC_STA_NULL;

#if CONFIG_AUD_TRAS_AEC_DUMP_MODE_UART
	bk_uart_deinit(CONFIG_AUD_TRAS_AEC_DUMP_UART_ID);
#endif

	LOGI("%s, %d, voc deinit complete \n", __func__, __LINE__);
	return BK_ERR_AUD_INTF_OK;
}

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
		LOGE("%s, %d, init uart fail \n", __func__, __LINE__);
	} else {
		LOGE("%s, %d, init uart ok \n", __func__, __LINE__);
	}
}

/* audio transfer driver voice mode init */
static bk_err_t aud_tras_drv_voc_init(aud_intf_voc_config_t* voc_cfg)
{
	bk_err_t ret = BK_OK;
	bk_err_t err = BK_ERR_AUD_INTF_FAIL;

	/* callback config */
//	aud_tras_drv_info.voc_info.aud_tras_drv_voc_event_cb = voc_cfg->aud_tras_drv_voc_event_cb;

	/* get aec config */
	aud_tras_drv_info.voc_info.aec_enable = voc_cfg->aec_enable;
	if (aud_tras_drv_info.voc_info.aec_enable) {
		aud_tras_drv_info.voc_info.aec_info = audio_tras_drv_malloc(sizeof(aec_info_t));
		if (aud_tras_drv_info.voc_info.aec_info == NULL) {
			LOGE("%s, %d, aec_info audio_tras_drv_malloc fail \n", __func__, __LINE__);
			err = BK_ERR_AUD_INTF_MEMY;
			goto aud_tras_drv_voc_init_exit;
		} else {
			aud_tras_drv_info.voc_info.aec_info->aec = NULL;
			aud_tras_drv_info.voc_info.aec_info->aec_config = NULL;
			aud_tras_drv_info.voc_info.aec_info->samp_rate = voc_cfg->samp_rate;
			aud_tras_drv_info.voc_info.aec_info->samp_rate_points = 0;
			aud_tras_drv_info.voc_info.aec_info->ref_addr = NULL;
			aud_tras_drv_info.voc_info.aec_info->mic_addr = NULL;
			aud_tras_drv_info.voc_info.aec_info->out_addr = NULL;
			aud_tras_drv_info.voc_info.aec_info->aec_ref_ring_buff = NULL;
			aud_tras_drv_info.voc_info.aec_info->aec_out_ring_buff = NULL;
			aud_tras_drv_info.voc_info.aec_info->ref_rb.address = NULL;
			aud_tras_drv_info.voc_info.aec_info->ref_rb.capacity = 0;
			aud_tras_drv_info.voc_info.aec_info->ref_rb.wp= 0;
			aud_tras_drv_info.voc_info.aec_info->ref_rb.rp = 0;
			aud_tras_drv_info.voc_info.aec_info->ref_rb.dma_id = DMA_ID_MAX;
			aud_tras_drv_info.voc_info.aec_info->ref_rb.dma_type = RB_DMA_TYPE_NULL;
			aud_tras_drv_info.voc_info.aec_info->aec_rb.address = NULL;
			aud_tras_drv_info.voc_info.aec_info->aec_rb.capacity = 0;
			aud_tras_drv_info.voc_info.aec_info->aec_rb.wp= 0;
			aud_tras_drv_info.voc_info.aec_info->aec_rb.rp = 0;
			aud_tras_drv_info.voc_info.aec_info->aec_rb.dma_id = DMA_ID_MAX;
			aud_tras_drv_info.voc_info.aec_info->aec_rb.dma_type = RB_DMA_TYPE_NULL;
			aud_tras_drv_info.voc_info.aec_info->aec_config = audio_tras_drv_malloc(sizeof(aec_config_t));
			if (aud_tras_drv_info.voc_info.aec_info->aec_config == NULL) {
				LOGE("%s, %d, aec_config_t audio_tras_drv_malloc fail \n", __func__, __LINE__);
				err = BK_ERR_AUD_INTF_MEMY;
				goto aud_tras_drv_voc_init_exit;
			} else {
				aud_tras_drv_info.voc_info.aec_info->aec_config->init_flags = voc_cfg->aec_setup->init_flags;
				aud_tras_drv_info.voc_info.aec_info->aec_config->mic_delay = voc_cfg->aec_setup->mic_delay;
				aud_tras_drv_info.voc_info.aec_info->aec_config->ec_depth = voc_cfg->aec_setup->ec_depth;
				aud_tras_drv_info.voc_info.aec_info->aec_config->ref_scale = voc_cfg->aec_setup->ref_scale;
				aud_tras_drv_info.voc_info.aec_info->aec_config->voice_vol = voc_cfg->aec_setup->voice_vol;
				aud_tras_drv_info.voc_info.aec_info->aec_config->TxRxThr = voc_cfg->aec_setup->TxRxThr;
				aud_tras_drv_info.voc_info.aec_info->aec_config->TxRxFlr = voc_cfg->aec_setup->TxRxFlr;
				aud_tras_drv_info.voc_info.aec_info->aec_config->ns_level = voc_cfg->aec_setup->ns_level;
				aud_tras_drv_info.voc_info.aec_info->aec_config->ns_para = voc_cfg->aec_setup->ns_para;
				aud_tras_drv_info.voc_info.aec_info->aec_config->drc = voc_cfg->aec_setup->drc;
			}
		}
	} else {
		aud_tras_drv_info.voc_info.aec_info = NULL;
	}
    // Modified by TUYA Start
    aud_tras_drv_info.voc_info.mic2_disable = voc_cfg->mic2_disable;
    // Modified by TUYA End

	aud_tras_drv_info.voc_info.aud_tx_rb = voc_cfg->aud_tx_rb;
	aud_tras_drv_info.voc_info.data_type = voc_cfg->data_type;
	LOGI("%s, %d, aud_tras_drv_info.voc_info.data_type:%d \n", __func__, __LINE__, aud_tras_drv_info.voc_info.data_type);
	aud_tras_drv_info.voc_info.mic_en = voc_cfg->mic_en;
	aud_tras_drv_info.voc_info.spk_en = voc_cfg->spk_en;
	aud_tras_drv_info.voc_info.mic_type = voc_cfg->mic_type;
	aud_tras_drv_info.voc_info.spk_type = voc_cfg->spk_type;

	if (aud_tras_drv_info.voc_info.mic_type == AUD_INTF_MIC_TYPE_BOARD) {
		/* get audio adc config */
		aud_tras_drv_info.voc_info.adc_config = audio_tras_drv_malloc(sizeof(aud_adc_config_t));
		if (aud_tras_drv_info.voc_info.adc_config == NULL) {
			LOGE("%s, %d, adc_config audio_tras_drv_malloc fail \n", __func__, __LINE__);
			err = BK_ERR_AUD_INTF_MEMY;
			goto aud_tras_drv_voc_init_exit;
		} else {
			aud_tras_drv_info.voc_info.adc_config->adc_chl =  AUD_ADC_CHL_LR;
			aud_tras_drv_info.voc_info.adc_config->samp_rate = voc_cfg->samp_rate;
			aud_tras_drv_info.voc_info.adc_config->adc_gain = voc_cfg->aud_setup.adc_gain;	//default: 0x2d
			aud_tras_drv_info.voc_info.adc_config->adc_samp_edge = AUD_ADC_SAMP_EDGE_RISING;
			aud_tras_drv_info.voc_info.adc_config->adc_mode = AUD_ADC_MODE_DIFFEN;
			aud_tras_drv_info.voc_info.adc_config->clk_src = AUD_CLK_XTAL;
		}
	} else {
		/* set audio uac config */
		if (aud_tras_drv_info.voc_info.uac_config == NULL) {
			aud_tras_drv_info.voc_info.uac_config = audio_tras_drv_malloc(sizeof(aud_uac_config_t));
			if (aud_tras_drv_info.voc_info.uac_config == NULL) {
				LOGE("%s, %d, uac_config audio_tras_drv_malloc fail \n", __func__, __LINE__);
				err = BK_ERR_AUD_INTF_MEMY;
				goto aud_tras_drv_voc_init_exit;
			} else {
				aud_tras_drv_info.voc_info.uac_config->mic_config.mic_format_tag = AUD_UAC_DATA_FORMAT_TYPE_PCM;
				aud_tras_drv_info.voc_info.uac_config->mic_config.mic_samp_rate = 8000;
				aud_tras_drv_info.voc_info.uac_config->spk_config.spk_format_tag = AUD_UAC_DATA_FORMAT_TYPE_PCM;
				aud_tras_drv_info.voc_info.uac_config->spk_config.spk_samp_rate = 8000;
				aud_tras_drv_info.voc_info.uac_config->spk_config.spk_volume = 0;
			}
			aud_tras_drv_info.voc_info.uac_config->mic_config.mic_samp_rate = voc_cfg->samp_rate;
			aud_tras_drv_info.voc_info.uac_config->spk_config.spk_samp_rate = voc_cfg->samp_rate;
			aud_tras_drv_info.voc_info.uac_config->spk_config.spk_volume = voc_cfg->aud_setup.dac_gain;
		}
	}

	if (aud_tras_drv_info.voc_info.spk_type == AUD_INTF_SPK_TYPE_BOARD) {
		/* get audio dac config */
		aud_tras_drv_info.voc_info.dac_config = audio_tras_drv_malloc(sizeof(aud_dac_config_t));
		if (aud_tras_drv_info.voc_info.adc_config == NULL) {
			LOGE("%s, %d, dac_config audio_tras_drv_malloc fail \n", __func__, __LINE__);
			err = BK_ERR_AUD_INTF_MEMY;
			goto aud_tras_drv_voc_init_exit;
		} else {
            // Modified by TUYA Start
			aud_tras_drv_info.voc_info.dac_config->samp_rate = voc_cfg->spk_samp_rate;
            // Modified by TUYA End
			aud_tras_drv_info.voc_info.dac_config->dac_chl = AUD_DAC_CHL_L;
			aud_tras_drv_info.voc_info.dac_config->work_mode = voc_cfg->aud_setup.spk_mode;
			aud_tras_drv_info.voc_info.dac_config->dac_gain = voc_cfg->aud_setup.dac_gain;	//default 2D  3F  15
			aud_tras_drv_info.voc_info.dac_config->dac_clk_invert = AUD_DAC_CLK_INVERT_RISING;
			aud_tras_drv_info.voc_info.dac_config->clk_src = AUD_CLK_XTAL;
		}
	}

	/* get ring buffer config */
	//aud_tras_drv_info.voc_info.mode = setup->aud_trs_mode;
    // Modified by TUYA Start
    if (aud_tras_drv_info.voc_info.aec_enable) {
		switch (voc_cfg->samp_rate) {
			case 8000:
				aud_tras_drv_info.voc_info.mic_samp_rate_points = 160;
				aud_tras_drv_info.voc_info.speaker_samp_rate_points = 160;
				break;

			case 16000:
				aud_tras_drv_info.voc_info.mic_samp_rate_points = 320;
				aud_tras_drv_info.voc_info.speaker_samp_rate_points = 320;
				break;

			default:
				break;
		}
    } else {
			aud_tras_drv_info.voc_info.mic_samp_rate_points = 320;
			aud_tras_drv_info.voc_info.speaker_samp_rate_points = 320;
    }
    bk_printf("%s, %d: mic: %d, spk: %d, rate: %d %d\n", __func__, __LINE__,
            voc_cfg->samp_rate, voc_cfg->spk_samp_rate,
            aud_tras_drv_info.voc_info.mic_samp_rate_points,
            aud_tras_drv_info.voc_info.speaker_samp_rate_points);
    // Modified by TUYA End

	aud_tras_drv_info.voc_info.mic_frame_number = voc_cfg->aud_setup.mic_frame_number;
	aud_tras_drv_info.voc_info.speaker_frame_number = voc_cfg->aud_setup.speaker_frame_number;

	/* get tx and rx context config */
	aud_tras_drv_info.voc_info.tx_info = voc_cfg->tx_info;
	aud_tras_drv_info.voc_info.rx_info = voc_cfg->rx_info;


	/*  -------------------------step0: init audio and config ADC and DAC -------------------------------- */
	/* config mailbox according audio transfer work mode */

	/*  -------------------------step2: init AEC and malloc two ring buffers -------------------------------- */
	/* init aec and config aec according AEC_enable*/
	if (aud_tras_drv_info.voc_info.aec_enable) {
		ret = aud_tras_drv_aec_cfg();
		if (ret != BK_OK) {
			err = BK_ERR_AUD_INTF_AEC;
			goto aud_tras_drv_voc_init_exit;
		}
		LOGI("%s, %d, aec samp_rate_points: %d \n", __func__, __LINE__, aud_tras_drv_info.voc_info.aec_info->samp_rate_points);
		ret = aud_tras_drv_aec_buff_cfg(aud_tras_drv_info.voc_info.aec_info);
		if (ret != BK_OK) {
			err = ret;
			goto aud_tras_drv_voc_init_exit;
		}
		LOGI("step2: init AEC and malloc two ring buffers complete \n");
	}

	/* -------------------step3: init and config DMA to carry mic and ref data ----------------------------- */
	aud_tras_drv_info.voc_info.mic_ring_buff = (int32_t *)audio_tras_drv_malloc(aud_tras_drv_info.voc_info.mic_samp_rate_points*4*aud_tras_drv_info.voc_info.mic_frame_number + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL);
	if (aud_tras_drv_info.voc_info.mic_ring_buff == NULL) {
		LOGE("%s, %d, malloc mic ring buffer fail \n", __func__, __LINE__);
		err = BK_ERR_AUD_INTF_MEMY;
		goto aud_tras_drv_voc_init_exit;
	}

	if (aud_tras_drv_info.voc_info.mic_type == AUD_INTF_MIC_TYPE_BOARD) {
		/* init dma driver */
		ret = bk_dma_driver_init();
		if (ret != BK_OK) {
			LOGE("%s, %d, dma driver init fail \n", __func__, __LINE__);
			err = BK_ERR_AUD_INTF_DMA;
			goto aud_tras_drv_voc_init_exit;
		}

		/* allocate free DMA channel */
		aud_tras_drv_info.voc_info.adc_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
		if ((aud_tras_drv_info.voc_info.adc_dma_id < DMA_ID_0) || (aud_tras_drv_info.voc_info.adc_dma_id >= DMA_ID_MAX)) {
			LOGE("%s, %d, malloc adc dma fail \n", __func__, __LINE__);
			err = BK_ERR_AUD_INTF_DMA;
			goto aud_tras_drv_voc_init_exit;
		}

		/* config audio adc dma to carry mic data to "mic_ring_buff" */
		ret = aud_tras_adc_dma_config(aud_tras_drv_info.voc_info.adc_dma_id, aud_tras_drv_info.voc_info.mic_ring_buff, (aud_tras_drv_info.voc_info.mic_samp_rate_points*4)*aud_tras_drv_info.voc_info.mic_frame_number + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL, aud_tras_drv_info.voc_info.mic_samp_rate_points*4, AUD_INTF_MIC_CHL_DUAL);
		if (ret != BK_OK) {
			LOGE("%s, %d, config audio adc dma fail \n", __func__, __LINE__);
			err = ret;
			goto aud_tras_drv_voc_init_exit;
		}

		ring_buffer_init(&(aud_tras_drv_info.voc_info.mic_rb), (uint8_t*)aud_tras_drv_info.voc_info.mic_ring_buff, aud_tras_drv_info.voc_info.mic_samp_rate_points*4*aud_tras_drv_info.voc_info.mic_frame_number + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL, aud_tras_drv_info.voc_info.adc_dma_id, RB_DMA_TYPE_WRITE);

		LOGI("step3: init and config mic DMA complete, adc_dma_id:%d, mic_ring_buff:%p, size:%d, carry_length:%d \n", aud_tras_drv_info.voc_info.adc_dma_id, aud_tras_drv_info.voc_info.mic_ring_buff, (aud_tras_drv_info.voc_info.mic_samp_rate_points*4)*aud_tras_drv_info.voc_info.mic_frame_number, aud_tras_drv_info.voc_info.mic_samp_rate_points*4);
	} else if (aud_tras_drv_info.voc_info.mic_type == AUD_INTF_MIC_TYPE_UAC) {
		/* init mic_ring_buff */
		ring_buffer_init(&(aud_tras_drv_info.voc_info.mic_rb), (uint8_t*)aud_tras_drv_info.voc_info.mic_ring_buff, aud_tras_drv_info.voc_info.mic_samp_rate_points*2*aud_tras_drv_info.voc_info.mic_frame_number + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL, DMA_ID_MAX, RB_DMA_TYPE_NULL);
		LOGI("%s, %d, uac mic_ring_buff:%p, size:%d \n", __func__, __LINE__, aud_tras_drv_info.voc_info.mic_ring_buff, aud_tras_drv_info.voc_info.mic_samp_rate_points*2*aud_tras_drv_info.voc_info.mic_frame_number + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL);

		/* debug */
//		uart_dump_mic_data(UART_ID_2, 2000000);

		/* register uac connect and disconnect callback */
		bk_aud_uac_register_disconnect_cb(aud_tras_uac_disconnect_cb);
		bk_aud_uac_register_connect_cb(aud_tras_uac_connect_cb);

		LOGI("%s, %d, init uac driver \n", __func__, __LINE__);
		ret = bk_aud_uac_driver_init();
		if (ret != BK_OK) {
			LOGE("%s, %d, init uac driver fail \n", __func__, __LINE__);
			err = BK_ERR_AUD_INTF_UAC_DRV;
			goto aud_tras_drv_voc_init_exit;
		}

		LOGI("step3: init voc mic ring buff:%p, size:%d complete \n", aud_tras_drv_info.voc_info.mic_ring_buff, aud_tras_drv_info.voc_info.mic_samp_rate_points*2*aud_tras_drv_info.voc_info.mic_frame_number + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL);
	} else {
		err = BK_ERR_AUD_INTF_UAC_MIC;
		goto aud_tras_drv_voc_init_exit;
	}

	/*  -------------------step4: init and config DMA to carry dac data ----------------------------- */
	aud_tras_drv_info.voc_info.speaker_ring_buff = (int32_t *)audio_tras_drv_malloc(aud_tras_drv_info.voc_info.speaker_samp_rate_points*2*aud_tras_drv_info.voc_info.speaker_frame_number + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL);
	if (aud_tras_drv_info.voc_info.speaker_ring_buff == NULL) {
		LOGE("%s, %d, malloc speaker ring buffer fail \n", __func__, __LINE__);
		err = BK_ERR_AUD_INTF_MEMY;
		goto aud_tras_drv_voc_init_exit;
	}

	if (aud_tras_drv_info.voc_info.spk_type == AUD_INTF_SPK_TYPE_BOARD) {
		/* init dma driver */
		ret = bk_dma_driver_init();
		if (ret != BK_OK) {
			LOGE("%s, %d, dma driver init fail \n", __func__, __LINE__);
			err = BK_ERR_AUD_INTF_DMA;
			goto aud_tras_drv_voc_init_exit;
		}

		/* allocate free DMA channel */
		aud_tras_drv_info.voc_info.dac_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
		if ((aud_tras_drv_info.voc_info.dac_dma_id < DMA_ID_0) || (aud_tras_drv_info.voc_info.dac_dma_id >= DMA_ID_MAX)) {
			LOGE("%s, %d, malloc dac dma fail \n", __func__, __LINE__);
			err = BK_ERR_AUD_INTF_DMA;
			goto aud_tras_drv_voc_init_exit;
		}

		/* config audio dac dma to carry dac data to "speaker_ring_buff" */
		ret = aud_tras_dac_dma_config(aud_tras_drv_info.voc_info.dac_dma_id, aud_tras_drv_info.voc_info.speaker_ring_buff, (aud_tras_drv_info.voc_info.speaker_samp_rate_points*2)*aud_tras_drv_info.voc_info.speaker_frame_number + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL, aud_tras_drv_info.voc_info.speaker_samp_rate_points*2, AUD_INTF_SPK_CHL_LEFT);
		if (ret != BK_OK) {
			LOGE("%s, %d, config audio adc dma fail \n", __func__, __LINE__);
			err = ret;
			goto aud_tras_drv_voc_init_exit;
		}

		ring_buffer_init(&(aud_tras_drv_info.voc_info.speaker_rb), (uint8_t*)aud_tras_drv_info.voc_info.speaker_ring_buff, aud_tras_drv_info.voc_info.speaker_samp_rate_points*2*aud_tras_drv_info.voc_info.speaker_frame_number + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL, aud_tras_drv_info.voc_info.dac_dma_id, RB_DMA_TYPE_READ);

		LOGI("step4: init and config speaker DMA complete, dac_dma_id:%d, speaker_ring_buff:%p, size:%d, carry_length:%d \r\n", aud_tras_drv_info.voc_info.dac_dma_id, aud_tras_drv_info.voc_info.speaker_ring_buff, (aud_tras_drv_info.voc_info.speaker_samp_rate_points*2)*aud_tras_drv_info.voc_info.speaker_frame_number + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL, aud_tras_drv_info.voc_info.speaker_samp_rate_points*2);
	} else if (aud_tras_drv_info.voc_info.spk_type == AUD_INTF_SPK_TYPE_UAC) {
		/* save one frame pcm speaker data for usb used */
		aud_tras_drv_info.voc_info.uac_spk_buff_size = aud_tras_drv_info.voc_info.speaker_samp_rate_points*2;
		aud_tras_drv_info.voc_info.uac_spk_buff = (uint8_t *)audio_tras_drv_malloc(aud_tras_drv_info.voc_info.uac_spk_buff_size);
		if (!aud_tras_drv_info.voc_info.uac_spk_buff) {
			err = BK_ERR_AUD_INTF_MEMY;
			goto aud_tras_drv_voc_init_exit;
		} else {
			os_memset(aud_tras_drv_info.voc_info.uac_spk_buff, 0x00, aud_tras_drv_info.voc_info.speaker_samp_rate_points*2);
		}

		ring_buffer_init(&(aud_tras_drv_info.voc_info.speaker_rb), (uint8_t*)aud_tras_drv_info.voc_info.speaker_ring_buff, aud_tras_drv_info.voc_info.speaker_samp_rate_points*2*aud_tras_drv_info.voc_info.speaker_frame_number + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL, DMA_ID_MAX, RB_DMA_TYPE_NULL);

		/* register uac connect and disconnect callback */
		if (aud_tras_drv_info.voc_info.mic_type != AUD_INTF_MIC_TYPE_UAC) {
			bk_aud_uac_register_disconnect_cb(aud_tras_uac_disconnect_cb);
			bk_aud_uac_register_connect_cb(aud_tras_uac_connect_cb);

			LOGI("%s, %d, init uac driver \n", __func__, __LINE__);
			ret = bk_aud_uac_driver_init();
			if (ret != BK_OK) {
				LOGE("%s, %d, init uac driver fail \n", __func__, __LINE__);
				err = BK_ERR_AUD_INTF_UAC_DRV;
				goto aud_tras_drv_voc_init_exit;
			}
		}

		LOGI("step4: init uac speaker_ring_buff:%p, spk_ring_buff_size:%d, uac_spk_buff:%p, uac_spk_buff_size:%d\n", aud_tras_drv_info.voc_info.speaker_ring_buff, (aud_tras_drv_info.voc_info.speaker_samp_rate_points*2)*aud_tras_drv_info.voc_info.speaker_frame_number + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL, aud_tras_drv_info.voc_info.uac_spk_buff, aud_tras_drv_info.voc_info.uac_spk_buff_size);
	} else {
		//TODO
	}

	/*  -------------------------step6: init all audio ring buffers -------------------------------- */
	/* init encoder and decoder temp buffer */
	aud_tras_drv_info.voc_info.encoder_temp.pcm_data = (int16_t *)audio_tras_drv_malloc(aud_tras_drv_info.voc_info.mic_samp_rate_points * 2);
	if (aud_tras_drv_info.voc_info.encoder_temp.pcm_data == NULL) {
		LOGE("%s, %d, malloc pcm_data of encoder used fail \n", __func__, __LINE__);
		err = BK_ERR_AUD_INTF_MEMY;
		goto aud_tras_drv_voc_init_exit;
	}

	aud_tras_drv_info.voc_info.decoder_temp.pcm_data = (int16_t *)audio_tras_drv_malloc(aud_tras_drv_info.voc_info.speaker_samp_rate_points*2);
	if (aud_tras_drv_info.voc_info.decoder_temp.pcm_data == NULL) {
		LOGE("%s, %d, malloc pcm_data of decoder used fail \n", __func__, __LINE__);
		err = BK_ERR_AUD_INTF_MEMY;
		goto aud_tras_drv_voc_init_exit;
	}

	switch (aud_tras_drv_info.voc_info.data_type) {
		case AUD_INTF_VOC_DATA_TYPE_G711A:
		case AUD_INTF_VOC_DATA_TYPE_G711U:
			LOGI("%s, %d, malloc law_data temp buffer \n", __func__, __LINE__);
			aud_tras_drv_info.voc_info.encoder_temp.law_data = (uint8_t *)audio_tras_drv_malloc(aud_tras_drv_info.voc_info.mic_samp_rate_points);
			if (aud_tras_drv_info.voc_info.encoder_temp.law_data == NULL) {
				LOGE("%s, %d, malloc law_data of encoder used fail \n", __func__, __LINE__);
				err = BK_ERR_AUD_INTF_MEMY;
				goto aud_tras_drv_voc_init_exit;
			}

			aud_tras_drv_info.voc_info.decoder_temp.law_data = (unsigned char *)audio_tras_drv_malloc(aud_tras_drv_info.voc_info.speaker_samp_rate_points);
			if (aud_tras_drv_info.voc_info.decoder_temp.law_data == NULL) {
				LOGE("%s, %d, malloc law_data of decoder used fail \n", __func__, __LINE__);
				err = BK_ERR_AUD_INTF_MEMY;
				goto aud_tras_drv_voc_init_exit;
			}
			break;

		case AUD_INTF_VOC_DATA_TYPE_PCM:
			//os_printf("not need to malloc law_data temp buffer \r\n");
			break;

		default:
			break;
	}

	/* audio debug */
	aud_tras_drv_info.voc_info.aud_tras_dump_tx_cb = NULL;
	aud_tras_drv_info.voc_info.aud_tras_dump_rx_cb = NULL;
	aud_tras_drv_info.voc_info.aud_tras_dump_aec_cb = NULL;

	/* change status: AUD_TRAS_DRV_VOC_NULL --> AUD_TRAS_DRV_VOC_IDLE */
	aud_tras_drv_info.voc_info.status = AUD_TRAS_DRV_VOC_STA_IDLE;
	LOGD("step6: init aud ring buff complete \n");

#ifdef CONFIG_UAC_MIC_SPK_COUNT_DEBUG
	if (aud_tras_drv_info.voc_info.mic_type == AUD_INTF_MIC_TYPE_UAC || aud_tras_drv_info.voc_info.spk_type == AUD_INTF_SPK_TYPE_UAC) {
		uac_mic_spk_count_open();
	}
#endif

	LOGI("%s, %d, init voc complete \n", __func__, __LINE__);

	return BK_ERR_AUD_INTF_OK;

aud_tras_drv_voc_init_exit:
	/* audio transfer driver deconfig */
	aud_tras_drv_voc_deinit();
	return err;
}

static bk_err_t aud_tras_drv_voc_start(void)
{
	bk_err_t ret = BK_OK;
	bk_err_t err = BK_ERR_AUD_INTF_FAIL;
	uint32_t size = 0;
	uint8_t *pcm_data = NULL;

	LOGI("%s \n", __func__);

	if (aud_tras_drv_info.voc_info.mic_en == AUD_INTF_VOC_MIC_OPEN) {
		if (aud_tras_drv_info.voc_info.mic_type == AUD_INTF_MIC_TYPE_BOARD) {
			/* init audio and config ADC and DAC */
			ret = aud_tras_adc_config(aud_tras_drv_info.voc_info.adc_config);
			if (ret != BK_OK) {
				LOGE("%s, %d, audio adc init fail \n", __func__, __LINE__);
				err = BK_ERR_AUD_INTF_ADC;
				goto audio_start_transfer_exit;
			}

			/* start DMA */
			ret = bk_dma_start(aud_tras_drv_info.voc_info.adc_dma_id);
			if (ret != BK_OK) {
				LOGE("%s, %d, start adc dma fail \n", __func__, __LINE__);
				err = BK_ERR_AUD_INTF_DMA;
				goto audio_start_transfer_exit;
			}

			/* enable adc */
			/* wait receive data and then open adc */
			bk_aud_adc_start();
		} else {
			LOGI("%s, %d, start uac mic \n", __func__, __LINE__);
			/* check uac connect status */
			if (aud_tras_drv_info.uac_status == AUD_INTF_UAC_CONNECTED) {
				ret = bk_aud_uac_start_mic();
				if (ret != BK_OK) {
					LOGE("%s, %d, start uac mic fail, ret: %d \n", __func__, __LINE__, ret);
					err = BK_ERR_AUD_INTF_UAC_MIC;
					goto audio_start_transfer_exit;
				} else {
					aud_tras_drv_info.uac_mic_open_status = true;
					aud_tras_drv_info.uac_mic_open_current = true;
				}
			} else {
				aud_tras_drv_info.uac_mic_open_status = true;
			}
		}
	}

	if (aud_tras_drv_info.voc_info.spk_en == AUD_INTF_VOC_SPK_OPEN) {
		pcm_data = (uint8_t *)audio_tras_drv_malloc(aud_tras_drv_info.voc_info.speaker_samp_rate_points*2);
		if (pcm_data == NULL) {
			LOGE("%s, %d, malloc temp pcm_data fial \n", __func__, __LINE__);
			err = BK_ERR_AUD_INTF_MEMY;
			goto audio_start_transfer_exit;
		} else {
			os_memset(pcm_data, 0x00, aud_tras_drv_info.voc_info.speaker_samp_rate_points*2);
		}

		if (aud_tras_drv_info.voc_info.spk_type == AUD_INTF_SPK_TYPE_BOARD) {
			ret = aud_tras_dac_config(aud_tras_drv_info.voc_info.dac_config);
			if (ret != BK_OK) {
				LOGE("%s, %d, audio dac init fail \n", __func__, __LINE__);
				err = BK_ERR_AUD_INTF_DAC;
				goto audio_start_transfer_exit;
			}

			/* enable dac */
			bk_aud_dac_start();
			aud_tras_dac_pa_ctrl(true);

			ret = bk_dma_start(aud_tras_drv_info.voc_info.dac_dma_id);
			if (ret != BK_OK) {
				LOGE("%s, %d, start dac dma fail \n", __func__, __LINE__);
				err = BK_ERR_AUD_INTF_DMA;
				goto audio_start_transfer_exit;
			}
		} else {
#if CONFIG_AUD_TRAS_UAC_SPK_VOL_CTRL_MODE_STOP_UAC_TRAS
			/* reopen uac mic */
			if ((aud_tras_drv_info.voc_info.mic_en == AUD_INTF_VOC_MIC_OPEN) && (aud_tras_drv_info.voc_info.mic_type == AUD_INTF_MIC_TYPE_UAC)) {
				bk_aud_uac_start_mic();
			}
#endif

			LOGI("%s, %d, start uac spk \n", __func__, __LINE__);
			/* check uac connect status */
			if (aud_tras_drv_info.uac_status == AUD_INTF_UAC_CONNECTED) {
				ret = bk_aud_uac_start_spk();
				if (ret != BK_OK) {
					LOGE("%s, %d, start uac spk fail, ret: %d \r\n", __func__, __LINE__, ret);
					err = BK_ERR_AUD_INTF_UAC_SPK;
					goto audio_start_transfer_exit;
				} else {
					aud_tras_drv_info.uac_spk_open_status = true;
					aud_tras_drv_info.uac_spk_open_current = true;
				}
			} else {
				aud_tras_drv_info.uac_spk_open_status = true;
			}
		}

		/* write two frame data to speaker and ref ring buffer */
		size = ring_buffer_write(&(aud_tras_drv_info.voc_info.speaker_rb), (uint8_t *)pcm_data, aud_tras_drv_info.voc_info.speaker_samp_rate_points*2);
		if (size != aud_tras_drv_info.voc_info.speaker_samp_rate_points*2) {
			LOGE("%s, %d, the data write to speaker_ring_buff error, size: %d \n", __func__, __LINE__, size);
			err = BK_ERR_AUD_INTF_RING_BUFF;
			goto audio_start_transfer_exit;
		}
		size = ring_buffer_write(&(aud_tras_drv_info.voc_info.speaker_rb), (uint8_t *)pcm_data, aud_tras_drv_info.voc_info.speaker_samp_rate_points*2);
		if (size != aud_tras_drv_info.voc_info.speaker_samp_rate_points*2) {
			LOGE("%s, %d, the data write to speaker_ring_buff error, size: %d \n", __func__, __LINE__, size);
			err = BK_ERR_AUD_INTF_RING_BUFF;
			goto audio_start_transfer_exit;
		}

		audio_tras_drv_free(pcm_data);
		pcm_data = NULL;
	}
	LOGI("%s, %d, voice start complete \n", __func__, __LINE__);

	/* change status:
				AUD_TRAS_DRV_VOC_STA_IDLE --> AUD_TRAS_DRV_VOC_STA_START
				AUD_TRAS_DRV_VOC_STA_STOP --> AUD_TRAS_DRV_VOC_STA_START
	*/
	aud_tras_drv_info.voc_info.status = AUD_TRAS_DRV_VOC_STA_START;


	return BK_ERR_AUD_INTF_OK;

audio_start_transfer_exit:
	//deinit audio transfer
	if (pcm_data)
		audio_tras_drv_free(pcm_data);

	return err;
}

static bk_err_t aud_tras_drv_voc_stop(void)
{
	bk_err_t ret = BK_OK;

	if (aud_tras_drv_info.voc_info.status == AUD_TRAS_DRV_VOC_STA_STOP)
		return ret;

	LOGI("%s \n", __func__);

	/* stop adc and dac dma */
	if (aud_tras_drv_info.voc_info.mic_type == AUD_INTF_MIC_TYPE_BOARD) {
		ret = bk_dma_stop(aud_tras_drv_info.voc_info.adc_dma_id);
		if (ret != BK_OK) {
			LOGE("%s, %d, stop adc dma fail \n", __func__, __LINE__);
			return BK_ERR_AUD_INTF_DMA;
		}
	} else {
		LOGI("%s, %d, stop uac mic \n", __func__, __LINE__);
		ret = bk_aud_uac_stop_mic();
		if (ret != BK_OK) {
			LOGE("%s, %d, stop uac mic fail \n", __func__, __LINE__);
			return BK_ERR_AUD_INTF_UAC_MIC;
		} else {
			aud_tras_drv_info.uac_mic_open_status = false;
			aud_tras_drv_info.uac_mic_open_current = false;
		}
	}

	if (aud_tras_drv_info.voc_info.spk_type == AUD_INTF_SPK_TYPE_BOARD) {
		ret = bk_dma_stop(aud_tras_drv_info.voc_info.dac_dma_id);
		if (ret != BK_OK) {
			LOGE("%s, %d, stop dac dma fail \n", __func__, __LINE__);
			return BK_ERR_AUD_INTF_DMA;
		}
	} else {
		LOGI("%s, %d, stop uac spk \n", __func__, __LINE__);
		ret = bk_aud_uac_stop_spk();
		if (ret != BK_OK) {
			//err = BK_ERR_AUD_INTF_DMA;
			LOGE("%s, %d, stop uac spk fail \n", __func__, __LINE__);
			return BK_ERR_AUD_INTF_UAC_SPK;
		} else {
			aud_tras_drv_info.uac_spk_open_status = false;
			aud_tras_drv_info.uac_spk_open_current = false;
		}
	}

	if (aud_tras_drv_info.voc_info.mic_type == AUD_INTF_MIC_TYPE_BOARD) {
		/* disable adc */
		bk_aud_adc_stop();
		bk_aud_adc_deinit();
	}

	if (aud_tras_drv_info.voc_info.spk_type == AUD_INTF_SPK_TYPE_BOARD) {
		/* disable dac */
		bk_aud_dac_stop();
		aud_tras_dac_pa_ctrl(false);
		bk_aud_dac_deinit();
	}

	if (aud_tras_drv_info.voc_info.mic_type == AUD_INTF_MIC_TYPE_BOARD || aud_tras_drv_info.voc_info.spk_type == AUD_INTF_SPK_TYPE_BOARD)
	/* deinit audio driver */
		bk_aud_driver_deinit();

	if (aud_tras_drv_info.voc_info.mic_type == AUD_INTF_MIC_TYPE_UAC || aud_tras_drv_info.voc_info.spk_type == AUD_INTF_SPK_TYPE_UAC)
		bk_aud_uac_driver_deinit();

	/* clear adc and dac ring buffer */
	ring_buffer_clear(&(aud_tras_drv_info.voc_info.speaker_rb));
	ring_buffer_clear(&(aud_tras_drv_info.voc_info.mic_rb));

	uac_mic_read_flag = false;
	uac_spk_write_flag = false;

	/* change status:
				AUD_TRAS_DRV_VOC_STA_IDLE --> AUD_TRAS_DRV_VOC_STA_STOP
				AUD_TRAS_DRV_VOC_STA_STOP --> AUD_TRAS_DRV_VOC_STA_STOP
	*/
	aud_tras_drv_info.voc_info.status = AUD_TRAS_DRV_VOC_STA_STOP;

	LOGI("%s, %d, stop voice transfer complete \n", __func__, __LINE__);

	return ret;
}

static bk_err_t aud_tras_drv_voc_ctrl_mic(aud_intf_voc_mic_ctrl_t mic_en)
{
	bk_err_t ret = BK_OK;
	bk_err_t err = BK_ERR_AUD_INTF_FAIL;

	GLOBAL_INT_DECLARATION();

	if (mic_en == AUD_INTF_VOC_MIC_OPEN) {
		if (aud_tras_drv_info.voc_info.mic_type == AUD_INTF_MIC_TYPE_BOARD) {
			LOGI("%s, %d, open onboard mic \n", __func__, __LINE__);
			/* enable adc */
			bk_aud_adc_start();

			ret = bk_dma_start(aud_tras_drv_info.voc_info.adc_dma_id);
			if (ret != BK_OK) {
				LOGE("%s, %d, start adc dma fail \n", __func__, __LINE__);
				err = BK_ERR_AUD_INTF_DMA;
				goto voc_ctrl_mic_fail;
			}
		} else {
			LOGI("%s, %d, open uac mic \n", __func__, __LINE__);
			ret = bk_aud_uac_start_mic();
			if (ret != BK_OK) {
				LOGE("%s, %d, start uac mic fail \n", __func__, __LINE__);
				err = BK_ERR_AUD_INTF_UAC_MIC;
				goto voc_ctrl_mic_fail;
			}
		}
	} else if (mic_en == AUD_INTF_VOC_MIC_CLOSE) {
		if (aud_tras_drv_info.voc_info.mic_type == AUD_INTF_MIC_TYPE_BOARD) {
			LOGI("%s, %d, close onboard mic \n", __func__, __LINE__);
			bk_aud_adc_stop();
			bk_dma_stop(aud_tras_drv_info.voc_info.adc_dma_id);
			ring_buffer_clear(&(aud_tras_drv_info.voc_info.mic_rb));
		} else {
			LOGI("%s, %d, close uac mic \n", __func__, __LINE__);
			ret = bk_aud_uac_stop_mic();
			if (ret != BK_OK) {
				LOGE("%s, %d, stop uac mic fail \n", __func__, __LINE__);
				err = BK_ERR_AUD_INTF_UAC_MIC;
				goto voc_ctrl_mic_fail;
			}
			uac_mic_read_flag = false;
		}
	} else {
		err = BK_ERR_AUD_INTF_PARAM;
		goto voc_ctrl_mic_fail;
	}

	GLOBAL_INT_DISABLE();
	if (aud_tras_drv_info.voc_info.mic_type == AUD_INTF_MIC_TYPE_BOARD)
		aud_tras_drv_info.voc_info.mic_en = mic_en;
	GLOBAL_INT_RESTORE();

	return BK_ERR_AUD_INTF_OK;

voc_ctrl_mic_fail:
	if (aud_tras_drv_info.voc_info.mic_type == AUD_INTF_MIC_TYPE_BOARD) {
		bk_aud_adc_stop();
		bk_dma_stop(aud_tras_drv_info.voc_info.adc_dma_id);
	}

	return err;
}

static bk_err_t aud_tras_drv_voc_ctrl_spk(aud_intf_voc_spk_ctrl_t spk_en)
{
	bk_err_t ret = BK_OK;
	bk_err_t err = BK_ERR_AUD_INTF_FAIL;
	uint32_t size = 0;
	uint8_t *pcm_data = NULL;

	GLOBAL_INT_DECLARATION();

	if (spk_en == AUD_INTF_VOC_SPK_OPEN) {
		pcm_data = (uint8_t *)audio_tras_drv_malloc(aud_tras_drv_info.voc_info.mic_samp_rate_points*2);
		if (pcm_data == NULL) {
			LOGE("%s, %d, malloc temp pcm_data fial \n", __func__, __LINE__);
			err = BK_ERR_AUD_INTF_MEMY;
			goto voc_ctrl_spk_fail;
		} else {
			os_memset(pcm_data, 0x00, aud_tras_drv_info.voc_info.mic_samp_rate_points*2);
		}

		if (aud_tras_drv_info.voc_info.spk_type == AUD_INTF_SPK_TYPE_BOARD) {
			LOGI("%s, %d, open onboard spk \n", __func__, __LINE__);
			/* enable dac */
			bk_aud_dac_start();
			aud_tras_dac_pa_ctrl(true);

			ret = bk_dma_start(aud_tras_drv_info.voc_info.dac_dma_id);
			if (ret != BK_OK) {
				LOGE("%s, %d, start dac dma fail \n", __func__, __LINE__);
				err = BK_ERR_AUD_INTF_DMA;
				goto voc_ctrl_spk_fail;
			}
		} else {
			LOGI("%s, %d, open uac spk \n", __func__, __LINE__);
			ret = bk_aud_uac_start_spk();
			if (ret != BK_OK) {
				LOGE("%s, %d, open uac spk fail \n", __func__, __LINE__);
				err = BK_ERR_AUD_INTF_UAC_SPK;
				goto voc_ctrl_spk_fail;
			}
			uac_spk_write_flag = false;
		}

		/* write two frame data to speaker and ref ring buffer */
		size = ring_buffer_write(&(aud_tras_drv_info.voc_info.speaker_rb), (uint8_t *)pcm_data, aud_tras_drv_info.voc_info.mic_samp_rate_points*2);
		if (size != aud_tras_drv_info.voc_info.mic_samp_rate_points*2) {
			LOGE("%s, %d, the data writeten to speaker_ring_buff error, size: %d \n", __func__, __LINE__, size);
			err = BK_ERR_AUD_INTF_RING_BUFF;
			goto voc_ctrl_spk_fail;
		}

		size = ring_buffer_write(&(aud_tras_drv_info.voc_info.speaker_rb), (uint8_t *)pcm_data, aud_tras_drv_info.voc_info.mic_samp_rate_points*2);
		if (size != aud_tras_drv_info.voc_info.mic_samp_rate_points*2) {
			LOGE("%s, %d, the data writeten to speaker_ring_buff error, size: %d !!!!!!!!\n", __func__, __LINE__, size);
		//	err = BK_ERR_AUD_INTF_RING_BUFF;
		//	goto voc_ctrl_spk_fail;
		}

		audio_tras_drv_free(pcm_data);
		pcm_data = NULL;
	} else if (spk_en == AUD_INTF_VOC_SPK_CLOSE) {
		if (aud_tras_drv_info.voc_info.spk_type == AUD_INTF_SPK_TYPE_BOARD) {
			LOGI("%s, %d, close onboard spk \n", __func__, __LINE__);
			bk_aud_dac_stop();
			aud_tras_dac_pa_ctrl(false);
			bk_dma_stop(aud_tras_drv_info.voc_info.dac_dma_id);
		} else {
			LOGI("%s, %d, close uac spk \n", __func__, __LINE__);
			ret = bk_aud_uac_stop_spk();
			if (ret != BK_OK) {
				LOGE("%s, %d, close uac spk fail \n", __func__, __LINE__);
				err = BK_ERR_AUD_INTF_UAC_SPK;
				goto voc_ctrl_spk_fail;
			}
		}
		ring_buffer_clear(&(aud_tras_drv_info.voc_info.speaker_rb));
	} else {
		err = BK_ERR_AUD_INTF_PARAM;
		goto voc_ctrl_spk_fail;
	}

	GLOBAL_INT_DISABLE();
	if (aud_tras_drv_info.voc_info.spk_type == AUD_INTF_SPK_TYPE_BOARD)
		aud_tras_drv_info.voc_info.spk_en = spk_en;
	GLOBAL_INT_RESTORE();

	return BK_ERR_AUD_INTF_OK;

voc_ctrl_spk_fail:
	if (pcm_data)
		audio_tras_drv_free(pcm_data);

	if (spk_en == AUD_INTF_VOC_SPK_OPEN) {
		if (aud_tras_drv_info.voc_info.spk_type == AUD_INTF_SPK_TYPE_BOARD) {
			bk_aud_dac_stop();
			aud_tras_dac_pa_ctrl(false);
			bk_dma_stop(aud_tras_drv_info.voc_info.dac_dma_id);
		} else {
			bk_aud_uac_stop_spk();
		}
	}

	return err;
}

static bk_err_t aud_tras_drv_voc_ctrl_aec(bool aec_en)
{
	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();
	aud_tras_drv_info.voc_info.aec_enable = aec_en;
	GLOBAL_INT_RESTORE();

	return BK_ERR_AUD_INTF_OK;
}

static bk_err_t aud_tras_drv_set_aec_para(aud_intf_voc_aec_ctl_t *aec_ctl)
{
	switch (aec_ctl->op) {
		case AUD_INTF_VOC_AEC_MIC_DELAY:
			aud_tras_drv_info.voc_info.aec_info->aec_config->mic_delay = aec_ctl->value;
			aec_ctrl(aud_tras_drv_info.voc_info.aec_info->aec, AEC_CTRL_CMD_SET_MIC_DELAY, aud_tras_drv_info.voc_info.aec_info->aec_config->mic_delay);
			break;

		case AUD_INTF_VOC_AEC_EC_DEPTH:
			aud_tras_drv_info.voc_info.aec_info->aec_config->ec_depth = aec_ctl->value;
			aec_ctrl(aud_tras_drv_info.voc_info.aec_info->aec, AEC_CTRL_CMD_SET_EC_DEPTH, aud_tras_drv_info.voc_info.aec_info->aec_config->ec_depth);
			break;

		case AUD_INTF_VOC_AEC_REF_SCALE:
			aud_tras_drv_info.voc_info.aec_info->aec_config->ref_scale = (uint8_t)aec_ctl->value;
			aec_ctrl(aud_tras_drv_info.voc_info.aec_info->aec, AEC_CTRL_CMD_SET_REF_SCALE, aud_tras_drv_info.voc_info.aec_info->aec_config->ref_scale);
			break;

		case AUD_INTF_VOC_AEC_VOICE_VOL:
			aud_tras_drv_info.voc_info.aec_info->aec_config->voice_vol = (uint8_t)aec_ctl->value;
			aec_ctrl(aud_tras_drv_info.voc_info.aec_info->aec, AEC_CTRL_CMD_SET_VOL, aud_tras_drv_info.voc_info.aec_info->aec_config->voice_vol);
			break;

		case AUD_INTF_VOC_AEC_TXRX_THR:
//			aud_tras_drv_info.voc_info.aec_info->aec_config->TxRxThr = aec_ctl->value;
//			aec_ctrl(aud_tras_drv_info.voc_info.aec_info->aec, AEC_CTRL_CMD_SET_TxRxThr, aud_tras_drv_info.voc_info.aec_info->aec_config->TxRxThr);
			break;

		case AUD_INTF_VOC_AEC_TXRX_FLR:
//			aud_tras_drv_info.voc_info.aec_info->aec_config->TxRxFlr = aec_ctl->value;
//			aec_ctrl(aud_tras_drv_info.voc_info.aec_info->aec, AEC_CTRL_CMD_SET_TxRxFlr, aud_tras_drv_info.voc_info.aec_info->aec_config->TxRxFlr);
			break;

		case AUD_INTF_VOC_AEC_NS_LEVEL:
			aud_tras_drv_info.voc_info.aec_info->aec_config->ns_level = (uint8_t)aec_ctl->value;
			aec_ctrl(aud_tras_drv_info.voc_info.aec_info->aec, AEC_CTRL_CMD_SET_NS_LEVEL, aud_tras_drv_info.voc_info.aec_info->aec_config->ns_level);
			break;

		case AUD_INTF_VOC_AEC_NS_PARA:
			aud_tras_drv_info.voc_info.aec_info->aec_config->ns_para = (uint8_t)aec_ctl->value;
			aec_ctrl(aud_tras_drv_info.voc_info.aec_info->aec, AEC_CTRL_CMD_SET_NS_PARA, aud_tras_drv_info.voc_info.aec_info->aec_config->ns_para);
			break;

		case AUD_INTF_VOC_AEC_DRC:
			aud_tras_drv_info.voc_info.aec_info->aec_config->drc = (uint8_t)aec_ctl->value;
			aec_ctrl(aud_tras_drv_info.voc_info.aec_info->aec, AEC_CTRL_CMD_SET_DRC, aud_tras_drv_info.voc_info.aec_info->aec_config->drc);
			break;

		case AUD_INTF_VOC_AEC_INIT_FLAG:
			aud_tras_drv_info.voc_info.aec_info->aec_config->init_flags = (uint16_t)aec_ctl->value;
			aec_ctrl(aud_tras_drv_info.voc_info.aec_info->aec, AEC_CTRL_CMD_SET_FLAGS, aud_tras_drv_info.voc_info.aec_info->aec_config->init_flags);
			break;

		default:
			break;
	}

	return BK_ERR_AUD_INTF_OK;
}

static bk_err_t aud_tras_drv_get_aec_para(void)
{
	LOGI("aud_intf aec params: \n");
	LOGI("init_flags: %d \n", aud_tras_drv_info.voc_info.aec_info->aec_config->init_flags);
	LOGI("ec_depth: %d \n", aud_tras_drv_info.voc_info.aec_info->aec_config->ec_depth);
	LOGI("ref_scale: %d \n", aud_tras_drv_info.voc_info.aec_info->aec_config->ref_scale);
	LOGI("voice_vol: %d \n", aud_tras_drv_info.voc_info.aec_info->aec_config->voice_vol);
	LOGI("TxRxThr: %d \n", aud_tras_drv_info.voc_info.aec_info->aec_config->TxRxThr);
	LOGI("TxRxFlr: %d \n", aud_tras_drv_info.voc_info.aec_info->aec_config->TxRxFlr);
	LOGI("ns_level: %d \n", aud_tras_drv_info.voc_info.aec_info->aec_config->ns_level);
	LOGI("ns_para: %d \n", aud_tras_drv_info.voc_info.aec_info->aec_config->ns_para);
	LOGI("drc: %d \n", aud_tras_drv_info.voc_info.aec_info->aec_config->drc);
	LOGI("end \n");

	return BK_ERR_AUD_INTF_OK;
}

static bk_err_t aud_tras_drv_voc_tx_debug(aud_intf_dump_data_callback callback)
{
	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();
	aud_tras_drv_info.voc_info.aud_tras_dump_tx_cb = callback;
	GLOBAL_INT_RESTORE();

	return BK_ERR_AUD_INTF_OK;
}

static bk_err_t aud_tras_drv_voc_rx_debug(aud_intf_dump_data_callback callback)
{
	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();
	aud_tras_drv_info.voc_info.aud_tras_dump_rx_cb = callback;
	GLOBAL_INT_RESTORE();

	return BK_ERR_AUD_INTF_OK;
}

static bk_err_t aud_tras_drv_voc_aec_debug(aud_intf_dump_data_callback callback)
{
	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();
	aud_tras_drv_info.voc_info.aud_tras_dump_aec_cb = callback;
	GLOBAL_INT_RESTORE();

	return BK_ERR_AUD_INTF_OK;
}

static bk_err_t aud_tras_drv_uac_register_connect_state_cb(void * cb)
{
	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();
	aud_tras_drv_info.uac_connect_state_cb_exist = true;
	aud_tras_drv_info.aud_tras_drv_uac_connect_state_cb = cb;
	GLOBAL_INT_RESTORE();

	return BK_ERR_AUD_INTF_OK;
}

static bk_err_t aud_tras_drv_spk_set_samp_rate(uint32_t samp_rate)
{
	bk_err_t ret = BK_ERR_AUD_INTF_OK;

	bk_aud_dac_stop();
	aud_tras_dac_pa_ctrl(false);
	ret = bk_aud_dac_set_samp_rate(samp_rate);
	bk_aud_dac_start();
	bk_aud_dac_start();
	aud_tras_dac_pa_ctrl(true);

	return ret;
}



bk_err_t aud_tras_drv_set_work_mode(aud_intf_work_mode_t mode)
{
	bk_err_t ret = BK_OK;

	LOGI("%s, %d, set mode: %d \n", __func__, __LINE__, mode);

	switch (mode) {
		case AUD_INTF_WORK_MODE_GENERAL:
			if ((aud_tras_drv_info.work_mode == AUD_INTF_WORK_MODE_VOICE) && (aud_tras_drv_info.voc_info.status != AUD_TRAS_DRV_VOC_STA_NULL)) {
				ret = aud_tras_drv_voc_deinit();
				if (ret != BK_OK) {
					LOGE("%s, %d, spk deinit fail \n", __func__, __LINE__);
					return BK_ERR_AUD_INTF_FAIL;
				}
			}
			aud_tras_drv_info.work_mode = AUD_INTF_WORK_MODE_GENERAL;
			break;

		case AUD_INTF_WORK_MODE_VOICE:
			if (aud_tras_drv_info.work_mode == AUD_INTF_WORK_MODE_GENERAL) {
				if (aud_tras_drv_info.mic_info.status != AUD_TRAS_DRV_MIC_STA_NULL) {
					ret = aud_tras_drv_mic_deinit();
					if (ret != BK_OK) {
						LOGE("%s, %d, mic deinit fail \n", __func__, __LINE__);
						return BK_ERR_AUD_INTF_FAIL;
					}
				}
				if (aud_tras_drv_info.spk_info.status != AUD_TRAS_DRV_SPK_STA_NULL) {
					ret = aud_tras_drv_spk_deinit();
					if (ret != BK_OK) {
						LOGE("%s, %d, spk deinit fail \n", __func__, __LINE__);
						return BK_ERR_AUD_INTF_FAIL;
					}
				}
			}
			aud_tras_drv_info.work_mode = AUD_INTF_WORK_MODE_VOICE;
			break;

		case AUD_INTF_WORK_MODE_NULL:
			if ((aud_tras_drv_info.work_mode == AUD_INTF_WORK_MODE_VOICE) && (aud_tras_drv_info.voc_info.status != AUD_TRAS_DRV_VOC_STA_NULL)) {
				ret = aud_tras_drv_voc_deinit();
				if (ret != BK_OK) {
					LOGE("%s, %d, spk deinit fail \n", __func__, __LINE__);
					return BK_ERR_AUD_INTF_FAIL;
				}
			}
			if (aud_tras_drv_info.work_mode == AUD_INTF_WORK_MODE_GENERAL) {
				if (aud_tras_drv_info.mic_info.status != AUD_TRAS_DRV_MIC_STA_NULL) {
					ret = aud_tras_drv_mic_deinit();
					if (ret != BK_OK) {
						LOGE("%s, %d, mic deinit fail \n", __func__, __LINE__);
						return BK_ERR_AUD_INTF_FAIL;
					}
				}
				if (aud_tras_drv_info.spk_info.status != AUD_TRAS_DRV_SPK_STA_NULL) {
					ret = aud_tras_drv_spk_deinit();
					if (ret != BK_OK) {
						LOGE("%s, %d, spk deinit fail \n", __func__, __LINE__);
						return BK_ERR_AUD_INTF_FAIL;
					}
				}
			}
			aud_tras_drv_info.work_mode = AUD_INTF_WORK_MODE_NULL;
			break;

		default:
			return BK_ERR_AUD_INTF_FAIL;
			break;
	}

	return BK_ERR_AUD_INTF_OK;
}

static bk_err_t aud_tras_drv_set_mic_gain(uint8_t value)
{
	bk_err_t ret = BK_ERR_AUD_INTF_OK;

	if (aud_tras_drv_info.mic_info.mic_type == AUD_INTF_MIC_TYPE_BOARD || aud_tras_drv_info.voc_info.mic_type == AUD_INTF_MIC_TYPE_BOARD) {
		bk_aud_adc_set_gain((uint32_t)value);
		ret = BK_ERR_AUD_INTF_OK;
	}

	return ret;
}

#if CONFIG_AUD_TRAS_UAC_SPK_VOL_CTRL_MODE_STOP_UAC_TRAS
static void uac_mic_spk_recover(void)
{
	LOGD("%s \n", __func__);
	if (aud_tras_drv_info.work_mode == AUD_INTF_WORK_MODE_GENERAL) {
		/* check mic status */
		if ((aud_tras_drv_info.mic_info.status == AUD_TRAS_DRV_MIC_STA_START) && (aud_tras_drv_info.mic_info.mic_type == AUD_INTF_MIC_TYPE_UAC)) {
			bk_aud_uac_start_mic();
		}

		/* check spk status */
		if ((aud_tras_drv_info.spk_info.status == AUD_TRAS_DRV_SPK_STA_START) && (aud_tras_drv_info.spk_info.spk_type == AUD_INTF_SPK_TYPE_UAC)) {
			bk_aud_uac_start_spk();
		}
	} else {
		if (aud_tras_drv_info.voc_info.status == AUD_TRAS_DRV_VOC_STA_START) {
			if ((aud_tras_drv_info.voc_info.mic_type == AUD_INTF_MIC_TYPE_UAC) && (aud_tras_drv_info.voc_info.mic_en == AUD_INTF_VOC_MIC_OPEN)) {
				bk_aud_uac_start_mic();
			}
			if ((aud_tras_drv_info.voc_info.spk_type == AUD_INTF_SPK_TYPE_UAC) && (aud_tras_drv_info.voc_info.spk_en == AUD_INTF_VOC_SPK_OPEN)) {
				bk_aud_uac_start_spk();
			}
		}
	}
}
#endif //CONFIG_AUD_TRAS_UAC_SPK_VOL_CTRL_MODE_STOP_UAC_TRAS

static bk_err_t aud_tras_drv_set_spk_gain(uint16_t value)
{
	bk_err_t ret = BK_OK;
	if (aud_tras_drv_info.spk_info.spk_type == AUD_INTF_SPK_TYPE_BOARD || aud_tras_drv_info.voc_info.spk_type == AUD_INTF_SPK_TYPE_BOARD) {
		bk_aud_dac_set_gain((uint32_t)value);
		return BK_ERR_AUD_INTF_OK;
	}

	if (aud_tras_drv_info.spk_info.spk_type == AUD_INTF_SPK_TYPE_UAC || aud_tras_drv_info.voc_info.spk_type == AUD_INTF_SPK_TYPE_UAC) {
		/* check uac support volume configuration */
		if (bk_aud_uac_check_spk_gain_cfg() == BK_OK) {
			LOGI("%s, %d, set uac speaker volume: %d \n", __func__, __LINE__, value);
#if CONFIG_AUD_TRAS_UAC_SPK_VOL_CTRL_MODE_STOP_UAC_TRAS
			/* step1: stop uac mic and speaker
			   step2: set volume
			   step3: recover uac mic and speaker status
			*/
			/* step1: stop uac mic and speaker */
			bk_aud_uac_stop_mic();
			bk_aud_uac_stop_spk();

			/* step2: set volume */
			ret = bk_aud_uac_set_spk_gain((uint32_t)value);

			/* step3: recover uac mic and speaker status */
			uac_mic_spk_recover();
#endif

#if CONFIG_AUD_TRAS_UAC_SPK_VOL_CTRL_MODE_DIRECT
			ret = bk_aud_uac_set_spk_gain((uint32_t)value);
#endif

#if CONFIG_AUD_TRAS_UAC_SPK_VOL_CTRL_MODE_MUTE
			bk_aud_uac_ctrl_spk_mute(1);
			ret = bk_aud_uac_set_spk_gain((uint32_t)value);
			bk_aud_uac_ctrl_spk_mute(0);
#endif

			return ret;
		} else {
			LOGW("%s, %d, The uac speaker not support volume configuration \n", __func__, __LINE__);
			return BK_ERR_AUD_INTF_PARAM;
		}
	}

	return ret;
}


static void aud_tras_drv_main(beken_thread_arg_t param_data)
{
	bk_err_t ret = BK_OK;
	aud_intf_drv_config_t *aud_trs_setup = NULL;

	aud_trs_setup = (aud_intf_drv_config_t *)param_data;

	aud_tras_drv_info.work_mode = aud_trs_setup->setup.work_mode;
	aud_tras_drv_info.aud_tras_tx_mic_data = aud_trs_setup->setup.aud_intf_tx_mic_data;
	aud_tras_drv_info.aud_tras_rx_spk_data = aud_trs_setup->setup.aud_intf_rx_spk_data;

	/* set work status to IDLE */
	aud_tras_drv_info.status = AUD_TRAS_DRV_STA_IDLE;

	rtos_set_semaphore(&aud_tras_drv_task_sem);

//	bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_320M);

	while(1) {
		aud_tras_drv_msg_t msg;
		media_mailbox_msg_t *mailbox_msg = NULL;
		//GPIO_UP(5);
		ret = rtos_pop_from_queue(&aud_trs_drv_int_msg_que, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == ret) {
			switch (msg.op) {
				case AUD_TRAS_DRV_IDLE:
					break;

				case AUD_TRAS_DRV_EXIT:
					LOGD("%s, %d, goto: AUD_TRAS_DRV_EXIT \n", __func__, __LINE__);
					goto aud_tras_drv_exit;
					break;

				case AUD_TRAS_DRV_SET_MODE:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_set_work_mode((aud_intf_work_mode_t)mailbox_msg->param);
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				/* mic op */
				case AUD_TRAS_DRV_MIC_INIT:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_mic_init((aud_intf_mic_config_t *)mailbox_msg->param);
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_MIC_DEINIT:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_mic_deinit();
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_MIC_START:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_mic_start();
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_MIC_PAUSE:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_mic_pause();
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_MIC_STOP:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_mic_stop();
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_MIC_SET_CHL:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_mic_set_chl((aud_intf_mic_chl_t)mailbox_msg->param);
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_MIC_SET_SAMP_RATE:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_mic_set_samp_rate(mailbox_msg->param);
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_MIC_SET_GAIN:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_set_mic_gain(*((uint8_t *)mailbox_msg->param));
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_MIC_TX_DATA:
					aud_tras_drv_mic_tx_data();
					break;

				/* spk op */
				case AUD_TRAS_DRV_SPK_INIT:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_spk_init((aud_intf_spk_config_t *)mailbox_msg->param);
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_SPK_DEINIT:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_spk_deinit();
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_SPK_START:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_spk_start();
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_SPK_PAUSE:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_spk_pause();
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_SPK_STOP:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_spk_stop();
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_SPK_SET_CHL:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_spk_set_chl((aud_intf_spk_chl_t)mailbox_msg->param);
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_SPK_SET_SAMP_RATE:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_spk_set_samp_rate(mailbox_msg->param);
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_SPK_SET_GAIN:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_set_spk_gain(*((uint16_t *)mailbox_msg->param));
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_SPK_REQ_DATA:
					aud_tras_drv_spk_req_data((audio_packet_t *)msg.param);
					break;

				/* voc op */
				case AUD_TRAS_DRV_VOC_INIT:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_voc_init((aud_intf_voc_config_t *)mailbox_msg->param);
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_VOC_DEINIT:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					if (aud_tras_drv_info.voc_info.status != AUD_TRAS_DRV_VOC_STA_NULL) {
						ret = aud_tras_drv_voc_deinit();
					} else {
						ret = BK_ERR_AUD_INTF_OK;
					}
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_VOC_START:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_voc_start();
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_VOC_STOP:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_voc_stop();
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_VOC_CTRL_MIC:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_voc_ctrl_mic((aud_intf_voc_mic_ctrl_t)mailbox_msg->param);
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_VOC_CTRL_SPK:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_voc_ctrl_spk((aud_intf_voc_spk_ctrl_t)mailbox_msg->param);
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_VOC_CTRL_AEC:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_voc_ctrl_aec((bool)mailbox_msg->param);
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_VOC_SET_MIC_GAIN:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_set_mic_gain(*((uint8_t *)mailbox_msg->param));
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_VOC_SET_SPK_GAIN:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_set_spk_gain(*((uint16_t *)mailbox_msg->param));
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_VOC_SET_AEC_PARA:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_set_aec_para((aud_intf_voc_aec_ctl_t *)mailbox_msg->param);
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_VOC_GET_AEC_PARA:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_get_aec_para();
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_VOC_TX_DEBUG:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_voc_tx_debug((aud_intf_dump_data_callback)mailbox_msg->param);
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_VOC_RX_DEBUG:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_voc_rx_debug((aud_intf_dump_data_callback)mailbox_msg->param);
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_VOC_AEC_DEBUG:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_voc_aec_debug((aud_intf_dump_data_callback)mailbox_msg->param);
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				/* uac op */
				case AUD_TRAS_DRV_UAC_REGIS_CONT_STATE_CB:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					ret = aud_tras_drv_uac_register_connect_state_cb((void *)mailbox_msg->param);
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				case AUD_TRAS_DRV_UAC_CONT:
					aud_tras_uac_connect_handle();
					break;

				case AUD_TRAS_DRV_UAC_DISCONT:
					aud_tras_uac_disconnect_handle();
					break;

				case AUD_TRAS_DRV_UAC_AUTO_CONT_CTRL:
					mailbox_msg = (media_mailbox_msg_t *)msg.param;
					aud_tras_uac_auto_connect_ctrl((bool)mailbox_msg->param);
					msg_send_rsp_to_media_major_mailbox(mailbox_msg, ret, APP_MODULE);
					break;

				/* voc int op */
				case AUD_TRAS_DRV_AEC:
					if (aud_tras_drv_info.voc_info.status == AUD_TRAS_DRV_VOC_STA_START) {
						aud_tras_aec();
					}
					break;

				case AUD_TRAS_DRV_ENCODER:
					if (aud_tras_drv_info.voc_info.status == AUD_TRAS_DRV_VOC_STA_START) {
						aud_tras_enc();
					}
					break;

				case AUD_TRAS_DRV_DECODER:
					if (aud_tras_drv_info.voc_info.status == AUD_TRAS_DRV_VOC_STA_START) {
						aud_tras_dec();
					}
					break;

				default:
					break;
			}
			//GPIO_DOWN(5);
		}
	}

aud_tras_drv_exit:
	/* deinit mic, speaker and voice */
	/* check audio transfer driver work status */
	switch (aud_tras_drv_info.work_mode) {
		case AUD_INTF_WORK_MODE_GENERAL:
			/* check mic work status */
			if (aud_tras_drv_info.mic_info.status != AUD_TRAS_DRV_MIC_STA_NULL) {
				/* stop mic and deinit */
				aud_tras_drv_mic_stop();
				aud_tras_drv_mic_deinit();
			}
			/* check speaker work status */
			if (aud_tras_drv_info.spk_info.status != AUD_TRAS_DRV_SPK_STA_NULL) {
				/* stop speaker and deinit */
				aud_tras_drv_spk_stop();
				aud_tras_drv_spk_deinit();
			}
			break;

		case AUD_INTF_WORK_MODE_VOICE:
			/* check voice work status */
			if (aud_tras_drv_info.voc_info.status != AUD_TRAS_DRV_VOC_STA_NULL) {
				/* stop voice transfer and deinit */
				aud_tras_drv_voc_stop();
				aud_tras_drv_voc_deinit();
			}
			break;

		default:
			break;
	}

	aud_tras_drv_info.work_mode = AUD_INTF_WORK_MODE_NULL;
	aud_tras_drv_info.aud_tras_tx_mic_data = NULL;
	aud_tras_drv_info.aud_tras_rx_spk_data = NULL;

	/* set work status to NULL */
	aud_tras_drv_info.status = AUD_TRAS_DRV_STA_NULL;


	/* delete msg queue */
	ret = rtos_deinit_queue(&aud_trs_drv_int_msg_que);
	if (ret != kNoErr) {
		LOGE("%s, %d, delete message queue fail \n", __func__, __LINE__);
	}
	aud_trs_drv_int_msg_que = NULL;
	LOGI("%s, %d, delete message queue complete \n", __func__, __LINE__);

#if AUD_MEDIA_SEM_ENABLE
	/* deinit semaphore used to  */
	if (mailbox_media_aud_mic_sem) {
		rtos_deinit_semaphore(&mailbox_media_aud_mic_sem);
		mailbox_media_aud_mic_sem = NULL;
	}
#endif
//	bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_DEFAULT);

	/* reset uac to default */
	aud_tras_drv_info.aud_tras_drv_uac_connect_state_cb = NULL;
	aud_tras_drv_info.uac_status = AUD_INTF_UAC_NORMAL_DISCONNECTED;
	aud_tras_drv_info.uac_auto_connect = true;

	rtos_set_semaphore(&aud_tras_drv_task_sem);

	/* delete task */
	aud_trs_drv_thread_hdl = NULL;
	rtos_delete_thread(NULL);
}

aud_intf_drv_config_t aud_trs_drv_setup_bak = {0};

bk_err_t aud_tras_drv_init(aud_intf_drv_config_t *setup_cfg)
{
	bk_err_t ret = BK_OK;

#if AUD_MEDIA_SEM_ENABLE
	/* init semaphore used to  */
	if (!mailbox_media_aud_mic_sem) {
		ret = rtos_init_semaphore(&mailbox_media_aud_mic_sem, 1);
		if (ret != BK_OK)
		{
			LOGE("%s, %d, create mailbox audio mic semaphore failed \n", __func__, __LINE__);
			goto fail;
		}
	}
#endif

	if (aud_tras_drv_task_sem == NULL) {
		ret = rtos_init_semaphore(&aud_tras_drv_task_sem, 1);
		if (ret != BK_OK)
		{
			LOGE("%s, %d, create audio tras drv task semaphore failed \n", __func__, __LINE__);
			goto fail;
		}
	}

	if ((!aud_trs_drv_thread_hdl) && (!aud_trs_drv_int_msg_que)) {
		LOGD("%s, %d, init audio transfer driver \n", __func__, __LINE__);
		os_memcpy(&aud_trs_drv_setup_bak, setup_cfg, sizeof(aud_intf_drv_config_t));

		ret = rtos_init_queue(&aud_trs_drv_int_msg_que,
							  "aud_tras_int_que",
							  sizeof(aud_tras_drv_msg_t),
							  TU_QITEM_COUNT);
		if (ret != kNoErr) {
			LOGE("%s, %d, create audio transfer driver internal message queue fail \n", __func__, __LINE__);
			goto fail;
		}
		LOGD("%s, %d, create audio transfer driver internal message queue complete \n", __func__, __LINE__);

		//create audio transfer driver task
		ret = rtos_create_thread(&aud_trs_drv_thread_hdl,
							 setup_cfg->setup.task_config.priority,
							 "aud_tras_drv",
							 (beken_thread_function_t)aud_tras_drv_main,
							 4096,
							 (beken_thread_arg_t)&aud_trs_drv_setup_bak);
		if (ret != kNoErr) {
			LOGE("%s, %d, create audio transfer driver task fail \n", __func__, __LINE__);
			rtos_deinit_queue(&aud_trs_drv_int_msg_que);
			aud_trs_drv_int_msg_que = NULL;
			aud_trs_drv_thread_hdl = NULL;
			goto fail;
		}

		ret = rtos_get_semaphore(&aud_tras_drv_task_sem, BEKEN_WAIT_FOREVER);
		if (ret != BK_OK)
		{
			LOGE("%s, %d, rtos_get_semaphore\n", __func__, __LINE__);
			goto fail;
		}

		LOGD("%s, %d, create audio transfer driver task complete \n", __func__, __LINE__);
	}

	return BK_OK;

fail:
	//LOGE("%s, %d, aud_tras_drv_init fail, ret: %d \n", __func__, __LINE__, ret);

	if(aud_tras_drv_task_sem)
	{
		rtos_deinit_semaphore(&aud_tras_drv_task_sem);
		aud_tras_drv_task_sem = NULL;
	}

	return BK_FAIL;
}

bk_err_t aud_tras_drv_deinit(void)
{
	bk_err_t ret;
	aud_tras_drv_msg_t msg;

	msg.op = AUD_TRAS_DRV_EXIT;
	if (aud_trs_drv_int_msg_que) {
		ret = rtos_push_to_queue_front(&aud_trs_drv_int_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			LOGE("%s, %d, audio send msg: AUD_TRAS_DRV_EXIT fail \n", __func__, __LINE__);
			return kOverrunErr;
		}

		ret = rtos_get_semaphore(&aud_tras_drv_task_sem, BEKEN_WAIT_FOREVER);
		if (ret != BK_OK)
		{
			LOGE("%s, %d, rtos_get_semaphore\n", __func__, __LINE__);
			return BK_FAIL;
		}

		if(aud_tras_drv_task_sem)
		{
			rtos_deinit_semaphore(&aud_tras_drv_task_sem);
			aud_tras_drv_task_sem = NULL;
		}
	} else {
		LOGW("%s, %d, aud_trs_drv_int_msg_que is NULL\n", __func__, __LINE__);
	}

	return BK_OK;
}


bk_err_t audio_event_handle(media_mailbox_msg_t * msg)
{
	bk_err_t ret = BK_OK;		 // Modified by TUYA for bigcore

	/* save mailbox msg received from media app */
	LOGD("%s, %d, event: %d \n", __func__, __LINE__, msg->event);

	switch (msg->event)
	{
		case EVENT_AUD_INIT_REQ:
			ret = aud_tras_drv_init((aud_intf_drv_config_t *)msg->param);
			msg_send_rsp_to_media_major_mailbox(msg, ret, APP_MODULE);
			break;

		case EVENT_AUD_DEINIT_REQ:
			ret = aud_tras_drv_deinit();
			msg_send_rsp_to_media_major_mailbox(msg, ret, APP_MODULE);
			break;

		case EVENT_AUD_SET_MODE_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_SET_MODE, (void *)msg);
			break;

		/* mic mode event */
		case EVENT_AUD_MIC_INIT_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_MIC_INIT, (void *)msg);
			break;

		case EVENT_AUD_MIC_DEINIT_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_MIC_DEINIT, (void *)msg);
			break;

		case EVENT_AUD_MIC_START_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_MIC_START, (void *)msg);
			break;

		case EVENT_AUD_MIC_PAUSE_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_MIC_PAUSE, (void *)msg);
			break;

		case EVENT_AUD_MIC_STOP_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_MIC_STOP, (void *)msg);
			break;

		case EVENT_AUD_MIC_SET_CHL_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_MIC_SET_CHL, (void *)msg);
			break;

		case EVENT_AUD_MIC_SET_SAMP_RATE_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_MIC_SET_SAMP_RATE, (void *)msg);
			break;

		case EVENT_AUD_MIC_SET_GAIN_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_MIC_SET_GAIN, (void *)msg);
			break;

		/* spk event */
		case EVENT_AUD_SPK_INIT_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_SPK_INIT, (void *)msg);
			break;

		case EVENT_AUD_SPK_DEINIT_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_SPK_DEINIT, (void *)msg);
			break;

		case EVENT_AUD_SPK_START_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_SPK_START, (void *)msg);
			break;

		case EVENT_AUD_SPK_PAUSE_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_SPK_PAUSE, (void *)msg);
			break;

		case EVENT_AUD_SPK_STOP_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_SPK_STOP, (void *)msg);
			break;

		case EVENT_AUD_SPK_SET_CHL_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_SPK_SET_CHL, (void *)msg);
			break;

		case EVENT_AUD_SPK_SET_SAMP_RATE_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_SPK_SET_SAMP_RATE, (void *)msg);
			break;

		case EVENT_AUD_SPK_SET_GAIN_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_SPK_SET_GAIN, (void *)msg);
			break;

		/* voc op */
		case EVENT_AUD_VOC_INIT_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_VOC_INIT, (void *)msg);
			break;

		case EVENT_AUD_VOC_DEINIT_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_VOC_DEINIT, (void *)msg);
			break;

		case EVENT_AUD_VOC_START_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_VOC_START, (void *)msg);
			break;

		case EVENT_AUD_VOC_STOP_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_VOC_STOP, (void *)msg);
			break;

		case EVENT_AUD_VOC_CTRL_MIC_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_VOC_CTRL_MIC, (void *)msg);
			break;

		case EVENT_AUD_VOC_CTRL_SPK_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_VOC_CTRL_SPK, (void *)msg);
			break;

		case EVENT_AUD_VOC_CTRL_AEC_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_VOC_CTRL_AEC, (void *)msg);
			break;

		case EVENT_AUD_VOC_SET_MIC_GAIN_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_VOC_SET_MIC_GAIN, (void *)msg);
			break;

		case EVENT_AUD_VOC_SET_SPK_GAIN_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_VOC_SET_SPK_GAIN, (void *)msg);
			break;

		case EVENT_AUD_VOC_SET_AEC_PARA_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_VOC_SET_AEC_PARA, (void *)msg);
			break;

		case EVENT_AUD_VOC_GET_AEC_PARA_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_VOC_GET_AEC_PARA, (void *)msg);
			break;

		case EVENT_AUD_VOC_TX_DEBUG_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_VOC_TX_DEBUG, (void *)msg);
			break;

		case EVENT_AUD_VOC_RX_DEBUG_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_VOC_RX_DEBUG, (void *)msg);
			break;

		case EVENT_AUD_VOC_AEC_DEBUG_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_VOC_AEC_DEBUG, (void *)msg);
			break;

		/* uac event */
		case EVENT_AUD_UAC_REGIS_CONT_STATE_CB_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_UAC_REGIS_CONT_STATE_CB, (void *)msg);
			break;
#if 0
		case EVENT_AUD_UAC_CONT_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_UAC_CONT, (void *)msg->param);
			break;

		case EVENT_AUD_UAC_DISCONT_REQ:
			//aud_tras_drv_send_msg(AUD_TRAS_DRV_UAC_CONT, NULL);
			break;
#endif
		case EVENT_AUD_UAC_AUTO_CONT_CTRL_REQ:
			aud_tras_drv_send_msg(AUD_TRAS_DRV_UAC_AUTO_CONT_CTRL, (void *)msg);
			break;

		case EVENT_AUD_MIC_DATA_NOTIFY:
			//GPIO_UP(6);
#if AUD_MEDIA_SEM_ENABLE
			ret = rtos_set_semaphore(&msg->sem);
			if (ret != BK_OK)
			{
				LOGE("%s semaphore set failed: %d\n", __func__, ret);
			}
#endif
			//GPIO_DOWN(6);
			break;

		case EVENT_AUD_SPK_DATA_NOTIFY:
			//TODO set sem
			//	;
			break;

		default:
			break;
	}

	return ret;
}



