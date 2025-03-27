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

#ifndef __AEC_H__
#define __AEC_H__
#include <stdint.h>

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#define AEC_EC_ENABLE           (1)
#define AEC_NS_ENABLE           (1)

#define MaxBand                 (10)

#define FFT_LEN_NB              (256)

#define FFT_LEN_WB              (FFT_LEN_NB<<1)

#define    AEC_FS               (16000)


#if (AEC_FS==8000)

#define    BD                   (1)

#else

#define    BD                   (2)

#endif



#define FFT_LEN_NB_HF           (FFT_LEN_NB>>1)

#define FFT_LEN_WB_HF           (FFT_LEN_WB>>1)

#define SymWin                  (1)            // 12ms delay

#define AsymWin                 (!SymWin)      // 6ms  delay



typedef struct
{   int32_t alpha;
    int32_t sm  [3];
    uint16_t G  [FFT_LEN_NB_HF*BD+1];
    int32_t Sc  [FFT_LEN_NB_HF*BD+1];
    int32_t Ss  [FFT_LEN_NB_HF*BD+1];
    int32_t pSNR[FFT_LEN_NB_HF*BD+1];
}NSContext;

typedef struct _AECContext
{
    uint8_t  flags;
    uint8_t  test;
    uint8_t  vol;
    uint8_t  ref_up;
    int8_t   mu_ec;
    int8_t   ec_depth;
    int8_t   ec_filter;
    int8_t   ref_scale;
    int8_t   ns_filter;
    int8_t   drc_mode;
    int16_t  fs;
    int16_t  is_ec;
    int16_t  cutbin1;
    int16_t  cutbin2;
    int16_t  hr_bin;
    int16_t  sbnum;
    int16_t  fftlen;
    int16_t  Flen;
    int16_t  frame_samples;
    int16_t  freq_scale;
    int16_t  rin_delay_rp;
    int16_t  rin_delay_wp;
    int16_t  max_mic_delay;
    int16_t  mic_delay;
    int16_t  delay_offset;
    int16_t  ec_rsd_thr1;
    int16_t  ec_rsd_thr2;
    int16_t  ec_rsd_thr3;
    int16_t  ec_rsd_thr4;
    int16_t  ec_guard;
    int16_t  spcnt;
    uint16_t minG;
    int32_t  ec_thr;
    int32_t  ec_thr2;
    int32_t  mic_max;
    int32_t  mic_eng;
    int32_t  ref_eng;
    int32_t  dc;
    int32_t  dcr;
    int32_t  cni_floor;
    int32_t  cni_fade;
    int32_t  s_max;
    int32_t  ns_mean;
    int32_t  drc_gain;
    uint32_t frame_cnt;
    int16_t *ana_win;
    int16_t *syn_win;
    int16_t *rin_delay;
    int16_t *sin;
    int16_t *rin;
    int16_t *out;
    uint16_t *EQF;
    uint16_t *OutGainPtr;
    int32_t *ST;
    int32_t *SF;
    int32_t *RF;
    int32_t *ang;

    int16_t drc_pos[6];
    int16_t ec_coe[MaxBand];
    int16_t SubBand[MaxBand];
    int32_t Rbuf[FFT_LEN_NB*BD];
    int32_t Sbuf[FFT_LEN_NB*BD];
    int32_t Ramp[FFT_LEN_NB_HF*BD+1];
    int32_t Samp[FFT_LEN_NB_HF*BD+1];
    int32_t tmp1[(FFT_LEN_NB_HF + 1) * 2 * BD];
    int32_t tmp2[(FFT_LEN_NB_HF + 1) * 2 * BD];
    int32_t tmp3[(FFT_LEN_NB_HF + 1) * 2 * BD];
    int32_t syn [(48*BD)<<SymWin];

    #if AEC_EC_ENABLE
    int32_t ECs[FFT_LEN_NB_HF * BD + 1];
    int32_t Hold[(FFT_LEN_NB_HF + 1) * 2 * BD];
    #endif

    #if AEC_NS_ENABLE
    int32_t   nspara[8];
    int16_t   nspara_step[5];
    NSContext VADInfo;
    NSContext NsInfo;
    #endif
    int16_t   refbuff[0];
}AECContext;

//typedef struct _AECContext AECContext;

/**
 * @brief AEC enum defines
 * @defgroup AEC enums
 * @ingroup AEC
 * @{
 */

enum AEC_CTRL_CMD
{
    AEC_CTRL_CMD_NULL = 0,
    AEC_CTRL_CMD_GET_FLAGS,
    AEC_CTRL_CMD_SET_FLAGS,
    AEC_CTRL_CMD_GET_MIC_DELAY,
    AEC_CTRL_CMD_SET_MIC_DELAY,
    AEC_CTRL_CMD_SET_DRC,
    AEC_CTRL_CMD_SET_EC_DEPTH,
    AEC_CTRL_CMD_SET_NS_LEVEL,
    AEC_CTRL_CMD_SET_NS_FILTER,
    AEC_CTRL_CMD_SET_EC_FILTER,
    AEC_CTRL_CMD_SET_VOL,
    AEC_CTRL_CMD_SET_REF_SCALE,
    AEC_CTRL_CMD_SET_NS_PARA,
    AEC_CTRL_CMD_SET_EC_COE,
    AEC_CTRL_CMD_SET_EC_THR,
    AEC_CTRL_CMD_SET_BANDS,
    AEC_CTRL_CMD_SET_MAX_DELAY,
    AEC_CTRL_CMD_SET_DELAY_BUFF,
    AEC_CTRL_CMD_SET_DRC_TAB,
    AEC_CTRL_CMD_SET_EQ_TAB,
    AEC_CTRL_CMD_SET_REF_UP,
    AEC_CTRL_CMD_GET_RX_BUF,
    AEC_CTRL_CMD_GET_TX_BUF,
    AEC_CTRL_CMD_GET_OUT_BUF,
    AEC_CTRL_CMD_GET_FRAME_SAMPLE,
};

/**
 * @}
 */
/**
 * @brief AUD struct defines
 * @defgroup bk_api_aud_structs structs in AUD
 * @ingroup bk_api_aud
 * @{
 */


/**
 * @}
 */


/**
 * @brief AEC API
 * @defgroup AEC API group
 * @{
 */


/**
 * @brief     Get AECContext size (byte)
 *
 * @param delay delay samples
 *
 * @return
 *    - uint32_t: size(byte)
 */
uint32_t  aec_size    (uint32_t delay);

/**
 * @brief     Init aec
 *
 * This API init aec function :
 *  - Set aec parameters
 *  - Init fft parameters
 *
 * Usage example:
 *
 *
 *     AECContext* aec = NULL;
 *
 *     //malloc aec memory
 *     aec = (AECContext*)os_malloc(aec_size());
 *
 *     //init aec
 *     aec_init(aec, 16000);
 *
 *
 * @param aec aec parameters
 * @param fs frame sample 8K/16K
 *
 * @return none
 */
void aec_init    (AECContext* aec, int16_t fs);

/**
 * @brief     Control aec
 *
 * This API control aec function :
 *  - Set aec parameters
 *  - Get data addr
 *
 * Usage example:
 *
 *
 *     AECContext* aec = NULL;
 *
 *     //malloc aec memory
 *     aec = (AECContext*)os_malloc(aec_size());
 *
 *     //init aec
 *     aec_init(aec, 16000);
 *
 *     //set mic delay
 *     aec_ctrl(aec, AEC_CTRL_CMD_SET_MIC_DELAY, 10);
 *
 *
 * @param aec aec parameters
 * @param cmd control command in enum AEC_CTRL_CMD
 * @param arg value
 *
 * @return none
 */
void aec_ctrl    (AECContext* aec, uint32_t cmd, uint32_t arg);

/**
 * @brief     Control aec
 *
 * This API control aec function :
 *  - Set aec parameters
 *  - Get data addr
 *
 * Usage example:
 *
 *
 *     AECContext* aec = NULL;
 *
 *     //malloc aec memory
 *     aec = (AECContext*)os_malloc(aec_size());
 *
 *     //init aec
 *     aec_init(aec, 16000);
 *
 *     //aec excute work
 *     aec_proc (aec, rin, sin, out);
 *
 *
 * @param aec aec parameters
 * @param rin reference data
 * @param sin source data
 * @param out output data
 *
 * @return none
 */
void aec_proc    (AECContext* aec, int16_t* rin, int16_t* sin, int16_t* out);

uint32_t aec_ver(void);
/**
 * @}
 */

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__AEC_H__
