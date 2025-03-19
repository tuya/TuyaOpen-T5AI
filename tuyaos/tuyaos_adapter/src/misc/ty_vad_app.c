// Copyright 2015-2024 Beken
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
#include "lib/vad.h"
#include "ty_vad_app.h"

#define VAD_INPUT_BUFF_NUM 6
#define MSG_COUNT 20
#define DURATION_PER_FRAME 20 /*20ms*/

// ADC底层用的是20ms的帧长
#define FRAME_COUNT_PER_SECOND (1000 / DURATION_PER_FRAME)

enum {
	CMD_VAD_START = 1,
	CMD_VAD_DATA,
	CMD_VAD_CANCEL,
	CMD_VAD_FLAG_CLEAR,
};

typedef enum {
    VAD_STATE_IDLE = 0,
    VAD_STATE_WORKING,
} VAD_WORK_STATE;

typedef struct
{
	uint16_t cmd;
	uint16_t data_len;
	uint8_t *data_buf;
} VAD_MSG_ST, *VAD_MSG_PTR;

typedef struct
{
    beken_queue_t msg_queue;
    int vad_start_ms;
    int vad_end_ms;
    int vad_silence_ms;
    uint16_t sample_rate;
    uint16_t channel;
} VAD_ST, *VAD_ST_PTR;

static VAD_ST_PTR vad_ptr = NULL;
static uint8_t vad_flag = TY_VAD_FLAG_VAD_NONE;
static uint8_t vad_silence_flag = 0;
static uint16_t vad_frame_len = 0;
static uint8_t vad_work_state = VAD_STATE_IDLE;
static uint8_t is_vad_init = 0;
static uint8_t *vad_buffer = NULL;

static void vad_handle_thread(void *arg)
{
    VAD_MSG_ST msg;
    unsigned char *in_pcm_buf = NULL;
    unsigned int data_len;
    int ret;
    ty_vad_config_t *cfg = (ty_vad_config_t *)arg;

    vad_ptr->vad_start_ms = cfg->start_threshold_ms;
    vad_ptr->vad_end_ms = cfg->end_threshold_ms;
    vad_ptr->sample_rate = cfg->sample_rate;
    vad_ptr->channel = cfg->channel;
    vad_ptr->vad_silence_ms = cfg->silence_threshold_ms;
    vad_work_state = VAD_STATE_IDLE;
    // NOTE: vad_buffer这里的帧长是mic回调过来的帧长，是10ms的帧长，所以要除以2。而底层的ADC采样的帧长是20ms
    vad_buffer = os_malloc((vad_ptr->sample_rate / FRAME_COUNT_PER_SECOND) * 2 * vad_ptr->channel / 2 * VAD_INPUT_BUFF_NUM);
    if (vad_buffer == NULL) {
        os_printf("vad buffer malloc failed\r\n");
        return;
    }

    is_vad_init = 1;
    while (1) {
        ret = rtos_pop_from_queue(&vad_ptr->msg_queue, &msg, BEKEN_WAIT_FOREVER);
        if (BK_OK != ret)
            continue;

        switch (msg.cmd) {
        case CMD_VAD_START:
            if (vad_work_state == VAD_STATE_IDLE) {
                vad_frame_len = (vad_ptr->sample_rate / FRAME_COUNT_PER_SECOND) * 2 * vad_ptr->channel;
                wb_vad_deinit();
                wb_vad_enter(vad_ptr->vad_start_ms, vad_ptr->vad_end_ms, vad_frame_len, vad_ptr->vad_silence_ms); // vad start
                os_printf("---vad_enter:%d---\r\n", vad_frame_len);
            }
            vad_work_state = VAD_STATE_WORKING;
            break;

        case CMD_VAD_DATA:
            in_pcm_buf = msg.data_buf;
            data_len = msg.data_len;
            if (vad_work_state == VAD_STATE_WORKING) {
                ret = wb_vad_entry((char *)in_pcm_buf, data_len); /*vad process*/
                if (ret == 1) {
                    os_printf("------------vad start----------\r\n");
                    vad_flag = TY_VAD_FLAG_VAD_START;
                }
                else if (ret == 2) {
                    os_printf("------------vad end----------\r\n");
                    vad_flag = TY_VAD_FLAG_VAD_END;
                    vad_work_state = VAD_STATE_IDLE;
                    ty_vad_task_send_msg(CMD_VAD_START, NULL, 0);
                }
                else if (ret == 3) {
                    os_printf("------------silence----------\r\n");
                    vad_silence_flag = 1;
                }
            }
            break;

        case CMD_VAD_CANCEL:
            os_printf("----vad cancel---\r\n");
            wb_vad_deinit();
            vad_flag = TY_VAD_FLAG_VAD_NONE;
            vad_work_state = VAD_STATE_IDLE;
            break;

        case CMD_VAD_FLAG_CLEAR:
            vad_flag = TY_VAD_FLAG_VAD_NONE;
            break;

        default:
            break;
        }
    }
}

int ty_vad_app_init(const ty_vad_config_t *config)
{
    int ret;
    if (vad_ptr != NULL) {
        os_printf("=====already init====\r\n");
        return 1;
    }

    vad_ptr = (VAD_ST_PTR)os_malloc(sizeof(VAD_ST));
    if (vad_ptr == NULL)
        return -1;

    ret = rtos_init_queue(&vad_ptr->msg_queue, "vad_que", sizeof(VAD_MSG_ST), MSG_COUNT);
    if (ret != BK_OK)
        return -1;
    ret = rtos_create_thread(NULL,
                             5,
                             "vad",
                             (beken_thread_function_t)vad_handle_thread,
                             4096,
                             (void *)config);
    if (ret != BK_OK) {
        rtos_deinit_queue(&vad_ptr->msg_queue);
        os_free(vad_ptr);
        vad_ptr = NULL;
    }
    return ret;
}

uint8_t ty_vad_app_is_init(void)
{
    return is_vad_init;
}

int ty_vad_app_start(void)
{
    if (vad_ptr == NULL)
        return -1;

    return ty_vad_task_send_msg(CMD_VAD_START, NULL, 0);
}

int ty_vad_app_stop(void)
{
    if (vad_ptr == NULL)
        return -1;

    return ty_vad_task_send_msg(CMD_VAD_CANCEL, NULL, 0);
}

int ty_vad_frame_put(unsigned char *data, unsigned int size)
{
    if (is_vad_init == 1 && vad_ptr != NULL) {
        static uint8_t frame_index = 0;
        if (ty_get_vad_work_status() == VAD_STATE_WORKING) {
            os_memcpy((void *)vad_buffer + frame_index * size, data, size);
            frame_index++;
            if ((frame_index & 0x01) == 0) {
                ty_vad_task_send_msg(CMD_VAD_DATA, vad_buffer + (frame_index >> 1) * size, size << 1);
            }

            if (frame_index >= VAD_INPUT_BUFF_NUM)
                frame_index = 0;
        }
        else {
            frame_index = 0;
        }
    }
    return 0;
}

bk_err_t ty_vad_task_send_msg(uint16_t cmd, uint8_t *buf, uint16_t data_len)
{
    bk_err_t ret = kGeneralErr;
    VAD_MSG_ST msg;

    if (vad_ptr == NULL)
        return ret;

    msg.cmd = cmd;
    msg.data_buf = buf;
    msg.data_len = data_len;

    ret = rtos_push_to_queue(&vad_ptr->msg_queue, &msg, BEKEN_NO_WAIT);
    return ret;
}

int ty_get_vad_frame_len(void)
{
    return vad_frame_len;
}

uint8_t ty_get_vad_work_status(void)
{
    return vad_work_state;
}

uint8_t ty_get_vad_flag(void)
{
    return vad_flag;
}

uint8_t ty_get_vad_silence_flag(void)
{
    return vad_silence_flag;
}

void ty_clear_vad_silence_flag(void)
{
    vad_silence_flag = 0;
}

void ty_set_vad_threshold(int start_threshold_ms, int end_threshold_ms, int silence_threshold_ms)
{
    if (vad_ptr != NULL) {
        vad_ptr->vad_start_ms = start_threshold_ms;
        vad_ptr->vad_end_ms = end_threshold_ms;
        vad_ptr->vad_silence_ms = silence_threshold_ms;
    }
}

void ty_set_mic_param(int sample_rate, int channel)
{
    if (vad_ptr != NULL) {
        vad_ptr->sample_rate = sample_rate;
        vad_ptr->channel = (uint16_t)channel;
    }
}
