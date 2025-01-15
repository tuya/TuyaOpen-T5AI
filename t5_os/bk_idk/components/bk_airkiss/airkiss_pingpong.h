#ifndef _AIRKISS_PING_PONG_H_
#define _AIRKISS_PING_PONG_H_

#include "airkiss_main.h"

#define BUF_LEN		              AIRKISS_MIN_RX_BUF_SIZE

#define PINIGPONG_DEBUG           1
#if PINIGPONG_DEBUG
#define PINIGPONG_PRT             os_printf
#define PINIGPONG_WARN            warning_prf
#define PINIGPONG_FATAL           fatal_prf
#else
#define PINIGPONG_PRT             null_prf
#define PINIGPONG_WARN            null_prf
#define PINIGPONG_FATAL           null_prf
#endif

struct buff_str {
	char 	*addr;
	struct buff_str *next_buf_addr;

	uint32_t read_enable;
	uint32_t write_enable;
	uint32_t length;
	uint32_t offset;
	uint32_t actual;
};

struct pingpong_buffer {
	struct buff_str buf1;
	struct buff_str buf2;

	struct buff_str *current_read;
	int read_switch;

	struct buff_str *current_write;
	int write_switch;
};

/* return valule: bytes have read, or 0 if not read enable */
extern int pingpong_init(void);
extern void pingpong_free(void);
extern uint32_t read_from_pingpong_buf(void *buf_user, const uint32_t count, uint32_t *actual);
extern uint32_t write_to_pingpong_buf(void *buf_user, const uint32_t count, const uint32_t actual);
#endif // _AIRKISS_PING_PONG_H_
// eof

