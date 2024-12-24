#include <common/bk_include.h>
#include "airkiss_pingpong.h"
#include <os/mem.h>
#include "bk_uart.h"

static struct pingpong_buffer g_pp_buf = {{0}};

/* return valule: bytes have read, or 0 if not read enable */
uint32_t read_from_pingpong_buf(void *buf_user, const uint32_t count, uint32_t *actual)
{
	uint32_t len;
	uint32_t ret = 0;
	struct buff_str *buf;
	GLOBAL_INT_DECLARATION();
	struct pingpong_buffer *pp_buf = &g_pp_buf;

	if (pp_buf->read_switch) {
		pp_buf->current_read = pp_buf->current_read->next_buf_addr;
		pp_buf->read_switch = 0;
	}

	buf = pp_buf->current_read;

	GLOBAL_INT_DISABLE();
	if (!buf->read_enable) {
		GLOBAL_INT_RESTORE();
		return 0;
	}
	GLOBAL_INT_RESTORE();

	len = min(count, buf->length - buf->offset);

	os_memcpy(buf_user, buf->addr, len);
	ret += len;
	buf->offset += ret;
	*actual = buf->actual;

	GLOBAL_INT_DISABLE();
	/* get to the buffer end, switch to next buffer */
	buf->read_enable = 0;
	buf->write_enable = 1;
	buf->offset = 0;
	pp_buf->read_switch = 1;
	GLOBAL_INT_RESTORE();

	return ret;
}


uint32_t write_to_pingpong_buf(void *buf_user, const uint32_t count, const uint32_t actual)
{
	uint32_t len;
	struct buff_str *buf;
	GLOBAL_INT_DECLARATION();
	struct pingpong_buffer *pp_buf = &g_pp_buf;

	if (pp_buf->write_switch) {
		pp_buf->current_write = pp_buf->current_write->next_buf_addr;
		pp_buf->write_switch = 0;
	}

	buf = pp_buf->current_write;

	GLOBAL_INT_DISABLE();
	if (!buf->write_enable) {
		GLOBAL_INT_RESTORE();
		return 0;
	}
	GLOBAL_INT_RESTORE();

	/* use memset instead write operations for test */
	len = min(BUF_LEN, count);
	os_memcpy(buf->addr, buf_user, len);
	buf->actual = actual;

	/* when write finished, enable write */
	GLOBAL_INT_DISABLE();
	buf->read_enable = 1;
	buf->write_enable = 0;
	pp_buf->write_switch = 1;
	GLOBAL_INT_RESTORE();

	return BUF_LEN;
}

int pingpong_init(void)
{
	char *mem1, *mem2;
	struct buff_str *buf1, *buf2;
	struct pingpong_buffer *pp_buf = &g_pp_buf;

	PINIGPONG_PRT("pingpong_init\r\n");
	buf1 = &pp_buf->buf1;
	buf2 = &pp_buf->buf2;

	mem1 = (char *)os_malloc(BUF_LEN + BUF_LEN);
	if (mem1 == NULL) {
		PINIGPONG_FATAL("pingpong_init() failed\r\n");
		return -1;
	}
	PINIGPONG_PRT("buf1_malloc:0x%x\r\n", mem1);
	mem2 = &mem1[BUF_LEN];

	buf1->addr = mem1;
	buf1->next_buf_addr = buf2;
	buf1->read_enable = 0;
	buf1->write_enable = 1;
	buf1->length = BUF_LEN;
	buf1->offset = 0;

	buf2->addr = mem2;
	buf2->next_buf_addr = buf1;
	buf2->read_enable = 0;
	buf2->write_enable = 1;
	buf2->length = BUF_LEN;
	buf2->offset = 0;

	pp_buf->current_read = buf1;
	pp_buf->read_switch = 0;
	pp_buf->current_write = buf1;
	pp_buf->write_switch = 0;

	return 0;
}

void pingpong_free(void)
{
	struct pingpong_buffer *pp_buf = &g_pp_buf;

	PINIGPONG_PRT("pingpong_free\r\n");
	if (pp_buf->buf1.addr) {
		PINIGPONG_PRT("buf1_free:0x%x\r\n", pp_buf->buf1.addr);
		os_free(pp_buf->buf1.addr);

		pp_buf->buf1.addr = NULL;
		pp_buf->buf1.next_buf_addr = NULL;
	}

	if (pp_buf->buf2.addr) {
		pp_buf->buf2.addr = NULL;
		pp_buf->buf2.next_buf_addr = NULL;
	}
}
// eof

