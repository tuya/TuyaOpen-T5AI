/*
 * tkl_video_enc.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "media_app.h"
#include "media_evt.h"
#include "tkl_video_enc.h"

extern void tuya_multimedia_power_on(void);
static TKL_VENC_PUT_CB upper_trans_cb = NULL;

static void __frame_test_cb(frame_buffer_t *frame)
{
    if (upper_trans_cb != NULL) {
        TKL_VENC_FRAME_T out_frame;
        out_frame.pbuf      = frame->frame;
        out_frame.buf_size  = frame->length;
        out_frame.width     = frame->width;
        out_frame.height    = frame->height;
        out_frame.seq       = frame->sequence;
        out_frame.timestamp = frame->timestamp;
        out_frame.codectype = TKL_CODEC_VIDEO_H264;
        if (frame->h264_type & (0x1 << H264_NAL_I_FRAME))
            out_frame.frametype = TKL_VIDEO_I_FRAME;
        else
            out_frame.frametype = TKL_VIDEO_PB_FRAME;

        //bk_printf("out: %d %d\r\n", out_frame.seq, frame->h264_type);
        upper_trans_cb(&out_frame);
    }
    return;
}


/**
* @brief video encode init
*
* @param[in] vi_chn: vi channel number
* @param[in] pconfig: venc config
* @param[in] count: count of pconfig
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_venc_init(int32_t vi_chn, TKL_VENC_CONFIG_T *pconfig, int32_t count)
{
    bk_wifi_set_wifi_media_mode(true);
    bk_wifi_set_video_quality(0);

    // tuya_multimedia_power_on();
    if (pconfig->enable_h264_pipeline) {
        media_app_h264_pipeline_open();
    }

    if ((pconfig != NULL) && (pconfig->put_cb != NULL)) {
        upper_trans_cb = pconfig->put_cb;
    }

    transfer_app_task_init(__frame_test_cb);
    media_send_msg_sync(EVENT_TRANSFER_OPEN_IND, PIXEL_FMT_H264);

    return OPRT_OK;
}

/**
* @brief video encode get frame
*
* @param[in] vi_chn: vi channel number
* @param[in] venc_chn: venc channel number
* @param[out] pframe:  output frame
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_venc_get_frame(TKL_VI_CHN_E vi_chn, TKL_VENC_CHN_E venc_chn, TKL_VENC_FRAME_T *pframe)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief video first snap
*
* @param[in] vi_chn: vi channel number
* @param[out] pframe: output frame
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_venc_get_first_snap(TKL_VI_CHN_E vi_chn, TKL_VENC_FRAME_T *pframe)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief video encode set osd
*
* @param[in] vi_chn: vi channel number
* @param[in] venc_chn: venc channel number
* @param[out] posd:  osd config
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_venc_set_osd(TKL_VI_CHN_E vi_chn, TKL_VENC_CHN_E venc_chn, TKL_VENC_OSD_T *posd)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief video encode set osd
*
* @param[in] vi_chn: vi channel number
* @param[in] venc_chn: venc channel number
* @param[in] idr_type: request idr type
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_venc_set_IDR(TKL_VI_CHN_E vi_chn, TKL_VENC_CHN_E venc_chn,  TKL_VENC_IDR_E idr_type)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief video encode set mask
*
* @param[in] vi_chn: vi channel number
* @param[in] venc_chn: venc channel number
* @param[out] pmask: mask config
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_venc_set_mask(TKL_VI_CHN_E vi_chn, TKL_VENC_CHN_E venc_chn, TKL_VENC_MASK_T *pmask)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief video settings format
*
* @param[in] vi_chn: vi channel number
* @param[in] venc_chn: venc channel number
* @param[in] pformat: format config
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_venc_set_format(TKL_VI_CHN_E vi_chn, TKL_VENC_CHN_E venc_chn, TKL_VENC_FORMAT_T *pformat)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief video get format
*
* @param[in] vi_chn: vi channel number
* @param[in] venc_chn: venc channel number
* @param[out] pformat: format config
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_venc_get_format(TKL_VI_CHN_E vi_chn, TKL_VENC_CHN_E venc_chn, TKL_VENC_FORMAT_T *pformat)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief video encode stream buff pool set
*
* @param[in] vi_chn: vi channel number
* @param[in] venc_chn: venc channel number
* @param[in] parg:  buff pool config
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_venc_set_video_stream_buffer(TKL_VI_CHN_E vi_chn, TKL_VENC_CHN_E venc_chn, TKL_VENC_STREAM_BUFF_T *parg)
{
    return OPRT_NOT_SUPPORTED;
}


/**
 * @brief video time callback
 *        Used to set osd time
 * @param[in] cb: time callback api
 *
 * @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
 */
OPERATE_RET tkl_venc_set_time_cb(TKL_VENC_TIME_CB cb)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief video encode  start
*
* @param[in] vi_chn: vi channel number
* @param[in] venc_chn: venc channel number
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_venc_start(TKL_VI_CHN_E vi_chn, TKL_VENC_CHN_E venc_chn)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief video encode  stop
*
* @param[in] vi_chn: vi channel number
* @param[in] venc_chn: venc channel number
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_venc_stop( TKL_VI_CHN_E vi_chn, TKL_VENC_CHN_E venc_chn)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief video encode uninit
*
* @param[in] vi_chn: vi channel number
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_venc_uninit(TKL_VI_CHN_E vi_chn, TKL_VENC_CONFIG_T *pconfig)
{
    bk_wifi_set_video_quality(2);
    bk_wifi_set_wifi_media_mode(false);
    if (pconfig->enable_h264_pipeline) {
        media_app_h264_pipeline_close();
    }
    return OPRT_OK;
}




