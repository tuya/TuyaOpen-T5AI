#ifndef _SYSTEM_H_
#define _SYSTEM_H_
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include "type.h"
#include "bl_config.h"



#if (CHIP_BK3266 || CHIP_BK3288)
#define BL_SPR_VICMR(x)	((9<< 11) + 0x200 + (x))	/* VIC mask registers */

#define SAMPLE_ALIGN   __attribute__((aligned(4)))
#define __PACKED_POST__  __attribute__((packed))


inline unsigned int get_spr(unsigned int sprnum);

inline void set_spr(unsigned int sprnum, unsigned int value);


void disable_intterupt(void);



void SYSirq_Disable_Interrupts_Save_Flags(UINT32 *flags);
void SYSirq_Interrupts_Restore_Flags(UINT32 flags);
#endif //CHIP_BK3266

void bk_delay(int num);
void delay_us(uint32 num);

uint32_t get_5mstime_cnt(void);
void set_5mstime_cnt(uint32_t cnt) ;
void set_1mstime_cnt(uint32_t cnt) ;
uint32_t get_1mstime_cnt(void);


void bsp_initial(void);


void BK3000_start_wdt(uint32_t val);


void uart_enable(uint8 flag,uint8 rx_thr);


void mem_set(void* dst, int val, uint32 cnt) ;



#endif // _SYSTEM_H_
// EOF
