#ifndef __BAT_ASR_API_H__
#define __BAT_ASR_API_H__

#include "bat_asr_type.h"

typedef void* BAT_INST;

#ifdef __cplusplus
extern "C" {
#endif
int BatCreateInst(BAT_INST* inst);
int BatVerify(const char* szKeyStr, int nKeyLen);
int BatCreateOfflineInst(BAT_INST* inst, const char* szNnet, const char* szGraph, const char* szPhoneMap);
int BatDestroyInst(BAT_INST inst);
int BatSetParameter(BAT_INST inst, AsrParamType eType, const char* szValue);
int BatStartProcess(BAT_INST inst);
int BatStopProcess(BAT_INST inst);
int BatWriteAudio(BAT_INST inst, char* pData, int nLen, int bFinish, PAsrResult* ppResult);
int BatGetWordSymbols(BAT_INST inst, AsrWord32** pWords, int* nWords);
int BatSetCmScale(BAT_INST inst, int nWordId, float fScale);
int BatGetCmScale(BAT_INST inst, int nWordId, float* fScale);
int BatGetVadStatus(BAT_INST inst, AsrVadStatus* eStatus);
#ifdef __cplusplus
	}
#endif

#endif // __BAT_ASR_API_H__
