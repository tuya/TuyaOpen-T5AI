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
#include <components/log.h>

#include "media_core.h"
#include "media_evt.h"
#include "media_app.h"
#include "bt_audio_act.h"

#include <driver/media_types.h>
#include <os/mem.h>
#include <modules/audio_rsp_types.h>
#include <modules/audio_rsp.h>
#include <modules/sbc_encoder.h>

#define TAG "bt_audio"

enum
{
    BT_AUDIO_DEBUG_LEVEL_ERROR,
    BT_AUDIO_DEBUG_LEVEL_WARNING,
    BT_AUDIO_DEBUG_LEVEL_INFO,
    BT_AUDIO_DEBUG_LEVEL_DEBUG,
    BT_AUDIO_DEBUG_LEVEL_VERBOSE,
};

#define BT_AUDIO_DEBUG_LEVEL BT_AUDIO_DEBUG_LEVEL_INFO

#define LOGE(format, ...) do{if(BT_AUDIO_DEBUG_LEVEL >= BT_AUDIO_DEBUG_LEVEL_ERROR)   BK_LOGE(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGW(format, ...) do{if(BT_AUDIO_DEBUG_LEVEL >= BT_AUDIO_DEBUG_LEVEL_WARNING) BK_LOGW(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGI(format, ...) do{if(BT_AUDIO_DEBUG_LEVEL >= BT_AUDIO_DEBUG_LEVEL_INFO)    BK_LOGI(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGD(format, ...) do{if(BT_AUDIO_DEBUG_LEVEL >= BT_AUDIO_DEBUG_LEVEL_DEBUG)   BK_LOGI(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGV(format, ...) do{if(BT_AUDIO_DEBUG_LEVEL >= BT_AUDIO_DEBUG_LEVEL_VERBOSE) BK_LOGI(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)


typedef void (*camera_connect_state_t)(uint8_t state);

enum
{
    BT_AUDIO_ACTION_NONE,
    BT_AUDIO_ACTION_RESAMPLE,
    BT_AUDIO_ACTION_ENCODE,
} BT_AUDIO_ACTION;


typedef struct
{
    uint32_t ret;
    uint32_t in_len;
    uint32_t out_len;
} bt_audio_resample_result_t;

static uint8_t s_is_rsp_inited;


static beken_thread_t s_bt_audio_task = NULL;
static beken_semaphore_t s_bt_audio_sema = NULL;

static volatile uint32_t s_bt_audio_action;
static media_mailbox_msg_t *s_bt_audio_action_mailbox;
static uint8_t s_bt_audio_task_run;
static aud_rsp_cfg_t s_rsp_cfg_final;

static void bt_audio_task(void *arg)
{
    int32_t ret = 0;

    while (s_bt_audio_task_run)
    {
        rtos_get_semaphore(&s_bt_audio_sema, 300);//BEKEN_WAIT_FOREVER);

        ret = 0;

        switch (s_bt_audio_action)
        {
        case 0:
            continue;
            break;

        case EVENT_BT_PCM_RESAMPLE_INIT_REQ:
            do
            {
                if (s_is_rsp_inited)
                {
                    LOGE("resample already init");
                    break;
                }

                extern bk_err_t bk_audio_osi_funcs_init(void);
                bk_audio_osi_funcs_init();

                bt_audio_resample_init_req_t *cfg = (typeof(cfg))s_bt_audio_action_mailbox->param;

                os_memset(&s_rsp_cfg_final, 0, sizeof(s_rsp_cfg_final));

                s_rsp_cfg_final.src_rate = cfg->src_rate;
                s_rsp_cfg_final.src_ch = cfg->src_ch;
                s_rsp_cfg_final.src_bits = cfg->src_bits;
                s_rsp_cfg_final.dest_rate = cfg->dest_rate;
                s_rsp_cfg_final.dest_ch = cfg->dest_ch;
                s_rsp_cfg_final.dest_bits = cfg->dest_bits;
                s_rsp_cfg_final.complexity = cfg->complexity;
                s_rsp_cfg_final.down_ch_idx = cfg->down_ch_idx;

                LOGI("resample init %p %d %d %d %d %d %d %d %d", cfg,
                     s_rsp_cfg_final.src_rate,
                     s_rsp_cfg_final.src_ch,
                     s_rsp_cfg_final.src_bits,
                     s_rsp_cfg_final.dest_rate,
                     s_rsp_cfg_final.dest_ch,
                     s_rsp_cfg_final.dest_bits,
                     s_rsp_cfg_final.complexity,
                     s_rsp_cfg_final.down_ch_idx);

                ret = bk_aud_rsp_init(s_rsp_cfg_final);

                if (ret)
                {
                    LOGE("bk_aud_rsp_init err %d !!", ret);
                    ret = -1;
                    break;
                }

                s_is_rsp_inited = 1;

            }
            while (0);

            if (ret)
            {
                if (s_is_rsp_inited)
                {
                    bk_aud_rsp_deinit();
                    s_is_rsp_inited = 0;
                }
            }

            break;

        case EVENT_BT_PCM_RESAMPLE_DEINIT_REQ:
            do
            {
                if (!s_is_rsp_inited)
                {
                    LOGE("resample already deinit");
                    break;
                }

                ret = bk_aud_rsp_deinit();

                if (ret)
                {
                    LOGE("bk_aud_rsp_deinit err %d !!", ret);
                    ret = -1;
                    break;
                }

                s_is_rsp_inited = 0;

            }
            while (0);

            if (ret)
            {
                if (s_is_rsp_inited)
                {
                    bk_aud_rsp_deinit();
                    s_is_rsp_inited = 0;
                }
            }

            break;

        case EVENT_BT_PCM_RESAMPLE_REQ:
        {
            bt_audio_resample_req_t *param = (typeof(param))(s_bt_audio_action_mailbox->param);

            uint32_t in_len = *(param->in_bytes_ptr) / (s_rsp_cfg_final.src_bits / 8);
            uint32_t out_len = *(param->out_bytes_ptr) / (s_rsp_cfg_final.dest_bits / 8);

            LOGD("resample start %p %p %p %d %d", param, param->in_addr, param->out_addr, in_len, out_len);

            ret = bk_aud_rsp_process((int16_t *)param->in_addr, &in_len, (int16_t *)param->out_addr, &out_len);

            if (ret)
            {
                LOGE("bk_aud_rsp_process err %d !!", ret);
            }
            else
            {
                *(param->in_bytes_ptr) = in_len * (s_rsp_cfg_final.src_bits / 8);
                *(param->out_bytes_ptr) = out_len * (s_rsp_cfg_final.dest_bits / 8);
            }

            LOGD("resample done %d %d", in_len, out_len);
        }
        break;

        case EVENT_BT_PCM_ENCODE_INIT_REQ:
            ret = 0;
            break;

        case EVENT_BT_PCM_ENCODE_DEINIT_REQ:
            ret = 0;
            break;

        case EVENT_BT_PCM_ENCODE_REQ:
        {
            bt_audio_encode_req_t *param = (typeof(param))(s_bt_audio_action_mailbox->param);

            if (!param || !param->handle || !param->in_addr || !param->out_len_ptr)
            {
                LOGE("encode req param err");
                ret = -1;
                break;
            }

            int32_t encode_len = 0;

            if (param->type != 0)
            {
                LOGE("type not match %d", param->type);
                ret = -1;
                break;
            }

            encode_len = sbc_encoder_encode((SbcEncoderContext *)param->handle, (const int16_t *)param->in_addr);

            if (!encode_len)
            {
                LOGE("encode err %d", encode_len);
                ret = -1;
                break;
            }

            *param->out_len_ptr = encode_len;
        }
        break;

        default:
            LOGE("unknow event 0x%x", s_bt_audio_action);
            ret = -1;
            break;
        }

        s_bt_audio_action = 0;
        msg_send_rsp_to_media_major_mailbox(s_bt_audio_action_mailbox, ret, APP_MODULE);
    }

    LOGI("exit");

    rtos_delete_thread(NULL);
}

static bk_err_t bt_audio_init_handle(media_mailbox_msg_t *msg)
{
    int ret = 0;

    LOGI("");

    if (s_bt_audio_task)
    {
        LOGE("already init");
        goto end;
    }

    if (!s_bt_audio_sema)
    {
        ret = rtos_init_semaphore(&s_bt_audio_sema, 1);
    }

    if (ret)
    {
        LOGE("sema init failed");
        ret = -1;
        goto end;
    }

    s_bt_audio_task_run = 1;
    ret = rtos_create_thread(&s_bt_audio_task,
                             4,
                             "bt_audio_task",
                             (beken_thread_function_t)bt_audio_task,
                             1024 * 5,
                             (beken_thread_arg_t)NULL);

    if (ret)
    {
        LOGE("task init failed");
        ret = -1;
        goto end;
    }

end:

    if (ret)
    {
        if (s_bt_audio_task)
        {
            s_bt_audio_task_run = 0;
            rtos_thread_join(s_bt_audio_task);
            s_bt_audio_task = NULL;
        }

        if (s_bt_audio_sema)
        {
            rtos_deinit_semaphore(&s_bt_audio_sema);
            s_bt_audio_sema = NULL;
        }
    }

    msg_send_rsp_to_media_major_mailbox(msg, ret, APP_MODULE);

    return ret;
}


static bk_err_t bt_audio_deinit_handle(media_mailbox_msg_t *msg)
{
    int ret = 0;

    LOGI("");

    if (!s_bt_audio_task)
    {
        LOGE("already deinit");
        goto end;
    }

    s_bt_audio_task_run = 0;
    rtos_thread_join(s_bt_audio_task);
    s_bt_audio_task = NULL;

    if (s_bt_audio_sema)
    {
        rtos_deinit_semaphore(&s_bt_audio_sema);
        s_bt_audio_sema = NULL;
    }

end:

    if (ret)
    {
        if (s_bt_audio_task)
        {
            s_bt_audio_task_run = 0;
            rtos_thread_join(s_bt_audio_task);
            s_bt_audio_task = NULL;
        }

        if (s_bt_audio_sema)
        {
            rtos_deinit_semaphore(&s_bt_audio_sema);
            s_bt_audio_sema = NULL;
        }
    }

    msg_send_rsp_to_media_major_mailbox(msg, ret, APP_MODULE);

    return ret;
}

bk_err_t bt_audio_event_handle(media_mailbox_msg_t *msg)
{
    bk_err_t ret = 0;

    switch (msg->event)
    {
    case EVENT_BT_AUDIO_INIT_REQ:
        ret = bt_audio_init_handle(msg);
        break;

    case EVENT_BT_AUDIO_DEINIT_REQ:
        ret = bt_audio_deinit_handle(msg);
        break;

    default:
        if (!s_bt_audio_task || !s_bt_audio_sema)
        {
            LOGE("task not run");
            msg_send_rsp_to_media_major_mailbox(msg, -1, APP_MODULE);
            break;
        }

        if (EVENT_BT_PCM_RESAMPLE_REQ != msg->event && EVENT_BT_PCM_ENCODE_REQ != msg->event)
        {
            LOGI("evt %d", msg->event);
        }

        s_bt_audio_action_mailbox = msg;
        s_bt_audio_action = msg->event;

        if (s_bt_audio_sema)
        {
            rtos_set_semaphore(&s_bt_audio_sema);
        }

        break;
    }

    return ret;
}

