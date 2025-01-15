/**
 ******************************************************************************
 * @file    platform_init.c
 * @version V1.0.0
 * @date    05-May-2014
 * @brief   This file provide functions called by _BK_ to drive stm32f2xx
 *          platform: - e.g. power save, reboot, platform initialize
 ******************************************************************************
 *  UNPUBLISHED PROPRIETARY SOURCE CODE
 *  Copyright (c) 2016 BEKEN Inc.
 *
 *  The contents of this file may not be disclosed to third parties, copied or
 *  duplicated in any form, in whole or in part, without the prior written
 *  permission of BEKEN Corporation.
 ******************************************************************************
 */
#include <common/bk_include.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/unistd.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>


#include <os/mem.h>

#include <os/os.h>
#include "common/bk_assert.h"


/************** wrap C library functions **************/
__attribute__((weak)) void *__wrap_malloc(size_t size)
{
	return os_malloc(size);
}

__attribute__((weak)) void *__wrap__malloc_r(void *p, size_t size)
{
	return os_malloc(size);
}

__attribute__((weak)) void __wrap_free(void *pv)
{
	os_free(pv);
}

__attribute__((weak)) void *__wrap_calloc(size_t a, size_t b)
{
	void *pvReturn;

	pvReturn = os_malloc(a * b);
	if (pvReturn)
    {
        os_memset(pvReturn, 0, a*b);
    }

	return pvReturn;
}

__attribute__((weak)) void *__wrap_realloc(void *pv, size_t size)
{
	return os_realloc(pv, size);
}

__attribute__((weak)) void __wrap__free_r(void *p, void *x)
{
	__wrap_free(x);
}

__attribute__((weak)) void *__wrap__realloc_r(void *p, void *x, size_t sz)
{
	return __wrap_realloc(x, sz);
}

__attribute__((weak)) void *__wrap_zalloc(size_t size)
{
	return os_zalloc(size);
}

int __wrap_strlen (char *src)
{
    int ret = 0;

    for (ret = 0; *src; src++)
        ret++;

    return ret;
}

int __wrap_strncmp(const char *s1, const char *s2, size_t n)
{
    BK_ASSERT(s1 && s2); /* ASSERT VERIFIED */

    if(0 == n) return 0;

    while(--n && *s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }

    return *s1 - *s2;
}


bool printf_is_init(void);
int __wrap_printf(const char *fmt, ...)
{

    va_list args;
    if(!printf_is_init())
        return 0;

    va_start(args,fmt);
    shell_log_out_port(LOG_COMMON_MODE, BK_LOG_WARN, NULL, fmt, args);

    va_end(args);

    return 0;
}


int __wrap_iprintf(const char *fmt, ...)
{
    va_list args;
    if(!printf_is_init())
        return 0;

    va_start(args,fmt);
    shell_log_out_port(LOG_COMMON_MODE, BK_LOG_WARN,NULL,fmt,args);

    va_end(args);

    return 0;
}

int __wrap_vprintf(const char *format,va_list args)
{
    char string[128];
    int len;

    len=vsnprintf(string, sizeof(string)-1, format, args);
    string[sizeof(string)-1]=0;

    printf(string);
    return len;
}



int __wrap_viprintf(const char *format,va_list args)
{
    char string[128];
    int len;

    len=vsnprintf(string, sizeof(string)-1, format, args);
    string[sizeof(string)-1]=0;

    printf(string);
    return len;
}

void __wrap___assert_func(const char *file, int line, const char *func, const char *failedexpr)
{
	os_printf("%s %d func %s expr %s\n", file, line, func, failedexpr);
	BK_ASSERT(0); /* ASSERT VERIFIED */
}

unsigned long __wrap_strtoul (const char *nptr, char **endptr, int base)
{
    const char *s = nptr;
    unsigned long result = 0;
    unsigned long cutoff;
    int cutlim;
    int overflow = 0;
    int any = 0;

    if (!nptr)
        return 0;
    // Skip leading whitespace
    while (isspace((unsigned char)*s)) {
        s++;
    }
    // Handle optional sign
    if (*s == '-') {
        s++;
    } else if (*s == '+') {
        s++;
    }
    // Determine the base if it's 0
    if (base == 0) {
        if (*s == '0') {
            if (tolower((unsigned char)s[1]) == 'x') {
                base = 16;
                s += 2;
            } else {
                base = 8;
                s++;
            }
        } else {
            base = 10;
        }
    } else if (base == 16 && *s == '0' && tolower((unsigned char)s[1]) == 'x') {
        s += 2;
    }
    // Set cutoff values to handle overflow
    cutoff = ULONG_MAX / (unsigned long)base;
    cutlim = ULONG_MAX % (unsigned long)base;
    // Convert the number
    for (; *s; s++) {
        int c = *s;
        if (isdigit((unsigned char)c)) {
            c -= '0';
        } else if (isalpha((unsigned char)c)) {
            c = tolower((unsigned char)c) - 'a' + 10;
        } else {
            break;
        }
        if (c >= base) {
            break;
        }
        if (any < 0 || result > cutoff || (result == cutoff && c > cutlim)) {
            overflow = 1;
        } else {
            any = 1;
            result = result * base + c;
        }
    }
    if (any == 0) {
        s = nptr;
    }
    if (endptr != NULL) {
        *endptr = (char *)(any ? s : nptr);
    }
    if (overflow) {
        errno = ERANGE;
        result = ULONG_MAX;
    }

    return result;
}

// eof

