#ifndef _BEKEN_AIR_KISS_H_
#define _BEKEN_AIR_KISS_H_

#include "bk_uart.h"
#include <os/os.h>

#define BK_AIRKISS_DEBUG           1
#if BK_AIRKISS_DEBUG
#define BK_AIRKISS_PRT             os_printf
#define BK_AIRKISS_WARN            warning_prf
#define BK_AIRKISS_FATAL           fatal_prf
#else
#define BK_AIRKISS_PRT             null_prf
#define BK_AIRKISS_WARN            null_prf
#define BK_AIRKISS_FATAL           null_prf
#endif

#define AIRKISS_SUCCESS                0
#define AIRKISS_FAILURE                1

#define MAX_CHANNEL_NUM                13
#define BK_AIRKISS_TIMEOUT             (1000 * 90)
#define BK_AIRKISS_SWITCH_TIMER        50     // ms
#define BK_AIRKISS_CYCLE_TIMER         20000  // 20s
#define MAX_UDP_RANDOM_SEND            20

#define BK_AIRKISS_THD_PRIORITY        BEKEN_DEFAULT_WORKER_PRIORITY


extern uint32_t bk_airkiss_get_decode_status(void);
extern uint32_t bk_airkiss_decode_isnot_over(void);

#endif // _BEKEN_AIR_KISS_H_
// eof
