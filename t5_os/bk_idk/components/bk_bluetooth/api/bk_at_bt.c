#include <common/bk_typedef.h>

//#include "common.h"
#include <../../../include/os/os.h>

#include <stdio.h>
#include <stdlib.h>
#include <driver/gpio.h>
#include "ethermind_export/bt_defines.h"
#include "atsvr_unite.h"
#include "bk_at_bluetooth.h"
#if CONFIG_BT
#include "bt_include.h"
#include "modules/sbc_encoder.h"
#include "components/bluetooth/bk_dm_bt.h"
#include "components/bluetooth/bk_dm_a2dp.h"
#endif


#if CONFIG_AUD_INTF
#include <driver/sbc_types.h>
//#include <driver/aud_types.h>
//#include <driver/aud.h>
#include <driver/audio_ring_buff.h>
//#include <components/aud_intf.h>
#include <driver/sbc.h>
#endif
#if CONFIG_ARCH_CM33
#include <driver/aon_rtc.h>
#endif
#if CONFIG_ARCH_CM33
#include <driver/aon_rtc.h>
#endif

#if CONFIG_CLI
#include <cli.h>
#endif

#include "components/bluetooth/bk_dm_bluetooth_types.h"
#include "components/bluetooth/bk_dm_gap_bt.h"

#define DM_L2CAP_CMD_ENABLE 1

#if DM_L2CAP_CMD_ENABLE
#include "components/bluetooth/bk_dm_l2cap.h"
#endif

/*global variable defination*/

#define TAG "AT_BLUETOOTH"


#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


#define LOCAL_NAME "soundbar"

#define INQUIRY_LAP 0x9e8B33U
#define INQUIRY_LEN 0x0A

#define CONNECTION_PACKET_TYPE 0xcc18
#define CONNECTION_PAGE_SCAN_REPETITIOIN_MODE 0x01
#define CONNECTION_CLOCK_OFFSET 0x00

#define DISCONNECT_REASON_REMOTE_USER_TERMINATE 0x13
//controller will check reason, now support disconnect reason:
//uint8_t ValidReason[] = {0x05,0x13,0x14,0x15,0x1A,0x29};
#define DISCONNECT_REASON DISCONNECT_REASON_REMOTE_USER_TERMINATE


#define SPP_HANDLE_INVALID                   0xFFU
#define PRINT_FUNC os_printf("%s \n", __func__)
#define SPP_ENABLE_DATA_CNF_LOG
#define SPP_TX_BUFF_SIZE  4096
#define SPP_TX_BUFF_LIST_NUM 2
#define BT_DEVICE_ADDR_ONLY_FRMT_SPECIFIER\
       "ADDR: %02X:%02X:%02X:%02X:%02X:%02X"
#define BT_DEVICE_ADDR_ONLY_SPACED_FRMT_SPECIFIER\
       "ADDR: %02X %02X %02X %02X %02X %02X"
#define BT_DEVICE_ADDR_ONLY_PRINT_STR(ref)\
        (ref)[0U],(ref)[1U],(ref)[2U],(ref)[3U],(ref)[4U],(ref)[5U]
#define BT_DEVICE_ADDR_FRMT_SPECIFIER\
        "ADDR: %02X:%02X:%02X:%02X:%02X:%02X, TYPE: %02X"
#define BT_DEVICE_ADDR_SPACED_FRMT_SPECIFIER\
        "ADDR: %02X %02X %02X %02X %02X %02X, TYPE: %02X"
#define BT_IGNORE_UNUSED_PARAM(v) (void)(v)

#define A2DP_SOURCE_SBC_FRAME_COUNT 5

#define AT_SYNC_CMD_TIMEOUT_MS          4000
#define AT_DISCON_CMD_TIMEOUT_MS        6000

typedef enum{
    STATE_DISCONNECT = 0,
    STATE_CONNECTING,
    STATE_CONNECTED,
    STATE_PROFILE_DISCONNECT,
    STATE_PROFILE_CONNECTED_AS_CLIENT,
    STATE_PROFILE_CONNECTED_AS_SERVER,
}connect_state_s;

typedef struct{
    uint16_t conn_handle;
    uint8_t conn_state;
    bd_addr_t peer_addr;
    uint8_t spp_init;
    SPP_HANDLE client_spp_handle;
    SPP_HANDLE server_spp_handle;
    uint32_t spp_record_handle;
    uint8_t local_server_channel;
    uint8_t peer_server_channel;
    uint8_t tx_confirm;
    uint32_t tx_throught_len;
    uint32_t tx_throught_total_len;
    uint32_t crc;
    uint64_t tx_time;
    uint8_t rx_through;
    uint32_t rx_through_len;
    uint64_t rx_time;
    float speed;
}spp_env_s;


typedef struct
{
    uint8_t inited;
    uint16_t conn_handle;
    bd_addr_t peer_addr;
    uint8_t conn_state;
    uint8_t start_status;
    uint8_t play_status;
    uint32_t mtu;
}a2dp_env_s;


typedef struct
{
    uint8_t type;
    uint16_t len;
    char *data;
} bt_audio_demo_msg_t;


static bt_err_t at_cmd_status = BK_ERR_BT_SUCCESS;

static beken_semaphore_t bt_at_cmd_sema = NULL;

static uint16_t conn_handle = 0xff;
static bk_bt_linkkey_storage_t s_bt_linkkey;

static spp_env_s spp_env;
#if CONFIG_AUD_INTF && CONFIG_A2DP_SOURCE_DEMO
static a2dp_env_s a2dp_env;
static union codec_info s_a2dp_cap_info;

#if CONFIG_FATFS
static beken_timer_t bt_a2dp_source_write_timer;
#endif

static SbcEncoderContext s_sbc_software_encoder_ctx;

static uint8_t mic_sco_data[1000] = {0};
static uint16_t mic_data_count = 0;

#endif

static char *tx_through_cmd_start = "spp tx throught start";
static char *tx_through_cmd_end = "spp tx throught end";
static API_RESULT bt_spp_event_notify_cb
                   (
                       /* IN */  SPP_HANDLE    spp_handle,
                       /* IN */  SPP_EVENTS    spp_event,
                       /* IN */  API_RESULT    status,
                       /* IN */  void *        data,
                       /* IN */  UINT16        data_length
                   );
static int bt_start_inquiry_handle(int sync,int argc, char **argv);
static int bt_create_connection_handle(int sync,int argc, char **argv);
static int bt_disconnect_handle(int sync,int argc, char **argv);
static int bt_spp_init_handle(int sync,int argc, char **argv);
static int bt_spp_connect_handle(int sync,int argc, char **argv);
static int bt_spp_tx_handle(int sync,int argc, char **argv);
static int bt_write_scan_enable_handle(int sync,int argc, char **argv);
static int bt_read_scan_enable_handle(int sync,int argc, char **argv);
static int bt_a2dp_sink_connect_handle(int sync,int argc, char **argv);
static int bt_a2dp_sink_disconnect_handle(int sync,int argc, char **argv);
#ifdef CONFIG_AUD_INTF
static int bt_enable_a2dp_sink_test_handle(int sync,int argc, char **argv);
static int bt_enable_hfp_unit_test_handle(int sync,int argc, char **argv);
static int bt_avrcp_ctrl_handle(int sync,int argc, char **argv);
static int bt_a2dp_sink_set_delay_value_handle( int sync,int argc, char **argv);
static int bt_a2dp_sink_get_delay_value_handle( int sync,int argc, char **argv);
#if CONFIG_A2DP_SOURCE_DEMO
static int bt_enable_a2dp_source_connect_handle(int sync,int argc, char **argv);
static int bt_enable_a2dp_source_disconnect_handle(int sync,int argc, char **argv);

static int bt_enable_a2dp_source_start_handle(int sync,int argc, char **argv);
static int bt_enable_a2dp_source_suspend_handle(int sync,int argc, char **argv);
static int bt_enable_a2dp_source_stop_handle(int sync,int argc, char **argv);

static int bt_enable_a2dp_source_write_test_handle(int sync,int argc, char **argv);
//static int bt_enable_a2dp_source_test_handle(int sync,int argc, char **argv);
//static int bt_enable_a2dp_source_test_stop_handle(int argc, char **argv);
#endif
#endif

static int bt_enable_opp_test_handle(int sync,int argc, char **argv);
static int bt_spp_through_test_handle(int sync,int argc, char **argv);
static void bt_spp_through_poll(char *tx_data);

#if DM_L2CAP_CMD_ENABLE
static int bt_l2cap_init_handle(int sync,int argc, char **argv);
static int bt_l2cap_connect_handle(int sync,int argc, char **argv);
static int bt_l2cap_disconnect_handle(int sync,int argc, char **argv);
static int bt_l2cap_tx_handle(int sync,int argc, char **argv);
static int bt_l2cap_stop_srv_handle(int sync,int argc, char **argv);
static int bt_l2cap_deinit_handle(int sync,int argc, char **argv);
#endif

static int bt_start_inquiry_handle_gap(int sync,int argc, char **argv);
static int bt_create_connection_handle_gap(int sync,int argc, char **argv);
static int bt_disconnect_handle_gap(int sync,int argc, char **argv);
static int bt_write_scan_enable_handle_gap(int sync,int argc, char **argv);
static int bt_read_scan_enable_handle_gap(int sync,int argc, char **argv);


static uint32_t crc_table(uint32_t index);
static uint32_t crc32(uint32_t crc, const uint8_t *buf, int len);
static unsigned int make_crc32_table(void);
static uint32_t calc_crc32(uint32_t crc, const uint8_t *buf, int len);
static void bt_spp_clear();

static int bt_at_findcmd_is_sync(char* name);

extern int bk_rand(void);
static int sema_value;

static int bt_get_addr_from_param(bd_addr_t *bdaddr, char *input_param)
{
    int err = kNoErr;
    uint8_t addr_len = os_strlen(input_param);
    char temp[3];
    uint8_t j = 1;
    uint8_t k = 0;

    if ( addr_len != (BK_BLE_GAP_BD_ADDR_LEN * 2 + 5))
    {
        err = kParamErr;
        return err;
    }

    for (uint8_t i = 0; i < addr_len; i++)
    {
        if (input_param[i] >= '0' && input_param[i] <= '9')
        {
            temp[k] = input_param[i];
            k += 1;
        }
        else if (input_param[i] >= 'a' && input_param[i] <= 'f')
        {
            temp[k] = input_param[i];
            k += 1;
        }
        else if (input_param[i] >= 'A' && input_param[i] <= 'F')
        {
            temp[k] = input_param[i];
            k += 1;
        }
        else if (input_param[i] == ':')
        {
            temp[k] = '\0';
            bdaddr->addr[BK_BLE_GAP_BD_ADDR_LEN - j] = os_strtoul(temp, NULL, 16) & 0xFF;
            k = 0;
            j++;
        }
        else
        {
            err = kParamErr;
            return err;
        }

        if (i == (addr_len - 1))
        {
            temp[k] = '\0';
            bdaddr->addr[BK_BLE_GAP_BD_ADDR_LEN - j] = os_strtoul(temp, NULL, 16) & 0xFF;
            k = 0;
        }
    }

    return kNoErr;
}

static void bt_at_sema_set()
{
    if (bt_at_cmd_sema != NULL)
    {
        sema_value++;
//        os_printf("-->sem set%d\r\n", sema_value);
        rtos_set_semaphore(&bt_at_cmd_sema);
    }
}

static bk_err_t bt_at_sema_get(uint32_t timeout_ms)
{
    bk_err_t re = 1;
    if (bt_at_cmd_sema != NULL)
    {
        re = rtos_get_semaphore(&bt_at_cmd_sema, timeout_ms);
        sema_value--;
    }
//    os_printf("-->sem get%d\r\n", sema_value);
    return re;
}



static uint32_t bt_at_event_cb(bt_event_enum_t event, void *param)
{
    switch (event)
    {
        case BK_DM_BT_EVENT_INQUIRY_RESULT:
            {
                uint8_t *addr = (uint8_t *)param;
                BK_LOGI(TAG,"BT Inquiryed addr: %x %x %x %x %x %x \r\n",*(addr+5), *(addr+4), *(addr+3), *(addr+2),*(addr+1),*(addr));
            }
            break;
        case BK_DM_BT_EVENT_DISCONNECT:
            {
                uint8_t *addr = (uint8_t *)param;
                BK_LOGI(TAG,"Disconnected from %x %x %x %x %x %x \r\n",addr[5],addr[4],addr[3],addr[2],addr[1],addr[0]);
                if (!os_memcmp(addr, spp_env.peer_addr.addr, 6))
                {
                    bt_spp_clear();
                }
            }
            break;
#if 0
        case BK_DM_BT_EVENT_CMD_COMPLETE:
            {
                LOGI("BT Event Complete!!! \r\n");
            }
            break;
#endif
        case BK_DM_BT_EVENT_CONNECTION_COMPLETE:
            {
                uint8_t *addr = (uint8_t *)param;
                BK_LOGI(TAG,"Connected to %02x:%02x:%02x:%02x:%02x:%02x\n",addr[5],addr[4],addr[3],addr[2],addr[1],addr[0]);
            }
            break;

        case BK_DM_BT_EVENT_LINKKEY_NOTIFY:
        {
            bk_bt_linkkey_storage_t *linkkey = (typeof(linkkey))param;

            BK_LOGI(TAG,"%s recv linkkey %02X:%02X:%02X:%02X:%02X:%02X\n", __func__,
                    linkkey->addr[5],
                    linkkey->addr[4],
                    linkkey->addr[3],
                    linkkey->addr[2],
                    linkkey->addr[1],
                    linkkey->addr[0]);

            memcpy(&s_bt_linkkey, linkkey, sizeof(s_bt_linkkey));

        }
            break;

        case BK_DM_BT_EVENT_LINKKEY_REQ:
        {
            uint8_t *addr = (typeof(addr))param;

            if(!memcmp(addr, s_bt_linkkey.addr, sizeof(s_bt_linkkey.addr)))
            {
                BK_LOGI(TAG,"%s found linkkey %02X:%02X:%02X:%02X:%02X:%02X\n", __func__,
                        addr[5],
                        addr[4],
                        addr[3],
                        addr[2],
                        addr[1],
                        addr[0]);

                bk_bt_gap_linkkey_reply(1, &s_bt_linkkey);
            }
            else
            {
                bk_bt_linkkey_storage_t tmp;

                BK_LOGI(TAG,"%s notfound linkkey %02X:%02X:%02X:%02X:%02X:%02X\n", __func__,
                         addr[5],
                         addr[4],
                         addr[3],
                         addr[2],
                         addr[1],
                         addr[0]);

                memset(&tmp, 0, sizeof(tmp));
                memcpy(tmp.addr, addr, sizeof(tmp.addr));

                bk_bt_gap_linkkey_reply(0, &tmp);
            }

        }
            break;

        default:
            break;
    }

    return 0;
}

static void bt_at_cmd_cb(bt_cmd_t cmd, bt_cmd_param_t *param)
{
    at_cmd_status = param->status;
    BK_LOGI(TAG,"%s %d %d\r\n", __func__, cmd, param->status);
    switch (cmd)
    {
        case BT_CMD_CREATE_CONNECT:
            if(param->status == BK_ERR_BT_SUCCESS)
            {
                conn_handle = *((uint16_t *)(param->param));
            }
            break;
        case BT_CMD_DISCONNECT:
            if(conn_handle == spp_env.conn_handle)
            {
              bt_spp_clear();
            }
            conn_handle = 0xff;
            break;
        case BT_CMD_SDP:
            if(at_cmd_status)
            {
                spp_env.peer_server_channel = *((uint8_t *)param->param);
            }
            break;
        case BT_CMD_READ_SCAN_ENABLE:
            {
                uint8_t scan_enable = 0xFF;
                if(param->status == BK_ERR_BT_SUCCESS)
                {
                    scan_enable = *(uint8_t *)(param->param+4);
                }
                BK_LOGI(TAG,"Read scan enable, status:%d, Scan Enable: 0x%x \r\n", param->status, scan_enable);
            }
            break;
        case BT_CMD_WRITE_SCAN_ENABLE:
            BK_LOGI(TAG,"Write scan enable, status:%d \r\n", param->status);
            break;
        default:
            break;
    }
    bt_at_sema_set();
}

static void bt_spp_clear()
{
    spp_env.conn_handle = 0xFF;
    spp_env.conn_state = STATE_DISCONNECT;
    spp_env.crc = 0xFFFFFFFF;
    os_memset(&spp_env.peer_addr, 0, 6);
    spp_env.peer_server_channel = 0xFF;
    spp_env.rx_through = 0;
    spp_env.rx_through_len = 0;
    spp_env.rx_time = 0;
    spp_env.speed = 0;
    spp_env.server_spp_handle = SPP_HANDLE_INVALID;
    spp_env.tx_confirm = 0;
    spp_env.tx_throught_len = 0;
    spp_env.tx_throught_total_len = 0;
    spp_env.tx_time = 0;
}

API_RESULT bt_spp_event_notify_cb
           (
               /* IN */  SPP_HANDLE spp_handle,
               /* IN */  SPP_EVENTS spp_event,
               /* IN */  API_RESULT      status,
               /* IN */  void          * data,
               /* IN */  UINT16          data_length
           )
{
    UINT32 index;
    UCHAR * l_data;
    l_data = (UCHAR*)( data );
#if 0
    LOGI("\n"\
           "SPP HANDLE : %u\n"\
           "EVENT      : %d\n"\
           "RESULT     : 0x%04X\n",
            (unsigned int) spp_handle, spp_event, status);
    if (API_SUCCESS != status)
    {
        LOGE("\nSPP Command failure\n");
        return API_FAILURE;
    }
#endif /* 0 */
    switch(spp_event)
    {
    case SPP_CONNECT_CNF:
        BK_LOGI(TAG,"SPP_CONNECT_CNF -> 0x%04X\n", status);
        BK_LOGI(TAG,"SPP Instance Connected : %u\n",(unsigned int) spp_handle);
        BK_LOGI(TAG,"Remote device " BT_DEVICE_ADDR_ONLY_FRMT_SPECIFIER " \n",
        BT_DEVICE_ADDR_ONLY_PRINT_STR (l_data));
        if (0x00 == status)
        {
            spp_env.conn_state = STATE_PROFILE_CONNECTED_AS_CLIENT;
            bt_at_sema_set();
        }
        break;
    case SPP_CONNECT_IND:
        BK_LOGI(TAG,"SPP_CONNECT_IND -> 0x%04X\n", status);
        BK_LOGI(TAG,"SPP Instance Connected : %u\n",(unsigned int) spp_handle);
        BK_LOGI(TAG,"Remote device " BT_DEVICE_ADDR_ONLY_FRMT_SPECIFIER " \n",
        BT_DEVICE_ADDR_ONLY_PRINT_STR (l_data));

        /* Set the global handle */
        /* g_spp_handle = spp_handle; */
        if(spp_env.conn_state != STATE_PROFILE_CONNECTED_AS_CLIENT || spp_env.conn_state != STATE_PROFILE_CONNECTED_AS_SERVER)
        {
            spp_env.conn_state = STATE_PROFILE_CONNECTED_AS_SERVER;
            uint16_t conn_handle = bk_bt_get_conn_handle(l_data);
            spp_env.conn_handle = conn_handle;
            spp_env.server_spp_handle = spp_handle;
            os_memcpy(&spp_env.peer_addr.addr[0], l_data, 6);
            BK_LOGI(TAG,"Conn Handle: 0x%02x\n, Server spp handle: 0x%02x\n", conn_handle, spp_env.server_spp_handle);
        }
        break;
    case SPP_DISCONNECT_CNF:
        BK_LOGI(TAG,"SPP_DISCONNECT_CNF -> Disconnection Successful\n");
        BK_LOGI(TAG,"Remote device " BT_DEVICE_ADDR_ONLY_FRMT_SPECIFIER " \n",
        BT_DEVICE_ADDR_ONLY_PRINT_STR (l_data));
        break;
    case SPP_DISCONNECT_IND:
        BK_LOGI(TAG,"SPP_DISCONNECT_IND -> Disconnection Successful\n");
        BK_LOGI(TAG,"Remote device " BT_DEVICE_ADDR_ONLY_FRMT_SPECIFIER " \n",
        BT_DEVICE_ADDR_ONLY_PRINT_STR (l_data));
        if(spp_env.conn_state == STATE_PROFILE_CONNECTED_AS_CLIENT || spp_env.conn_state == STATE_PROFILE_CONNECTED_AS_SERVER)
        {
            spp_env.conn_state = STATE_PROFILE_DISCONNECT;
        }
        break;
    case SPP_STOP_CNF:
        BK_LOGI(TAG,"SPP_STOP_CNF -> Stop Successful\n");
        break;
    case SPP_SEND_CNF:
    {
        #if 0
        UCHAR * buffer;
        API_RESULT retval;
        #endif
        BK_LOGI(TAG,"Received spp send cnf\n");
        BK_LOGI(TAG,"   spp handle = %d\n", spp_handle);
//            os_printf("    Buffer = %p\n", l_data);
        BK_LOGI(TAG,"    Actual Data Length = %d\n", data_length);
        if (0x00 != status)
        {
            BK_LOGE(TAG,"status: *** 0x%04X\n", status);
            break;
        }

        if(spp_env.tx_confirm)
        {
            BK_LOGI(TAG,"\n----------------CHAR DUMP-----------------------\n");
            for (index = 0U; index < data_length; index++)
            {
                LOGI("%c ", l_data[index]);
            }
            BK_LOGI(TAG,"\n------------------------------------------------\n");

            spp_env.tx_confirm = 0;
//            os_printf("free buff->%p \r\n", data);
            os_free(data);
            bt_at_sema_set();
            break;
        }

        if(spp_env.tx_throught_total_len > 0)
        {
            spp_env.tx_throught_len += data_length;
            uint64_t current_time = rtos_get_time();
            float spend_time = (float)(current_time - spp_env.tx_time)/1000;

            BK_LOGI(TAG,"---->spend time: %f s\r\n", spend_time);
            float speed = (float)spp_env.tx_throught_len/1024/spend_time;
            spp_env.crc = calc_crc32(spp_env.crc, (uint8_t *)l_data, data_length);
            spp_env.speed = speed;
            BK_LOGI(TAG,"Spp tx length: %d, speed: %.3fKB/s \r\n", spp_env.tx_throught_len, speed);
            if(spp_env.tx_throught_len < spp_env.tx_throught_total_len)
            {
                bt_spp_through_poll(data);
            }else
            {
                bt_at_sema_set();
            }
        }
    }
    break;
    case SPP_RECVD_DATA_IND:
        {
            do{
                if(!spp_env.rx_through)
                {
                    BK_LOGI(TAG,"SPP_RECVD_DATA_IND -> Data received successfully\n");
                    BK_LOGI(TAG,"\n----------------CHAR DUMP-----------------------\n");
                    for (index = 0U; index < data_length; index++)
                    {
                        BK_LOGI(TAG,"%c ", l_data[index]);
                    }
                    BK_LOGI(TAG,"\n------------------------------------------------\n");
                    #if 0
                    LOGI("\n----------------HEX DUMP------------------------\n");
                    for (index = 0U; index < data_length; index++)
                    {
                        LOGI("%02X ", l_data[index]);
                    }
                    LOGI("\n------------------------------------------------\n");
                    #endif
                }else
                {
                    if(data_length == os_strlen(tx_through_cmd_end) && !os_strncmp((char *)l_data, tx_through_cmd_end, data_length))
                    {
                        BK_LOGI(TAG,"\n----------------CHAR DUMP-----------------------\n");
                        for (index = 0U; index < data_length; index++)
                        {
                            BK_LOGI(TAG,"%c ", l_data[index]);
                        }
                        BK_LOGI(TAG,"\n------------------------------------------------\n");
                        break;
                    }
//                    for (index = 0U; index < data_length; index++)
//                    {
    //                    os_printf("0x%02x ", l_data[index]);
//                    }
                    spp_env.rx_through_len+=data_length;
                    spp_env.crc = calc_crc32(spp_env.crc, l_data, data_length);
                    uint64_t current_time = rtos_get_time();
                    float spend_time = (float)(current_time - spp_env.rx_time)/1000;

                    BK_LOGI(TAG,"-----> spend time: %f s\r\n", spend_time);
                    float speed = (float)spp_env.rx_through_len/ 1024 / spend_time;
                    spp_env.speed = speed;
                    BK_LOGI(TAG,"Spp received data len: %d, speed :%.4fKB/s\r\n", spp_env.rx_through_len, speed);
                }
            }while(0);
            if(!os_strncmp((char *)l_data, tx_through_cmd_start, data_length))
            {
                BK_LOGI(TAG,"Spp received spp tx through start cmd \r\n");
                spp_env.crc = 0xffffffff;
                make_crc32_table();
                spp_env.rx_through = 1;
                spp_env.rx_time = rtos_get_time();

            }
            if(!os_strncmp((char *)l_data, tx_through_cmd_end, data_length))
            {
                BK_LOGI(TAG,"Spp received spp tx through end cmd \r\n");
                BK_LOGI(TAG,"Spp tx through test finish, rx total len: %d, crc value: 0x%x, speed: %.4fKB/s \r\n", spp_env.rx_through_len, spp_env.crc, spp_env.speed);
                spp_env.rx_through = 0;
                spp_env.crc = 0xffffffff;
                spp_env.rx_through_len = 0;
                spp_env.rx_time = 0;
                spp_env.speed = 0;
            }
        }
        break;
    default:
        BK_LOGE(TAG,"\nUnknown command type\n");
        break;
    } /* switch */
    BT_IGNORE_UNUSED_PARAM(spp_handle);
    return 0x00;
}

#if CONFIG_AUD_INTF && CONFIG_A2DP_SOURCE_DEMO
static void user_a2dp_connection_change(uint8_t status, uint8_t reason)
{

    if (a2dp_env.conn_state != status)
    {
        a2dp_env.conn_state = status;

        if(status == BK_A2DP_CONNECTION_STATE_CONNECTED || status == BK_A2DP_CONNECTION_STATE_DISCONNECTED)
        {
            if (bt_at_cmd_sema != NULL)
            {
                rtos_set_semaphore( &bt_at_cmd_sema );
            }
        }

    }

    BK_LOGI(TAG,"%s %d %d\n", __func__, status, reason);
}

static void user_a2dp_start_cnf(uint8_t result, uint8_t reason, uint32_t mtu)
{
    BK_LOGI(TAG,"%s %d %d\n", __func__, result, reason);

    if(result == 0 && a2dp_env.start_status == 0 )
    {
        a2dp_env.start_status = 1;
        a2dp_env.mtu = mtu;

        if (bt_at_cmd_sema != NULL)
        {
            rtos_set_semaphore( &bt_at_cmd_sema );
        }
    }
}

static void user_a2dp_suspend_cnf(uint8_t result, uint8_t reason)
{
    BK_LOGI(TAG,"%s %d %d\n", __func__, result, reason);

    if(result == 0 && a2dp_env.start_status == 1 )
    {
        a2dp_env.start_status = 0;

        if (bt_at_cmd_sema != NULL)
        {
            rtos_set_semaphore( &bt_at_cmd_sema );
        }
    }
}

static void user_a2dp_final_cap_select(union codec_info *info)
{
    memcpy(&s_a2dp_cap_info, info, sizeof(*info));
}

const static bt_a2dp_source_cb_t bt_a2dp_source_cb =
{
    .a2dp_connection_change = user_a2dp_connection_change,
    .a2dp_start_cnf = user_a2dp_start_cnf,
    .a2dp_suspend_cnf = user_a2dp_suspend_cnf,
    .a2dp_final_cap_select = user_a2dp_final_cap_select,
};
#endif
#if 0
static uint8_t get_real_idx(const uint8_t idx)
{
    uint8_t at_cmd_cnt = bt_at_cmd_cnt();
    uint8_t real_idx;

    for (real_idx = 0; real_idx < at_cmd_cnt; real_idx++) {
        if (bt_at_cmd_table[real_idx].idx == idx) {
            break;
        }
    }

    return real_idx;
}
#endif
static int bt_start_inquiry_handle(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    int err = kNoErr;
    if (sync)
    {
        err = rtos_init_semaphore(&bt_at_cmd_sema, 1);
        if(err != kNoErr){
            goto error;
        }
    }

    if(bk_bt_get_host_stack_type() == BK_BT_HOST_STACK_TYPE_ETHERMIND)
    {
        bk_bt_gap_set_event_callback(bt_at_event_cb);
        err = bk_bt_inquiry(INQUIRY_LAP, INQUIRY_LEN, 0, bt_at_cmd_cb);
        if(!err)
        {
            atsvr_cmd_rsp_ok();
            if (bt_at_cmd_sema != NULL)
                rtos_deinit_semaphore(&bt_at_cmd_sema);
            return err;
        }else
        {
            goto error;
        }
    }
error:
    atsvr_cmd_rsp_error();
    if (bt_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&bt_at_cmd_sema);
    return err;
}

static int bt_create_connection_handle(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    int err = kNoErr;
    if (sync)
    {
        err = rtos_init_semaphore(&bt_at_cmd_sema, 1);
        if(err != kNoErr){
            goto error;
        }
    }
    if(argc < 1)
    {
        LOGE("Parameters error! \r\n");
        goto error;
    }

    if(bk_bt_get_host_stack_type() == BK_BT_HOST_STACK_TYPE_ETHERMIND)
    {
        bk_bt_gap_set_event_callback(bt_at_event_cb);
        bd_addr_t addr;
        uint8_t allow_role_switch = 0;
        err = bt_get_addr_from_param(&addr, argv[0]);
        if(err) goto error;
        if(argc == 2)
        {
            allow_role_switch = os_strtoul(argv[1], NULL, 16) & 0xFF;
        }
        err = bk_bt_connect(&(addr.addr[0]),
                              CONNECTION_PACKET_TYPE,
                              CONNECTION_PAGE_SCAN_REPETITIOIN_MODE,
                              0,
                              CONNECTION_CLOCK_OFFSET,
                              allow_role_switch,
                              bt_at_cmd_cb);
        if(!err)
        {
			atsvr_cmd_rsp_ok();
            if (bt_at_cmd_sema != NULL)
                rtos_deinit_semaphore(&bt_at_cmd_sema);
            return err;
        }else
        {
            goto error;
        }
    }
error:
    atsvr_cmd_rsp_error();
    if (bt_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&bt_at_cmd_sema);
    return err;
}

static int bt_disconnect_handle(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    int err = kNoErr;
    if (sync)
    {
        err = rtos_init_semaphore(&bt_at_cmd_sema, 1);
        if(err != kNoErr){
            goto error;
        }
    }

    if(argc != 1)
    {
        LOGE("Parameters error! \r\n");
        goto error;
    }

    if(bk_bt_get_host_stack_type() == BK_BT_HOST_STACK_TYPE_ETHERMIND)
    {
        bk_bt_gap_set_event_callback(bt_at_event_cb);
        bd_addr_t addr;
        err = bt_get_addr_from_param(&addr, argv[0]);
        if(err) goto error;

        err = bk_bt_disconnect(&(addr.addr[0]), DISCONNECT_REASON, bt_at_cmd_cb);
        if(bt_at_cmd_sema == NULL) goto error;
        err = bt_at_sema_get(5*1000);
        if(!err)
        {
            atsvr_cmd_rsp_ok();
            if (bt_at_cmd_sema != NULL)
                rtos_deinit_semaphore(&bt_at_cmd_sema);
            return err;
        }else
        {
            goto error;
        }
    }
error:
    atsvr_cmd_rsp_error();
    if (bt_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&bt_at_cmd_sema);
    return err;
}

static int bt_spp_connect_handle(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    int err = kNoErr;
    if (sync)
    {
        err = rtos_init_semaphore(&bt_at_cmd_sema, 1);
        if(err != kNoErr){
            goto error;
        }
    }
    if(argc != 1)
    {
        LOGE("Parameters error! \r\n");
        goto error;
    }
    if(bk_bt_get_host_stack_type() == BK_BT_HOST_STACK_TYPE_ETHERMIND)
    {
        if(spp_env.conn_state == STATE_PROFILE_CONNECTED_AS_CLIENT || spp_env.conn_state == STATE_PROFILE_CONNECTED_AS_SERVER || spp_env.conn_state == STATE_PROFILE_DISCONNECT)
        {
            LOGE("Spp exisit one connection now, please disconnect first!!\r\n");
            goto error;
        }
        bk_bt_gap_set_event_callback(bt_at_event_cb);
        err = bt_get_addr_from_param(&spp_env.peer_addr, argv[0]);
        if(err) goto error;
        if(!spp_env.spp_init)
        {
            bk_bt_spp_init(bt_spp_event_notify_cb);
            spp_env.client_spp_handle = SPP_HANDLE_INVALID;
            bk_bt_spp_start((uint32_t *)&spp_env.client_spp_handle, &spp_env.local_server_channel, &spp_env.spp_record_handle);
            spp_env.spp_init = 1;
            LOGI("Spp init, spp handle: 0x%02x , record handle: 0x%02x  \r\n", spp_env.client_spp_handle, spp_env.spp_record_handle);
        }
        if(spp_env.conn_state != STATE_DISCONNECT)
        {
            LOGE("With remote device is connected, please disconnect first \r\n");
            goto error;
        }
        bk_bt_connect(&(spp_env.peer_addr.addr[0]),
                        CONNECTION_PACKET_TYPE,
                        CONNECTION_PAGE_SCAN_REPETITIOIN_MODE,
                        0,
                        CONNECTION_CLOCK_OFFSET,
                        1,
                        bt_at_cmd_cb);
        if(bt_at_cmd_sema == NULL) goto error;
        err = bt_at_sema_get(10*1000);
        if(err != kNoErr) goto error;
        if(at_cmd_status == 0x00)
        {
            spp_env.conn_state = STATE_CONNECTED;
            spp_env.conn_handle = conn_handle;
        }else
        {
            LOGE("BT Connect fail !!!!!!\r\n");
            goto error;
        }
        bk_bt_sdp(spp_env.conn_handle, &(spp_env.peer_addr.addr[0]), bt_at_cmd_cb);
        err = bt_at_sema_get(10*1000);
        if(err != kNoErr) goto error;
        if(!at_cmd_status) goto error;

        bk_bt_spp_connect(&(spp_env.peer_addr.addr[0]), spp_env.peer_server_channel, (uint32_t)spp_env.client_spp_handle, bt_at_cmd_cb);
        err = bt_at_sema_get(10*1000);
        if(!err)
        {
			atsvr_cmd_rsp_ok();
            if (bt_at_cmd_sema != NULL)
                rtos_deinit_semaphore(&bt_at_cmd_sema);
//            os_printf("------------>spp connect cnf1 %d\r\n", spp_env.conn_state);
            return err;
        }else
        {
            goto error;
        }
    }
error:
    if(spp_env.conn_state == STATE_CONNECTED)
    {
        LOGE("Spp connect fail, disconnect acl link \r\n!!");
        bk_bt_disconnect(&spp_env.peer_addr.addr[0], DISCONNECT_REASON, bt_at_cmd_cb);
    }
	atsvr_cmd_rsp_error();
    os_memset(&spp_env, 0, sizeof(spp_env_s));
    if (bt_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&bt_at_cmd_sema);
    return err;
}

static int bt_spp_tx_handle(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    int err = kNoErr;
    if (sync)
    {
        err = rtos_init_semaphore(&bt_at_cmd_sema, 1);
        if(err != kNoErr){
            goto error;
        }
    }
    if(argc != 1)
    {
        LOGE("Parameters error! \r\n");
        goto error;
    }
    if(bk_bt_get_host_stack_type() == BK_BT_HOST_STACK_TYPE_ETHERMIND)
    {
        bk_bt_gap_set_event_callback(bt_at_event_cb);
        if(spp_env.conn_state != STATE_PROFILE_CONNECTED_AS_CLIENT && spp_env.conn_state != STATE_PROFILE_CONNECTED_AS_SERVER)
        {
            LOGE("Please connet spp first !! \r\n");
            goto error;
        }
        uint16_t tx_len = os_strlen((char *)argv[0]);
        char * tx_data = (char *)os_malloc(tx_len);
//        os_printf("alloc buffer->%p\r\n", tx_data);
        os_memcpy(tx_data, argv[0], tx_len);
        LOGI("spp tx data %d: %s \r\n", tx_len,  argv[0]);
        if(tx_len)
        {
            spp_env.tx_confirm = 1;
            uint32_t spp_hanle  = ((spp_env.conn_state == STATE_PROFILE_CONNECTED_AS_CLIENT) ? spp_env.client_spp_handle : spp_env.server_spp_handle);
            bk_bt_spp_tx(spp_hanle, tx_data, tx_len, NULL);
            err = rtos_get_semaphore(&bt_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
            if(!err)
            {
                return err;
            }else
            {
                if(tx_data)
                {
                    LOGE("spp tx fail, over time !!\r\n");
                    os_free(tx_data);
                    tx_data = NULL;
                }
                goto error;
            }
        }else
        {
            LOGE("Data is null !!\r\n");
            return err;
        }
    }
error:
	atsvr_cmd_rsp_error();
    if (bt_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&bt_at_cmd_sema);
    return err;
}

static void bt_spp_through_poll(char *tx_data)
{
    if(spp_env.tx_throught_len < spp_env.tx_throught_total_len)
    {
        uint32_t remain_len = spp_env.tx_throught_total_len - spp_env.tx_throught_len;
        uint32_t tx_len = (remain_len>=SPP_TX_BUFF_SIZE) ? SPP_TX_BUFF_SIZE:remain_len;
        uint8_t rand = bk_rand() & 0xff;
        os_memset(tx_data, rand, tx_len);
        uint32_t spp_hanle  = ((spp_env.conn_state == STATE_PROFILE_CONNECTED_AS_CLIENT) ? spp_env.client_spp_handle : spp_env.server_spp_handle);
        bk_bt_spp_tx(spp_hanle, tx_data, tx_len, NULL);
    }
}

static int bt_spp_through_test_handle(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    int err = kNoErr;
    if (sync)
    {
        err = rtos_init_semaphore(&bt_at_cmd_sema, 1);
        if(err != kNoErr){
            goto error;
        }
    }

    if(argc != 1)
    {
        LOGE("Parameters error! \r\n");
        goto error;
    }

    if(bk_bt_get_host_stack_type() == BK_BT_HOST_STACK_TYPE_ETHERMIND)
    {
        if(spp_env.conn_state != STATE_PROFILE_CONNECTED_AS_CLIENT && spp_env.conn_state != STATE_PROFILE_CONNECTED_AS_SERVER)
        {
            LOGE("Please connet spp first !! \r\n");
            goto error;
        }
        uint32_t tx_data_length = os_strtoul(argv[0], NULL, 16) & 0xFFFFFFFF;
        LOGI("spp tx throught test, data len:%d \r\n", tx_data_length);
        if(tx_data_length == 0)
        {
            tx_data_length = 0xffff;
            LOGE("data len is 0, set to default, data len:%d \r\n", tx_data_length);
        }
        char *tx_buff = (char *)os_malloc(os_strlen(tx_through_cmd_start));
        os_memcpy(tx_buff, tx_through_cmd_start, os_strlen(tx_through_cmd_start));
//        os_printf("alloc buffer->%p\r\n", tx_buff);
        spp_env.tx_confirm = 1;
        uint32_t spp_hanle  = ((spp_env.conn_state == STATE_PROFILE_CONNECTED_AS_CLIENT) ? spp_env.client_spp_handle : spp_env.server_spp_handle);
        bk_bt_spp_tx(spp_hanle, tx_buff, os_strlen(tx_through_cmd_start), NULL);
        err = bt_at_sema_get(AT_SYNC_CMD_TIMEOUT_MS);
        if(err)
        {
            if(tx_buff)
            {
                LOGE("spp tx through init fail, tx cmd over time !!\r\n");
                os_free(tx_buff);
                tx_buff = NULL;
            }
            goto error;
        }

        spp_env.tx_throught_total_len = tx_data_length;
        spp_env.crc = 0xFFFFFFFF;
        make_crc32_table();
        spp_env.tx_time = rtos_get_time();

        char *tx_data = (char *)os_malloc(SPP_TX_BUFF_SIZE);
        bt_spp_through_poll(tx_data);
        err = bt_at_sema_get(AT_SYNC_CMD_TIMEOUT_MS*tx_data_length);
        os_free(tx_data);
        if(err)
        {
            LOGE("spp tx through tx fail, tx cmd over time !!\r\n");
            goto error;
        }

        tx_buff = (char *)os_malloc(os_strlen(tx_through_cmd_end));
        os_memcpy(tx_buff, tx_through_cmd_end, os_strlen(tx_through_cmd_end));
//        os_printf("alloc buffer->%p\r\n", tx_buff);
        spp_env.tx_confirm = 1;
        bk_bt_spp_tx(spp_hanle, tx_buff, os_strlen(tx_through_cmd_end), NULL);
        err = bt_at_sema_get(AT_SYNC_CMD_TIMEOUT_MS);
        if(err)
        {
            if(tx_buff)
            {
                LOGE("spp tx through end fail, tx cmd over time !!\r\n");
                os_free(tx_buff);
                tx_buff = NULL;
            }
            goto error;
        }
        LOGI("Spp tx thtrouth finish    \n");
        LOGI("TX throuth CRC value: 0x%x , speed: %.3fKB/s \r\n", spp_env.crc, spp_env.speed);
        spp_env.speed = 0;
        spp_env.tx_confirm = 0;
        spp_env.tx_throught_len = 0;
        spp_env.tx_throught_total_len = 0;
        spp_env.tx_time = 0;
        spp_env.crc = 0xffffffff;
        return err;
    }
error:
	atsvr_cmd_rsp_error();
    if (bt_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&bt_at_cmd_sema);
    return err;
}

static int bt_spp_init_handle(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    int err = kNoErr;
    if(bk_bt_get_host_stack_type() == BK_BT_HOST_STACK_TYPE_ETHERMIND)
    {
        //bk_bt_write_scan_enable(3, bt_at_cmd_cb);
        bk_bt_gap_set_visibility(BK_BT_CONNECTABLE, BK_BT_DISCOVERABLE);
        if(!spp_env.spp_init)
        {
            bk_bt_gap_set_event_callback(bt_at_event_cb);
            bk_bt_spp_init(bt_spp_event_notify_cb);
            spp_env.client_spp_handle = SPP_HANDLE_INVALID;
            bk_bt_spp_start((uint32_t *)&spp_env.client_spp_handle, &spp_env.local_server_channel, &spp_env.spp_record_handle);
            spp_env.spp_init = 1;
        }
        LOGI("SPP INIT, SPP__HANDL:0x%x\r\n", spp_env.client_spp_handle);
    }
    return err;
}

static int bt_write_scan_enable_handle(int sync, int argc, char **argv)
{
    PRINT_FUNC;
	int err = kNoErr;
    if (sync)
    {
        err = rtos_init_semaphore(&bt_at_cmd_sema, 1);
        if(err != kNoErr){
            goto error;
        }
    }

    if(argc != 1)
    {
        LOGE("Parameters error! \r\n");
        goto error;
    }

    if(bk_bt_get_host_stack_type() == BK_BT_HOST_STACK_TYPE_ETHERMIND)
    {
        bk_bt_gap_set_event_callback(bt_at_event_cb);
        uint8_t scan_enable = os_strtoul(argv[0], NULL, 10) & 0xFFFFFFFF;
        if(scan_enable > 0x03)
        {
            LOGE("%s para error, scan_enable:%d \r\n", scan_enable);
            goto error;
        }
//        os_printf("%s, %d \r\n", __func__, scan_enable);
        //serr = bk_bt_write_scan_enable(scan_enable, bt_at_cmd_cb);
        //if(err) goto error;

        //err = rtos_get_semaphore(&bt_at_cmd_sema, AT_AT_SYNC_CMD_TIMEOUT_MS);
        err = bk_bt_gap_set_visibility(BK_BT_CONNECTABLE, BK_BT_DISCOVERABLE);
        if(!err)
        {
			atsvr_cmd_rsp_ok();
            if (bt_at_cmd_sema != NULL)
                rtos_deinit_semaphore(&bt_at_cmd_sema);
            return err;
        }else
        {
            goto error;
        }
    }
error:
	atsvr_cmd_rsp_ok();
    if (bt_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&bt_at_cmd_sema);
    return err;
}

static int bt_read_scan_enable_handle(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    int err = kNoErr;
    if (sync)
    {
        err = rtos_init_semaphore(&bt_at_cmd_sema, 1);
        if(err != kNoErr){
            goto error;
        }
    }

    if(bk_bt_get_host_stack_type() == BK_BT_HOST_STACK_TYPE_ETHERMIND)
    {
        bk_bt_gap_set_event_callback(bt_at_event_cb);

        err = bk_bt_read_scan_enable(bt_at_cmd_cb);
        if(err) goto error;

        err = rtos_get_semaphore(&bt_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if(!err)
        {
			atsvr_cmd_rsp_ok();
            if (bt_at_cmd_sema != NULL)
                rtos_deinit_semaphore(&bt_at_cmd_sema);
            return err;
        }else
        {
            goto error;
        }
    }
error:
	atsvr_cmd_rsp_error();
    if (bt_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&bt_at_cmd_sema);
    return err;
}

#ifdef CONFIG_AUD_INTF
static int bt_enable_a2dp_sink_test_handle(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    int err = kNoErr;

#if CONFIG_A2DP_SINK_DEMO
    uint8_t aac_supported = 0;

    if (argc < 1)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    if (os_strcmp(argv[0], "1") == 0)
    {
        aac_supported = 1;
    }
    else if (os_strcmp(argv[0], "0") == 0)
    {
        aac_supported = 0;
    }
    else
    {
        LOGE("the input param is error\n");
        err = kParamErr;
        goto error;
    }

    extern int a2dp_sink_demo_init(uint8_t aac_supported);
    err = a2dp_sink_demo_init(aac_supported);
#else
    err = BK_ERR_NOT_SUPPORT;
#endif

    if (!err)
    {
		atsvr_cmd_rsp_ok();
        return err;
    }
    else
    {
        goto error;
    }

error:
	atsvr_cmd_rsp_error();
    return err;
}

static int bt_enable_hfp_unit_test_handle(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    int err = kNoErr;

#if CONFIG_HFP_HF_DEMO
    uint8_t msbc_supported = 0;

    if (argc < 1)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    if (os_strcmp(argv[0], "1") == 0)
    {
        msbc_supported = 1;
    }
    else if (os_strcmp(argv[0], "0") == 0)
    {
        msbc_supported = 0;
    }
    else
    {
        LOGE("the input param is error\n");
        err = kParamErr;
        goto error;
    }

    extern int hfp_hf_demo_init(uint8_t msbc_supported);
    err = hfp_hf_demo_init(msbc_supported);
#else
    err = BK_ERR_NOT_SUPPORT;
#endif

    if (!err)
    {
        atsvr_cmd_rsp_ok();
        return err;
    }
    else
    {
        goto error;
    }

error:
	atsvr_cmd_rsp_error();
    if (bt_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&bt_at_cmd_sema);
    return err;
}
#endif

static int bt_a2dp_sink_connect_handle(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    int err = kNoErr;
    bd_addr_t remote_bdaddr;

    err = bt_get_addr_from_param(&remote_bdaddr, argv[0]);
    if (err) goto error;

    err = bk_bt_a2dp_sink_connect(remote_bdaddr.addr);

    if (err)
    {
        LOGE("%s a2dp sink connect err %d\n", __func__, err);
        goto error;
    }
    else
    {
        atsvr_cmd_rsp_ok();
        return err;
    }

error:
	atsvr_cmd_rsp_error();
    return err;
}

static int bt_a2dp_sink_disconnect_handle(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    int err = kNoErr;
    bd_addr_t remote_bdaddr;

    err = bt_get_addr_from_param(&remote_bdaddr, argv[0]);

    if (err) goto error;

    err = bk_bt_a2dp_sink_disconnect(remote_bdaddr.addr);

    if(err)
    {
        LOGE("%s a2dp sink disconnect err %d\n", __func__, err);
        goto error;
    }
    else
    {
        atsvr_cmd_rsp_ok();
        return err;
    }

error: 
    atsvr_cmd_rsp_error();
    return err;
}

#ifdef CONFIG_AUD_INTF
static int bt_avrcp_ctrl_handle(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    int err = kNoErr;

    if (argc < 1)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

#if CONFIG_A2DP_SINK_DEMO
    if (os_strcmp(argv[0], "play") == 0)
    {
        void bk_bt_app_avrcp_ct_play(void);
        bk_bt_app_avrcp_ct_play();
    }
    else if (os_strcmp(argv[0], "pause") == 0)
    {
        void bk_bt_app_avrcp_ct_pause(void);
        bk_bt_app_avrcp_ct_pause();
    }
    else if (os_strcmp(argv[0], "next") == 0)
    {
        void bk_bt_app_avrcp_ct_next(void);
        bk_bt_app_avrcp_ct_next();
    }
    else if (os_strcmp(argv[0], "prev") == 0)
    {
        void bk_bt_app_avrcp_ct_prev(void);
        bk_bt_app_avrcp_ct_prev();
    }
    else if (os_strcmp(argv[0], "rewind") == 0)
    {
        void bk_bt_app_avrcp_ct_rewind(void);
        bk_bt_app_avrcp_ct_rewind();
    }
    else if (os_strcmp(argv[0], "fast_forward") == 0)
    {
        void bk_bt_app_avrcp_ct_fast_forward(void);
        bk_bt_app_avrcp_ct_fast_forward();
    }
    else if (os_strcmp(argv[0], "vol_up") == 0)
    {
        void bk_bt_app_avrcp_ct_vol_up(void);
        bk_bt_app_avrcp_ct_vol_up();
    }
    else if (os_strcmp(argv[0], "vol_down") == 0)
    {
        void bk_bt_app_avrcp_ct_vol_down(void);
        bk_bt_app_avrcp_ct_vol_down();
    }
    else
    {
        LOGE("the input param is error\n");
        err = kParamErr;
        goto error;
    }
#else
    err = BK_ERR_NOT_SUPPORT;
#endif

    if (!err)
    {
        atsvr_cmd_rsp_ok();
        return err;
    }
    else
    {
        goto error;
    }

error:
    atsvr_cmd_rsp_error();
    return err;
}

static int bt_a2dp_sink_set_delay_value_handle(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    int err = kNoErr;
    uint16_t delay_value;

    delay_value = os_strtoul(argv[0], NULL, 10) & 0xFFFF;

    err = bk_bt_a2dp_sink_set_delay_value(delay_value);

    if (err)
    {
        LOGE("%s a2dp sink set delay value err %d\n", __func__, err);
        goto error;
    }
    else
    {
		atsvr_cmd_rsp_ok();
        return err;
    }

error:
	atsvr_cmd_rsp_error();
    return err;
}

static int bt_a2dp_sink_get_delay_value_handle(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    int err = kNoErr;

    err = bk_bt_a2dp_sink_get_delay_value();

    if (err)
    {
        LOGE("%s a2dp sink get delay value err %d\n", __func__, err);
        goto error;
    }
    else
    {
		atsvr_cmd_rsp_ok();
        return err;
    }

error:
	atsvr_cmd_rsp_error();
    return err;
}

#endif

#if CONFIG_FATFS
#include "ff.h"
#include "test_fatfs.h"
#endif

bt_opp_server_cb_t bt_opp_server_cb;
#if CONFIG_FATFS
static FIL opp_file;
static char opp_file_name[100] = {0};
static uint8_t opp_file_is_open = 0;
#endif

void bt_opp_server_push_started(uint8_t *obj_name, uint16_t len)
{
    LOGI("OPP Push Object Name: %s, len %d\r\n", obj_name, len);

#if CONFIG_FATFS
    os_memset(opp_file_name, 0, sizeof(opp_file_name));

    sprintf(opp_file_name, "1:/%s", obj_name);
    FRESULT fr = f_open(&opp_file, opp_file_name, FA_CREATE_ALWAYS | FA_WRITE);
    if (fr != FR_OK)
    {
        LOGE("open %s fail, error = %d.\r\n", opp_file_name, fr);
    }
    else
    {
        opp_file_is_open = 1;
    }
#endif
}

void bt_opp_server_push_finished(void)
{
    LOGI("OPP Push Object Finish!\r\n");

#if CONFIG_FATFS
    if (opp_file_is_open)
    {
        FRESULT fr = f_close(&opp_file);
        if (fr != FR_OK)
        {
            LOGE("close %s fail, error = %d.\r\n", opp_file_name, fr);
        }

        opp_file_is_open = 0;
    }
#endif
}

void bt_opp_server_push_data_ind(uint8_t *data, uint16_t data_len)
{
#if CONFIG_FATFS
    if (opp_file_is_open)
    {
        uint32 bw = 0;
        /* write data to file */
        FRESULT fr = f_write(&opp_file, (void *)data, data_len, &bw);
        if (fr != FR_OK)
        {
            LOGE("write %s fail, error = %d.\r\n", opp_file_name, fr);
        }
    }
#endif
}

static int bt_enable_opp_test_handle(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    int err = kNoErr;

    uint8_t role = 0;

    if (argc < 1)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    if (os_strcmp(argv[0], "1") == 0)
    {
        role = 1;//server
    }
    else if (os_strcmp(argv[0], "0") == 0)
    {
        role = 0;//client
    }
    else
    {
        LOGE("the input param is error\n");
        err = kParamErr;
        goto error;
    }

    err = rtos_init_semaphore(&bt_at_cmd_sema, 1);
    if (err != kNoErr){
        goto error;
    }

    if (bk_bt_get_host_stack_type() == BK_BT_HOST_STACK_TYPE_ETHERMIND)
    {
       err = bk_bt_gap_set_visibility(BK_BT_CONNECTABLE, BK_BT_DISCOVERABLE);

        if (!err)
        {
            if (role)
            {
                bt_opp_server_cb.push_started = bt_opp_server_push_started;
                bt_opp_server_cb.push_finished = bt_opp_server_push_finished;
                bt_opp_server_cb.push_data_ind = bt_opp_server_push_data_ind;
                err = bk_bt_opp_server_init(&bt_opp_server_cb);
            }
            else
            {
                //client todo
            }

            if (!err)
            {
				atsvr_cmd_rsp_ok();
                if (bt_at_cmd_sema != NULL)
                    rtos_deinit_semaphore(&bt_at_cmd_sema);
                return err;
            }
            else
            {
                goto error;
            }
        }
        else
        {
            goto error;
        }

    }
error:
	atsvr_cmd_rsp_error();
    if (bt_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&bt_at_cmd_sema);
    return err;
}

#if CONFIG_AUD_INTF && CONFIG_A2DP_SOURCE_DEMO

#define A2DP_SOURCE_WRITE_AUTO_TIMER_MS 30
static beken_time_t s_last_get_sample_time = 0;
static uint32_t s_remain_send_sample_count = 0;
static uint32_t s_remain_send_sample_miss_num = 0;

typedef struct
{
    uint8_t type;
    uint16_t len;
    char *data;
} bt_a2dp_source_msg_t;

enum
{
    BT_A2DP_SOURCE_MSG_NULL = 0,
    BT_A2DP_SOURCE_START_MSG = 1,
    BT_A2DP_SOURCE_STOP_MSG = 2,
};

beken_queue_t bt_a2dp_source_msg_que = NULL;
beken_thread_t bt_a2dp_source_thread_handle = NULL;
#if CONFIG_FATFS
static FIL pcm_file_fd;
#endif

static int bt_enable_a2dp_source_connect_handle(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    int err = kNoErr;
    if (sync)
    {
        err = rtos_init_semaphore(&bt_at_cmd_sema, 1);
        if(err != kNoErr){
            LOGE("%s init sem err %d\n", __func__, err);
            goto error;
        }
    }
    if(bk_bt_get_host_stack_type() == BK_BT_HOST_STACK_TYPE_ETHERMIND)
    {
        bk_bt_gap_set_event_callback(bt_at_event_cb);

        err = bt_get_addr_from_param(&a2dp_env.peer_addr, argv[0]);

        if(err)
        {
          LOGE("%s bt_get_addr_from_param err %d\n", __func__, err);
          goto error;
        }
        if(!a2dp_env.inited)
        {
            //bk_bt_a2dp_source_init((void *)&bt_a2dp_source_cb);
            (void)bt_a2dp_source_cb;
            //bk_bt_a2dp_source_register_callback(NULL);
            a2dp_env.inited = 1;
        }

        if(a2dp_env.conn_state != BK_A2DP_CONNECTION_STATE_DISCONNECTED)
        {
            LOGE("With remote device is not idle, please disconnect first\n");
            goto error;
        }

//        a2dp_env.conn_state = STATE_CONNECTING;

        bk_bt_connect(&(a2dp_env.peer_addr.addr[0]),
                        CONNECTION_PACKET_TYPE,
                        CONNECTION_PAGE_SCAN_REPETITIOIN_MODE,
                        0,
                        CONNECTION_CLOCK_OFFSET,
                        1,
                        bt_at_cmd_cb);

        if(bt_at_cmd_sema == NULL)
		{
			LOGE("%s bt_at_cmd_sema null\n", __func__);
			goto error;
		}

        err = rtos_get_semaphore(&bt_at_cmd_sema, 12 * 1000);

        if(err != kNoErr)
		{
			LOGE("%s rtos_get_semaphore err %d\n", __func__, err);
			goto error;
		}

        if(at_cmd_status == 0x00)
        {
//            a2dp_env.conn_state = STATE_CONNECTED;
            a2dp_env.conn_handle = conn_handle;
        }

//        bk_bt_sdp(a2dp_env.conn_handle, &(a2dp_env.peer_addr.addr[0]), bt_at_cmd_cb);
//
//        err = rtos_get_semaphore(&bt_at_cmd_sema, 60 * 1000);
//
//        if(err != kNoErr) goto error;
//
//        if(!at_cmd_status) goto error;

        err = bk_bt_a2dp_source_connect(a2dp_env.peer_addr.addr);

        if(err)
        {
            LOGE("%s connect a2dp err %d\n", __func__, err);
            goto error;
        }

        err = rtos_get_semaphore(&bt_at_cmd_sema, 6 * 1000);

        if(!err)
        {
//            spp_env.conn_state = STATE_PROFILE_CONNECTED;
              atsvr_cmd_rsp_ok();
            if (bt_at_cmd_sema != NULL)
                rtos_deinit_semaphore(&bt_at_cmd_sema);
            return err;
        }
        else
        {
            goto error;
        }
    }
error:
	atsvr_cmd_rsp_error();
//    os_memset(&spp_env, 0, sizeof(spp_env_s));
    if (bt_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&bt_at_cmd_sema);
    return err;
}

static int bt_enable_a2dp_source_disconnect_handle(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    int err = kNoErr;
    if (sync)
    {
        err = rtos_init_semaphore(&bt_at_cmd_sema, 1);
        if(err != kNoErr){
            goto error;
        }
    }
    if(bk_bt_get_host_stack_type() == BK_BT_HOST_STACK_TYPE_ETHERMIND)
    {
        bk_bt_gap_set_event_callback(bt_at_event_cb);

        err = bt_get_addr_from_param(&a2dp_env.peer_addr, argv[0]);

        if(err) goto error;

        if(a2dp_env.conn_state != BK_A2DP_CONNECTION_STATE_CONNECTED)
        {
            LOGE("With remote device is not connected\n");
            goto error;
        }

        err = bk_bt_a2dp_source_disconnect(a2dp_env.peer_addr.addr);

        if(err)
        {
            LOGE("%s disconnect a2dp err %d\n", __func__, err);
            goto error;
        }

        err = rtos_get_semaphore(&bt_at_cmd_sema, 6 * 1000);

        if(err)
        {
            LOGE("%s disconnect a2dp timeout %d\n", __func__, err);
            goto error;
        }

        err = bk_bt_disconnect(a2dp_env.peer_addr.addr, DISCONNECT_REASON_REMOTE_USER_TERMINATE, bt_at_cmd_cb);

        if(err)
        {
            LOGE("%s disconnect link err %d\n", __func__, err);
            goto error;
        }

        err = rtos_get_semaphore(&bt_at_cmd_sema, 6 * 1000);

        if(!err)
        {
//            spp_env.conn_state = STATE_PROFILE_CONNECTED;
			atsvr_cmd_rsp_ok();

            os_memset(&a2dp_env, 0, sizeof(a2dp_env));

            if (bt_at_cmd_sema != NULL)
                rtos_deinit_semaphore(&bt_at_cmd_sema);
            return err;
        }
        else
        {
            goto error;
        }
    }
error:
	atsvr_cmd_rsp_error();
//    os_memset(&spp_env, 0, sizeof(spp_env_s));
    if (bt_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&bt_at_cmd_sema);
    return err;
}

static int bt_a2dp_source_mic_data_handler(uint8_t *data, unsigned int len)//640
{
//    uint8_t sent_threshold = ((CODEC_VOICE_MSBC == bt_audio_hfp_codec) ? SCO_MSBC_SAMPLES_PER_FRAME * 2 : SCO_CVSD_SAMPLES_PER_FRAME * 2);
    uint32_t sent_threshold = s_sbc_software_encoder_ctx.pcm_length * 2; //pcm_length == 128
    uint32_t index = 0;
    int32_t encode_len = 0;
    bt_err_t ret = 0;

    static uint32_t last_len = 0;
    //LOGE("[send_mic_data_to_air]  %d \r\n",len);
    if(((uint32_t)data) % 2)
    {
        LOGE("%s err data is not 2 bytes aligned !!!\n", __func__);
    }

    if(last_len != len)
    {
        LOGE("%s len %d\n", __func__, len);
        last_len = len;
    }

    if(len > sizeof(mic_sco_data))
    {
        LOGE("%s too long %d !\n", __func__, len);
        return 0;
    }

    os_memcpy(&mic_sco_data[mic_data_count], data, len);
    mic_data_count += len;

    for (uint32_t i = 0; i < mic_data_count / sent_threshold; ++i)
    {
        encode_len = sbc_encoder_encode(&s_sbc_software_encoder_ctx, (const int16_t *)(mic_sco_data + i * sent_threshold));
        if(encode_len < 0)
        {
            LOGE("%s encode err %d\n", __func__, encode_len);
            mic_data_count = 0;
            return 0;
        }

        ret = bk_bt_a2dp_source_write(&(a2dp_env.peer_addr), 1, s_sbc_software_encoder_ctx.stream, encode_len);

        if(ret)
        {
            LOGE("%s bk_bt_a2dp_source_write err %d\n", __func__, ret);
        }

    }

    os_memmove(mic_sco_data, &mic_sco_data[(mic_data_count / sent_threshold) * sent_threshold], mic_data_count % sent_threshold);

    mic_data_count = mic_data_count % sent_threshold;

    return 0;

    while (mic_data_count >= sent_threshold)
    {
        encode_len = sbc_encoder_encode(&s_sbc_software_encoder_ctx, (const int16_t *)(mic_sco_data + index));
        if(encode_len < 0)
        {
            LOGE("%s encode err %d\n", __func__, encode_len);
            mic_data_count = 0;
            return 0;
        }

        mic_data_count -= sent_threshold;
        index += sent_threshold;


        ret = bk_bt_a2dp_source_write(&(a2dp_env.peer_addr), 1, s_sbc_software_encoder_ctx.stream, encode_len);

        if(ret)
        {
            LOGE("%s bk_bt_a2dp_source_write err %d\n", __func__, ret);
        }

    }


    os_memmove(mic_sco_data, &mic_sco_data[sent_threshold], mic_data_count);

    return len;
}

static int32_t bt_a2dp_source_prepare_sbc_encoder(void)
{
    bk_err_t ret = BK_OK;

    LOGE("%s\n", __func__);

    memset(&s_sbc_software_encoder_ctx, 0, sizeof(s_sbc_software_encoder_ctx));

    sbc_encoder_init(&s_sbc_software_encoder_ctx, s_a2dp_cap_info.sbc_codec.sample_rate, 1);

    uint8_t alloc_mode = 0;

    switch (s_a2dp_cap_info.sbc_codec.alloc_mode)
    {
    case 2://A2DP_SBC_ALLOCATION_METHOD_SNR:
        alloc_mode = 1;
        break;

    default:
    case 1://A2DP_SBC_ALLOCATION_METHOD_LOUDNESS:
        alloc_mode = 0;
        break;
    }

    ret = sbc_encoder_ctrl(&s_sbc_software_encoder_ctx, SBC_ENCODER_CTRL_CMD_SET_ALLOCATION_METHOD, alloc_mode); //0:loundness, 1:SNR

    if (ret != SBC_ENCODER_ERROR_OK)
    {
        LOGE("%s SBC_ENCODER_CTRL_CMD_SET_ALLOCATION_METHOD err %d\n", __func__, ret);
        return ret;
    }

    ret = sbc_encoder_ctrl(&s_sbc_software_encoder_ctx, SBC_ENCODER_CTRL_CMD_SET_BITPOOL, s_a2dp_cap_info.sbc_codec.bit_pool);

    if (ret != SBC_ENCODER_ERROR_OK)
    {
        LOGE("%s SBC_ENCODER_CTRL_CMD_SET_BITPOOL err %d\n", __func__, ret);
        return ret;
    }


    uint8_t block_mode = 3;

    switch (s_a2dp_cap_info.sbc_codec.block_len)
    {
    case 4://A2DP_SBC_BLOCK_LENGTH_4:
        block_mode = 0;
        break;

    case 8://A2DP_SBC_BLOCK_LENGTH_8:
        block_mode = 1;
        break;

    case 12://A2DP_SBC_BLOCK_LENGTH_12:
        block_mode = 2;
        break;

    default:
    case 16://A2DP_SBC_BLOCK_LENGTH_16:
        block_mode = 3;
        break;
    }

    ret = sbc_encoder_ctrl(&s_sbc_software_encoder_ctx, SBC_ENCODER_CTRL_CMD_SET_BLOCK_MODE, block_mode); //0:4, 1:8, 2:12, 3:16

    if (ret != SBC_ENCODER_ERROR_OK)
    {
        LOGE("%s SBC_ENCODER_CTRL_CMD_SET_BLOCK_MODE err %d\n", __func__, ret);
        return ret;
    }

    uint8_t channle_mode = 3;

    switch (s_a2dp_cap_info.sbc_codec.channel_mode)
    {
    case 8:
        channle_mode = 0;
        break;

    case 4:
        channle_mode = 1;
        break;

    case 2:
        channle_mode = 2;
        break;

    default:
    case 1:
        channle_mode = 3;
        break;
    }

    ret = sbc_encoder_ctrl(&s_sbc_software_encoder_ctx, SBC_ENCODER_CTRL_CMD_SET_CHANNEL_MODE, channle_mode); //0:MONO, 1:DUAL, 2:STEREO, 3:JOINT STEREO

    if (ret != SBC_ENCODER_ERROR_OK)
    {
        LOGE("%s SBC_ENCODER_CTRL_CMD_SET_CHANNEL_MODE err %d\n", __func__, ret);
        return ret;
    }

    uint8_t samp_rate_select = 2;

    switch (s_a2dp_cap_info.sbc_codec.sample_rate)
    {
    case 16000:
        samp_rate_select = 0;
        break;

    case 32000:
        samp_rate_select = 1;
        break;

    default:
    case 44100:
        samp_rate_select = 2;
        break;

    case 48000:
        samp_rate_select = 3;
        break;
    }

    ret = sbc_encoder_ctrl(&s_sbc_software_encoder_ctx, SBC_ENCODER_CTRL_CMD_SET_SAMPLE_RATE_INDEX, samp_rate_select); //0:16000, 1:32000, 2:44100, 3:48000

    if (ret != SBC_ENCODER_ERROR_OK)
    {
        LOGE("%s SBC_ENCODER_CTRL_CMD_SET_SAMPLE_RATE_INDEX err %d\n", __func__, ret);
        return ret;
    }


    uint8_t subband = 1;

    switch (s_a2dp_cap_info.sbc_codec.subbands)
    {
    case 4:
        subband = 0;
        break;

    default:
    case 8:
        subband = 1;
        break;
    }

    ret = sbc_encoder_ctrl(&s_sbc_software_encoder_ctx, SBC_ENCODER_CTRL_CMD_SET_SUBBAND_MODE, subband); //0:4, 1:8

    if (ret != SBC_ENCODER_ERROR_OK)
    {
        LOGE("%s SBC_ENCODER_CTRL_CMD_SET_SUBBAND_MODE err %d\n", __func__, ret);
        return ret;
    }


    LOGE("%s sbc encode count %d\n", __func__, s_sbc_software_encoder_ctx.pcm_length);

    return 0;
}


static int32_t bt_a2dp_source_start_voc(void)
{
    bk_err_t ret = BK_OK;

    LOGE("%s\n", __func__);

    LOGE("%s TODO: most sound box a2dp support neither mono channel nor 16 kHZ !!!\n", __func__);
    aud_intf_drv_setup_t aud_intf_drv_setup;
    aud_intf_work_mode_t aud_work_mode = 0;
    aud_intf_voc_setup_t aud_voc_setup;

    memset(&aud_intf_drv_setup, 0, sizeof(aud_intf_drv_setup));
    memset(&aud_voc_setup, 0, sizeof(aud_voc_setup));

    aud_intf_drv_setup.work_mode = AUD_INTF_WORK_MODE_NULL;
    aud_intf_drv_setup.task_config.priority = 3;
    aud_intf_drv_setup.aud_intf_rx_spk_data = NULL;
    aud_intf_drv_setup.aud_intf_tx_mic_data = bt_a2dp_source_mic_data_handler;

    ret = bk_aud_intf_drv_init(&aud_intf_drv_setup);

    if (ret != BK_ERR_AUD_INTF_OK)
    {
        LOGE("%s bk_aud_intf_drv_init fail, ret:%d\n", __func__, ret);
        return ret;
    }
    else
    {
        LOGE("%s bk_aud_intf_drv_init complete\n", __func__);
    }

    aud_work_mode = AUD_INTF_WORK_MODE_VOICE;

    ret = bk_aud_intf_set_mode(aud_work_mode);

    if (ret != BK_ERR_AUD_INTF_OK)
    {
        LOGE("%s bk_aud_intf_set_mode fail, ret:%d\n", __func__, ret);
        return ret;
    }
    else
    {
        LOGE("%s bk_aud_intf_set_mode complete\n", __func__);
    }

    aud_voc_setup.aec_enable = true;
    aud_voc_setup.samp_rate = AUD_INTF_VOC_SAMP_RATE_16K;
    aud_voc_setup.data_type = AUD_INTF_VOC_DATA_TYPE_PCM;
    aud_voc_setup.spk_mode = AUD_DAC_WORK_MODE_SIGNAL_END; //AUD_DAC_WORK_MODE_DIFFEN;
    aud_voc_setup.mic_type = AUD_INTF_MIC_TYPE_BOARD;
    aud_voc_setup.mic_gain = 0x2d;
    aud_voc_setup.spk_gain = 0x2d;
    aud_voc_setup.aec_cfg.ec_depth = 20;
    aud_voc_setup.aec_cfg.TxRxThr = 30;
    aud_voc_setup.aec_cfg.TxRxFlr = 6;
    aud_voc_setup.aec_cfg.ns_level = 2;
    aud_voc_setup.aec_cfg.ns_para = 1;

    ret = bk_aud_intf_voc_init(aud_voc_setup);
    if (ret != BK_ERR_AUD_INTF_OK)
    {
        LOGE("%s bk_aud_intf_voc_init fail, ret:%d\n", __func__, ret);
        return ret;
    }

    ret = bk_aud_intf_voc_start();
    if (ret != BK_ERR_AUD_INTF_OK)
    {
        LOGE("%s bk_aud_intf_voc_start fail, ret:%d\n", __func__, ret);
        return ret;
    }

    LOGE("%s ok\n", __func__);

    return 0;
}


static int bt_enable_a2dp_source_start_handle(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    int err = kNoErr;
    if (sync)
    {
        err = rtos_init_semaphore(&bt_at_cmd_sema, 1);
        if(err != kNoErr){
            goto error;
        }
    }
    if(bk_bt_get_host_stack_type() == BK_BT_HOST_STACK_TYPE_ETHERMIND)
    {
        err = bt_get_addr_from_param(&a2dp_env.peer_addr, argv[0]);

        if(err) goto error;

        if(a2dp_env.conn_state != BK_A2DP_CONNECTION_STATE_CONNECTED)
        {
            LOGE("With remote device is not connected\n");
            goto error;
        }

        if(a2dp_env.start_status != 0)
        {
            LOGE("is already start\n");
            goto error;
        }

        err = bk_bt_a2dp_source_start(&(a2dp_env.peer_addr));

        if(err)
        {
            LOGE("%s start err %d\n", __func__, err);
            goto error;
        }

        err = rtos_get_semaphore(&bt_at_cmd_sema, 6 * 1000);
        if(!err)
        {
//            spp_env.conn_state = STATE_PROFILE_CONNECTED;
			atsvr_cmd_rsp_ok();
            if (bt_at_cmd_sema != NULL)
                rtos_deinit_semaphore(&bt_at_cmd_sema);
            return err;
        }
        else
        {
            goto error;
        }
    }
error:
	atsvr_cmd_rsp_error();
//    os_memset(&spp_env, 0, sizeof(spp_env_s));
    if (bt_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&bt_at_cmd_sema);
    return err;
}


static int bt_enable_a2dp_source_suspend_handle(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    int err = kNoErr;
    if (sync)
    {
        err = rtos_init_semaphore(&bt_at_cmd_sema, 1);
        if(err != kNoErr){
            goto error;
        }
    }
    if(bk_bt_get_host_stack_type() == BK_BT_HOST_STACK_TYPE_ETHERMIND)
    {
        err = bt_get_addr_from_param(&a2dp_env.peer_addr, argv[0]);

        if(err) goto error;

        if(a2dp_env.conn_state != BK_A2DP_CONNECTION_STATE_CONNECTED)
        {
            LOGE("With remote device is not connected\n");
            goto error;
        }

        if(a2dp_env.start_status == 0)
        {
            LOGE("is already suspend\n");
            goto error;
        }

        err = bk_bt_a2dp_source_suspend(&(a2dp_env.peer_addr));

        if(err)
        {
            LOGE("%s suspend err %d\n", __func__, err);
            goto error;
        }

        err = rtos_get_semaphore(&bt_at_cmd_sema, 6 * 1000);
        if(!err)
        {
			atsvr_cmd_rsp_ok();
            if (bt_at_cmd_sema != NULL)
                rtos_deinit_semaphore(&bt_at_cmd_sema);
            return err;
        }
        else
        {
            goto error;
        }
    }
error:
	atsvr_cmd_rsp_error();
    if (bt_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&bt_at_cmd_sema);
    return err;
}

static void bt_a2dp_set_get_sample_clean(void)
{
    s_last_get_sample_time = 0;
    s_remain_send_sample_count = 0;
    s_remain_send_sample_miss_num = 0;
}

static int bt_enable_a2dp_source_stop_handle(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    int err = kNoErr;
    bt_a2dp_source_msg_t a2dp_msg;
    if (sync)
    {
        err = rtos_init_semaphore(&bt_at_cmd_sema, 1);
        if(err != kNoErr){
            goto error;
        }
    }
    if(bk_bt_get_host_stack_type() == BK_BT_HOST_STACK_TYPE_ETHERMIND)
    {
        int rc = -1;

        a2dp_msg.len = 0;
        a2dp_msg.type = BT_A2DP_SOURCE_STOP_MSG;
        a2dp_env.play_status = 0;

        rc = rtos_push_to_queue(&bt_a2dp_source_msg_que, &a2dp_msg, BEKEN_NO_WAIT);
        if (kNoErr != rc)
        {
            LOGE("%s, send queue failed\r\n",__func__);
            goto error;
        }

		atsvr_cmd_rsp_ok();
        if (bt_at_cmd_sema != NULL)
            rtos_deinit_semaphore(&bt_at_cmd_sema);
        return err;

    }
error:
	atsvr_cmd_rsp_error();
    if (bt_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&bt_at_cmd_sema);
    return err;
}

#if CONFIG_FATFS
static void bt_a2dp_source_write_from_file_timer_hdl(void *param)
{
    FRESULT fr;
    static uint32_t offset = 0;
    unsigned int read_len = 0;
    FIL *pcm_file_fd = (typeof(pcm_file_fd))param;
    int32_t encode_len = 0;
    bt_err_t ret = 0;
    uint8_t *tmp_buf = NULL;
    uint32_t encode_index = 0;

    uint32_t sent_threshold = s_sbc_software_encoder_ctx.pcm_length * 2 *
            s_sbc_software_encoder_ctx.num_channels; //pcm_length == 128 when 16 block 8 subband 1 channel
    //s_sbc_software_encoder_ctx.frame.blocks * s_sbc_software_encoder_ctx.frame.subbands == s_sbc_software_encoder_ctx.pcm_length;

    tmp_buf = os_malloc(A2DP_SOURCE_SBC_FRAME_COUNT * 128); //max 128

    if(!tmp_buf)
    {
        LOGE("%s malloc err\n", __func__);
        return;
    }

    for (uint32_t i = 0; i < A2DP_SOURCE_SBC_FRAME_COUNT; ++i)
    {
        fr = f_read(pcm_file_fd, (void *)(mic_sco_data + offset), sent_threshold, &read_len);
        if (fr != FR_OK)
        {
            LOGE("%s read fail %d\n", __func__, fr);
            os_free(tmp_buf);
            return;
        }

        if(read_len < sent_threshold)
        {
            LOGE("%s read len %d < %d !!\n", __func__, read_len, sent_threshold);

            if(read_len == 0)
            {
                LOGE("%s read file end !!\n", __func__, read_len, sent_threshold);
                os_free(tmp_buf);
                rtos_stop_timer(&bt_a2dp_source_write_timer);
                return;
            }

            memset(mic_sco_data + read_len, 0, sent_threshold - read_len);
        }

        encode_len = sbc_encoder_encode(&s_sbc_software_encoder_ctx, (const int16_t *)(mic_sco_data));
        if(encode_len < 0)
        {
           LOGE("%s encode err %d\n", __func__, encode_len);
           os_free(tmp_buf);
           return;
        }

        if(encode_index + encode_len > A2DP_SOURCE_SBC_FRAME_COUNT * 128)
        {
            LOGE("%s encode data large than malloc !! %d\n", __func__, encode_len);
            os_free(tmp_buf);
            return;

        }
        memcpy(tmp_buf + encode_index, s_sbc_software_encoder_ctx.stream, encode_len);
        encode_index += encode_len;

    }

    ret = bk_bt_a2dp_source_write(&(a2dp_env.peer_addr), A2DP_SOURCE_SBC_FRAME_COUNT, tmp_buf, encode_index);

    if(ret)
    {
        LOGE("%s bk_bt_a2dp_source_write err %d\n", __func__, ret);
    }

    os_free(tmp_buf);
}
#endif

#if CONFIG_FATFS
static void bt_a2dp_source_write_from_file_timer_auto_hdl(void *param)
{
    bt_a2dp_source_msg_t msg;
    int rc = -1;

    if (bt_a2dp_source_msg_que == NULL)
        return;

    msg.len = sizeof(FIL *);
    msg.data = param;

    msg.type = BT_A2DP_SOURCE_START_MSG;

    if(a2dp_env.play_status == 1)
    {
        rc = rtos_push_to_queue(&bt_a2dp_source_msg_que, &msg, BEKEN_NO_WAIT);
        if (kNoErr != rc)
        {
            //LOGE("%s, send queue failed\r\n",__func__);
        }
    }
}

#endif
#if CONFIG_FATFS
void bt_a2dp_source_write_from_file(FIL *fd)
{

    unsigned int read_len = 0;
    static uint32_t read_len_all = 0;
    FIL *pcm_file_fd = fd;
    FRESULT fr;
    int32_t encode_len = 0;
    bt_err_t ret = 0;
    uint8_t *tmp_buf = NULL;
    uint32_t encode_index = 0;


    //    beken_time_t last_time = s_last_get_sample_time;
    beken_time_t cur_time = rtos_get_time();
    beken_time_t will_send_period_time = 0;


    uint32_t will_send_frame_count = 0;
//    uint32_t sample_count = 0, sample_miss_num = 0;


    //s_sbc_software_encoder_ctx.frame.blocks * s_sbc_software_encoder_ctx.frame.subbands == s_sbc_software_encoder_ctx.pcm_length;
    //pcm_length == 128 when 16 block 8 subband
    uint32_t one_pcm_frame_encode_sample_count_per_channel = s_sbc_software_encoder_ctx.pcm_length;
    uint32_t one_pcm_frame_encode_sample_bytes = one_pcm_frame_encode_sample_count_per_channel * 2 * s_sbc_software_encoder_ctx.num_channels;
    const uint32_t send_sbc_frame_count = 2;// A2DP_SOURCE_SBC_FRAME_COUNT;
    const uint32_t ignore_max_mtu = 0;

    uint32_t i = 0, j = 0;

    if (s_last_get_sample_time == 0)
    {
        will_send_period_time = A2DP_SOURCE_WRITE_AUTO_TIMER_MS;//first
    }
    else
    {
        will_send_period_time = cur_time - s_last_get_sample_time;
    }

    s_remain_send_sample_count += will_send_period_time * s_sbc_software_encoder_ctx.sample_rate / 1000;
    s_remain_send_sample_miss_num += will_send_period_time * s_sbc_software_encoder_ctx.sample_rate % 1000;

    s_remain_send_sample_count += s_remain_send_sample_miss_num / 1000;
    s_remain_send_sample_miss_num = s_remain_send_sample_miss_num % 1000;

    s_last_get_sample_time = cur_time;

    if(!s_remain_send_sample_count)
    {
        return;
    }
    else if (s_remain_send_sample_count < one_pcm_frame_encode_sample_count_per_channel)
    {
        if (will_send_period_time * s_sbc_software_encoder_ctx.sample_rate / 1000 <
                send_sbc_frame_count * one_pcm_frame_encode_sample_count_per_channel)
        {
            return;
        }
    }
    else if (s_remain_send_sample_count >= one_pcm_frame_encode_sample_count_per_channel &&
            s_remain_send_sample_count < one_pcm_frame_encode_sample_count_per_channel * send_sbc_frame_count)
    {
        if (0)//will_send_period_time * s_sbc_software_encoder_ctx.sample_rate / 1000 < send_sbc_frame_count * one_pcm_frame_encode_sample_count_per_channel)
        {
            return;
        }
    }


    will_send_frame_count = ((s_remain_send_sample_count / one_pcm_frame_encode_sample_count_per_channel > 1) ?
            (s_remain_send_sample_count / one_pcm_frame_encode_sample_count_per_channel) : 1);

//    if(will_send_frame_count > A2DP_SOURCE_SBC_FRAME_COUNT)
//    {
//        will_send_frame_count = A2DP_SOURCE_SBC_FRAME_COUNT;
//    }

    if(will_send_frame_count > 100)
    {
        LOGE("%s\n", __func__);
    }

    if(!will_send_frame_count)
    {
        LOGE("%s 0\n", __func__);
    }

    //printf("%s will_send_frame_count %d \n", __func__, will_send_frame_count);

    tmp_buf = os_malloc(will_send_frame_count * 128); //max 128

    if(tmp_buf == NULL)
    {
        LOGE("%s tmp_buf no buffer malloc\n", __func__);
        bt_a2dp_set_get_sample_clean();
        return;
    }
    do
    {
//        for (; i < will_send_frame_count; ++i)
        for(i = 0; ( ignore_max_mtu || encode_index + encode_len <= a2dp_env.mtu) && i + j < will_send_frame_count; ++i)
        {
            fr = f_read(pcm_file_fd, (void *)(mic_sco_data), one_pcm_frame_encode_sample_bytes, &read_len);
//            read_len = fread((mic_sco_data), 1, one_pcm_frame_encode_sample_bytes, pcm_file_fd);

            if (fr != FR_OK)
            {
                LOGE("%s read fail %d\n", __func__, fr);
                os_free(tmp_buf);
                return;
            }

            read_len_all += read_len;

            if (read_len < one_pcm_frame_encode_sample_bytes)
            {
                LOGE("%s read len %d < %d!!\n", __func__, read_len, one_pcm_frame_encode_sample_bytes);


                if (read_len == 0)
                {
                    LOGE("%s read file end %d %d !!\n", __func__, read_len, one_pcm_frame_encode_sample_bytes);
                    os_free(tmp_buf);

//                    fclose(pcm_file_fd);
//                    *(FILE **)param = NULL;
                    rtos_stop_timer(&bt_a2dp_source_write_timer);
                    bt_a2dp_set_get_sample_clean();
                    a2dp_env.play_status = 0;


                    if(1) //repeat
                    {
                        fr = f_lseek(pcm_file_fd, 0);
                        if (fr != FR_OK)
                        {
                            LOGE("%s f_lseek fail.\n", __func__);
                            f_close(pcm_file_fd);
                            memset(pcm_file_fd, 0, sizeof(*pcm_file_fd));
                            return;
                        }

                        rtos_start_timer(&bt_a2dp_source_write_timer);
                        a2dp_env.play_status = 1;
                    }
                    else
                    {
                        f_close(pcm_file_fd);
                        memset(pcm_file_fd, 0, sizeof(*pcm_file_fd));
                    }

                    return;
                }

                memset(mic_sco_data + read_len, 0, one_pcm_frame_encode_sample_bytes - read_len);
            }

            encode_len = sbc_encoder_encode(&s_sbc_software_encoder_ctx, (const int16_t *)(mic_sco_data));

            if (encode_len < 0)
            {
                LOGE("%s encode err %d\n", __func__, encode_len);
                os_free(tmp_buf);
                return;
            }

            if (encode_index + encode_len > will_send_frame_count * 128)
            {
                LOGE("%s encode data large than malloc !! %d\n", __func__, encode_len);
                os_free(tmp_buf);
                return;
            }

            memcpy(tmp_buf + encode_index, s_sbc_software_encoder_ctx.stream, encode_len);
            encode_index += encode_len;
        }
//        while(encode_index + encode_len <= a2dp_env.mtu && i < will_send_frame_count);

        ret = bk_bt_a2dp_source_write(&(a2dp_env.peer_addr), i, tmp_buf, encode_index);


        if (encode_index > 10000 || i > 5)
        {
            LOGE("%s encode_index %d i %d\n", __func__, encode_index, i);
        }

        if (ret)
        {
            LOGE("%s bk_bt_a2dp_source_write err %d\n", __func__, ret);
        }

        encode_index = 0;
        j += i;
    }
    while(j < will_send_frame_count);

    if(s_remain_send_sample_count < j * one_pcm_frame_encode_sample_count_per_channel)
    {
        s_remain_send_sample_count = 0;
    }
    else
    {
        s_remain_send_sample_count -= j * one_pcm_frame_encode_sample_count_per_channel;
    }

    os_free(tmp_buf);

}
#endif

void bt_a2dp_source_main(void *arg)
{
    while (1) {
        bk_err_t err;
        bt_a2dp_source_msg_t msg;

        err = rtos_pop_from_queue(&bt_a2dp_source_msg_que, &msg, BEKEN_WAIT_FOREVER);
        if (kNoErr == err)
        {
            switch (msg.type) {
#if CONFIG_FATFS
                case BT_A2DP_SOURCE_START_MSG:
                {
                    if(a2dp_env.play_status == 1)
                        bt_a2dp_source_write_from_file((FIL *)msg.data);
                }
                break;

                case BT_A2DP_SOURCE_STOP_MSG:
                {
                    f_close(&pcm_file_fd);
                    memset(&pcm_file_fd, 0, sizeof(pcm_file_fd));
                    rtos_stop_timer(&bt_a2dp_source_write_timer);
                    rtos_deinit_timer(&bt_a2dp_source_write_timer);
                    bt_a2dp_set_get_sample_clean();
                }
                break;
#endif
            }
        }
    }

    rtos_deinit_queue(&bt_a2dp_source_msg_que);
    bt_a2dp_source_msg_que = NULL;
    bt_a2dp_source_thread_handle = NULL;
    rtos_delete_thread(NULL);
}

int bt_a2dp_source_task_init(void)
{
    bk_err_t ret = BK_OK;
    if ((!bt_a2dp_source_thread_handle) && (!bt_a2dp_source_msg_que))
    {
        ret = rtos_init_queue(&bt_a2dp_source_msg_que,
                              "bt_a2dp_source_msg_que",
                              sizeof(bt_audio_demo_msg_t),
                              10);
        if (ret != kNoErr) {
            LOGE("bt_audio demo msg queue failed \r\n");
            return BK_FAIL;
        }

        ret = rtos_create_thread(&bt_a2dp_source_thread_handle,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "bt_a2dp_source",
                             (beken_thread_function_t)bt_a2dp_source_main,
                             4096,
                             (beken_thread_arg_t)0);
        if (ret != kNoErr) {
            LOGE("bt_a2dp_source task fail \r\n");
            rtos_deinit_queue(&bt_a2dp_source_msg_que);
            bt_a2dp_source_msg_que = NULL;
            bt_a2dp_source_thread_handle = NULL;
        }

        return kNoErr;
    }
    else
    {
        return kInProgressErr;
    }
}

#if CONFIG_FATFS
static int32_t bt_a2dp_source_encode_from_file(uint8_t * path)
{
    FRESULT fr;
    uint32_t inter = (uint32_t)((640 * 1000.0) / (float)s_sbc_software_encoder_ctx.sample_rate);

    char full_path[64] = {0};

    test_mount(1);

    //need s16le dual pcm
//    sprintf((char *)full_path, "%d:/%s", mount_label, path);
    sprintf((char *)full_path, "%s", path);


    LOGE("%s send interval %d\n", __func__, inter);
    a2dp_env.play_status = 1;

    bt_a2dp_source_task_init();

    if (rtos_is_timer_init(&bt_a2dp_source_write_timer))
    {
        if (rtos_is_timer_running(&bt_a2dp_source_write_timer))
        {
            rtos_stop_timer(&bt_a2dp_source_write_timer);
        }

        rtos_deinit_timer(&bt_a2dp_source_write_timer);
    }

    if (!rtos_is_timer_init(&bt_a2dp_source_write_timer))
    {
//        rtos_init_timer(&bt_a2dp_source_write_timer, inter, bt_a2dp_source_write_from_file_timer_hdl, (void *)&pcm_file_fd);
        rtos_init_timer(&bt_a2dp_source_write_timer, A2DP_SOURCE_WRITE_AUTO_TIMER_MS, bt_a2dp_source_write_from_file_timer_auto_hdl, (void *)&pcm_file_fd);
    }


    rtos_change_period(&bt_a2dp_source_write_timer, inter);

    if(pcm_file_fd.fs)
    {
        f_close(&pcm_file_fd);
    }

    memset(&pcm_file_fd, 0, sizeof(pcm_file_fd));
    fr = f_open(&pcm_file_fd, (const char *)full_path, FA_OPEN_EXISTING | FA_READ);
    if (fr != FR_OK)
    {
        LOGE("open %s fail.\n", full_path);
        return -1;
    }

    rtos_start_timer(&bt_a2dp_source_write_timer);
    return 0;
}
#endif

static int bt_enable_a2dp_source_write_test_handle(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    int err = kNoErr;
    uint8_t choise = 0;//0 mic 1 file 2 static code

    if (0)//bt_at_cmd_table[3].is_sync_cmd)
    {
        err = rtos_init_semaphore(&bt_at_cmd_sema, 1);
        if(err != kNoErr){
            goto error;
        }
    }

    if(bk_bt_get_host_stack_type() == BK_BT_HOST_STACK_TYPE_ETHERMIND)
    {
        err = bt_get_addr_from_param(&a2dp_env.peer_addr, argv[0]);
        if(err) goto error;

        if(argc >= 2)
        {
            if(strcasecmp(argv[1], "mic") == 0)
            {
                choise = 0;
            }
            else if(strcasecmp(argv[1], "file") == 0)
            {
                choise = 1;

                if(argc < 3)
                {
                    LOGE("%s please input file path\n", __func__);
                    goto error;
                }
            }
            else
            {
                LOGE("%s unknow case %S\n", __func__, argv[1]);
                goto error;
            }

        }

        err = bt_a2dp_source_prepare_sbc_encoder();
        if(err)
        {
            goto error;
        }

#if 1
        if(a2dp_env.conn_state != BK_A2DP_CONNECTION_STATE_CONNECTED)
        {
            LOGE("With remote device is not connected\n");
            goto error;
        }

        if(a2dp_env.start_status == 0)
        {
            LOGE("is suspend\n");
            goto error;
        }
#endif

        switch(choise)
        {
        case 0:
            err = bt_a2dp_source_start_voc();
            break;

#if CONFIG_FATFS
        case 1:
            err = bt_a2dp_source_encode_from_file((uint8_t *)argv[2]);
            break;
#endif
        default:
            break;
        }

//        err = rtos_get_semaphore(&bt_at_cmd_sema, 60 * 1000);
        if(!err)
        {
			atsvr_cmd_rsp_ok();
            if (bt_at_cmd_sema != NULL)
                rtos_deinit_semaphore(&bt_at_cmd_sema);
            return err;
        }
        else
        {
            goto error;
        }
    }

error:
	atsvr_cmd_rsp_error();
    if (bt_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&bt_at_cmd_sema);
    return err;
}

#if 0
static int bt_enable_a2dp_source_test_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    PRINT_FUNC;
    char *msg;
    int err = kNoErr;

    if (0)//bt_at_cmd_table[3].is_sync_cmd)
    {
        err = rtos_init_semaphore(&bt_at_cmd_sema, 1);
        if(err != kNoErr){
            goto error;
        }
    }

    if(bk_bt_get_host_stack_type() == BK_BT_HOST_STACK_TYPE_ETHERMIND)
    {
        err = bt_get_addr_from_param(&a2dp_env.peer_addr, argv[0]);
        if(err) goto error;

        if(argc >= 2)
        {
            if(strcasecmp(argv[1], "file") == 0)
            {
                if(argc < 3)
                {
                    LOGE("%s please input file path\n", __func__);
                    goto error;
                }

                err = bt_a2dp_source_demo_test(a2dp_env.peer_addr.addr, 1, (uint8_t *)argv[2]);
            }
            else
            {
                LOGE("%s unknow case %s\n", __func__, argv[1]);
                goto error;
            }

        }

        if(!err)
        {
            msg = AT_CMD_RSP_SUCCEED;
            os_memcpy(pcWriteBuffer, msg, os_strlen(msg));

            if (bt_at_cmd_sema != NULL)
            {
                rtos_deinit_semaphore(&bt_at_cmd_sema);
            }

            return err;
        }
        else
        {
            goto error;
        }
    }

error:
    msg = AT_CMD_RSP_ERROR;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    if (bt_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&bt_at_cmd_sema);
    return err;
}

static int bt_enable_a2dp_source_test_stop_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    PRINT_FUNC;
    char *msg;
    int err = kNoErr;

    if (0)//bt_at_cmd_table[3].is_sync_cmd)
    {
        err = rtos_init_semaphore(&bt_at_cmd_sema, 1);
        if(err != kNoErr){
            goto error;
        }
    }

    if(bk_bt_get_host_stack_type() == BK_BT_HOST_STACK_TYPE_ETHERMIND)
    {
        err = bt_a2dp_source_demo_stop_all();

        if(!err)
        {
            msg = AT_CMD_RSP_SUCCEED;
            os_memcpy(pcWriteBuffer, msg, os_strlen(msg));

            if (bt_at_cmd_sema != NULL)
            {
                rtos_deinit_semaphore(&bt_at_cmd_sema);
            }

            return err;
        }
        else
        {
            goto error;
        }
    }

error:
    msg = AT_CMD_RSP_ERROR;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    if (bt_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&bt_at_cmd_sema);
    return err;
}
#endif

#endif

#define DO1(buf) crc = crc_table(((int)crc ^ (*buf++)) & 0xff) ^ (crc >> 8);
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);

static uint32_t crc_table(uint32_t index)
{
	uint32_t c = index;
	uint32_t poly = 0xedb88320L;
	int k;

	for (k = 0; k < 8; k++)
		c = c & 1 ? poly ^ (c >> 1) : c >> 1;

	return c;
}

uint32_t crc32(uint32_t crc, const uint8_t *buf, int len)
{
    if (buf == NULL) return 0L;

  //  crc = crc ^ 0xffffffffL;
    while (len >= 8)
    {
      DO8(buf);
      len -= 8;
    }
    if (len) do {
      DO1(buf);
    } while (--len);
    return crc;// ^ 0xffffffffL;
}


static unsigned int crc32_table[256];

unsigned int make_crc32_table(void)
{
	uint32_t c;
	int i = 0;
	int bit = 0;

	for(i = 0;i < 256;i++)
	{
		c = (unsigned int)i;

		for(bit = 0;bit < 8;bit++)
		{
			if(c&1)
			{
				c = (c>>1)^(0xEDB88320);
			}
			else
			{
				c = c >> 1;
			}
		}

		crc32_table[i] = c;
//		os_printf("crc32_table[%d] = %08x\r\n",i,crc32_table[i]);
	}

	return 0;
}

uint32_t calc_crc32(uint32_t crc, const uint8_t *buf, int len)
{
	while(len--)
	{
		crc = (crc >> 8)^(crc32_table[(crc^*buf++)&0xff]);
	}

	return crc;
}

/********************************************************/
/*********************L2CAP AT CMD **********************/
/********************************************************/
#if DM_L2CAP_CMD_ENABLE

#define AT_DM_L2CAP_LOCAL_PSM   0x1231
#define AT_DM_L2CAP_REMOTE_PSM  0x1231
#define INVALID_HANDLE          0xFF
#define INVALID_LCID            0xFFFF
#define L2CAP_START_END         "END"

typedef enum{
    STATE_ACL_DISCONNECT = 0,
    STATE_ACL_CONNECTING,
    STATE_ACL_CONNECTED,
    STATE_L2CAP_CONNECTING,
    STATE_L2CAP_CONNECTED,
    STATE_L2CAP_DISCONNECT,
    STATE_L2CAP_DISCONNECTING,
    STATE_L2CAP_DISCONNECTED,
    STATE_l2CAP_SRV_DISCONNECTING,
    STATE_l2CAP_SRV_DISCONNECTED,
}l2cap_connect_state_s;

typedef void (* bt_l2cap_tx_fun)(uint16_t lcid, uint16_t *data_len, uint16_t *tx_len);

typedef struct
{
    uint8_t init;
    bk_bd_addr_t remote_addr;
    uint16_t handle;
    uint8_t conn_state;
    uint16_t fd[10];
    uint16_t rx_len[10];
    uint16_t data_len[10];
    uint16_t tx_len[10];
    bt_l2cap_tx_fun tx_fun[10];
    uint8_t fd_index;
    beken_thread_t l2cap_tx_thread;
}dm_l2cap_env_t;

typedef struct{
    beken_thread_t *l2cap_tx_thread;
    uint16_t lcid;
    uint16_t len;
}l2cap_thread_args_t;

static dm_l2cap_env_t l2cap_env = {0};

static int bt_at_param_analyze(char *str, char *match, void *param, uint8_t array_size)
{
//    LOGI("arv--> %s \r\n", str);
    char mm[100] = {0};
    char m1[10] = {0};
    char *s1 = "";
    char *s2 = "=%2s";
    char *s3 = "=%*";
    char *s4 = "%ds%s";
    strcat(mm, s1);
    strcat(mm, match);
    strcat(mm, s2);
    char m2[4] = "%2x";
    char type[3] = "ss";
//    LOGI("m1: %s \r\n", mm);
    int sf = sscanf(str, mm, type);
//    LOGI("sf:%d, type :%c %c \r\n", sf, type[0], type[1]);
    if(sf <= 0)
    {
        return sf;
    }
    os_memset(mm, 0, 100);
    strcat(mm, s1);
    strcat(mm, match);
    strcat(mm, s3);
    int p = strlen(s1) + strlen(match) + strlen(s3);
    int ret = 0;
    switch (type[0])
    {
        case 'h':
            sprintf(m1, s4, 1, "%x");
            break;
        case 's':
            sprintf(m1, s4, 1, "%s");
            break;
        case 'a':
            if(type[1] == 'h')
            {
                m2[2] = 'x';
            }else if(type[1] == 'd')
            {
                m2[2] = 'd';
            }
            for(int i=0;i<array_size;i++)
            {
                sprintf(m1, s4, (i+1)*2, m2);
                mm[p] = 0;
                strcat(mm, m1);
//                LOGI("m2 : %s \r\n", mm);
                sf = sscanf(str, mm, (uint8_t *)(param+i));
                if(sf != 1)
                {
                    break;
                }
                ret++;
            }
            return ret;
        case 'd':
            sprintf(m1, s4, 1, "%d");
            break;
        default :
            sprintf(m1, s4, 1, "%x");
            break;
    }
    strcat(mm, m1);
//    LOGI("m2 : %s \r\n", mm);
    sf = sscanf(str, mm, param);
    return sf;
}

static int bt_at_params_analyze(int argc, char **argv, char *match, void *param, uint8_t array_size)
{
    int res = 0;
    for(int i=0;i<argc;i++)
    {
        if(argv[i] !=NULL)
        {
            res = bt_at_param_analyze(argv[i], match, param, array_size);
            if(res)
            {
                break;
            }
        }
    }
    return res;
}

static void bt_l2cap_event_callback(bk_bt_l2cap_cb_event_t event, bk_bt_l2cap_cb_param_t *param)
{
    switch (event)
    {
        case BK_BT_L2CAP_INIT_EVT:
            LOGI("%s init status: %d \r\n", __func__, param->init.status);
            break;
        case BK_BT_L2CAP_UNINIT_EVT:
            LOGI("%s uninit status: %d \r\n", __func__, param->uninit.status);
            break;
        case BK_BT_L2CAP_OPEN_EVT:
            LOGI("%s open status: %d \r\n", __func__, param->open.status);
            if(param->open.status == BK_BT_L2CAP_SUCCESS)
            {
                l2cap_env.conn_state = STATE_L2CAP_CONNECTED;
                for(int i=0;i<sizeof(l2cap_env.fd)/sizeof(l2cap_env.fd[0]);i++)
                {
                    if(l2cap_env.fd[i] == INVALID_LCID)
                    {
                        l2cap_env.fd[i] = (uint16_t)param->open.fd;
                        l2cap_env.fd_index++;
                        break;
                    }
                }
                LOGI("L2cap connection success, remote addr:0x%x,0x%x,0x%x,0x%x,0x%x,0x%x  LCID: 0x%x\r\n",
                param->open.rem_bda[0],param->open.rem_bda[1],param->open.rem_bda[2],param->open.rem_bda[3],param->open.rem_bda[4],param->open.rem_bda[5],
                param->open.fd);
            }
            bt_at_sema_set();
            break;
        case BK_BT_L2CAP_CLOSE_EVT:
            LOGI("%s stop status: %d \r\n", __func__, param->close.status);
            if(l2cap_env.conn_state == STATE_L2CAP_DISCONNECTING)
            {
                if(param->start.status == BK_BT_L2CAP_SUCCESS)
                {
                    LOGI("L2cap channle disconnection success, LCID: 0x%x\r\n", param->close.fd);
                    l2cap_env.conn_state = STATE_L2CAP_DISCONNECTED;
                }
                bt_at_sema_set();
            }else if(l2cap_env.conn_state == STATE_l2CAP_SRV_DISCONNECTING)
            {
                if(param->start.status == BK_BT_L2CAP_SUCCESS)
                {
                    LOGI("L2cap channle disconnection success, LCID: 0x%x\r\n", param->close.fd);
                    for(int i=0;i<sizeof(l2cap_env.fd)/sizeof(l2cap_env.fd[0]);i++)
                    {
                        if(l2cap_env.fd[i] == (uint16_t)param->close.fd)
                        {
                            l2cap_env.fd[i] = INVALID_LCID;
                            l2cap_env.data_len[i] = 0;
                            l2cap_env.tx_len[i] = 0;
                            l2cap_env.rx_len[i] = 0;
                            l2cap_env.tx_fun[i] = NULL;
                            l2cap_env.fd_index--;
                            break;
                        }
                    }
                }
            }else
            {
                //remote disconnect ind
                if(param->start.status == BK_BT_L2CAP_SUCCESS)
                {
                    LOGI("L2cap channle disconnection Ind succ, LCID: 0x%x\r\n", param->close.fd);
                    for(int i=0;i<sizeof(l2cap_env.fd)/sizeof(l2cap_env.fd[0]);i++)
                    {
                        if(l2cap_env.fd[i] == (uint16_t)param->close.fd)
                        {
                            l2cap_env.fd[i] = INVALID_LCID;
                            l2cap_env.data_len[i] = 0;
                            l2cap_env.tx_len[i] = 0;
                            l2cap_env.rx_len[i] = 0;
                            l2cap_env.tx_fun[i] = NULL;
                            l2cap_env.fd_index--;
                            break;
                        }
                    }
                }
            }
            break;
        case BK_BT_L2CAP_START_EVT:
            LOGI("%s start server status: %d \r\n", __func__, param->start.status);
            break;
        case BK_BT_L2CAP_CL_INIT_EVT:
            LOGI("%s client conn cnf status: %d \r\n", __func__, param->cl_init.status);
            if(param->cl_init.status == BK_BT_L2CAP_SUCCESS)
            {
                l2cap_env.conn_state = STATE_L2CAP_CONNECTED;
            }
            break;
        case BK_BT_L2CAP_SRV_STOP_EVT:
            LOGI("%s stop server status: %d, psm:0x%x \r\n", __func__, param->srv_stop.status, param->srv_stop.psm);
            if(l2cap_env.conn_state == STATE_l2CAP_SRV_DISCONNECTING)
            {
                bt_at_sema_set();
            }
            break;
    }
}

static void bt_l2cap_ind_data_callback(int fd, void *data, uint16_t size)
{
    uint8_t start = 0;
    char *s = L2CAP_START_END;
    uint16_t t = 0;
    if(!os_memcmp(data, s, os_strlen(s)))
    {
        start = 1;
    }
    for(int i=0;i<sizeof(l2cap_env.fd)/sizeof(l2cap_env.fd[0]);i++)
    {
        if(l2cap_env.fd[i] == (uint16_t)fd)
        {
            if(start)
            {
                LOGI("<============================data ind end============================>\r\n");
                LOGI("lcid:0x%x, total_len:%d \r\n", fd, l2cap_env.rx_len[i]);
                l2cap_env.rx_len[i] = 0;
            }
            else
            {
                l2cap_env.rx_len[i]+=size;
            }
            t = l2cap_env.rx_len[i];
            break;
        }
    }
    if(!start)
        LOGI("lcid:0x%x, data:0x%x, size:%d \r\n", fd, ((uint8_t *)data)[0], t);
}


static int bt_l2cap_init_handle(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    bk_err_t ert = kNoErr;
    do
    {
        if(bk_bt_get_host_stack_type() != BK_BT_HOST_STACK_TYPE_ETHERMIND)
        {
            ert = BK_FAIL;
            LOGI("Current stack does not support l2cap \r\n");
            break;
        }
        if(l2cap_env.init)
        {
            LOGI("L2cap has been initated \r\n");
            break;
        }
        bk_bt_gap_set_event_callback(bt_at_event_cb);
        bk_bt_gap_set_visibility(BK_BT_CONNECTABLE, BK_BT_DISCOVERABLE);
        bk_bt_l2cap_init();
        bk_bt_l2cap_start_srv(BK_BT_L2CAP_SEC_NONE, AT_DM_L2CAP_LOCAL_PSM);
        bk_bt_l2cap_start_srv(BK_BT_L2CAP_SEC_NONE, AT_DM_L2CAP_LOCAL_PSM+2);
        bk_bt_l2cap_set_data_callback(bt_l2cap_ind_data_callback);
        bk_bt_l2cap_register_callback(bt_l2cap_event_callback);
        l2cap_env.handle = INVALID_HANDLE;
        l2cap_env.init = 1;
        os_memset(l2cap_env.fd, 0xFF, sizeof(l2cap_env.fd));
    }
    while (0);
	atsvr_cmd_rsp_ok();
    return ert;
}

static int bt_l2cap_connect_handle(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    bk_err_t ert = kNoErr;
    do
    {
        if(kNoErr != (ert = rtos_init_semaphore(&bt_at_cmd_sema, 1)))
        {
            LOGE("semaphore init fail \r\n");
            break;
        }
        if(argc != 2)
        {
            ert = kParamErr;
            LOGE("Invalid parameters \r\n");
            break;
        }
        uint16_t psm = AT_DM_L2CAP_LOCAL_PSM;
        uint8_t addr[6] = {0xFF};
        #if 1
        if(1 != bt_at_params_analyze(argc, argv, "psm", &psm, 0))
        {
            LOGE("Parameters 1 error\r\n");
            ert = kParamErr;
            break;
        }
        #endif
        if(6 != bt_at_params_analyze(argc, argv, "addr", addr, 6))
        {
            LOGE("Parameters 2 error\r\n");
            ert = kParamErr;
            break;
        }
        for(int i=0;i<6;i++)
        {
            l2cap_env.remote_addr[i] = addr[5-i];
        }
        LOGI("Param 1 analyze :0x%x \r\n", psm);
        LOGI("Param 2 analyze :0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x \r\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
        if(l2cap_env.conn_state == STATE_ACL_DISCONNECT)
        {
            bk_bt_connect(l2cap_env.remote_addr,
                        CONNECTION_PACKET_TYPE,
                        CONNECTION_PAGE_SCAN_REPETITIOIN_MODE,
                        0,
                        CONNECTION_CLOCK_OFFSET,
                        1,
                        bt_at_cmd_cb);
            ert = bt_at_sema_get(AT_SYNC_CMD_TIMEOUT_MS*3);
            if(ert)
            {
                ert = kGeneralErr;
                LOGE("Connection time out \r\n");
                break;
            }
            if(at_cmd_status == BK_ERR_BT_SUCCESS)
            {
                l2cap_env.conn_state = STATE_ACL_CONNECTED;
                l2cap_env.handle = conn_handle;
            }else
            {
                ert = kGeneralErr;
                LOGE("Connection Fail, status:0x%x \r\n", at_cmd_status);
                break;
            }
        }
        bk_bt_l2cap_connect(BK_BT_L2CAP_SEC_NONE, psm, l2cap_env.remote_addr);
        uint8_t prev_state = l2cap_env.conn_state;
        l2cap_env.conn_state = STATE_L2CAP_CONNECTING;
        ert = bt_at_sema_get(AT_SYNC_CMD_TIMEOUT_MS);
        if(ert)
        {
            ert = kGeneralErr;
            l2cap_env.conn_state = prev_state;
            LOGE("L2cap connection time out \r\n");
            break;
        }
        if(l2cap_env.conn_state != STATE_L2CAP_CONNECTED)
        {
            ert = kGeneralErr;
            l2cap_env.conn_state = prev_state;
            break;
        }
    }
    while (0);
    if(ert!=kNoErr)
    {
        atsvr_cmd_rsp_error();
    }
        atsvr_cmd_rsp_ok();
    if (bt_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&bt_at_cmd_sema);
        bt_at_cmd_sema = NULL;
    }
    return ert;
}

static int bt_l2cap_disconnect_handle(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    bk_err_t ert = kNoErr;
    do{
        if(kNoErr != (ert = rtos_init_semaphore(&bt_at_cmd_sema, 1)))
        {
            LOGE("semaphore init fail \r\n");
            break;
        }
        if(argc != 1)
        {
            ert = kParamErr;
            LOGE("Invalid parameters \r\n");
            break;
        }
        uint16_t lcid = 0xFFFF;
        if(1 != bt_at_params_analyze(argc, argv, "lcid", &lcid, 0))
        {
            LOGE("Parameters 1 error\r\n");
            ert = kParamErr;
            break;
        }
        int find = -1;
        for (int i  = 0; i < sizeof(l2cap_env.fd)/sizeof(l2cap_env.fd[0]); ++i)
        {
            if((uint8_t)lcid == l2cap_env.fd[i])
            {
                find = i;
                break;
            }
        }
        if(find == -1)
        {
            ert = kGeneralErr;
            LOGE("Invalid lcid, no 0x%x channle connection \r\n", lcid);
            break;
        }
        bk_bt_l2cap_close(lcid);
        l2cap_env.conn_state = STATE_L2CAP_DISCONNECTING;
        ert = bt_at_sema_get(AT_DISCON_CMD_TIMEOUT_MS);
        if(ert)
        {
            ert = kGeneralErr;
            LOGE("L2cap disconnection time out \r\n");
            break;
        }
        if(l2cap_env.conn_state == STATE_L2CAP_DISCONNECTED)
        {
            l2cap_env.fd[find] = INVALID_LCID;
            l2cap_env.data_len[find] = 0;
            l2cap_env.tx_len[find] = 0;
            l2cap_env.rx_len[find] = 0;
            l2cap_env.tx_fun[find] = NULL;
            l2cap_env.fd_index--;
        }
        if(l2cap_env.fd_index == 0)
        {
            l2cap_env.conn_state = STATE_ACL_CONNECTED;
        }
        else
        {
            l2cap_env.conn_state = STATE_L2CAP_CONNECTED;
        }
    }while(0);
    if(ert!=kNoErr)
    {
        atsvr_cmd_rsp_error();
    }
	atsvr_cmd_rsp_ok();
    if (bt_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&bt_at_cmd_sema);
        bt_at_cmd_sema = NULL;
    }
    return ert;
}

static int bt_l2cap_stop_srv_handle(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    bk_err_t ert = kNoErr;
    do{
        uint16_t psm  = AT_DM_L2CAP_LOCAL_PSM;
        if(kNoErr != (ert = rtos_init_semaphore(&bt_at_cmd_sema, 1)))
        {
            LOGE("semaphore init fail \r\n");
            break;
        }
        if(argc !=1)
        {
            ert = kParamErr;
            LOGE("Invalid parameters number \r\n");
            break;
        }
        if(1!=bt_at_params_analyze(argc, argv, "psm", &psm, 0))
        {
            LOGE("Parameters 1 error\r\n");
            ert = kParamErr;
            break;
        }
        bk_bt_l2cap_stop_srv(psm);
        l2cap_env.conn_state = STATE_l2CAP_SRV_DISCONNECTING;
        ert = bt_at_sema_get(AT_DISCON_CMD_TIMEOUT_MS*2);
        if(ert)
        {
            ert = kGeneralErr;
            LOGE("L2cap stop srv time out, psm:0x%x \r\n", psm);
            break;
        }
        if(l2cap_env.fd_index == 0)
        {
            l2cap_env.conn_state = STATE_ACL_CONNECTED;
        }else
        {
            l2cap_env.conn_state = STATE_L2CAP_CONNECTED;
        }
    }while(0);
    if(ert!=kNoErr)
    {
		atsvr_cmd_rsp_error();

    }
	atsvr_cmd_rsp_ok();

    if (bt_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&bt_at_cmd_sema);
        bt_at_cmd_sema = NULL;
    }
    return ert;
}


static void l2cap_tx_fun(uint16_t lcid, uint16_t *data_len, uint16_t *tx_len)
{
    uint8_t *tmp_data = NULL;
    tmp_data = (uint8_t *)os_malloc(1024);
    int w = 0;
    uint16_t tt = 0;
    if(tmp_data == NULL) goto out;
    LOGI("%s, lcid:0x%x, len:%d \r\n", __func__, lcid, *data_len);
    if(*data_len)
    {
        uint8_t rand = bk_rand() & 0xff;
        w = 1024;
        tt = *data_len>w ? w:*data_len;
        os_memset(tmp_data, rand, tt);
        w = bk_bt_l2cap_write((int)lcid, tmp_data, tt);
        if(w >= 0)
        {
            *data_len -=w;
            *tx_len += w;
//            LOGI("L2cap write, data:0x%x , write_len:%d, total_len:%d \r\n", tmp_data[0], w, ww);
        }else
        {
            LOGE("L2cap write error, lcid:0x%x, write_len:%d \r\n", lcid, *tx_len);
            *data_len = 0;
            *tx_len = 0;
        }
    }
    if(*data_len == 0)
    {
        char *start = L2CAP_START_END;
        os_memcpy(tmp_data, start, os_strlen(start));
        w = bk_bt_l2cap_write((int)lcid, tmp_data , os_strlen(start));
        if(w<0) goto out;
        LOGI("======================> L2cap write end<====================== \r\n");
        LOGI("---> L2cap write, LCID:0x%x, total_len:%d \r\n", lcid, *tx_len);
        *tx_len = 0;
    }
    out:
    os_free(tmp_data);
}

static void bt_l2cap_tx_thread(void *arg)
{
    LOGI("%s start \r\n", __func__);
    while(l2cap_env.init)
    {
        uint32_t total_len = 0;
        for(int i=0;i<sizeof(l2cap_env.fd)/sizeof(l2cap_env.fd[0]);i++)
        {
            total_len += l2cap_env.tx_len[i];
            if(l2cap_env.fd[i] != INVALID_LCID && l2cap_env.tx_fun[i]!=NULL)
            {
                l2cap_env.tx_fun[i](l2cap_env.fd[i], &l2cap_env.data_len[i], &l2cap_env.tx_len[i]);
                if(l2cap_env.tx_len[i] == 0)
                {
                    l2cap_env.tx_fun[i]=NULL;
                }
            }
        }
        if(l2cap_env.fd_index == 0 || total_len == 0)
        {
            rtos_delay_milliseconds(1000);
        }else
        {
            rtos_delay_milliseconds(100);
        }
    }
    LOGI("%s exit \r\n", __func__);
    l2cap_env.l2cap_tx_thread = NULL;
    rtos_delete_thread(NULL);
}

static int bt_l2cap_tx_handle(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    bk_err_t ert = kNoErr;
    do{
        uint16_t lcid  = INVALID_LCID;
        uint16_t data_len = 0;
        if(argc !=2)
        {
            ert = kParamErr;
            LOGE("Invalid parameters number \r\n");
            break;
        }
        if(1!=bt_at_params_analyze(argc, argv, "lcid", &lcid, 0))
        {
            LOGE("Parameters 1 error\r\n");
            ert = kParamErr;
            break;
        }
        if(1!=bt_at_params_analyze(argc, argv, "data_length", &data_len, 0))
        {
            LOGE("Parameters 2 error\r\n");
            ert = kParamErr;
            break;
        }
        int i=0;
        for(;i<sizeof(l2cap_env.fd)/sizeof(l2cap_env.fd[0]);i++)
        {
            if(l2cap_env.fd[i] == lcid && l2cap_env.tx_len[i] == 0)
            {
                l2cap_env.data_len[i] = data_len;
                l2cap_env.tx_fun[i] = l2cap_tx_fun;
                break;
            }
        }
        if(i == sizeof(l2cap_env.fd)/sizeof(l2cap_env.fd[0]))
        {
            LOGE("No lcid foud for:0x%x !!!\r\n", lcid);
            ert = kParamErr;
            break;
        }
        if(l2cap_env.l2cap_tx_thread != NULL)
        {
            break;
        }
        rtos_create_thread(&l2cap_env.l2cap_tx_thread,
					5,
					"l2cap_tx_thread",
					(beken_thread_function_t)bt_l2cap_tx_thread,
					1024,
					(beken_thread_arg_t)0);

    }while(0);
    if(ert!=kNoErr)
    {
		atsvr_cmd_rsp_error();
    }
	atsvr_cmd_rsp_ok();
    return ert;
}

static int bt_l2cap_deinit_handle(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    bk_err_t ert = kNoErr;
    if(l2cap_env.init)
    {
        bk_bt_l2cap_stop_srv(AT_DM_L2CAP_LOCAL_PSM);
        bk_bt_l2cap_stop_srv(AT_DM_L2CAP_LOCAL_PSM+2);
        l2cap_env.init = 0;
    }
    else
    {
        ert = kParamErr;
        LOGE("L2cap do not init! \r\n");
    }
    if(ert!=kNoErr)
    {
		atsvr_cmd_rsp_error();
    }
	atsvr_cmd_rsp_ok();
    return ert;
}

#endif
/*******************************************************/


static int bt_start_inquiry_handle_gap(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    int err = kNoErr;

    bk_bt_gap_set_event_callback(bt_at_event_cb);

    err = bk_bt_gap_start_discovery(BK_BT_INQ_MODE_GENERAL_INQUIRY, 0x0A, 0);

    if (err)
    {
        goto error;
    }

	atsvr_cmd_rsp_ok();
    return err;
error:
	atsvr_cmd_rsp_error();
    return err;
}

static int bt_create_connection_handle_gap(int sync, int argc, char **argv)
{
    int err = kNoErr;
    if (argc < 1)
    {
        LOGE("Parameters error! \r\n");
        goto error;
    }

    bk_bt_gap_set_event_callback(bt_at_event_cb);
    bd_addr_t addr;
    uint8_t allow_role_switch = 0;
    err = bt_get_addr_from_param(&addr, argv[0]);
    if (err)
    {
        goto error;
    }
    if (argc == 2)
    {
        allow_role_switch = os_strtoul(argv[1], NULL, 16) & 0xFF;
    }

    err = bk_bt_gap_connect(addr.addr, allow_role_switch);
    if (err)
    {
        goto error;
    }
	atsvr_cmd_rsp_ok();
    return err;
error:
	atsvr_cmd_rsp_error();
    return err;
}

static int bt_disconnect_handle_gap(int sync, int argc, char **argv)
{
    int err = kNoErr;

    if (argc != 1)
    {
        LOGE("Parameters error! \r\n");
        goto error;
    }
    bk_bt_gap_set_event_callback(bt_at_event_cb);
    bd_addr_t addr;
    err = bt_get_addr_from_param(&addr, argv[0]);
    if (err)
    {
        goto error;
    }
    err = bk_bt_gap_disconnect(addr.addr, DISCONNECT_REASON);
    if (err)
    {
        goto error;
    }

	atsvr_cmd_rsp_ok();
    return err;
error:
	atsvr_cmd_rsp_error();
    if (bt_at_cmd_sema != NULL)
        rtos_deinit_semaphore(&bt_at_cmd_sema);
    return err;
}

static int bt_write_scan_enable_handle_gap(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    int err = kNoErr;

    if (argc != 1)
    {
        LOGE("Parameters error! \r\n");
        goto error;
    }

    bk_bt_gap_set_event_callback(bt_at_event_cb);
    uint8_t scan_enable = os_strtoul(argv[0], NULL, 10) & 0xFFFFFFFF;
    if (scan_enable > 0x03)
    {
        LOGE("%s para error, scan_enable:%d \r\n", __func__, scan_enable);
        goto error;
    }
    LOGE("scan_enable:%d \r\n", scan_enable);
    err = bk_bt_gap_set_scan_mode(scan_enable & 0x02, scan_enable & 0x01);
    if (err)
    {
        goto error;
    }
	atsvr_cmd_rsp_ok();
    return err;
error:
	atsvr_cmd_rsp_error();
    return err;
}

static int bt_read_scan_enable_handle_gap(int sync, int argc, char **argv)
{
    PRINT_FUNC;
    int err = kNoErr;

    bk_bt_gap_set_event_callback(bt_at_event_cb);

    err = bk_bt_gap_get_scan_mode();
    if (err)
    {
        goto error;
    }
	atsvr_cmd_rsp_ok();
    return err;
error:
	atsvr_cmd_rsp_error();
    return err;
}


const struct _atsvr_command bt_cmds_table[] = {
#if 1
    ATSVR_CMD_HADLER("AT+BTINQUIRY","start inquiry",
    				 NULL,bt_start_inquiry_handle,false,0,0,NULL,false),
    ATSVR_CMD_HADLER("AT+BTCONNECT","create connection", 
    				NULL,bt_create_connection_handle,false,0,0,NULL,false),
    ATSVR_CMD_HADLER("AT+BTDISCONNECT","disconnect", 
    				NULL,bt_disconnect_handle,true,5*1000,true,NULL,false),

    ATSVR_CMD_HADLER("AT+BTSPPCONNECT","spp connect", 
    				NULL,bt_spp_connect_handle,true,10*1000,true,NULL,false),

    ATSVR_CMD_HADLER("AT+BTWRITESCANENABLE","write_scan_enable",
    				NULL,bt_write_scan_enable_handle,true,0,0,NULL,false),
    ATSVR_CMD_HADLER("AT+BTREADSCANENABLE","read_scan_enable", 
    				NULL,bt_read_scan_enable_handle,true,0,true,NULL,false),
#ifdef CONFIG_AUD_INTF
    ATSVR_CMD_HADLER("AT+BTA2DPSINKTEST","enable a2dp sink test", 
    				NULL,bt_enable_a2dp_sink_test_handle,false,0,0,NULL,false),
    ATSVR_CMD_HADLER("AT+BTHFPUNITTEST","enable hfp unit test", 
    				NULL,bt_enable_hfp_unit_test_handle,false,0,0,NULL,false),
#endif
    ATSVR_CMD_HADLER("AT+BTOPPTEST","enable opp test", 
    				NULL,bt_enable_opp_test_handle,false,0,0,NULL,false),

#if CONFIG_AUD_INTF && CONFIG_A2DP_SOURCE_DEMO
    ATSVR_CMD_HADLER("AT+BTA2DPSOURCECONNECT","enable a2dp source connect", 
    				NULL,bt_enable_a2dp_source_connect_handle,true,0,true,NULL,false),
    ATSVR_CMD_HADLER("AT+BTA2DPSOURCEDISCONNECT","enable a2dp source disconnect", 
    				NULL,bt_enable_a2dp_source_disconnect_handle,true, 0,true,NULL,false),
    ATSVR_CMD_HADLER("AT+BTA2DPSOURCESTART", "enable a2dp source start",
    				NULL,bt_enable_a2dp_source_start_handle,true,0,true,NULL,false),
    ATSVR_CMD_HADLER("AT+BTA2DPSOURCESUSPEND","enable a2dp source suspend", 
    				NULL,bt_enable_a2dp_source_suspend_handle,true,0,true,NULL,false),
    ATSVR_CMD_HADLER("AT+BTA2DPSOURCEWRITE_TEST","enable a2dp source write test", 
    				NULL,bt_enable_a2dp_source_write_test_handle,true,0,0,NULL,false),
    ATSVR_CMD_HADLER("AT+BTA2DPSOURCESTOP","enable a2dp source stop", 
    				NULL,bt_enable_a2dp_source_stop_handle,true,0,0,NULL,false),
//  ATSVR_CMD_HADLER("AT+BT_A2DP_SOURCE_TEST","connect to soundbar and play mp3", bt_enable_a2dp_source_test_handle,false,0,0,NULL,false),
#endif
    ATSVR_CMD_HADLER("AT+BTSPPTX","spp_tx", 
    				NULL,bt_spp_tx_handle,true,AT_SYNC_CMD_TIMEOUT_MS,true,NULL,false),
    ATSVR_CMD_HADLER("AT+BTSPPINIT","spp_init", 
    				NULL,bt_spp_init_handle,false,0,0,NULL,false),
    ATSVR_CMD_HADLER("AT+BTSPPTHROUGHTEST","spp_through_test",
    				NULL,bt_spp_through_test_handle,true,0,true,NULL,false),
    ATSVR_CMD_HADLER("AT+BTA2DPSINKCONNECT","a2dp sink connect", 
    				NULL,bt_a2dp_sink_connect_handle,true,0,0,NULL,false),
    ATSVR_CMD_HADLER("AT+BTA2DPSINKDISCONNECT","a2dp sink disconnect",
    				NULL,bt_a2dp_sink_disconnect_handle,true,0,0,NULL,false),
#ifdef CONFIG_AUD_INTF
    ATSVR_CMD_HADLER("AT+BTAVRCPCTRL","avrcp ctrl", 
    				NULL,bt_avrcp_ctrl_handle,true,0,0,NULL,false),
#endif
#if CONFIG_A2DP_SOURCE_DEMO
//   ATSVR_CMD_HADLER("AT+BTA2DPSOURCETESTSTOP","connect to soundbar and play mp3", bt_enable_a2dp_source_test_stop_handl,false,0,0,NULL,false},
#endif
#if DM_L2CAP_CMD_ENABLE
    ATSVR_CMD_HADLER("AT+BTL2CAPINIT","l2cap init", 
    				NULL,bt_l2cap_init_handle,true,0,0,NULL,false),
    ATSVR_CMD_HADLER("AT+BTL2CAPCONNECT","l2cap connect", 
    				NULL,bt_l2cap_connect_handle,true,AT_SYNC_CMD_TIMEOUT_MS*4,true,NULL,false),
    ATSVR_CMD_HADLER("AT+BTL2CAPDISCONNECT","l2cap disconnect", 
    				NULL,bt_l2cap_disconnect_handle,true,AT_DISCON_CMD_TIMEOUT_MS,true,NULL,false),
    ATSVR_CMD_HADLER("AT+BTL2CAPTX", "l2cap tx", 
    				NULL,bt_l2cap_tx_handle,true,0,0,NULL,false),
    ATSVR_CMD_HADLER("AT+BTL2CAPSTOP","l2cap stop", 
    				NULL,bt_l2cap_stop_srv_handle,true,AT_DISCON_CMD_TIMEOUT_MS*2,true,NULL,false),
    ATSVR_CMD_HADLER("AT+BTL2CAPDEINIT","l2cap deinit", 
    				NULL,bt_l2cap_deinit_handle,true,0,0,NULL,false),
#endif
#ifdef CONFIG_AUD_INTF
    ATSVR_CMD_HADLER("AT+BTA2DPSINKDELAYVALUE","set delay reporting value",
    				bt_a2dp_sink_get_delay_value_handle,bt_a2dp_sink_set_delay_value_handle,true,0,0,NULL,false),
    ATSVR_CMD_HADLER("AT+BTA2DPSINKDELAY_VALUE?","get delay reporting value",
    				NULL,bt_a2dp_sink_get_delay_value_handle,true,0,0,NULL,false),
#endif
#else
    ATSVR_CMD_HADLER("AT+BTINQUIRY", "start inquiry", 
    				NULL,btstart_inquiry_handle_gap,false,0,0,NULL,false),
    ATSVR_CMD_HADLER("AT+BTCONNECT","create connection", 
    				NULL,bt_create_connection_handle_gap,false,0,0,NULL,false),
    ATSVR_CMD_HADLER("AT+BTDISCONNECT","disconnect", 
    				NULL,bt_disconnect_handle_gap,true,0,0,NULL,false),
    ATSVR_CMD_HADLER("AT+BTWRITE_SCAN_ENABLE","write_scan_enable", 
    				NULL,bt_write_scan_enable_handle_gap,true,0,0,NULL,false),
    ATSVR_CMD_HADLER("AT+BTREAD_SCAN_ENABLE","read_scan_enable", 
    				NULL,bt_read_scan_enable_handle_gap,true,0,0,NULL,false),
#endif
};

#define BT_AT_CNT sizeof(bt_cmds_table) / sizeof(bt_cmds_table[0])

void bt_at_cmd_init(void)
{
	int ret;
	ret = atsvr_register_commands(bt_cmds_table, sizeof(bt_cmds_table) / sizeof(bt_cmds_table[0]),"bt",NULL);
	if(0 == ret)
		BK_LOGI(TAG,"BT AT cmds init OK\r\n");
}

