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
#define PWM_FREQ         (16000)

#define PWM_CH_0        PWM_ID_0
#define PWM_CH_1        PWM_ID_2
#define PWM_CH_2        PWM_ID_4
#define PWM_CH_3        PWM_ID_6
#define PWM_CH_4        PWM_ID_8

static uint32_t s_period = PWM_CLOCK_SOURCE / PWM_FREQ;

#define PHASE_SHIFT_CONFIG { \
	.psc = 0, \
	.chan_num = 5,\
	.period_cycle = s_period, \
	.duty_config[0] = { \
		.chan = PWM_CH_0, \
		.duty_cycle = 0, \
	}, \
	.duty_config[1] = { \
		.chan = PWM_CH_1, \
 		.duty_cycle = 0, \
	}, \
	.duty_config[2] = { \
		.chan = PWM_CH_2, \
		.duty_cycle = 0, \
	}, \
	.duty_config[3] = { \
		.chan = PWM_CH_3, \
		.duty_cycle = 0, \
	}, \
	.duty_config[4] = { \
		.chan = PWM_CH_4, \
		.duty_cycle = 0, \
	}, \
	.duty_config[5] = { \
		.chan = PWM_CH_5, \
		.duty_cycle = 0, \
	}, \
} \

int main(void)
{
	pwm_phase_shift_config_t config = PHASE_SHIFT_CONFIG;

	BK_LOG_ON_ERR(bk_pwm_driver_init());
	BK_LOG_ON_ERR(bk_pwm_phase_shift_init(&config));
	BK_LOG_ON_ERR(bk_pwm_phase_shift_start());

	rtos_delay_milliseconds(1000);

	while (1) {
		BK_LOG_ON_ERR(bk_pwm_phase_shift_set_duty(PWM_CH_0, 0));
		BK_LOG_ON_ERR(bk_pwm_phase_shift_set_duty(PWM_CH_1, s_period * 0.4f));
		BK_LOG_ON_ERR(bk_pwm_phase_shift_set_duty(PWM_CH_3, s_period * 0.3f));
		BK_LOG_ON_ERR(bk_pwm_phase_shift_set_duty(PWM_CH_4, s_period * 0.4f));
		BK_LOG_ON_ERR(bk_pwm_phase_shift_update_duty());
		rtos_delay_milliseconds(10);

		BK_LOG_ON_ERR(bk_pwm_phase_shift_set_duty(PWM_CH_0, s_period * 0.2f));
		BK_LOG_ON_ERR(bk_pwm_phase_shift_set_duty(PWM_CH_1, s_period * 0.2f));
		BK_LOG_ON_ERR(bk_pwm_phase_shift_set_duty(PWM_CH_3, s_period * 0.2f));
		BK_LOG_ON_ERR(bk_pwm_phase_shift_set_duty(PWM_CH_4, s_period * 0.2f));
		BK_LOG_ON_ERR(bk_pwm_phase_shift_update_duty());
		rtos_delay_milliseconds(10);
	}

	return 0;
}

