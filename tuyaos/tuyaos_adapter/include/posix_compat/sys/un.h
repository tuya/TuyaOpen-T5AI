/**
 * @file sys/un.h
 * @brief Minimal AF_UNIX definitions for libre's re_sa.h on MCU.
 *
 * The P2P path only uses IPv4 sockets; libre's header references
 * struct sockaddr_un unconditionally. newlib/lwIP on the MCU does not
 * provide sys/un.h, so define the type without an implementation.
 */
#ifndef RE_TUYAOPEN_SYS_UN_H
#define RE_TUYAOPEN_SYS_UN_H

#include <sys/types.h>

#ifndef AF_UNIX
#define AF_UNIX 1
#endif

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 108
#endif

struct sockaddr_un {
    sa_family_t sun_family;
    char sun_path[UNIX_PATH_MAX];
};

#endif
