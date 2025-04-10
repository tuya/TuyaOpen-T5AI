/*
 * Copyright (c) 2017 Spectimbre.
 */

#ifndef _TUTUCLEAR_H_
#define _TUTUCLEAR_H_

#include "tutu_typedef.h"

// Set below to TUTUClearConfig_t.Version for version control
#define TUTUCLEAR_VERSION   MAKETUTUVER(2, 8, 0)


/* ------------------------------------------------------------------------- */
#ifdef __cplusplus
extern "C" {
#endif
UW32 TUTUClear_QueryMemSz(void);
W16 TUTUClear_Init(void *pTUTUClearObjectMem, void **pTUTUClearObject);
W16 TUTUClear_Release(void **pTUTUClearObject);
W16 TUTUClear_OneFrame(void *pTUTUClearObject, W16 *ptMIC,  W32 *pw32WakeWord);
#ifdef __cplusplus
}
#endif

#endif // _TUTUCLEAR_H_
