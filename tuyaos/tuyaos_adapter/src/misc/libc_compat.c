/**
 * @file libc_compat.c
 * @brief newlib gaps on the T5AI toolchain: clock_gettime() and pipe().
 *
 * The P2P ICE stack (libre) calls clock_gettime() from its timer code. newlib
 * declares the function but the Beken toolchain provides no implementation:
 * the monotonic clock comes from tkl_system_get_millisecond(), the wall clock
 * from time().
 *
 * pipe() is only referenced by libre's mqueue path, which P2P does not use;
 * return ENOSYS instead of pulling a VFS-backed pipe into the image.
 *
 * These are platform libc gaps, so they live in the T5AI port rather than in
 * the portable P2P sources.
 *
 * @copyright Copyright (c) 2026 Tuya Inc. All Rights Reserved.
 */
#include <errno.h>
#include <time.h>
#include <unistd.h>

#include "tkl_system.h"

int clock_gettime(clockid_t clock_id, struct timespec *ts)
{
    if (ts == NULL) {
        errno = EINVAL;
        return -1;
    }

#ifdef CLOCK_REALTIME
    if (clock_id == CLOCK_REALTIME) {
        ts->tv_sec = time(NULL);
        ts->tv_nsec = 0;
        return 0;
    }
#endif

    {
        SYS_TIME_T ms = tkl_system_get_millisecond();

        ts->tv_sec = (time_t)(ms / 1000U);
        ts->tv_nsec = (long)((ms % 1000U) * 1000000U);
    }
    return 0;
}

int pipe(int fds[2])
{
    (void)fds;
    errno = ENOSYS;
    return -1;
}

/*
 * lwIP has getaddrinfo() but no gai_strerror(); libre logs the error text on
 * failure. Provide the same fallback the ESP-IDF port uses.
 */
const char *gai_strerror(int errcode)
{
    (void)errcode;
    return "getaddrinfo failed";
}
