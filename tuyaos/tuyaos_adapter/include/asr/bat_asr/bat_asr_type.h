#ifndef __BAT_ASR_TYPE_H__
#define __BAT_ASR_TYPE_H__
#include <stdint.h>
typedef enum AsrResTypeTag
{
	RES_AC_MODEL = 0,
	RES_CMD_GRAPH,
	RES_PHONE_MAP,
} AsrResType;

typedef enum AsrParamTypeTag
{
	REC_DECODER_BEAM,
	REC_CM_THRESHOLD1,
	REC_CM_THRESHOLD2,
	REC_CM_THRESHOLD3,
	ASR_SELF_ADAPT_ON,
	ASR_LANGUAGE_TYPE, //0:chinese 1:english
	ASR_VAD_ON,
	ASR_VAD_THRESHOLD,
	ASR_ENERGYCM_ON,
} AsrParamType;

typedef enum AsrVadStatusTag
{
	VAD_INACTIVED = 0,
	VAD_BEGIN,
	VAD_SPEECH,
	VAD_END,
} AsrVadStatus;

typedef char AsrWord32[32];

typedef struct AsrResultTag
{
	int nBegin;
	int nEnd;
	char szText[32];
    short nWordId;
    float fCmScore;
	float fCmScore2;
	float fCmScore3;
	float fCmScore4;
} AsrResult, *PAsrResult;

/** Bluetooth Device Address */
typedef struct {
	uint8_t  val[6];
} bt_addr_t;

/** Bluetooth LE Device Address */
typedef struct {
	uint8_t      type;
	bt_addr_t a;
} bt_addr_le_t;

#endif // __FIG_ASR_TYPE_H__