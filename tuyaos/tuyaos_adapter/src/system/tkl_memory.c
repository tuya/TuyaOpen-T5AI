/**
 * @file tkl_memory.c
 * @brief the default weak implements of tuya hal memory, this implement only used when OS=linux
 * @version 0.1
 * @date 2020-05-15
 *
 * @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
 *
 */

#include "sdkconfig.h"
#include "tkl_memory.h"
#include <os/mem.h>

#define CONFIG_HAVE_PSRAM 1

extern void *tkl_system_calloc(size_t nitems, size_t size);
extern void *tkl_system_realloc(void* ptr, size_t size);
extern void *tkl_system_psram_malloc(const size_t size);
extern void tkl_system_psram_free(void* ptr);
extern void *pvPortRealloc(void *pv, size_t size);

extern void bk_printf(const char *fmt, ...);

static BOOL_T s_psram_malloc_force = FALSE;
void tkl_system_psram_malloc_force_set(BOOL_T enable)
{
    s_psram_malloc_force = enable;
}

BOOL_T tkl_system_psram_malloc_force_get(void)
{
    return s_psram_malloc_force;
}

/**
* @brief Alloc memory of system
*
* @param[in] size: memory size
*
* @note This API is used to alloc memory of system.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
void* tkl_system_malloc(const size_t size)
{
    // if (size > 4096) {
    //     bk_printf("tkl_system_malloc big memory, size(%d), caller %p\r\n", size, __builtin_return_address(0));
    // }

    void* ptr = NULL;
    if (s_psram_malloc_force) {
        ptr = tkl_system_psram_malloc(size);
    } else {
#if defined(ENABLE_EXT_RAM) && (ENABLE_EXT_RAM == 1)
        ptr = tkl_system_psram_malloc(size);
#else
        ptr = os_malloc(size);
#endif
    }

    if(NULL == ptr) {
        bk_printf("tkl_system_malloc failed, size(%d),remain %d,%d\r\n",
                size,
                tkl_system_get_free_heap_size(),
                tkl_system_psram_get_free_heap_size());
    }
    return ptr;
}

/**
* @brief Free memory of system
*
* @param[in] ptr: memory point
*
* @note This API is used to free memory of system.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
void tkl_system_free(void* ptr)
{
    if (s_psram_malloc_force) {
        tkl_system_psram_free(ptr);
    } else {
        os_free(ptr);
    }
}

/**
* @brief set memory
*
* @param[in] size: memory size
*
* @note This API is used to alloc memory of system.
*
* @return the memory address malloced
*/
void *tkl_system_memset(void* src, int32_t ch, const size_t n)
{
    return os_memset(src, ch, n);
}

/**
* @brief Alloc memory of system
*
* @param[in] size: memory size
*
* @note This API is used to alloc memory of system.
*
* @return the memory address malloced
*/
void *tkl_system_memcpy(void* src, const void* dst, const size_t n)
{
    return os_memcpy(src, dst, n);
}

/**
 * @brief Allocate and clear the memory
 *
 * @param[in]       nitems      the numbers of memory block
 * @param[in]       size        the size of the memory block
 */
void *tkl_system_calloc(size_t nitems, size_t size)
{
    void *ptr = NULL;

    if (size && nitems > (~(size_t) 0) / size) {
        return NULL;
    }

    size_t total = nitems * size;

    if (s_psram_malloc_force) {
        ptr = psram_zalloc(total);
    } else {
#if defined(ENABLE_EXT_RAM) && (ENABLE_EXT_RAM == 1)
        ptr = psram_zalloc(total);
#else
        ptr = os_zalloc(total);
#endif
    }

    if (ptr == NULL) {
        bk_printf("tkl_system_calloc failed, total_size(%d)! nitems = %d size = %d\r\n", total, nitems, size);
    }
    return ptr;
}

/**
 * @brief Re-allocate the memory
 *
 * @param[in]       nitems      source memory address
 * @param[in]       size        the size after re-allocate
 */
void *tkl_system_realloc(void* ptr, size_t size)
{
    if (NULL == ptr) {
        return tkl_system_malloc(size);
    }
    if (0 == size) {
        tkl_system_free(ptr);
        return NULL;
    }

    if (s_psram_malloc_force) {
        return bk_psram_realloc(ptr, size);
    } else {
#if defined(ENABLE_EXT_RAM) && (ENABLE_EXT_RAM == 1)
        /* pvPortRealloc() is pointer-aware: it keeps the block in its
         * current heap, copies min(old, new) and releases the old pointer
         * through the matching heap. */
        return pvPortRealloc(ptr, size);
#else
        return os_realloc(ptr, size);
#endif
    }
}

/**
* @brief Alloc memory of system
*
* @param[in] size: memory size
*
* @note This API is used to alloc memory of system.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
void* tkl_system_psram_malloc(const size_t size)
{
#if CONFIG_HAVE_PSRAM

    // if (size > 20480) {
    //     bk_printf("tkl_system_psram_malloc big memory, size(%d), caller %p\r\n", size, __builtin_return_address(0));
    // }

    void* ptr = psram_malloc(size);
    if(NULL == ptr) {
        bk_printf("tkl_psram_malloc failed, size(%d), remain %d\r\n", size, tkl_system_psram_get_free_heap_size());
    }
    return ptr;
#else
    bk_printf("not support %s\r\n", __func__);
    return NULL;
#endif // CONFIG_HAVE_PSRAM
}

/**
* @brief Free memory of system
*
* @param[in] ptr: memory point
*
* @note This API is used to free memory of system.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
void tkl_system_psram_free(void* ptr)
{
#if CONFIG_HAVE_PSRAM
    psram_free(ptr);
#else
    bk_printf("not support %s\r\n", __func__);
#endif // CONFIG_HAVE_PSRAM
}

/**
 * @brief Allocate and clear the memory in psram
 *
 * @param[in]       nitems      the numbers of memory block
 * @param[in]       size        the size of the memory block
 */
void *tkl_system_psram_calloc(size_t nitems, size_t size)
{
#if CONFIG_HAVE_PSRAM
    if (size && nitems > (~(size_t) 0) / size)
        return NULL;

    void *ptr = psram_zalloc(nitems * size);
    if (ptr == NULL) {
        bk_printf("tkl_system_calloc failed, total_size(%d)! nitems = %d size = %d\r\n", nitems * size,nitems,size);
    }
    return ptr;
#else
    bk_printf("not support %s\r\n", __func__);
    return NULL;
#endif // CONFIG_HAVE_PSRAM
}

/**
 * @brief Re-allocate the memory in psram
 *
 * @param[in]       nitems      source memory address
 * @param[in]       size        the size after re-allocate
 */
void *tkl_system_psram_realloc(void* ptr, size_t size)
{
#if CONFIG_HAVE_PSRAM
    return bk_psram_realloc(ptr, size);
#else
    bk_printf("not support %s\r\n", __func__);
    return NULL;
#endif // CONFIG_HAVE_PSRAM
}

/**
* @brief Get free heap size in psram
*
* @param void
*
* @note This API is used for getting free heap size.
*
* @return size of free heap
*/
int32_t tkl_system_psram_get_free_heap_size(void)
{
#if CONFIG_HAVE_PSRAM
    return (int32_t)xPortGetPsramFreeHeapSize();
#else
    bk_printf("not support %s\r\n", __func__);
    return 0;
#endif // CONFIG_HAVE_PSRAM
}

