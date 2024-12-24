/**
 * @file tuya_os_adapt_output.h
 * @brief 日志操作接口
 * 
 * @copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 * 
 */

#ifndef _TUYA_OS_ADAPT_OUTPUT_H
#define _TUYA_OS_ADAPT_OUTPUT_H

#include <stdbool.h>
#include <stdint.h>
#include "tuya_os_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TUYA_OS_ADAPT_PRINT printf
#define TOS_DEBUG(fmt, args...) TUYA_OS_ADAPT_PRINT("[TOS_DBG]" fmt "\r\n", ##args)
#define TOS_NOTICE(fmt, args...) TUYA_OS_ADAPT_PRINT("[TOS_NOTICE]" fmt "\r\n", ##args)
#define TOS_ERR(fmt, args...) TUYA_OS_ADAPT_PRINT("[TOS_ERR]" fmt "\r\n", ##args)
#define TOS_INFO(fmt, args...) TUYA_OS_ADAPT_PRINT("[TOS_INFO]" fmt "\r\n", ##args)
#define TOS_WARN(fmt, args...) TUYA_OS_ADAPT_PRINT("[TOS_WARN]" fmt "\r\n", ##args)

#define TUYA_OS_CLI_TEST
#define TUYA_OS_CLI_ADAPTER_PRINT tkl_log_output
/**
 * @brief tuya_os_adapt_output_log用于输出log信息
 * 
 * @param[in] *str log buffer指针
 * 函数实现在tuya_os_adapt_output.c
 */
void tuya_os_adapt_output_log(const signed char *str);

/**
 * @brief 用于关闭原厂sdk默认的log口
 * 
 */
int tuya_os_adapt_log_close(void);

/**
 * @brief 用于恢复原厂sdk默认的log口
 * 
 */
int tuya_os_adapt_log_open(void);


/* add begin: by sunkz, interface regist */
OPERATE_RET tuya_os_adapt_reg_output_intf(void);
/* add end */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

