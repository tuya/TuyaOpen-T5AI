/**
 * @file tkl_memory.c
 * @brief the default weak implements of tuya hal memory, this implement only used when OS=linux
 * @version 0.1
 * @date 2020-05-15
 * 
 * @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
 * 
 */

#include "tkl_memory.h"
#include <os/mem.h>

extern void bk_printf(const char *fmt, ...);
extern size_t xPortGetPsramFreeHeapSize( void );
/**
* @brief Alloc memory of system
*
* @param[in] size: memory size
*
* @note This API is used to alloc memory of system.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
void* tkl_system_malloc(const SIZE_T size)
{
    void* ptr = os_malloc(size);
    if(NULL == ptr) {
        bk_printf("tkl_system_malloc failed, size(%d)!\r\n", size);
    }

    // bk_printf("cpu%d heap: %d / %d\r\n", CONFIG_CPU_INDEX, tkl_system_get_free_heap_size(), xPortGetMinimumEverFreeHeapSize());

    if (size > 4096) {
        bk_printf("tkl_system_malloc big memory, size(%d)!\r\n", size);
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
    os_free(ptr);
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
void *tkl_system_memset(void* src, int ch, const SIZE_T n)
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
void *tkl_system_memcpy(void* src, const void* dst, const SIZE_T n)
{
    return os_memcpy(src, dst, n);
}

/**
 * @brief Allocate and clear the memory
 * 
 * @param[in]       nitems      the numbers of memory block
 * @param[in]       size        the size of the memory block
 */
VOID_T *tkl_system_calloc(size_t nitems, size_t size)
{
        if (size && nitems > (~(size_t) 0) / size)
                return NULL;

        void *ptr =  os_zalloc(nitems * size);
    if (ptr == NULL) {
        bk_printf("tkl_system_calloc failed, total_size(%d)! nitems = %d size = %d, free: %d psram free: %d\r\n",
                nitems * size,nitems,size, tkl_system_get_free_heap_size(), xPortGetPsramFreeHeapSize());
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
    return os_realloc(ptr, size);
}

#define CONFIG_HAVE_PSRAM 1
/**
* @brief Alloc memory of system
*
* @param[in] size: memory size
*
* @note This API is used to alloc memory of system.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
void* tkl_system_psram_malloc(const SIZE_T size)
{
#if CONFIG_HAVE_PSRAM
    void* ptr = psram_malloc(size);
    if(NULL == ptr) {
        bk_printf("tkl_psram_malloc failed, size(%d)!\r\n", size);
    }

    if (size > 4096) {
        bk_printf("tkl_psram_malloc big memory, size(%d)!\r\n", size);
    }

    return ptr;
#else
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
#endif // CONFIG_HAVE_PSRAM
}

/**
 * @brief Re-allocate the memory
 *
 * @param[in]       nitems      source memory address
 * @param[in]       size        the size after re-allocate
 */
void *tkl_system_psram_realloc(void* ptr, size_t size)
{
#if CONFIG_HAVE_PSRAM 
    void* p_addr = bk_psram_realloc(ptr, size);
    if(NULL == p_addr) {
        bk_printf("tkl_system_psram_realloc failed, size(%d)!\r\n", size);
    }

    return p_addr;
#endif // CONFIG_HAVE_PSRAM
}

