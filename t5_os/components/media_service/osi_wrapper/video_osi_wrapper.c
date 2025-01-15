#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

#include "video_osi_wrapper.h"
#include "avilib_adp.h"

#include <setjmp.h>

#if CONFIG_FATFS
#include "ff.h"
#endif

extern bk_err_t video_osi_funcs_init(void *config);

static void *malloc_wrapper(size_t size)
{
	return os_malloc(size);
}

static void *zalloc_wrapper(size_t num, size_t size)
{
	return os_zalloc(num * size);
}

static void *realloc_wrapper(void *old_mem, size_t size)
{
	return os_realloc(old_mem, size);
}

static void *psram_malloc_wrapper(size_t size)
{
	return psram_malloc(size);
}

static void *psram_zalloc_wrapper(size_t num, size_t size)
{
	return psram_zalloc(num * size);
}

static void *psram_realloc_wrapper(void *old_mem, size_t size)
{
	return bk_psram_realloc(old_mem, size);
}

static void free_wrapper(void *ptr)
{
	os_free(ptr);
}

static void *memcpy_wrapper(void *out, const void *in, uint32_t n)
{
	return os_memcpy(out, in, n);
}

static void memcpy_word_wrapper(void *out, const void *in, uint32_t n)
{
	return os_memcpy_word(out, in, n);
}

static void assert_wrapper(uint8_t expr, char *expr_s, const char *func)
{
	if (!(expr))
	{
		bk_printf("(%s) has assert failed at %s.\n", expr_s, func);
		while (1);
	}
}

static uint32_t get_time_wrapper(void)
{
	return rtos_get_time();
}

static uint32_t f_open_wrapper(void **fp, const void *path, uint8_t mode)
{
#if CONFIG_FATFS
	*fp = os_malloc(sizeof(FIL));
	return f_open((FIL *)*fp, (char *)path, mode);
#else
	return -1;
#endif
}

static uint32_t f_close_wrapper(void *fp)
{
#if CONFIG_FATFS
	FRESULT ret = FR_OK;
	ret = f_close((FIL *)fp);
	os_free(fp);
	return ret;

#else
	return -1;
#endif
}

static uint32_t f_write_wrapper(void *fp, const void *buff, uint32_t btw, uint32_t *bw)
{
#if CONFIG_FATFS
	return f_write((FIL *)fp, (void *)buff, (UINT)btw, (UINT *)bw);
#else
	return -1;
#endif
}

static uint32_t f_read_wrapper(void *fp, const void *buff, uint32_t btr, uint32_t *br)
{
#if CONFIG_FATFS
	return f_read((FIL *)fp, (void *)buff, (UINT)btr, (UINT *)br);
#else
	return -1;
#endif
}

static uint32_t f_lseek_wrapper(void *fp, uint32_t ofs)
{
#if CONFIG_FATFS
	return f_lseek((FIL *)fp, (FSIZE_t)ofs);
#else
	return -1;
#endif
}

static uint32_t f_tell_wrapper(void *fp)
{
#if CONFIG_FATFS
	FIL *tmp_fp = (FIL *)fp;
	return f_tell(tmp_fp);
#else
	return -1;
#endif
}

static uint32_t f_size_wrapper(void *fp)
{
#if CONFIG_FATFS
	FIL *tmp_fp = (FIL *)fp;
	return f_size(tmp_fp);
#else
	return -1;
#endif
}


static uint32_t get_avi_index_start_addr_wrapper(void)
{
	return AVI_INDEX_START_ADDR;
}

static uint32_t get_avi_index_count_wrapper(void)
{
	return AVI_INDEX_COUNT;
}

static bk_video_osi_funcs_t video_osi_funcs =
{
	.malloc = malloc_wrapper,
	.zalloc = zalloc_wrapper,
	.realloc = realloc_wrapper,
	.psram_malloc = psram_malloc_wrapper,
	.psram_zalloc = psram_zalloc_wrapper,
	.psram_realloc = psram_realloc_wrapper,
	.free = free_wrapper,
	.memcpy = memcpy_wrapper,
	.memcpy_word = memcpy_word_wrapper,

	.log_write = bk_printf_ext,
	.assert = assert_wrapper,
	.get_time = get_time_wrapper,

	.f_open = f_open_wrapper,
	.f_close = f_close_wrapper,
	.f_write = f_write_wrapper,
	.f_read = f_read_wrapper,
	.f_lseek = f_lseek_wrapper,
	.f_tell = f_tell_wrapper,
	.f_size = f_size_wrapper,

	.get_avi_index_start_addr = get_avi_index_start_addr_wrapper,
	.get_avi_index_count = get_avi_index_count_wrapper,
};

bk_err_t bk_video_osi_funcs_init(void)
{
	bk_err_t ret = BK_OK;
	ret = video_osi_funcs_init(&video_osi_funcs);
	return ret;
}

