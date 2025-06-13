/**
 * @file tkl_asr.c
 * @version 0.1
 * @date 2025-04-08
 */

#include "tkl_asr.h"

#include "asr.h"
#include "fst_01.h"
#include "fst_02.h"
#include "fst_types.h"

/***********************************************************
************************macro define************************
***********************************************************/
#define TKL_ASR_DETECT_CHUNK_SIZE (960)

/***********************************************************
***********************typedef define***********************
***********************************************************/


/***********************************************************
***********************variable define**********************
***********************************************************/
static Fst sg_wanson_fst;
static TKL_ASR_WAKEUP_WORD_E sg_wakeup_word_list[TKL_ASR_WAKEUP_WORD_MAX];
static uint8_t sg_wakeup_word_cnt = 0;

/***********************************************************
***********************function define**********************
***********************************************************/
static bool  __compare_wakeup_word(char *text, TKL_ASR_WAKEUP_WORD_E wakeup_word)
{
    bool ret = false;

    if(NULL == text || strlen(text) == 0) {
        return false;
    }

    switch(wakeup_word) {
        case TKL_ASR_WAKEUP_NIHAO_TUYA:
            if (strcmp(text, "你好涂鸦") == 0   ||
                strcmp(text, "嘿体优丫") == 0   ||
                strcmp(text, "嗨体优丫") == 0   ||
                strcmp(text, "嘿突丫") == 0     ||
                strcmp(text, "嗨突丫") == 0) {  

                ret = true;
            }
        break;
        case TKL_ASR_WAKEUP_NIHAO_XIAOZHI:
            if(strcmp(text, "你好小智") == 0) {
                ret = true;
            }
        break;
        case TKL_ASR_WAKEUP_XIAOZHI_TONGXUE:
            if(strcmp(text, "小智同学") == 0) {
                ret = true;
            }
        break;
        case TKL_ASR_WAKEUP_XIAOZHI_GUANJIA:
            if(strcmp(text, "小智管家") == 0) {
                ret = true;
            }
        break;
        case TKL_ASR_WAKEUP_XIAOAI_XIAOAI:
            if(strcmp(text, "小艾小艾") == 0) {
                ret = true;
            }
        break;
        case TKL_ASR_WAKEUP_XIAODU_XIAODU:
            if(strcmp(text, "小杜小杜") == 0) {
                ret = true;
            }
        break;
        default:
            bk_printf("not support wakeup word:%d", wakeup_word);
        break;
    }

    return ret;
}

OPERATE_RET tkl_asr_init(void)
{
    if(Wanson_ASR_Init() < 0) {
        bk_printf("Wanson_ASR_Init error\r\n");
        return OPRT_COM_ERROR;
    }

    memset(&sg_wanson_fst, 0x00, sizeof(Fst));
#if 1
    sg_wanson_fst.states     = fst01_states;
    sg_wanson_fst.num_states = fst01_num_states;
    sg_wanson_fst.finals     = fst01_finals;
    sg_wanson_fst.num_finals = fst01_num_finals;
    sg_wanson_fst.words      = fst01_words;
#else 
    sg_wanson_fst.states     = fst02_states;
    sg_wanson_fst.num_states = fst02_num_states;
    sg_wanson_fst.finals     = fst02_finals;
    sg_wanson_fst.num_finals = fst02_num_finals;
    sg_wanson_fst.words      = fst02_words;
#endif
    Wanson_ASR_Set_Fst(&sg_wanson_fst);

    bk_printf("tkl_asr_init OK!\n");

    return OPRT_OK;
}

OPERATE_RET tkl_asr_wakeup_word_config(TKL_ASR_WAKEUP_WORD_E *wakeup_word_arr, uint8_t arr_cnt)
{
    if(NULL == wakeup_word_arr || 0 == arr_cnt || arr_cnt > TKL_ASR_WAKEUP_WORD_MAX) {
        return OPRT_INVALID_PARM;
    }

    memcpy(sg_wakeup_word_list, wakeup_word_arr, arr_cnt * sizeof(TKL_ASR_WAKEUP_WORD_E));
    sg_wakeup_word_cnt  = arr_cnt;

    return OPRT_OK;
}

uint32_t tkl_asr_get_process_uint_size(void)
{
    return TKL_ASR_DETECT_CHUNK_SIZE;
}

TKL_ASR_WAKEUP_WORD_E tkl_asr_recognize_wakeup_word(uint8_t *data, uint32_t len)
{
    uint8_t i = 0;
    int ret = 0;
    char *text;
    float score;

    if(NULL == data || 0 == len) {
        bk_printf("param err\r\n");
        return TKL_ASR_WAKEUP_WORD_UNKNOWN;
    }

    if(0 == sg_wakeup_word_cnt) {
        bk_printf("wakeup word not config\r\n");
        return TKL_ASR_WAKEUP_WORD_UNKNOWN;
    }

    if(len < TKL_ASR_DETECT_CHUNK_SIZE) {
        bk_printf("datalen not enough \r\n");
        return TKL_ASR_WAKEUP_WORD_UNKNOWN;
    }

    ret = Wanson_ASR_Recog((short*)data, len/2, (const char **)&text, &score);
    if(ret != 1) {
        if(ret == -1) {
            bk_printf("Wanson_ASR_Recog error\r\n");
        }
        return TKL_ASR_WAKEUP_WORD_UNKNOWN;
    }

    bk_printf("Wanson_ASR_Recog -> %s \n", text);

    for(i=0; i<sg_wakeup_word_cnt; i++) {
        if(true ==__compare_wakeup_word(text, sg_wakeup_word_list[i])) {
            return sg_wakeup_word_list[i];
        }
    }

    return TKL_ASR_WAKEUP_WORD_UNKNOWN;
}

OPERATE_RET tkl_asr_deinit(void)
{
    Wanson_ASR_Release();

    memset(sg_wakeup_word_list, 0x00, sizeof(sg_wakeup_word_list));
    sg_wakeup_word_cnt = 0;

    return OPRT_OK;
}


