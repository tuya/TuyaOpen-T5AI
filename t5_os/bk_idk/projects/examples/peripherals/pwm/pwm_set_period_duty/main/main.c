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

#include <components/log.h>
#include <driver/pwm.h>
#include <os/os.h>

#define TAG "PWM_EXAM"

#define PWM_CLOCK_SOURCE (26000000)
#define PWM_FREQ         (1000)
#define DUTY_MAX         (2047)

#define PWM_CH_0        PWM_ID_0
#define PWM_CH_1        PWM_ID_4
#define PWM_CH_2        PWM_ID_6
#define PWM_CH_3        PWM_ID_8
#define PWM_CH_4        PWM_ID_10

#define PWM_STEP        (69)
#define LOOP_CYCLE      (30)

static uint32_t s_period = PWM_CLOCK_SOURCE / PWM_FREQ;

static void pwm_init(pwm_chan_t channel)
{
	pwm_init_config_t init_config = {0};
	init_config.psc = 0,
	init_config.period_cycle = s_period;
	init_config.duty_cycle = 2;
	init_config.duty2_cycle = 0;
	init_config.duty3_cycle = 0;
	bk_pwm_init(channel, &init_config);
	bk_pwm_start(channel);
}

void pwm_update_duty(pwm_chan_t channel, uint32_t duty)
{
	if (duty > DUTY_MAX) {
		duty = DUTY_MAX;
	}

	pwm_period_duty_config_t pwm_config = {
		.psc = 0,
		.period_cycle = s_period,
		.duty_cycle = duty * s_period / DUTY_MAX,
		.duty2_cycle = 0,
		.duty3_cycle = 0,
	};

	BK_LOGI(TAG, "chan:%d period=%d duty=%d\n", channel, pwm_config.period_cycle, pwm_config.duty_cycle);
	bk_pwm_set_period_duty(channel, &pwm_config);
}

int main(void)
{
	bk_pwm_driver_init();

	pwm_init(PWM_CH_0);
	pwm_init(PWM_CH_1);
	pwm_init(PWM_CH_2);
	pwm_init(PWM_CH_3);
	pwm_init(PWM_CH_4);

	while (1) {
		BK_LOGI(TAG, "turn on\n");
		for (uint32_t loop = 0; loop < LOOP_CYCLE; loop++) {
			pwm_update_duty(PWM_CH_0, (loop + 1) * PWM_STEP);
			pwm_update_duty(PWM_CH_1, (loop + 1) * PWM_STEP);
			pwm_update_duty(PWM_CH_2, (loop + 1) * PWM_STEP);
			pwm_update_duty(PWM_CH_3, (loop + 1) * PWM_STEP);
			pwm_update_duty(PWM_CH_4, (loop + 1) * PWM_STEP);
			rtos_delay_milliseconds(100);
		}
		rtos_delay_milliseconds(1000);

		BK_LOGI(TAG, "turn off\n");
		for (uint32_t loop = 0; loop < LOOP_CYCLE; loop++) {
			pwm_update_duty(PWM_CH_0, (29 - loop) * PWM_STEP);
			pwm_update_duty(PWM_CH_1, (29 - loop) * PWM_STEP);
			pwm_update_duty(PWM_CH_2, (29 - loop) * PWM_STEP);
			pwm_update_duty(PWM_CH_3, (29 - loop) * PWM_STEP);
			pwm_update_duty(PWM_CH_4, (29 - loop) * PWM_STEP);
			rtos_delay_milliseconds(100);
		}
		rtos_delay_milliseconds(1000);
	}

	return 0;
}
