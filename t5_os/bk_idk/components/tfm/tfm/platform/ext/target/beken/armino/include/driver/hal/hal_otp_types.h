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

#include <common/bk_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief OTP defines
 * @defgroup bk_api_otp_defs macos
 * @ingroup bk_api_otp
 * @{
 */
#define BK_ERR_OTP_READ_BUFFER_NULL   (BK_ERR_OTP_BASE - 1) /**< The pointed buff is NULL */
#define BK_ERR_OTP_ADDR_OUT_OF_RANGE  (BK_ERR_OTP_BASE - 2) /**< OTP address is out of range */
#define BK_ERR_NO_READ_PERMISSION     (BK_ERR_OTP_BASE - 3) /**< Not have read privilege  */
#define BK_ERR_NO_WRITE_PERMISSION    (BK_ERR_OTP_BASE - 4) /**< Not have write privilege */
#define BK_ERR_OTP_PERMISSION_WRONG   (BK_ERR_OTP_BASE - 5) /**< OTP permission value error*/
#define BK_ERR_OTP_NOT_EMPTY          (BK_ERR_OTP_BASE - 6) /**< try to write area already has value*/
#define BK_ERR_OTP_UPDATE_NOT_EQUAL   (BK_ERR_OTP_BASE - 7) /**< after update not equal to expected*/

#define BK_ERR_OTP_OPERATION_ERROR    (BK_ERR_OTP_BASE - 8) /**< otp operation error*/
#define BK_ERR_OTP_OPERATION_WARNING  (BK_ERR_OTP_BASE - 9) /**< otp operation warning*/
#define BK_ERR_OTP_OPERATION_WRONG    (BK_ERR_OTP_BASE - 10) /**< otp operation wrong*/
#define BK_ERR_OTP_OPERATION_FORBID   (BK_ERR_OTP_BASE - 11)/**< otp operation forbid*/
#define BK_ERR_OTP_OPERATION_FAIL     (BK_ERR_OTP_BASE - 12)/** otp operation other error*/
#define BK_ERR_OTP_INIT_FAIL          (BK_ERR_OTP_BASE - 13)/** otp init fail*/
#define BK_ERR_OTP_INDEX_WRONG        (BK_ERR_OTP_BASE - 14) /**< OTP item index error*/

#define BK_ERR_OTP_OPERATION_ERROR_MASK         (1 << 16)
#define BK_ERR_OTP_OPERATION_FAIL_MASK          (1 << 19)
#define BK_ERR_OTP_INIT_FAIL_MASK               (1 << 20)
#define BK_ERR_OTP_FULL_ERROR_MASK              (0x1F << 16)

typedef uint8_t otp_unit_t; /**< otp uint id */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

