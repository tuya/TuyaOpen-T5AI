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

#pragma once

#include <common/bk_include.h>
#include <driver/hal/hal_otp_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

typedef enum{
    /*Hardware*/
    OTP_MEMORY_CHECK_MARK = 0,
    OTP_AES_KEY,

    /*ARM*/
    OTP_MODEL_ID,
    OTP_MODEL_KEY,
    OTP_ARM_DEVICE_ID,/*repeated*/
    OTP_DEVICE_ROOT_KEY,
    OTP_BL1_BOOT_PUBLIC_KEY_HASH,
    OTP_BL2_BOOT_PUBLIC_KEY_HASH,
    OTP_ARM_LCS,/*repeated*/
    OTP_LOCK_CONTROL,

    /**/
    OTP_BL1_SECURITY_COUNTER,
    OTP_BL2_SECURITY_COUNTER,
    OTP_HUK,
    OTP_IAK,
    OTP_IAK_LEN,
    OTP_IAK_TYPE,
    OTP_IAK_ID,
    OTP_BOOT_SEED,
    OTP_LCS,
    OTP_IMPLEMENTATION_ID,
    OTP_HW_VERSION,
    OTP_VERIFICATION_SERVICE_URL,
    OTP_PROFILE_DEFINITION,
    OTP_ENTROPY_SEED,
    OTP_SECURE_DEBUG_PK,

    /**/
    OTP_MAC_ADDRESS,
    OTP_VDDDIG_BANDGAP,
    OTP_DIA,
    OTP_GADC_CALIBRATION,
    OTP_SDMADC_CALIBRATION,
    OTP_DEVICE_ID,
    OTP_MEMORY_CHECK_VDDDIG,
    OTP_GADC_TEMPERATURE,
    OTP_RESERVED,
    OTP_CHIP_RESERVED,
    OTP_APB_TEST,

    OTP_APB_MAX,
} otp_id_t;

typedef enum{
    OTP_PHY_PWR = 0,
    OTP_RFCALI1,
    OTP_RFCALI2,
    OTP_RFCALI3,
    OTP_RFCALI4,
}otp2_id_t;

typedef enum{
    OTP_READ_WRITE = 0,
    OTP_READ_ONLY,
    OTP_NO_ACCESS,
} otp_privilege_t;

typedef enum{
    OTP_BUSY = 0x1,
    OTP_ERROR = 0x2,
    OTP_WARNING = 0x4,
    OTP_WRONG = 0x8,
    OTP_FORBID = 0x10,
} otp_status_t;