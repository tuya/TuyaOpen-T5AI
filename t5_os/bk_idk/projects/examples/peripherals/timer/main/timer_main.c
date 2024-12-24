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

#include <common/sys_config.h>
#include <components/log.h>
#include <os/mem.h>
#include <driver/timer.h>
#include "sys_driver.h"

#define TAG "timer_example"

static timer_id_t timer_id = TIMER_ID0;
static uint32_t timer_ms = 2000;
static uint32_t isr_count = 0;

static void timer1_examples_isr(timer_id_t timer_id)
{
	BK_LOGI(TAG, "timer1(%d) enter timer1_example_isr\r\n", timer_id);
	BK_LOGI(TAG, "current counter value:%d\r\n", bk_timer_get_cnt(timer_id));
	BK_LOGI(TAG, "period counter value:%d\r\n", bk_timer_get_period(timer_id));
	BK_LOGI(TAG, "timer0-5 enable status:0x%x\r\n", bk_timer_get_enable_status());
	BK_LOG_ON_ERR(bk_timer_disable(timer_id));
	BK_LOG_ON_ERR(bk_timer_stop(timer_id));
	BK_LOG_ON_ERR(bk_timer_cancel(timer_id));
}

static void timer0_examples_isr(timer_id_t timer_id)
{
	BK_LOGI(TAG, "timer0(%d) enter timer0_example_isr\r\n", timer_id);
	isr_count++;
	BK_LOGI(TAG, "current counter value:%d\r\n", bk_timer_get_cnt(timer_id));
	BK_LOGI(TAG, "period counter value:%d\r\n", bk_timer_get_period(timer_id));
	BK_LOG_ON_ERR(bk_timer_disable(timer_id));
	BK_LOGI(TAG, "timer0-5 enable status:0x%x\r\n", bk_timer_get_enable_status());
	BK_LOGI(TAG, "current counter value(%d)\r\n", bk_timer_get_cnt(timer_id));
	BK_LOG_ON_ERR(bk_timer_enable(timer_id));
	BK_LOGI(TAG, "timer0-5 enable status:0x%x\r\n", bk_timer_get_enable_status());
	BK_LOGI(TAG, "current counter value(%d)\r\n", bk_timer_get_cnt(timer_id));
	if (isr_count >= 2) {
		BK_LOG_ON_ERR(bk_timer_stop(timer_id));
		BK_LOG_ON_ERR(bk_timer_cancel(timer_id));
		timer_id = TIMER_ID1;
		timer_ms = 1000;
		BK_LOG_ON_ERR(bk_timer_start(timer_id, timer_ms, timer1_examples_isr));
	}
}

int main(void)
{	
	BK_LOG_ON_ERR(bk_timer_driver_init());
	BK_LOG_ON_ERR(bk_timer_start(timer_id, timer_ms, timer0_examples_isr));

	return 0;
}
