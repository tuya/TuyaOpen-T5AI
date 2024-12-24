#pragma once

typedef struct
{
	void *(* malloc)(size_t size);
	void *(* zalloc)(size_t num, size_t size);
	void *(* realloc)(void *old_mem, size_t size);
	void *(* psram_malloc)(size_t size);
	void *(* psram_zalloc)(size_t num, size_t size);
	void *(* psram_realloc)(void *old_mem, size_t size);
	void (* free)(void *ptr);
	void *(* memcpy)(void *out, const void *in, uint32_t n);
	void (* memcpy_word)(void *out, const void *in, uint32_t n);

	void (* log_write)(int level, char *tag, const char *fmt, ...);
	void (* assert)(uint8_t expr, char *expr_s, const char *func);
	uint32_t (* get_time)(void);

	uint32_t (* f_open)(void **fp, const void *path, uint8_t mode);
	uint32_t (* f_close)(void *fp);
	uint32_t (* f_write)(void *fp, const void *buff, uint32_t btw, uint32_t *bw);
	uint32_t (* f_read)(void *fp, const void *buff, uint32_t btr, uint32_t *br);
	uint32_t (* f_lseek)(void *fp, uint32_t ofs);
	uint32_t (* f_tell)(void *fp);
	uint32_t (* f_size)(void *fp);

	uint32_t (* get_avi_index_start_addr)(void);
	uint32_t (* get_avi_index_count)(void);
} bk_video_osi_funcs_t;

bk_err_t bk_video_osi_funcs_init(void);

