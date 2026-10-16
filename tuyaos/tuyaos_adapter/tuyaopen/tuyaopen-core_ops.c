/**
 * @file tuyaopen-core_ops.c
 * @brief tuyaopen-core 的 ops 卡注册：TAL_API_OPS_T / TAL_NETWORK_OPS_T / TAL_NETIF_OPS_T。
 *
 * 分层（C2）：core 的 tal_api.c / tal_network.c / tal_netif.c 经 extern 引用这些卡
 * 实例完成分发；卡成员直接绑定本平台（tuyaos/tuyaos_adapter）的 tkl_* 实现（薄卡）。
 * 契约类型取 tuyaopen/interface/（core 与平台共用同一份定义）。
 *
 * 单独立文件（不写进平台实现文件）：平台侧 src 下各 tkl_*.c 保持原样零改动。
 * 注：netif 卡绑定 tkl_wired_*——本平台 tkl_wired.c 无条件编入（CMakeLists srcs），
 * 故本卡不设门控（LINUX 平台因 tkl_wired.c 受 ENABLE_WIRED 门控而带门控）。
 *
 * @copyright Copyright (c) 2021-2026 Tuya Inc. All Rights Reserved.
 */

#include "tal_api_ops.h"     /* TAL_API_OPS_T */
#include "tal_network_ops.h" /* TAL_NETWORK_OPS_T */
#include "tal_bluetooth_ops.h" /* TAL_BLUETOOTH_OPS_T */
#include "tal_netif_ops.h"   /* TAL_NETIF_OPS_T */

#include "tkl_output.h"
#include "tkl_memory.h"
#include "tkl_system.h"
#include "tkl_semaphore.h"
#include "tkl_mutex.h"
#include "tkl_thread.h"
#include "tkl_queue.h"
#include "tkl_network.h"
#include "tkl_wired.h"

/* ---------------------------------------------------------------------------
 * TAL_API_OPS_T：log / 内存 / system / 信号量 / 互斥 / 线程 / 队列
 * --------------------------------------------------------------------------- */
TAL_API_OPS_T tal_api_ops_platform = {
    //! log output
    .log_output = tkl_log_output,
    //! memory
    .malloc = tkl_system_malloc,
    .free   = tkl_system_free,
    //! system
    .sleep           = tkl_system_sleep,
    .reset           = tkl_system_reset,
    .get_millisecond = tkl_system_get_millisecond,
    .get_random      = tkl_system_get_random,
    //! semaphore
    .semaphore_create_init = tkl_semaphore_create_init,
    .semaphore_wait        = tkl_semaphore_wait,
    .semaphore_post        = tkl_semaphore_post,
    .semaphore_release     = tkl_semaphore_release,
    //! mutex
    .mutex_create_init = tkl_mutex_create_init,
    .mutex_lock        = tkl_mutex_lock,
    .mutex_unlock      = tkl_mutex_unlock,
    .mutex_release     = tkl_mutex_release,
    //! thread
    .thread_create        = tkl_thread_create,
    .thread_release       = tkl_thread_release,
    .thread_set_self_name = tkl_thread_set_self_name,
    .thread_is_self       = tkl_thread_is_self,
    .thread_get_id        = tkl_thread_get_id,
    //! queue
    .queue_create_init = tkl_queue_create_init,
    .queue_post        = tkl_queue_post,
    .queue_fetch       = tkl_queue_fetch,
    .queue_free        = tkl_queue_free,
#if defined(ENABLE_EXT_RAM) && (ENABLE_EXT_RAM == 1)
    //! ex ram（外部 RAM，如 PSRAM）：复用平台既有 tkl_system_psram_*（Linux 无独立外部
    //! RAM，平台实现退化为普通内存）；宏隔离与 core 的 TAL_API_OPS_T 一致
    .ex_ram_malloc = tkl_system_psram_malloc,
    .ex_ram_free   = tkl_system_psram_free,
#endif
};

/* ---------------------------------------------------------------------------
 * TAL_NETWORK_OPS_T：标准网络卡（socket 全量绑定 tkl_net_*）；
 * 局域网/对端成员按 core 侧同名宏门控（core 的开关宏不传播到平台编译单元，
 * 故此处逐个判断）
 * --------------------------------------------------------------------------- */
TAL_NETWORK_OPS_T tal_network_ops_platform = {
    .name = "tkl",
    .type = TAL_NET_TYPE_PLATFORM,
    .ipaddr = 0,
    .ops =
        {
            .get_errno = tkl_net_get_errno,
            .fd_set = tkl_net_fd_set,
            .fd_isset = tkl_net_fd_isset,
            .fd_zero = tkl_net_fd_zero,
            .select = tkl_net_select,
            .get_nonblock = tkl_net_get_nonblock,
            .set_block = tkl_net_set_block,
            .close = tkl_net_close,
            .socket_create = tkl_net_socket_create,
            .connect = tkl_net_connect,
            .bind = tkl_net_bind,
            .send = tkl_net_send,
            .recv = tkl_net_recv,
            .set_timeout = tkl_net_set_timeout,
            .set_reuse = tkl_net_set_reuse,
            .disable_nagle = tkl_net_disable_nagle,
            .gethostbyname = tkl_net_gethostbyname,
            .set_keepalive = tkl_net_set_keepalive,
            .str2addr = tkl_net_str2addr,
            .addr2str = tkl_net_addr2str,
            .setsockopt = tkl_net_setsockopt,
#if defined(ENABLE_LOCAL_LINK) || defined(ENABLE_TUYA_P2P) || defined(ENABLE_AI_COMPONENTS)
            /* 局域网/对端服务成员 */
            .listen = tkl_net_listen,
            .accept = tkl_net_accept,
#endif
#if defined(ENABLE_LOCAL_LINK) || defined(ENABLE_TUYA_P2P)
            .send_to = tkl_net_send_to,
            .recvfrom = tkl_net_recvfrom,
#endif
#if defined(ENABLE_LOCAL_LINK)
            /* 局域网协议专用成员（tuya_lan.c 收帧） */
            .recv_nd_size = tkl_net_recv_nd_size,
#endif
#if defined(ENABLE_TUYA_P2P)
            /* p2p 专用成员 */
            .fd_clear = tkl_net_fd_clear,
            .shutdown = tkl_net_shutdown,
            .set_bufsize = tkl_net_set_bufsize,
            .getsockname = tkl_net_getsockname,
            .getpeername = tkl_net_getpeername,
#endif
        },
};

/* ---------------------------------------------------------------------------
 * TAL_BLUETOOTH_OPS_T：空卡（成员全 NULL）——本仓 T5AI 侧尚未接平台 BT 栈，
 * core 侧分发检测到 NULL 统一优雅降级（OPRT_NOT_SUPPORTED）。
 * 注：本平台配置默认开 ENABLE_BLUETOOTH，core 的 ble/ 会编入并引用本卡。
 * --------------------------------------------------------------------------- */
TAL_BLUETOOTH_OPS_T tal_bluetooth_ops_platform = {
    /* stack */
    .stack_init               = NULL,
    .stack_deinit             = NULL,
    .stack_gatt_link          = NULL,
    /* 事件注册 */
    .gap_callback_register    = NULL,
    .gatt_callback_register   = NULL,
    /* GAP */
    .gap_addr_set             = NULL,
    .gap_address_get          = NULL,
    .gap_adv_start            = NULL,
    .gap_adv_stop             = NULL,
    .gap_adv_rsp_data_set     = NULL,
    .gap_adv_rsp_data_update  = NULL,
    .gap_scan_start           = NULL,
    .gap_scan_stop            = NULL,
    .gap_connect              = NULL,
    .gap_disconnect           = NULL,
    .gap_conn_param_update    = NULL,
    .gap_rssi_get             = NULL,
    /* GATT server */
    .gatts_service_add        = NULL,
    .gatts_value_set          = NULL,
    .gatts_value_notify       = NULL,
    .gatts_exchange_mtu_reply = NULL,
    /* GATT client */
    .gattc_all_service_discovery = NULL,
    .gattc_all_char_discovery    = NULL,
    .gattc_char_desc_discovery   = NULL,
    .gattc_write_without_rsp     = NULL,
    .gattc_read                  = NULL,
    .gattc_exchange_mtu_request  = NULL,
};

/* ---------------------------------------------------------------------------
 * TAL_NETIF_OPS_T：链路状态 + 取 IP（复用 tkl_wired）
 * get_ip 签名逐字一致直接绑定；set_link_cb 需薄包装——core 的
 * TAL_NETIF_LINK_CHANGE_CB(void(*)(TAL_NETIF_LINK_STAT_E)) 与 tkl 的
 * TKL_WIRED_STATUS_CHANGE_CB(void(*)(TKL_WIRED_STAT_E)) 是两个不同 C 类型
 * （值域一致 DOWN=0/UP=1），故存下 core 回调、向 tkl 注册转投 shim。
 * --------------------------------------------------------------------------- */
static TAL_NETIF_LINK_CHANGE_CB s_link_cb;

static void __link_cb_shim(TKL_WIRED_STAT_E status)
{
    if (s_link_cb) {
        s_link_cb((TAL_NETIF_LINK_STAT_E)status);
    }
}

static OPERATE_RET __set_link_cb(TAL_NETIF_LINK_CHANGE_CB cb)
{
    s_link_cb = cb;
    return tkl_wired_set_status_cb(__link_cb_shim);
}

TAL_NETIF_OPS_T tal_netif_ops_platform = {
    .get_ip      = tkl_wired_get_ip,
    .set_link_cb = __set_link_cb,
};
