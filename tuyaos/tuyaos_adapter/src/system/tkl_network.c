#include "tkl_network.h"
#include "tkl_system.h"

#include <assert.h>
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/inet.h"

typedef struct NETWORK_ERRNO_TRANS {
    int sys_err;
    int priv_err;
} NETWORK_ERRNO_TRANS_S;

#define UNW_TO_SYS_FD_SET(fds)  ((fd_set*)fds)

const NETWORK_ERRNO_TRANS_S TKL_ERRNOrans[] = {
    {EINTR, EINTR},
    {EBADF, EBADF},
    {EAGAIN, EAGAIN},
    {EFAULT, EFAULT},
    {EBUSY, EBUSY},
    {EINVAL, EINVAL},
    {ENFILE, ENFILE},
    {EMFILE, EMFILE},
    {ENOSPC, ENOSPC},
    {EPIPE, EPIPE},
    {EWOULDBLOCK, EWOULDBLOCK},
    {ENOTSOCK, ENOTSOCK},
    {ENOPROTOOPT, ENOPROTOOPT},
    {EADDRINUSE, EADDRINUSE},
    {EADDRNOTAVAIL, EADDRNOTAVAIL},
    {ENETDOWN, ENETDOWN},
    {ENETUNREACH, ENETUNREACH},
    {ENETRESET, ENETRESET},
    {ECONNRESET, ECONNRESET},
    {ENOBUFS, ENOBUFS},
    {EISCONN, EISCONN},
    {ENOTCONN, ENOTCONN},
    {ETIMEDOUT, ETIMEDOUT},
    {ECONNREFUSED, ECONNREFUSED},
    {EHOSTDOWN, EHOSTDOWN},
    {EHOSTUNREACH, EHOSTUNREACH},
    {ENOMEM, ENOMEM},
    {EMSGSIZE, EMSGSIZE}
};

extern void bk_printf(const char *fmt, ...);

/**
 * @brief 用于获取错误序号
 *
 * @retval         errno
 */
TUYA_ERRNO tkl_net_get_errno(void)
{
    int i = 0;

    int sys_err = errno;

    for (i = 0; i < (int)sizeof(TKL_ERRNOrans) / sizeof(TKL_ERRNOrans[0]); i++) {
        if (TKL_ERRNOrans[i].sys_err == sys_err) {
            return TKL_ERRNOrans[i].priv_err;
        }
    }

    return -100 - sys_err;
}

// /**
//  * @brief : 用于ip字符串数据转换网络序ip地址(4B)
//  * @param[in]      ip    ip字符串    "192.168.1.1"
//  * @return  ip地址(4B)
//  */
// char* tkl_net_addr(const char *ip)
// {
//     if (ip == NULL) {
//         return NULL;
//     }

//     return inet_addr((char *)ip);
// }

/**
 * @brief : Ascii网络字符串地址转换为主机序(4B)地址
 * @param[in]            ip_str
 * @return   主机序ip地址(4B)
 */
TUYA_IP_ADDR_T tkl_net_str2addr(const char *ip)
{
    if (ip == NULL) {
        return 0xFFFFFFFF;
    }

    TUYA_IP_ADDR_T addr1 = inet_addr((char *)ip);
    TUYA_IP_ADDR_T addr2 = ntohl(addr1);

    return addr2;
}

/**
 * @brief : set fds
 * @param[in]      fd
 * @param[inout]      fds
 * @return  0: success  <0: fail
 */
OPERATE_RET tkl_net_fd_set(const int fd, TUYA_FD_SET_T* fds)
{
    if ((fd < 0) || (fds == NULL)) {
        return -3000 + fd;
    }

    FD_SET(fd, UNW_TO_SYS_FD_SET(fds));

    return OPRT_OK;
}

/**
 * @brief : clear fds
 * @param[in]      fd
 * @param[inout]      fds
 * @return  0: success  <0: fail
 */
OPERATE_RET tkl_net_fd_clear(const int fd, TUYA_FD_SET_T* fds)
{
    if ((fd < 0) || (fds == NULL)) {
        return -3000 + fd;
    }

    FD_CLR(fd, UNW_TO_SYS_FD_SET(fds));

    return OPRT_OK;
}

/**
 * @brief : 判断fds是否被置位
 * @param[in]      fd
 * @param[in]      fds
 * @return  0-没有可读fd other-有可读fd
 */
OPERATE_RET tkl_net_fd_isset(const int fd, TUYA_FD_SET_T* fds)
{
    if ((fd < 0) || (fds == NULL)) {
        return -3000 + fd;
    }

    return FD_ISSET(fd, UNW_TO_SYS_FD_SET(fds));
}

/**
 * @brief : init fds
 * @param[inout]      fds
 * @return  0: success  <0: fail
 */
int tkl_net_fd_zero(TUYA_FD_SET_T *fds)
{
    if (fds == NULL) {
        return 0xFFFFFFFF;
    }

    FD_ZERO(UNW_TO_SYS_FD_SET(fds));

    return OPRT_OK;
}

/**
 * @brief : select
 * @param[in]         maxfd
 * @param[inout]      readfds
 * @param[inout]      writefds
 * @param[inout]      errorfds
 * @param[inout]      ms_timeout
 * @return  0: success  <0: fail
 */
int tkl_net_select(const int maxfd, TUYA_FD_SET_T *readfds, TUYA_FD_SET_T *writefds, TUYA_FD_SET_T *errorfds,
                             const uint32_t ms_timeout)
{
    if (maxfd <= 0) {
        return -3000 + maxfd;
    }

    struct timeval *tmp = NULL;
    struct timeval timeout = {ms_timeout / 1000, (ms_timeout % 1000) * 1000};
    if (0 != ms_timeout) {
        tmp = &timeout;
    } else {
        tmp = NULL;
    }

    return select(maxfd, UNW_TO_SYS_FD_SET(readfds), UNW_TO_SYS_FD_SET(writefds), UNW_TO_SYS_FD_SET(errorfds), tmp);
}

/**
 * @brief : close fd
 * @param[in]      fd
 * @return  0: success  <0: fail
 */
TUYA_ERRNO tkl_net_close(const int fd)
{
    if (fd < 0) {
        return -3000 + fd;
    }

    return close(fd);
}

/**
 * @brief : shutdow fd
 * @param[in]      fd
 * @param[in]      how
 * @return  0: success  <0: fail
*/
TUYA_ERRNO tkl_net_shutdown(const int fd, const int how)
{
    if (fd < 0) {
        return -3000 + fd;
    }

    return shutdown(fd, how);
}

/**
 * @brief : creat fd
 * @param[in]      type
 * @return  >=0: socketfd  <0: fail
*/
int tkl_net_socket_create(const TUYA_PROTOCOL_TYPE_E type)
{
    int fd = -1;

    if (PROTOCOL_TCP == type) {
        fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    } else if (PROTOCOL_RAW == type) {
        fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    } else {
        fd = socket(AF_INET, SOCK_DGRAM, 0);
    }

    bk_printf("tkl_net_socket_create type(%d), fd(%d)!\r\n",type, fd);

    return fd;
}

/**
 * @brief : connect
 * @param[in]      fd
 * @param[in]      addr
 * @param[in]      port
 * @return  0: success  Other: fail
*/
TUYA_ERRNO tkl_net_connect(const int fd, const TUYA_IP_ADDR_T addr, const unsigned short port)
{
    if (fd < 0) {
        return -3000 + fd;
    }

    struct sockaddr_in sock_addr;
    unsigned short tmp_port = port;
    TUYA_IP_ADDR_T tmp_addr = addr;

    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(tmp_port);
    sock_addr.sin_addr.s_addr = htonl(tmp_addr);

    return connect(fd, (struct sockaddr *)&sock_addr, sizeof(struct sockaddr_in));
}

/**
 * @brief : raw connect
 * @param[in]      fd
 * @param[in]      p_socket
 * @param[in]      len
 * @return  0: success  Other: fail
*/
TUYA_ERRNO tkl_net_connect_raw(const int fd, void *p_socket_addr, const int len)
{
    if (fd < 0) {
        return -3000 + fd;
    }

    return connect(fd, (struct sockaddr *)p_socket_addr, len);
}

/**
 * @brief : bind
 * @param[in]      fd
 * @param[in]      addr
 * @param[in]      port
 * @return  0: success  Other: fail
*/
TUYA_ERRNO tkl_net_bind(const int fd, const TUYA_IP_ADDR_T addr, const unsigned short port)
{
    if (fd < 0) {
        return -3000 + fd;
    }

    unsigned short tmp_port = port;
    TUYA_IP_ADDR_T tmp_addr = addr;

    struct sockaddr_in sock_addr;
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(tmp_port);
    sock_addr.sin_addr.s_addr = htonl(tmp_addr);

    return bind(fd, (struct sockaddr *)&sock_addr, sizeof(struct sockaddr_in));
}

/**
 * @brief : bind ip
 * @param[in]            fd
 * @param[inout]         addr
 * @param[inout]         port
 * @return  0: success  <0: fail
*/
OPERATE_RET tkl_net_socket_bind(const int fd, const char *ip)
{
    if (NULL == ip) {
        return -3000;
    }

    struct sockaddr_in addr_client   = {0};
    addr_client.sin_family   = AF_INET;
    addr_client.sin_addr.s_addr      = inet_addr(ip);
    addr_client.sin_port     = 0;    /// 0 表示由系统自动分配端口号

    if (0 != bind(fd, (struct sockaddr *)&addr_client, sizeof(addr_client))) {
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

/**
 * @brief : listen
 * @param[in]      fd
 * @param[in]      backlog
 * @return  0: success  < 0: fail
*/
TUYA_ERRNO tkl_net_listen(const int fd, const int backlog)
{
    if (fd < 0) {
        return -3000 + fd;
    }

    return listen(fd, backlog);
}

/**
 * @brief : accept
 * @param[in]            fd
 * @param[inout]         addr
 * @param[inout]         port
 * @return  >=0: 新接收到的socketfd  others: fail
*/
TUYA_ERRNO tkl_net_accept(const int fd, TUYA_IP_ADDR_T *addr, unsigned short *port)
{
    if (fd < 0) {
        return -3000 + fd;
    }

    struct sockaddr_in sock_addr;
    socklen_t len = sizeof(struct sockaddr_in);
    int cfd = accept(fd, (struct sockaddr *)&sock_addr, &len);
    if (cfd < 0) {
        return OPRT_COM_ERROR;
    }

    if (addr) {
        *addr = ntohl((sock_addr.sin_addr.s_addr));
    }

    if (port) {
        *port = ntohs((sock_addr.sin_port));
    }

    return cfd;
}

/**
 * @brief : send
 * @param[in]      fd
 * @param[in]      buf
 * @param[in]      nbytes
 * @return  nbytes has sended
*/
TUYA_ERRNO tkl_net_send(const int fd, const void *buf, const uint32_t nbytes)
{
    if ((fd < 0) || (buf == NULL) || (nbytes == 0)) {
        return -3000 + fd;
    }

    return send(fd, buf, nbytes, 0);
}

/**
 * @brief : send to
 * @param[in]      fd
 * @param[in]      buf
 * @param[in]      nbytes
 * @param[in]      addr
 * @param[in]      port
 * @return  nbytes has sended
*/
TUYA_ERRNO tkl_net_send_to(const int fd, const void *buf, const uint32_t nbytes, \
                              const TUYA_IP_ADDR_T addr, const uint16_t port)
{
    if ((fd < 0) || (buf == NULL) || (nbytes == 0)) {
        return -3000 + fd;
    }

    unsigned short tmp_port = port;
    TUYA_IP_ADDR_T tmp_addr = addr;

    struct sockaddr_in sock_addr;
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(tmp_port);
    sock_addr.sin_addr.s_addr = htonl(tmp_addr);

    return sendto(fd, buf, nbytes, 0, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
}

/**
 * @brief : recv
 * @param[in]         fd
 * @param[inout]      buf
 * @param[in]         nbytes
 * @return  nbytes has received
 */
TUYA_ERRNO tkl_net_recv(const int fd, void *buf, const uint32_t nbytes)
{
    if ((fd < 0) || (buf == NULL) || (nbytes == 0)) {
        return -3000 + fd;
    }
    int flags = fcntl(fd, F_GETFL, 0);

    int noblock = flags & O_NONBLOCK;

    if (!noblock) {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(fd, &set);
        if (select(fd + 1, &set, NULL, NULL, NULL) < 0) {
            return -1;
        }
    }

    int ret = 0;
    ret = recv(fd,buf,nbytes,0);
    if (ret <= 0) {
        TUYA_ERRNO err = tkl_net_get_errno();
        if (EWOULDBLOCK == err || \
                EINTR == err || \
                EAGAIN == err) {
            tkl_system_sleep(10);
            ret = recv(fd,buf,nbytes,0);
        }
    }

    return ret;
}

/**
 * @brief : Receive enough data to specify
 * @param[in]            fd
 * @param[inout]         buf
 * @param[in]            buf_size
 * @param[in]            nd_size
 * @return  nbytes has received
*/
int tkl_net_recv_nd_size(const int fd, \
                                   void *buf, \
                                   const uint32_t buf_size, \
                                   const uint32_t nd_size)
{
    if ((fd < 0) || (NULL == buf) || (buf_size == 0) || \
            (nd_size == 0) || (buf_size < nd_size)) {
        return -3000 + fd;
    }


    unsigned int rd_size = 0;
    int ret = 0;

    while (rd_size < nd_size) {
        ret = recv(fd, ((uint8_t *)buf + rd_size), nd_size - rd_size, 0);
        if (ret <= 0) {
            TUYA_ERRNO err = tkl_net_get_errno();
            if (EWOULDBLOCK == err || \
                    EINTR == err || \
                    EAGAIN == err) {
                tkl_system_sleep(10);
                continue;
            }

            break;
        }

        rd_size += ret;
    }

    if (rd_size < nd_size) {
        return -2;
    }

    return rd_size;
}


/**
 * @brief : recvfrom
 * @param[in]         fd
 * @param[inout]      buf
 * @param[in]         nbytes
 * @param[inout]         addr
 * @param[inout]         port
 * @return  nbytes has received
 */
TUYA_ERRNO tkl_net_recvfrom(const int fd, \
                               void *buf, \
                               const uint32_t nbytes, \
                               TUYA_IP_ADDR_T *addr, \
                               uint16_t *port)
{
    if ((fd < 0) || (buf == NULL) || (nbytes == 0)) {
        return -3000 + fd;
    }

    struct sockaddr_in sock_addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    int ret = recvfrom(fd, buf, nbytes, 0, (struct sockaddr *)&sock_addr, &addr_len);
    if (ret <= 0) {
        TUYA_ERRNO err = tkl_net_get_errno();
        if (EWOULDBLOCK == err || \
                EINTR == err || \
                EAGAIN == err) {
            tkl_system_sleep(10);
            ret = recvfrom(fd, buf, nbytes, 0, (struct sockaddr *)&sock_addr, &addr_len);
        }
    }

    if (ret <= 0) {
        return ret;
    }

    if (addr) {
        *addr = ntohl(sock_addr.sin_addr.s_addr);
    }

    if (port) {
        *port = ntohs(sock_addr.sin_port);
    }

    return ret;
}

/**
 * @brief : set block block or not
 * @param[in]      fd
 * @param[in]      block
 * @return  0: success  <0: fail
 */
int tkl_net_set_block(const int fd, const bool_t block)
{
    if (fd < 0) {
        return -3000 + fd;
    }

    int flags = fcntl(fd, F_GETFL, 0);
    if (block) {
        flags &= (~O_NONBLOCK);
    } else {
        flags |= O_NONBLOCK;
    }

    if (fcntl(fd, F_SETFL, flags) < 0) {
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

/**
 * @brief : get fd block info
 * @param[in]      fd
 * @return  <0-失败   >0-非阻塞    0-阻塞
*/
int tkl_net_get_nonblock(const int fd)
{
    if (fd < 0) {
        return -3000 + fd;
    }


    if ((fcntl(fd, F_GETFL, 0) & O_NONBLOCK) != O_NONBLOCK) {
        return 0;
    }

    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return 1;
    }

    return 0 ;
}

/**
 * @brief : set timeout
 * @param[in]         fd
 * @param[in]         ms_timeout
 * @param[in]         type
 * @return  0: success  <0: fail
*/
OPERATE_RET tkl_net_set_timeout(const int fd, const int ms_timeout, const TUYA_TRANS_TYPE_E type)
{
    if (fd < 0) {
        return -3000 + fd;
    }

    struct timeval timeout = {ms_timeout / 1000, (ms_timeout % 1000) * 1000};
    int optname = ((type == TRANS_RECV) ? SO_RCVTIMEO : SO_SNDTIMEO);

    if (0 != setsockopt(fd, SOL_SOCKET, optname, (char *)&timeout, sizeof(timeout))) {
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

/**
 * @brief : set buf size
 * @param[in]         fd
 * @param[in]         buf_size
 * @param[in]         type
 * @return  0: success  <0: fail
 */
OPERATE_RET tkl_net_set_bufsize(const int fd, const int buf_size, const TUYA_TRANS_TYPE_E type)
{
    if (fd < 0) {
        return -3000 + fd;
    }

    int size = buf_size;
    int optname = ((type == TRANS_RECV) ? SO_RCVBUF : SO_SNDBUF);

    if (0 != setsockopt(fd, SOL_SOCKET, optname, (char *)&size, sizeof(size))) {
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

/**
 * @brief : set reuse
 * @param[in]         fd
 * @return  0: success  <0: fail
 */
int tkl_net_set_reuse(const int fd)
{
    int ret = 0;
    if (fd < 0) {
        bk_printf("tkl_net_set_reuse err, fd(%d)!\r\n", fd);
        return -3000 + fd;
    }

    int flag = 1;
    ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&flag, sizeof(int));
    if (0 != ret) {
        bk_printf("setsockopt err, ret(%d)!\r\n", ret);
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

/**
 * @brief : disable nagle
 * @param[in]         fd
 * @return  0: success  <0: fail
 */
int tkl_net_disable_nagle(const int fd)
{
    if (fd < 0) {
        return -3000 + fd;
    }

    int flag = 1;
    if (0 != setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&flag, sizeof(int))) {
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

/**
 * @brief : set broadcast
 * @param[in]         fd
 * @return  0: success  <0: fail
 */
int tkl_net_set_boardcast(const int fd)
{
    if (fd < 0) {
        return -3000 + fd;
    }

    int flag = 1;
    if (0 != setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (const char *)&flag, sizeof(int))) {
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

/**
 * @brief : tcp保活设置
 * @param[in]            fd-the socket fd
 * @param[in]            alive-open(1) or close(0)
 * @param[in]            idle-how long to send a alive packet(in seconds)
 * @param[in]            intr-time between send alive packets (in seconds)
 * @param[in]            cnt-keep alive packets fail times to close the connection
 * @return  0: success  <0: fail
 */
int tkl_net_set_keepalive(int fd, const bool_t alive, const uint32_t idle, const uint32_t intr,
                                    const uint32_t cnt)
{
    if (fd < 0) {
        return -3000 + fd;
    }

    int ret = 0;
    int keepalive = alive;
    int keepidle = idle;
    int keepinterval = intr;
    int keepcount = cnt;

    ret |= setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive, sizeof(keepalive));
    ret |= setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, (void *)&keepidle, sizeof(keepidle));
    ret |= setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&keepinterval, sizeof(keepinterval));
    ret |= setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, (void *)&keepcount, sizeof(keepcount));
    if (0 != ret) {
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

/**
 * @brief : dns parse
 * @param[in]            domain
 * @param[inout]         addr
 * @return  0: success  <0: fail
 */
OPERATE_RET tkl_net_gethostbyname(const char *domain, TUYA_IP_ADDR_T *addr)
{
    if ((domain == NULL) || (addr == NULL)) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    struct hostent *h = NULL;
    for (int i = 0; i < 5; i++) {
        h = gethostbyname(domain);
        if (h != NULL) {
            break;
        }
        tkl_system_sleep(20 * (i+1));
    }

    if (h == NULL) {
        return OPRT_COM_ERROR;
    }

    *addr = ntohl(((struct in_addr *)(h->h_addr_list[0]))->s_addr);

    return OPRT_OK;
}

/**
* @brief Change ip address to string
*
* @param[in] ipaddr: ip address
*
* @note This API is used to change ip address(in host byte order) to string(in IPv4 numbers-and-dots(xx.xx.xx.xx) notion).
*
* @return ip string
*/

char* tkl_net_addr2str(const TUYA_IP_ADDR_T ipaddr)
{
#if defined(ENABLE_LWIP) && (ENABLE_LWIP == 1)
    unsigned int addr = lwip_htonl(ipaddr);
    return ip_ntoa((ip_addr_t *) &addr);
#else
    return inet_ntoa(ipaddr);
#endif

}

OPERATE_RET tkl_net_setsockopt(const int fd, const TUYA_OPT_LEVEL level, const TUYA_OPT_NAME optname, const void *optval, const int optlen)
{
    int ret = 0;

    ret |= setsockopt(fd, level, optname, optval, optlen);
    return ret;
}

OPERATE_RET tkl_net_getsockopt(const int fd, const TUYA_OPT_LEVEL level, const TUYA_OPT_NAME optname, void *optval, int *optlen)
{
    int ret = 0;
    ret = getsockopt(fd, level, optname, optval, (socklen_t *)optlen);

    return ret;
}

/**
* @brief Get ip address by socket fd
*
* @param[in] fd: file descriptor
* @param[out] addr: ip address
*
* @note This API is used for getting ip address by socket fd.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_net_get_socket_ip(const int fd, TUYA_IP_ADDR_T *addr)
{
    return OPRT_NOT_SUPPORTED;
}

/**
* @brief Set socket fd close mode
*
* @param[in] fd: file descriptor
*
* @note This API is used for setting socket fd close mode, the socket fd will not be closed in child processes generated by fork calls.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_net_set_cloexec(IN const int fd)
{
    return OPRT_OK;
}

/**
* @brief Get socket name
*
* @param[in] fd: file descriptor
* @param[out] addr: ip address
* @param[out] port: port information
*
* @note This API is used to Get the current name for the specified socket
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_net_getsockname(int fd, TUYA_IP_ADDR_T *addr, uint16_t *port)
{
    return 0;
}

/**
* @brief Enable broadcast option of socket fd
*
* @param[in] fd: file descriptor
*
* @note This API is used to enable broadcast option of socket fd.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_net_set_broadcast(const int fd)
{
    return 0;
}

/**
* @brief Get name of connected peer socket
*
* @param[in] fd: file descriptor
* @param[out] addr: ip address
* @param[out] port: port information
*
* @note This API is used to Get the name of connected peer socket.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_net_getpeername(int fd, TUYA_IP_ADDR_T *addr, uint16_t *port)
{
    return 0;
}
char g_tkl_station_hostname[16] = {0};
/**
* @brief Set the system hostname
*
* @param[in] hostname: hostname to set
*
* @note This API is used to set the system hostname.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_net_sethostname(const char *hostname)
{
    extern int net_dhcp_hostname_set(char *hostname);
    snprintf(g_tkl_station_hostname, sizeof(g_tkl_station_hostname), "%s", hostname);
    net_dhcp_hostname_set(g_tkl_station_hostname);

    return 0;
}


