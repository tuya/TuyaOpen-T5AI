/*
 * tkl_audio.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */
#include "aud_intf.h"
#include "tkl_audio.h"
#include "tkl_semaphore.h"
#include "tkl_system.h"
#include "tkl_gpio.h"

// 驱动内部使用的是4
#define DRIVER_SPEAK_FIFO_FRAME_NUM 4


extern void tuya_multimedia_power_on(void);
static aud_intf_drv_setup_t aud_intf_drv_setup = DEFAULT_AUD_INTF_DRV_SETUP_CONFIG();
static aud_intf_voc_setup_t aud_voc_setup = DEFAULT_AUD_INTF_VOC_SETUP_CONFIG();
static aud_intf_mic_setup_t aud_intf_mic_setup = DEFAULT_AUD_INTF_MIC_SETUP_CONFIG();
static aud_intf_spk_setup_t aud_intf_spk_setup = DEFAULT_AUD_INTF_SPK_SETUP_CONFIG();
static aud_intf_work_mode_t aud_work_mode = AUD_INTF_WORK_MODE_NULL;

static TKL_AUDIO_TYPE_E current_card = TKL_AUDIO_TYPE_UAC;
static int board_spk_gpio;
static int board_spk_gpio_polarity;

TKL_FRAME_PUT_CB user_mic_cb = NULL;
TKL_FRAME_SPK_CB user_spk_cb = NULL;

extern void* tkl_system_psram_malloc(size_t size);
extern void tkl_system_psram_free(void* ptr);
extern bk_err_t media_app_get_usb_connect_status(void);

typedef struct {
    bool mic_init;
    bool spk_init;
    bool mic_start;
    bool spk_start;
}AUIDO_INIT_T;

static AUIDO_INIT_T s_audio_init = {0,0};
static TKL_SEM_HANDLE s_spk_sem = NULL;

static int __tkl_audio_mic_trans_callback(unsigned char *data, unsigned int len)
{
    if (user_mic_cb != NULL) {
        TKL_AUDIO_FRAME_INFO_T frame;
        frame.type = TKL_AUDIO_FRAME;
        frame.channel = TKL_AUDIO_CHANNEL_MONO;
        // frame.datebits = 16;
        if  (current_card == TKL_AUDIO_TYPE_UAC) {
            frame.sample = aud_voc_setup.samp_rate;
        }
        else {
            frame.sample = aud_intf_mic_setup.samp_rate;
        }

        frame.pbuf = (CHAR_T*)data;
        frame.buf_size = len;
        frame.used_size = len;
        frame.timestamp = -1;
        frame.pts = -1;
        user_mic_cb(&frame);
    }

    return len;
}

static int __tkl_audio_spk_trans_callback(unsigned int size)
{
    if (user_spk_cb) {
        user_spk_cb((void *)size);
    }
}

static void __audio_connect_state_cb_handle(uint8_t state)
{
	os_printf("[--%s--] state: %d \n", __func__, state);
}

/**
* @brief ai init
*
* @param[in] pconfig: audio config
* @param[in] count: count of pconfig
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_init(TKL_AUDIO_CONFIG_T *pconfig, int count)
{
    int ret;

    if (pconfig == NULL)
        return OPRT_INVALID_PARM;

    aud_voc_setup.aec_enable = 1;
    aud_voc_setup.mic2_disable = !pconfig->enable;
    if (pconfig->card == TKL_AUDIO_TYPE_UAC) {
        aud_voc_setup.data_type = AUD_INTF_VOC_DATA_TYPE_PCM;
        aud_voc_setup.spk_mode = AUD_DAC_WORK_MODE_SIGNAL_END;

        aud_voc_setup.mic_type = AUD_INTF_MIC_TYPE_UAC;
        aud_voc_setup.spk_type = AUD_INTF_SPK_TYPE_UAC;

    } else{
        aud_voc_setup.data_type = AUD_INTF_VOC_DATA_TYPE_PCM;
        aud_voc_setup.mic_mode = AUD_ADC_MODE_DIFFEN;
        aud_voc_setup.spk_mode = AUD_DAC_WORK_MODE_DIFFEN;
        aud_voc_setup.mic_gain = 0x2f;
        aud_voc_setup.spk_gain = 0x2f;
        aud_voc_setup.mic_type = AUD_INTF_MIC_TYPE_BOARD;
        aud_voc_setup.spk_type = AUD_INTF_SPK_TYPE_BOARD;
    }

    if (pconfig->enable) {
        if ((pconfig->sample != 8000) && (pconfig->sample != 16000)) {
            bk_printf("voice only support 8k/16k sample, set value %d\r\n", pconfig->sample);
            return -1;
        }
    }

    aud_voc_setup.samp_rate = pconfig->sample;
    if ((pconfig->spk_sample == 0) || (pconfig->enable)) {
        // enable置1,表示语音对讲，此时因AEC需要，mic跟spk的
        // 采样率需要相等
        aud_voc_setup.spk_samp_rate = pconfig->sample;
    } else {
        aud_voc_setup.spk_samp_rate = pconfig->spk_sample;
    }

    aud_intf_drv_setup.aud_intf_tx_mic_data = __tkl_audio_mic_trans_callback;
    // aud_intf_drv_setup.aud_intf_rx_spk_data = __tkl_audio_spk_trans_callback;

    tkl_semaphore_create_init(&s_spk_sem,0,8);

    ret = bk_aud_intf_drv_init(&aud_intf_drv_setup);
    if (ret != BK_OK) {
        bk_printf("bk_aud_intf_drv_init fail, ret:%d\n", ret);
        goto error;
    }

    aud_work_mode = AUD_INTF_WORK_MODE_VOICE;
    ret = bk_aud_intf_set_mode(aud_work_mode);
    if (ret != BK_OK) {
        bk_printf("bk_aud_intf_set_mode fail, ret:%d\n", ret);
        goto error;
    }

    if (pconfig->card == TKL_AUDIO_TYPE_UAC) {
        /* uac recover connection */
        if (aud_voc_setup.mic_type == AUD_INTF_MIC_TYPE_UAC) {
            ret = bk_aud_intf_register_uac_connect_state_cb(__audio_connect_state_cb_handle);
            if (ret != BK_OK) {
                bk_printf("bk_aud_intf_register_uac_connect_state_cb fail, ret:%d\n", ret);
                goto error;
            }

            ret = bk_aud_intf_uac_auto_connect_ctrl(true);
            if (ret != BK_OK) {
                bk_printf("aud_tras_uac_auto_connect_ctrl fail, ret:%d\n", ret);
                goto error;
            }
        }

        ret = bk_aud_intf_voc_init(aud_voc_setup);
        if (ret != BK_OK) {
            bk_printf("bk_aud_intf_voc_init fail, ret:%d\n", ret);
            goto error;
        }

        s_audio_init.mic_init = true;
        s_audio_init.spk_init = true;
        s_audio_init.mic_start = false;
        s_audio_init.spk_start = false;

    } else if (pconfig->card == TKL_AUDIO_TYPE_BOARD) {

        ret = bk_aud_intf_voc_init(aud_voc_setup);
        if (ret != BK_ERR_AUD_INTF_OK) {
            os_printf("bk_aud_intf_voc_init fail, ret:%d \r\n", ret);
        } else {
            os_printf("bk_aud_intf_voc_init complete \r\n");
            s_audio_init.mic_init = true;
            s_audio_init.spk_init = true;
            s_audio_init.mic_start = false;
            s_audio_init.spk_start = false;
        }
    }
    if (pconfig->spk_gpio < 56) {
        board_spk_gpio = pconfig->spk_gpio;
        board_spk_gpio_polarity = pconfig->spk_gpio_polarity;
    }

    // 如果外设是uvc，判断连接状态，
    // 此处未连接的情况下，初始化成功，但是deinit时候会返回失败
    if (pconfig->card == TKL_AUDIO_TYPE_UAC) {
        int camera_status = 0;
        int count = 0;
        do
        {
            camera_status = media_app_get_usb_connect_status();
            if (camera_status) {
                break;
            }
            tkl_system_sleep(50);
        } while (count++ < 10);
        if (!camera_status) {
            bk_printf("uvc status error: %d\r\n", camera_status);
            goto error;
        }
    }
	user_spk_cb = pconfig->spk_cb;
    if (pconfig->put_cb != NULL) {
        // delay 500ms, 不上报开始500ms的数据，初始化阶段声音可能存在失真杂音
        // tkl_system_sleep(500);
        user_mic_cb = pconfig->put_cb;
    }

    current_card = pconfig->card;
    return OPRT_OK;

error:

    bk_aud_intf_voc_deinit();

    current_card = TKL_AUDIO_TYPE_UAC;
    aud_work_mode = AUD_INTF_WORK_MODE_NULL;
    bk_aud_intf_set_mode(AUD_INTF_WORK_MODE_NULL);
    bk_aud_intf_drv_deinit();
    if (s_spk_sem) {
        tkl_semaphore_release(s_spk_sem);
        s_spk_sem = NULL;
    }
    return OPRT_COM_ERROR;
}

/**
* @brief ai start
*
* @param[in] card: card number
* @param[in] chn: channel number
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_start(int card, TKL_AI_CHN_E chn)
{
    int ret;
    if(aud_work_mode == AUD_INTF_WORK_MODE_NULL) {
        bk_printf("tkl_ai_start fail, not init ,aud_work_mode:%d\n", aud_work_mode);
        return OPRT_COM_ERROR;
    }

    ret = bk_aud_intf_voc_start();
    if (ret != BK_OK) {
        bk_printf("bk_aud_intf_voc_start fail, ret:%d\n", ret);
        return OPRT_COM_ERROR;
    }

    if (current_card == TKL_AUDIO_TYPE_BOARD) {
        TUYA_GPIO_BASE_CFG_T cfg;
        cfg.direct = TUYA_GPIO_OUTPUT;
        if (board_spk_gpio_polarity == 0) {
            cfg.mode = TUYA_GPIO_PULLUP;
            cfg.level = TUYA_GPIO_LEVEL_HIGH;
        }
        else if (board_spk_gpio_polarity == 1) {
            cfg.mode = TUYA_GPIO_PULLDOWN;
            cfg.level = TUYA_GPIO_LEVEL_LOW;
        }

        tkl_gpio_init(board_spk_gpio, &cfg);
    }

    s_audio_init.spk_start = true;
    s_audio_init.mic_start = true;

    return OPRT_OK;
}


/**
* @brief ai set mic volume
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[in] vol: mic volume,[0, 100]
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_set_vol(int card, TKL_AI_CHN_E chn, int vol)
{
    if (!s_audio_init.mic_init) {
        return OPRT_RESOURCE_NOT_READY;
    }

    uint32_t volume = 0;
    volume =(uint32_t)(vol * 0x3F / 100);
    return bk_aud_intf_set_mic_gain(volume);
}


/**
* @brief ai get frame
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[out] pframe: audio frame, pframe->pbuf allocated by upper layer application
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_get_frame(int card, TKL_AI_CHN_E chn, TKL_AUDIO_FRAME_INFO_T *pframe)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief ai set vqe param
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[in] type: vqe type
* @param[in] pparam: vqe param
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_set_vqe(int card, TKL_AI_CHN_E chn, TKL_AUDIO_VQE_TYPE_E type, TKL_AUDIO_VQE_PARAM_T *pparam)
{
    OPERATE_RET ret = OPRT_NOT_SUPPORTED;
    if (card == TKL_AUDIO_TYPE_UAC && type == TKL_AUDIO_VQE_AEC) {
        ret = bk_aud_intf_voc_aec_ctrl(pparam->enable);
    }

    return ret;
}


/**
* @brief ai get vqe param
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[in] type: vqe type
* @param[out] pparam: vqe param
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_get_vqe(int card, TKL_AI_CHN_E chn, TKL_AUDIO_VQE_TYPE_E type, TKL_AUDIO_VQE_PARAM_T *pparam)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief ai stop
*
* @param[in] card: card number
* @param[in] chn: channel number
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_stop(int card, TKL_AI_CHN_E chn)
{
    int ret;
    if(aud_work_mode == AUD_INTF_WORK_MODE_NULL) {
        bk_printf("bk_aud_intf_voc_stop fail, not init ,aud_work_mode:%d\n", aud_work_mode);
        return OPRT_COM_ERROR;
    }

    if (current_card == TKL_AUDIO_TYPE_UAC) {
        if  (!s_audio_init.mic_start) {
            return OPRT_OK;
        }
        ret = bk_aud_intf_voc_stop();
        if (ret != BK_OK) {
            bk_printf("bk_aud_intf_voc_stop fail, ret:%d\n", ret);
            return OPRT_COM_ERROR;
        }
        s_audio_init.mic_start = false;
        s_audio_init.spk_start = false;
    } else if (current_card == TKL_AUDIO_TYPE_BOARD) {
        if (board_spk_gpio_polarity == 0)
            tkl_gpio_write(board_spk_gpio, 0);
        else
            tkl_gpio_write(board_spk_gpio, 1);

        // ret = bk_aud_intf_mic_deinit();
        ret = bk_aud_intf_voc_stop();
        if (ret != BK_OK) {
            bk_printf("bk_aud_intf_mic_stop fail, ret:%d\n", ret);
            return OPRT_COM_ERROR;
        }
        s_audio_init.mic_start = false;
        s_audio_init.spk_start = false;
        return ret;
    }
    return OPRT_OK;
}


/**
* @brief ai uninit
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_uninit(void)
{
    int ret = OPRT_OK;

    if (!s_audio_init.mic_init) {
        return OPRT_OK;
    }

    ret = bk_aud_intf_voc_deinit();
    if (ret == BK_OK) {
        s_audio_init.mic_init = false;
        s_audio_init.spk_init = false;
    }

    aud_work_mode = AUD_INTF_WORK_MODE_NULL;
    bk_aud_intf_set_mode(aud_work_mode);
    bk_aud_intf_drv_deinit();
    // current_card = TKL_AUDIO_TYPE_UAC;
    if (s_spk_sem) {
        tkl_semaphore_release(s_spk_sem);
        s_spk_sem = NULL;
    }
    s_audio_init.mic_start = false;
    s_audio_init.spk_start = false;

    bk_printf("------ [%s %d]\r\n", __func__, __LINE__);
    return ret;
}

/**
* @param[in] count: config count
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ao_init(TKL_AUDIO_CONFIG_T *pconfig, int count, void **handle)
{
    int ret = OPRT_NOT_SUPPORTED;
    if (s_audio_init.spk_init) {
        *handle = (void *)1;
        return OPRT_OK;
    }

    return ret;
}


/**
* @brief ao start
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[out] handle: handle of start
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ao_start(int card, TKL_AO_CHN_E chn, void *handle)
{
    int ret = OPRT_RESOURCE_NOT_READY;
    if (s_audio_init.spk_start) {
        return OPRT_OK;
    }

    return ret;
}


/**
* @brief ao set volume
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[in] vol: mic volume,[0, 100]
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ao_set_vol(int card, TKL_AO_CHN_E chn, void *handle, int vol)
{
    int ret = OPRT_OK;
    if (!s_audio_init.spk_init) {
        return OPRT_RESOURCE_NOT_READY;
    }
    uint32_t volume = 0;
    if (card == TKL_AUDIO_TYPE_BOARD) {
        volume =(uint32_t)(vol * 0x3F / 100) ;
    }
    else {
        volume = (vol);
    }

    ret = bk_aud_intf_set_spk_gain(volume);
    aud_intf_spk_setup.spk_gain = volume;
    return ret;
}


/**
* @brief ao get volume
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[in] vol: mic volume,[0, 100]
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ao_get_vol(int card, TKL_AO_CHN_E chn, void *handle, int *vol)
{
    if (!s_audio_init.spk_init) {
        return OPRT_RESOURCE_NOT_READY;
    }
    if (card == TKL_AUDIO_TYPE_BOARD) {
        *vol = (aud_intf_spk_setup.spk_gain * 100) / 0x3F;
    }
    else {
        *vol = aud_intf_spk_setup.spk_gain;
    }
    return OPRT_OK;
}


/**
* @brief ao output frame
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[in] handle: handle of start
* @param[in] pframe: output frame
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ao_put_frame(int card, TKL_AO_CHN_E chn, void *handle, TKL_AUDIO_FRAME_INFO_T *pframe)
{
    OPERATE_RET ret;
    // buf_ring_put_data(pframe->pbuf, pframe->buf_size, pframe->codectype);
    if (pframe == NULL) {
        return OPRT_INVALID_PARM;
    }
    if (!s_audio_init.spk_init || !s_audio_init.spk_start) {
        return OPRT_RESOURCE_NOT_READY;
    }
    int spk_ringbuf_size = aud_intf_spk_setup.frame_size * DRIVER_SPEAK_FIFO_FRAME_NUM;
    int offset = 0; // 偏移量
    int remaining_size = pframe->used_size; // 剩余数据大小

    // 分段调用 bk_aud_intf_write_spk_data
    while (remaining_size > 0) {
        // 计算当前要写入的数据大小
        int chunk_size = (remaining_size > spk_ringbuf_size) ? spk_ringbuf_size : remaining_size;

write_spk_retry:
        // 调用写数据函数
        ret = bk_aud_intf_write_spk_data((uint8_t*)(pframe->pbuf + offset), chunk_size);
        // ret = tkl_semaphore_wait(s_spk_sem, 1000);
        if (ret == BK_ERR_BUSY) {
            tkl_system_sleep(10);
            goto write_spk_retry;
        }

        if (ret != OPRT_OK) {
            os_printf("audio intf spk semaphore wait fail, ret:%d \r\n", ret);
            return ret;
        }
        // 更新偏移量和剩余大小
        offset += chunk_size;
        remaining_size -= chunk_size;
    }

    return OPRT_OK;
}

/**
* @brief clear speaker buffer
*
* @param[in] card: card number
* @param[in] chn: channel number
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ao_clear_buffer(int card, TKL_AO_CHN_E chn)
{
    extern bk_err_t bk_aud_intf_clean_spk_data(void);
    bk_aud_intf_clean_spk_data();
    return OPRT_OK;
}


/**
* @brief ao stop
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[in] handle: handle of start
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ao_stop(int card, TKL_AO_CHN_E chn, void *handle)
{
    int ret = OPRT_NOT_SUPPORTED;
    if (!s_audio_init.spk_start) {
        return OPRT_OK;
    }

    if(aud_work_mode == AUD_INTF_WORK_MODE_NULL) {
        bk_printf("bk_aud_intf_voc_stop 0 fail, not init ,aud_work_mode:%d\n", aud_work_mode);
        return OPRT_COM_ERROR;
    }
    if (card != TKL_AUDIO_TYPE_BOARD) {
        if (s_audio_init.spk_start) {
            ret = bk_aud_intf_voc_stop();
            if (ret != BK_ERR_AUD_INTF_OK) {
                os_printf("audio intf spk stop fail, ret:%d \r\n", ret);
                return ret;
            }
            s_audio_init.spk_start = false;
        }
        ret = OPRT_OK;
    }
    return ret;
}


/**
* @brief ao uninit
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ao_uninit(void *handle)
{
    if  (!s_audio_init.spk_init) {
        return OPRT_OK;
    }
    int ret = OPRT_NOT_SUPPORTED;
    if (current_card == TKL_AUDIO_TYPE_BOARD) {
        // ret = bk_aud_intf_mic_deinit();
        ret  = bk_aud_intf_spk_deinit();
        if (ret != BK_ERR_AUD_INTF_OK) {
            os_printf("bk_aud_intf_spk_deinit fail, ret:%d \r\n", ret);
            return ret;
        }
    }
    if (s_spk_sem) {
        tkl_semaphore_release(s_spk_sem);
        s_spk_sem = NULL;
    }
    s_audio_init.spk_init = false;
    s_audio_init.spk_start = false;
    ret = OPRT_OK;
    return ret;
}


/**
* @brief audio input detect start
*
* @param[in] card: card number
* @param[in] type: detect type
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_detect_start(int card, TKL_MEDIA_DETECT_TYPE_E type)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief audio input detect stop
*
* @param[in] card: card number
* @param[in] type: detect type
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_detect_stop(int card, TKL_MEDIA_DETECT_TYPE_E type)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief audio detect get result
*
* @param[in] card: card number
* @param[in] type: detect type
* @param[out] presult: audio detect result
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_detect_get_result(int card, TKL_MEDIA_DETECT_TYPE_E type, TKL_AUDIO_DETECT_RESULT_T *presult)
{
    return OPRT_NOT_SUPPORTED;
}
