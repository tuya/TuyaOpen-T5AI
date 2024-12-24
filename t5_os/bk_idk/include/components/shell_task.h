#ifndef _shell_task_h_
#define _shell_task_h_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

int handle_shell_input(char *inbuf, int in_buf_size, char * outbuf, int out_buf_size);
void shell_task(void *pvParameters);
void create_log_handle_task(void);

#ifdef CONFIG_SYS_CPU0
int shell_cmd_forward(char *cmd, u16 cmd_len);
#endif

int shell_assert_out(bool bContinue, char * format, ...);
int shell_assert_raw(bool bContinue, char * data_buff, u16 data_len);
int shell_trace_out( u32 trace_id, ... );
int shell_spy_out( u16 spy_id, u8 * data_buf, u16 data_len);
int shell_get_cpu_id(void);
int shell_level_check_valid(int level);

#define LOG_COMMON_MODE           0x03   /* could use static or dynamic memory, block mode */
#define LOG_NONBLOCK_MODE         0x02   /* could use static or dynamic memory, non-block mode */
#define LOG_STATIC_BLOCK_MODE     0x01   /* could use static memory, block mode */
#define LOG_STAIC_NONBLOCK_MODE   0x00   /* could use static memory, non-block mode */
void shell_log_out_port(int block_mode, int level, char * prefix, const char * format, va_list ap);
int shell_log_raw_data(const u8 *data, u16 data_len);

int shell_echo_get(void);
void shell_echo_set(int en_flag);
void shell_set_log_level(int level);
int shell_get_log_level(void);
int shell_get_log_statist(u32 * info_list, u32 num);
void print_dynamic_log_info(void);

void shell_log_flush(void);
void shell_set_uart_port(uint8_t uart_port);

void shell_cmd_ind_out(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* _shell_task_h_ */
