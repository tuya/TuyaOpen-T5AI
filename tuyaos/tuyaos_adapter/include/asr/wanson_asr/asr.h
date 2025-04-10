#ifndef ASR_ASR_H_
#define ASR_ASR_H_

#include "fst_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_WANSON_ASR_GROUP_VERSION

/**
 * @brief asr语音初始化
 * 
 * @return int 0 - OK, -1 - Error
 */
int  Wanson_ASR_Init();

/**
 * @brief 设置分组
 * 
 * @param fst 分组ID对应的struct
 */
void Wanson_ASR_Set_Fst(Fst *fst);

/*****************************************
* Input:
*      - buf      : Audio data (16k, 16bit, mono) 
*      - buf_len  : Now must be 480 (30ms) 
*
* Output:
*      - text     : The text of ASR 
*      - score    : The confidence of ASR (Now not used)
*
* Return value    :  0 - No result
*                    1 - Has result 
*                   -1 - Error
******************************************/
int  Wanson_ASR_Recog(short *buf, int buf_len, const char **text, float *score);

void Wanson_ASR_Release();

#ifdef __cplusplus
}
#endif

#endif
