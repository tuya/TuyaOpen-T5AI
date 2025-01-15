/**
 * @file tkl_output.c
 * @brief the default weak implements of tuya os output, this implement only used when OS=linux
 * @version 0.1
 * @date 2019-08-15
 *
 * @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
 *
 */

#include "tkl_output.h"
#include <components/log.h>

#define MODE_NAME  "tkl"

extern void bk_printf_port(int level, char *tag, const char *fmt, va_list args);

/**
* @brief Output log information
*
* @param[in] str: log information
*
* @note This API is used for outputing log information
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
extern void bk_printf(const char *fmt, ...);
void tkl_log_output(IN const char *str, ...)
{
    if (str == NULL) {
        return;
    }
    extern void bk_printf(const char *fmt, ...);
    bk_printf(str);

//    va_list args;
//    va_start(args, str);
//    bk_printf_port(BK_LOG_ERROR, MODE_NAME, str, args);
//    va_end(args);
}

/**
* @brief Close log port
*
* @param void
*
* @note This API is used for closing log port.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_log_close(void)
{
    return OPRT_OK;
}

/**
* @brief Open log port
*
* @param void
*
* @note This API is used for openning log port.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_log_open(void)
{
    return OPRT_OK;
}


