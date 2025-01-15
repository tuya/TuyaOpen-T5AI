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

#include "otp_hal.h"
#include "otp_ll.h"
#include <driver/hal/hal_flash_types.h>

bk_err_t otp_hal_init(otp_hal_t *hal)
{
	hal->hw = (otp_hw_t *)OTP_LL_REG_BASE();
	hal->hw2 = (otp2_hw_t *)OTP2_LL_REG_BASE();
	if(otp_ll_init(hal->hw) == -1)
		return BK_FAIL;
	return BK_OK;
}

bk_err_t otp_hal_deinit(otp_hal_t *hal)
{
	hal->hw = (otp_hw_t *)OTP_LL_REG_BASE();
	otp_ll_deinit(hal->hw);
	return BK_OK;
}