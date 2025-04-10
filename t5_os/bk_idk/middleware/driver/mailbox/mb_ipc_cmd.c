// Copyright 2020-2022 Beken
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
#include <driver/mb_chnl_buff.h>
#include "mb_ipc_cmd.h"
#include "driver/dma.h"
#include "driver/flash.h"

#if CONFIG_CACHE_ENABLE
#include "cache.h"
#endif

#define MOD_TAG		"IPC"

#if (CONFIG_CPU_CNT > 1)

#define IPC_RSP_CMD_FLAG		0x80
#define IPC_RSP_CMD_MASK		0x7F

#define IPC_RSP_TIMEOUT			10		/* 10ms */
#define IPC_XCHG_DATA_MAX		32 // MB_CHNL_BUFF_LEN

typedef union
{
	struct
	{
		mb_chnl_hdr_t	chnl_hdr;
		void *	cmd_buff;
		u16		cmd_data_len;
	};

	mb_chnl_cmd_t	mb_cmd;
} ipc_cmd_t;

typedef union
{
	struct
	{
		mb_chnl_hdr_t	chnl_hdr;
		void *	rsp_buff;
		u16		rsp_data_len;
	};

	mb_chnl_ack_t	mb_ack;
} ipc_rsp_t;

typedef u32 (* ipc_rx_cmd_hdlr_t)(void * chnl_cb, mb_chnl_ack_t *ack_buf);

typedef struct
{
	/* chnl data */
	u8					chnl_inited;
	u8					chnl_id;
	beken_semaphore_t	chnl_sema;

	/* tx cmd data */
	beken_semaphore_t	rsp_sema;
	u8			*	tx_xchg_buff;
	u8				tx_cmd;
	volatile u8		tx_cmd_in_process;
	volatile u8		tx_cmd_failed;
	u32				rsp_buf[IPC_XCHG_DATA_MAX / sizeof(u32)];
	u16				rsp_len;

	/* rx cmd data */
	ipc_rx_cmd_hdlr_t	rx_cmd_handler;
	u8			*	rx_xchg_buff;
	u8				rx_cmd;
	volatile u8		rx_cmd_in_process;
	u32				cmd_buf[IPC_XCHG_DATA_MAX / sizeof(u32)];
	u16				cmd_len;
} ipc_chnl_cb_t;

#ifdef CONFIG_FREERTOS_SMP
#include "spinlock.h"
static volatile spinlock_t mb_ipc_spin_lock = SPIN_LOCK_INIT;
#endif // CONFIG_FREERTOS_SMP
static inline uint32_t ipc_enter_critical()
{
	uint32_t flags = rtos_disable_int();

#ifdef CONFIG_FREERTOS_SMP
	spin_lock(&mb_ipc_spin_lock);
#endif // CONFIG_FREERTOS_SMP

	return flags;
}

static inline void ipc_exit_critical(uint32_t flags)
{
#ifdef CONFIG_FREERTOS_SMP
	spin_unlock(&mb_ipc_spin_lock);
#endif // CONFIG_FREERTOS_SMP

	rtos_enable_int(flags);
}

static void ipc_cmd_rx_isr(ipc_chnl_cb_t *chnl_cb, mb_chnl_cmd_t *cmd_buf)
{
	u32		result = ACK_STATE_FAIL;
	mb_chnl_ack_t * ack_buf = (mb_chnl_ack_t *)cmd_buf;
	ipc_cmd_t     * ipc_cmd = (ipc_cmd_t *)cmd_buf;

	/* chnl_cb->rx_xchg_buff == ipc_cmd->cmd_buff. MUST be true. */

	if(cmd_buf->hdr.cmd & IPC_RSP_CMD_FLAG)  /* ipc rsp from other CPU. */
	{
		if(chnl_cb->tx_cmd_in_process == 0)	/* unsolicited ipc response. */
		{
			goto ipc_cmd_rx_isr_exit;
		}

		if(chnl_cb->tx_cmd != (cmd_buf->hdr.cmd & IPC_RSP_CMD_MASK))
		{
			/* un-matched rpc response. */
			goto ipc_cmd_rx_isr_exit;
		}

		/* communication ok, function is completed. */
		chnl_cb->tx_cmd_failed = 0;
		chnl_cb->tx_cmd_in_process = 0;

		BK_LOGI(MOD_TAG, "IPC_RSP_CMD: %d, %d, %d\r\n", cmd_buf->param1, cmd_buf->param2, cmd_buf->param3);

		if(ipc_cmd->cmd_data_len > sizeof(chnl_cb->rsp_buf))
		{
			/* buffer insufficient, failed. */
			chnl_cb->tx_cmd_failed = 1;
			chnl_cb->rsp_len = 0;
		}
		else
		{
			/* returns ACK_STATE_COMPLETE, 
			   so must copy data from cmd_buff in case that the buffer is released. */
			if((ipc_cmd->cmd_buff != NULL) && (ipc_cmd->cmd_data_len > 0))
			{
				#if CONFIG_CACHE_ENABLE
				flush_dcache(ipc_cmd->cmd_buff, ipc_cmd->cmd_data_len);
				#endif
				
				memcpy(chnl_cb->rsp_buf, ipc_cmd->cmd_buff, ipc_cmd->cmd_data_len);

				chnl_cb->rsp_len = ipc_cmd->cmd_data_len;
			}
			else
			{
				chnl_cb->rsp_len = 0;
			}
		}

		rtos_set_semaphore(&chnl_cb->rsp_sema);

		ipc_rsp_t * ipc_rsp = (ipc_rsp_t *)ack_buf;

		/* it is a rsp command, nothing need to be returned. */
		ipc_rsp->rsp_buff = NULL;
		ipc_rsp->rsp_data_len = 0;

		result = ACK_STATE_COMPLETE;

		goto ipc_cmd_rx_isr_exit;
	}
	else
	{
		chnl_cb->rx_cmd = cmd_buf->hdr.cmd;
		chnl_cb->rx_cmd_in_process = 1;

		BK_LOGD(MOD_TAG, "IPC_CMD: %d, %d, %d\r\n", cmd_buf->param1, cmd_buf->param2, cmd_buf->param3);

		if(ipc_cmd->cmd_data_len > sizeof(chnl_cb->cmd_buf))
		{
			result = ACK_STATE_FAIL;
		}
		else
		{
			/* may return ACK_STATE_COMPLETE, 
			   so must copy data from cmd_buff in case that the buffer is released. */
			if((ipc_cmd->cmd_buff != NULL) && (ipc_cmd->cmd_data_len > 0))
			{
				#if CONFIG_CACHE_ENABLE
				flush_dcache(ipc_cmd->cmd_buff, ipc_cmd->cmd_data_len);
				#endif
				
				memcpy(chnl_cb->cmd_buf, ipc_cmd->cmd_buff, ipc_cmd->cmd_data_len);

				chnl_cb->cmd_len = ipc_cmd->cmd_data_len;
			}
			else
			{
				chnl_cb->cmd_len = 0;
			}

			result = chnl_cb->rx_cmd_handler(chnl_cb, ack_buf);
		}

		if(result != ACK_STATE_PENDING)
			chnl_cb->rx_cmd_in_process = 0;
	}

ipc_cmd_rx_isr_exit:

	/* overwrite the cmd_buf after the ISR handle complete.
	 * return the ack info to caller using the SAME buffer with cmd buffer.
	 *     !!!! [input as param / output as result ]  !!!!
	 */
	ack_buf->ack_state = result;

	return;

}

static void ipc_cmd_tx_cmpl_isr(ipc_chnl_cb_t *chnl_cb, mb_chnl_ack_t *ack_buf)  /* tx_cmpl_isr */
{
	if(ack_buf->hdr.cmd == chnl_cb->tx_cmd)
	{
		if(chnl_cb->tx_cmd_in_process == 0)	/* RSP_CMD rx_isr may arrive before this tx_cmpl_isr. */
			return;

		/* IPC cmd tx complete. */

		if( (ack_buf->hdr.state & CHNL_STATE_COM_FAIL) 
			|| (ack_buf->ack_state == ACK_STATE_FAIL) )
		{
			chnl_cb->tx_cmd_failed = 1;
			chnl_cb->tx_cmd_in_process  = 0;

			/* communication failed or function failed. */
			rtos_set_semaphore(&chnl_cb->rsp_sema);
		}
		else if(ack_buf->ack_state == ACK_STATE_COMPLETE)
		{
			/* communication ok, function is completed. */
			chnl_cb->tx_cmd_failed = 0;
			chnl_cb->tx_cmd_in_process = 0;

			ipc_rsp_t * ipc_rsp = (ipc_rsp_t *)ack_buf;

			/* chnl_cb->tx_xchg_buff == ipc_rsp->rsp_buff. MUST be true. */

			if(ipc_rsp->rsp_data_len > sizeof(chnl_cb->rsp_buf))
			{
				/* buffer insufficient, failed. */
				chnl_cb->tx_cmd_failed = 1;
				chnl_cb->rsp_len = 0;
			}
			else
			{
				if((ipc_rsp->rsp_buff != NULL) && (ipc_rsp->rsp_data_len > 0))
				{
					#if CONFIG_CACHE_ENABLE
					flush_dcache(ipc_rsp->rsp_buff, ipc_rsp->rsp_data_len);;
					#endif

					memcpy(chnl_cb->rsp_buf, ipc_rsp->rsp_buff, ipc_rsp->rsp_data_len);
					chnl_cb->rsp_len = ipc_rsp->rsp_data_len;
				}
				else
				{
					chnl_cb->rsp_len = 0;
				}
			}
			
			rtos_set_semaphore(&chnl_cb->rsp_sema);
		}
		else
		{
			/* communication ok, function is pending. */
		}

		return;
	}

	/*
	 *   !!!  FAULT  !!!
	 */
	BK_LOGE(MOD_TAG, "Fault in %s,cmd:%d\r\n", __func__, ack_buf->hdr.cmd);

	return;

}

static bk_err_t ipc_chnl_init(ipc_chnl_cb_t *chnl_cb, u8 chnl_id, ipc_rx_cmd_hdlr_t rx_handler)
{
	bk_err_t		ret_code;

	if(chnl_cb->chnl_inited)
		return BK_OK;

	memset(chnl_cb, 0, sizeof(ipc_chnl_cb_t));
	chnl_cb->chnl_id = chnl_id;

	chnl_cb->tx_xchg_buff = mb_chnl_get_tx_buff(chnl_id);
	chnl_cb->rx_xchg_buff = mb_chnl_get_rx_buff(chnl_id);

	if( (chnl_cb->tx_xchg_buff == NULL) || 
		(chnl_cb->tx_xchg_buff == NULL) )
	{
		return BK_FAIL;
	}

	ret_code = rtos_init_semaphore_adv(&chnl_cb->chnl_sema, 1, 1);
	if(ret_code != BK_OK)
	{
		return ret_code;
	}

	ret_code = rtos_init_semaphore(&chnl_cb->rsp_sema, 1);
	if(ret_code != BK_OK)
	{
		rtos_deinit_semaphore(&chnl_cb->chnl_sema);

		return ret_code;
	}

	ret_code = mb_chnl_open(chnl_id, chnl_cb);
	if(ret_code != BK_OK)
	{
		rtos_deinit_semaphore(&chnl_cb->chnl_sema);
		rtos_deinit_semaphore(&chnl_cb->rsp_sema);

		return ret_code;
	}

	chnl_cb->rx_cmd_handler = rx_handler;

	mb_chnl_ctrl(chnl_id, MB_CHNL_SET_RX_ISR, (void *)ipc_cmd_rx_isr);
	mb_chnl_ctrl(chnl_id, MB_CHNL_SET_TX_CMPL_ISR, (void *)ipc_cmd_tx_cmpl_isr);

	chnl_cb->chnl_inited = 1;

	return BK_OK;
}

static bk_err_t ipc_send_cmd(ipc_chnl_cb_t *chnl_cb, u8 cmd, u8 *cmd_buf, u16 cmd_len, u8 * rsp_buf, u16 rsp_buf_len)
{
	bk_err_t	ret_val = BK_FAIL;
	ipc_cmd_t	ipc_cmd;

	if(!chnl_cb->chnl_inited)
		return BK_FAIL;

	rtos_get_semaphore(&chnl_cb->chnl_sema, BEKEN_WAIT_FOREVER);

	chnl_cb->tx_cmd = cmd;
	chnl_cb->tx_cmd_failed = 0;
	chnl_cb->tx_cmd_in_process = 1;

	void * dst_buf = (void *)chnl_cb->tx_xchg_buff;

	do
	{
		if(cmd_len > IPC_XCHG_DATA_MAX)
		{
			ret_val = BK_ERR_PARAM;
			break;
		}

		if(cmd_buf == NULL)
			cmd_len = 0;

		if(cmd_len > 0)
			memcpy(dst_buf, cmd_buf, cmd_len);

		ipc_cmd.chnl_hdr.data = 0;	/* clear hdr. */
		ipc_cmd.chnl_hdr.cmd  = cmd;
		ipc_cmd.cmd_buff      = dst_buf;
		ipc_cmd.cmd_data_len  = cmd_len;

		ret_val = mb_chnl_write(chnl_cb->chnl_id, (mb_chnl_cmd_t *)&ipc_cmd);
		if(ret_val != BK_OK)
		{
			break;
		}

		ret_val = rtos_get_semaphore(&chnl_cb->rsp_sema, IPC_RSP_TIMEOUT);  /* isr_callback will set this semaphore. */
		if(ret_val != BK_OK)
		{
			mb_chnl_ctrl(chnl_cb->chnl_id, MB_CHNL_TX_RESET, NULL);
			break;
		}

		chnl_cb->tx_cmd_in_process  = 0;  /* must have been set to 0 by callback. */

		if(chnl_cb->tx_cmd_failed)
		{
			ret_val = BK_FAIL;
			break;
		}

		if((rsp_buf == NULL) || (rsp_buf_len == 0))
		{
			ret_val = BK_OK;
			break;
		}

		if(rsp_buf_len < chnl_cb->rsp_len)
		{
			ret_val = BK_ERR_PARAM;
			break;
		}

		if(chnl_cb->rsp_len > 0)
			memcpy(rsp_buf, chnl_cb->rsp_buf, chnl_cb->rsp_len);

		ret_val = BK_OK;

	}while(0);

	chnl_cb->tx_cmd_in_process = 0;

	rtos_set_semaphore(&chnl_cb->chnl_sema);

	return ret_val;

}

static bk_err_t ipc_send_special_cmd(ipc_chnl_cb_t *chnl_cb, u8 cmd)
{
	bk_err_t	ret_val;
	ipc_cmd_t	ipc_cmd;

	if (!chnl_cb->chnl_inited)
		return BK_FAIL;

	ipc_cmd.chnl_hdr.data = 0;	/* clear hdr. */
	ipc_cmd.chnl_hdr.cmd  = cmd;
	ipc_cmd.cmd_buff      = NULL;
	ipc_cmd.cmd_data_len  = 0;

	ret_val = mb_chnl_ctrl(chnl_cb->chnl_id, MB_CHNL_WRITE_SYNC, &ipc_cmd);

	return ret_val;
}


static ipc_chnl_cb_t	ipc_chnl_cb; // = { .chnl_id = MB_CHNL_HW_CTRL, .chnl_inited = 0 };

#if CONFIG_SYS_CPU1
#if CONFIG_MAILBOX_V2_0
// static ipc_chnl_cb_t	ipc_chnl_cb2;
#endif
#endif

typedef struct
{
	u16		res_id;
	u16		cpu_id;
} ipc_res_req_t;

typedef struct
{
	u32		user_id;
	u32		chnl_id;
} ipc_dma_free_t;

#if (CONFIG_SHELL_ASYNCLOG)
extern void shell_set_log_cpu(u8 req_cpu);
#endif

#if (CONFIG_SYS_CPU0)
static void mb_ipc_power_on_notify(void);
static void mb_ipc_heartbeat_notify(void);
static void mb_ipc_dump_notify(u32 dump);
#endif

static u32 ipc_cmd_handler(ipc_chnl_cb_t *chnl_cb, mb_chnl_ack_t *ack_buf)
{
	/* must NOT change ack_buf->hdr. */

	u32		result = ACK_STATE_FAIL;
	ipc_rsp_t * ipc_rsp = (ipc_rsp_t *)ack_buf;

	/* chnl_cb->rx_xchg_buff == ipc_cmd->cmd_buff == ipc_rsp->rsp_buff. MUST be true. */

	ipc_rsp->rsp_buff = chnl_cb->rx_xchg_buff;

	switch(chnl_cb->rx_cmd)
	{
		case IPC_RES_AVAILABLE_INDICATION:
			if(chnl_cb->cmd_len >= sizeof(u16))
			{
				ipc_rsp->rsp_data_len = 0;

				u16  res_id = *((u16 *)chnl_cb->cmd_buf);

				if(amp_res_available(res_id) == BK_OK)
				{
					result = ACK_STATE_COMPLETE;
				}
				else
				{
					result = ACK_STATE_FAIL;
				}
			}
			else
			{
				ipc_rsp->rsp_data_len = 0;
				result = ACK_STATE_FAIL;
			}
			break;

		case IPC_TEST_CMD:
			if(chnl_cb->cmd_len >= sizeof(u32))
			{
				ipc_rsp->rsp_data_len = sizeof(u32);

				u32 * p_src = (u32 *)chnl_cb->cmd_buf;
				u32 * p_dst = (u32 *)ipc_rsp->rsp_buff;

				*p_dst = (*p_src) + 1;
				
				result = ACK_STATE_COMPLETE;
			}
			else
			{
				ipc_rsp->rsp_data_len = 0;
				result = ACK_STATE_FAIL;
			}
			break;

		case IPC_GET_POWER_SAVE_FLAG:
			{
				ipc_rsp->rsp_data_len = sizeof(u32);

				u32 * p_dst = (u32 *)ipc_rsp->rsp_buff;

				// return PS flag to caller.
				*p_dst = 0xAA; //(u32)get_cpu1_ps_flag();

				result = ACK_STATE_COMPLETE;
			}
			break;

		#if CONFIG_SYS_CPU0
		case IPC_CPU1_POWER_UP_INDICATION:		// cpu1 indication, power up successfully.
			{
				/* no params. */
				/* inform modules who care CPU1 state. */
				mb_ipc_power_on_notify();
				
				/* no returns. */
				ipc_rsp->rsp_data_len = 0;
				result = ACK_STATE_COMPLETE;
			}
			break;

		case IPC_CPU1_HEART_BEAT_INDICATION:	// cpu1 indication, alive indication.
			// contains the power save flag?
			if(chnl_cb->cmd_len >= sizeof(u32))
			{
				//u32 * p_src = (u32 *)chnl_cb->cmd_buf;

				// save the param.
				
				mb_ipc_heartbeat_notify();

			}

			/* succeeded anyway but no returns. */
			ipc_rsp->rsp_data_len = 0;
			result = ACK_STATE_COMPLETE;
			break;
		#endif

		#if (CONFIG_SYS_CPU0)

		case IPC_CPU1_TRAP_HANDLE_BEGIN:		// cpu1 indication, dump begin.
			{
				ipc_rsp->rsp_data_len = 0;
				result = ACK_STATE_COMPLETE;

				mb_ipc_dump_notify(1);
				/* no params, no returns. */
				#if (CONFIG_SHELL_ASYNCLOG)
				shell_set_log_cpu(MAILBOX_CPU1);
				#endif
			}
			break;
		
		case IPC_CPU1_TRAP_HANDLE_END:		// cpu1 indication, dump end.
			{
				ipc_rsp->rsp_data_len = 0;
				result = ACK_STATE_COMPLETE;

				mb_ipc_dump_notify(0);
				/* no params, no returns. */
				#if (CONFIG_SHELL_ASYNCLOG)
				shell_set_log_cpu(CONFIG_CPU_CNT);
				shell_log_flush();
				#endif
				bk_reboot();
			}
			break;

		#endif

		#if 0 // (CONFIG_SYS_CPU1)
		
		case IPC_SET_CPU1_HEART_RATE:
			if(chnl_cb->cmd_len >= sizeof(u32))
			{
				ipc_rsp->rsp_data_len = 0;

				//u32 * p_src = (u32 *)chnl_cb->cmd_buf;

				// save the param.
				//set_cpu1_heart_rate(*p_src);
				
				result = ACK_STATE_COMPLETE;
			}
			else
			{
				ipc_rsp->rsp_data_len = 0;
				result = ACK_STATE_FAIL;
			}
			break;

		case IPC_GET_CPU1_HEART_RATE:
			{
				ipc_rsp->rsp_data_len = sizeof(u32);

				u32 * p_dst = (u32 *)ipc_rsp->rsp_buff;

				// return heart_rate to caller.
				*p_dst = 0xBB; //(u32)get_cpu1_heart_rate();

				result = ACK_STATE_COMPLETE;
			}
			break;
		#endif

		#ifdef AMP_RES_SERVER
		case IPC_RES_ACQUIRE_CNT:
			if(chnl_cb->cmd_len >= sizeof(ipc_res_req_t))
			{
				ipc_res_req_t * res_req = (ipc_res_req_t *)chnl_cb->cmd_buf;
				amp_res_req_cnt_t * res_cnt_list = (amp_res_req_cnt_t *)ipc_rsp->rsp_buff;

				/* call amp_res_acquire_cnt in interrupt disabled state. */
				u32  int_mask = ipc_enter_critical();
				bk_err_t ret_code = amp_res_acquire_cnt(res_req->res_id, res_req->cpu_id, res_cnt_list);
				ipc_exit_critical(int_mask);

				if(ret_code == BK_OK)
				{
					ipc_rsp->rsp_data_len = sizeof(amp_res_req_cnt_t);

					result = ACK_STATE_COMPLETE;
				}
				else
				{
					ipc_rsp->rsp_data_len = 0;
					result = ACK_STATE_FAIL;
				}
			}
			else
			{
				ipc_rsp->rsp_data_len = 0;
				result = ACK_STATE_FAIL;
			}
			break;

		case IPC_RES_RELEASE_CNT:
			if(chnl_cb->cmd_len >= sizeof(ipc_res_req_t))
			{
				ipc_res_req_t * res_req = (ipc_res_req_t *)chnl_cb->cmd_buf;
				amp_res_req_cnt_t * res_cnt_list = (amp_res_req_cnt_t *)ipc_rsp->rsp_buff;

				/* call amp_res_release_cnt in interrupt disabled state. */
				u32  int_mask = ipc_enter_critical();
				bk_err_t ret_code = amp_res_release_cnt(res_req->res_id, res_req->cpu_id, res_cnt_list);
				ipc_exit_critical(int_mask);

				if(ret_code == BK_OK)
				{
					ipc_rsp->rsp_data_len = sizeof(amp_res_req_cnt_t);

					result = ACK_STATE_COMPLETE;
				}
				else
				{
					ipc_rsp->rsp_data_len = 0;
					result = ACK_STATE_FAIL;
				}
			}
			else
			{
				ipc_rsp->rsp_data_len = 0;
				result = ACK_STATE_FAIL;
			}
			break;

		case IPC_ALLOC_DMA_CHNL:
			if(chnl_cb->cmd_len >= sizeof(u32))
			{
				u32  user_id = *((u32 *)chnl_cb->cmd_buf);

				ipc_rsp->rsp_data_len = sizeof(u8);

				u8 * chnl_id = (u8 *)ipc_rsp->rsp_buff;

				extern u8 dma_chnl_alloc(u32 user_id);

				*chnl_id = dma_chnl_alloc(user_id);

				result = ACK_STATE_COMPLETE;
			}
			else
			{
				ipc_rsp->rsp_data_len = 0;
				result = ACK_STATE_FAIL;
			}
			break;

		case IPC_FREE_DMA_CHNL:
			if(chnl_cb->cmd_len >= sizeof(ipc_dma_free_t))
			{
				ipc_dma_free_t * free_req = (ipc_dma_free_t *)chnl_cb->cmd_buf;

				extern bk_err_t dma_chnl_free(u32 user_id, dma_id_t chnl_id);

				bk_err_t ret_val = dma_chnl_free(free_req->user_id, (dma_id_t)free_req->chnl_id);

				ipc_rsp->rsp_data_len = 0;

				if(ret_val == BK_OK)
					result = ACK_STATE_COMPLETE;
				else
					result = ACK_STATE_FAIL;
			}
			else
			{
				ipc_rsp->rsp_data_len = 0;
				result = ACK_STATE_FAIL;
			}
			break;

		case IPC_DMA_CHNL_USER:
			if(chnl_cb->cmd_len >= sizeof(u8))
			{
				u8    chnl_id = *((u8 *)chnl_cb->cmd_buf);

				ipc_rsp->rsp_data_len = sizeof(u32);

				u32 * user_id = (u32 *)ipc_rsp->rsp_buff;

				extern u32 dma_chnl_user(dma_id_t user_id);

				*user_id = dma_chnl_user((dma_id_t)chnl_id);

				result = ACK_STATE_COMPLETE;
			}
			else
			{
				ipc_rsp->rsp_data_len = 0;
				result = ACK_STATE_FAIL;
			}
			break;
		#endif
		case IPC_FLASH_VOTE_LINE_MODE:
#if !CONFIG_SYS_CPU2
			if(chnl_cb->cmd_len >= sizeof(u8))
			{
				u8    v_line_mode = *((u8 *)chnl_cb->cmd_buf);

				ipc_rsp->rsp_data_len = sizeof(u32);

				u32 * ret_line_mode = (u32 *)ipc_rsp->rsp_buff;

				uint32_t ret = 0;
				extern bk_err_t bk_flash_set_line_mode(flash_line_mode_t line_mode);
				ret = bk_flash_set_line_mode(v_line_mode);
				if(ret != BK_OK) {
					*ret_line_mode = ret;
				} else {
					*ret_line_mode = v_line_mode;
				}
				result = ACK_STATE_COMPLETE;
			}
			else
			{
				ipc_rsp->rsp_data_len = 0;
				result = ACK_STATE_FAIL;
			}
#endif
			break;
		default:
			{
				ipc_rsp->rsp_data_len = 0;
				result = ACK_STATE_FAIL;
			}
			break;
	}

	return result;
}

#if (CONFIG_SYS_CPU0)
#include "../../../components/bk_rtos/rtos_ext.h"

#define MB_IPC_START_CORE_FLAG		0x01
#define MB_IPC_STOP_CORE_FLAG		0x02
#define MB_IPC_POWER_UP_FLAG		0x04
#define MB_IPC_HEARTBEAT_FLAG		0x08

#define MB_IPC_ALL_FLAGS			(MB_IPC_START_CORE_FLAG | MB_IPC_STOP_CORE_FLAG | MB_IPC_POWER_UP_FLAG | MB_IPC_HEARTBEAT_FLAG)

enum
{
	CORE_POWER_OFF = 0,
	CORE_STARTING,
	CORE_POWER_ON,
};

static rtos_event_ext_t		mb_ipc_heart_event;
static u32             cpu1_heartbeat_timestamp = 0;
static volatile u8     cpu1_state = CORE_POWER_OFF;
static volatile u8     cpu1_dump = 0;

//void start_cpu1_core(void);
//void stop_cpu1_core(void);

void mb_ipc_reset_notify(u32 power_on)
{
	if(power_on)
	{
		if(cpu1_state != CORE_POWER_ON)
		{
			cpu1_state = CORE_STARTING;
			rtos_set_event_ex(&mb_ipc_heart_event, MB_IPC_START_CORE_FLAG);
		}
	}
	else
	{
		cpu1_state = CORE_POWER_OFF;
		rtos_set_event_ex(&mb_ipc_heart_event, MB_IPC_STOP_CORE_FLAG);
	}
}

static void mb_ipc_heartbeat_notify(void)
{
	rtos_set_event_ex(&mb_ipc_heart_event, MB_IPC_HEARTBEAT_FLAG);
}

static void mb_ipc_power_on_notify(void)
{
	rtos_set_event_ex(&mb_ipc_heart_event, MB_IPC_POWER_UP_FLAG);
}

static void mb_ipc_dump_notify(u32 dump)
{
	cpu1_dump = (dump != 0);
}

static int mb_ipc_heartbeat_timeout(void)
{
	u32   cur_time;

	cur_time = (u32)rtos_get_time();

	if(cpu1_state == CORE_POWER_OFF)
	{
		return 0;
	}
	if((cpu1_state == CORE_STARTING) || (cpu1_dump != 0))
	{
		cpu1_heartbeat_timestamp = cur_time;
		return 0;
	}

	if(cur_time >= cpu1_heartbeat_timestamp)
	{
		cur_time -= cpu1_heartbeat_timestamp;
	}
	else
	{
		cur_time += (~(cpu1_heartbeat_timestamp)) + 1;  // wrap around. 
	}
	
	if(cur_time < CONFIG_INT_WDT_PERIOD_MS)
	{
		cpu1_heartbeat_timestamp = (u32)rtos_get_time();
		return 0;
	}

	return 1;
}

static volatile uint32_t cpu1_retry_cnt = 0;
static void mb_ipc_task( void *para )
{
	bk_err_t	ret_val;
	u32    events;
	u32    check_time = BEKEN_WAIT_FOREVER;
	
	ret_val = rtos_init_event_ex(&mb_ipc_heart_event);

	if(ret_val != BK_OK)
	{
		rtos_delete_thread(NULL);
		return;
	}

	while(1)
	{
		events = rtos_wait_event_ex(&mb_ipc_heart_event, MB_IPC_ALL_FLAGS, true, check_time);

		if(events == 0)
		{
			// timeout, so check heartbeat.
			events = MB_IPC_HEARTBEAT_FLAG;
		}

		if(events & MB_IPC_STOP_CORE_FLAG)  // process this event at first!!!!
		{
			if(cpu1_state == CORE_POWER_OFF)
			{
				events = 0;  // clear all events.
			}
		}
		
		if(events & MB_IPC_START_CORE_FLAG)
		{
			u8   retry_cnt = 0;

			while(cpu1_state == CORE_STARTING)
			{
				mb_ipc_heartbeat_timeout();
				
				if(events & MB_IPC_POWER_UP_FLAG)
				{
					if(cpu1_state == CORE_STARTING)
					{
						cpu1_retry_cnt = 0;
						cpu1_state = CORE_POWER_ON;
						break;  // cpu1 power on. 
					}
				}
				else
				{
					if(retry_cnt > 0)
					{
						cpu1_retry_cnt++;
						BK_LOGE(MOD_TAG, "IPC core1 start timeout\r\n");
						//stop_cpu1_core();
						rtos_delay_milliseconds(6);
						//start_cpu1_core();
						break;
					}
					else
					{
						events = rtos_wait_event_ex(&mb_ipc_heart_event, MB_IPC_POWER_UP_FLAG, true, 2000);//2s
					}
				}

				retry_cnt++;
			}

			// discard this event when not in CORE_STARTING state.
		}
		
		if(events & MB_IPC_HEARTBEAT_FLAG)
		{
			if(mb_ipc_heartbeat_timeout())
			{
				BK_LOGE(MOD_TAG, "IPC core1 timeout\r\n");
				/*when cpu1 heartbeat timeout, then system reboot*/
				BK_ASSERT(false);
			}
		}

		if(cpu1_state == CORE_POWER_OFF)
		{
			check_time = BEKEN_WAIT_FOREVER;
		}
		else
		{
			check_time = CONFIG_INT_WDT_PERIOD_MS;
		}

        if (cpu1_retry_cnt >= 3) {

            cpu1_retry_cnt = 0;

            BK_ASSERT(0);

        }

	}
}

int mb_ipc_cpu_is_power_on(u32 cpu_id)
{
	if(cpu1_state == CORE_POWER_ON)
	{
		return 1;
	}

	return 0;
}

#endif

#if (CONFIG_SYS_CPU1)

#define MB_IPC_HEARTBEAT_TIME		2000

static void mb_ipc_task( void *para )
{
	ipc_send_power_up();

	while(1)
	{
		rtos_delay_milliseconds(MB_IPC_HEARTBEAT_TIME);
		ipc_send_heart_beat(0);
	}
}
#endif

bk_err_t ipc_init(void)
{
	bk_err_t	ret_val = BK_FAIL;

#if (CONFIG_SYS_CPU0 || CONFIG_SYS_CPU1)

	ret_val = ipc_chnl_init(&ipc_chnl_cb, MB_CHNL_HW_CTRL, (ipc_rx_cmd_hdlr_t)ipc_cmd_handler);

#if CONFIG_SYS_CPU1
#if CONFIG_MAILBOX_V2_0
//	if(ret_val == BK_OK)
//		ret_val = ipc_chnl_init(&ipc_chnl_cb, CP2_MB_CHNL_CTRL, (ipc_rx_cmd_hdlr_t)ipc_cmd_handler);
#endif
#endif

	if(ret_val != BK_OK)
	{
		BK_LOGE(MOD_TAG, "Ipc failed at %d: %d\r\n", __LINE__, ret_val);

		return ret_val;
	}

	ret_val = rtos_create_thread(NULL, BEKEN_DEFAULT_WORKER_PRIORITY, "mb_ipc", mb_ipc_task, 1536, 0);

#endif
  
#if CONFIG_SYS_CPU2
	ret_val = ipc_chnl_init(&ipc_chnl_cb, CP1_MB_CHNL_CTRL, (ipc_rx_cmd_hdlr_t)ipc_cmd_handler);
#endif

	if(ret_val != BK_OK)
	{
		BK_LOGE(MOD_TAG, "Ipc failed at %d: %d\r\n", __LINE__, ret_val);
	}

	return ret_val;
}

bk_err_t ipc_send_available_ind(u16 resource_id)
{
	return ipc_send_cmd(&ipc_chnl_cb, IPC_RES_AVAILABLE_INDICATION, \
						(u8 *)&resource_id, sizeof(resource_id), NULL, 0);
}

u32 ipc_send_test_cmd(u32 param)
{
	ipc_send_cmd(&ipc_chnl_cb, IPC_TEST_CMD, (u8 *)&param, sizeof(param), (u8 *)&param, sizeof(param));

	return param;
}

u32 ipc_send_get_ps_flag(void)
{
	u32  param = -1;
	
	ipc_send_cmd(&ipc_chnl_cb, IPC_GET_POWER_SAVE_FLAG, NULL, 0, (u8 *)&param, sizeof(param));

	return param;
}

#if 0 // (CONFIG_SYS_CPU0)

u32 ipc_send_get_heart_rate(void)
{
	u32  param = -1;
	
	ipc_send_cmd(&ipc_chnl_cb, IPC_GET_CPU1_HEART_RATE, NULL, 0, (u8 *)&param, sizeof(param));

	return param;
}

bk_err_t ipc_send_set_heart_rate(u32 param)
{
	return ipc_send_cmd(&ipc_chnl_cb, IPC_SET_CPU1_HEART_RATE, (u8 *)&param, sizeof(param), NULL, 0);
}
#endif

#if (CONFIG_SYS_CPU1)

bk_err_t ipc_send_power_up(void)
{
	return ipc_send_cmd(&ipc_chnl_cb, IPC_CPU1_POWER_UP_INDICATION, NULL, 0, NULL, 0);
}

bk_err_t ipc_send_heart_beat(u32 param)
{
	return ipc_send_cmd(&ipc_chnl_cb, IPC_CPU1_HEART_BEAT_INDICATION, (u8 *)&param, sizeof(param), NULL, 0);
}

#endif

#if CONFIG_SYS_CPU1
bk_err_t ipc_send_trap_handle_begin(void)
{
	return ipc_send_special_cmd(&ipc_chnl_cb, IPC_CPU1_TRAP_HANDLE_BEGIN);
}

bk_err_t ipc_send_trap_handle_end(void)
{
	return ipc_send_special_cmd(&ipc_chnl_cb, IPC_CPU1_TRAP_HANDLE_END);
}
#endif

#ifdef AMP_RES_CLIENT
bk_err_t ipc_send_res_acquire_cnt(u16 resource_id, u16 cpu_id, amp_res_req_cnt_t *cnt_list)
{
	ipc_res_req_t	res_req;

	res_req.res_id = resource_id;
	res_req.cpu_id = cpu_id;

	return ipc_send_cmd(&ipc_chnl_cb, IPC_RES_ACQUIRE_CNT, \
						(u8 *)&res_req, sizeof(res_req), (u8 *)cnt_list, sizeof(amp_res_req_cnt_t));
}

bk_err_t ipc_send_res_release_cnt(u16 resource_id, u16 cpu_id, amp_res_req_cnt_t *cnt_list)
{
	ipc_res_req_t	res_req;

	res_req.res_id = resource_id;
	res_req.cpu_id = cpu_id;

	return ipc_send_cmd(&ipc_chnl_cb, IPC_RES_RELEASE_CNT, \
						(u8 *)&res_req, sizeof(res_req), (u8 *)cnt_list, sizeof(amp_res_req_cnt_t));
}

u8 ipc_send_alloc_dma_chnl(u32 user_id)
{
	bk_err_t	ret_val = BK_FAIL;
	u8	dma_chnl_id = -1;

	ret_val = ipc_send_cmd(&ipc_chnl_cb, IPC_ALLOC_DMA_CHNL, \
						(u8 *)&user_id, sizeof(user_id), (u8 *)&dma_chnl_id, sizeof(dma_chnl_id));

	if(ret_val != BK_OK)
		return (u8)(-1);

	return dma_chnl_id;
}

bk_err_t ipc_send_free_dma_chnl(u32 user_id, u8 chnl_id)
{
	bk_err_t	ret_val = BK_FAIL;

	ipc_dma_free_t	dma_free;

	dma_free.user_id = user_id;
	dma_free.chnl_id = chnl_id;

	ret_val = ipc_send_cmd(&ipc_chnl_cb, IPC_FREE_DMA_CHNL, \
						(u8 *)&dma_free, sizeof(dma_free), NULL, 0);

	return ret_val;
}

u32 ipc_send_dma_chnl_user(u8 chnl_id)
{
	bk_err_t	ret_val = BK_FAIL;
	u32		user_id = -1;

	ret_val = ipc_send_cmd(&ipc_chnl_cb, IPC_DMA_CHNL_USER, \
						(u8 *)&chnl_id, sizeof(chnl_id), (u8 *)&user_id, sizeof(user_id));

	if(ret_val != BK_OK)
		return (u32)(-1);

	return user_id;
}

u32 ipc_vote_flash_line_mode(u32 v_line_mode)
{
	bk_err_t	ret_val = BK_FAIL;
	u32 ret_line_mode = -1;

	ret_val = ipc_send_cmd(&ipc_chnl_cb, IPC_FLASH_VOTE_LINE_MODE, \
						(u8 *)&v_line_mode, sizeof(v_line_mode), (u8 *)&ret_line_mode, sizeof(ret_line_mode));

	if(ret_val != BK_OK)
		return (u32)(-1);

	return ret_line_mode;
}

#endif

#endif
