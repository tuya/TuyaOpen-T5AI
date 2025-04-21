/**
 * @file tkl_vad.c
 * @version 0.1
 * @date 2025-04-16
 */

#include <os/os.h>
#include <os/mem.h>
#include "lib/vad.h"

#include "tkl_memory.h"
#include "tkl_queue.h"
#include "tkl_thread.h"
#include "tkl_vad.h"

/***********************************************************
************************macro define************************
***********************************************************/
#define VAD_INPUT_BUFF_NUM     10
#define MSG_COUNT              20
#define DURATION_PER_FRAME     20 /*20ms*/

// ADC底层用的是20ms的帧长
#define FRAME_COUNT_PER_SECOND (1000 / DURATION_PER_FRAME)

/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef uint8_t TKL_VAD_MSG_CMD_E;
#define VAD_MSG_CMD_START         0
#define VAD_MSG_CMD_DATA          1
#define VAD_MSG_CMD_CANCLE        2
#define VAD_MSG_CMD_DEINIT        3

typedef struct {
    TKL_QUEUE_HANDLE  msg_queue;
    uint8_t          *buffer;
    uint32_t          frame_len;
    
    int               vad_start_ms;
    int               vad_end_ms;
    int               vad_silence_ms;
    int               frame_duration_ms;
    uint16_t          sample_rate;
    uint16_t          channel;
    float             scale;


    bool              is_vad_work;
    TKL_VAD_STATUS_T  vad_state;
    bool              is_vad_silence;

}TKL_VAD_INFO_T;

typedef struct {
	TKL_VAD_MSG_CMD_E cmd;
	uint32_t          data_len;
	uint8_t          *data_buf;
}TKL_VAD_MSG_T;



/***********************************************************
********************function declaration********************
***********************************************************/


/***********************************************************
***********************variable define**********************
***********************************************************/
static TKL_VAD_INFO_T *sg_p_vad_info = NULL;
static TKL_THREAD_HANDLE sg_vad_thread_handle = NULL;
static bool is_vad_init = false;
/***********************************************************
***********************function define**********************
***********************************************************/
static void __tkl_vad_send_msg(TKL_VAD_MSG_CMD_E cmd, uint8_t *data, uint32_t len)
{
    TKL_VAD_MSG_T msg;

    msg.cmd = cmd;
    msg.data_buf = data;
    msg.data_len = len;

    tkl_queue_post(sg_p_vad_info->msg_queue, &msg, 0);
}

static void __tkl_vad_task(void *arg)
{
    TKL_VAD_MSG_T msg;
    OPERATE_RET ret;

    while(1) {
        ret = tkl_queue_fetch(sg_p_vad_info->msg_queue, &msg, TKL_QUEUE_WAIT_FROEVER);
        if(ret != OPRT_OK) {
            continue;
        }

        switch(msg.cmd) {
            case VAD_MSG_CMD_START:
                if (false == sg_p_vad_info->is_vad_work) {
                    wb_vad_deinit();
                    wb_vad_enter(sg_p_vad_info->vad_start_ms, sg_p_vad_info->vad_end_ms, sg_p_vad_info->frame_len, sg_p_vad_info->vad_silence_ms); // vad start
                    os_printf("---vad_enter:%d---\r\n", sg_p_vad_info->frame_len);
                }
                sg_p_vad_info->is_vad_work = true;
            break;
            case VAD_MSG_CMD_DATA: {
                int ret = 0;

                if (true == sg_p_vad_info->is_vad_work) {
                    ret = wb_vad_entry((char *) msg.data_buf, msg.data_len, sg_p_vad_info->scale); /*vad process*/
                    if (ret == 1) {
                        sg_p_vad_info->vad_state = TKL_VAD_STATUS_SPEECH;
                        os_printf("------------vad start----------\r\n");
                    }else if (ret == 2) {
                        sg_p_vad_info->vad_state = TKL_VAD_STATUS_NONE;
                        sg_p_vad_info->is_vad_work = false;
                        __tkl_vad_send_msg(VAD_MSG_CMD_START, NULL, 0);
                        os_printf("------------vad end----------\r\n");
                    }else if (ret == 3) {
                        sg_p_vad_info->is_vad_silence = true;
                        os_printf("------------silence----------\r\n");
                    }
                }
            }
            break;
            case VAD_MSG_CMD_CANCLE:
                wb_vad_deinit();
                sg_p_vad_info->vad_state = TKL_VAD_STATUS_NONE;
                sg_p_vad_info->is_vad_work = false;
                os_printf("----vad cancel---\r\n");
            break;
            case VAD_MSG_CMD_DEINIT: {
                wb_vad_deinit();
                os_free(sg_p_vad_info->buffer);
                tkl_queue_free(sg_p_vad_info->msg_queue);
                os_free(sg_p_vad_info);
                TKL_THREAD_HANDLE tmp_thrd = sg_vad_thread_handle;
                sg_vad_thread_handle = NULL;
                tkl_thread_release(tmp_thrd);
                return;
            }
            break;
            default:
            break;
        }
    }
}


OPERATE_RET tkl_vad_init(TKL_VAD_CONFIG_T *config)
{
    OPERATE_RET ret;

    if(NULL == config) {
        return OPRT_INVALID_PARM;
    }

    if(true == is_vad_init) {
        return OPRT_OK;
    }

    sg_p_vad_info = (TKL_VAD_INFO_T *)os_malloc(sizeof(TKL_VAD_INFO_T));
    if (NULL == sg_p_vad_info) {
        return OPRT_MALLOC_FAILED;
    }
    memset(sg_p_vad_info, 0x00, sizeof(TKL_VAD_INFO_T));

    sg_p_vad_info->vad_silence_ms    = 0;
    sg_p_vad_info->vad_start_ms      = config->speech_min_ms;
    sg_p_vad_info->vad_end_ms        = config->noise_min_ms;
    sg_p_vad_info->sample_rate       = config->sample_rate;
    sg_p_vad_info->channel           = config->channel_num;
    sg_p_vad_info->frame_duration_ms = config->frame_duration_ms;
    sg_p_vad_info->scale             = config->scale;
    if(0 == sg_p_vad_info->scale) {
        sg_p_vad_info->scale = 0;
    }

    sg_p_vad_info->frame_len = (sg_p_vad_info->sample_rate / FRAME_COUNT_PER_SECOND) * 2 * sg_p_vad_info->channel;

    ret = tkl_queue_create_init(&sg_p_vad_info->msg_queue, sizeof(TKL_VAD_MSG_T), MSG_COUNT);
    if (ret != OPRT_OK) {
        os_free(sg_p_vad_info);
        sg_p_vad_info = NULL;
        return ret;
    }

    // NOTE: vad_buffer这里的帧长是mic回调过来的帧长，是10ms的帧长，所以要除以2。而底层的ADC采样的帧长是20ms
    sg_p_vad_info->buffer = os_malloc(sg_p_vad_info->frame_len/ 2 * VAD_INPUT_BUFF_NUM);
    if (sg_p_vad_info->buffer  == NULL) {
        return OPRT_MALLOC_FAILED;
    }

    ret = tkl_thread_create(&sg_vad_thread_handle, "vad", 4096, 5, __tkl_vad_task, NULL);
    if (ret != OPRT_OK) {
        os_free(sg_p_vad_info);
        sg_p_vad_info = NULL;
        os_free(sg_p_vad_info->buffer);
        sg_p_vad_info->buffer = NULL;
        return ret;
    }

    is_vad_init = true;

    return OPRT_OK;
}

OPERATE_RET tkl_vad_feed(uint8_t *data, uint32_t len)
{
    static uint8_t frame_index = 0;

    if(NULL == data || 0 == len) {
        return OPRT_INVALID_PARM;
    }

    if(false == is_vad_init) {
        bk_printf("vad not init\n");
        return OPRT_COM_ERROR;
    }

    if (true == sg_p_vad_info->is_vad_work) {
        os_memcpy((void *)sg_p_vad_info->buffer + frame_index * len, data, len);
        frame_index++;
        if ((frame_index & 0x01) == 0) {
            __tkl_vad_send_msg(VAD_MSG_CMD_DATA, sg_p_vad_info->buffer + (frame_index >> 1) * len, len << 1);
        }

        if (frame_index >= VAD_INPUT_BUFF_NUM)
            frame_index = 0;
    }
    else {
        frame_index = 0;
    }

    return OPRT_OK;
}

TKL_VAD_STATUS_T tkl_vad_get_status(void)
{
    if(false == is_vad_init) {
        return TKL_VAD_STATUS_NONE;
    }

    return sg_p_vad_info->vad_state;
}

OPERATE_RET tkl_vad_start(void)
{
    if(false == is_vad_init) {
        return OPRT_COM_ERROR;
    }

    if(true == sg_p_vad_info->is_vad_work) {
        return OPRT_OK;
    }

    __tkl_vad_send_msg(VAD_MSG_CMD_START, NULL, 0);

    return OPRT_OK;
}

OPERATE_RET tkl_vad_stop(void)
{
    if(false == is_vad_init) {
        return OPRT_COM_ERROR;
    }

    if(false == sg_p_vad_info->is_vad_work) {
        return OPRT_OK;
    }

    __tkl_vad_send_msg(VAD_MSG_CMD_CANCLE, NULL, 0);

    return OPRT_OK;
}

OPERATE_RET tkl_vad_deinit(void)
{
    if(false == is_vad_init) {
        return OPRT_COM_ERROR;
    }

    is_vad_init = false;
    __tkl_vad_send_msg(VAD_MSG_CMD_DEINIT, NULL, 0);

    return OPRT_OK;
}