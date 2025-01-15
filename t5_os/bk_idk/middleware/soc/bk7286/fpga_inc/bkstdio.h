#ifndef __BK_STDIO_H__
#define __BK_STDIO_H__

#include <stdint.h>

#define PRINT_UART_PORT     UART0
#define GET_KEY_VALUE()     uart_rx_value
#define SET_KEY_VALUE(v)    uart_rx_value = v
#define BKTRACE0            bk_printf("[TRACE]:%s:%d, %s\n", __FILE__, __LINE__, __func__);

extern volatile char uart_rx_esc;
extern volatile char uart_rx_flag;
extern volatile char uart_rx_done;
extern volatile char uart_rx_value;
extern volatile int  uart_rx_index;
extern unsigned char uart_rx_buf[];

void bk_uart_rx_cmd(char* cmd);
void bk_uart_rx_int_callback(char);
void bk_uart_rx_done_callback(void);

uint8_t bk_uart_input_get(void);
int32_t bk_uart_input_digital_number_get(char* hint);
int32_t bk_uart_input_digital_value_get(char* hint);

void bk_putch(char c);
int  bk_printf(const char *fmt, ...);
int  bk_sprintf(char* buffer, const char* format, ...);
int  bk_sscanf(const char* buffer, const char* format, ...);

#endif//__BK_STDIO_H__
