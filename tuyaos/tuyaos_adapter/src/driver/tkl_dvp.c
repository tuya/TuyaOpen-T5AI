#include "tkl_dvp.h"
#include "tkl_memory.h"
#include "tkl_output.h"
#include "tkl_semaphore.h"
#include "tkl_queue.h"
#include "tkl_thread.h"
#include "tkl_system.h"
#include "FreeRTOS.h"
#include "task.h"
#include <driver/int.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <driver/media_types.h>
#include <driver/yuv_buf.h>
#include <driver/h264.h>
#include <driver/video_common_driver.h>

#define HARDWARE_BLOCK_WDITH_BYTE   8
#define HARDWARE_BLOCK_LINE         8
#define YUV_PIXEL_SIZE_BYTE         2 //YUV422一个像素点占两个字节

#define BLOCK_WIDTH           8   // 像素处理块宽
#define BLOCK_HEIGHT          8   // 像素处理块高

#define clk_m(a) (a * 1000 * 1000)

#define YUV422_PER_PIXEL_BYTE   (2)

#define DVP_H264_SEI_SIZE       (96)
#define DVP_DMA_CACHE           (1024 * 10)
#define DVP_MSG_QUE_SIZE        (10)
#define DVP_TASK_EXIT_POST_RETRY (5)

typedef enum
{
    DVP_DRV_TURN_OFF = 0,
    DVP_DRV_TURNING_OFF,
    DVP_DRV_TURNING_ON,
    DVP_DRV_TURN_ON,
};

typedef enum {
    DVP_YUV_EOF = 0,
    DVP_JPEG_EOF,
    DVP_H264_EOF,
    DVP_TASK_EXIT,
};

typedef struct {
    uint32_t event;
    uint32_t param;
} TKL_DVP_MSG_T;

typedef struct
{
    uint32_t in_addr;
    uint32_t in_channel;
    uint32_t in_line_size;
    uint32_t in_offset;
    uint32_t out_addr;
    uint32_t out_channel;
    uint32_t out_offset;
#if 0
    uint8_t sei[H264_SELF_DEFINE_SEI_SIZE]; // save frame infomation
#endif
    uint8_t sequence;
    uint8_t is_i_frame_flag;
} ENCODER_MANAGE_T;

typedef struct
{
    TUYA_DVP_CFG_T dvp_cfg;
    yuv_mode_t cur_work_mode;
    uint8_t is_mix_mode;
    uint8_t *pingpong_buf;
    uint32_t pingpong_len;
    TUYA_DVP_FRAME_MANAGE_T *base_frame;
    TUYA_FRAME_FMT_E base_frame_fmt;
    uint32_t base_frame_len;
    TUYA_DVP_FRAME_MANAGE_T *encoded_frame;
    TUYA_FRAME_FMT_E encoded_frame_fmt;
    uint32_t encoded_frame_len;
    ENCODER_MANAGE_T encoder_manage;
    yuv_buf_config_t yuv_buf_module_config;
    jpeg_config_t jpeg_module_config;
    uint8_t drv_stat;
    uint8_t output_enable;
    uint32_t frame_id;
    uint8_t error_flag;
    mclk_freq_t bk_clk;
    TKL_QUEUE_HANDLE msg_queue;
    TKL_THREAD_HANDLE dvp_frame_task;
    TKL_SEM_HANDLE drv_close_sem;
    TKL_SEM_HANDLE task_close_sem;
} DVP_MODULE_MANAGE_T;

DVP_MODULE_MANAGE_T g_dvp_module_manage =
{
    .encoder_manage =
    {
        .in_channel = DMA_ID_MAX,
        .out_channel = DMA_ID_MAX,
    },

    .yuv_buf_module_config =
    {
        .mclk_div = YUV_MCLK_DIV_3,
        .yuv_mode_cfg =
        {
            .hsync = 1,
            .vsync = 1,
            .yuv_format = YUV_FORMAT_YUYV,
        },
    },
    .jpeg_module_config =
    {
        .hsync = 1,
        .vsync = 1,
        .mode = JPEG_MODE,
        .sensor_fmt = YUV_FORMAT_YUYV,
    },

    .pingpong_buf = NULL,
    .base_frame = NULL,
    .encoded_frame = NULL,
    .is_mix_mode = false,
    .bk_clk = MCLK_24M,
    .frame_id = 0,
    .drv_stat = DVP_DRV_TURN_OFF,
    .error_flag = false,
    .msg_queue = NULL,
    .dvp_frame_task = NULL,
    .drv_close_sem = NULL,
    .task_close_sem = NULL,
    .output_enable = false,
};

static OPERATE_RET __tkl_dvp_deinit(DVP_MODULE_MANAGE_T *dvp_mgmt, uint8_t skip_hw);

static OPERATE_RET __ty_output_mode_to_bk_work_mode(TUYA_CAMERA_OUTPUT_MODE output_mode, yuv_mode_t *bk_work_mode)
{
    switch (output_mode)
    {
        case TUYA_CAMERA_OUTPUT_YUV422:
            (*bk_work_mode) = YUV_MODE;
            break;
        case TUYA_CAMERA_OUTPUT_JPEG:
        case TUYA_CAMERA_OUTPUT_JPEG_YUV422_BOTH:
            (*bk_work_mode) = JPEG_MODE;
            break;
        case TUYA_CAMERA_OUTPUT_H264:
        case TUYA_CAMERA_OUTPUT_H264_YUV422_BOTH:
            (*bk_work_mode) = H264_MODE;
            break;
        default:
            bk_printf("%s this device dont support mode(%d)\r\n", __func__, output_mode);
            return OPRT_NOT_SUPPORTED;
    }

    return OPRT_OK;
}

static OPERATE_RET __dvp_config_param_check(DVP_MODULE_MANAGE_T *dvp_mgmt)
{
    OPERATE_RET ret = OPRT_OK;
    TUYA_DVP_CFG_T *dvp_cfg = &(dvp_mgmt->dvp_cfg);
    uint16_t width = dvp_cfg->width;
    uint16_t height = dvp_cfg->height;
    TUYA_CAMERA_OUTPUT_MODE output_mode = dvp_cfg->output_mode;

    ret = __ty_output_mode_to_bk_work_mode(output_mode, &(dvp_mgmt->cur_work_mode));
    if (ret)
        return OPRT_NOT_SUPPORTED;


    if (output_mode == TUYA_CAMERA_OUTPUT_YUV422
        || output_mode == TUYA_CAMERA_OUTPUT_JPEG_YUV422_BOTH
        || output_mode == TUYA_CAMERA_OUTPUT_H264_YUV422_BOTH)
    {
        dvp_mgmt->base_frame_fmt = TUYA_FRAME_FMT_YUV422;
        dvp_mgmt->base_frame_len = width * height * YUV422_PER_PIXEL_BYTE;

        dvp_mgmt->is_mix_mode = (output_mode != TUYA_CAMERA_OUTPUT_YUV422) ? true : false;
    }

    if (dvp_mgmt->cur_work_mode == H264_MODE)
    {
        dvp_mgmt->encoded_frame_fmt = TUYA_FRAME_FMT_H264;
        dvp_mgmt->encoded_frame_len = CONFIG_H264_FRAME_SIZE;
    }

    if (dvp_mgmt->cur_work_mode == JPEG_MODE)
    {
        dvp_mgmt->encoded_frame_fmt = TUYA_FRAME_FMT_JPEG;
        dvp_mgmt->encoded_frame_len = CONFIG_JPEG_FRAME_SIZE;
    }

    yuv_mode_cfg_t *yuv_buf_cfg = &(dvp_mgmt->yuv_buf_module_config.yuv_mode_cfg);
    jpeg_config_t *jpeg_cfg = &(dvp_mgmt->jpeg_module_config);
    switch (dvp_cfg->sync_polarity)
    {
    case TUYA_DVP_SYNC_MODE_0:
        yuv_buf_cfg->hsync = jpeg_cfg->hsync = 1;
        yuv_buf_cfg->vsync = jpeg_cfg->vsync = 1;
        break;
    case TUYA_DVP_SYNC_MODE_1:
        yuv_buf_cfg->hsync = jpeg_cfg->hsync = 1;
        yuv_buf_cfg->vsync = jpeg_cfg->vsync = 0;
        break;
    case TUYA_DVP_SYNC_MODE_2:
        yuv_buf_cfg->hsync = jpeg_cfg->hsync = 0;
        yuv_buf_cfg->vsync = jpeg_cfg->vsync = 1;
        break;
    case TUYA_DVP_SYNC_MODE_3:
        yuv_buf_cfg->hsync = jpeg_cfg->hsync = 0;
        yuv_buf_cfg->vsync = jpeg_cfg->vsync = 0;
        break;
    default:
        bk_printf("%s the sync mode %d is invalid\r\n", __func__, dvp_cfg->sync_polarity);
        return OPRT_NOT_SUPPORTED;
    }

    return OPRT_OK;
}

static OPERATE_RET __ty_clk_to_bk_clk(uint32_t clk, mclk_freq_t *outclk)
{
    switch (clk)
    {
        case 0:
            (*outclk) = 0;
            break;
        case clk_m(15):
            (*outclk) = MCLK_15M;
            break;
        case clk_m(16):
            (*outclk) = MCLK_16M;
            break;
        case clk_m(20):
            (*outclk) = MCLK_20M;
            break;
        case clk_m(24):
            (*outclk) = MCLK_24M;
            break;
        case clk_m(30):
            (*outclk) = MCLK_30M;
            break;
        case clk_m(32):
            (*outclk) = MCLK_32M;
            break;
        case clk_m(40):
            (*outclk) = MCLK_40M;
            break;
        case clk_m(48):
            (*outclk) = MCLK_48M;
            break;
        default:
            (*outclk) = MCLK_24M;
            break;
    }

    return OPRT_OK;
}

static OPERATE_RET __dvp_yuv_buf_module_init(DVP_MODULE_MANAGE_T *dvp_mgmt)
{
    // TODO: mclk_div/yuv_format/vsync/hsync从sensor配置获取
    TUYA_DVP_CFG_T *dvp_cfg = &(dvp_mgmt->dvp_cfg);
    yuv_buf_config_t yuv_buf_config_cur = {0};
    memcpy(&yuv_buf_config_cur, &(dvp_mgmt->yuv_buf_module_config), sizeof(yuv_buf_config_t));

    yuv_buf_config_cur.work_mode = dvp_mgmt->cur_work_mode;

    // 横向width的像素块个数
    yuv_buf_config_cur.x_pixel = dvp_cfg->width / BLOCK_WIDTH; // 除以BLOCK_WIDTH得到横向块数

    // 纵向像素块个数
    yuv_buf_config_cur.y_pixel = dvp_cfg->height / BLOCK_HEIGHT; // 除以BLOCK_HEIGHT得到纵向块数

    if (dvp_mgmt->cur_work_mode == YUV_MODE)
        goto init_yuv_buf;

    // 申请pingpong buf
    if (dvp_mgmt->pingpong_buf != NULL)
        goto init_yuv_buf;

    if (dvp_mgmt->cur_work_mode == H264_MODE)
    {
        // h264编码器每16行一输入，16 * pixel_per_size * pingpong
        dvp_mgmt->pingpong_len = dvp_cfg->width * 32 * 2;
        dvp_mgmt->pingpong_buf = (uint8_t *)os_malloc(dvp_mgmt->pingpong_len);
    }
    else if (dvp_mgmt->cur_work_mode == JPEG_MODE)
    {
        // jepg编码器每8行一输入，8 * pixel_per_size * pingpong
        dvp_mgmt->pingpong_len = dvp_cfg->width * 16 * 2;
        dvp_mgmt->pingpong_buf = (uint8_t *)os_malloc(dvp_mgmt->pingpong_len);
    }

    if (dvp_mgmt->pingpong_buf == NULL)
    {
        bk_printf("%s malloc pingpong buf failed\r\n", __func__);
        return OPRT_MALLOC_FAILED;
    }

init_yuv_buf:
    yuv_buf_config_cur.base_addr = (dvp_mgmt->pingpong_buf == NULL) ? NULL : dvp_mgmt->pingpong_buf;

    uint8_t ret = bk_yuv_buf_init(&yuv_buf_config_cur);
	if (ret != BK_OK)
	{
		bk_printf("yuv_buf yuv mode init error\n");
		return OPRT_COM_ERROR;
	}

    return OPRT_OK;
}

static OPERATE_RET __dvp_jpeg_module_init(DVP_MODULE_MANAGE_T *dvp_mgmt)
{
    jpeg_config_t jpeg_config_cur = {0};
    memcpy(&jpeg_config_cur, &(dvp_mgmt->jpeg_module_config), sizeof(jpeg_config_t));

    TUYA_DVP_CFG_T *dvp_cfg = &(dvp_mgmt->dvp_cfg);
    jpeg_config_cur.x_pixel = dvp_cfg->width / BLOCK_WIDTH;
    jpeg_config_cur.y_pixel = dvp_cfg->height / BLOCK_WIDTH;

    jpeg_config_cur.clk = dvp_mgmt->bk_clk;
    jpeg_config_cur.mode = JPEG_MODE;

    uint8_t ret = bk_jpeg_enc_init(&jpeg_config_cur);
    if (ret != BK_OK)
    {
        bk_printf("jpeg init error\n");
        return OPRT_MALLOC_FAILED;
    }

    return OPRT_OK;
}

static void __dvp_dma_finish_cb(dma_id_t id)
{
    g_dvp_module_manage.encoder_manage.out_offset += DVP_DMA_CACHE;
}

static OPERATE_RET __dvp_encoder_output_dma_config(DVP_MODULE_MANAGE_T *dvp_mgmt)
{
    TUYA_DVP_CFG_T *dvp_cfg = &(dvp_mgmt->dvp_cfg);

    if (dvp_cfg->inter_cfg.assign_cb == NULL)
        return OPRT_COM_ERROR;

    OPERATE_RET ret = 0;
    dma_config_t dma_config = {0};
    ENCODER_MANAGE_T *coder_manage = &(dvp_mgmt->encoder_manage);

    coder_manage->sequence = 0;
    coder_manage->out_offset = 0;
    if (dvp_mgmt->cur_work_mode == H264_MODE)
    {
        bk_h264_get_fifo_addr(&(coder_manage->out_addr));
        coder_manage->out_channel = bk_fixed_dma_alloc(DMA_DEV_H264, DMA_ID_8);
    }
    else if (dvp_mgmt->cur_work_mode == JPEG_MODE)
    {
        bk_jpeg_enc_get_fifo_addr(&(coder_manage->out_addr));
        coder_manage->out_channel = bk_fixed_dma_alloc(DMA_DEV_JPEG, DMA_ID_8);
    }
    if (coder_manage->out_channel >= DMA_ID_MAX)
    {
        bk_printf("malloc dma fail \r\n");
        return OPRT_MALLOC_FAILED;
    }

    dvp_mgmt->encoded_frame = dvp_cfg->inter_cfg.assign_cb(dvp_mgmt->encoded_frame_fmt, dvp_cfg->inter_cfg.cb_param);
    if (dvp_mgmt->encoded_frame ==NULL)
    {
        bk_printf("%s, assign idle frame fail\r\n", __func__);
        return OPRT_MALLOC_FAILED;
    }
    dvp_mgmt->encoded_frame->frame_fmt = dvp_mgmt->encoded_frame_fmt;
    dvp_mgmt->encoded_frame->width = dvp_cfg->width;
    dvp_mgmt->encoded_frame->height = dvp_cfg->height;
    dvp_mgmt->encoded_frame->is_frame_complete = false;
    dvp_mgmt->encoded_frame->is_i_frame = false;

    dma_config.mode = DMA_WORK_MODE_REPEAT;
    dma_config.chan_prio = 0;
    dma_config.src.width = DMA_DATA_WIDTH_32BITS;
    dma_config.src.start_addr = coder_manage->out_addr;
    dma_config.dst.dev = DMA_DEV_DTCM;
    dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
    dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;

    if (dvp_mgmt->cur_work_mode == H264_MODE)
    {
        dma_config.src.dev = DMA_DEV_H264;
    }
    else if (dvp_mgmt->cur_work_mode == JPEG_MODE)
    {
        dma_config.src.dev = DMA_DEV_JPEG;
    }

    dma_config.dst.start_addr = (uint32_t)dvp_mgmt->encoded_frame->data;
    dma_config.dst.end_addr = (uint32_t)(dvp_mgmt->encoded_frame->data + dvp_mgmt->encoded_frame_len);

    ret = bk_dma_init(coder_manage->out_channel, &dma_config);
    if (ret)
    {
        bk_printf("init dma(%d) fail \r\n", coder_manage->out_channel);
        return OPRT_MALLOC_FAILED;
    }

    bk_dma_set_transfer_len(coder_manage->out_channel, DVP_DMA_CACHE);
    bk_dma_register_isr(coder_manage->out_channel, NULL, __dvp_dma_finish_cb);
    bk_dma_enable_finish_interrupt(coder_manage->out_channel);
#if (CONFIG_SPE)
    bk_dma_set_src_burst_len(coder_manage->out_channel, BURST_LEN_SINGLE);
    bk_dma_set_dest_burst_len(coder_manage->out_channel, BURST_LEN_INC16);
    bk_dma_set_dest_sec_attr(coder_manage->out_channel, DMA_ATTR_SEC);
    bk_dma_set_src_sec_attr(coder_manage->out_channel, DMA_ATTR_SEC);
#endif
    ret = bk_dma_start(coder_manage->out_channel);
    if (ret)
    {
        bk_printf("start dma(%d) fail \r\n", coder_manage->out_channel);
        return OPRT_MALLOC_FAILED;
    }

    return OPRT_OK;
}

static OPERATE_RET __dvp_encoder_input_dma_config(DVP_MODULE_MANAGE_T *dvp_mgmt)
{
    TUYA_DVP_CFG_T *dvp_cfg = &(dvp_mgmt->dvp_cfg);

    if (dvp_cfg->inter_cfg.assign_cb == NULL)
        return OPRT_COM_ERROR;

    OPERATE_RET ret = 0;
    dma_config_t dma_config = {0};
    ENCODER_MANAGE_T *coder_manage = &(dvp_mgmt->encoder_manage);

    coder_manage->in_offset = 0;
    coder_manage->in_addr = bk_yuv_buf_get_em_base_addr();
    coder_manage->in_line_size = (dvp_mgmt->pingpong_len >> 1);
    coder_manage->in_channel = bk_dma_alloc(DMA_DEV_DTCM);
    if (coder_manage->in_channel >= DMA_ID_MAX)
    {
        bk_printf("malloc dma fail \r\n");
        return OPRT_MALLOC_FAILED;
    }

    dvp_mgmt->base_frame = dvp_cfg->inter_cfg.assign_cb(dvp_mgmt->base_frame_fmt, dvp_cfg->inter_cfg.cb_param);
    if (dvp_mgmt->base_frame == NULL)
    {
        bk_printf("%s, assign idle frame fail\r\n", __func__);
        return OPRT_MALLOC_FAILED;
    }

    dvp_mgmt->base_frame->width = dvp_cfg->width;
    dvp_mgmt->base_frame->height = dvp_cfg->height;
    dvp_mgmt->base_frame->frame_fmt = dvp_mgmt->base_frame_fmt;
    dvp_mgmt->base_frame->data_len = dvp_mgmt->base_frame_len;
    dvp_mgmt->base_frame->is_frame_complete = false;

    dma_config.mode = DMA_WORK_MODE_SINGLE;
    dma_config.chan_prio = 1;
    dma_config.src.dev = DMA_DEV_DTCM;
    dma_config.src.width = DMA_DATA_WIDTH_32BITS;
    dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.src.start_addr = (uint32_t)coder_manage->in_addr;
    dma_config.src.end_addr = (uint32_t)(coder_manage->in_addr + coder_manage->in_line_size);

    dma_config.dst.dev = DMA_DEV_DTCM;
    dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
    dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.dst.start_addr = (uint32_t)dvp_mgmt->base_frame->data;
    dma_config.dst.end_addr = (uint32_t)(dvp_mgmt->base_frame->data + coder_manage->in_line_size);

    ret = bk_dma_init(coder_manage->in_channel, &dma_config);
    if (ret)
    {
        bk_printf("init dma(%d) fail \r\n", coder_manage->in_channel);
        return OPRT_MALLOC_FAILED;
    }

    bk_dma_set_transfer_len(coder_manage->in_channel, coder_manage->in_line_size);
#if (CONFIG_SPE)
    bk_dma_set_src_burst_len(coder_manage->in_channel, 3);
    bk_dma_set_dest_burst_len(coder_manage->in_channel, 3);
    bk_dma_set_dest_sec_attr(coder_manage->in_channel, DMA_ATTR_SEC);
    bk_dma_set_src_sec_attr(coder_manage->in_channel, DMA_ATTR_SEC);
#endif

    return OPRT_OK;
}

static OPERATE_RET __dvp_yuv_mode_init(DVP_MODULE_MANAGE_T *dvp_mgmt)
{
    TUYA_DVP_CFG_T *dvp_cfg = &(dvp_mgmt->dvp_cfg);

    if (dvp_cfg->inter_cfg.assign_cb == NULL)
        return OPRT_COM_ERROR;

    dvp_mgmt->base_frame = dvp_cfg->inter_cfg.assign_cb(dvp_mgmt->base_frame_fmt, dvp_cfg->inter_cfg.cb_param);
    if (dvp_mgmt->base_frame == NULL)
    {
        bk_printf("%s, assign idle frame fail\r\n", __func__);
        return OPRT_MALLOC_FAILED;
    }

    dvp_mgmt->base_frame->width = dvp_cfg->width;
    dvp_mgmt->base_frame->height = dvp_cfg->height;
    dvp_mgmt->base_frame->frame_fmt = dvp_mgmt->base_frame_fmt;
    dvp_mgmt->base_frame->data_len = dvp_mgmt->base_frame_len;
    dvp_mgmt->base_frame->is_frame_complete = false;
    bk_yuv_buf_set_em_base_addr((uint32_t)dvp_mgmt->base_frame->data);

    return OPRT_OK;
}

static OPERATE_RET __dvp_h264_mode_init(DVP_MODULE_MANAGE_T *dvp_mgmt)
{
    OPERATE_RET ret = 0;
    TUYA_DVP_CFG_T *dvp_cfg = &(dvp_mgmt->dvp_cfg);

    ret = bk_h264_init(dvp_cfg->width, dvp_cfg->height);
    if (ret)
        return OPRT_COM_ERROR;

    ret = __dvp_encoder_output_dma_config(dvp_mgmt);
    if (ret)
        return OPRT_COM_ERROR;

#if 0
    os_memset(&coder_manage->sei[0], 0xFF, DVP_H264_SEI_SIZE);

    h264_encode_sei_init(&coder_manage->sei[0]);
#endif

    if (dvp_cfg->output_mode == TUYA_CAMERA_OUTPUT_H264_YUV422_BOTH)
    {
        ret = __dvp_encoder_input_dma_config(dvp_mgmt);
        if (ret)
            return OPRT_COM_ERROR;
    }

    return ret;
}

static OPERATE_RET __dvp_jpeg_mode_init(DVP_MODULE_MANAGE_T *dvp_mgmt)
{
    OPERATE_RET ret = 0;
    TUYA_DVP_CFG_T *dvp_cfg = &(dvp_mgmt->dvp_cfg);

    ret = __dvp_jpeg_module_init(dvp_mgmt);
    if (ret)
        return OPRT_COM_ERROR;

    ret = __dvp_encoder_output_dma_config(dvp_mgmt);
    if (ret)
        return OPRT_COM_ERROR;

    if (dvp_cfg->output_mode == TUYA_CAMERA_OUTPUT_JPEG_YUV422_BOTH)
    {
        ret = __dvp_encoder_input_dma_config(dvp_mgmt);
        if (ret)
            return OPRT_COM_ERROR;
    }

    return ret;
}

static OPERATE_RET __dvp_hardware_init(DVP_MODULE_MANAGE_T *dvp_mgmt)
{
    OPERATE_RET ret = 0;

    ret = __dvp_yuv_buf_module_init(dvp_mgmt);
    if (ret)
        return ret;

    if (dvp_mgmt->cur_work_mode == YUV_MODE)
    {
        ret =  __dvp_yuv_mode_init(dvp_mgmt);
        goto end;
    }

    if (dvp_mgmt->cur_work_mode == H264_MODE)
    {
        ret = __dvp_h264_mode_init(dvp_mgmt);
    }

    if (dvp_mgmt->cur_work_mode == JPEG_MODE)
    {
        ret = __dvp_jpeg_mode_init(dvp_mgmt);
    }

end:
    return ret;
}

static void __dvp_yuv_eof_handler(DVP_MODULE_MANAGE_T *dvp_mgmt)
{
    if (!dvp_mgmt || dvp_mgmt->drv_stat != DVP_DRV_TURN_ON)
		return;

    if (dvp_mgmt->error_flag)
        return;

    TUYA_DVP_CFG_T *dvp_cfg = &(dvp_mgmt->dvp_cfg);
    if (dvp_cfg->inter_cfg.assign_cb == NULL)
        return;

    dvp_mgmt->base_frame->frame_id = dvp_mgmt->frame_id++;
    dvp_mgmt->base_frame->is_frame_complete = true;
    dvp_mgmt->base_frame->total_frame_len = dvp_mgmt->base_frame_len;
    TUYA_DVP_FRAME_MANAGE_T *new_frame = dvp_cfg->inter_cfg.assign_cb(dvp_mgmt->base_frame_fmt, dvp_cfg->inter_cfg.cb_param);
    if (new_frame)
    {
		new_frame->width = dvp_cfg->width;
		new_frame->height = dvp_cfg->height;
		new_frame->frame_fmt = dvp_mgmt->base_frame_fmt;
		new_frame->data_len = dvp_mgmt->base_frame_len;
        new_frame->is_frame_complete = false;
        if (dvp_cfg->inter_cfg.post_cb)
            dvp_cfg->inter_cfg.post_cb(dvp_mgmt->base_frame, dvp_cfg->inter_cfg.cb_param);

		dvp_mgmt->base_frame = new_frame;
    }

    bk_yuv_buf_set_em_base_addr((uint32_t)dvp_mgmt->base_frame->data);
}

static void __dvp_h264_eof_handler(DVP_MODULE_MANAGE_T *dvp_mgmt)
{
    if (!dvp_mgmt || dvp_mgmt->drv_stat != DVP_DRV_TURN_ON)
		return;

    if (dvp_mgmt->error_flag)
        return;

    TUYA_DVP_CFG_T *dvp_cfg = &(dvp_mgmt->dvp_cfg);
    if (dvp_cfg->inter_cfg.assign_cb == NULL)
        return;

    if (dvp_mgmt->encoded_frame == NULL
        || dvp_mgmt->encoded_frame->data == NULL)
    {
        tkl_log_output("Error: encode_frame NULL\r\n");
        return;
    }

    uint32_t real_length = bk_h264_get_encode_count() * 4;
    uint32_t remain_length = 0;
    ENCODER_MANAGE_T *coder_manage = &(dvp_mgmt->encoder_manage);

    coder_manage->sequence++;

    if (coder_manage->sequence > H264_GOP_FRAME_CNT)
    {
        coder_manage->sequence = 1;
    }

    coder_manage->is_i_frame_flag = (coder_manage->sequence == 1) ? true : false;

#if (CONFIG_H264_GOP_START_IDR_FRAME)
    if (coder_manage->sequence == H264_GOP_FRAME_CNT)
    {
        bk_h264_soft_reset();
        coder_manage->sequence = 0;
    }
#endif

    if (real_length > CONFIG_H264_FRAME_SIZE - 0x20)
    {
        bk_printf("%s size over h264 buffer range, %d\r\n", __func__, real_length);
        dvp_mgmt->error_flag = true;
    }

    bk_dma_flush_src_buffer(coder_manage->out_channel);

    remain_length = DVP_DMA_CACHE - bk_dma_get_remain_len(coder_manage->out_channel);

    bk_dma_stop(coder_manage->out_channel);

    coder_manage->out_offset += remain_length;

    if (coder_manage->out_offset != real_length)
    {
        uint32_t left_length = real_length - coder_manage->out_offset;
        bk_printf("%s size no match:%d-%d=%d\r\n", __func__, real_length, coder_manage->out_offset, left_length);
        if (left_length != DVP_DMA_CACHE)
        {
            dvp_mgmt->error_flag = true;
            return;
        }
    }

    coder_manage->out_offset = 0;

    dvp_mgmt->encoded_frame->frame_id = dvp_mgmt->frame_id++;
    dvp_mgmt->encoded_frame->data_len = real_length;
    dvp_mgmt->encoded_frame->total_frame_len = real_length;
    dvp_mgmt->encoded_frame->is_frame_complete = true;
    dvp_mgmt->encoded_frame->is_i_frame = coder_manage->is_i_frame_flag;

#if 0
    handle->encode_frame->crc = hnd_crc8(handle->encode_frame->frame, handle->encode_frame->length, 0xFF);
    handle->encode_frame->length += H264_SELF_DEFINE_SEI_SIZE;
    os_memcpy(&handle->sei[23], (uint8_t *)handle->encode_frame, sizeof(frame_buffer_t));
    os_memcpy(&handle->encode_frame->frame[handle->encode_frame->length - H264_SELF_DEFINE_SEI_SIZE], &handle->sei[0], H264_SELF_DEFINE_SEI_SIZE);
#endif

    TUYA_DVP_FRAME_MANAGE_T *new_frame = dvp_cfg->inter_cfg.assign_cb(TUYA_FRAME_FMT_H264, dvp_cfg->inter_cfg.cb_param);
    if (new_frame)
    {
        new_frame->width = dvp_cfg->width;
        new_frame->height = dvp_cfg->height;
        new_frame->frame_fmt = TUYA_FRAME_FMT_H264;
        new_frame->is_frame_complete = false;
        new_frame->is_i_frame = false;
        if (dvp_cfg->inter_cfg.post_cb)
            dvp_cfg->inter_cfg.post_cb(dvp_mgmt->encoded_frame, dvp_cfg->inter_cfg.cb_param);

        dvp_mgmt->encoded_frame = new_frame;
    }
    else
    {
        bk_h264_soft_reset();
        dvp_mgmt->encoded_frame->data_len = 0;
        coder_manage->sequence = 0;
    }

out:
    bk_dma_set_dest_addr(coder_manage->out_channel, (uint32_t)dvp_mgmt->encoded_frame->data,
        (uint32_t)dvp_mgmt->encoded_frame->data + dvp_mgmt->encoded_frame_len);
    bk_dma_start(coder_manage->out_channel);

    if (!dvp_mgmt->error_flag &&
        dvp_cfg->output_mode == TUYA_CAMERA_OUTPUT_H264_YUV422_BOTH)
    {
        coder_manage->in_offset = 0;
        bk_dma_flush_src_buffer(coder_manage->in_channel);
        dvp_mgmt->base_frame->frame_id = dvp_mgmt->frame_id - 1;
        dvp_mgmt->base_frame->is_frame_complete = true;
        dvp_mgmt->base_frame->total_frame_len = dvp_mgmt->base_frame_len;
        new_frame = dvp_cfg->inter_cfg.assign_cb(dvp_mgmt->base_frame_fmt, dvp_cfg->inter_cfg.cb_param);
        if (new_frame)
        {
            new_frame->width = dvp_cfg->width;
            new_frame->height = dvp_cfg->height;
            new_frame->frame_fmt = dvp_mgmt->base_frame_fmt;
            new_frame->data_len = dvp_mgmt->base_frame_len;
            new_frame->is_frame_complete = false;
            if (dvp_cfg->inter_cfg.post_cb)
                dvp_cfg->inter_cfg.post_cb(dvp_mgmt->base_frame, dvp_cfg->inter_cfg.cb_param);

            dvp_mgmt->base_frame = new_frame;
        }
    }

    return;
}

static void __dvp_jpeg_eof_handler(DVP_MODULE_MANAGE_T *dvp_mgmt)
{
    if (!dvp_mgmt || dvp_mgmt->drv_stat != DVP_DRV_TURN_ON)
		return;

    if (dvp_mgmt->error_flag)
        return;

    TUYA_DVP_CFG_T *dvp_cfg = &(dvp_mgmt->dvp_cfg);
    if (dvp_cfg->inter_cfg.assign_cb == NULL)
        return;

    if (dvp_mgmt->encoded_frame == NULL
        || dvp_mgmt->encoded_frame->data == NULL)
    {
        tkl_log_output("g_dvp_module_manage->encode_frame NULL error\n");
        return;
    }

    ENCODER_MANAGE_T *coder_manage = &(dvp_mgmt->encoder_manage);
    bk_dma_flush_src_buffer(coder_manage->out_channel);

    uint32_t real_length = bk_jpeg_enc_get_frame_size();
    uint32_t remain_length = 0;

    remain_length = DVP_DMA_CACHE - bk_dma_get_remain_len(coder_manage->out_channel);

    bk_dma_stop(coder_manage->out_channel);

    uint32_t tmp_flag = false;
    coder_manage->out_offset = coder_manage->out_offset + remain_length - JPEG_CRC_SIZE;

    if (coder_manage->out_offset != real_length)
    {
        tkl_log_output("%s size no match:%u-%u=%u\r\n", __func__, real_length, coder_manage->out_offset, real_length - coder_manage->out_offset);
        dvp_mgmt->error_flag = true;
        return;
    }

    coder_manage->out_offset = 0;

    uint8_t *jpeg_buf = dvp_mgmt->encoded_frame->data;
    uint8_t eof_flag = false;
    for (uint32_t i = real_length; i > real_length - 10; i--)
    {
        if (jpeg_buf[i - 1] == 0xD9 && jpeg_buf[i - 2] == 0xFF)
        {
            real_length = i;
            eof_flag = true;
            break;
        }
        eof_flag = false;
    }

    if (!eof_flag)
    {
        dvp_mgmt->encoded_frame->data_len = 0;
        goto out;
    }

    dvp_mgmt->encoded_frame->frame_id = dvp_mgmt->frame_id++;
    dvp_mgmt->encoded_frame->data_len = real_length;
    dvp_mgmt->encoded_frame->total_frame_len = real_length;
    dvp_mgmt->encoded_frame->is_frame_complete = true;

    TUYA_DVP_FRAME_MANAGE_T *new_frame = dvp_cfg->inter_cfg.assign_cb(TUYA_FRAME_FMT_JPEG, dvp_cfg->inter_cfg.cb_param);
    if (new_frame)
    {
        new_frame->width = dvp_cfg->width;
        new_frame->height = dvp_cfg->height;
        new_frame->frame_fmt = TUYA_FRAME_FMT_JPEG;
        new_frame->is_frame_complete = false;
        if (dvp_cfg->inter_cfg.post_cb)
            dvp_cfg->inter_cfg.post_cb(dvp_mgmt->encoded_frame, dvp_cfg->inter_cfg.cb_param);

        dvp_mgmt->encoded_frame = new_frame;
    }
    else
    {
        dvp_mgmt->encoded_frame->data_len = 0;
    }

out:
    bk_dma_set_dest_addr(coder_manage->out_channel, (uint32_t)dvp_mgmt->encoded_frame->data,
        (uint32_t)dvp_mgmt->encoded_frame->data + dvp_mgmt->encoded_frame_len);
    bk_dma_start(coder_manage->out_channel);

    if (!dvp_mgmt->error_flag &&
        dvp_cfg->output_mode == TUYA_CAMERA_OUTPUT_JPEG_YUV422_BOTH)
    {
        coder_manage->in_offset = 0;
        bk_dma_flush_src_buffer(coder_manage->in_channel);
        dvp_mgmt->base_frame->frame_id = dvp_mgmt->frame_id - 1;
        dvp_mgmt->base_frame->is_frame_complete = true;
        dvp_mgmt->base_frame->total_frame_len = dvp_mgmt->base_frame_len;
        new_frame = dvp_cfg->inter_cfg.assign_cb(dvp_mgmt->base_frame_fmt, dvp_cfg->inter_cfg.cb_param);
        if (new_frame)
        {
            new_frame->width = dvp_cfg->width;
            new_frame->height = dvp_cfg->height;
            new_frame->frame_fmt = dvp_mgmt->base_frame_fmt;
            new_frame->data_len = dvp_mgmt->base_frame_len;
            new_frame->is_frame_complete = false;
            if (dvp_cfg->inter_cfg.post_cb)
                dvp_cfg->inter_cfg.post_cb(dvp_mgmt->base_frame, dvp_cfg->inter_cfg.cb_param);

            dvp_mgmt->base_frame = new_frame;
        }
    }

    return;
}

static void __yuv_arv_isr_cb(yuv_buf_unit_t id, void *param)
{
    DVP_MODULE_MANAGE_T *dvp_mgmt = (DVP_MODULE_MANAGE_T *)param;
    if (dvp_mgmt == NULL || dvp_mgmt->msg_queue == NULL)
        return;

    TKL_DVP_MSG_T msg = {DVP_YUV_EOF, NULL};
    if (tkl_queue_post(dvp_mgmt->msg_queue, &msg, 0) != OPRT_OK)
        dvp_mgmt->error_flag = true;
}

static void __jpeg_eof_isr_cb(yuv_buf_unit_t id, void *param)
{
    DVP_MODULE_MANAGE_T *dvp_mgmt = (DVP_MODULE_MANAGE_T *)param;
    if (dvp_mgmt == NULL || dvp_mgmt->msg_queue == NULL)
        return;

    TKL_DVP_MSG_T msg = {DVP_JPEG_EOF, NULL};
    if (tkl_queue_post(dvp_mgmt->msg_queue, &msg, 0) != OPRT_OK)
        dvp_mgmt->error_flag = true;
}

static void __h264_eof_isr_cb(yuv_buf_unit_t id, void *param)
{
    DVP_MODULE_MANAGE_T *dvp_mgmt = (DVP_MODULE_MANAGE_T *)param;
    if (dvp_mgmt == NULL || dvp_mgmt->msg_queue == NULL)
        return;

    TKL_DVP_MSG_T msg = {DVP_H264_EOF, NULL};
    if (tkl_queue_post(dvp_mgmt->msg_queue, &msg, 0) != OPRT_OK)
        dvp_mgmt->error_flag = true;
}

static void __yuv_sm0_isr_cb(yuv_buf_unit_t id, void *param)
{
    DVP_MODULE_MANAGE_T *dvp_mgmt = (DVP_MODULE_MANAGE_T *)param;
    if (!dvp_mgmt || dvp_mgmt->drv_stat != DVP_DRV_TURN_ON)
		return;

    if (dvp_mgmt->error_flag)
        return;

    ENCODER_MANAGE_T *coder_manage = &(dvp_mgmt->encoder_manage);

    if ((coder_manage->in_offset + coder_manage->in_line_size) > dvp_mgmt->base_frame_len)
    {
        coder_manage->in_offset = 0;
    }

    if (bk_dma_get_enable_status(coder_manage->in_channel))
    {
        tkl_log_output("%s dma channel to xfer yuv data is still busy\r\n", __func__);
        dvp_mgmt->error_flag = true;
        return;
    }

    bk_dma_stop(coder_manage->in_channel);
    bk_dma_set_src_start_addr(coder_manage->in_channel,
                              (uint32_t)coder_manage->in_addr);
    bk_dma_set_dest_start_addr(coder_manage->in_channel,
                               (uint32_t)(dvp_mgmt->base_frame->data + coder_manage->in_offset));
    bk_dma_start(coder_manage->in_channel);
    coder_manage->in_offset += coder_manage->in_line_size;
}

static void __yuv_sm1_isr_cb(yuv_buf_unit_t id, void *param)
{
    DVP_MODULE_MANAGE_T *dvp_mgmt = (DVP_MODULE_MANAGE_T *)param;
    if (!dvp_mgmt || dvp_mgmt->drv_stat != DVP_DRV_TURN_ON)
		return;

    if (dvp_mgmt->error_flag)
        return;

    ENCODER_MANAGE_T *coder_manage = &(dvp_mgmt->encoder_manage);

    if ((coder_manage->in_offset + coder_manage->in_line_size) > dvp_mgmt->base_frame_len)
    {
        coder_manage->in_offset = 0;
    }

    if (bk_dma_get_enable_status(coder_manage->in_channel))
    {
        tkl_log_output("%s dma channel to xfer yuv data is still busy\r\n", __func__);
        dvp_mgmt->error_flag = true;
        return;
    }

    bk_dma_stop(coder_manage->in_channel);
    bk_dma_set_src_start_addr(coder_manage->in_channel,
                              (uint32_t)coder_manage->in_addr + coder_manage->in_line_size);
    bk_dma_set_dest_start_addr(coder_manage->in_channel,
                               (uint32_t)(dvp_mgmt->base_frame->data + coder_manage->in_offset));
    bk_dma_start(coder_manage->in_channel);
    coder_manage->in_offset += coder_manage->in_line_size;
}

static void __error_isr_cb(jpeg_unit_t id, void *param)
{
    DVP_MODULE_MANAGE_T *dvp_mgmt = (DVP_MODULE_MANAGE_T *)param;
    if (dvp_mgmt != NULL && !dvp_mgmt->error_flag)
        dvp_mgmt->error_flag = true;
}

static void __tkl_dvp_reset(DVP_MODULE_MANAGE_T *dvp_mgmt)
{
    TUYA_DVP_CFG_T *dvp_cfg = &(dvp_mgmt->dvp_cfg);

    bk_yuv_buf_stop(YUV_MODE);

    yuv_mode_t mode = dvp_mgmt->cur_work_mode;
	if (mode == JPEG_MODE || mode == JPEG_YUV_MODE)
	{
        bk_yuv_buf_stop(JPEG_MODE);
		bk_jpeg_enc_soft_reset();
	}
	else if (mode == H264_MODE || mode == H264_YUV_MODE)
	{
        bk_yuv_buf_stop(H264_MODE);
        bk_h264_encode_disable();
        bk_h264_init(dvp_cfg->width, dvp_cfg->height);
	}

    bk_yuv_buf_soft_reset();

    ENCODER_MANAGE_T *coder_manage = &(dvp_mgmt->encoder_manage);
    coder_manage->sequence = 0;

    if (dvp_mgmt->is_mix_mode)
    {
        bk_dma_flush_src_buffer(coder_manage->in_channel);
        coder_manage->in_offset = 0;
    }

    if (coder_manage->out_channel < DMA_ID_MAX)
    {
        bk_dma_flush_src_buffer(coder_manage->out_channel);
        coder_manage->out_offset = 0;
        bk_dma_stop(coder_manage->out_channel);
        if (dvp_mgmt->encoded_frame != NULL)
        {
            dvp_mgmt->encoded_frame->data_len = 0;
        }
        bk_dma_start(coder_manage->out_channel);
    }

	if (mode == JPEG_MODE || mode == JPEG_YUV_MODE)
	{
        bk_yuv_buf_start(JPEG_MODE);
	}
	else if (mode == H264_MODE || mode == H264_YUV_MODE)
	{
        bk_yuv_buf_start(H264_MODE);
        bk_h264_encode_enable();
	}
    else
    {
        bk_yuv_buf_start(YUV_MODE);
    }
}

static void __yuv_vsync_negedge_cb(jpeg_unit_t id, void *param)
{
    DVP_MODULE_MANAGE_T *dvp_mgmt = (DVP_MODULE_MANAGE_T *)param;
    if (!dvp_mgmt)
        return;

    if (dvp_mgmt->drv_stat == DVP_DRV_TURNING_OFF || !dvp_mgmt->output_enable)
    {
        bk_yuv_buf_stop(YUV_MODE);
		bk_yuv_buf_stop(JPEG_MODE);
		bk_yuv_buf_stop(H264_MODE);

        if (dvp_mgmt->drv_stat == DVP_DRV_TURNING_OFF && 
            g_dvp_module_manage.drv_close_sem != NULL)
            tkl_semaphore_post(g_dvp_module_manage.drv_close_sem);

        return;
    }

    if (!dvp_mgmt->error_flag)
        return;

    __tkl_dvp_reset(dvp_mgmt);

    dvp_mgmt->error_flag = false;
}

static void __dvp_frame_task(void *args)
{
	if (!args)
		return;

	TKL_DVP_MSG_T msg = {0};
    DVP_MODULE_MANAGE_T *dvp_mgmt = (DVP_MODULE_MANAGE_T *)args;
    uint8_t task_running = true;

	OPERATE_RET ret = tkl_queue_create_init(&(dvp_mgmt->msg_queue), sizeof(TKL_DVP_MSG_T), DVP_MSG_QUE_SIZE);

	while (task_running)
	{
		ret = tkl_queue_fetch(dvp_mgmt->msg_queue, &msg, TKL_QUEUE_WAIT_FROEVER);
        if(ret == OPRT_OK)
		{
            switch(msg.event)
			{
                case DVP_YUV_EOF:
				{
                    __dvp_yuv_eof_handler(dvp_mgmt);
					break;
				}
                case DVP_H264_EOF:
                {
                    __dvp_h264_eof_handler(dvp_mgmt);
                    break;
                }
                case DVP_JPEG_EOF:
                {
                    __dvp_jpeg_eof_handler(dvp_mgmt);
                    break;
                }
                case DVP_TASK_EXIT:
				{
                    while(tkl_queue_fetch(dvp_mgmt->msg_queue, &msg, 0) == OPRT_OK);
					task_running = false;
                    break;
				}
                default:
                    break;
            }
        }
	}

	tkl_queue_free(dvp_mgmt->msg_queue);
	dvp_mgmt->msg_queue = NULL;

	bk_printf("Tkl dvp task ready to release.....\r\n");
	if (dvp_mgmt->task_close_sem)
		tkl_semaphore_post(dvp_mgmt->task_close_sem);

	/* 统一由 deinit 侧 tkl_thread_release；此处挂起，避免自删与外部删双删 */
	vTaskSuspend(NULL);
}

static void __dvp_isr_register(DVP_MODULE_MANAGE_T *dvp_mgmt)
{
    TUYA_DVP_CFG_T *dvp_cfg = &(dvp_mgmt->dvp_cfg);
    switch (dvp_cfg->output_mode)
    {
    case TUYA_CAMERA_OUTPUT_YUV422:
        bk_yuv_buf_register_isr(YUV_BUF_YUV_ARV, __yuv_arv_isr_cb, (void *)dvp_mgmt);
        break;
    case TUYA_CAMERA_OUTPUT_JPEG:
    case TUYA_CAMERA_OUTPUT_JPEG_YUV422_BOTH:
        bk_jpeg_enc_register_isr(JPEG_EOF, __jpeg_eof_isr_cb, (void *)dvp_mgmt);
        bk_jpeg_enc_register_isr(JPEG_FRAME_ERR, __error_isr_cb, NULL);
        break;
    case TUYA_CAMERA_OUTPUT_H264:
    case TUYA_CAMERA_OUTPUT_H264_YUV422_BOTH:
        bk_h264_register_isr(H264_FINAL_OUT, __h264_eof_isr_cb, (void *)dvp_mgmt);
        break;
    default:
        break;
    }

    if (dvp_cfg->output_mode == TUYA_CAMERA_OUTPUT_H264_YUV422_BOTH
        || dvp_cfg->output_mode == TUYA_CAMERA_OUTPUT_JPEG_YUV422_BOTH)
    {
        bk_yuv_buf_register_isr(YUV_BUF_SM0_WR, __yuv_sm0_isr_cb, (void *)dvp_mgmt);
        bk_yuv_buf_register_isr(YUV_BUF_SM1_WR, __yuv_sm1_isr_cb, (void *)dvp_mgmt);
    }

    bk_yuv_buf_register_isr(YUV_BUF_VSYNC_NEGEDGE, __yuv_vsync_negedge_cb, (void *)dvp_mgmt);
	bk_yuv_buf_register_isr(YUV_BUF_SEN_RESL, __error_isr_cb, (void *)dvp_mgmt);
    bk_yuv_buf_register_isr(YUV_BUF_FULL, __error_isr_cb, (void *)dvp_mgmt);
    bk_yuv_buf_register_isr(YUV_BUF_H264_ERR, __error_isr_cb, (void *)dvp_mgmt);
    bk_yuv_buf_register_isr(YUV_BUF_ENC_SLOW, __error_isr_cb, (void *)dvp_mgmt);
}

static OPERATE_RET __tkl_dvp_init(DVP_MODULE_MANAGE_T *dvp_mgmt)
{
    OPERATE_RET ret = 0;
    ret = __dvp_config_param_check(dvp_mgmt);
    if (ret)
        return OPRT_NOT_SUPPORTED;

    TUYA_DVP_CFG_T *dvp_cfg = &(dvp_mgmt->dvp_cfg);
    uint32_t clk = dvp_cfg->inter_cfg.sensor_clk;
    __ty_clk_to_bk_clk(clk, &dvp_mgmt->bk_clk);

    // SMP版本gpio功能都在usr_gpio_cfg配好
    // bk_video_gpio_init(DVP_GPIO_ALL);

    if (dvp_mgmt->bk_clk)
    {
        //enable mclk
        // bk_video_dvp_mclk_enable(YUV_MODE);
        //update mclk config
        bk_video_set_mclk(dvp_mgmt->bk_clk);

        if (dvp_cfg->inter_cfg.setup_cb != NULL)
        {
            ret = dvp_cfg->inter_cfg.setup_cb(dvp_cfg->inter_cfg.cb_param);
            if (ret != OPRT_OK)
                return ret;
        }
    }

    ret = tkl_thread_create_in_psram(&(dvp_mgmt->dvp_frame_task), "dvp_frame", 16384, 4, __dvp_frame_task, (void *)dvp_mgmt);
    if (ret)
    {
        dvp_mgmt->dvp_frame_task = NULL;
        return OPRT_NOT_SUPPORTED;
    }

    ret = __dvp_hardware_init(dvp_mgmt);
    if (ret)
        goto failed;

    __dvp_isr_register(dvp_mgmt);

    dvp_mgmt->output_enable = true;

    yuv_mode_t work_mode = dvp_mgmt->cur_work_mode;

    if (work_mode == JPEG_MODE && dvp_cfg->encoded_quality.jpeg_cfg.enable)
    {
        JPEG_CFG *jpeg_cfg = &dvp_cfg->encoded_quality.jpeg_cfg;
        uint16_t max_bytes = (jpeg_cfg->max_size << 10) & 0xFFFF;
        uint16_t min_bytes = (jpeg_cfg->min_size << 10) & 0xFFFF;
        bk_jpeg_enc_encode_config(1, max_bytes, min_bytes);
        bk_printf("JPEG ENCODE CFG SET: max size: %ld byte, min size: %ld byte\r\n", max_bytes, min_bytes);
    }

    bk_yuv_buf_start(work_mode);
    if (work_mode == H264_MODE)
        bk_h264_encode_enable();

    return OPRT_OK;

failed:
    __tkl_dvp_deinit(dvp_mgmt, true);
    dvp_mgmt->output_enable = true;
    dvp_mgmt->error_flag = false;
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_dvp_init(TUYA_DVP_CFG_T *dvp_cfg)
{
    OPERATE_RET ret = 0;

    if (!dvp_cfg)
        return OPRT_INVALID_PARM;

    memcpy(&(g_dvp_module_manage.dvp_cfg), dvp_cfg, sizeof(TUYA_DVP_CFG_T));

    g_dvp_module_manage.drv_stat = DVP_DRV_TURNING_ON;
    ret = __tkl_dvp_init(&g_dvp_module_manage);
    /* 失败必须 TURN_OFF，否则上层仍会走 tkl_dvp_deinit（Not wait vsync / 再拆一次） */
    g_dvp_module_manage.drv_stat = (ret == OPRT_OK) ? DVP_DRV_TURN_ON : DVP_DRV_TURN_OFF;

    return ret;
}

static void __dvp_frame_task_release(DVP_MODULE_MANAGE_T *dvp_mgmt)
{
    if (dvp_mgmt->dvp_frame_task == NULL)
        return;

    TKL_THREAD_HANDLE frame_task = dvp_mgmt->dvp_frame_task;
    uint8_t exit_ready = false;
    uint8_t retry = 0;

    tkl_semaphore_create_init(&dvp_mgmt->task_close_sem, 0, 1);
    if (dvp_mgmt->msg_queue)
    {
        TKL_DVP_MSG_T msg = {DVP_TASK_EXIT, NULL};
        for (retry = 0; retry < DVP_TASK_EXIT_POST_RETRY; retry++)
        {
            if (tkl_queue_post(dvp_mgmt->msg_queue, &msg, 0) == OPRT_OK)
            {
                exit_ready = true;
                break;
            }
            bk_printf("[%s] post EXIT fail, retry %d/%d\r\n",
                      __func__, retry + 1, DVP_TASK_EXIT_POST_RETRY);
            tkl_system_sleep(1);
        }
    }
    else
    {
        bk_printf("[%s] msg_queue NULL, force delete\r\n", __func__);
    }

    if (exit_ready && dvp_mgmt->task_close_sem)
    {
        if (tkl_semaphore_wait(dvp_mgmt->task_close_sem, 500))
        {
            bk_printf("[%s] wait EXIT timeout, force delete\r\n", __func__);
        }
    }
    else if (!exit_ready)
    {
        bk_printf("[%s] EXIT not posted, force delete\r\n", __func__);
    }

    tkl_system_sleep(2);

    tkl_thread_release(frame_task);
    dvp_mgmt->dvp_frame_task = NULL;

    if (dvp_mgmt->task_close_sem)
    {
        tkl_semaphore_release(dvp_mgmt->task_close_sem);
        dvp_mgmt->task_close_sem = NULL;
    }

    if (dvp_mgmt->msg_queue)
    {
        tkl_queue_free(dvp_mgmt->msg_queue);
        dvp_mgmt->msg_queue = NULL;
    }
}

static OPERATE_RET __tkl_dvp_deinit(DVP_MODULE_MANAGE_T *dvp_mgmt, uint8_t skip_hw)
{
	// SMP版本gpio功能都在usr_gpio_cfg配好
	// bk_video_gpio_deinit(DVP_GPIO_ALL);

    // step 1: deinit hardware (unconditional, same as first version;
    if (!skip_hw)
    {
        bk_yuv_buf_deinit();
        bk_h264_encode_disable();
        bk_h264_deinit();
        bk_jpeg_enc_deinit();
    }

    if (dvp_mgmt->bk_clk)
    {
        bk_video_dvp_mclk_disable();
        dvp_mgmt->bk_clk = 0;
    }

    // step 2: deinit dma
    ENCODER_MANAGE_T *coder_manage = &(dvp_mgmt->encoder_manage);
    if (dvp_mgmt->cur_work_mode == H264_MODE && coder_manage->out_channel < DMA_ID_MAX)
    {
        bk_dma_stop(coder_manage->out_channel);
        bk_dma_deinit(coder_manage->out_channel);
        bk_dma_free(DMA_DEV_H264, coder_manage->out_channel);
        coder_manage->out_channel = DMA_ID_MAX;
    }

    if (dvp_mgmt->cur_work_mode == JPEG_MODE && coder_manage->out_channel < DMA_ID_MAX)
    {
        bk_dma_stop(coder_manage->out_channel);
        bk_dma_deinit(coder_manage->out_channel);
        bk_dma_free(DMA_DEV_JPEG, coder_manage->out_channel);
        coder_manage->out_channel = DMA_ID_MAX;
    }

    if (dvp_mgmt->is_mix_mode && coder_manage->in_channel < DMA_ID_MAX)
    {
        bk_dma_stop(coder_manage->in_channel);
        bk_dma_deinit(coder_manage->in_channel);
        bk_dma_free(DMA_DEV_DTCM, coder_manage->in_channel);
        coder_manage->in_channel = DMA_ID_MAX;
    }
    dvp_mgmt->is_mix_mode = false;

    // step3: exit dvp_frame thread
    __dvp_frame_task_release(dvp_mgmt);

    dvp_mgmt->output_enable = false;

    if (dvp_mgmt->pingpong_buf)
    {
        os_free(dvp_mgmt->pingpong_buf);
        dvp_mgmt->pingpong_buf = NULL;
    }

    dvp_mgmt->base_frame_len = 0;
    dvp_mgmt->base_frame = NULL;

    dvp_mgmt->encoded_frame_len = 0;
    dvp_mgmt->encoded_frame = NULL;

    TUYA_DVP_CFG_T *cfg = &(dvp_mgmt->dvp_cfg);
    memset(cfg, 0, sizeof(TUYA_DVP_CFG_T));

    dvp_mgmt->error_flag = false;

    return OPRT_OK;
}

OPERATE_RET tkl_dvp_deinit()
{
    if (g_dvp_module_manage.drv_stat != DVP_DRV_TURN_ON)
        return OPRT_COM_ERROR;

    OPERATE_RET ret = 0;
    g_dvp_module_manage.drv_stat = DVP_DRV_TURNING_OFF;

    // if output_enable is false, directly deinit hardware
    if (g_dvp_module_manage.output_enable == false)
        goto deinit_hardware;

    // wait frame_neg sync
    tkl_semaphore_create_init(&g_dvp_module_manage.drv_close_sem, 0, 1);
    if (g_dvp_module_manage.drv_close_sem)
    {
        if (tkl_semaphore_wait(g_dvp_module_manage.drv_close_sem, 500))
            bk_printf("[%s] Not wait yuv vsync negedge!\r\n", __func__);
        tkl_system_sleep(2);
        tkl_semaphore_release(g_dvp_module_manage.drv_close_sem);
        g_dvp_module_manage.drv_close_sem = NULL;
    }

deinit_hardware:
    ret = __tkl_dvp_deinit(&g_dvp_module_manage, false);
    g_dvp_module_manage.drv_stat = DVP_DRV_TURN_OFF;
    g_dvp_module_manage.output_enable = true;
    g_dvp_module_manage.error_flag = false;

    return ret;
}

OPERATE_RET tkl_dvp_start()
{
    if (g_dvp_module_manage.drv_stat != DVP_DRV_TURN_ON || g_dvp_module_manage.output_enable)
        return OPRT_COM_ERROR;

    g_dvp_module_manage.output_enable = true;
    __tkl_dvp_reset(&g_dvp_module_manage);
    return OPRT_OK;
}

OPERATE_RET tkl_dvp_stop()
{
    if (g_dvp_module_manage.drv_stat != DVP_DRV_TURN_ON)
        return OPRT_COM_ERROR;

    g_dvp_module_manage.output_enable = false;
    return OPRT_OK;
}

/*
 * H.264 quality ladder. BK exposes no absolute bitrate register: the encoder
 * is driven by QP and the per-macroblock bit budget. These values are the
 * starting points for 480x480@20; the 1024 entry matches the SDK's MIDDLE
 * quality level. Measure the real output on a board and adjust the table when
 * the resolution, frame rate or product scene changes.
 */
typedef struct
{
    uint32_t kbps;
    uint8_t init_qp;
    uint8_t i_max_qp;
    uint8_t p_max_qp;
    uint16_t imb_bits;
    uint16_t pmb_bits;
} DVP_H264_PRESET_T;

static const DVP_H264_PRESET_T sg_dvp_h264_presets[] =
{
    {1024, 40, 40, 25, 160, 60},
    { 700, 45, 45, 30, 120, 45},
    { 450, 50, 50, 38,  76, 30},
    { 250, 51, 51, 45,  40, 16},
};

OPERATE_RET tkl_dvp_set_h264_bitrate(uint32_t kbps)
{
    const DVP_H264_PRESET_T *preset = NULL;
    compress_ratio_t ratio = {0};
    bk_err_t ret;
    uint32_t i;

    if (kbps == 0)
        return OPRT_INVALID_PARM;

    if (g_dvp_module_manage.cur_work_mode != H264_MODE)
        return OPRT_NOT_SUPPORTED;

    if (g_dvp_module_manage.drv_stat != DVP_DRV_TURN_ON ||
        !g_dvp_module_manage.output_enable)
        return OPRT_COM_ERROR;

    /* Highest preset not above the request. Never report a rate that the
     * encoder cannot represent; callers retry with a valid value. */
    for (i = 0; i < sizeof(sg_dvp_h264_presets) / sizeof(sg_dvp_h264_presets[0]); i++)
    {
        if (sg_dvp_h264_presets[i].kbps <= kbps &&
            (preset == NULL || sg_dvp_h264_presets[i].kbps > preset->kbps))
        {
            preset = &sg_dvp_h264_presets[i];
        }
    }
    if (preset == NULL)
        return OPRT_NOT_SUPPORTED;

    ratio.mode        = H264_MODE;
    ratio.qp.init_qp  = preset->init_qp;
    ratio.qp.i_min_qp = 0;
    ratio.qp.i_max_qp = preset->i_max_qp;
    ratio.qp.p_min_qp = 0;
    ratio.qp.p_max_qp = preset->p_max_qp;
    ratio.imb_bits    = preset->imb_bits;
    ratio.pmb_bits    = preset->pmb_bits;
    /* The driver applies these in the FINAL_OUT ISR, at the end of the next
     * encoded frame. A second request before that returns BK_FAIL. */
    ratio.enable      = 1;

    ret = bk_h264_set_base_config(&ratio);
    if (ret == BK_OK)
    {
        bk_printf("[tkl_dvp] h264 bitrate %u kbps -> preset %u kbps "
                  "(qp %u/%u/%u, imb %u, pmb %u)\r\n",
                  kbps, preset->kbps, preset->init_qp, preset->i_max_qp,
                  preset->p_max_qp, preset->imb_bits, preset->pmb_bits);
        return OPRT_OK;
    }
    if (ret == BK_FAIL)
    {
        /* h264_compress.enable is still set: the previous change has not
         * reached a frame boundary yet. */
        return OPRT_RESOURCE_NOT_READY;
    }

    return OPRT_COM_ERROR;
}
