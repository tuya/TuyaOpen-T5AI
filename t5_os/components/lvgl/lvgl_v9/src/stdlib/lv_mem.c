/**
 * @file lv_mem.c
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_mem.h"
#include "lv_string.h"
#include "../misc/lv_assert.h"
#include "../misc/lv_log.h"
#include "../core/lv_global.h"

#if LV_USE_OS == LV_OS_PTHREAD
    #include <pthread.h>
#endif

#if LV_USE_STDLIB_MALLOC == LV_STDLIB_CUSTOM
    #include <os/mem.h>
#endif

/*********************
 *      DEFINES
 *********************/
/*memset the allocated memories to 0xaa and freed memories to 0xbb (just for testing purposes)*/
#ifndef LV_MEM_ADD_JUNK
    #define LV_MEM_ADD_JUNK  0
#endif

#define zero_mem LV_GLOBAL_DEFAULT()->memory_zero

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  GLOBAL PROTOTYPES
 **********************/
void * lv_malloc_core(size_t size);
void * lv_realloc_core(void * p, size_t new_size);
void lv_free_core(void * p);
void lv_mem_monitor_core(lv_mem_monitor_t * mon_p);
lv_result_t lv_mem_test_core(void);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/
#if LV_USE_LOG && LV_LOG_TRACE_MEM
    #define LV_TRACE_MEM(...) LV_LOG_TRACE(__VA_ARGS__)
#else
    #define LV_TRACE_MEM(...)
#endif

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void * lv_malloc(size_t size)
{
    LV_TRACE_MEM("allocating %lu bytes", (unsigned long)size);
    if(size == 0) {
        LV_TRACE_MEM("using zero_mem");
        return &zero_mem;
    }

#if LV_USE_STDLIB_MALLOC != LV_STDLIB_CUSTOM
    void * alloc = lv_malloc_core(size);
#else
    void * alloc = LV_MEM_CUSTOM_ALLOC(size);
#endif
    if(alloc == NULL) {
        LV_LOG_INFO("couldn't allocate memory (%lu bytes)", (unsigned long)size);
#if LV_LOG_LEVEL <= LV_LOG_LEVEL_INFO
        lv_mem_monitor_t mon;
        lv_mem_monitor(&mon);
        LV_LOG_INFO("used: %zu (%3d %%), frag: %3d %%, biggest free: %zu",
                    mon.total_size - mon.free_size, mon.used_pct, mon.frag_pct,
                    mon.free_biggest_size);
#endif
        return NULL;
    }

#if LV_MEM_ADD_JUNK
    lv_memset(alloc, 0xaa, size);
#endif

    LV_TRACE_MEM("allocated at %p", alloc);
    return alloc;
}

void * lv_malloc_zeroed(size_t size)
{
    LV_TRACE_MEM("allocating %lu bytes", (unsigned long)size);
    if(size == 0) {
        LV_TRACE_MEM("using zero_mem");
        return &zero_mem;
    }

#if LV_USE_STDLIB_MALLOC != LV_STDLIB_CUSTOM
    void * alloc = lv_malloc_core(size);
#else
    void * alloc = LV_MEM_CUSTOM_ALLOC(size);
#endif
    if(alloc == NULL) {
        LV_LOG_INFO("couldn't allocate memory (%lu bytes)", (unsigned long)size);
#if LV_LOG_LEVEL <= LV_LOG_LEVEL_INFO
        lv_mem_monitor_t mon;
        lv_mem_monitor(&mon);
        LV_LOG_INFO("used: %zu (%3d %%), frag: %3d %%, biggest free: %zu",
                    mon.total_size - mon.free_size, mon.used_pct, mon.frag_pct,
                    mon.free_biggest_size);
#endif
        return NULL;
    }

    lv_memzero(alloc, size);

    LV_TRACE_MEM("allocated at %p", alloc);
    return alloc;
}

void lv_free(void * data)
{
    LV_TRACE_MEM("freeing %p", data);
    if(data == &zero_mem) return;
    if(data == NULL) return;

#if LV_USE_STDLIB_MALLOC != LV_STDLIB_CUSTOM
    lv_free_core(data);
#else
    LV_MEM_CUSTOM_FREE(data);
#endif
}

void * lv_realloc(void * data_p, size_t new_size)
{
    LV_TRACE_MEM("reallocating %p with %lu size", data_p, (unsigned long)new_size);
    if(new_size == 0) {
        LV_TRACE_MEM("using zero_mem");
        lv_free(data_p);
        return &zero_mem;
    }

    if(data_p == &zero_mem) return lv_malloc(new_size);

#if LV_USE_STDLIB_MALLOC != LV_STDLIB_CUSTOM
    void * new_p = lv_realloc_core(data_p, new_size);
#else
    void * new_p = LV_MEM_CUSTOM_REALLOC(data_p, new_size);
#endif
    if(new_p == NULL) {
        LV_LOG_ERROR("couldn't reallocate memory");
        return NULL;
    }

    LV_TRACE_MEM("reallocated at %p", new_p);
    return new_p;
}

lv_result_t lv_mem_test(void)
{
    if(zero_mem != ZERO_MEM_SENTINEL) {
        LV_LOG_WARN("zero_mem is written");
        return LV_RESULT_INVALID;
    }

#if LV_USE_STDLIB_MALLOC != LV_STDLIB_CUSTOM
    return lv_mem_test_core();
#else
    return LV_RESULT_OK;
#endif
}

void lv_mem_monitor(lv_mem_monitor_t * mon_p)
{
    lv_memzero(mon_p, sizeof(lv_mem_monitor_t));

#if LV_USE_STDLIB_MALLOC != LV_STDLIB_CUSTOM
    lv_mem_monitor_core(mon_p);
#endif
}

// Modified by TUYA Start
void * lv_psram_mem_alloc(size_t size)
{
    MEM_TRACE("allocating %lu bytes", (unsigned long)size);
    if(size == 0) {
        MEM_TRACE("using zero_mem");
        return &zero_mem;
    }

    void * alloc = psram_malloc(size);
    if(alloc == NULL) {
        LV_LOG_INFO("couldn't allocate memory (%lu bytes)", (unsigned long)size);
#if LV_LOG_LEVEL <= LV_LOG_LEVEL_INFO
        lv_mem_monitor_t mon;
        lv_mem_monitor(&mon);
        LV_LOG_INFO("used: %6d (%3d %%), frag: %3d %%, biggest free: %6d",
                    (int)(mon.total_size - mon.free_size), mon.used_pct, mon.frag_pct,
                    (int)mon.free_biggest_size);
#endif
    }
#if LV_MEM_ADD_JUNK
    else {
        lv_memset(alloc, 0xaa, size);
    }
#endif

    if(alloc) {
        MEM_TRACE("allocated at %p", alloc);
    }
    return alloc;
}

void * lv_psram_mem_realloc(void * data_p, size_t new_size)
{
    MEM_TRACE("reallocating %p with %lu size", data_p, (unsigned long)new_size);
    if(new_size == 0) {
        MEM_TRACE("using zero_mem");
        lv_mem_free(data_p);
        return &zero_mem;
    }

    if(data_p == &zero_mem) return lv_psram_mem_alloc(new_size);

    void * new_p = bk_psram_realloc(data_p, new_size);
    if(new_p == NULL) {
        LV_LOG_ERROR("couldn't allocate memory");
        return NULL;
    }

    MEM_TRACE("allocated at %p", new_p);
    return new_p;
}

lv_res_t lv_mem_test(void)
{
    if(zero_mem != ZERO_MEM_SENTINEL) {
        LV_LOG_WARN("zero_mem is written");
        return LV_RES_INV;
    }

#if LV_MEM_CUSTOM == 0
    if(lv_tlsf_check(tlsf)) {
        LV_LOG_WARN("failed");
        return LV_RES_INV;
    }

    if(lv_tlsf_check_pool(lv_tlsf_get_pool(tlsf))) {
        LV_LOG_WARN("pool failed");
        return LV_RES_INV;
    }
#endif
    MEM_TRACE("passed");
    return LV_RES_OK;
}

// Modified by TUYA End
/**********************
 *   STATIC FUNCTIONS
 **********************/
