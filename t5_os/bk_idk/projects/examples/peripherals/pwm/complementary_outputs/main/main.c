// Copyright 2024-2025 Beken
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

#include <stdio.h>
#include <os/os.h>
#include <driver/pwm.h>

#define TAG "PWM_EXAM"

#define PWM_CLOCK_SOURCE                (26000000)
#define PWM_FREQ                        (1000)
#define PWM_LOOP_MAX_NUM                (8)
#define PWM_DEAD_TIME_RATIO             (0.1f)
#define PWM_GROUP_CHAN1_INIT_DUTY_RATIO (0.1f)
#define PWM_GROUP_CHAN2_INIT_DUTY_RATIO (1 - PWM_GROUP_CHAN1_INIT_DUTY_RATIO - PWM_DEAD_TIME_RATIO)

#define PWM_CH_0        PWM_ID_0
#define PWM_CH_1        PWM_ID_4
#define PWM_CH_2        PWM_ID_6
#define PWM_CH_3        PWM_ID_8

static uint32_t s_period = PWM_CLOCK_SOURCE / PWM_FREQ;

static pwm_group_t pwm_group_init_without_dead_time(void)
{
	pwm_group_t group = 0;
	pwm_group_init_config_t init_config = {0};

	init_config.chan1 = PWM_CH_0;
	init_config.chan2 = PWM_CH_1;
	init_config.period_cycle = s_period;
	init_config.chan1_duty_cycle = s_period * 0.4f;
	init_config.chan2_duty_cycle = s_period * (1 - 0.4f);

	BK_LOGI(TAG, "period=%d chan1_duty=%d init_chan2_duty=%d\n", init_config.period_cycle, init_config.chan1_duty_cycle, init_config.chan2_duty_cycle);
	BK_LOG_ON_ERR(bk_pwm_group_init(&init_config, &group));
	BK_LOG_ON_ERR(bk_pwm_group_start(group));

	return group;
}

static pwm_group_t pwm_group_init_with_dead_time(void)
{
	pwm_group_t group = 0;
	pwm_group_init_config_t init_config = {0};

	init_config.chan1 = PWM_CH_2;
	init_config.chan2 = PWM_CH_3;
	init_config.period_cycle = s_period;
	init_config.chan1_duty_cycle = s_period * PWM_GROUP_CHAN1_INIT_DUTY_RATIO;
	init_config.chan2_duty_cycle = s_period * PWM_GROUP_CHAN2_INIT_DUTY_RATIO;

	BK_LOGI(TAG, "period=%d chan1_duty=%d chan2_duty=%d\n", init_config.period_cycle, init_config.chan1_duty_cycle, init_config.chan2_duty_cycle);
	BK_LOG_ON_ERR(bk_pwm_group_init(&init_config, &group));
	BK_LOG_ON_ERR(bk_pwm_group_start(group));

	return group;
}

static void pwm_group_update_duty(pwm_group_t group, uint32_t chan1_duty, uint32_t chan2_duty)
{
	pwm_group_config_t pwm_group_config = {0};

	pwm_group_config.period_cycle = s_period;
	pwm_group_config.chan1_duty_cycle = chan1_duty;
	pwm_group_config.chan2_duty_cycle = chan2_duty;

	BK_LOGI(TAG, "period=%d chan1_duty=%d chan2_duty=%d\n", s_period, chan1_duty, chan2_duty);
	BK_LOG_ON_ERR(bk_pwm_group_set_config(group, &pwm_group_config));
}

int main(void)
{
	pwm_group_t group1 = 0;
	pwm_group_t group2 = 0;

	BK_LOG_ON_ERR(bk_pwm_driver_init());

	group1 = pwm_group_init_without_dead_time();
	BK_LOGI(TAG, "group%d start without dead time\r\n", group1);

	rtos_delay_milliseconds(1000);

	group2 = pwm_group_init_with_dead_time();
	BK_LOGI(TAG, "group%d start with dead time\r\n", group2);

	BK_LOGI(TAG, "group%d update chan1 and chan2 duty\r\n", group2);
	for (uint32_t loop = 0; loop < PWM_LOOP_MAX_NUM; loop++) {
		pwm_group_update_duty(group2,
							  s_period * (PWM_GROUP_CHAN1_INIT_DUTY_RATIO + 0.1f * loop),
							  s_period * (PWM_GROUP_CHAN2_INIT_DUTY_RATIO - 0.1f * loop));
		rtos_delay_milliseconds(100);
	}

	pwm_group_update_duty(group2, s_period, 0);
	rtos_delay_milliseconds(1000);

	for (uint32_t loop = 0; loop < PWM_LOOP_MAX_NUM; loop++) {
		pwm_group_update_duty(group2,
							  s_period * (PWM_GROUP_CHAN2_INIT_DUTY_RATIO - 0.1f * loop),
							  s_period * (PWM_GROUP_CHAN1_INIT_DUTY_RATIO + 0.1f * loop));
		rtos_delay_milliseconds(100);
	}

	pwm_group_update_duty(group2, 0, s_period);
	rtos_delay_milliseconds(1000);

	return 0;
}
