// Copyright 2020-2024 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <string.h>

#include <os/os.h>
#include <driver/mailbox_channel.h>
#include <driver/mb_ipc.h>

#if CONFIG_CACHE_ENABLE
#include "cache.h"
#endif

#define MOD_TAG		"IPC"

#define DEBUG_MB_IPC	0

#if (CONFIG_CPU_CNT > 1)

#define CURRENT_CPU			SRC_CPU

#define IPC_CPU0_BIT		(0x01 << (MAILBOX_CPU0))
#define IPC_CPU1_BIT		(0x01 << (MAILBOX_CPU1))
#define IPC_CPU2_BIT		(0x01 << (MAILBOX_CPU2))

#define MB_IPC_RSP_FLAG		0x80
#define MB_IPC_CMD_MASK		0x7F

/* =================================
 *
 * port num configuration
 *
 * ===============================*/
#define SERVER_PORT_NUM		4   // max reserved server PORT num supported by this design is 15 (1~15).
#define MAX_CONNET_PER_SVR	3   // at most 3 clients could connect to one server. max support value is 4.
#define CLIENT_PORT_NUM		12  // max client PORT num supported by this design is 48 (16~63).

#define SERVER_PORT_MIN		1   // port 0 is invaid.
#define SERVER_PORT_MAX		(SERVER_PORT_MIN + SERVER_PORT_NUM - 1)   
#define SERVER_SOCKET_NUM	(SERVER_PORT_NUM * MAX_CONNET_PER_SVR)

#define CLIENT_PORT_MIN		16
#define CLIENT_PORT_MAX		(CLIENT_PORT_MIN + CLIENT_PORT_NUM - 1)
#define CLIENT_SOCKET_NUM	CLIENT_PORT_NUM   // CLIENT_PORT_NUM == SERVER_SOCKET_NUM, because 1-1 connection.

#define IPC_SOCKET_NUM		(SERVER_SOCKET_NUM + CLIENT_SOCKET_NUM)

#define IS_SERVER_PORT(port)      ( (port >= SERVER_PORT_MIN) && (port <= SERVER_PORT_MAX) )
#define IS_CLIENT_PORT(port)      ( (port >= CLIENT_PORT_MIN) && (port <= CLIENT_PORT_MAX) )

#define IS_NOT_SERVER_PORT(port)      ( (port < SERVER_PORT_MIN) || (port > SERVER_PORT_MAX) )
#define IS_NOT_CLIENT_PORT(port)      ( (port < CLIENT_PORT_MIN) || (port > CLIENT_PORT_MAX) )

#define IPC_PARAM1_MASK		0x00FFFFFF	/* used to get byte0~2 of param1. */

typedef union
{
	struct
	{
		mb_chnl_hdr_t    hdr;  // used internally by logical chnl driver.

		/* u32   param1; */  /* this param1 is used internally by router/API-implemant layer. */
		u8               dst_port      : 6;
		u8               dst_cpu       : 2;   // byte-0
		
		u8               src_port      : 6;
		u8               src_cpu       : 2;   // byte-1
		
		u8               tag              ;   // byte-2,.
		
		u8               api_impl_status  : 4;
		u8               route_status     : 4;   // byte-3, used for router/api-implementation layer status.
		
		/* param2, param3 are used for API call parameters. */
		/* u32   param2; */
		u32              cmd_data_len  : 16;
		u32              cmd_data_crc8 : 8;
		u32              user_cmd      : 8;      // NOTE: 0xFF is an invalid ID.
		
		/* u32   param3; */
		void           * cmd_data_buff;
	};

	mb_chnl_cmd_t	mb_cmd;
} mb_ipc_cmd_t;

#define SOCKET_CONNECT_ID_MASK		0x03

typedef union
{
	struct
	{
		u8      src_port : 6;
		u8      src_cpu  : 2;

		u8      connect_id  : 2;   /* used for server port. support max 4 connections to server. */
		u8      reserved1   : 5;
		u8      sock_valid  : 1;
		
		u16      reserved2;
	};
	
	u32		data;
} mb_ipc_socket_handle_t;

typedef struct
{
	u8             dst_cpus;
	u8             log_chnl;
	u16            chnl_tx_queue_out_idx;
	u16            chnl_tx_queue_in_idx;
	mb_ipc_cmd_t   chnl_tx_queue[IPC_SOCKET_NUM + 1]; // reserved 1 for queue empty judgement.
} mb_ipc_route_t;

#define CTRL_RX_SEMA_NOTIFY       0x02

#define STATE_TX_IN_PROCESS       0x01
#define STATE_RX_IN_PROCESS       0x02

#define USE_FLAG_ALLOCATED        0x01
#define USE_FLAG_CONNECTED        0x02

typedef u32 (* mb_ipc_notify_isr_t)(u32  handle, u32 connect_id);

typedef struct
{
	/* chnl data */
	u8					use_flag;     // USE_FLAG_xx

	u8                  dst_port : 6;
	u8                  dst_cpu  : 2;
	
	u8                  src_port : 6;
	u8                  src_cpu  : 2;

	u8                  ctrl_mode;   // CTRL_xx

	u8					run_state;   // STATE_xx, tx in process / rx in process.
	u8					rx_tag;
	u8                  tx_tag;

	/* tx cmd data */
	u8					tx_status;

	beken_semaphore_t	 tx_notify_sema;
	// notify sender when receiver has responded.(receiver has read the cmd data.)
	// because the rx/tx sides share the cmd buffer, 
	// so receiver must notify sender the buffer share completed.
	
	mb_ipc_cmd_t		tx_cmd;
		
	/* rx cmd data */
	u32					rx_read_offset;

	union
	{
		beken_semaphore_t	 rx_notify_sema;
		mb_ipc_notify_isr_t	 rx_notify_isr;     // notify receiver there is cmd data arrived.
	};

	mb_ipc_cmd_t		rx_cmd;

	// facility data.
	mb_ipc_socket_handle_t   handle;
} mb_ipc_socket_t;

static mb_ipc_route_t		ipc_route_tbl[] = 
	{
		#if CONFIG_SOC_SMP
		{.dst_cpus = (IPC_CPU2_BIT),                .log_chnl = CP2_MB_CHNL_IPC, 
			.chnl_tx_queue_out_idx = 0, .chnl_tx_queue_in_idx = 0,  },
		#else

		#if CONFIG_SYS_CPU0
		{.dst_cpus = (IPC_CPU1_BIT | IPC_CPU2_BIT), .log_chnl = CP1_MB_CHNL_IPC, 
			.chnl_tx_queue_out_idx = 0, .chnl_tx_queue_in_idx = 0,  },
		#endif

		#if CONFIG_SYS_CPU1
		{.dst_cpus = IPC_CPU0_BIT,                .log_chnl = CP0_MB_CHNL_IPC, 
			.chnl_tx_queue_out_idx = 0, .chnl_tx_queue_in_idx = 0,  },
		{.dst_cpus = IPC_CPU2_BIT,                .log_chnl = CP2_MB_CHNL_IPC, 
			.chnl_tx_queue_out_idx = 0, .chnl_tx_queue_in_idx = 0,  },
		#endif
		
		#endif

		#if CONFIG_SYS_CPU2
		#if !CONFIG_SYS_CPU2_COOP_WITH_SMP
		{.dst_cpus = (IPC_CPU0_BIT | IPC_CPU1_BIT), .log_chnl = CP1_MB_CHNL_IPC, 
			.chnl_tx_queue_out_idx = 0, .chnl_tx_queue_in_idx = 0,  },
		#else
		{.dst_cpus = (IPC_CPU0_BIT | IPC_CPU1_BIT), .log_chnl = CP0_MB_CHNL_IPC, 
			.chnl_tx_queue_out_idx = 0, .chnl_tx_queue_in_idx = 0,  },
		#endif
		#endif
	};

static mb_ipc_socket_t    ipc_server_socket_tbl[SERVER_PORT_NUM][MAX_CONNET_PER_SVR];
static mb_ipc_socket_t    ipc_client_socket_tbl[CLIENT_PORT_NUM];

static int      ipc_socket_rx_handler(mb_ipc_socket_t * ipc_socket, mb_ipc_cmd_t *ipc_cmd);
static void     ipc_socket_tx_cmpl_handler(mb_ipc_socket_t * ipc_socket, mb_ipc_cmd_t * ipc_cmd);
static mb_ipc_socket_t *   ipc_socket_search_sender(u8 src_port, u8 dst_cpu, u8 dst_port);
static mb_ipc_socket_t *   ipc_socket_search_recver(u8 dst_port, u8 src_cpu, u8 src_port);

/* polynomial = 0x31. */
static const u8 crc8_table[] =
{
0x00,0x31,0x62,0x53,0xc4,0xf5,0xa6,0x97,0xb9,0x88,0xdb,0xea,0x7d,0x4c,0x1f,0x2e,
0x43,0x72,0x21,0x10,0x87,0xb6,0xe5,0xd4,0xfa,0xcb,0x98,0xa9,0x3e,0x0f,0x5c,0x6d,
0x86,0xb7,0xe4,0xd5,0x42,0x73,0x20,0x11,0x3f,0x0e,0x5d,0x6c,0xfb,0xca,0x99,0xa8,
0xc5,0xf4,0xa7,0x96,0x01,0x30,0x63,0x52,0x7c,0x4d,0x1e,0x2f,0xb8,0x89,0xda,0xeb,
0x3d,0x0c,0x5f,0x6e,0xf9,0xc8,0x9b,0xaa,0x84,0xb5,0xe6,0xd7,0x40,0x71,0x22,0x13,
0x7e,0x4f,0x1c,0x2d,0xba,0x8b,0xd8,0xe9,0xc7,0xf6,0xa5,0x94,0x03,0x32,0x61,0x50,
0xbb,0x8a,0xd9,0xe8,0x7f,0x4e,0x1d,0x2c,0x02,0x33,0x60,0x51,0xc6,0xf7,0xa4,0x95,
0xf8,0xc9,0x9a,0xab,0x3c,0x0d,0x5e,0x6f,0x41,0x70,0x23,0x12,0x85,0xb4,0xe7,0xd6,
0x7a,0x4b,0x18,0x29,0xbe,0x8f,0xdc,0xed,0xc3,0xf2,0xa1,0x90,0x07,0x36,0x65,0x54,
0x39,0x08,0x5b,0x6a,0xfd,0xcc,0x9f,0xae,0x80,0xb1,0xe2,0xd3,0x44,0x75,0x26,0x17,
0xfc,0xcd,0x9e,0xaf,0x38,0x09,0x5a,0x6b,0x45,0x74,0x27,0x16,0x81,0xb0,0xe3,0xd2,
0xbf,0x8e,0xdd,0xec,0x7b,0x4a,0x19,0x28,0x06,0x37,0x64,0x55,0xc2,0xf3,0xa0,0x91,
0x47,0x76,0x25,0x14,0x83,0xb2,0xe1,0xd0,0xfe,0xcf,0x9c,0xad,0x3a,0x0b,0x58,0x69,
0x04,0x35,0x66,0x57,0xc0,0xf1,0xa2,0x93,0xbd,0x8c,0xdf,0xee,0x79,0x48,0x1b,0x2a,
0xc1,0xf0,0xa3,0x92,0x05,0x34,0x67,0x56,0x78,0x49,0x1a,0x2b,0xbc,0x8d,0xde,0xef,
0x82,0xb3,0xe0,0xd1,0x46,0x77,0x24,0x15,0x3b,0x0a,0x59,0x68,0xff,0xce,0x9d,0xac
};

u8 cal_crc8_0x31(u8 *data_buf, u16 len)
{
	u8    crc = 0x00;

	for (u32 i = 0; i < len; i++)
	{
		crc = crc8_table[crc ^ data_buf[i]];
	}

	return (crc);
}

#if CONFIG_SOC_SMP
#include "spinlock.h"
static SPINLOCK_SECTION volatile spinlock_t mb_ipc_spin_lock = SPIN_LOCK_INIT;
#endif // CONFIG_SOC_SMP
static inline uint32_t mb_ipc_enter_critical()
{
	uint32_t flags = rtos_disable_int();

#if CONFIG_SOC_SMP
	spin_lock(&mb_ipc_spin_lock);
#endif // CONFIG_SOC_SMP

	return flags;
}

static inline void mb_ipc_exit_critical(uint32_t flags)
{
#if CONFIG_SOC_SMP
	spin_unlock(&mb_ipc_spin_lock);
#endif // CONFIG_SOC_SMP

	rtos_enable_int(flags);
}

/**********************************************************************************************/
/****                                                                                      ****/
/****                                router layer                                          ****/
/****                                                                                      ****/
/**********************************************************************************************/

static mb_ipc_route_t * ipc_router_search_route(u8 dst_cpu, u32 * route_id)
{
	mb_ipc_route_t  * ipc_route = NULL;

	for(int i = 0; i < ARRAY_SIZE(ipc_route_tbl); i++)
	{
		if(ipc_route_tbl[i].dst_cpus & (0x01 << dst_cpu))
		{
			ipc_route = &ipc_route_tbl[i];
			*route_id = i;
			break;
		}
	}

	return ipc_route;
}

/*
 * because ipc_cmd is a pointer, 
 * it may pending in the tx_queue, 
 * so the memory ipc_cmd pointed must **NOT** be in stack (temporary memory).
 * 
 */
static int ipc_router_send(mb_ipc_route_t *ipc_route, mb_ipc_cmd_t *ipc_cmd)
{
	int   queue_cnt; // number of items in the queue.
	int   free_cnt;

	/* used to protect ipc_route->chnl_tx_queue_in_idx. */
	u32 temp = mb_ipc_enter_critical();

	if(ipc_route->chnl_tx_queue_in_idx >= ipc_route->chnl_tx_queue_out_idx)
	{
		queue_cnt = ipc_route->chnl_tx_queue_in_idx - ipc_route->chnl_tx_queue_out_idx;
		free_cnt  = ARRAY_SIZE(ipc_route->chnl_tx_queue) - queue_cnt;
	}
	else
	{
		free_cnt  = ipc_route->chnl_tx_queue_out_idx - ipc_route->chnl_tx_queue_in_idx;
		queue_cnt = ARRAY_SIZE(ipc_route->chnl_tx_queue) - free_cnt;
	}
	
	if( free_cnt <= 1 )  // reserved one for empty judgement.
	{
		mb_ipc_exit_critical(temp);
		
		return  IPC_ROUTE_QUEUE_FULL;
	}
	
	u16  tx_idx = ipc_route->chnl_tx_queue_in_idx;
	
	// memcpy(&ipc_route->chnl_tx_queue[tx_idx], ipc_cmd, sizeof(mb_ipc_cmd_t));
	ipc_route->chnl_tx_queue[tx_idx].mb_cmd.hdr.data = ipc_cmd->mb_cmd.hdr.data;
	ipc_route->chnl_tx_queue[tx_idx].mb_cmd.param1 = ipc_cmd->mb_cmd.param1;
	ipc_route->chnl_tx_queue[tx_idx].mb_cmd.param2 = ipc_cmd->mb_cmd.param2;
	ipc_route->chnl_tx_queue[tx_idx].mb_cmd.param3 = ipc_cmd->mb_cmd.param3;
	
	tx_idx = (tx_idx + 1) % ARRAY_SIZE(ipc_route->chnl_tx_queue);
	ipc_route->chnl_tx_queue_in_idx = tx_idx;

	mb_ipc_exit_critical(temp);  /* used to protect ipc_route->chnl_tx_queue_in_idx. */

	// it is the first item pushed to the queue, then try to send.
	if(queue_cnt == 0)
	{
		mb_chnl_write(ipc_route->log_chnl, &ipc_cmd->mb_cmd);
	}

	return IPC_ROUTE_STATUS_OK;
}

/* 
 * it MUST be protected by critical-section.
 * it is called by RX-ISR.
 */
#if !CONFIG_SOC_SMP
static int ipc_router_fwd(mb_ipc_cmd_t *ipc_cmd)
{
	u32               route_id;
	mb_ipc_route_t  * ipc_route = ipc_router_search_route(ipc_cmd->dst_cpu, &route_id);

	if(ipc_route == NULL)
	{
		// no route for this destination cpu.
		return IPC_ROUTE_UNREACHABLE;
	}

	// forward this cmd.
	return ipc_router_send(ipc_route, ipc_cmd);
}
#endif

static void ipc_router_rx_isr(void *param, mb_chnl_cmd_t *cmd_buf)
{
	mb_ipc_cmd_t     * ipc_cmd = (mb_ipc_cmd_t *)cmd_buf;

	#if !CONFIG_SOC_SMP
	/* please refer to <design document> P45 rx_isr 2). */
	// if it is a fwd cmd/rsp.
	if( ipc_cmd->dst_cpu != CURRENT_CPU )  // it is not for me, then forward it.
	{
		BK_ASSERT(CURRENT_CPU == MAILBOX_CPU1); // in current configuration, CPU1 used to forward cmd.

		ipc_cmd->route_status = ipc_router_fwd(ipc_cmd);

		return;
	}
	#endif

	// it is for me, process the cmd/rsp.

	// find the ipc_socket connection for the cmd/rsp.
	mb_ipc_socket_t * ipc_socket;
	ipc_socket = ipc_socket_search_recver(ipc_cmd->dst_port, ipc_cmd->src_cpu, ipc_cmd->src_port);

	ipc_cmd->route_status = ipc_socket_rx_handler(ipc_socket, ipc_cmd);

	#if DEBUG_MB_IPC
	if(ipc_cmd->route_status != IPC_ROUTE_STATUS_OK)
	{
		// ipc_socket may be == NULL, don't reference it.
		BK_LOGE(MOD_TAG, "rx error: cmd-%02x, param1=%08x!\r\n", 
			ipc_cmd->hdr.cmd, ipc_cmd->mb_cmd.param1);
	}
	#endif

	return;
}

static void ipc_router_tx_cmpl_isr(void *param, mb_chnl_ack_t *ack_buf)  /* tx_cmpl_isr */
{
	mb_ipc_cmd_t     * ipc_cmd = (mb_ipc_cmd_t *)ack_buf;
	u32                route_id = (u32)param;

	// data sanity check.
	if(route_id >= ARRAY_SIZE(ipc_route_tbl))
	{
		BK_LOGE(MOD_TAG, "%s error @%d, id=%d!\r\n", __FUNCTION__, __LINE__, route_id);
		return;   /* something wrong. */
	}

	// data sanity check.
	if(ipc_route_tbl[route_id].chnl_tx_queue_in_idx == ipc_route_tbl[route_id].chnl_tx_queue_out_idx)
	{
		// queue empty.
		BK_LOGE(MOD_TAG, "%s error @%d, indx=%d!\r\n", __FUNCTION__, __LINE__, 
			ipc_route_tbl[route_id].chnl_tx_queue_in_idx);
		
		return;   /* something wrong. */
	}

	u16  tx_idx = ipc_route_tbl[route_id].chnl_tx_queue_out_idx;
	
	mb_ipc_cmd_t * queue_cmd = &ipc_route_tbl[route_id].chnl_tx_queue[tx_idx];

	// data sanity check.
	if( (((queue_cmd->mb_cmd.param1 ^ ipc_cmd->mb_cmd.param1) & IPC_PARAM1_MASK) != 0)
		|| (queue_cmd->hdr.cmd != ipc_cmd->hdr.cmd)
		#if DEBUG_MB_IPC
		|| (queue_cmd->mb_cmd.param2 != ipc_cmd->mb_cmd.param2)
		|| (queue_cmd->mb_cmd.param3 != ipc_cmd->mb_cmd.param3) 
		#endif
		) /* param1, other fields not compared. */
	{
		BK_LOGE(MOD_TAG, "%s tx2 error @%d! %x != %x.\r\n", __FUNCTION__, __LINE__, 
			queue_cmd->mb_cmd.param1, ipc_cmd->mb_cmd.param1);

		#if DEBUG_MB_IPC
		BK_LOGE(MOD_TAG, "tx2 error: ipc-cmd-%02x, %x != %x.\r\n", ipc_cmd->hdr.cmd, 
			queue_cmd->mb_cmd.param2, ipc_cmd->mb_cmd.param2);
		BK_LOGE(MOD_TAG, "tx2 error: que-cmd-%02x, %x != %x.\r\n", queue_cmd->hdr.cmd, 
			queue_cmd->mb_cmd.param3, ipc_cmd->mb_cmd.param3);
		#endif

		return;   /* something wrong. */ /* not the CMD just sent. */
	}

	/* remove the first cmd from the queue. */
	tx_idx = (tx_idx + 1) % ARRAY_SIZE(ipc_route_tbl[route_id].chnl_tx_queue);
	ipc_route_tbl[route_id].chnl_tx_queue_out_idx = tx_idx;

	#if !CONFIG_SOC_SMP
	// it is a forwarded cmd/rsp.
	if( ipc_cmd->src_cpu != CURRENT_CPU )
	{
		if( (ipc_cmd->hdr.state & CHNL_STATE_COM_FAIL) || 
			(ipc_cmd->route_status != IPC_ROUTE_STATUS_OK) )
		{
			BK_LOGE(MOD_TAG, "Fwd failed, cmd %02x, param1=%08x!\r\n", 
				ipc_cmd->hdr.cmd, ipc_cmd->mb_cmd.param1);
		}

		goto tx_cmpl_isr_next_cmd;
	}
	#endif

	mb_ipc_socket_t * ipc_socket;
	
	// search the ipc_socket by port;
	ipc_socket = ipc_socket_search_sender(ipc_cmd->src_port, ipc_cmd->dst_cpu, ipc_cmd->dst_port);

	if(ipc_socket == NULL)
	{
		BK_LOGE(MOD_TAG, "%s tx error @%d!, cmd-%02x, param1=%08x!\r\n", __FUNCTION__, __LINE__, 
			ipc_cmd->hdr.cmd, ipc_cmd->mb_cmd.param1);

		goto tx_cmpl_isr_next_cmd;
	}

	ipc_socket_tx_cmpl_handler(ipc_socket, ipc_cmd);

tx_cmpl_isr_next_cmd:

	/* refer to <design document> P45 tx_cmpl_isr 3). */

	if(ipc_route_tbl[route_id].chnl_tx_queue_in_idx == ipc_route_tbl[route_id].chnl_tx_queue_out_idx)
		return;  // no pending cmd in the queue.

	queue_cmd = &ipc_route_tbl[route_id].chnl_tx_queue[tx_idx];

	// send next pending cmd. // refer to <design document> P45 tx_cmpl_isr 3).
	mb_chnl_write(ipc_route_tbl[route_id].log_chnl, &queue_cmd->mb_cmd);

	return;

}

/**********************************************************************************************/
/****                                                                                      ****/
/****                          API implementation layer                                    ****/
/****                                                                                      ****/
/**********************************************************************************************/

/* src_cpu must be CURRENT_CPU. */
static mb_ipc_socket_t * ipc_socket_search_sender(u8 src_port, u8 dst_cpu, u8 dst_port)
{
	mb_ipc_socket_t * ipc_socket = NULL;
	
	if( IS_SERVER_PORT(src_port) )
	{
		int svr_idx = src_port - SERVER_PORT_MIN;
		
		for(int i = 0; i < MAX_CONNET_PER_SVR; i++)
		{
			if(ipc_server_socket_tbl[svr_idx][i].dst_port != dst_port)
				continue;
			if(ipc_server_socket_tbl[svr_idx][i].dst_cpu != dst_cpu)
				continue;

			ipc_socket = &ipc_server_socket_tbl[svr_idx][i];
			
			// data sanity check.
			#if DEBUG_MB_IPC
			if( (ipc_socket->handle.connect_id != i) || 
				(ipc_socket->handle.src_port != src_port) || 
				(ipc_socket->handle.sock_valid == 0) )
			{
				BK_LOGE(MOD_TAG, "%s error @%d!\r\n", __FUNCTION__, __LINE__);
			}
			#endif
			
			break;
		}
	}
	else if( IS_CLIENT_PORT(src_port) )
	{
		ipc_socket = &ipc_client_socket_tbl[src_port - CLIENT_PORT_MIN];

		if( (ipc_socket->dst_cpu != dst_cpu) || (ipc_socket->dst_port != dst_port) )
		{
			return NULL;
		}

		#if DEBUG_MB_IPC
		// data sanity check.
		if( (ipc_socket->handle.src_port != src_port) || 
			(ipc_socket->handle.sock_valid == 0) )
		{
			BK_LOGE(MOD_TAG, "%s error @%d!\r\n", __FUNCTION__, __LINE__);
		}
		#endif
	}
	else
	{
		// invalid port number.
		return NULL;
	}

	return ipc_socket;
}

/* dst_cpu must be CURRENT_CPU. */
static mb_ipc_socket_t * ipc_socket_search_recver(u8 dst_port, u8 src_cpu, u8 src_port)
{
	mb_ipc_socket_t * ipc_socket = NULL;
	
	if( IS_SERVER_PORT(dst_port) )
	{
		int svr_idx = dst_port - SERVER_PORT_MIN;
		
		for(int i = 0; i < MAX_CONNET_PER_SVR; i++)
		{
			if(ipc_server_socket_tbl[svr_idx][i].dst_port != src_port)
				continue;
			if(ipc_server_socket_tbl[svr_idx][i].dst_cpu != src_cpu)
				continue;

			ipc_socket = &ipc_server_socket_tbl[svr_idx][i];
			
			// data sanity check.
			#if DEBUG_MB_IPC
			if( (ipc_socket->handle.connect_id != i) || 
				(ipc_socket->handle.src_cpu  != CURRENT_CPU) || 
				(ipc_socket->handle.src_port != dst_port) || 
				(ipc_socket->handle.sock_valid == 0) )
			{
				BK_LOGE(MOD_TAG, "%s error @%d!\r\n", __FUNCTION__, __LINE__);
			}
			#endif
			
			break;
		}
	}
	else if( IS_CLIENT_PORT(dst_port) )
	{
		ipc_socket = &ipc_client_socket_tbl[dst_port - CLIENT_PORT_MIN];

		if( (ipc_socket->dst_cpu != src_cpu) || (ipc_socket->dst_port != src_port) )
		{
			return NULL;
		}
		
		#if DEBUG_MB_IPC
		// data sanity check.
		if( (ipc_socket->handle.src_port != dst_port) || 
			(ipc_socket->handle.sock_valid == 0) )
		{
			BK_LOGE(MOD_TAG, "%s error @%d!\r\n", __FUNCTION__, __LINE__);
		}
		#endif
	}
	else
	{
		// invalid port number.
		return NULL;
	}

	return ipc_socket;
}

/* dst_cpu must be CURRENT_CPU. */
static mb_ipc_socket_t * ipc_socket_server_accept(u8 dst_port, u8 src_cpu, u8 src_port)
{
	mb_ipc_socket_t * ipc_socket = NULL;
	
	if( IS_SERVER_PORT(dst_port) )
	{
		int svr_idx = dst_port - SERVER_PORT_MIN;
		
		for(int i = 0; i < MAX_CONNET_PER_SVR; i++)
		{
			int svr_dst_port = ipc_server_socket_tbl[svr_idx][i].dst_port;
			
			if( IS_CLIENT_PORT(svr_dst_port) ) // connected with someone client.
				continue;

			ipc_socket = &ipc_server_socket_tbl[svr_idx][i];
			
			// data sanity check.
			#if DEBUG_MB_IPC
			if( (ipc_socket->src_port != dst_port) || 
				(ipc_socket->src_cpu  != CURRENT_CPU ) )
			{
				BK_LOGE(MOD_TAG, "%s error @%d!\r\n", __FUNCTION__, __LINE__);
			}
			#endif
			
			ipc_socket->dst_cpu  = src_cpu;
			ipc_socket->dst_port = src_port;

			ipc_socket->run_state = 0;
			ipc_socket->tx_status = 0;
			ipc_socket->rx_read_offset = 0;

			// data sanity check.
			#if DEBUG_MB_IPC
			if( (ipc_socket->handle.connect_id != i) || 
				(ipc_socket->handle.src_port != dst_port) || 
				(ipc_socket->handle.src_cpu  != CURRENT_CPU ) )
			{
				BK_LOGE(MOD_TAG, "%s error @%d!\r\n", __FUNCTION__, __LINE__);
			}
			#endif
			
			break;
		}
	}
	else
	{
		// dst_port is not server port.
		return NULL;
	}

	return ipc_socket;
}

static void ipc_socket_set_addr(mb_ipc_socket_t * ipc_socket, mb_ipc_cmd_t *ipc_cmd)
{
	ipc_cmd->dst_cpu  = ipc_socket->dst_cpu;
	ipc_cmd->dst_port = ipc_socket->dst_port;
	ipc_cmd->src_cpu  = ipc_socket->src_cpu;
	ipc_cmd->src_port = ipc_socket->src_port;
}

static int ipc_socket_local_tx_cmd(mb_ipc_socket_t * ipc_socket, mb_ipc_cmd_t *ipc_cmd);
static int ipc_socket_local_tx_rsp(mb_ipc_socket_t * ipc_socket, mb_ipc_cmd_t *ipc_cmd);

/*
 * because ipc_cmd is a pointer, 
 * it may pending in the tx_queue, 
 * so the memory ipc_cmd pointed must **NOT** be in stack (temporary memory).
 * 
 */
static int ipc_socket_tx_rsp(mb_ipc_socket_t * ipc_socket, mb_ipc_cmd_t *ipc_cmd)
{
	if(ipc_socket->dst_cpu == ipc_socket->src_cpu)
	{
		return ipc_socket_local_tx_rsp(ipc_socket, ipc_cmd);
	}
	
	u32               temp;
	mb_ipc_route_t  * ipc_route = ipc_router_search_route(ipc_socket->dst_cpu, &temp);

	if(ipc_route == NULL)
	{
		// no route for this destination cpu.
		return IPC_ROUTE_UNREACHABLE;
	}

	ipc_cmd->hdr.cmd |= MB_IPC_RSP_FLAG;   // set to rsp packet.
	
	ipc_socket_set_addr(ipc_socket, ipc_cmd);

	uint32_t flags = rtos_disable_int();
	
	int route_status = ipc_router_send(ipc_route, ipc_cmd);

	if(route_status != IPC_ROUTE_STATUS_OK)
	{
		ipc_socket->run_state &= ~STATE_RX_IN_PROCESS;  // clear rx_in_process.
	}

	rtos_enable_int(flags);

	return route_status;
}

/*
 * because ipc_cmd is a pointer, 
 * it may pending in the tx_queue, 
 * so the memory ipc_cmd pointed must **NOT** be in stack (temporary memory).
 * 
 */
static int ipc_socket_tx_cmd(mb_ipc_socket_t * ipc_socket, mb_ipc_cmd_t *ipc_cmd, u32 retry)
{
	if(retry == 0)
	{
		ipc_socket->tx_tag++;
	}
	ipc_cmd->tag = ipc_socket->tx_tag;
	
	if(ipc_socket->dst_cpu == ipc_socket->src_cpu)
	{
		return ipc_socket_local_tx_cmd(ipc_socket, ipc_cmd);
	}
	
	u32               temp;
	mb_ipc_route_t  * ipc_route = ipc_router_search_route(ipc_socket->dst_cpu, &temp);

	if(ipc_route == NULL)
	{
		// no route for this destination cpu.
		return IPC_ROUTE_UNREACHABLE;
	}

	ipc_socket_set_addr(ipc_socket, ipc_cmd);

	uint32_t flags = rtos_disable_int();
	
	int route_status = ipc_router_send(ipc_route, ipc_cmd);

	if(route_status == IPC_ROUTE_STATUS_OK)
	{
		ipc_socket->run_state |= STATE_TX_IN_PROCESS;
	}

	rtos_enable_int(flags);

	return route_status;
}

static void ipc_socket_rx_notify(mb_ipc_socket_t * ipc_socket)
{
	/* rx notification. */
	if(ipc_socket->ctrl_mode & CTRL_RX_SEMA_NOTIFY)
	{
		rtos_set_semaphore(&ipc_socket->rx_notify_sema);
	}
	else
	{
		ipc_socket->rx_notify_isr(ipc_socket->handle.data, ipc_socket->handle.connect_id);
	}
}

static void ipc_socket_tx_cmpl_notify(mb_ipc_socket_t * ipc_socket)
{
	ipc_socket->run_state &= ~STATE_TX_IN_PROCESS;  // clear tx_in_process.

	/* tx_complete notification. */
	rtos_set_semaphore(&ipc_socket->tx_notify_sema);
}

static void ipc_socket_tx_cmpl_handler(mb_ipc_socket_t * ipc_socket, mb_ipc_cmd_t * ipc_cmd)
{
	// if it is a rsp.
	/* refer to <design document> P44 tx_cmpl_isr 1). */
	if( ipc_cmd->hdr.cmd & MB_IPC_RSP_FLAG )
	{
		if( (ipc_cmd->hdr.state & CHNL_STATE_COM_FAIL) || 
			(ipc_cmd->route_status != IPC_ROUTE_STATUS_OK) )
		{
			BK_LOGE(MOD_TAG, "Rsp failed, cmd %02x, param1=%08x!\r\n", 
				ipc_cmd->hdr.cmd, ipc_cmd->mb_cmd.param1);
		}

		// completed the rx-process, so clear the flag.
		ipc_socket->run_state &= ~STATE_RX_IN_PROCESS;  // clear rx_in_process.

		if(ipc_cmd->hdr.cmd == (MB_IPC_RSP_FLAG | MB_IPC_DISCONNECT_CMD)) // disconnect rsp send ok.
		{
			ipc_socket->dst_cpu  = 0;
			ipc_socket->dst_port = 0;  // clear the connection.
		}
		
		return;
	}

	// it is a cmd.

	// queue_cmd points to the tx_cmd, ipc_cmd must match the current state of tx_cmd.
	mb_ipc_cmd_t * queue_cmd = &ipc_socket->tx_cmd;

	// data sanity check.
	if( (((queue_cmd->mb_cmd.param1 ^ ipc_cmd->mb_cmd.param1) & IPC_PARAM1_MASK) != 0)
		|| (queue_cmd->hdr.cmd != ipc_cmd->hdr.cmd)
		#if DEBUG_MB_IPC
		|| (queue_cmd->mb_cmd.param2 != ipc_cmd->mb_cmd.param2)
		|| (queue_cmd->mb_cmd.param3 != ipc_cmd->mb_cmd.param3) 
		#endif
		) /* param1, other fields not compared. */
	{
		BK_LOGE(MOD_TAG, "%s tx2 error @%d! %x != %x.\r\n", __FUNCTION__, __LINE__, 
			queue_cmd->mb_cmd.param1, ipc_cmd->mb_cmd.param1);

		#if DEBUG_MB_IPC
		BK_LOGE(MOD_TAG, "tx2 error: ipc-cmd-%02x, %x != %x.\r\n", ipc_cmd->hdr.cmd, 
			queue_cmd->mb_cmd.param2, ipc_cmd->mb_cmd.param2);
		BK_LOGE(MOD_TAG, "tx2 error: que-cmd-%02x, %x != %x.\r\n", queue_cmd->hdr.cmd, 
			queue_cmd->mb_cmd.param3, ipc_cmd->mb_cmd.param3);
		#endif

		return;   /* something wrong. */ /* not the CMD just sent. */
	}
	
	// if send failed.
	/* refer to <design document> P44 tx_cmpl_isr 2). */
	if( (ipc_cmd->hdr.state & CHNL_STATE_COM_FAIL) || 
		(ipc_cmd->route_status != IPC_ROUTE_STATUS_OK) )
	{

		if(ipc_cmd->hdr.state & CHNL_STATE_COM_FAIL)
		{
			// mailbox channel tx failed. 
			ipc_socket->tx_status = MB_IPC_TX_FAILED;
		}
		else  // (ipc_cmd->route_status != IPC_ROUTE_STATUS_OK)
		{
			// router failed.
			ipc_socket->tx_status = MB_IPC_ROUTE_BASE_FAILED + ipc_cmd->route_status;
		}

		ipc_socket_tx_cmpl_notify(ipc_socket);

		return;
	}

}

static int ipc_socket_rx_rsp_handler(mb_ipc_socket_t * ipc_socket, mb_ipc_cmd_t *ipc_cmd)
{
	if(ipc_socket == NULL)
	{
		// no connection for this rsp.
		return IPC_ROUTE_UNREACHABLE;
	}

	if( (ipc_socket->run_state & STATE_TX_IN_PROCESS) == 0 )	/* unsolicited ipc response. */
	{
		return IPC_ROUTE_UNSOLICITED_RSP;
	}

	if(ipc_socket->tx_tag != ipc_cmd->tag)
	{
		/* un-matched response. */
		return IPC_ROUTE_UNMATCHED_RSP;
	}

	if(ipc_socket->tx_cmd.hdr.cmd != (ipc_cmd->hdr.cmd & MB_IPC_CMD_MASK))
	{
		/* un-matched response. */
		return IPC_ROUTE_UNMATCHED_RSP;
	}

	// data sanity check.
	if( (ipc_socket->tx_cmd.tag != ipc_cmd->tag)
		#if DEBUG_MB_IPC
		|| (ipc_socket->tx_cmd.mb_cmd.param2 != ipc_cmd->mb_cmd.param2)
		|| (ipc_socket->tx_cmd.mb_cmd.param3 != ipc_cmd->mb_cmd.param3) 
		#endif
		) /* param1, other fields not compared. */
	{
		#if DEBUG_MB_IPC
		BK_LOGE(MOD_TAG, "%s error @%d, %d != %d.\r\n", __FUNCTION__, __LINE__, 
			ipc_socket->tx_cmd.tag, ipc_cmd->tag);
		#endif

		/* not the response for the CMD just sent. */
		return IPC_ROUTE_UNMATCHED_RSP;
	}

	#if 0   // can't clear connection here, becasue tx_cmpl_isr will use this connection again.
	        // if there is direct connection between 2 cores, rsp will arrive ahead of cmd-tx_cmpl_isr.
	if(ipc_cmd->hdr.cmd == (MB_IPC_RSP_FLAG | MB_IPC_DISCONNECT_CMD)) // disconnected.
	{
		// it must have been disconnected in the peer side.
		ipc_socket->dst_cpu  = 0;
		ipc_socket->dst_port = 0;  // clear the connection.
	}
	#endif

	/* save cmd return-status, tx is completed. */
	if(ipc_cmd->api_impl_status != IPC_API_IMPL_STATUS_OK)
		ipc_socket->tx_status = MB_IPC_API_BASE_FAILED + ipc_cmd->api_impl_status;
	else
		ipc_socket->tx_status = MB_IPC_TX_OK;

	ipc_socket_tx_cmpl_notify(ipc_socket);

	return IPC_ROUTE_STATUS_OK;  // this status return is worthless but for debug.
}

static void ipc_socket_rx_cmd_error_handler(mb_ipc_socket_t * ipc_socket, int error_code)
{
	ipc_socket->rx_cmd.api_impl_status = error_code & 0x0F;

	ipc_socket_tx_rsp(ipc_socket, &ipc_socket->rx_cmd);
}

// connect command handler, it is handled internally.
static int ipc_socket_rx_cmd_connect(mb_ipc_socket_t * ipc_socket)
{
	ipc_socket->use_flag  |= USE_FLAG_CONNECTED;
	ipc_socket->run_state &= ~STATE_TX_IN_PROCESS;  // must have no cmd in sending, clear tx_in_process to be sure. 
	ipc_socket->tx_status = 0;  // must have no cmd in sending, so clear the tx status too.

	ipc_socket_tx_rsp(ipc_socket, &ipc_socket->rx_cmd);

	return -1;  // don't notify upper layer for further process.
}

// disconnect command handler, it is handled internally.
static int ipc_socket_rx_cmd_disconnect(mb_ipc_socket_t * ipc_socket)
{
	if( (ipc_socket->run_state & STATE_TX_IN_PROCESS) != 0 )
	{
		ipc_socket->tx_status = MB_IPC_DISCONNECTED;

		/* tx may be failed. disconnect notification. */
		ipc_socket_tx_cmpl_notify(ipc_socket);
	}

	ipc_socket->use_flag  &= ~USE_FLAG_CONNECTED;
	ipc_socket->run_state &= ~STATE_TX_IN_PROCESS;    // cleared TX_IN_PROCESS.

	ipc_socket_tx_rsp(ipc_socket, &ipc_socket->rx_cmd);

	if(IS_CLIENT_PORT(ipc_socket->src_port))
	{
	//	ipc_socket_rx_notify(ipc_socket);
		return 0;  // need to notify upper layer for further process.
		// if it is a client, notify app to close socket to free resources.
		// server must NOT close socket. it will wait for new connection.
	}
	
	return -1;  // don't notify upper layer for further process.

}

static int ipc_socket_rx_cmd_default_handler(mb_ipc_socket_t * ipc_socket)
{
	// connect/disconnect commands don't check use_flag_connected & wait_tag.
	// other commands need these checks.
	if( (ipc_socket->use_flag & USE_FLAG_CONNECTED) == 0 )
	{
		ipc_socket_rx_cmd_error_handler(ipc_socket, IPC_API_IMPL_RX_NOT_CONNECT);
		
		return -1;  // need not to notify upper layer.
	}

	if(ipc_socket->rx_tag == ipc_socket->rx_cmd.tag)
	{
		// coomand has processed in previously recv, it is a re-sent cmd.
		ipc_socket_tx_rsp(ipc_socket, &ipc_socket->rx_cmd);

		return -1;  // need not to notify upper layer.
	}
	
//	ipc_socket_rx_notify(ipc_socket);

	return 0;  // need to notify upper layer for further process.
}

static int ipc_socket_rx_cmd_handler(mb_ipc_socket_t * ipc_socket, mb_ipc_cmd_t *ipc_cmd)
{
	if(ipc_socket == NULL)
	{
		if(ipc_cmd->hdr.cmd == MB_IPC_CONNECT_CMD)  // it is a connect cmd, not includes the connect response.
		{
			// try to establish the connection.
			u32 temp = mb_ipc_enter_critical();
			ipc_socket = ipc_socket_server_accept(ipc_cmd->dst_port, ipc_cmd->src_cpu, ipc_cmd->src_port);
			mb_ipc_exit_critical(temp);
			if(ipc_socket == NULL)
			{
				// no connection for this cmd.
				return IPC_ROUTE_UNREACHABLE;
			}
		}
		else
		{
			// no connection for this cmd.
			return IPC_ROUTE_UNREACHABLE;
		}
	}

	int route_status = IPC_ROUTE_STATUS_OK;  // return to indicate that cmd has reached target. 

	if( ipc_socket->run_state & STATE_RX_IN_PROCESS )	/* has a cmd uncompleted. */
	{
		ipc_cmd->api_impl_status = IPC_API_IMPL_RX_BUSY;

		#if 0
		// send rsp to fail this cmd.
		ipc_socket_tx_rsp(ipc_socket, ipc_cmd);
		      /* can't call ipc_socket_tx_rsp, because ipc_cmd point to stack memory. */
		#else
		route_status = IPC_ROUTE_RX_BUSY;
		     /*             !!! NOTE !!!                     */
			 /*       no way to report this fail.            */
		     /* it is a temporary solution to indicate fail. */
			 /* actually, it is a API layer fail,            */
			 /* should return fail state in api_impl_status. */
		#endif
	}
	else  /* it is OK to receive the cmd. */
	{
		// save the cmd for further processing.
		// memcpy(&ipc_socket->rx_cmd, ipc_cmd, sizeof(mb_ipc_cmd_t));
		ipc_socket->rx_cmd.mb_cmd.hdr.data = ipc_cmd->mb_cmd.hdr.data;
		ipc_socket->rx_cmd.mb_cmd.param1 = ipc_cmd->mb_cmd.param1;
		ipc_socket->rx_cmd.mb_cmd.param2 = ipc_cmd->mb_cmd.param2;
		ipc_socket->rx_cmd.mb_cmd.param3 = ipc_cmd->mb_cmd.param3;
		
		ipc_socket->rx_read_offset = 0;
		ipc_socket->run_state |= STATE_RX_IN_PROCESS;

		if( (ipc_socket->use_flag & USE_FLAG_ALLOCATED) == 0 )  // socket must have been initialized.
		{
			ipc_socket_rx_cmd_error_handler(ipc_socket, IPC_API_IMPL_NOT_INITED);
			
			return IPC_ROUTE_STATUS_OK;
		}

		ipc_socket->rx_cmd.api_impl_status = IPC_API_IMPL_STATUS_OK;  // clear the status before providing to upper layer.

		int  ret_val = 0;
		
		/* rx cmd process. */
		switch(ipc_socket->rx_cmd.hdr.cmd)
		{
			case MB_IPC_CONNECT_CMD:
				ret_val = ipc_socket_rx_cmd_connect(ipc_socket);
				break;

			case MB_IPC_DISCONNECT_CMD:
				ret_val = ipc_socket_rx_cmd_disconnect(ipc_socket);
				break;

			case MB_IPC_SEND_CMD:
				// no special process, go through to default handler.
				// break;
				
			default:
				ret_val = ipc_socket_rx_cmd_default_handler(ipc_socket);
				break;
		}

		ipc_socket->rx_tag = ipc_socket->rx_cmd.tag;

		if(ret_val == 0)  // need to notify app for further process.
			ipc_socket_rx_notify(ipc_socket);
		
		route_status = IPC_ROUTE_STATUS_OK;  // return to indicate that cmd has reached target.
		// must return IPC_ROUTE_STATUS_OK !!!!
		// because the communication is ok, the cmd result will be returned in RSP package.
	}

	return route_status;
}

static int ipc_socket_rx_handler(mb_ipc_socket_t * ipc_socket, mb_ipc_cmd_t *ipc_cmd)
{
	/* please refer to <design document> P45 rx_isr 3). */
	// it is a rsp, processed in API-implementation layer.
	if(ipc_cmd->hdr.cmd & MB_IPC_RSP_FLAG)  /* ipc rsp from other CPU. */
	{
		// this status return is worthless but for debug.
		return ipc_socket_rx_rsp_handler(ipc_socket, ipc_cmd);
	}
	else   // it is a cmd. 
	{
		/* please refer to <design document> P45 rx_isr 4). */

		return ipc_socket_rx_cmd_handler(ipc_socket, ipc_cmd);
	}
}

static int ipc_socket_local_tx_cmd(mb_ipc_socket_t * ipc_socket, mb_ipc_cmd_t *ipc_cmd)
{
	mb_ipc_socket_t * tx_ipc_socket = ipc_socket;
	
	mb_ipc_cmd_t	  rx_cmd;

	// simulate the mailbox tx and rx process.
	
	// simulate local tx, (set tx flag). 
	// always tx successfully, because target resides in local cpu.
	ipc_socket->run_state |= STATE_TX_IN_PROCESS;

	ipc_socket_set_addr(ipc_socket, ipc_cmd);

	// simulate the target rx. 
	// copy mailbox data from device registers to statck variable.
	// memcpy(&rx_cmd, ipc_cmd, sizeof(rx_cmd));  // receive the cmd.
	rx_cmd.mb_cmd.hdr.data = ipc_cmd->mb_cmd.hdr.data;
	rx_cmd.mb_cmd.param1 = ipc_cmd->mb_cmd.param1;
	rx_cmd.mb_cmd.param2 = ipc_cmd->mb_cmd.param2;
	rx_cmd.mb_cmd.param3 = ipc_cmd->mb_cmd.param3;

	ipc_cmd = &rx_cmd;

	// find the ipc_socket connection for the cmd.
	ipc_socket = ipc_socket_search_recver(ipc_cmd->dst_port, ipc_cmd->src_cpu, ipc_cmd->src_port);

	ipc_cmd->route_status = ipc_socket_rx_cmd_handler(ipc_socket, ipc_cmd);

	// simulate the local tx complete.
	ipc_socket_tx_cmpl_handler(tx_ipc_socket, ipc_cmd);

	// always return tx success, cmd fail is handled in cmd tx_cmpl_handler.
	return IPC_ROUTE_STATUS_OK;
}

static int ipc_socket_local_tx_rsp(mb_ipc_socket_t * ipc_socket, mb_ipc_cmd_t *ipc_cmd)
{
	mb_ipc_socket_t * tx_ipc_socket = ipc_socket;
	
	mb_ipc_cmd_t	  rx_cmd;

	// simulate the mailbox tx and rx process.

	ipc_socket_set_addr(ipc_socket, ipc_cmd);

	ipc_cmd->hdr.cmd |= MB_IPC_RSP_FLAG;   // set to rsp packet.
	
	// simulate the target rx. 
	// copy mailbox data from device registers to statck variable.
	// memcpy(&rx_cmd, ipc_cmd, sizeof(rx_cmd));  // receive the rsp.
	rx_cmd.mb_cmd.hdr.data = ipc_cmd->mb_cmd.hdr.data;
	rx_cmd.mb_cmd.param1 = ipc_cmd->mb_cmd.param1;
	rx_cmd.mb_cmd.param2 = ipc_cmd->mb_cmd.param2;
	rx_cmd.mb_cmd.param3 = ipc_cmd->mb_cmd.param3;

	ipc_cmd = &rx_cmd;

	// find the ipc_socket connection for the rsp.
	ipc_socket = ipc_socket_search_recver(ipc_cmd->dst_port, ipc_cmd->src_cpu, ipc_cmd->src_port);

	// must clear this flag before call peer side rx_rsp_handler.
	// because, perr side app may send another cmd immediately as soon as tx_in_process is cleared.
	// if not clear rx_in_process in this side in advances, peer side task may send fail.
	// tx_ipc_socket->run_state &= ~STATE_RX_IN_PROCESS;  // clear rx_in_process.

	// simulate the local tx complete. (will clear STATE_RX_IN_PROCESS in this function)
	ipc_socket_tx_cmpl_handler(tx_ipc_socket, ipc_cmd);

	ipc_cmd->route_status = ipc_socket_rx_rsp_handler(ipc_socket, ipc_cmd);
	// (ipc_cmd->route_status) this status return is worthless but for debug.
	// so can clear STATE_RX_IN_PROCESS of tx_ipc_socket->run_state in advance.

	// always return tx success, other fails are handled in tx_cmpl_handler.
	return IPC_ROUTE_STATUS_OK;
}

/************************************************************************/
/****                mb_ipc_socket APIs                              ****/
/************************************************************************/

static u32 get_handle_from_socket(mb_ipc_socket_t * ipc_socket, u8 idx)
{
	mb_ipc_socket_handle_t   ipc_handle;

	ipc_handle.data = 0;

	ipc_handle.src_cpu  = ipc_socket->src_cpu;
	ipc_handle.src_port = ipc_socket->src_port;

	ipc_handle.connect_id = (idx & SOCKET_CONNECT_ID_MASK);
	ipc_handle.sock_valid = 1;
	
	return ipc_handle.data;
}

static mb_ipc_socket_t * get_socket_from_handle(u32 handle)
{
	mb_ipc_socket_t * ipc_socket = NULL;
	mb_ipc_socket_handle_t   ipc_handle;

	ipc_handle.data = handle;

	// data sanity check.
	if(ipc_handle.sock_valid == 0)
	{
		return NULL;
	}
	
	#if !CONFIG_SOC_SMP
	if(ipc_handle.src_cpu != CURRENT_CPU)
	{
		return NULL;
	}
	#endif

	u8     src_port = ipc_handle.src_port;
	
	if( IS_SERVER_PORT(src_port) )
	{
		if(ipc_handle.connect_id >= MAX_CONNET_PER_SVR)
		{
			return NULL;
		}
		
		ipc_socket = &ipc_server_socket_tbl[src_port - SERVER_PORT_MIN][ipc_handle.connect_id];
	}
	else if( IS_CLIENT_PORT(src_port) )
	{
		ipc_socket = &ipc_client_socket_tbl[src_port - CLIENT_PORT_MIN];
	}
	else
	{
		// invalid port number.
		return NULL;
	}

	return ipc_socket;
}

static void init_client_socket(void)
{
	memset(&ipc_client_socket_tbl[0], 0, sizeof(ipc_client_socket_tbl));
	
	for(int i = 0; i < CLIENT_PORT_NUM; i++)
	{
		ipc_client_socket_tbl[i].src_cpu  = CURRENT_CPU;
		ipc_client_socket_tbl[i].src_port = CLIENT_PORT_MIN + i;
		ipc_client_socket_tbl[i].handle.data = get_handle_from_socket(&ipc_client_socket_tbl[i], 0);
	}
}

static void init_server_socket(void)
{
	memset(&ipc_server_socket_tbl[0][0], 0, sizeof(ipc_server_socket_tbl));
	
	for(int i = 0; i < SERVER_PORT_NUM; i++)
	{
		for(int j = 0; j < MAX_CONNET_PER_SVR; j++)
		{
			ipc_server_socket_tbl[i][j].src_cpu  = CURRENT_CPU;
			ipc_server_socket_tbl[i][j].src_port = SERVER_PORT_MIN + i;
			ipc_server_socket_tbl[i][j].handle.data = get_handle_from_socket(&ipc_server_socket_tbl[i][j], (u8)j);
		}
	}
}

static u8 ipc_allocate_port(u8 port)
{
	int   port_idx = 0;
	
	if(port == 0)  // invalid port number.
	{
		// allocate client port.
		for(port_idx = 0; port_idx < CLIENT_PORT_NUM; port_idx++)
		{
			if(ipc_client_socket_tbl[port_idx].use_flag == 0 )
			{
				port = port_idx + CLIENT_PORT_MIN;
				
				return port;
			}
		}

		return 0;
	}

	if( IS_CLIENT_PORT(port) )
	{
		port_idx = port - CLIENT_PORT_MIN;

		if(ipc_client_socket_tbl[port_idx].use_flag != 0 )
		{
			return 0;
		}
	}
	else if( IS_SERVER_PORT(port) )
	{
		port_idx = port - SERVER_PORT_MIN;
		
		if(ipc_server_socket_tbl[port_idx][0].use_flag != 0)
		{
			return 0;
		}
	}
	else
	{
		// invalid port number.
		return 0;
	}

	return port;
}

u32 mb_ipc_socket(u8 port, void * rx_callback)
{
	mb_ipc_socket_handle_t   ipc_handle;

	ipc_handle.data = 0;

	int   port_idx = 0;

	beken_semaphore_t  tx_semaphore = NULL;
	beken_semaphore_t  rx_semaphore = NULL;

	rtos_init_semaphore(&tx_semaphore, 1);
	
	if(tx_semaphore == NULL)
	{
		return ipc_handle.data;
	}

	if(rx_callback == NULL)  // not callback notification
	{
		rtos_init_semaphore(&rx_semaphore, 1);
		
		if(rx_semaphore == NULL)
		{
			rtos_deinit_semaphore(&tx_semaphore);
			
			return ipc_handle.data;
		}
	}

	u32   temp = mb_ipc_enter_critical();
	
	port = ipc_allocate_port(port);

	if(port == 0)
	{
		mb_ipc_exit_critical(temp);

		rtos_deinit_semaphore(&tx_semaphore);
		if(rx_semaphore != NULL)
		{
			rtos_deinit_semaphore(&rx_semaphore);
		}

		return ipc_handle.data;
	}

	if( IS_CLIENT_PORT(port) )
	{
		port_idx = port - CLIENT_PORT_MIN;

		ipc_client_socket_tbl[port_idx].tx_notify_sema = tx_semaphore;

		if(rx_callback == NULL)
		{
			ipc_client_socket_tbl[port_idx].ctrl_mode |= CTRL_RX_SEMA_NOTIFY; // semaphore notification.
			ipc_client_socket_tbl[port_idx].rx_notify_sema = rx_semaphore;
		}
		else
		{
			ipc_client_socket_tbl[port_idx].ctrl_mode &= ~CTRL_RX_SEMA_NOTIFY; // callback notification.
			ipc_client_socket_tbl[port_idx].rx_notify_isr = (mb_ipc_notify_isr_t)rx_callback;
		}
		
		ipc_client_socket_tbl[port_idx].use_flag = USE_FLAG_ALLOCATED;
		ipc_handle.data = ipc_client_socket_tbl[port_idx].handle.data;
		
	}
	else if( IS_SERVER_PORT(port) )
	{
		port_idx = port - SERVER_PORT_MIN;
		
		// the same setting for all clients connected to this server.
		for(int j = 0; j < MAX_CONNET_PER_SVR; j++)
		{
			ipc_server_socket_tbl[port_idx][j].tx_notify_sema = tx_semaphore;

			if(rx_callback == NULL)
			{
				ipc_server_socket_tbl[port_idx][j].ctrl_mode |= CTRL_RX_SEMA_NOTIFY; // semaphore notification.
				ipc_server_socket_tbl[port_idx][j].rx_notify_sema = rx_semaphore;
			}
			else
			{
				ipc_server_socket_tbl[port_idx][j].ctrl_mode &= ~CTRL_RX_SEMA_NOTIFY; // callback notification.
				ipc_server_socket_tbl[port_idx][j].rx_notify_isr = (mb_ipc_notify_isr_t)rx_callback;
			}
			
			ipc_server_socket_tbl[port_idx][j].use_flag = USE_FLAG_ALLOCATED;
		}
		
		ipc_handle.data = ipc_server_socket_tbl[port_idx][0].handle.data;
	}

	mb_ipc_exit_critical(temp);

	return ipc_handle.data;
}

#define MB_IPC_RETRY_DELAY		2
#define MB_IPC_RETRY_MAX		1

int mb_ipc_connect(u32 handle, u8 dst_cpu, u8 dst_port, u32 time_out)
{
	// data sanity check.
	if(IS_NOT_SERVER_PORT(dst_port))
	{
		return -MB_IPC_INVALID_PORT;
	}
	
	mb_ipc_socket_t * ipc_socket = get_socket_from_handle(handle);

	if(ipc_socket == NULL)
	{
		return -MB_IPC_INVALID_HANDLE;
	}

	// data sanity check.
	if( (ipc_socket->use_flag & USE_FLAG_ALLOCATED) == 0 )
	{
		return -MB_IPC_NOT_INITED;
	}

	if(ipc_socket->use_flag & USE_FLAG_CONNECTED)
	{
		return -MB_IPC_INVALID_STATE;
	}

	// set the dst cpu-port firstly, because the tx_cmd/rx_rsp need these informations.
	ipc_socket->dst_cpu  = dst_cpu;
	ipc_socket->dst_port = dst_port;
	
	ipc_socket->run_state = 0;
	ipc_socket->tx_status = 0;
	ipc_socket->rx_read_offset = 0;

	memset(&ipc_socket->tx_cmd, 0, sizeof(ipc_socket->tx_cmd));

	ipc_socket->tx_cmd.hdr.cmd = MB_IPC_CONNECT_CMD;

	u32   retry = 0;
	int   ret_val = 0;

	do
	{
		rtos_get_semaphore(&ipc_socket->tx_notify_sema, 0);  // clear the semaphore state.
		
		int route_status = ipc_socket_tx_cmd(ipc_socket, &ipc_socket->tx_cmd, retry);
		
		if(route_status != IPC_ROUTE_STATUS_OK)
		{
			ret_val = -(MB_IPC_ROUTE_BASE_FAILED + route_status);
			break;  // not retry for router error.
		}

		route_status = rtos_get_semaphore(&ipc_socket->tx_notify_sema, time_out);
		
		if(route_status != 0)
		{
			ipc_socket->run_state &= ~STATE_TX_IN_PROCESS;  // clear tx_in_process. prevent cmd from being handled in ISR.
			ret_val = -MB_IPC_TX_TIMEOUT;
			// rtos_delay_milliseconds(MB_IPC_RETRY_DELAY);
			continue;
		}
		
		if(ipc_socket->tx_status == MB_IPC_TX_OK)
		{
			ipc_socket->use_flag |= USE_FLAG_CONNECTED;
			return 0;
		}
		else
		{
			ret_val = -ipc_socket->tx_status;
			break;  // not retry.
		}

	} while((ret_val != 0) && (++retry < MB_IPC_RETRY_MAX));

	// if(ret_val != 0)  // must be TRUE.
	{
		ipc_socket->dst_cpu  = 0;
		ipc_socket->dst_port = 0;  // clear the connection.
	}

	return ret_val;
}

int mb_ipc_disconnect(u32 handle, u8 dst_cpu, u8 dst_port, u32 time_out)
{
	mb_ipc_socket_t * ipc_socket = get_socket_from_handle(handle);

	if(ipc_socket == NULL)
	{
		return -MB_IPC_INVALID_HANDLE;
	}

	// data sanity check.
	if( (ipc_socket->use_flag & USE_FLAG_ALLOCATED) == 0 )
	{
		return -MB_IPC_NOT_INITED;
	}

	if(dst_port == 0)
	{
		ipc_socket->use_flag &= ~USE_FLAG_CONNECTED;
		return 0;
	}

	ipc_socket->dst_cpu  = dst_cpu;
	ipc_socket->dst_port = dst_port;
	
	ipc_socket->run_state &= ~STATE_TX_IN_PROCESS;  // clear tx_in_process. prevent previous cmd from being handled in ISR.

	memset(&ipc_socket->tx_cmd, 0, sizeof(ipc_socket->tx_cmd));

	ipc_socket->tx_cmd.hdr.cmd = MB_IPC_DISCONNECT_CMD;

	u32   retry = 0;
	int   ret_val = 0;

	do
	{
		rtos_get_semaphore(&ipc_socket->tx_notify_sema, 0);  // clear the semaphore state.
		
		int route_status = ipc_socket_tx_cmd(ipc_socket, &ipc_socket->tx_cmd, retry);
		
		if(route_status != IPC_ROUTE_STATUS_OK)
		{
			ret_val = -(MB_IPC_ROUTE_BASE_FAILED + route_status);
			break;  // not retry for router error.
		}

		route_status = rtos_get_semaphore(&ipc_socket->tx_notify_sema, time_out);
		
		if(route_status != 0)
		{
			ipc_socket->run_state &= ~STATE_TX_IN_PROCESS;  // clear tx_in_process. prevent cmd from being handled in ISR.
			ret_val = -MB_IPC_TX_TIMEOUT;
			// rtos_delay_milliseconds(MB_IPC_RETRY_DELAY);
			continue;
		}

		if(ipc_socket->dst_port == 0)
		{
			ipc_socket->use_flag &= ~USE_FLAG_CONNECTED;
			return 0;
		}

		if(ipc_socket->tx_status == MB_IPC_TX_OK)
		{
			ipc_socket->dst_cpu  = 0;
			ipc_socket->dst_port = 0;
			
			ipc_socket->use_flag &= ~USE_FLAG_CONNECTED;
			return 0;
		}
		else
		{
			if( (ipc_socket->tx_status > MB_IPC_API_BASE_FAILED) || 
				(ipc_socket->tx_status == (MB_IPC_ROUTE_BASE_FAILED + IPC_ROUTE_UNREACHABLE)) )
			{
				ipc_socket->dst_cpu  = 0;
				ipc_socket->dst_port = 0;

				ipc_socket->use_flag &= ~USE_FLAG_CONNECTED;
				return 0;
			}
			return -ipc_socket->tx_status;  // not retry.
		}
	} while((ret_val != 0) && (++retry < MB_IPC_RETRY_MAX));

	return ret_val;
}

int mb_ipc_close(u32 handle, u32 time_out)
{
	mb_ipc_socket_t * ipc_socket = get_socket_from_handle(handle);

	if(ipc_socket == NULL)
	{
		return -MB_IPC_INVALID_HANDLE;
	}

	int result = mb_ipc_disconnect(handle, ipc_socket->dst_cpu, ipc_socket->dst_port, time_out);

	if(result != 0)
	{
		return result;
	}

	if(IS_SERVER_PORT(ipc_socket->src_port))
	{
		// all server sockets share the same resources.
		// server sockets are not closed, so can't free any resources.
		return 0;
	}
	
	// free resources.
	rtos_deinit_semaphore(&ipc_socket->tx_notify_sema);

	if(ipc_socket->ctrl_mode & CTRL_RX_SEMA_NOTIFY)
	{
		rtos_deinit_semaphore(&ipc_socket->rx_notify_sema);
	}
	
	ipc_socket->use_flag = 0;   // &= ~(USE_FLAG_CONNECTED | USE_FLAG_ALLOCATED); // disconnected, socket closed.
	
	return 0;
}

int mb_ipc_send_async(u32 handle, u8 user_cmd, u8 * data_buff, u32 data_len)
{
	mb_ipc_socket_t * ipc_socket = get_socket_from_handle(handle);

	if(ipc_socket == NULL)
	{
		return -MB_IPC_INVALID_HANDLE;
	}

	// data sanity check.
	if( (ipc_socket->use_flag & USE_FLAG_ALLOCATED) == 0 )
	{
		return -MB_IPC_NOT_INITED;
	}

	if( (ipc_socket->use_flag & USE_FLAG_CONNECTED) == 0 )
	{
		return -MB_IPC_INVALID_STATE;
	}

	if(ipc_socket->run_state & STATE_TX_IN_PROCESS)
	{
		return -MB_IPC_TX_BUSY;
	}

	memset(&ipc_socket->tx_cmd, 0, sizeof(ipc_socket->tx_cmd));

	ipc_socket->tx_cmd.hdr.cmd = MB_IPC_SEND_CMD;
	
	ipc_socket->tx_cmd.user_cmd      = user_cmd;
	ipc_socket->tx_cmd.cmd_data_len  = data_len;
	ipc_socket->tx_cmd.cmd_data_buff = data_buff;
	ipc_socket->tx_cmd.cmd_data_crc8 = cal_crc8_0x31(data_buff, data_len);

	int route_status = ipc_socket_tx_cmd(ipc_socket, &ipc_socket->tx_cmd, 0);
	
	if(route_status != IPC_ROUTE_STATUS_OK)
	{
		return -(MB_IPC_ROUTE_BASE_FAILED + route_status);
	}

	return 0;
}

int mb_ipc_send(u32 handle, u8 user_cmd, u8 * data_buff, u32 data_len, u32 time_out)
{
	mb_ipc_socket_t * ipc_socket = get_socket_from_handle(handle);

	if(ipc_socket == NULL)
	{
		return -MB_IPC_INVALID_HANDLE;
	}

	// data sanity check.
	if( (ipc_socket->use_flag & USE_FLAG_ALLOCATED) == 0 )
	{
		return -MB_IPC_NOT_INITED;
	}

	if( (ipc_socket->use_flag & USE_FLAG_CONNECTED) == 0 )
	{
		return -MB_IPC_INVALID_STATE;
	}

	if(ipc_socket->run_state & STATE_TX_IN_PROCESS)
	{
		return -MB_IPC_TX_BUSY;
	}

	memset(&ipc_socket->tx_cmd, 0, sizeof(ipc_socket->tx_cmd));

	ipc_socket->tx_cmd.hdr.cmd = MB_IPC_SEND_CMD;
	
	ipc_socket->tx_cmd.user_cmd      = user_cmd;
	ipc_socket->tx_cmd.cmd_data_len  = data_len;
	ipc_socket->tx_cmd.cmd_data_buff = data_buff;
	ipc_socket->tx_cmd.cmd_data_crc8 = cal_crc8_0x31(data_buff, data_len);

	u32   retry = 0;
	int   ret_val = 0;

	do
	{
		rtos_get_semaphore(&ipc_socket->tx_notify_sema, 0);  // clear the semaphore state.

		int route_status = ipc_socket_tx_cmd(ipc_socket, &ipc_socket->tx_cmd, retry);
		
		if(route_status != IPC_ROUTE_STATUS_OK)
		{
			ret_val = -(MB_IPC_ROUTE_BASE_FAILED + route_status);
			break;  // not retry for router error.
		}

		route_status = rtos_get_semaphore(&ipc_socket->tx_notify_sema, time_out);
		
		if(route_status != 0)
		{
			ipc_socket->run_state &= ~STATE_TX_IN_PROCESS;  // clear tx_in_process. prevent cmd from being handled in ISR.
			ret_val = -MB_IPC_TX_TIMEOUT;
			// rtos_delay_milliseconds(MB_IPC_RETRY_DELAY);
			continue;
		}
		
		if(ipc_socket->tx_status == MB_IPC_TX_OK)
		{
			return 0;
		}
		else
		{
			return -ipc_socket->tx_status;  // not retry.
		}
	} while((ret_val != 0) && (++retry < MB_IPC_RETRY_MAX));

	return ret_val;
}

int mb_ipc_get_connection(u32 handle, u8 *src, u8 * dst)
{
	mb_ipc_socket_t * ipc_socket = get_socket_from_handle(handle);

	if(ipc_socket == NULL)
	{
		return -MB_IPC_INVALID_HANDLE;
	}

	*src = (ipc_socket->src_cpu << 6) + ipc_socket->src_port;
	*dst = (ipc_socket->dst_cpu << 6) + ipc_socket->dst_port;

	return 0;
}

int mb_ipc_get_recv_data_len(u32 handle)
{
	mb_ipc_socket_t * ipc_socket = get_socket_from_handle(handle);

	if(ipc_socket == NULL)
	{
		return -MB_IPC_INVALID_HANDLE;
	}

	// data sanity check.
	if( (ipc_socket->use_flag & USE_FLAG_ALLOCATED) == 0 )
	{
		return -MB_IPC_NOT_INITED;
	}

	if( (ipc_socket->use_flag & USE_FLAG_CONNECTED) == 0 )
	{
		return -MB_IPC_INVALID_STATE;
	}

	if(ipc_socket->run_state & STATE_RX_IN_PROCESS)
	{
		if(ipc_socket->rx_cmd.hdr.cmd != MB_IPC_SEND_CMD)
			return -MB_IPC_NO_DATA;
		
		int recv_len = ipc_socket->rx_cmd.cmd_data_len;
		return recv_len;
	}
	else
	{
		return -MB_IPC_NO_DATA;
	}
}

int mb_ipc_get_recv_event(u32 handle, u32 * event_flag)
{
	if(event_flag == NULL)
		return -MB_IPC_INVALID_PARAM;

	*event_flag = MB_IPC_CMD_MAX + 1;
	
	mb_ipc_socket_t * ipc_socket = get_socket_from_handle(handle);

	if(ipc_socket == NULL)
	{
		return -MB_IPC_INVALID_HANDLE;
	}

	// data sanity check.
	if( (ipc_socket->use_flag & USE_FLAG_ALLOCATED) == 0 )
	{
		return -MB_IPC_NOT_INITED;
	}

	if(ipc_socket->run_state & STATE_RX_IN_PROCESS)
	{
		if( (ipc_socket->use_flag & USE_FLAG_CONNECTED) == 0 )
		{
			return -MB_IPC_INVALID_STATE;
		}

		*event_flag = (ipc_socket->rx_cmd.hdr.cmd & MB_IPC_CMD_MASK);
	}
	else if(ipc_socket->rx_cmd.hdr.cmd == (MB_IPC_DISCONNECT_CMD | MB_IPC_RSP_FLAG))
	{
		*event_flag = MB_IPC_DISCONNECT_CMD;  // disconnect cmd reponsed automatically before notifying app.
	}
	else if(ipc_socket->rx_cmd.hdr.cmd == (MB_IPC_CONNECT_CMD | MB_IPC_RSP_FLAG))
	{
		if( (ipc_socket->use_flag & USE_FLAG_CONNECTED) == 0 )
		{
			return -MB_IPC_INVALID_STATE;
		}

		*event_flag = MB_IPC_CONNECT_CMD;  // connect cmd reponsed automatically before notifying app.
	}
	
	return 0;
}

#if CONFIG_CACHE_ENABLE
#ifndef CONFIG_DCACHE_SIZE
#define CONFIG_DCACHE_SIZE      0x8000   /* DCACHE SIZE 32KB. */
#endif
#endif

int mb_ipc_recv_async(u32 handle, u8 * user_cmd, u8 * data_buff, u32 buff_len)
{
	mb_ipc_socket_t * ipc_socket = get_socket_from_handle(handle);

	if(ipc_socket == NULL)
	{
		return -MB_IPC_INVALID_HANDLE;
	}

	// data sanity check.
	if( (ipc_socket->use_flag & USE_FLAG_ALLOCATED) == 0 )
	{
		return -MB_IPC_NOT_INITED;
	}

	if( (ipc_socket->use_flag & USE_FLAG_CONNECTED) == 0 )
	{
		return -MB_IPC_INVALID_STATE;
	}

	if(user_cmd != NULL)
	{
		*user_cmd = INVALID_USER_CMD_ID;  // initialise.
	}

	if(ipc_socket->run_state & STATE_RX_IN_PROCESS)
	{
		if(ipc_socket->rx_cmd.hdr.cmd != MB_IPC_SEND_CMD)
			return 0;
		
		if(user_cmd != NULL)
		{
			*user_cmd = ipc_socket->rx_cmd.user_cmd;
		}
		
		if( (data_buff == NULL) || (buff_len == 0) )
		{
			// discard all data in buffer. after this call, rx_cmd.hdr.cmd is changed.
			ipc_socket_tx_rsp(ipc_socket, &ipc_socket->rx_cmd);
			return 0;
		}

		if(ipc_socket->rx_read_offset >= ipc_socket->rx_cmd.cmd_data_len)
		{
			// after this call, rx_cmd.hdr.cmd is changed.
			ipc_socket_tx_rsp(ipc_socket, &ipc_socket->rx_cmd);
			return 0;
		}

		u8  * src_buf = (u8 *)(ipc_socket->rx_cmd.cmd_data_buff) + ipc_socket->rx_read_offset;
		u32   read_len = MIN((ipc_socket->rx_cmd.cmd_data_len - ipc_socket->rx_read_offset), buff_len);

		#if CONFIG_CACHE_ENABLE
		if(ipc_socket->rx_cmd.cmd_data_len >= (CONFIG_DCACHE_SIZE / 2))
			flush_all_dcache();
		else
			flush_dcache(ipc_socket->rx_cmd.cmd_data_buff, ipc_socket->rx_cmd.cmd_data_len);
		#endif

		memcpy(data_buff, src_buf, read_len);
		ipc_socket->rx_read_offset += read_len;

		// check the buff data validity.
		u8  crc8 = cal_crc8_0x31(ipc_socket->rx_cmd.cmd_data_buff, ipc_socket->rx_cmd.cmd_data_len);

		if(crc8 != ipc_socket->rx_cmd.cmd_data_crc8)  // data buff is damaged in the sender side.
		{
			ipc_socket_rx_cmd_error_handler(ipc_socket, IPC_API_IMPL_RX_DATA_FAILED);
			return -MB_IPC_RX_DATA_FAILED;
		}

		if(ipc_socket->rx_read_offset >= ipc_socket->rx_cmd.cmd_data_len)
		{
			ipc_socket_tx_rsp(ipc_socket, &ipc_socket->rx_cmd);  // after this call, rx_cmd.hdr.cmd is changed.
		}
		
		return read_len;
	}

	return 0;
}

int mb_ipc_recv(u32 handle, u8 * user_cmd, u8 * data_buff, u32 buff_len, u32 time_out)
{
	mb_ipc_socket_t * ipc_socket = get_socket_from_handle(handle);

	if(ipc_socket == NULL)
	{
		return -MB_IPC_INVALID_HANDLE;
	}

	int   read_len;

recv_data_async:
	
	read_len = mb_ipc_recv_async(handle, user_cmd, data_buff, buff_len);

	if(read_len != 0)
		return read_len;
	
	if(user_cmd != NULL)
	{
		if(*user_cmd != INVALID_USER_CMD_ID)
			return read_len;
	}
	
	if(ipc_socket->ctrl_mode & CTRL_RX_SEMA_NOTIFY)
	{
		// wait to receive send command.
		int route_status = rtos_get_semaphore(&ipc_socket->rx_notify_sema, time_out);

		if(route_status != 0)
		{
			return 0;
		}
		else
		{
			goto recv_data_async;
		}
	}

	return 0;
}

u32 mb_ipc_server_get_connect_handle(u32 handle, u32 connect_id)
{
	mb_ipc_socket_handle_t   ipc_handle;

#if MAX_CONNET_PER_SVR > (SOCKET_CONNECT_ID_MASK + 1)
#error coonection count exceeded the limitation.
#endif

	ipc_handle.data = handle;

	if(IS_NOT_SERVER_PORT(ipc_handle.src_port))
	{
		ipc_handle.data = 0;
		return ipc_handle.data;
	}

	if(connect_id >= MAX_CONNET_PER_SVR)
	{
		ipc_handle.data = 0;
		return ipc_handle.data;
	}

	ipc_handle.connect_id = (connect_id & SOCKET_CONNECT_ID_MASK);
	
	return ipc_handle.data;
}

int mb_ipc_server_close(u32 handle, u32 time_out)
{
	mb_ipc_socket_t * ipc_socket = get_socket_from_handle(handle);

	if(ipc_socket == NULL)
	{
		return -MB_IPC_INVALID_HANDLE;
	}

	if(IS_NOT_SERVER_PORT(ipc_socket->src_port))
	{
		return -MB_IPC_INVALID_PORT;
	}

	// data sanity check.
	if( (ipc_socket->use_flag & USE_FLAG_ALLOCATED) == 0 )
	{
		return 0;
	}

	u32   svr_handle;
	mb_ipc_socket_t  * svr_socket;
	
	for(int j = 0; j < MAX_CONNET_PER_SVR; j++)
	{
		// svr_handle = mb_ipc_server_get_connect_handle(handle, j);
		svr_handle = get_handle_from_socket(ipc_socket, j);
		svr_socket = get_socket_from_handle(svr_handle);

		if(svr_socket == NULL)
		{
			continue;
		}

		if(svr_socket->use_flag & USE_FLAG_CONNECTED)
		{
			mb_ipc_close(svr_handle, time_out);
		}
		
		svr_socket->use_flag = 0; // disconnected & closed.
	}

	// free resources.
	// all server sockets shared the same resources.
	rtos_deinit_semaphore(&ipc_socket->tx_notify_sema);

	if(ipc_socket->ctrl_mode & CTRL_RX_SEMA_NOTIFY)
	{
		rtos_deinit_semaphore(&ipc_socket->rx_notify_sema);
	}

	return 0;
}

int mb_ipc_server_send(u32 handle, u8 user_cmd, u8 * data_buff, u32 data_len, u32 time_out)
{
	mb_ipc_socket_t * ipc_socket = get_socket_from_handle(handle);

	if(ipc_socket == NULL)
		return -MB_IPC_INVALID_HANDLE;

	if(IS_NOT_SERVER_PORT(ipc_socket->src_port))
		return -MB_IPC_INVALID_PORT;

	// data sanity check.
	if( (ipc_socket->use_flag & USE_FLAG_ALLOCATED) == 0 )
	{
		return -MB_IPC_NOT_INITED;
	}

	u32   svr_handle;
	int   run_status;
	int   cnt = 0;

	// send to all connected clients.
	for(int j = 0; j < MAX_CONNET_PER_SVR; j++)
	{
		// svr_handle = mb_ipc_server_get_connect_handle(handle, j);
		svr_handle = get_handle_from_socket(ipc_socket, j);

		run_status = mb_ipc_send(svr_handle, user_cmd, data_buff, data_len, time_out);
		
		if(run_status == 0)
		{
			cnt++;
		}
	}

	if(cnt > 0)
		return 0;
	else
		return -MB_IPC_TX_FAILED;

}

int mb_ipc_server_recv(u32 handle, u8 * user_cmd, u8 * data_buff, u32 buff_len, u32 time_out)
{
	mb_ipc_socket_t * ipc_socket = get_socket_from_handle(handle);

	if(ipc_socket == NULL)
		return -MB_IPC_INVALID_HANDLE;

	if(IS_NOT_SERVER_PORT(ipc_socket->src_port))
		return -MB_IPC_INVALID_PORT;

	// data sanity check.
	if( (ipc_socket->use_flag & USE_FLAG_ALLOCATED) == 0 )
	{
		return -MB_IPC_NOT_INITED;
	}

svr_recv_data_async:

	for(int j = 0; j < MAX_CONNET_PER_SVR; j++)
	{
		// svr_handle = mb_ipc_server_get_connect_handle(handle, j);
		u32 svr_handle = get_handle_from_socket(ipc_socket, j);

		int read_len = mb_ipc_recv_async(svr_handle, user_cmd, data_buff, buff_len);

		if(read_len > 0)
			return read_len;

		if(read_len < 0)
			continue;
		
		if(user_cmd != NULL)
		{
			if(*user_cmd != INVALID_USER_CMD_ID)
				return read_len;
		}
	}

	if(ipc_socket->ctrl_mode & CTRL_RX_SEMA_NOTIFY)
	{
		// wait to receive send command.
		int route_status = rtos_get_semaphore(&ipc_socket->rx_notify_sema, time_out);

		if(route_status != 0)
		{
			return 0;
		}
		else
		{
			goto svr_recv_data_async;
		}
	}

	return 0;
}

bk_err_t mb_ipc_init(void)
{
	bk_err_t		ret_code;

	static u8       chnl_inited = 0;

	if(chnl_inited != 0)
		return BK_OK;

	int route_id = 0;

	for(route_id = 0; route_id < ARRAY_SIZE(ipc_route_tbl); route_id++)
	{
		u8    chnl_id = ipc_route_tbl[route_id].log_chnl;
		
		ret_code = mb_chnl_open(chnl_id, (void *)route_id);
		
		if(ret_code != BK_OK)
		{
			goto init_exit;
		}
		
		mb_chnl_ctrl(chnl_id, MB_CHNL_SET_RX_ISR, (void *)ipc_router_rx_isr);
		mb_chnl_ctrl(chnl_id, MB_CHNL_SET_TX_CMPL_ISR, (void *)ipc_router_tx_cmpl_isr);
	}

	init_client_socket();
	init_server_socket();

	chnl_inited = 1;

	return BK_OK;

init_exit:
	
	for(int i =0; i < route_id; i++)
	{
		mb_chnl_close(ipc_route_tbl[i].log_chnl);
	}

	return ret_code;
}

#endif

