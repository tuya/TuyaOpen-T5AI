
#include <stdio.h>
#include "bl_config.h"
#include "system.h"

void bk_delay(int num) {
	volatile int i, j;

	for (i = 0; i < num; i++) {
		for (j = 0; j < 100; j++)
			;
	}
}

void delay_us(uint32 num) {

	volatile uint32 i, j, us_count;
	us_count = 4;
	for (i = 0; i < num; i++)
		for (j = 0; j < us_count; j++)
			;

}



static uint32_t timeout100mscnt = 0;
static  uint32_t timeout5mscnt = 0;
static  uint32_t timeout1mscnt = 0;


uint32_t get_time_cnt(void) {
	return timeout100mscnt;
}

uint32_t get_5mstime_cnt_int(void) {
	return timeout5mscnt;
}

static   uint32_t cpu_cnt = 0; //get_5mstime_cnt use at rd_status ,so cpu_cnt  == 224,time == 5ms
uint32_t get_5mstime_cnt(void) {

	cpu_cnt++;
//	if((cpu_cnt % 224) == 0) //one rd_stutus
	if( cpu_cnt >= 224 ) //one rd_stutus
	{
		timeout5mscnt++;
		cpu_cnt = 0;
		BK3000_start_wdt(0xA000);
	}
//	bl_printf("get_5ms %d,%d\r\n",cpu_cnt,timeout5mscnt);
	return timeout5mscnt;
}

static uint32_t cpu_cnt1 = 0;
uint32_t get_1mstime_cnt(void) {
	cpu_cnt1++;
//	if((cpu_cnt1 % 900) == 0)
if( (cpu_cnt1 >= 900)  )
	{
		timeout1mscnt++;
		cpu_cnt1 = 0;
		BK3000_start_wdt(0xA000);
	}

	return timeout1mscnt;
}

void set_5mstime_cnt(uint32_t cnt) {
	 cpu_cnt = 0;
	 timeout5mscnt = cnt;
}

void set_1mstime_cnt(uint32_t cnt) {
	 cpu_cnt1 = 0;
	 timeout1mscnt = cnt;
}


void mem_set(void* dst, int val, uint32 cnt) {
	uint8 *d = (uint8*) dst;
	uint32 *d32st = (uint32*) dst;
	uint32 data_set = (val << 24) + (val << 16) + (val << 8)  + val  ;

	if((cnt%4) == 0){

	   cnt = cnt/4;
	   while(cnt--)
		   *d32st++ = data_set ;
	}

	else {

	    while (cnt--)
		   *d++ = (uint8) val;
	}
}
// EOF
