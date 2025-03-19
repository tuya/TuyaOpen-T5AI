
#if CONFIG_TUYA_LOG_OPTIMIZATION

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "cli.h"
#include <os/os.h>
#include <common/bk_compiler.h>
#include "shell_drv.h"
#include <components/ate.h>
#include <modules/pm.h>
#include <driver/gpio.h>
#include "bk_wdt.h"
#if CONFIG_AT
#include "atsvr_port.h"
#endif

#define DEV_UART        1
#define DEV_MAILBOX     2

#if CONFIG_SYS_PRINT_DEV_UART
#define LOG_DEV    DEV_UART
#define CMD_DEV    DEV_UART
#elif CONFIG_SYS_PRINT_DEV_MAILBOX
#define LOG_DEV    DEV_MAILBOX
#define CMD_DEV    DEV_MAILBOX
#endif

// #if (CMD_DEV != DEV_MAILBOX)
#if (CONFIG_SYS_CPU0)
#if CONFIG_MAILBOX
#ifndef CONFIG_FREERTOS_SMP
#define FWD_CMD_TO_MBOX
#define RECV_CMD_LOG_FROM_MBOX
#endif
#endif
#endif

#if defined(FWD_CMD_TO_MBOX)
#define MBOX_RSP_BLK_ID			0
#define MBOX_IND_BLK_ID			1

#define MBOX_FWD_PEND_NUM		2    /* (1 Rsp + 1 Ind) */
#else
#define MBOX_FWD_PEND_NUM		0
#endif

#define SHELL_WAIT_OUT_TIME		(2000) 		// 2s.

#define SHELL_TASK_WAIT_TIME	(100)		// 100ms
#define SHELL_TASK_WAKE_CYCLE	(20)		// 2s

#define SHELL_EVENT_TX_REQ  	0x01
#define SHELL_EVENT_RX_IND  	0x02
#define SHELL_EVENT_WAKEUP      0x04

#if (LOG_DEV == DEV_MAILBOX)   //cpu1
#define SHELL_LOG_BUF1_LEN      1024
#define SHELL_LOG_BUF2_LEN      512
#define SHELL_LOG_BUF3_LEN      128

#define SHELL_LOG_BUF1_NUM      10
#define SHELL_LOG_BUF2_NUM      64
#define SHELL_LOG_BUF3_NUM      1024
#endif   //  (LOG_DEV == DEV_MAILBOX)

#if (LOG_DEV == DEV_UART)      //cpu0
#define SHELL_LOG_BUF1_LEN      1024
#define SHELL_LOG_BUF2_LEN      512
#define SHELL_LOG_BUF3_LEN      128
#if CONFIG_RELEASE_VERSION
#define SHELL_LOG_BUF1_NUM      10
#define SHELL_LOG_BUF2_NUM      64
#define SHELL_LOG_BUF3_NUM      512
#else
#if !CONFIG_UART_RING_BUFF
#define SHELL_LOG_BUF1_NUM      10
#define SHELL_LOG_BUF2_NUM      64
#define SHELL_LOG_BUF3_NUM      512
#else
#define SHELL_LOG_BUF1_NUM      8
#define SHELL_LOG_BUF2_NUM      30
#define SHELL_LOG_BUF3_NUM      50
#endif
#endif
#endif   // (LOG_DEV == DEV_UART)

#define SHELL_LOG_BUF_NUM       (SHELL_LOG_BUF1_NUM + SHELL_LOG_BUF2_NUM + SHELL_LOG_BUF3_NUM)
#define SHELL_LOG_PEND_NUM      (SHELL_LOG_BUF_NUM * 2 + 4 + MBOX_FWD_PEND_NUM)
/* the worst case may be (one log + one hint) in pending queue, so twice the log_num for pending queue.*/
/* 1: RSP, 1: reserved(queue empty), 1: cmd ovf, 1: ind). */
/* MBOX_FWD_PEND_NUM (1 Rsp + 1 Ind) every slave core. */
#define SHELL_LOG_BUSY_NUM      (8)

#if (CMD_DEV == DEV_MAILBOX)
#define SHELL_RX_BUF_LEN		140
#endif
#if (CMD_DEV == DEV_UART)
#define SHELL_RX_BUF_LEN		4
#endif

#define SHELL_ASSERT_BUF_LEN	140
#define SHELL_CMD_BUF_LEN		200
#define SHELL_RSP_BUF_LEN		140
#define SHELL_IND_BUF_LEN		132

#define SHELL_RSP_QUEUE_ID	    (7)
#define SHELL_FWD_QUEUE_ID      (8)
#define SHELL_ROM_QUEUE_ID		(9)
#define SHELL_IND_QUEUE_ID		(10)

#define MAX_TRACE_ARGS      10
#define MOD_NAME_LEN        4

#define HEX_SYNC_CHAR       0xFE
#define HEX_MOD_CHAR        0xFF
#define HEX_ESC_CHAR        0xFD

#define TBL_SIZE(tbl)		(sizeof(tbl) / sizeof(tbl[0]))

typedef struct
{
	beken_semaphore_t   event_semaphore;  // will release from ISR.
	u32       event_flag;
} os_ext_event_t;

enum
{
	CMD_TYPE_TEXT = 0,
	CMD_TYPE_HEX,
	CMD_TYPE_BKREG,   /* patch for BK_REG tool cmd. */
	CMD_TYPE_INVALID,
};

/* patch for BK_REG tool. */
enum
{
	BKREG_WAIT_01 = 0,
	BKREG_WAIT_E0,
	BKREG_WAIT_FC,
};

typedef struct
{
	u8     rsp_buff[SHELL_RSP_BUF_LEN];
	beken_semaphore_t   rsp_buf_semaphore;

	u8     rx_buff[SHELL_RX_BUF_LEN];

	u8     cur_cmd_type;
	u8     cmd_buff[SHELL_CMD_BUF_LEN];
	u16    cmd_data_len;
	u8     cmd_ovf_hint;

	/* patch for BK_REG tool. */
	/* added one state machine for BK_REG tool cmd. */
	u8     bkreg_state;
	u8     bkreg_left_byte;
	/* patch end. */

	u8     assert_buff[SHELL_ASSERT_BUF_LEN];

	u8     log_level;
	u8     echo_enable;
	u8     log_flush;

	/* patch for AT cmd handling. */
	u8     cmd_ind_buff[SHELL_IND_BUF_LEN];
	beken_semaphore_t   ind_buf_semaphore;
} cmd_line_t;

#define GET_BLOCK_ID(blocktag)          ((blocktag) & 0x7FF)
#define GET_QUEUE_ID(blocktag)          (((blocktag) & 0xF800) >> 11)
#define MAKE_BLOCK_TAG(blk_id, q_id)    (((blk_id) & 0x7FF) | (((q_id) & 0x1F) << 11) )

typedef struct
{
	u16     blk_tag;        /* bit0~bit7: blk_id,    bit8~bit11: queue_id; */
	u16     packet_len;
} tx_packet_t;

typedef struct
{
	tx_packet_t     packet_list[SHELL_LOG_PEND_NUM];
	u16     list_out_idx;
	u16     list_in_idx;
} pending_queue_t;

typedef struct
{
	u16     blk_list[SHELL_LOG_BUSY_NUM];
	u16     list_out_idx;
	u16     list_in_idx;
	u16     free_cnt;
} busy_queue_t;

typedef struct
{
	u8   *  log_buf;
	u16   * blk_list;
	u16      blk_num;
	u16      blk_len;
	u16     list_out_idx;
	u16     list_in_idx;
	u16     free_blk_num;
	u32     ovf_cnt;
} free_queue_t;

#if (CONFIG_CACHE_ENABLE) && (CONFIG_LV_USE_DEMO_METER)
#define  SHELL_DECLARE_MEMORY_ATTR __attribute__((section(".sram_cache")))
#elif CONFIG_SOC_BK7258 && CONFIG_SYS_PRINT_DEV_UART
#define  SHELL_DECLARE_MEMORY_ATTR __attribute__((section(".dtcm_section")))
#else
#define  SHELL_DECLARE_MEMORY_ATTR
#endif


static free_queue_t       free_queue[3];

#if (CONFIG_CACHE_ENABLE) && (CONFIG_LV_USE_DEMO_METER)
static __attribute__((section(".sram_cache")))  busy_queue_t       log_busy_queue;
static __attribute__((section(".sram_cache")))  pending_queue_t    pending_queue;
#else
static busy_queue_t       log_busy_queue;
static pending_queue_t    pending_queue;
#endif

#if (CONFIG_CACHE_ENABLE) && (CONFIG_LV_USE_DEMO_METER)
static __attribute__((section(".sram_cache")))  cmd_line_t  cmd_line_buf;
#else
static cmd_line_t  cmd_line_buf;
#endif

#if (LOG_DEV == DEV_UART)
static shell_dev_t * log_dev = &shell_uart;
#endif
#if (LOG_DEV == DEV_MAILBOX)
static shell_dev_t * log_dev = &shell_dev_mb;
#endif

#if (CMD_DEV == DEV_UART)
static shell_dev_t * cmd_dev = &shell_uart;
#endif
#if (CMD_DEV == DEV_MAILBOX)
static shell_dev_t * cmd_dev = &shell_dev_mb;
#endif

#if (CONFIG_SYS_CPU0)
static const char	 shell_fault_str[] = "\r\n!!some LOGs discarded!!\r\n";
static const u16     shell_fault_str_len = sizeof(shell_fault_str) - 1;
#endif
#if (!CONFIG_SYS_CPU0)
static const char	 shell_fault_str[] = "\r\n!!CPUx:some LOGs discarded!!\r\n";
static const u16     shell_fault_str_len = sizeof(shell_fault_str) - 1;
#endif

#if defined(FWD_CMD_TO_MBOX) || defined(RECV_CMD_LOG_FROM_MBOX)

typedef struct
{
	log_cmd_t	rsp_buf;
	log_cmd_t   ind_buf;
} fwd_slave_data_t;

static fwd_slave_data_t  ipc_fwd_data;

static shell_dev_ipc_t * ipc_dev = &shell_dev_ipc;

static int result_fwd(int blk_id);
static u32 shell_ipc_rx_indication(u16 cmd, log_cmd_t *data, u16 cpu_id);

#endif

static const char	 shell_cmd_ovf_str[] = "\r\n!!some CMDs lost!!\r\n";
static const u16     shell_cmd_ovf_str_len = sizeof(shell_cmd_ovf_str) - 1;
static const char  * shell_prompt_str[2] = {"\r\n$", "\r\n#"};

static u8     shell_init_ok = bFALSE;
static u8     fault_hint_print = 0;
static u32    shell_log_overflow = 0;
static u32    shell_log_count = 0;
static u8     prompt_str_idx = 0;

static u32    shell_pm_wake_time = SHELL_TASK_WAKE_CYCLE;   // wait cycles before enter sleep.
static u8     shell_pm_wake_flag = 1;

#if (CONFIG_CPU_CNT > 1) && (LOG_DEV != DEV_MAILBOX)
/* patch for multi-cpu dump. */
static u8     shell_log_owner_cpu = 0;
static u8     shell_log_req_cpu = 0;
#endif

#ifdef CONFIG_FREERTOS_SMP
#include "spinlock.h"
static volatile spinlock_t shell_spin_lock = SPIN_LOCK_INIT;
#endif // CONFIG_FREERTOS_SMP

static inline uint32_t shell_task_enter_critical()
{
	uint32_t flags = rtos_disable_int();

#ifdef CONFIG_FREERTOS_SMP
	spin_lock(&shell_spin_lock);
#endif // CONFIG_FREERTOS_SMP

	return flags;
}

static inline void shell_task_exit_critical(uint32_t flags)
{
#ifdef CONFIG_FREERTOS_SMP
	spin_unlock(&shell_spin_lock);
#endif // CONFIG_FREERTOS_SMP

	rtos_enable_int(flags);
}

#if 1

static os_ext_event_t   shell_task_event;

static bool_t create_shell_event(void)
{
	shell_task_event.event_flag = 0;
	rtos_init_semaphore(&shell_task_event.event_semaphore, 1);

	return bTRUE;
}

/* this API may be called from ISR. */
bool_t set_shell_event(u32 event_flag)
{
	u32  int_mask;

	int_mask = shell_task_enter_critical();

	shell_task_event.event_flag |= event_flag;

	shell_task_exit_critical(int_mask);

	rtos_set_semaphore(&shell_task_event.event_semaphore);

	return bTRUE;
}

u32 wait_any_event(u32 timeout)
{
	u32  int_mask;
	u32  event_flag;

	int  result;

	while(bTRUE)
	{
		int_mask = shell_task_enter_critical();

		event_flag = shell_task_event.event_flag;
		shell_task_event.event_flag = 0;

		shell_task_exit_critical(int_mask);

		if((event_flag != 0) || (timeout == 0))
		{
			return event_flag;
		}
		else
		{
			result = rtos_get_semaphore(&shell_task_event.event_semaphore, timeout);

			if(result == kTimeoutErr)
				return 0;
		}
	}

}

#else

static beken_semaphore_t   shell_semaphore;  // will release from ISR.

static bool_t create_shell_event(void)
{
	rtos_init_semaphore(&shell_semaphore, 1);

	return bTRUE;
}

static bool_t set_shell_event(u32 event_flag)
{
	(void)event_flag;

	rtos_set_semaphore(&shell_semaphore);

	return bTRUE;
}

static u32 wait_any_event(u32 timeout)
{
	int result;

	result = rtos_get_semaphore(&shell_semaphore, timeout);

	if(result == kTimeoutErr)
		return 0;

	return SHELL_EVENT_RX_IND;
}
#endif

static void tx_req_process(void);

static u8 * alloc_log_blk(u16 log_len, u16 *blk_tag)
{
	u16    free_blk_id;
	u8     queue_id;
	u8     ovf_cnt_saved = 0;
	free_queue_t * free_q;
	u8     *       blk_buf = NULL;

	u32  int_mask = shell_task_enter_critical();

	for(queue_id = 0; queue_id < TBL_SIZE(free_queue); queue_id++)
	{
		free_q = &free_queue[queue_id];

		/*    queue ascending in blk_len.    */
		if(free_q->blk_len < log_len)
			continue;

		if(free_q->free_blk_num > 0)
		{
			free_blk_id = free_q->blk_list[free_q->list_out_idx];

			// free_q->list_out_idx = (free_q->list_out_idx + 1) % free_q->blk_num;
			if((free_q->list_out_idx + 1) < free_q->blk_num)
				free_q->list_out_idx++;
			else
				free_q->list_out_idx = 0;

			free_q->free_blk_num--;

			blk_buf = &free_q->log_buf[free_blk_id * free_q->blk_len];
			*blk_tag = MAKE_BLOCK_TAG(free_blk_id, queue_id);

			break;
		}
		else
		{
			if(ovf_cnt_saved == 0)
			{
				free_q->ovf_cnt++;
				ovf_cnt_saved = 1;
			}
		}
	}

	if(blk_buf == NULL)
	{
		shell_log_overflow++;
	}
	else
	{
		fault_hint_print = 0;
		shell_log_count++;
	}

	shell_task_exit_critical(int_mask);

	return blk_buf;
}

static bool_t free_log_blk(u16 block_tag)
{
	u8      queue_id = GET_QUEUE_ID(block_tag);
	u16     blk_id = GET_BLOCK_ID(block_tag);
	free_queue_t *free_q;

	if(queue_id >= TBL_SIZE(free_queue))
		return bFALSE;

	free_q = &free_queue[queue_id];

	if(blk_id >= free_q->blk_num)
		return bFALSE;

	//disable_interrupt(); // called from tx-complete only, don't lock interrupt.

	free_q->blk_list[free_q->list_in_idx] = blk_id;

	//free_q->list_in_idx = (free_q->list_in_idx + 1) % free_q->blk_num;
	if((free_q->list_in_idx + 1) < free_q->blk_num)
		free_q->list_in_idx++;
	else
		free_q->list_in_idx = 0;

	free_q->free_blk_num++;

	//enable_interrupt(); // called from tx-complete only, don't lock interrupt.

	return bTRUE;
}

static int merge_log_data(u16 blk_tag, u16 data_len)
{
	if(pending_queue.list_out_idx == pending_queue.list_in_idx)  /* queue empty! */
	{
		return 0;
	}

	u8      queue_id = GET_QUEUE_ID(blk_tag);
	u16     blk_id = GET_BLOCK_ID(blk_tag);
	if(queue_id >= TBL_SIZE(free_queue))  /* not log buffer */
	{
		return 0;
	}

	free_queue_t *free_q;
	free_q = &free_queue[queue_id];

	u8		* src_buf = NULL;

	if(blk_id < free_q->blk_num)
	{
		src_buf = &free_q->log_buf[blk_id * free_q->blk_len];
	}
	else
		return 0;

	u16    pre_in_idx;

	if(pending_queue.list_in_idx > 0)
		pre_in_idx = pending_queue.list_in_idx - 1;
	else
		pre_in_idx = SHELL_LOG_PEND_NUM - 1;

	queue_id = GET_QUEUE_ID(pending_queue.packet_list[pre_in_idx].blk_tag);
	blk_id = GET_BLOCK_ID(pending_queue.packet_list[pre_in_idx].blk_tag);
	if(queue_id >= TBL_SIZE(free_queue))  /* not log buffer */
	{
		return 0;
	}

	u8		* dst_buf = NULL;
	u16       buf_len = 0;

	free_q = &free_queue[queue_id];

	if(blk_id < free_q->blk_num)
	{
		dst_buf = &free_q->log_buf[blk_id * free_q->blk_len];
		buf_len = free_q->blk_len;
	}
	else
		return 0;

	u16       cur_len = pending_queue.packet_list[pre_in_idx].packet_len;

	if((cur_len + data_len) > buf_len)  /* can be merged into one buffer? */
		return 0;

	memcpy(dst_buf + cur_len, src_buf, data_len);

	pending_queue.packet_list[pre_in_idx].packet_len += data_len;

	free_log_blk(blk_tag);  /* merged, free the log buffer. */

	return 1;
}

/* call this in interrupt !* DISABLED *! context. */
static void push_pending_queue(u16 blk_tag, u16 data_len)
{
	//get_shell_mutex();

	if(merge_log_data(blk_tag, data_len))  /* has been merged? if so, doesn't enqueue the log. */
		return;

	pending_queue.packet_list[pending_queue.list_in_idx].blk_tag = blk_tag;
	pending_queue.packet_list[pending_queue.list_in_idx].packet_len = data_len;

	//pending_queue.list_in_idx = (pending_queue.list_in_idx + 1) % SHELL_LOG_PEND_NUM;
	if((pending_queue.list_in_idx + 1) < SHELL_LOG_PEND_NUM)
		pending_queue.list_in_idx++;
	else
		pending_queue.list_in_idx = 0;

	//release_shell_mutex();

	return;
}

/* call this in interrupt !* DISABLED *! context. */
static void pull_pending_queue(u16 *blk_tag, u16 *data_len)
{
	*blk_tag     = pending_queue.packet_list[pending_queue.list_out_idx].blk_tag;
	*data_len   = pending_queue.packet_list[pending_queue.list_out_idx].packet_len;

	//pending_queue.list_out_idx = (pending_queue.list_out_idx + 1) % SHELL_LOG_PEND_NUM;
	if((pending_queue.list_out_idx + 1) < SHELL_LOG_PEND_NUM)
		pending_queue.list_out_idx++;
	else
		pending_queue.list_out_idx = 0;

	return;
}

int shell_assert_out(bool bContinue, char * format, ...);

/* call from cmd TX ISR. */
static int cmd_tx_complete(u8 *pbuf, u16 buf_tag)
{
	u8      queue_id = GET_QUEUE_ID(buf_tag);
	u16     blk_id = GET_BLOCK_ID(buf_tag);

	/* rsp ok ?? */
	if( queue_id == SHELL_RSP_QUEUE_ID )    /* rsp. */
	{
		/* it is called from cmd_dev tx ISR. */

		if ( (pbuf != cmd_line_buf.rsp_buff) || (blk_id != 0) )
		{
			/* something wrong!!! */
			shell_assert_out(bTRUE, "FAULT: in rsp.\r\n");
		}

		/* rsp compelete, rsp_buff can be used for next cmd/response. */
		rtos_set_semaphore(&cmd_line_buf.rsp_buf_semaphore);

		return 1;
	}

	if( queue_id == SHELL_IND_QUEUE_ID )    /* cmd_ind. */
	{
		/* it is called from cmd_dev tx ISR. */

		if ( (pbuf != cmd_line_buf.cmd_ind_buff) || (blk_id != 0) )
		{
			/* something wrong!!! */
			shell_assert_out(bTRUE, "FAULT: indication.\r\n");
		}

		/* indication tx compelete, cmd_ind_buff can be used for next cmd_indication. */
		rtos_set_semaphore(&cmd_line_buf.ind_buf_semaphore);

		return 1;
	}

	if( queue_id == SHELL_ROM_QUEUE_ID )    /* fault hints buffer, point to flash. */
	{
		/* it is called from cmd_dev tx ISR. */

		if (blk_id == 1)
		{
			if(pbuf != (u8 *)shell_cmd_ovf_str)
			{
				/* something wrong!!! */
				shell_assert_out(bTRUE, "FATAL:t-%x,p-%x\r\n", buf_tag, pbuf);
			}

			cmd_line_buf.cmd_ovf_hint = 0;

			return 1;
		}
	}

	if( queue_id == SHELL_FWD_QUEUE_ID )    /* slave buffer. */
	{
		#if defined(FWD_CMD_TO_MBOX)
		result_fwd(blk_id);
		return 1;
		#endif
	}

	return 0;
}
/* call from TX ISR. */
static void shell_cmd_tx_complete(u8 *pbuf, u16 buf_tag)
{
	u32  int_mask = shell_task_enter_critical();
	int  tx_handled = cmd_tx_complete(pbuf, buf_tag);

	if(tx_handled == 0)  /* not handled. */
	{
		/*        FAULT !!!!      */
		shell_assert_out(bTRUE, "FATAL:%x,\r\n", buf_tag);
	}

	shell_task_exit_critical(int_mask);
}

/* call from log TX ISR. */
static int log_tx_complete(u8 *pbuf, u16 buf_tag)
{
	u16     block_tag;
	u8      queue_id = GET_QUEUE_ID(buf_tag);
	u16     blk_id = GET_BLOCK_ID(buf_tag);
	free_queue_t *free_q;

	if( queue_id == SHELL_ROM_QUEUE_ID )    /* fault hints buffer, point to flash. */
	{
		/* it is called from log_dev tx ISR. */

		if (blk_id == 0)
		{
			if(pbuf != (u8 *)shell_fault_str)
			{
				/* something wrong!!! */
				shell_assert_out(bTRUE, "FATAL:t-%x,p-%x\r\n", buf_tag, pbuf);
			}

			return 1;
		}
	}

	if (queue_id < TBL_SIZE(free_queue))   /* from log busy queue. */
	{
		/* it is called from log_dev tx ISR. */

		free_q = &free_queue[queue_id];

		block_tag = log_busy_queue.blk_list[log_busy_queue.list_out_idx];

		if( ( buf_tag != block_tag ) || (blk_id >= free_q->blk_num) ||
			( (&free_q->log_buf[blk_id * free_q->blk_len]) != pbuf) )
		{
			/* something wrong!!! */
			/*        FAULT !!!!      */
			shell_assert_out(bTRUE, "FATAL:%x,%x\r\n", buf_tag, block_tag);

			return -1;
		}

		/* de-queue from busy queue. */
		//log_busy_queue.list_out_idx = (log_busy_queue.list_out_idx + 1) % SHELL_LOG_BUSY_NUM;
		if((log_busy_queue.list_out_idx + 1) < SHELL_LOG_BUSY_NUM)
			log_busy_queue.list_out_idx++;
		else
			log_busy_queue.list_out_idx = 0;

		log_busy_queue.free_cnt++;

		/* free buffer to queue. */
		free_log_blk(block_tag);

		return 1;
	}

	return 0;
}

/* call from TX ISR. */
static void shell_log_tx_complete(u8 *pbuf, u16 buf_tag)
{
	u32  int_mask = shell_task_enter_critical();

	int log_tx_req = log_tx_complete(pbuf, buf_tag);

	if(log_tx_req == 1)
	{
		//set_shell_event(SHELL_EVENT_TX_REQ);  // notify shell task to process the log tx.
		tx_req_process();
	}
	else if(log_tx_req == 0)  /* not handled. */
	{
		/*        FAULT !!!!      */
		shell_assert_out(bTRUE, "FATAL:%x,\r\n", buf_tag);
	}

	shell_task_exit_critical(int_mask);
}

/* call from TX ISR. */
static void shell_tx_complete(u8 *pbuf, u16 buf_tag)
{
	u32  int_mask = shell_task_enter_critical();

	int tx_req = 0;

	tx_req = cmd_tx_complete(pbuf, buf_tag);

	if(tx_req == 0) /* not a cmd tx event, maybe it is a log tx event. */
	{
		tx_req = log_tx_complete(pbuf, buf_tag);
	}

	if(tx_req == 1)
	{
		//set_shell_event(SHELL_EVENT_TX_REQ);  // notify shell task to process the log tx.
		tx_req_process();
	}
	else if(tx_req == 0)  /* not handled. */
	{
		/*        FAULT !!!!      */
		shell_assert_out(bTRUE, "FATAL:%x,\r\n", buf_tag);
	}

	shell_task_exit_critical(int_mask);
}

/* call from RX ISR. */
static void shell_rx_indicate(void)
{
	set_shell_event(SHELL_EVENT_RX_IND);

	return;
}

static u16 append_link_data_byte(u8 * link_buf, u16 buf_len, u8 * data_ptr, u16 data_len)
{
	u16   cnt = 0, i;

	for(i = 0; i < data_len; i++)
	{
		if( (*data_ptr == HEX_SYNC_CHAR) ||
			(*data_ptr == HEX_ESC_CHAR) )
		{
			if(cnt < (buf_len - 1))
			{
				link_buf[cnt] = HEX_ESC_CHAR;
				cnt++;
				link_buf[cnt] = (*data_ptr) ^ HEX_MOD_CHAR;
				cnt++;
			}
		}
		else
		{
			if(cnt < buf_len)
			{
				link_buf[cnt] = (*data_ptr);
				cnt++;
			}
		}

		data_ptr++;
	}

	return cnt;
}

static bool_t echo_out(u8 * echo_str, u16 len)
{
	u16	 wr_cnt;

	if(len == 0)
		return bTRUE;

	wr_cnt = cmd_dev->dev_drv->write_echo(cmd_dev, echo_str, len);

	return (wr_cnt == len);
}

static void cmd_info_out(u8 * msg_buf, u16 msg_len, u16 blk_tag)
{
	if(msg_len == 0)
		return;

	u32  int_mask = shell_task_enter_critical();

	if(log_dev != cmd_dev)
	{
		/* dedicated device for cmd, don't enqueue the msg to pending queue. */
		/* send to cmd dev directly. */
		/* should have a count semaphore for write_asyn calls for rsp/ind/cmd_hint & slave rsp/ind. *
		 * otherwise there will be coupled with driver, drv tx_queue_len MUST be >= 5. */
		cmd_dev->dev_drv->write_async(cmd_dev, msg_buf, msg_len, blk_tag);

	}
	else
	{
		/* shared device for cmd & log, push the rsp msg to pending queue. */
		push_pending_queue(blk_tag, msg_len);

		//set_shell_event(SHELL_EVENT_TX_REQ);  // notify shell task to process the log tx. can't be called in int-disabled context.
		tx_req_process();
	}

	shell_task_exit_critical(int_mask);
}

/*    NOTICE:  this can only be called by shell task internally (cmd handler). */
/*             it is not a re-enterance function becaue of using rsp_buff. */
static bool_t cmd_rsp_out(u8 * rsp_msg, u16 msg_len)
{
	u16    rsp_blk_tag = MAKE_BLOCK_TAG(0, SHELL_RSP_QUEUE_ID);

	if(rsp_msg != cmd_line_buf.rsp_buff)
	{
		if(msg_len > sizeof(cmd_line_buf.rsp_buff))
		{
			msg_len = sizeof(cmd_line_buf.rsp_buff);;
		}

		memcpy(cmd_line_buf.rsp_buff, rsp_msg, msg_len);
	}

	cmd_info_out(cmd_line_buf.rsp_buff, msg_len, rsp_blk_tag);

	return bTRUE;
}

/* it is not a re-enterance function, should sync using ind_buf_semaphore. */
static bool_t cmd_ind_out(u8 * ind_msg, u16 msg_len)
{
	u16    ind_blk_tag = MAKE_BLOCK_TAG(0, SHELL_IND_QUEUE_ID);

	if(ind_msg != cmd_line_buf.cmd_ind_buff)
	{
		if(msg_len > sizeof(cmd_line_buf.cmd_ind_buff))
		{
			msg_len = sizeof(cmd_line_buf.cmd_ind_buff);;
		}

		memcpy(cmd_line_buf.cmd_ind_buff, ind_msg, msg_len);
	}

	cmd_info_out(cmd_line_buf.cmd_ind_buff, msg_len, ind_blk_tag);

	return bTRUE;
}

static bool_t cmd_hint_out(void)
{
	u16    hint_blk_tag = MAKE_BLOCK_TAG(1, SHELL_ROM_QUEUE_ID);

	cmd_info_out((u8 *)shell_cmd_ovf_str, shell_cmd_ovf_str_len, hint_blk_tag);

	return bTRUE;
}

static bool_t log_hint_out(void)
{
	if(fault_hint_print)	/* sent one hint since last allocation fail.*/
		return bTRUE;

	u16    hint_blk_tag = MAKE_BLOCK_TAG(0, SHELL_ROM_QUEUE_ID);

	u32  int_mask = shell_task_enter_critical();

	push_pending_queue(hint_blk_tag, shell_fault_str_len);

	//set_shell_event(SHELL_EVENT_TX_REQ);  // notify shell task to process the log tx.
	tx_req_process();

	fault_hint_print = 1;

	shell_task_exit_critical(int_mask);

	return bTRUE;
}

/* call this in interrupt !* DISABLED *! context. */
static void tx_req_process(void)
{
	u8		*packet_buf = NULL;
	u16		block_tag;
	u16		log_len;
	u16		tx_ready;
	u16		blk_id;
	u8		queue_id;
	free_queue_t *free_q;

	/* maybe tx_req is from tx_complete_callback, check if there any log in queue. */
	if(pending_queue.list_out_idx == pending_queue.list_in_idx)  /* queue empty! */
		return;

	if(log_busy_queue.free_cnt == 0)
		return;

	tx_ready = 0;

	log_dev->dev_drv->io_ctrl(log_dev, SHELL_IO_CTRL_GET_STATUS, &tx_ready);

	if(tx_ready == 0)
		return;

	/**    ====     POP from pending queue     ====    **/
	pull_pending_queue(&block_tag, &log_len);

	queue_id = GET_QUEUE_ID(block_tag);
	blk_id = GET_BLOCK_ID(block_tag);

	if (queue_id < TBL_SIZE(free_queue))
	{
		free_q = &free_queue[queue_id];

		if(blk_id < free_q->blk_num)
		{
			packet_buf = &free_q->log_buf[blk_id * free_q->blk_len];
		}
	}
	else if(queue_id == SHELL_RSP_QUEUE_ID)
	{
		packet_buf = cmd_line_buf.rsp_buff;

		if((log_dev != cmd_dev) || (blk_id != 0))
		{
			shell_assert_out(bTRUE, "xFATAL: in Tx_req\r\n");
			/*		  FAULT !!!!	  */
			/* if log_dev is not the same with cmd_dev,
			 * rsp will not be pushed into pending queue.
			 */
		}
	}
	else if(queue_id == SHELL_IND_QUEUE_ID)
	{
		packet_buf = cmd_line_buf.cmd_ind_buff;

		if((log_dev != cmd_dev) || (blk_id != 0))
		{
			shell_assert_out(bTRUE, "xFATAL: in Tx_req\r\n");
			/*		  FAULT !!!!	  */
			/* if log_dev is not the same with cmd_dev,
			 * indication will not be pushed into pending queue.
			 */
		}
	}
	else if(queue_id == SHELL_ROM_QUEUE_ID)
	{
		if(blk_id == 0)
		{
			packet_buf = (u8 *)shell_fault_str;
		}
		else if(blk_id == 1)
		{
			packet_buf = (u8 *)shell_cmd_ovf_str;
			if(log_dev != cmd_dev)
			{
				shell_assert_out(bTRUE, "xFATAL: in Tx_req id=%x\r\n", blk_id);
				/*		  FAULT !!!!	  */
				/* if log_dev is not the same with cmd_dev,
				 * cmd_hint will not be pushed into pending queue.
				 */
			}
		}
		else
		{
				/*		  FAULT !!!!	  */
				shell_assert_out(bTRUE, "xFATAL: in Tx_req id=%x\r\n", blk_id);
		}
	}
	#if defined(FWD_CMD_TO_MBOX)
	else if(queue_id == SHELL_FWD_QUEUE_ID)
	{
		if(blk_id == MBOX_RSP_BLK_ID)
		{
			packet_buf = (u8 *)ipc_fwd_data.rsp_buf.buf;
		}
		else if(blk_id == MBOX_IND_BLK_ID)
		{
			packet_buf = (u8 *)ipc_fwd_data.ind_buf.buf;
			if(log_dev != cmd_dev)
			{
				shell_assert_out(bTRUE, "xFATAL: in Tx_req id=%x\r\n", blk_id);
				/*		  FAULT !!!!	  */
				/* if log_dev is not the same with cmd_dev,
				 * fwd_data will not be pushed into pending queue.
				 */
			}
		}
		else
		{
				/*		  FAULT !!!!	  */
				shell_assert_out(bTRUE, "xFATAL: in Tx_req id=%x\r\n", blk_id);
		}
	}
	#endif
	else
	{
		/*		  FAULT !!!!	  */
		shell_assert_out(bTRUE, "xFATAL: in Tx_req %x.\r\n", block_tag);
	}

	if(packet_buf == NULL)
		return;

	/* rom & rsp buff not enter busy-queue. */
	if(queue_id < TBL_SIZE(free_queue))
	{
		log_busy_queue.free_cnt--;
		log_busy_queue.blk_list[log_busy_queue.list_in_idx] = block_tag;
		//log_busy_queue.list_in_idx = (log_busy_queue.list_in_idx + 1) % SHELL_LOG_BUSY_NUM;
		if((log_busy_queue.list_in_idx + 1) < SHELL_LOG_BUSY_NUM)
			log_busy_queue.list_in_idx++;
		else
			log_busy_queue.list_in_idx = 0;
	}

	log_dev->dev_drv->write_async(log_dev, packet_buf, log_len, block_tag); /* send to log dev driver. */
	/* if driver return 0, should free log-block or not de-queue pending queue and try again. */
	/* if return 1, push log-block into busy queue is OK. */

	return;
}

int shell_trace_out( u32 trace_id, ... );
int shell_spy_out( u16 spy_id, u8 * data_buf, u16 data_len);

static void rx_ind_process(void)
{
	u16   read_cnt, buf_len, echo_len;
	u16   i = 0;
	u8    cmd_rx_done = bFALSE, need_backspace = bFALSE;

	if(cmd_dev->dev_type == SHELL_DEV_MAILBOX)
	{
		buf_len = SHELL_RX_BUF_LEN;
	}
	else /* if(cmd_dev->dev_type == SHELL_DEV_UART) */
	{
		buf_len = 1;  /* for UART device, read one by one. */
	}

	while(bTRUE)
	{
		u8  * rx_temp_buff = &cmd_line_buf.rx_buff[0];

		read_cnt = cmd_dev->dev_drv->read(cmd_dev, rx_temp_buff, buf_len);

		echo_len = 0;

		for(i = 0; i < read_cnt; i++)
		{
			if(cmd_line_buf.cur_cmd_type == CMD_TYPE_INVALID)
			{
				echo_len++;

				if((rx_temp_buff[i] >= 0x20) && (rx_temp_buff[i] < 0x7f))
				{
					cmd_line_buf.cur_cmd_type = CMD_TYPE_TEXT;

					cmd_line_buf.cmd_data_len = 0;
					cmd_line_buf.cmd_buff[cmd_line_buf.cmd_data_len] = rx_temp_buff[i];
					cmd_line_buf.cmd_data_len++;

					continue;
				}

				/* patch for BK_REG tool. */
				if(cmd_line_buf.bkreg_state == BKREG_WAIT_01)
				{
					if(rx_temp_buff[i] == 0x01)
						cmd_line_buf.bkreg_state = BKREG_WAIT_E0;
				}
				else if(cmd_line_buf.bkreg_state == BKREG_WAIT_E0)
				{
					if(rx_temp_buff[i] == 0xE0)
						cmd_line_buf.bkreg_state = BKREG_WAIT_FC;
					else if(rx_temp_buff[i] != 0x01)
						cmd_line_buf.bkreg_state = BKREG_WAIT_01;
				}
				else if(cmd_line_buf.bkreg_state == BKREG_WAIT_FC)
				{
					if(rx_temp_buff[i] == 0xFC)
					{
						cmd_line_buf.cur_cmd_type = CMD_TYPE_BKREG;

						cmd_line_buf.cmd_buff[0] = 0x01;
						cmd_line_buf.cmd_buff[1] = 0xE0;
						cmd_line_buf.cmd_buff[2] = 0xFC;

						cmd_line_buf.cmd_data_len = 3;

						echo_len = 0;   // cann't echo anything.

						continue;
					}
					else if(rx_temp_buff[i] != 0x01)
						cmd_line_buf.bkreg_state = BKREG_WAIT_01;
					else
						cmd_line_buf.bkreg_state = BKREG_WAIT_E0;
				}

			}

			if(cmd_line_buf.cur_cmd_type == CMD_TYPE_TEXT)
			{
				echo_len++;
				if(rx_temp_buff[i] == '\b')
				{
					if(cmd_line_buf.cmd_data_len > 0)
					{
						cmd_line_buf.cmd_data_len--;

						if(cmd_line_buf.cmd_data_len == 0)
							need_backspace = bTRUE;
					}
				}
				else if((rx_temp_buff[i] == '\n') || (rx_temp_buff[i] == '\r'))
				{
					if(cmd_line_buf.cmd_data_len < sizeof(cmd_line_buf.cmd_buff))
					{
						cmd_line_buf.cmd_buff[cmd_line_buf.cmd_data_len] = 0;
					}
					else
					{
						cmd_line_buf.cmd_buff[cmd_line_buf.cmd_data_len - 1] = 0;  // in case cmd_data_len overflow.
					}

					cmd_rx_done = bTRUE;
					break;
				}
				else if((rx_temp_buff[i] >= 0x20))
				{
					if(cmd_line_buf.cmd_data_len < sizeof(cmd_line_buf.cmd_buff))
					{
						cmd_line_buf.cmd_buff[cmd_line_buf.cmd_data_len] = rx_temp_buff[i];
						cmd_line_buf.cmd_data_len++;
					}
				}

			}

			/* patch for BK_REG tool. */
			if(cmd_line_buf.cur_cmd_type == CMD_TYPE_BKREG)
			{
				echo_len = 0;   // cann't echo anything.

				/* p[0] = 0x1, p[1]=0xe0, p[2]=0xfc, p[3]=len. */
				if(cmd_line_buf.cmd_data_len == 3)
				{
					cmd_line_buf.bkreg_left_byte = rx_temp_buff[i] + 1;  // +1, because will -1 in next process.

					if((cmd_line_buf.bkreg_left_byte + 3) >= sizeof(cmd_line_buf.cmd_buff))  // 3 bytes of header + 1 byte of len.
					{
						cmd_line_buf.cmd_data_len = 0;

						cmd_rx_done = bTRUE;
						break;
					}
				}

				if(cmd_line_buf.cmd_data_len < sizeof(cmd_line_buf.cmd_buff))
				{
					cmd_line_buf.cmd_buff[cmd_line_buf.cmd_data_len] = rx_temp_buff[i];
					cmd_line_buf.cmd_data_len++;
				}

				cmd_line_buf.bkreg_left_byte--;

				if(cmd_line_buf.bkreg_left_byte == 0)
				{
					cmd_rx_done = bTRUE;
					break;
				}
			}
		}

		if( cmd_rx_done )
		{
			/* patch for BK_REG tool. */
			if(cmd_line_buf.cur_cmd_type == CMD_TYPE_BKREG)
			{
				break;  // cann't echo anything.
			}

			if(cmd_line_buf.echo_enable)
			{
				echo_out(&rx_temp_buff[0], echo_len);
				echo_out((u8 *)"\r\n", 2);
			}

			break;
		}
		else
		{
			/* patch for BK_REG tool. */
			if( (cmd_line_buf.cur_cmd_type == CMD_TYPE_BKREG) ||
				((cmd_line_buf.cur_cmd_type == CMD_TYPE_INVALID) && (cmd_line_buf.bkreg_state != BKREG_WAIT_01)) )
			{
				 // cann't echo anything.
			}
			else if(cmd_line_buf.echo_enable)
			{
				if(echo_len > 0)
				{
					if( (rx_temp_buff[echo_len - 1] == '\b') ||
						(rx_temp_buff[echo_len - 1] == 0x7f) ) /* DEL */
					{
						echo_len--;
						if((cmd_line_buf.cmd_data_len > 0) || need_backspace)
							echo_out((u8 *)"\b \b", 3);
					}

					u8    cr_lf = 0;

					if(echo_len == 1)
					{
						if( (rx_temp_buff[echo_len - 1] == '\r') ||
							(rx_temp_buff[echo_len - 1] == '\n') )
						{
							cr_lf = 1;
						}
					}
					else if(echo_len == 2)
					{
						if( (memcmp(rx_temp_buff, "\r\n", 2) == 0) ||
							(memcmp(rx_temp_buff, "\n\r", 2) == 0) )
						{
							cr_lf = 1;
						}
					}

					if(cr_lf != 0)
					{
						echo_out((u8 *)shell_prompt_str[prompt_str_idx], 3);
						echo_len = 0;
					}
				}
				echo_out(rx_temp_buff, echo_len);
			}
		}

		if(read_cnt < buf_len) /* all data are read out. */
			break;
	}

	if(read_cnt < buf_len) /* all data are read out. */
	{
	}
	else  /* cmd pends in buffer, handle it in new loop cycle. */
	{
		set_shell_event(SHELL_EVENT_RX_IND);
	}

	/* can re-use *buf_len*. */
	if( cmd_rx_done )
	{
		if(cmd_line_buf.cur_cmd_type == CMD_TYPE_TEXT)
		{
			if(cmd_line_buf.cmd_ovf_hint == 0)
			{
				u16		rx_ovf = 0;
				cmd_dev->dev_drv->io_ctrl(cmd_dev, SHELL_IO_CTRL_GET_RX_STATUS, &rx_ovf);

				if(rx_ovf != 0)
				{
					cmd_hint_out();
					cmd_line_buf.cmd_ovf_hint = 1;
				}
			}

			rtos_get_semaphore(&cmd_line_buf.rsp_buf_semaphore, SHELL_WAIT_OUT_TIME);

			cmd_line_buf.rsp_buff[0] = 0;
			/* handle command. */
			if( cmd_line_buf.cmd_data_len > 0 )
			#if defined(CONFIG_AT) && defined(CONFIG_SYS_CPU0)
// Modified by TUYA Start
#if CONFIG_TUYA_LOGIC_MODIFY
            #if (CONFIG_TUYA_UART_PRINT_PORT == AT_UART_PORT_CFG)
				atsvr_msg_get_input((char *)cmd_line_buf.cmd_buff, cmd_line_buf.cmd_data_len, (char *)cmd_line_buf.rsp_buff, SHELL_RSP_BUF_LEN - 4);
			#else
				handle_shell_input( (char *)cmd_line_buf.cmd_buff, cmd_line_buf.cmd_data_len, (char *)cmd_line_buf.rsp_buff, SHELL_RSP_BUF_LEN - 4 );
			#endif
#else
            #if (CONFIG_UART_PRINT_PORT == AT_UART_PORT_CFG)
				atsvr_msg_get_input((char *)cmd_line_buf.cmd_buff, cmd_line_buf.cmd_data_len, (char *)cmd_line_buf.rsp_buff, SHELL_RSP_BUF_LEN - 4);
			#else
				handle_shell_input( (char *)cmd_line_buf.cmd_buff, cmd_line_buf.cmd_data_len, (char *)cmd_line_buf.rsp_buff, SHELL_RSP_BUF_LEN - 4 );
			#endif
#endif
// Modified by TUYA End
			#else
				handle_shell_input( (char *)cmd_line_buf.cmd_buff, cmd_line_buf.cmd_data_len, (char *)cmd_line_buf.rsp_buff, SHELL_RSP_BUF_LEN - 4 );
			#endif
			cmd_line_buf.rsp_buff[SHELL_RSP_BUF_LEN - 4] = 0;

			buf_len = strlen((char *)cmd_line_buf.rsp_buff);
			if(buf_len > (SHELL_RSP_BUF_LEN - 4))
				buf_len = (SHELL_RSP_BUF_LEN - 4);
			buf_len += sprintf((char *)&cmd_line_buf.rsp_buff[buf_len], shell_prompt_str[prompt_str_idx]);

			cmd_rsp_out(cmd_line_buf.rsp_buff, buf_len);
		}

		/* patch for BK_REG tool. */
		if(cmd_line_buf.cur_cmd_type == CMD_TYPE_BKREG)
		{
			if(cmd_line_buf.cmd_data_len > 3)
			{
#if CONFIG_BKREG
				extern int bkreg_run_command(const char *cmd, int flag);

				bkreg_run_command((const char *)&cmd_line_buf.cmd_buff[0], (int)cmd_line_buf.cmd_data_len);
#endif // CONFIG_BKREG
			}
		}

		cmd_line_buf.cur_cmd_type = CMD_TYPE_INVALID;  /* reset cmd line to interpret new cmd. */
		cmd_line_buf.cmd_data_len = 0;
		cmd_line_buf.bkreg_state = BKREG_WAIT_01;	/* reset state machine. */
	}

	return;
}

extern gpio_id_t bk_uart_get_rx_gpio(uart_id_t id);

static void shell_rx_wakeup(int gpio_id);

static void shell_power_save_enter(void)
{
	u32		flush_log = cmd_line_buf.log_flush;

	log_dev->dev_drv->io_ctrl(log_dev, SHELL_IO_CTRL_TX_SUSPEND, (void *)flush_log);
	cmd_dev->dev_drv->io_ctrl(cmd_dev, SHELL_IO_CTRL_RX_SUSPEND, NULL);

	if(cmd_dev->dev_type == SHELL_DEV_UART)
	{
		u8   uart_port = UART_ID_MAX;

		cmd_dev->dev_drv->io_ctrl(cmd_dev, SHELL_IO_CTRL_GET_UART_PORT, &uart_port);

		bk_uart_pm_backup(uart_port);

		u32  gpio_id = bk_uart_get_rx_gpio(uart_port);

		bk_gpio_register_isr(gpio_id, (gpio_isr_t)shell_rx_wakeup);
	}
}

static void shell_power_save_exit(void)
{
	log_dev->dev_drv->io_ctrl(log_dev, SHELL_IO_CTRL_TX_RESUME, NULL);
	cmd_dev->dev_drv->io_ctrl(cmd_dev, SHELL_IO_CTRL_RX_RESUME, NULL);
}

static void wakeup_process(void)
{
	bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_LOG, 0, 0);
	shell_pm_wake_flag = 1;
	shell_pm_wake_time = SHELL_TASK_WAKE_CYCLE;
}

static void shell_rx_wakeup(int gpio_id)
{
	wakeup_process();
	set_shell_event(SHELL_EVENT_WAKEUP);

	shell_log_raw_data((const u8*)"wakeup\r\n", sizeof("wakeup\r\n") - 1);

	if(cmd_dev->dev_type == SHELL_DEV_UART)
	{
		bk_gpio_register_isr(gpio_id, NULL);
	}
}

static void shell_task_init(void)
{
	u8    *shell_log_buff1 = NULL;
	u8    *shell_log_buff2 = NULL;
	u8    *shell_log_buff3 = NULL;
	u16   *buff1_free_list = NULL;
	u16   *buff2_free_list = NULL;
	u16   *buff3_free_list = NULL;

	shell_log_buff1 = psram_malloc(SHELL_LOG_BUF1_NUM * SHELL_LOG_BUF1_LEN);
	shell_log_buff2 = psram_malloc(SHELL_LOG_BUF2_NUM * SHELL_LOG_BUF2_LEN);
	shell_log_buff3 = psram_malloc(SHELL_LOG_BUF3_NUM * SHELL_LOG_BUF3_LEN);
	buff1_free_list = psram_malloc(SHELL_LOG_BUF1_NUM*2);
	buff2_free_list = psram_malloc(SHELL_LOG_BUF2_NUM*2);
	buff3_free_list = psram_malloc(SHELL_LOG_BUF3_NUM*2);

	if (shell_log_buff1 == NULL || shell_log_buff2 == NULL || shell_log_buff3 == NULL ||
		buff1_free_list == NULL || buff2_free_list == NULL || buff3_free_list == NULL)
	{
		BK_ASSERT(false);
	}

	free_queue[0].log_buf = shell_log_buff3;
	free_queue[0].blk_list = buff3_free_list;
	free_queue[0].blk_num = SHELL_LOG_BUF3_NUM;
	free_queue[0].blk_len = SHELL_LOG_BUF3_LEN;
	free_queue[0].list_out_idx = 0;
	free_queue[0].list_in_idx = 0;
	free_queue[0].free_blk_num = SHELL_LOG_BUF3_NUM;
	free_queue[0].ovf_cnt = 0;

	free_queue[1].log_buf = shell_log_buff2;
	free_queue[1].blk_list = buff2_free_list;
	free_queue[1].blk_num = SHELL_LOG_BUF2_NUM;
	free_queue[1].blk_len = SHELL_LOG_BUF2_LEN;
	free_queue[1].list_out_idx = 0;
	free_queue[1].list_in_idx = 0;
	free_queue[1].free_blk_num = SHELL_LOG_BUF2_NUM;
	free_queue[1].ovf_cnt = 0;

	free_queue[2].log_buf = shell_log_buff1;
	free_queue[2].blk_list = buff1_free_list;
	free_queue[2].blk_num = SHELL_LOG_BUF1_NUM;
	free_queue[2].blk_len = SHELL_LOG_BUF1_LEN;
	free_queue[2].list_out_idx = 0;
	free_queue[2].list_in_idx = 0;
	free_queue[2].free_blk_num = SHELL_LOG_BUF1_NUM;
	free_queue[2].ovf_cnt = 0;
	u16		i;

	for(i = 0; i < SHELL_LOG_BUF1_NUM; i++)
	{
		buff1_free_list[i] = i;
	}
	for(i = 0; i < SHELL_LOG_BUF2_NUM; i++)
	{
		buff2_free_list[i] = i;
	}
	for(i = 0; i < SHELL_LOG_BUF3_NUM; i++)
	{
		buff3_free_list[i] = i;
	}

	memset(&log_busy_queue, 0, sizeof(log_busy_queue));
	memset(&pending_queue, 0, sizeof(pending_queue));

	log_busy_queue.free_cnt = SHELL_LOG_BUSY_NUM;

	cmd_line_buf.cur_cmd_type = CMD_TYPE_INVALID;
	cmd_line_buf.cmd_data_len = 0;
	cmd_line_buf.bkreg_state = BKREG_WAIT_01;
	cmd_line_buf.log_level = BK_LOG_LEVEL;
	cmd_line_buf.echo_enable = bTRUE;
	cmd_line_buf.log_flush = 1;
	cmd_line_buf.cmd_ovf_hint = 0;

	rtos_init_semaphore_ex(&cmd_line_buf.rsp_buf_semaphore, 1, 1);  // one buffer for cmd_rsp.
	rtos_init_semaphore_ex(&cmd_line_buf.ind_buf_semaphore, 1, 1);  // one buffer for cmd_ind.

	create_shell_event();

	if(log_dev != cmd_dev)
	{
		cmd_dev->dev_drv->init(cmd_dev);
		cmd_dev->dev_drv->open(cmd_dev, shell_cmd_tx_complete, shell_rx_indicate); // rx cmd, tx rsp.

		log_dev->dev_drv->init(log_dev);
		log_dev->dev_drv->open(log_dev, shell_log_tx_complete, NULL);  // tx log.
	}
	else
	{
		cmd_dev->dev_drv->init(cmd_dev);
		cmd_dev->dev_drv->open(cmd_dev, shell_tx_complete, shell_rx_indicate); // rx cmd, tx (rsp & log).
	}

	#if defined(FWD_CMD_TO_MBOX) || defined(RECV_CMD_LOG_FROM_MBOX)
	ipc_dev->dev_drv->init(ipc_dev);
	ipc_dev->dev_drv->open(ipc_dev, (shell_ipc_rx_t)shell_ipc_rx_indication);   /* register rx-callback to copy log data to buffer. */
	#endif

	shell_init_ok = bTRUE;

	{
		pm_cb_conf_t enter_config;
		enter_config.cb = (pm_cb)shell_power_save_enter;
		enter_config.args = NULL;

		pm_cb_conf_t exit_config;
		exit_config.cb = (pm_cb)shell_power_save_exit;
		exit_config.args = NULL;

		#if 0
		bk_pm_sleep_register_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_UART1, &enter_config, &exit_config);

		u8   uart_port = UART_ID_MAX;

		cmd_dev->dev_drv->io_ctrl(cmd_dev, SHELL_IO_CTRL_GET_UART_PORT, &uart_port);

		shell_rx_wakeup(bk_uart_get_rx_gpio(uart_port));
		#endif

		u8 uart_port = UART_ID_MAX;
		cmd_dev->dev_drv->io_ctrl(cmd_dev, SHELL_IO_CTRL_GET_UART_PORT, &uart_port);

		u8 pm_uart_port = uart_id_to_pm_uart_id(uart_port);
		bk_pm_sleep_register_cb(PM_MODE_LOW_VOLTAGE, pm_uart_port, &enter_config, &exit_config);

		shell_rx_wakeup(bk_uart_get_rx_gpio(uart_port));
	}

	if(ate_is_enabled())
		prompt_str_idx = 1;
	else
		prompt_str_idx = 0;

}

void shell_task( void *para )
{
	u32    Events;
	u32    timeout = SHELL_TASK_WAIT_TIME;

	shell_task_init();

	echo_out((u8 *)shell_prompt_str[prompt_str_idx], 3);

	while(bTRUE)
	{
		Events = wait_any_event(timeout);  // WAIT_EVENT;

		if(Events & SHELL_EVENT_TX_REQ)
		{
			echo_out((u8 *)"Unsolicited", sizeof("Unsolicited") - 1);
			echo_out((u8 *)shell_prompt_str[prompt_str_idx], 3);
		}

		if(Events & SHELL_EVENT_RX_IND)
		{
			wakeup_process();
			rx_ind_process();
		}

		if(Events & SHELL_EVENT_WAKEUP)
		{
			// TODO
		}

		if(Events == 0)
		{
			if(shell_pm_wake_time > 0)
				shell_pm_wake_time--;

			if(shell_pm_wake_time == 0)
			{
				if(shell_pm_wake_flag != 0)
				{
					shell_pm_wake_flag = 0;
					bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_LOG, 1, 0);
					//shell_log_raw_data((const u8*)"sleep\r\n", sizeof("sleep\r\n") - 1);
				}
			}
		}

		if(shell_pm_wake_flag)
		{
			timeout = SHELL_TASK_WAIT_TIME;
		}
		else
		{
			timeout = BEKEN_WAIT_FOREVER;
		}
	}
}

int shell_get_cpu_id(void)
{
#ifdef CONFIG_FREERTOS_SMP
	return rtos_get_core_id();
#elif (CONFIG_CPU_CNT > 1)

	if(log_dev->dev_type == SHELL_DEV_MAILBOX)
		return SELF_CPU;

#endif

	return -1;
}

static int shell_cpu_check_valid(void)
{
#if (CONFIG_CPU_CNT > 1) && (LOG_DEV != DEV_MAILBOX)

	if(shell_log_owner_cpu == 0)
		return 1;

	u8    req_cpu = 0;
#ifdef CONFIG_FREERTOS_SMP
	req_cpu = 1 << rtos_get_core_id();
#else
	req_cpu = shell_log_req_cpu;
#endif

	if(shell_log_owner_cpu == req_cpu)
		return 1;

	return 0;

#else

	return 1;

#endif
}

int shell_level_check_valid(int level)
{
	if( !shell_cpu_check_valid() )
		return 0;

	if(level > cmd_line_buf.log_level)
		return 0;

	return 1;
}

int shell_log_out_sync(int level, char *prefix, const char *format, va_list ap)
{
	u32         int_mask;
	char       *pbuf;
	u16         data_len = 0, buf_len;

	if( !shell_level_check_valid(level) )
		return 0;

	pbuf = (char *)&cmd_line_buf.assert_buff[0];
	buf_len = sizeof(cmd_line_buf.assert_buff);

	int_mask = shell_task_enter_critical();

	if(prefix != NULL)
	{
		strncpy(pbuf, prefix, buf_len);
		pbuf[buf_len - 1] = 0;
		data_len = strlen(pbuf);
	}
	else
	{
		data_len = 0;
	}

	data_len += vsnprintf( &pbuf[data_len], buf_len - data_len, format, ap );

	if(data_len >= buf_len)
		data_len = buf_len - 1;

	if ( (data_len != 0) && (pbuf[data_len - 1] == '\n') )
	{
		if ((data_len == 1) || (pbuf[data_len - 2] != '\r'))
		{
			pbuf[data_len] = '\n';      /* '\n\0' replaced with '\r\n', may not end with '\0'. */
			pbuf[data_len - 1] = '\r';
			data_len++;
		}
	}

	log_dev->dev_drv->write_sync(log_dev, (u8 *)pbuf, data_len);

	shell_task_exit_critical(int_mask);

	return 1;
}

int shell_log_raw_data(const u8 *data, u16 data_len)
{
	u8   *packet_buf;
	u16   free_blk_tag;

	if (!shell_init_ok)
	{
		return 0; // bFALSE;
	}

	if( !shell_cpu_check_valid() )
		return 0;

	if (NULL == data || 0 == data_len)
	{
		return 0; // bFALSE;
	}

	packet_buf = alloc_log_blk(data_len, &free_blk_tag);

	if (NULL == packet_buf)
	{
		log_hint_out();
		return 0; // bFALSE;
	}

	memcpy(packet_buf, data, data_len);

	u32 int_mask = shell_task_enter_critical();

	// push to pending queue.
	push_pending_queue(free_blk_tag, data_len);

	// notify shell task to process the log tx.
	tx_req_process();

	shell_task_exit_critical(int_mask);

	return 1; // bTRUE;
}

void shell_log_out_port(int level, char *prefix, const char *format, va_list ap)
{
	u8   * packet_buf;
	u16    free_blk_tag;
	u16    log_len = 0, buf_len;

	if( !shell_init_ok )
	{
		cmd_line_buf.log_level = BK_LOG_LEVEL;	// if not intialized, set log_level temporarily here. !!!patch!!!

		shell_log_out_sync(level, prefix, format, ap);

		return ;
	}

	if( !shell_level_check_valid(level) )
	{
		return ;
	}

	buf_len = vsnprintf( NULL, 0, format, ap ) + 1;  /* for '\0' */

	if(prefix != NULL)
		buf_len += strlen(prefix);

	if(buf_len == 0)
		return;

	packet_buf = alloc_log_blk(buf_len, &free_blk_tag);

	if(packet_buf == NULL)
	{
		log_hint_out();
		return ;
	}

	log_len = 0;

	if(prefix != NULL)
	{
		strcpy((char *)&packet_buf[0], prefix);
		log_len = strlen((char *)packet_buf);
	}

	log_len += vsnprintf( (char *)&packet_buf[log_len], buf_len - log_len, format, ap );

	if(log_len >= buf_len)
		log_len = buf_len - 1;

	if ( (log_len != 0) && (packet_buf[log_len - 1] == '\n') )
	{
		if ((log_len == 1) || (packet_buf[log_len - 2] != '\r'))
		{
			packet_buf[log_len] = '\n';     /* '\n\0' replaced with '\r\n', may not end with '\0'. */
			packet_buf[log_len - 1] = '\r';
			log_len++;
		}
	}

	u32  int_mask = shell_task_enter_critical();

	// push to pending queue.
	push_pending_queue(free_blk_tag, log_len);

	//set_shell_event(SHELL_EVENT_TX_REQ);  // notify shell task to process the log tx.
	tx_req_process();

	shell_task_exit_critical(int_mask);

	return ;
}

int shell_assert_out(bool bContinue, char * format, ...)
{
	u32         int_mask;
	char       *pbuf;
	u16         data_len, buf_len;
	va_list     arg_list;

	if( !shell_cpu_check_valid() )
		return 0;

	pbuf = (char *)&cmd_line_buf.assert_buff[0];
	buf_len = sizeof(cmd_line_buf.assert_buff);

	/* just disabled interrupts even when dump out in SMP. */
	/* because other core's dump has been blocked by shell_cpu_check_valid(). */
	/* can't try to get SPINLOCK, because may be it is called from HardFault handler, */
	/* and meanwhile the spinlock is held by other cores. */

	// int_mask = shell_task_enter_critical();
	int_mask = rtos_disable_int();

	va_start( arg_list, format );

	data_len = vsnprintf( pbuf, buf_len, format, arg_list );

	va_end( arg_list );

	if(data_len >= buf_len)
		data_len = buf_len - 1;

	log_dev->dev_drv->write_sync(log_dev, (u8 *)pbuf, data_len);

	if( bContinue )
	{
		// shell_task_exit_critical(int_mask);
		rtos_enable_int(int_mask);
	}
	else
	{
		while(bTRUE)
		{
		}
	}

	return 1;//bTRUE;;

}

int shell_assert_raw(bool bContinue, char * data_buff, u16 data_len)
{
	u32         int_mask;

	if( !shell_cpu_check_valid() )
		return 0;

	/* just disabled interrupts even when dump out in SMP. */
	/* because other core's dump has been blocked by shell_cpu_check_valid(). */
	/* can't try to get SPINLOCK, because may be it is called from HardFault handler, */
	/* and meanwhile the spinlock is held by other cores. */

	// int_mask = shell_task_enter_critical();
	int_mask = rtos_disable_int();

	log_dev->dev_drv->write_sync(log_dev, (u8 *)data_buff, data_len);

	if( bContinue )
	{
		// shell_task_exit_critical(int_mask);
		rtos_enable_int(int_mask);
	}
	else
	{
		while(1)
		{
		}
	}

	return 1;//bTRUE;;

}

#if defined(FWD_CMD_TO_MBOX) || defined(RECV_CMD_LOG_FROM_MBOX)
static int cmd_rsp_fwd(u8 * rsp_msg, u16 msg_len)
{
	u16    rsp_blk_tag = MAKE_BLOCK_TAG(MBOX_RSP_BLK_ID, SHELL_FWD_QUEUE_ID);

	cmd_info_out(rsp_msg, msg_len, rsp_blk_tag);

	return 1;
}

static int cmd_ind_fwd(u8 * ind_msg, u16 msg_len)
{
	u16    ind_blk_tag = MAKE_BLOCK_TAG(MBOX_IND_BLK_ID, SHELL_FWD_QUEUE_ID);

	cmd_info_out(ind_msg, msg_len, ind_blk_tag);

	return 1;
}

static int result_fwd(int blk_id)
{
	log_cmd_t   * log_cmd;

	if(blk_id == MBOX_RSP_BLK_ID)
	{
		log_cmd = &ipc_fwd_data.rsp_buf;
	}
	else if(blk_id == MBOX_IND_BLK_ID)
	{
		log_cmd = &ipc_fwd_data.ind_buf;
	}
	else
	{
		return 0;
	}

	log_cmd->hdr.data = 0;
	log_cmd->hdr.cmd = MB_CMD_LOG_OUT_OK;

	return ipc_dev->dev_drv->write_cmd(ipc_dev, (mb_chnl_cmd_t *)log_cmd);
}

static u32 shell_ipc_rx_indication(u16 cmd, log_cmd_t *log_cmd, u16 cpu_id)
{
	u32   result = ACK_STATE_FAIL;
	u8  * data = log_cmd->buf;
	u16   data_len = log_cmd->len;

	if(shell_log_owner_cpu != 0)
	{
		if(shell_log_owner_cpu == (0x01 << cpu_id))
			shell_log_req_cpu = shell_log_owner_cpu;
		else
			return result;
	}

	if(cmd == MB_CMD_LOG_OUT)
	{
		u8      queue_id = GET_QUEUE_ID(log_cmd->tag);

		if(queue_id == SHELL_RSP_QUEUE_ID)
		{
			memcpy(&ipc_fwd_data.rsp_buf, log_cmd, sizeof(ipc_fwd_data.rsp_buf));
			cmd_rsp_fwd(data, data_len);

			result = ACK_STATE_PENDING;
		}
		else if(queue_id == SHELL_IND_QUEUE_ID)
		{
			memcpy(&ipc_fwd_data.ind_buf, log_cmd, sizeof(ipc_fwd_data.ind_buf));
			cmd_ind_fwd(data, data_len);

			result = ACK_STATE_PENDING;
		}
		else  // no cmd_hint from slave, so must be log from slave.
		{
			result = shell_log_raw_data(data, data_len);

			if(result == 0)
				result = ACK_STATE_FAIL;
			else
				result = ACK_STATE_COMPLETE;
		}
	}
	else if(cmd == MB_CMD_ASSERT_OUT)
	{
		#if (CONFIG_TASK_WDT)
		bk_task_wdt_feed();
		#endif
		#if (CONFIG_INT_WDT)
		bk_int_wdt_feed();
		#endif

		shell_assert_raw(true, (char *)data, data_len);

		result = ACK_STATE_COMPLETE;
	}

	shell_log_req_cpu = 0;

	/* no cmd handler. */
	return result;
}

int shell_cmd_forward(char *cmd, u16 cmd_len)
{
	mb_chnl_cmd_t	mb_cmd_buf;
	user_cmd_t * user_cmd = (user_cmd_t *)&mb_cmd_buf;

	user_cmd->hdr.data = 0;
	user_cmd->hdr.cmd = MB_CMD_USER_INPUT;
	user_cmd->buf = (u8 *)cmd;
	user_cmd->len = cmd_len;

	u32  int_mask = shell_task_enter_critical();

	int ret_code = ipc_dev->dev_drv->write_cmd(ipc_dev, &mb_cmd_buf);

	shell_task_exit_critical(int_mask);

	return ret_code;
}
#endif

void shell_echo_set(int en_flag)
{
	if(en_flag != 0)
		cmd_line_buf.echo_enable = bTRUE;
	else
		cmd_line_buf.echo_enable = bFALSE;
}

int shell_echo_get(void)
{
	if(cmd_line_buf.echo_enable)
		return 1;

	return 0;
}

void shell_set_log_level(int level)
{
	cmd_line_buf.log_level = level;
}

int shell_get_log_level(void)
{
	return cmd_line_buf.log_level;
}

void shell_set_log_flush(int flush_flag)
{
	cmd_line_buf.log_flush = flush_flag;
}

int shell_get_log_flush(void)
{
	return cmd_line_buf.log_flush;
}

int shell_get_log_statist(u32 * info_list, u32 num)
{
	int   cnt = 0;
	if(num > 0)
	{
		info_list[0] = shell_log_overflow;
		cnt++;
	}
	if(num > 1)
	{
		info_list[1] = shell_log_count;
		cnt++;
	}
	if(num > 2)
	{
		info_list[2] = free_queue[0].ovf_cnt;
		cnt++;
	}
	if(num > 3)
	{
		info_list[3] = free_queue[1].ovf_cnt;
		cnt++;
	}
	if(num > 4)
	{
		info_list[4] = free_queue[2].ovf_cnt;
		cnt++;
	}

	return cnt;
}

void shell_log_flush(void)
{
	u32         int_mask;

	/* just disabled interrupts even when dump out in SMP. */
	/* because it is called by power_saving process or by dumping process. */
	/* in any case, only one core is running. */
	/* can't try to get SPINLOCK, because may be it is called from HardFault handler, */
	/* and meanwhile the spinlock is held by other cores. */

	// int_mask = shell_task_enter_critical();
	int_mask = rtos_disable_int();

	log_dev->dev_drv->io_ctrl(log_dev, SHELL_IO_CTRL_FLUSH, NULL);

	// shell_task_exit_critical(int_mask);
	rtos_enable_int(int_mask);
}

void shell_set_uart_port(uint8_t uart_port)
{
#if (LOG_DEV == DEV_UART)
	if(log_dev->dev_type != SHELL_DEV_UART)
	{
		return;
	}

	if ((bk_get_printf_port() != uart_port) && (uart_port < UART_ID_MAX))
	{
		u32  int_mask = shell_task_enter_critical();

		shell_log_flush();
		log_dev->dev_drv->close(log_dev);
		log_dev->dev_drv->io_ctrl(log_dev, SHELL_IO_CTRL_SET_UART_PORT, &uart_port);

		bk_set_printf_port(uart_port);

		log_dev->dev_drv->init(log_dev);
		if(log_dev != cmd_dev)
		{
			log_dev->dev_drv->open(log_dev, shell_log_tx_complete, NULL);  // tx log.
		}
		else
		{
			log_dev->dev_drv->open(log_dev, shell_tx_complete, shell_rx_indicate);
		}

		shell_task_exit_critical(int_mask);
	}
#endif
}

void shell_cmd_ind_out(const char *format, ...)
{
	u16   data_len, buf_len = SHELL_IND_BUF_LEN;
	va_list  arg_list;

	rtos_get_semaphore(&cmd_line_buf.ind_buf_semaphore, SHELL_WAIT_OUT_TIME);

	va_start(arg_list, format);
	data_len = vsnprintf( (char *)&cmd_line_buf.cmd_ind_buff[0], buf_len - 1, format, arg_list );
	va_end(arg_list);

	if(data_len >= buf_len)
		data_len = buf_len - 1;

	cmd_ind_out(cmd_line_buf.cmd_ind_buff, data_len);
}

void shell_set_log_cpu(u8 req_cpu)
{
#if (CONFIG_CPU_CNT > 1) && (LOG_DEV != DEV_MAILBOX)
	if(req_cpu >= CONFIG_CPU_CNT)
	{
		shell_log_owner_cpu = 0;
		shell_log_req_cpu = 0;
	}
	else if(shell_log_owner_cpu == 0)
	{
		shell_log_owner_cpu = 1 << req_cpu;
		shell_log_req_cpu   = 1 << req_cpu;
	}
#endif
}

#else // !CONFIG_TUYA_LOG_OPTIMIZATION

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "cli.h"
#include <os/os.h>
#include <common/bk_compiler.h>
#include <components/shell_task.h>
#include "shell_drv.h"
#include <components/ate.h>
#include <modules/pm.h>
#include <driver/gpio.h>
#include "bk_wdt.h"
#if CONFIG_AT
#include "atsvr_unite.h"
#if CONFIG_AT_DATA_MODE
#include "at_sal_ex.h"
#endif
#endif

#define DEV_UART        1
#define DEV_MAILBOX     2

#if CONFIG_SYS_PRINT_DEV_UART
#define LOG_DEV    DEV_UART
#define CMD_DEV    DEV_UART
#elif CONFIG_SYS_PRINT_DEV_MAILBOX
#define LOG_DEV    DEV_MAILBOX
#define CMD_DEV    DEV_MAILBOX
#endif

// #if (CMD_DEV != DEV_MAILBOX)
#if (CONFIG_SYS_CPU0)
#if CONFIG_MAILBOX
#ifndef CONFIG_FREERTOS_SMP
#define FWD_CMD_TO_MBOX
#define RECV_CMD_LOG_FROM_MBOX
#endif
#endif
#endif

#if defined(FWD_CMD_TO_MBOX)
#define MBOX_RSP_BLK_ID			0
#define MBOX_IND_BLK_ID			1
#define MBOX_COMMON_BLK_ID		2

#define MBOX_FWD_PEND_NUM		2    /* (1 Rsp + 1 Ind) */
#else
#define MBOX_FWD_PEND_NUM		0
#endif

#define SHELL_LOG_BLOCK_TIME  (10)
#define SHELL_LOG_FWD_WAIT_TIME  (1)

#define SHELL_WAIT_OUT_TIME		(2000) 		// 2s.

#define SHELL_TASK_WAIT_TIME	(100)		// 100ms
#define SHELL_TASK_WAKE_CYCLE	(20)		// 2s

#define SHELL_EVENT_TX_REQ  	0x01
#define SHELL_EVENT_RX_IND  	0x02
#define SHELL_EVENT_WAKEUP      0x04
#define SHELL_EVENT_DYM_FREE    0x08
#define SHELL_EVENT_MB_LOG      0x10
#define SHELL_EVENT_MB_FWD      0x20

#define SHELL_LOG_BUF1_LEN      136
#define SHELL_LOG_BUF2_LEN      80
#define SHELL_LOG_BUF3_LEN      40

#if (LOG_DEV == DEV_MAILBOX)
#define SHELL_LOG_BUF1_NUM      2
#define SHELL_LOG_BUF2_NUM      4
#define SHELL_LOG_BUF3_NUM      8
#define SHELL_DYM_LOG_NUM_MAX   50
#endif   //  (LOG_DEV == DEV_MAILBOX)

#if (LOG_DEV == DEV_UART)
#if CONFIG_RELEASE_VERSION
#define SHELL_LOG_BUF1_NUM      4
#define SHELL_LOG_BUF2_NUM      16
#define SHELL_LOG_BUF3_NUM      32
#define SHELL_DYM_LOG_NUM_MAX   50
#else
#if !CONFIG_UART_RING_BUFF
#define SHELL_LOG_BUF1_NUM      8
#define SHELL_LOG_BUF2_NUM      40
#define SHELL_LOG_BUF3_NUM      60
#define SHELL_DYM_LOG_NUM_MAX   100
#else
#define SHELL_LOG_BUF1_NUM      8
#define SHELL_LOG_BUF2_NUM      30
#define SHELL_LOG_BUF3_NUM      50
#define SHELL_DYM_LOG_NUM_MAX   100
#endif
#endif
#endif   // (LOG_DEV == DEV_UART)

#define SHELL_LOG_BUF_NUM       (SHELL_LOG_BUF1_NUM + SHELL_LOG_BUF2_NUM + SHELL_LOG_BUF3_NUM)
#define SHELL_LOG_PEND_NUM      (SHELL_LOG_BUF_NUM * 2 + 4 + MBOX_FWD_PEND_NUM + SHELL_DYM_LOG_NUM_MAX)
/* the worst case may be (one log + one hint) in pending queue, so twice the log_num for pending queue.*/
/* 1: RSP, 1: reserved(queue empty), 1: cmd ovf, 1: ind). */
/* MBOX_FWD_PEND_NUM (1 Rsp + 1 Ind) every slave core. */
#define SHELL_LOG_BUSY_NUM      (8)
#define SHELL_ASSERT_BUF_LEN	140

#if (CMD_DEV == DEV_MAILBOX)
#define SHELL_RX_BUF_LEN		140
#endif
#if (CMD_DEV == DEV_UART)
#define SHELL_RX_BUF_LEN		4
#endif

#if CONFIG_AT_DATA_MODE
#define SHELL_CMD_BUF_LEN		4096
#else
#define SHELL_CMD_BUF_LEN		200
#endif
#define SHELL_RSP_BUF_LEN		140
#define SHELL_IND_BUF_LEN		132

#define SHELL_RSP_QUEUE_ID	    (7)
#define SHELL_FWD_QUEUE_ID      (8)
#define SHELL_ROM_QUEUE_ID		(9)
#define SHELL_IND_QUEUE_ID		(10)
#define SHELL_DYM_QUEUE_ID		(11)

#define TBL_SIZE(tbl)		(sizeof(tbl) / sizeof(tbl[0]))

#define LOG_BLOCK_MASK    LOG_STATIC_BLOCK_MODE
#define LOG_MALLOC_MASK   LOG_NONBLOCK_MODE

typedef struct
{
	beken_semaphore_t   event_semaphore;  // will release from ISR.
	u32       event_flag;
} os_ext_event_t;

enum
{
	CMD_TYPE_TEXT = 0,
	CMD_TYPE_HEX,
	CMD_TYPE_BKREG,   /* patch for BK_REG tool cmd. */
	CMD_TYPE_INVALID,
};

/* patch for BK_REG tool. */
enum
{
	BKREG_WAIT_01 = 0,
	BKREG_WAIT_E0,
	BKREG_WAIT_FC,
};

typedef struct
{
	u8     rsp_buff[SHELL_RSP_BUF_LEN];
	beken_semaphore_t   rsp_buf_semaphore;

	u8     rx_buff[SHELL_RX_BUF_LEN];

	u8     cur_cmd_type;
	u8     cmd_buff[SHELL_CMD_BUF_LEN];
	u16    cmd_data_len;
	u8     cmd_ovf_hint;

	/* patch for BK_REG tool. */
	/* added one state machine for BK_REG tool cmd. */
	u8     bkreg_state;
	u8     bkreg_left_byte;
	/* patch end. */

	u8     echo_enable;

	/* patch for AT cmd handling. */
	u8     cmd_ind_buff[SHELL_IND_BUF_LEN];
	beken_semaphore_t   ind_buf_semaphore;

	/* cmd FWD */
    #if defined(FWD_CMD_TO_MBOX)
    beken_semaphore_t   cmd_fwd_semaphore;
    #endif
} cmd_line_t;

#define GET_BLOCK_ID(blocktag)          ((blocktag) & 0x7FF)
#define GET_QUEUE_ID(blocktag)          (((blocktag) & 0xF800) >> 11)
#define MAKE_BLOCK_TAG(blk_id, q_id)    (((blk_id) & 0x7FF) | (((q_id) & 0x1F) << 11) )

typedef struct
{
	u16     blk_tag;        /* bit0~bit7: blk_id,    bit8~bit11: queue_id; */
	u16     packet_len;
} tx_packet_t;

typedef struct
{
	tx_packet_t     packet_list[SHELL_LOG_PEND_NUM];
	u16     list_out_idx;
	u16     list_in_idx;
} pending_queue_t;

typedef struct
{
	u16     blk_list[SHELL_LOG_BUSY_NUM];
	u16     list_out_idx;
	u16     list_in_idx;
	u16     free_cnt;
} busy_queue_t;

typedef struct
{
	u8   *  const  log_buf;
	u16   * const  blk_list;
	const u16      blk_num;
	const u16      blk_len;
	u16     list_out_idx;
	u16     list_in_idx;
	u16     free_blk_num;
	u32     ovf_cnt;
} free_queue_t;

typedef struct dynamic_log_node_t dynamic_log_node;
struct dynamic_log_node_t
{
	dynamic_log_node *next;
	u32 len;
	u8 ptr[0];
};

#define LOG_MALLOC os_malloc
#define LOG_FREE os_free

static dynamic_log_node s_dynamic_header = {NULL};
static dynamic_log_node *s_to_free_list = NULL;
static dynamic_log_node *s_curr_node = NULL;
static dynamic_log_node *s_dym_tail_node = &s_dynamic_header;

static int s_block_mode = LOG_COMMON_MODE;

static u16 s_dynamic_log_num = 0;   // dynamic log in send queue
static u16 s_dynamic_log_total_len = 0;  // total consumption of dynamic log memory
static u16 s_dynamic_log_num_in_mem = 0;  // number of dynamic log in memory, including no free log.
static u16 s_dynamic_log_mem_max = 0;  // maximum of consumption

#define DYM_NODE_SIZE (sizeof(dynamic_log_node))


#if (CONFIG_CACHE_ENABLE) && (CONFIG_LV_USE_DEMO_METER)
#define  SHELL_DECLARE_MEMORY_ATTR __attribute__((section(".sram_cache")))
#elif CONFIG_SOC_BK7258 && CONFIG_SYS_PRINT_DEV_UART
#define  SHELL_DECLARE_MEMORY_ATTR __attribute__((section(".dtcm_section")))
#else
#define  SHELL_DECLARE_MEMORY_ATTR
#endif

beken_semaphore_t   log_buf_semaphore = NULL;

static SHELL_DECLARE_MEMORY_ATTR u8    shell_log_buff1[SHELL_LOG_BUF1_NUM * SHELL_LOG_BUF1_LEN];
static SHELL_DECLARE_MEMORY_ATTR u8    shell_log_buff2[SHELL_LOG_BUF2_NUM * SHELL_LOG_BUF2_LEN];
static SHELL_DECLARE_MEMORY_ATTR u8    shell_log_buff3[SHELL_LOG_BUF3_NUM * SHELL_LOG_BUF3_LEN];
static SHELL_DECLARE_MEMORY_ATTR u16   buff1_free_list[SHELL_LOG_BUF1_NUM];
static SHELL_DECLARE_MEMORY_ATTR u16   buff2_free_list[SHELL_LOG_BUF2_NUM];
static SHELL_DECLARE_MEMORY_ATTR u16   buff3_free_list[SHELL_LOG_BUF3_NUM];


/*    queue sort ascending in blk_len.    */
static free_queue_t       free_queue[3] =
	{
		{.log_buf = shell_log_buff3, .blk_list = buff3_free_list, .blk_num = SHELL_LOG_BUF3_NUM, \
			.blk_len = SHELL_LOG_BUF3_LEN, .list_out_idx = 0, .list_in_idx = 0, \
			.free_blk_num = SHELL_LOG_BUF3_NUM, .ovf_cnt = 0},

		{.log_buf = shell_log_buff2, .blk_list = buff2_free_list, .blk_num = SHELL_LOG_BUF2_NUM, \
			.blk_len = SHELL_LOG_BUF2_LEN, .list_out_idx = 0, .list_in_idx = 0, \
			.free_blk_num = SHELL_LOG_BUF2_NUM, .ovf_cnt = 0},

		{.log_buf = shell_log_buff1, .blk_list = buff1_free_list, .blk_num = SHELL_LOG_BUF1_NUM, \
			.blk_len = SHELL_LOG_BUF1_LEN, .list_out_idx = 0, .list_in_idx = 0, \
			.free_blk_num = SHELL_LOG_BUF1_NUM, .ovf_cnt = 0},
	};

#if (CONFIG_CACHE_ENABLE) && (CONFIG_LV_USE_DEMO_METER)
static __attribute__((section(".sram_cache")))  busy_queue_t       log_busy_queue;
static __attribute__((section(".sram_cache")))  pending_queue_t    pending_queue;
#else
static busy_queue_t       log_busy_queue;
static pending_queue_t    pending_queue;
#endif

#if (CONFIG_CACHE_ENABLE) && (CONFIG_LV_USE_DEMO_METER)
static __attribute__((section(".sram_cache")))  cmd_line_t  cmd_line_buf;
#else
static cmd_line_t  cmd_line_buf;
#endif

#if (LOG_DEV == DEV_UART)
static shell_dev_t * log_dev = &shell_uart;
#endif
#if (LOG_DEV == DEV_MAILBOX)
static shell_dev_t * log_dev = &shell_dev_mb;
#endif

#if (CMD_DEV == DEV_UART)
static shell_dev_t * cmd_dev = &shell_uart;
#endif
#if (CMD_DEV == DEV_MAILBOX)
static shell_dev_t * cmd_dev = &shell_dev_mb;
#endif

#if (CONFIG_SYS_CPU0)
static const char	 shell_fault_str[] = "\r\n!!some LOGs discarded!!\r\n";
static const u16     shell_fault_str_len = sizeof(shell_fault_str) - 1;
#endif
#if (!CONFIG_SYS_CPU0)
static const char	 shell_fault_str[] = "\r\n!!CPUx:some LOGs discarded!!\r\n";
static const u16     shell_fault_str_len = sizeof(shell_fault_str) - 1;
#endif

#if defined(FWD_CMD_TO_MBOX) || defined(RECV_CMD_LOG_FROM_MBOX)

typedef struct
{
	log_cmd_t	rsp_buf;
	log_cmd_t   ind_buf;
	log_cmd_t   common_log_buf;
} fwd_slave_data_t;

static u8 s_fwd_status = 0;

static fwd_slave_data_t  ipc_fwd_data;

mb_chnl_cmd_t	s_mb_cmd_buf;

static shell_dev_ipc_t * ipc_dev = &shell_dev_ipc;

static int result_fwd(int blk_id);
static u32 shell_ipc_rx_indication(u16 cmd, log_cmd_t *data, u16 cpu_id);
static void shell_ipc_tx_complete(u16 cmd);
static void set_fwd_state(int blk_id);
#endif

static const char	 shell_cmd_ovf_str[] = "\r\n!!some CMDs lost!!\r\n";
static const u16     shell_cmd_ovf_str_len = sizeof(shell_cmd_ovf_str) - 1;
static const char  * shell_prompt_str[2] = {"\r\n$", "\r\n#"};
static u8            prompt_str_idx = 0;
static u8            cmd_rx_init_ok = 0;
static u8            log_handle_init_ok = 0;

static u8     fault_hint_print = 0;
static u32    shell_log_overflow = 0;
static u32    shell_log_count = 0;
// Modified by TUYA Start
static u8     shell_log_level = BK_LOG_LEVEL;
// Modified by TUYA End
static u8     log_flush_enabled = 1;
static u8     shell_assert_buff[SHELL_ASSERT_BUF_LEN];
static u8     log_tx_init_ok = 0;

static u32    shell_pm_wake_time = SHELL_TASK_WAKE_CYCLE;   // wait cycles before enter sleep.
static u8     shell_pm_wake_flag = 1;

#if (CONFIG_CPU_CNT > 1) && (LOG_DEV != DEV_MAILBOX)
/* patch for multi-cpu dump. */
static u8     shell_log_owner_cpu = 0;
static u8     shell_log_req_cpu = 0;
#endif

#ifdef CONFIG_FREERTOS_SMP
#include "spinlock.h"
static volatile spinlock_t shell_spin_lock = SPIN_LOCK_INIT;
#endif // CONFIG_FREERTOS_SMP


static dynamic_log_node *dynamic_list_pop_front(void);
static void free_list_push_front(dynamic_log_node *dym_node);
static void check_and_free_dynamic_node(void);
static u8 *alloc_dynamic_log_blk(u16 log_len, u16 *blk_tag);
static void dynamic_list_push_back(dynamic_log_node *dym_node);
static u8 * alloc_buffer(int block_mode, u16 *blk_tag, u16 buf_len);
static inline void dynamic_list_push_back_by_buffer(u8 *packet_buf);
static int shell_log_raw_data_internel(bool hint, const u8 *data, u16 data_len);
static void output_insert_log(u16 buf_len, char *prefix, const char *format, va_list ap);
static void output_insert_data(const u8 *data, u16 data_len);
static dynamic_log_node *dynamic_list_switch(void);
static void log_handle_task( void *para );

static inline uint32_t shell_task_enter_critical()
{
	uint32_t flags = rtos_disable_int();

#ifdef CONFIG_FREERTOS_SMP
	spin_lock(&shell_spin_lock);
#endif // CONFIG_FREERTOS_SMP

	return flags;
}

static inline void shell_task_exit_critical(uint32_t flags)
{
#ifdef CONFIG_FREERTOS_SMP
	spin_unlock(&shell_spin_lock);
#endif // CONFIG_FREERTOS_SMP

	rtos_enable_int(flags);
}

#if 1

static os_ext_event_t   shell_task_event;
static os_ext_event_t   shell_log_event;

static bool_t create_shell_event(void)
{
	shell_task_event.event_flag = 0;
	rtos_init_semaphore(&shell_task_event.event_semaphore, 1);

	return bTRUE;
}

/* this API may be called from ISR. */
static bool_t set_shell_event(os_ext_event_t *ext_event, u32 event_flag)
{
	BK_ASSERT(ext_event != NULL);
	u32  int_mask;

	int_mask = shell_task_enter_critical();

	ext_event->event_flag |= event_flag;

	shell_task_exit_critical(int_mask);

	rtos_set_semaphore(&ext_event->event_semaphore);

	return bTRUE;
}

static u32 wait_any_event(os_ext_event_t *ext_event, u32 timeout)
{
	BK_ASSERT(ext_event != NULL);
	u32  int_mask;
	u32  event_flag;

	int  result;

	while(bTRUE)
	{
		int_mask = shell_task_enter_critical();

		event_flag = ext_event->event_flag;
		ext_event->event_flag = 0;

		shell_task_exit_critical(int_mask);

		if((event_flag != 0) || (timeout == 0))
		{
			return event_flag;
		}
		else
		{
			result = rtos_get_semaphore(&ext_event->event_semaphore, timeout);

			if(result == kTimeoutErr)
				return 0;
		}
	}

}

#else

static beken_semaphore_t   shell_semaphore;  // will release from ISR.

static bool_t create_shell_event(void)
{
	rtos_init_semaphore(&shell_semaphore, 1);

	return bTRUE;
}

static bool_t set_shell_event(u32 event_flag)
{
	(void)event_flag;

	rtos_set_semaphore(&shell_semaphore);

	return bTRUE;
}

static u32 wait_any_event(u32 timeout)
{
	int result;

	result = rtos_get_semaphore(&shell_semaphore, timeout);

	if(result == kTimeoutErr)
		return 0;

	return SHELL_EVENT_RX_IND;
}
#endif

static void tx_req_process(void);

static u8 * alloc_log_blk(u16 log_len, u16 *blk_tag)
{
	u16    free_blk_id;
	u8     queue_id;
	u8     ovf_cnt_saved = 0;
	free_queue_t * free_q;
	u8     *       blk_buf = NULL;

	u32  int_mask = shell_task_enter_critical();

	for(queue_id = 0; queue_id < TBL_SIZE(free_queue); queue_id++)
	{
		free_q = &free_queue[queue_id];

		/*    queue ascending in blk_len.    */
		if(free_q->blk_len < log_len)
			continue;

		if(free_q->free_blk_num > 0)
		{
			free_blk_id = free_q->blk_list[free_q->list_out_idx];

			// free_q->list_out_idx = (free_q->list_out_idx + 1) % free_q->blk_num;
			if((free_q->list_out_idx + 1) < free_q->blk_num)
				free_q->list_out_idx++;
			else
				free_q->list_out_idx = 0;

			free_q->free_blk_num--;

			blk_buf = &free_q->log_buf[free_blk_id * free_q->blk_len];
			*blk_tag = MAKE_BLOCK_TAG(free_blk_id, queue_id);

			break;
		}
		else
		{
			if(ovf_cnt_saved == 0)
			{
				free_q->ovf_cnt++;
				ovf_cnt_saved = 1;
			}
		}
	}

	if(blk_buf == NULL)
	{
		shell_log_overflow++;
	}
	else
	{
		fault_hint_print = 0;
		shell_log_count++;
	}

	shell_task_exit_critical(int_mask);

	return blk_buf;
}

static bool_t free_log_blk(u16 block_tag)
{
	u8      queue_id = GET_QUEUE_ID(block_tag);
	u16     blk_id = GET_BLOCK_ID(block_tag);
	free_queue_t *free_q;

	if(queue_id >= TBL_SIZE(free_queue))
		return bFALSE;

	free_q = &free_queue[queue_id];

	if(blk_id >= free_q->blk_num)
		return bFALSE;

	//disable_interrupt(); // called from tx-complete only, don't lock interrupt.

	free_q->blk_list[free_q->list_in_idx] = blk_id;

	//free_q->list_in_idx = (free_q->list_in_idx + 1) % free_q->blk_num;
	if((free_q->list_in_idx + 1) < free_q->blk_num)
		free_q->list_in_idx++;
	else
		free_q->list_in_idx = 0;

	free_q->free_blk_num++;

	//enable_interrupt(); // called from tx-complete only, don't lock interrupt.

	return bTRUE;
}

static int merge_log_data(u16 blk_tag, u16 data_len)
{
	if(pending_queue.list_out_idx == pending_queue.list_in_idx)  /* queue empty! */
	{
		return 0;
	}

	u8      queue_id = GET_QUEUE_ID(blk_tag);
	u16     blk_id = GET_BLOCK_ID(blk_tag);
	if(queue_id >= TBL_SIZE(free_queue))  /* not log buffer */
	{
		return 0;
	}

	free_queue_t *free_q;
	free_q = &free_queue[queue_id];

	u8		* src_buf = NULL;

	if(blk_id < free_q->blk_num)
	{
		src_buf = &free_q->log_buf[blk_id * free_q->blk_len];
	}
	else
		return 0;

	u16    pre_in_idx;

	if(pending_queue.list_in_idx > 0)
		pre_in_idx = pending_queue.list_in_idx - 1;
	else
		pre_in_idx = SHELL_LOG_PEND_NUM - 1;

	queue_id = GET_QUEUE_ID(pending_queue.packet_list[pre_in_idx].blk_tag);
	blk_id = GET_BLOCK_ID(pending_queue.packet_list[pre_in_idx].blk_tag);
	if(queue_id >= TBL_SIZE(free_queue))  /* not log buffer */
	{
		return 0;
	}

	u8		* dst_buf = NULL;
	u16       buf_len = 0;

	free_q = &free_queue[queue_id];

	if(blk_id < free_q->blk_num)
	{
		dst_buf = &free_q->log_buf[blk_id * free_q->blk_len];
		buf_len = free_q->blk_len;
	}
	else
		return 0;

	u16       cur_len = pending_queue.packet_list[pre_in_idx].packet_len;

	if((cur_len + data_len) > buf_len)  /* can be merged into one buffer? */
		return 0;

	memcpy(dst_buf + cur_len, src_buf, data_len);

	pending_queue.packet_list[pre_in_idx].packet_len += data_len;

	free_log_blk(blk_tag);  /* merged, free the log buffer. */

	return 1;
}

/* call this in interrupt !* DISABLED *! context. */
static void push_pending_queue(u16 blk_tag, u16 data_len)
{
	//get_shell_mutex();

	if(merge_log_data(blk_tag, data_len))  /* has been merged? if so, doesn't enqueue the log. */
		return;

	pending_queue.packet_list[pending_queue.list_in_idx].blk_tag = blk_tag;
	pending_queue.packet_list[pending_queue.list_in_idx].packet_len = data_len;

	//pending_queue.list_in_idx = (pending_queue.list_in_idx + 1) % SHELL_LOG_PEND_NUM;
	if((pending_queue.list_in_idx + 1) < SHELL_LOG_PEND_NUM)
		pending_queue.list_in_idx++;
	else
		pending_queue.list_in_idx = 0;

	//release_shell_mutex();

	return;
}

/* call this in interrupt !* DISABLED *! context. */
static void pull_pending_queue(u16 *blk_tag, u16 *data_len)
{
	*blk_tag     = pending_queue.packet_list[pending_queue.list_out_idx].blk_tag;
	*data_len   = pending_queue.packet_list[pending_queue.list_out_idx].packet_len;

	//pending_queue.list_out_idx = (pending_queue.list_out_idx + 1) % SHELL_LOG_PEND_NUM;
	if((pending_queue.list_out_idx + 1) < SHELL_LOG_PEND_NUM)
		pending_queue.list_out_idx++;
	else
		pending_queue.list_out_idx = 0;

	return;
}

int shell_assert_out(bool bContinue, char * format, ...);

/* call from cmd TX ISR. */
static int cmd_tx_complete(u8 *pbuf, u16 buf_tag)
{
	u8      queue_id = GET_QUEUE_ID(buf_tag);
	u16     blk_id = GET_BLOCK_ID(buf_tag);

	/* rsp ok ?? */
	if( queue_id == SHELL_RSP_QUEUE_ID )    /* rsp. */
	{
		/* it is called from cmd_dev tx ISR. */

		if ( (pbuf != cmd_line_buf.rsp_buff) || (blk_id != 0) )
		{
			/* something wrong!!! */
			shell_assert_out(bTRUE, "FAULT: in rsp.\r\n");
		}

		/* rsp compelete, rsp_buff can be used for next cmd/response. */
		rtos_set_semaphore(&cmd_line_buf.rsp_buf_semaphore);

		return 1;
	}

	if( queue_id == SHELL_IND_QUEUE_ID )    /* cmd_ind. */
	{
		/* it is called from cmd_dev tx ISR. */

		if ( (pbuf != cmd_line_buf.cmd_ind_buff) || (blk_id != 0) )
		{
			/* something wrong!!! */
			shell_assert_out(bTRUE, "FAULT: indication.\r\n");
		}

		/* indication tx compelete, cmd_ind_buff can be used for next cmd_indication. */
		rtos_set_semaphore(&cmd_line_buf.ind_buf_semaphore);

		return 1;
	}

	if( queue_id == SHELL_ROM_QUEUE_ID )    /* fault hints buffer, point to flash. */
	{
		/* it is called from cmd_dev tx ISR. */

		if (blk_id == 1)
		{
			if(pbuf != (u8 *)shell_cmd_ovf_str)
			{
				/* something wrong!!! */
				shell_assert_out(bTRUE, "FATAL:t-%x,p-%x\r\n", buf_tag, pbuf);
			}

			cmd_line_buf.cmd_ovf_hint = 0;

			return 1;
		}
	}

	if( queue_id == SHELL_FWD_QUEUE_ID )    /* slave buffer. */
	{
		#if defined(RECV_CMD_LOG_FROM_MBOX)
		if (log_handle_init_ok) {
			set_fwd_state(blk_id);
		} else {
			result_fwd(blk_id);
		}
		return 1;
		#endif
	}

	return 0;
}
/* call from TX ISR. */
static void shell_cmd_tx_complete(u8 *pbuf, u16 buf_tag)
{
	u32  int_mask = shell_task_enter_critical();
	int  tx_handled = cmd_tx_complete(pbuf, buf_tag);

	if(tx_handled == 0)  /* not handled. */
	{
		/*        FAULT !!!!      */
		shell_assert_out(bTRUE, "FATAL:%x,\r\n", buf_tag);
	}

	shell_task_exit_critical(int_mask);
}

/* call from log TX ISR. */
static int log_tx_complete(u8 *pbuf, u16 buf_tag)
{
	u16     block_tag;
	u8      queue_id = GET_QUEUE_ID(buf_tag);
	u16     blk_id = GET_BLOCK_ID(buf_tag);
	free_queue_t *free_q;

	if( queue_id == SHELL_ROM_QUEUE_ID )    /* fault hints buffer, point to flash. */
	{
		/* it is called from log_dev tx ISR. */

		if (blk_id == 0)
		{
			if(pbuf != (u8 *)shell_fault_str)
			{
				/* something wrong!!! */
				shell_assert_out(bTRUE, "FATAL:t-%x,p-%x\r\n", buf_tag, pbuf);
			}

			return 1;
		}
	}

	if (queue_id < TBL_SIZE(free_queue))   /* from log busy queue. */
	{
		/* it is called from log_dev tx ISR. */

		free_q = &free_queue[queue_id];

		block_tag = log_busy_queue.blk_list[log_busy_queue.list_out_idx];

		if( ( buf_tag != block_tag ) || (blk_id >= free_q->blk_num) ||
			( (&free_q->log_buf[blk_id * free_q->blk_len]) != pbuf) )
		{
			/* something wrong!!! */
			/*        FAULT !!!!      */
			#if CONFIG_SYS_CPU0
			shell_assert_out(bTRUE, "FATAL:%x,%x\r\n", buf_tag, block_tag);
			#else
			shell_assert_out(bTRUE, "FATAL-cpx:%x,%x\r\n", buf_tag, block_tag);
			#endif

			return -1;
		}

		/* de-queue from busy queue. */
		//log_busy_queue.list_out_idx = (log_busy_queue.list_out_idx + 1) % SHELL_LOG_BUSY_NUM;
		if((log_busy_queue.list_out_idx + 1) < SHELL_LOG_BUSY_NUM)
			log_busy_queue.list_out_idx++;
		else
			log_busy_queue.list_out_idx = 0;

		log_busy_queue.free_cnt++;

		/* free buffer to queue. */
		free_log_blk(block_tag);

		if (log_buf_semaphore != NULL) {
			rtos_set_semaphore(&log_buf_semaphore);
		}

		return 1;
	}

	if (queue_id == SHELL_DYM_QUEUE_ID) {
		dynamic_log_node *node = dynamic_list_pop_front();
		free_list_push_front(node);
		if (log_buf_semaphore != NULL)
			set_shell_event(&shell_log_event, SHELL_EVENT_DYM_FREE);
		return 1;
	}
	return 0;
}

/* call from TX ISR. */
static void shell_log_tx_complete(u8 *pbuf, u16 buf_tag)
{
	u32  int_mask = shell_task_enter_critical();

	int log_tx_req = log_tx_complete(pbuf, buf_tag);

	if(log_tx_req == 1)
	{
		//set_shell_event(SHELL_EVENT_TX_REQ);  // notify shell task to process the log tx.
		tx_req_process();
	}
	else if(log_tx_req == 0)  /* not handled. */
	{
		/*        FAULT !!!!      */
		shell_assert_out(bTRUE, "FATAL:%x,\r\n", buf_tag);
	}

	shell_task_exit_critical(int_mask);
}

/* call from TX ISR. */
static void shell_tx_complete(u8 *pbuf, u16 buf_tag)
{
	u32  int_mask = shell_task_enter_critical();

	int tx_req = 0;

	tx_req = cmd_tx_complete(pbuf, buf_tag);

	if(tx_req == 0) /* not a cmd tx event, maybe it is a log tx event. */
	{
		tx_req = log_tx_complete(pbuf, buf_tag);
	}

	if(tx_req == 1)
	{
		//set_shell_event(SHELL_EVENT_TX_REQ);  // notify shell task to process the log tx.
		tx_req_process();
	}
	else if(tx_req == 0)  /* not handled. */
	{
		/*        FAULT !!!!      */
		shell_assert_out(bTRUE, "FATAL:%x,\r\n", buf_tag);
	}

	shell_task_exit_critical(int_mask);
}

/* call from RX ISR. */
static void shell_rx_indicate(void)
{
	set_shell_event(&shell_task_event, SHELL_EVENT_RX_IND);

	return;
}

static bool_t echo_out(u8 * echo_str, u16 len)
{
	u16	 wr_cnt;

	if(len == 0)
		return bTRUE;

	wr_cnt = cmd_dev->dev_drv->write_echo(cmd_dev, echo_str, len);

	return (wr_cnt == len);
}

static void cmd_info_out(u8 * msg_buf, u16 msg_len, u16 blk_tag)
{
	if(msg_len == 0)
		return;

	u32  int_mask = shell_task_enter_critical();

	if(log_dev != cmd_dev)
	{
		/* dedicated device for cmd, don't enqueue the msg to pending queue. */
		/* send to cmd dev directly. */
		/* should have a count semaphore for write_asyn calls for rsp/ind/cmd_hint & slave rsp/ind. *
		 * otherwise there will be coupled with driver, drv tx_queue_len MUST be >= 5. */
		cmd_dev->dev_drv->write_async(cmd_dev, msg_buf, msg_len, blk_tag);

	}
	else
	{
		/* shared device for cmd & log, push the rsp msg to pending queue. */
		push_pending_queue(blk_tag, msg_len);

		//set_shell_event(SHELL_EVENT_TX_REQ);  // notify shell task to process the log tx. can't be called in int-disabled context.
		tx_req_process();
	}

	shell_task_exit_critical(int_mask);
}

/*    NOTICE:  this can only be called by shell task internally (cmd handler). */
/*             it is not a re-enterance function becaue of using rsp_buff. */
static bool_t cmd_rsp_out(u8 * rsp_msg, u16 msg_len)
{
	u16    rsp_blk_tag = MAKE_BLOCK_TAG(0, SHELL_RSP_QUEUE_ID);

	if(rsp_msg != cmd_line_buf.rsp_buff)
	{
		if(msg_len > sizeof(cmd_line_buf.rsp_buff))
		{
			msg_len = sizeof(cmd_line_buf.rsp_buff);;
		}

		memcpy(cmd_line_buf.rsp_buff, rsp_msg, msg_len);
	}

	cmd_info_out(cmd_line_buf.rsp_buff, msg_len, rsp_blk_tag);

	return bTRUE;
}

/* it is not a re-enterance function, should sync using ind_buf_semaphore. */
static bool_t cmd_ind_out(u8 * ind_msg, u16 msg_len)
{
	u16    ind_blk_tag = MAKE_BLOCK_TAG(0, SHELL_IND_QUEUE_ID);

	if(ind_msg != cmd_line_buf.cmd_ind_buff)
	{
		if(msg_len > sizeof(cmd_line_buf.cmd_ind_buff))
		{
			msg_len = sizeof(cmd_line_buf.cmd_ind_buff);;
		}

		memcpy(cmd_line_buf.cmd_ind_buff, ind_msg, msg_len);
	}

	cmd_info_out(cmd_line_buf.cmd_ind_buff, msg_len, ind_blk_tag);

	return bTRUE;
}

static bool_t cmd_hint_out(void)
{
	u16    hint_blk_tag = MAKE_BLOCK_TAG(1, SHELL_ROM_QUEUE_ID);

	cmd_info_out((u8 *)shell_cmd_ovf_str, shell_cmd_ovf_str_len, hint_blk_tag);

	return bTRUE;
}

static bool_t log_hint_out(void)
{
	if(fault_hint_print)	/* sent one hint since last allocation fail.*/
		return bTRUE;

	u16    hint_blk_tag = MAKE_BLOCK_TAG(0, SHELL_ROM_QUEUE_ID);

	u32  int_mask = shell_task_enter_critical();

	push_pending_queue(hint_blk_tag, shell_fault_str_len);

	//set_shell_event(SHELL_EVENT_TX_REQ);  // notify shell task to process the log tx.
	tx_req_process();

	fault_hint_print = 1;

	shell_task_exit_critical(int_mask);

	return bTRUE;
}

/* call this in interrupt !* DISABLED *! context. */
static void tx_req_process(void)
{
	u8		*packet_buf = NULL;
	u16		block_tag;
	u16		log_len;
	u16		tx_ready;
	u16		blk_id;
	u8		queue_id;
	free_queue_t *free_q;

	/* maybe tx_req is from tx_complete_callback, check if there any log in queue. */
	if(pending_queue.list_out_idx == pending_queue.list_in_idx)  /* queue empty! */
		return;

	if(log_busy_queue.free_cnt == 0)
		return;

	tx_ready = 0;

	log_dev->dev_drv->io_ctrl(log_dev, SHELL_IO_CTRL_GET_STATUS, &tx_ready);

	if(tx_ready == 0)
		return;

	/**    ====     POP from pending queue     ====    **/
	pull_pending_queue(&block_tag, &log_len);

	queue_id = GET_QUEUE_ID(block_tag);
	blk_id = GET_BLOCK_ID(block_tag);

	if (queue_id < TBL_SIZE(free_queue))
	{
		free_q = &free_queue[queue_id];

		if(blk_id < free_q->blk_num)
		{
			packet_buf = &free_q->log_buf[blk_id * free_q->blk_len];
		}
	}
	else if(queue_id == SHELL_RSP_QUEUE_ID)
	{
		packet_buf = cmd_line_buf.rsp_buff;

		if((log_dev != cmd_dev) || (blk_id != 0))
		{
			shell_assert_out(bTRUE, "xFATAL: in Tx_req\r\n");
			/*		  FAULT !!!!	  */
			/* if log_dev is not the same with cmd_dev,
			 * rsp will not be pushed into pending queue.
			 */
		}
	}
	else if(queue_id == SHELL_IND_QUEUE_ID)
	{
		packet_buf = cmd_line_buf.cmd_ind_buff;

		if((log_dev != cmd_dev) || (blk_id != 0))
		{
			shell_assert_out(bTRUE, "xFATAL: in Tx_req\r\n");
			/*		  FAULT !!!!	  */
			/* if log_dev is not the same with cmd_dev,
			 * indication will not be pushed into pending queue.
			 */
		}
	}
	else if(queue_id == SHELL_ROM_QUEUE_ID)
	{
		if(blk_id == 0)
		{
			packet_buf = (u8 *)shell_fault_str;
		}
		else if(blk_id == 1)
		{
			packet_buf = (u8 *)shell_cmd_ovf_str;
			if(log_dev != cmd_dev)
			{
				shell_assert_out(bTRUE, "xFATAL: in Tx_req id=%x\r\n", blk_id);
				/*		  FAULT !!!!	  */
				/* if log_dev is not the same with cmd_dev,
				 * cmd_hint will not be pushed into pending queue.
				 */
			}
		}
		else
		{
				/*		  FAULT !!!!	  */
				shell_assert_out(bTRUE, "xFATAL: in Tx_req id=%x\r\n", blk_id);
		}
	}
	else if (queue_id == SHELL_DYM_QUEUE_ID)
	{
		dynamic_log_node *node = dynamic_list_switch();
		packet_buf = node->ptr;
	}
	#if defined(FWD_CMD_TO_MBOX)
	else if(queue_id == SHELL_FWD_QUEUE_ID)
	{
		if(blk_id == MBOX_RSP_BLK_ID)
		{
			packet_buf = (u8 *)ipc_fwd_data.rsp_buf.buf;
		}
		else if(blk_id == MBOX_IND_BLK_ID)
		{
			packet_buf = (u8 *)ipc_fwd_data.ind_buf.buf;
			if(log_dev != cmd_dev)
			{
				shell_assert_out(bTRUE, "xFATAL: in Tx_req id=%x\r\n", blk_id);
				/*		  FAULT !!!!	  */
				/* if log_dev is not the same with cmd_dev,
				 * fwd_data will not be pushed into pending queue.
				 */
			}
		}
		else
		{
				/*		  FAULT !!!!	  */
				shell_assert_out(bTRUE, "xFATAL: in Tx_req id=%x\r\n", blk_id);
		}
	}
	#endif
	else
	{
		/*		  FAULT !!!!	  */
		shell_assert_out(bTRUE, "xFATAL: in Tx_req %x.\r\n", block_tag);
	}

	if(packet_buf == NULL)
		return;

	/* rom & rsp buff not enter busy-queue. */
	if(queue_id < TBL_SIZE(free_queue))
	{
		log_busy_queue.free_cnt--;
		log_busy_queue.blk_list[log_busy_queue.list_in_idx] = block_tag;
		//log_busy_queue.list_in_idx = (log_busy_queue.list_in_idx + 1) % SHELL_LOG_BUSY_NUM;
		if((log_busy_queue.list_in_idx + 1) < SHELL_LOG_BUSY_NUM)
			log_busy_queue.list_in_idx++;
		else
			log_busy_queue.list_in_idx = 0;
	}

	log_dev->dev_drv->write_async(log_dev, packet_buf, log_len, block_tag); /* send to log dev driver. */
	/* if driver return 0, should free log-block or not de-queue pending queue and try again. */
	/* if return 1, push log-block into busy queue is OK. */

	return;
}

static void rx_ind_process(void)
{
	u16   read_cnt, buf_len, echo_len;
	u16   i = 0;
	u8    cmd_rx_done = bFALSE, need_backspace = bFALSE;
#if CONFIG_AT_DATA_MODE
	int   at_data_len = 0;
#endif
	if(cmd_dev->dev_type == SHELL_DEV_MAILBOX)
	{
		buf_len = SHELL_RX_BUF_LEN;
	}
	else /* if(cmd_dev->dev_type == SHELL_DEV_UART) */
	{
		buf_len = 1;  /* for UART device, read one by one. */
	}

#if CONFIG_AT_DATA_MODE
	if(ATSVR_WK_DATA_HANDLE== get_atsvr_work_state())
	{
		at_data_len = get_data_len();
	}
#endif

	while(bTRUE)
	{
		u8  * rx_temp_buff = &cmd_line_buf.rx_buff[0];

		read_cnt = cmd_dev->dev_drv->read(cmd_dev, rx_temp_buff, buf_len);

		echo_len = 0;

		for(i = 0; i < read_cnt; i++)
		{
			if(cmd_line_buf.cur_cmd_type == CMD_TYPE_INVALID)
			{
				echo_len++;

				if((rx_temp_buff[i] >= 0x20) && (rx_temp_buff[i] < 0x7f))
				{
					cmd_line_buf.cur_cmd_type = CMD_TYPE_TEXT;

					cmd_line_buf.cmd_data_len = 0;
					cmd_line_buf.cmd_buff[cmd_line_buf.cmd_data_len] = rx_temp_buff[i];
					cmd_line_buf.cmd_data_len++;

					continue;
				}

				/* patch for BK_REG tool. */
				if(cmd_line_buf.bkreg_state == BKREG_WAIT_01)
				{
					if(rx_temp_buff[i] == 0x01)
						cmd_line_buf.bkreg_state = BKREG_WAIT_E0;
				}
				else if(cmd_line_buf.bkreg_state == BKREG_WAIT_E0)
				{
					if(rx_temp_buff[i] == 0xE0)
						cmd_line_buf.bkreg_state = BKREG_WAIT_FC;
					else if(rx_temp_buff[i] != 0x01)
						cmd_line_buf.bkreg_state = BKREG_WAIT_01;
				}
				else if(cmd_line_buf.bkreg_state == BKREG_WAIT_FC)
				{
					if(rx_temp_buff[i] == 0xFC)
					{
						cmd_line_buf.cur_cmd_type = CMD_TYPE_BKREG;

						cmd_line_buf.cmd_buff[0] = 0x01;
						cmd_line_buf.cmd_buff[1] = 0xE0;
						cmd_line_buf.cmd_buff[2] = 0xFC;

						cmd_line_buf.cmd_data_len = 3;

						echo_len = 0;   // cann't echo anything.

						continue;
					}
					else if(rx_temp_buff[i] != 0x01)
						cmd_line_buf.bkreg_state = BKREG_WAIT_01;
					else
						cmd_line_buf.bkreg_state = BKREG_WAIT_E0;
				}

			}

			if(cmd_line_buf.cur_cmd_type == CMD_TYPE_TEXT)
			{
				echo_len++;
				if(rx_temp_buff[i] == '\b')
				{
					if(cmd_line_buf.cmd_data_len > 0)
					{
						cmd_line_buf.cmd_data_len--;

						if(cmd_line_buf.cmd_data_len == 0)
							need_backspace = bTRUE;
					}
				}
				else if((rx_temp_buff[i] == '\n') || (rx_temp_buff[i] == '\r'))
				{
					#if CONFIG_AT_DATA_MODE
						if(ATSVR_WK_DATA_HANDLE== get_atsvr_work_state())
						{
							if(cmd_line_buf.cmd_data_len < at_data_len)
							{
								cmd_line_buf.cmd_buff[cmd_line_buf.cmd_data_len] = rx_temp_buff[i];
								cmd_line_buf.cmd_data_len++;
								continue;
							}
							//else
							//	cmd_line_buf.cmd_buff[cmd_line_buf.cmd_data_len] = 0;  // in case cmd_data_len overflow.

							cmd_rx_done = bTRUE;
							break;
						}

					#endif
						if(cmd_line_buf.cmd_data_len < sizeof(cmd_line_buf.cmd_buff))
						{

							cmd_line_buf.cmd_buff[cmd_line_buf.cmd_data_len] = 0;

						}
						else
						{
							cmd_line_buf.cmd_buff[cmd_line_buf.cmd_data_len - 1] = 0;  // in case cmd_data_len overflow.
						}

					cmd_rx_done = bTRUE;
					break;
				}
				else if((rx_temp_buff[i] >= 0x20))
				{
					if(cmd_line_buf.cmd_data_len < sizeof(cmd_line_buf.cmd_buff))
					{
						cmd_line_buf.cmd_buff[cmd_line_buf.cmd_data_len] = rx_temp_buff[i];
						cmd_line_buf.cmd_data_len++;
					}
				}

			}

			/* patch for BK_REG tool. */
			if(cmd_line_buf.cur_cmd_type == CMD_TYPE_BKREG)
			{
				echo_len = 0;   // cann't echo anything.

				/* p[0] = 0x1, p[1]=0xe0, p[2]=0xfc, p[3]=len. */
				if(cmd_line_buf.cmd_data_len == 3)
				{
					cmd_line_buf.bkreg_left_byte = rx_temp_buff[i] + 1;  // +1, because will -1 in next process.

					if((cmd_line_buf.bkreg_left_byte + 3) >= sizeof(cmd_line_buf.cmd_buff))  // 3 bytes of header + 1 byte of len.
					{
						cmd_line_buf.cmd_data_len = 0;

						cmd_rx_done = bTRUE;
						break;
					}
				}

				if(cmd_line_buf.cmd_data_len < sizeof(cmd_line_buf.cmd_buff))
				{
					cmd_line_buf.cmd_buff[cmd_line_buf.cmd_data_len] = rx_temp_buff[i];
					cmd_line_buf.cmd_data_len++;
				}

				cmd_line_buf.bkreg_left_byte--;

				if(cmd_line_buf.bkreg_left_byte == 0)
				{
					cmd_rx_done = bTRUE;
					break;
				}
			}
		}

		if( cmd_rx_done )
		{
			/* patch for BK_REG tool. */
			if(cmd_line_buf.cur_cmd_type == CMD_TYPE_BKREG)
			{
				break;  // cann't echo anything.
			}

			if(cmd_line_buf.echo_enable)
			{
				echo_out(&rx_temp_buff[0], echo_len);
				echo_out((u8 *)"\r\n", 2);
			}

			break;
		}
		else
		{
			/* patch for BK_REG tool. */
			if( (cmd_line_buf.cur_cmd_type == CMD_TYPE_BKREG) ||
				((cmd_line_buf.cur_cmd_type == CMD_TYPE_INVALID) && (cmd_line_buf.bkreg_state != BKREG_WAIT_01)) )
			{
				 // cann't echo anything.
			}
			else if(cmd_line_buf.echo_enable)
			{
				if(echo_len > 0)
				{
					if( (rx_temp_buff[echo_len - 1] == '\b') ||
						(rx_temp_buff[echo_len - 1] == 0x7f) ) /* DEL */
					{
						echo_len--;
						if((cmd_line_buf.cmd_data_len > 0) || need_backspace)
							echo_out((u8 *)"\b \b", 3);
					}

					u8    cr_lf = 0;

					if(echo_len == 1)
					{
						if( (rx_temp_buff[echo_len - 1] == '\r') ||
							(rx_temp_buff[echo_len - 1] == '\n') )
						{
							cr_lf = 1;
						}
					}
					else if(echo_len == 2)
					{
						if( (memcmp(rx_temp_buff, "\r\n", 2) == 0) ||
							(memcmp(rx_temp_buff, "\n\r", 2) == 0) )
						{
							cr_lf = 1;
						}
					}

					if(cr_lf != 0)
					{
						echo_out((u8 *)shell_prompt_str[prompt_str_idx], 3);
						echo_len = 0;
					}
				}
				echo_out(rx_temp_buff, echo_len);
			}
		}

		if(read_cnt < buf_len) /* all data are read out. */
			break;
	}

	if(read_cnt < buf_len) /* all data are read out. */
	{
	}
	else  /* cmd pends in buffer, handle it in new loop cycle. */
	{
		set_shell_event(&shell_task_event, SHELL_EVENT_RX_IND);
	}

	/* can re-use *buf_len*. */
	if( cmd_rx_done )
	{
		if(cmd_line_buf.cur_cmd_type == CMD_TYPE_TEXT)
		{
			if(cmd_line_buf.cmd_ovf_hint == 0)
			{
				u16		rx_ovf = 0;
				cmd_dev->dev_drv->io_ctrl(cmd_dev, SHELL_IO_CTRL_GET_RX_STATUS, &rx_ovf);

				if(rx_ovf != 0)
				{
					cmd_hint_out();
					cmd_line_buf.cmd_ovf_hint = 1;
				}
			}

			rtos_get_semaphore(&cmd_line_buf.rsp_buf_semaphore, SHELL_WAIT_OUT_TIME);

			cmd_line_buf.rsp_buff[0] = 0;
			/* handle command. */
			if( cmd_line_buf.cmd_data_len > 0 )
			#if defined(CONFIG_AT) && defined(CONFIG_SYS_CPU0)
			#if (CONFIG_UART_PRINT_PORT == AT_UART_PORT_CFG)
				atsvr_msg_get_input((char *)cmd_line_buf.cmd_buff, cmd_line_buf.cmd_data_len, (char *)cmd_line_buf.rsp_buff, SHELL_RSP_BUF_LEN - 4);
			#else
				handle_shell_input( (char *)cmd_line_buf.cmd_buff, cmd_line_buf.cmd_data_len, (char *)cmd_line_buf.rsp_buff, SHELL_RSP_BUF_LEN - 4 );
			#endif
			#else
				handle_shell_input( (char *)cmd_line_buf.cmd_buff, cmd_line_buf.cmd_data_len, (char *)cmd_line_buf.rsp_buff, SHELL_RSP_BUF_LEN - 4 );
			#endif
			cmd_line_buf.rsp_buff[SHELL_RSP_BUF_LEN - 4] = 0;

			buf_len = strlen((char *)cmd_line_buf.rsp_buff);
			if(buf_len > (SHELL_RSP_BUF_LEN - 4))
				buf_len = (SHELL_RSP_BUF_LEN - 4);
			buf_len += sprintf((char *)&cmd_line_buf.rsp_buff[buf_len], shell_prompt_str[prompt_str_idx]);

			cmd_rsp_out(cmd_line_buf.rsp_buff, buf_len);

			rtos_delay_milliseconds(4); // delay 4 ms, so idle task has time to release resources of delete-pendign task.
		}

		/* patch for BK_REG tool. */
		if(cmd_line_buf.cur_cmd_type == CMD_TYPE_BKREG)
		{
			if(cmd_line_buf.cmd_data_len > 3)
			{
#if CONFIG_BKREG
				extern int bkreg_run_command(const char *cmd, int flag);

				bkreg_run_command((const char *)&cmd_line_buf.cmd_buff[0], (int)cmd_line_buf.cmd_data_len);
#endif // CONFIG_BKREG
			}
		}

		cmd_line_buf.cur_cmd_type = CMD_TYPE_INVALID;  /* reset cmd line to interpret new cmd. */
		cmd_line_buf.cmd_data_len = 0;
		cmd_line_buf.bkreg_state = BKREG_WAIT_01;	/* reset state machine. */
	}

	return;
}

extern gpio_id_t bk_uart_get_rx_gpio(uart_id_t id);

static void shell_rx_wakeup(int gpio_id);

static bk_err_t shell_enter_deep_sleep(uint64_t sleep_time, void *args)
{
	if(log_tx_init_ok)
	{
		log_dev->dev_drv->io_ctrl(log_dev, SHELL_IO_CTRL_TX_SUSPEND, (void *)(u32)log_flush_enabled);
	}

	if(cmd_rx_init_ok)
	{
		cmd_dev->dev_drv->io_ctrl(cmd_dev, SHELL_IO_CTRL_RX_SUSPEND, NULL);
        }

	return BK_OK;
}

static void shell_power_save_enter(void)
{
	if(log_tx_init_ok)
	{
		log_dev->dev_drv->io_ctrl(log_dev, SHELL_IO_CTRL_TX_SUSPEND, (void *)(u32)log_flush_enabled);
	}

	if(cmd_rx_init_ok)
	{
		cmd_dev->dev_drv->io_ctrl(cmd_dev, SHELL_IO_CTRL_RX_SUSPEND, NULL);

		if(cmd_dev->dev_type == SHELL_DEV_UART)
		{
			u8   uart_port = UART_ID_MAX;

			cmd_dev->dev_drv->io_ctrl(cmd_dev, SHELL_IO_CTRL_GET_UART_PORT, &uart_port);

			bk_uart_pm_backup(uart_port);

			u32  gpio_id = bk_uart_get_rx_gpio(uart_port);

			bk_gpio_register_isr(gpio_id, (gpio_isr_t)shell_rx_wakeup);
		}
	}
}

static void shell_power_save_exit(void)
{
	if(log_tx_init_ok)
	{
		log_dev->dev_drv->io_ctrl(log_dev, SHELL_IO_CTRL_TX_RESUME, NULL);
	}

	if(cmd_rx_init_ok)
	{
		cmd_dev->dev_drv->io_ctrl(cmd_dev, SHELL_IO_CTRL_RX_RESUME, NULL);
	}
}

static void wakeup_process(void)
{
	bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_LOG, 0, 0);
	shell_pm_wake_flag = 1;
	shell_pm_wake_time = SHELL_TASK_WAKE_CYCLE;
}

static void shell_rx_wakeup(int gpio_id)
{
	wakeup_process();
	set_shell_event(&shell_task_event, SHELL_EVENT_WAKEUP);

	shell_log_raw_data((const u8*)"wakeup\r\n", sizeof("wakeup\r\n") - 1);

	if(cmd_dev->dev_type == SHELL_DEV_UART)
	{
		bk_gpio_register_isr(gpio_id, NULL);
	}
}

// if use psram as dynamic log memory, first malloc will init psram.
// in the process of psram-initialization, will output some logs.
// in the case, logs psram-init can not use dynamic logs.
// so, initialization psram in advace will prevent logs discard at startup.
static void dynamic_log_init(void)
{
#if CONFIG_PSRAM_AS_SYS_MEMORY
	bool bk_psram_heap_init_flag_get();
	if (bk_psram_heap_init_flag_get() == bFALSE) {
		void *ptr = LOG_MALLOC(0);
		if (ptr != NULL)
			LOG_FREE(ptr);
	}
#endif
}

static void shell_log_tx_init(void)
{
	u16		i;

	if(log_tx_init_ok != 0)
		return;

	for(i = 0; i < SHELL_LOG_BUF1_NUM; i++)
	{
		buff1_free_list[i] = i;
	}
	for(i = 0; i < SHELL_LOG_BUF2_NUM; i++)
	{
		buff2_free_list[i] = i;
	}
	for(i = 0; i < SHELL_LOG_BUF3_NUM; i++)
	{
		buff3_free_list[i] = i;
	}

	memset(&log_busy_queue, 0, sizeof(log_busy_queue));
	memset(&pending_queue, 0, sizeof(pending_queue));

	log_busy_queue.free_cnt = SHELL_LOG_BUSY_NUM;

	log_dev->dev_drv->init(log_dev);
	log_dev->dev_drv->open(log_dev, shell_log_tx_complete, NULL);  // tx log.

	#if defined(FWD_CMD_TO_MBOX) || defined(RECV_CMD_LOG_FROM_MBOX)
	ipc_dev->dev_drv->init(ipc_dev);
	ipc_dev->dev_drv->open(ipc_dev, (shell_ipc_rx_t)shell_ipc_rx_indication, (shell_ipc_tx_complete_t)shell_ipc_tx_complete);   /* register rx-callback to copy log data to buffer. */
	#endif

	log_tx_init_ok = 1;

	{
		pm_cb_conf_t enter_config;
		enter_config.cb = (pm_cb)shell_power_save_enter;
		enter_config.args = NULL;

		pm_cb_conf_t exit_config;
		exit_config.cb = (pm_cb)shell_power_save_exit;
		exit_config.args = NULL;

		#if 0
		bk_pm_sleep_register_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_UART1, &enter_config, &exit_config);

		u8   uart_port = UART_ID_MAX;

		cmd_dev->dev_drv->io_ctrl(cmd_dev, SHELL_IO_CTRL_GET_UART_PORT, &uart_port);

		shell_rx_wakeup(bk_uart_get_rx_gpio(uart_port));
		#endif

		u8 uart_port = UART_ID_MAX;
		log_dev->dev_drv->io_ctrl(log_dev, SHELL_IO_CTRL_GET_UART_PORT, &uart_port);

		u8 pm_uart_port = uart_id_to_pm_uart_id(uart_port);
		bk_pm_sleep_register_cb(PM_MODE_LOW_VOLTAGE, pm_uart_port, &enter_config, &exit_config);
	}

	dynamic_log_init();
}

#define LOG_HANDLE_TASK_STACK 0x200
beken_thread_t log_thread_handle = NULL;
void create_log_handle_task(void)
{
	int ret;
	shell_log_event.event_flag = 0;

	check_and_free_dynamic_node();

	rtos_init_semaphore(&shell_log_event.event_semaphore, 1);

	rtos_init_semaphore_ex(&log_buf_semaphore, 1, 1);               // semaphore for log block mode.

	ret = rtos_create_thread(&log_thread_handle,
							 4,
							 "log_hanlder",
							 (beken_thread_function_t)log_handle_task,
							 LOG_HANDLE_TASK_STACK,
							 0);
	if (ret != 0) {
		os_printf("create log handler task fail!\r\n");
		return;
	}
	log_handle_init_ok = 1;
}


static void shell_task_init(void)
{
	if(log_tx_init_ok == 0)
	{
		shell_log_tx_init();
	}

	/*  ================================    cmd channel initialize   ====================================  */
	cmd_line_buf.cur_cmd_type = CMD_TYPE_INVALID;
	cmd_line_buf.cmd_data_len = 0;
	cmd_line_buf.bkreg_state = BKREG_WAIT_01;
	cmd_line_buf.echo_enable = bTRUE;
	cmd_line_buf.cmd_ovf_hint = 0;

	rtos_init_semaphore_ex(&cmd_line_buf.rsp_buf_semaphore, 1, 1);  // one buffer for cmd_rsp.
	rtos_init_semaphore_ex(&cmd_line_buf.ind_buf_semaphore, 1, 1);  // one buffer for cmd_ind.

	/* cmd fwd */
	#if defined(FWD_CMD_TO_MBOX)
    rtos_init_semaphore_ex(&cmd_line_buf.cmd_fwd_semaphore, 1, 0);  // fwd to CPUx
    #endif

	create_shell_event();

	if(log_dev != cmd_dev)
	{
		cmd_dev->dev_drv->init(cmd_dev);
		cmd_dev->dev_drv->open(cmd_dev, shell_cmd_tx_complete, shell_rx_indicate); // rx cmd, tx rsp.

		// log_dev->dev_drv->init(log_dev);
		// log_dev->dev_drv->open(log_dev, shell_log_tx_complete, NULL);  // tx log.
	}
	else
	{
		u32 int_mask = shell_task_enter_critical();
		cmd_dev->dev_drv->io_ctrl(cmd_dev, SHELL_IO_CTRL_SET_RX_ISR, shell_rx_indicate); // rx cmd, tx (rsp & log).
		cmd_dev->dev_drv->io_ctrl(cmd_dev, SHELL_IO_CTRL_SET_TX_CMPL_ISR, shell_tx_complete); // rx cmd, tx (rsp & log).
		shell_task_exit_critical(int_mask);
	}

	cmd_rx_init_ok = 1;

	{
		pm_cb_conf_t enter_config;
		enter_config.cb = (pm_cb)shell_power_save_enter;
		enter_config.args = NULL;

		pm_cb_conf_t exit_config;
		exit_config.cb = (pm_cb)shell_power_save_exit;
		exit_config.args = NULL;

		#if 0
		bk_pm_sleep_register_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_UART1, &enter_config, &exit_config);

		u8   uart_port = UART_ID_MAX;

		cmd_dev->dev_drv->io_ctrl(cmd_dev, SHELL_IO_CTRL_GET_UART_PORT, &uart_port);

		shell_rx_wakeup(bk_uart_get_rx_gpio(uart_port));
		#endif

		uart_id_t uart_port = UART_ID_MAX;
		cmd_dev->dev_drv->io_ctrl(cmd_dev, SHELL_IO_CTRL_GET_UART_PORT, &uart_port);

		u8 pm_uart_port = uart_id_to_pm_uart_id(uart_port);
		bk_pm_sleep_register_cb(PM_MODE_LOW_VOLTAGE, pm_uart_port, &enter_config, &exit_config);

		shell_rx_wakeup(bk_uart_get_rx_gpio(uart_port));

		enter_config.cb = (pm_cb)shell_enter_deep_sleep;
		exit_config.args = (void *)PM_CB_PRIORITY_1;

		bk_pm_sleep_register_cb(PM_MODE_DEEP_SLEEP, pm_uart_port, &enter_config, &exit_config);
	}

	if(ate_is_enabled())
		prompt_str_idx = 1;
	else
		prompt_str_idx = 0;

}

#if defined(RECV_CMD_LOG_FROM_MBOX)
static void set_fwd_state(int blk_id)
{
	u32  int_mask = shell_task_enter_critical();
	s_fwd_status |= 1 << blk_id;
	shell_task_exit_critical(int_mask);
	set_shell_event(&shell_log_event, SHELL_EVENT_MB_FWD);
}

static int get_fwd_blk_id(void)
{
	int blk_id = -1;
	u32  int_mask = shell_task_enter_critical();
	for (int i = 0; i < 3; i++) {
		if (s_fwd_status & (1 << i)) {
			int mask = ~(1 << i);
			s_fwd_status &= mask;
			blk_id = i;
			break;
		}
	}
	shell_task_exit_critical(int_mask);
	return blk_id;
}

#define FORWARD_TRY_COUNT 10
static void fwd_mb_log_state(void)
{
	int ret = 0;
	int blk_id = get_fwd_blk_id();
	if (blk_id < 0) {
		return;
	}
	int try_cnt = FORWARD_TRY_COUNT;
	do {
		ret = result_fwd(blk_id);
		try_cnt--;
		if (ret) {
			break;
		}
		if (try_cnt == 0) {
			shell_assert_out(1, "Error: forward mb log state fail! blk: %x\r\n", blk_id);
			break;
		}
		rtos_delay_milliseconds(SHELL_LOG_FWD_WAIT_TIME);
	} while (1);
}

static void output_mb_log_ex(void)
{
	// it will not hint log discarded from cpu1.
	shell_log_raw_data_internel(1, ipc_fwd_data.common_log_buf.buf, ipc_fwd_data.common_log_buf.len);
	set_fwd_state(MBOX_COMMON_BLK_ID);
}

#define INVALID_LOG_TAG 0xFFFF
void reset_forward_log_status(void)
{
	u32  int_mask = shell_task_enter_critical();

	ipc_fwd_data.rsp_buf.tag = INVALID_LOG_TAG;
	ipc_fwd_data.ind_buf.tag = INVALID_LOG_TAG;
	ipc_fwd_data.common_log_buf.tag = INVALID_LOG_TAG;

	shell_task_exit_critical(int_mask);
}
#endif

static void log_handle_task( void *para )
{
	u32    Events;
	while(bTRUE)
	{
		Events = wait_any_event(&shell_log_event, BEKEN_WAIT_FOREVER);

		if(Events & SHELL_EVENT_DYM_FREE)
		{
			check_and_free_dynamic_node();
		}
	#if defined(RECV_CMD_LOG_FROM_MBOX)
		if(Events & SHELL_EVENT_MB_LOG)
		{
			output_mb_log_ex();
		}
		if(Events & SHELL_EVENT_MB_FWD)
		{
			fwd_mb_log_state();
		}
	#endif
	}
}

void shell_task( void *para )
{
	u32    Events;
	u32    timeout = SHELL_TASK_WAIT_TIME;

	shell_task_init();

	echo_out((u8 *)shell_prompt_str[prompt_str_idx], 3);

	while(bTRUE)
	{
		Events = wait_any_event(&shell_task_event, timeout);  // WAIT_EVENT;

		if(Events & SHELL_EVENT_TX_REQ)
		{
			echo_out((u8 *)"Unsolicited", sizeof("Unsolicited") - 1);
			echo_out((u8 *)shell_prompt_str[prompt_str_idx], 3);
		}

		if(Events & SHELL_EVENT_RX_IND)
		{
			wakeup_process();
			rx_ind_process();
		}

		if(Events & SHELL_EVENT_WAKEUP)
		{
			// TODO
		}

		if(Events == 0)
		{
			if(shell_pm_wake_time > 0)
				shell_pm_wake_time--;

			if(shell_pm_wake_time == 0)
			{
				if(shell_pm_wake_flag != 0)
				{
					shell_pm_wake_flag = 0;
					bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_LOG, 1, 0);
					//shell_log_raw_data((const u8*)"sleep\r\n", sizeof("sleep\r\n") - 1);
				}
			}
		}

		if(shell_pm_wake_flag)
		{
			timeout = SHELL_TASK_WAIT_TIME;
		}
		else
		{
			timeout = BEKEN_WAIT_FOREVER;
		}
	}
}

int shell_get_cpu_id(void)
{
#ifdef CONFIG_FREERTOS_SMP
	return rtos_get_core_id();
#elif (CONFIG_CPU_CNT > 1)

	if(log_dev->dev_type == SHELL_DEV_MAILBOX)
		return SELF_CPU;

#endif

	return -1;
}

static int shell_cpu_check_valid(void)
{
#if (CONFIG_CPU_CNT > 1) && (LOG_DEV != DEV_MAILBOX)

	if(shell_log_owner_cpu == 0)
		return 1;

	u8    req_cpu = 0;
#ifdef CONFIG_FREERTOS_SMP
	req_cpu = 1 << rtos_get_core_id();
#else
	req_cpu = shell_log_req_cpu;
#endif

	if(shell_log_owner_cpu == req_cpu)
		return 1;

	return 0;

#else

	return 1;

#endif
}

int shell_level_check_valid(int level)
{
	if( !shell_cpu_check_valid() )
		return 0;

	if(level > shell_log_level)
		return 0;

	return 1;
}

#if 0
int shell_log_out_sync(int level, char *prefix, const char *format, va_list ap)
{
	u32         int_mask;
	char       *pbuf;
	u16         data_len = 0, buf_len;

	if( !shell_level_check_valid(level) )
		return 0;

	pbuf = (char *)&shell_assert_buff[0];
	buf_len = sizeof(shell_assert_buff);

	int_mask = shell_task_enter_critical();

	if(prefix != NULL)
	{
		strncpy(pbuf, prefix, buf_len);
		pbuf[buf_len - 1] = 0;
		data_len = strlen(pbuf);
	}
	else
	{
		data_len = 0;
	}

	data_len += vsnprintf( &pbuf[data_len], buf_len - data_len, format, ap );

	if(data_len >= buf_len)
		data_len = buf_len - 1;

	if ( (data_len != 0) && (pbuf[data_len - 1] == '\n') )
	{
		if ((data_len == 1) || (pbuf[data_len - 2] != '\r'))
		{
			pbuf[data_len] = '\n';      /* '\n\0' replaced with '\r\n', may not end with '\0'. */
			pbuf[data_len - 1] = '\r';
			data_len++;
		}
	}

	log_dev->dev_drv->write_sync(log_dev, (u8 *)pbuf, data_len);

	shell_task_exit_critical(int_mask);

	return 1;
}
#endif

static int shell_log_raw_data_internel(bool hint, const u8 *data, u16 data_len)
{
	u8   *packet_buf;
	u16   blk_tag;

	if( !log_tx_init_ok )
	{
		shell_log_tx_init();
	}

	if( !shell_cpu_check_valid() )
		return 0;

	if (NULL == data || 0 == data_len)
	{
		return 0; // bFALSE;
	}

	packet_buf = alloc_buffer(s_block_mode, &blk_tag, data_len);

	if (NULL == packet_buf)
	{
		if (hint == 0)
			return 0;

		if (s_block_mode & LOG_BLOCK_MASK) {
			output_insert_data(data, data_len);
			return 1;
		}
		else
			log_hint_out();
		return 0; // bFALSE;
	}

	memcpy(packet_buf, data, data_len);

	u32 int_mask = shell_task_enter_critical();

	if ( GET_QUEUE_ID(blk_tag) == SHELL_DYM_QUEUE_ID ) {
		dynamic_list_push_back_by_buffer(packet_buf);
	}

	// push to pending queue.
	push_pending_queue(blk_tag, data_len);

	// notify shell task to process the log tx.
	tx_req_process();

	shell_task_exit_critical(int_mask);

	return 1; // bTRUE;
}

int shell_log_raw_data(const u8 *data, u16 data_len)
{
	return shell_log_raw_data_internel(1, data, data_len);
}

static int check_block_mode(int block_mode)
{
	block_mode &= s_block_mode;
	if (rtos_local_irq_disabled() || rtos_is_in_interrupt_context() ||
		log_buf_semaphore == NULL || rtos_is_scheduler_suspended()) {
		block_mode &= LOG_NONBLOCK_MODE;
	}
	return block_mode & LOG_BLOCK_MASK;
}

static inline int transfer_line_end(int data_len, char * ptr)
{
	if ( (data_len != 0) && (ptr[data_len - 1] == '\n') )
	{
		if (data_len == 1 || ptr[data_len - 2] != '\r')
		{
			ptr[data_len] = '\n';
			ptr[data_len - 1] = '\r';
			(data_len)++;
		}
	}
	return data_len;
}

static int combine_log_with_prefix(const char *prefix, char *pbuf, int buf_len, const char *format, va_list ap)
{
	int log_len = 0;
	if (prefix != NULL) {
		strcpy(pbuf, prefix);
		log_len = strlen(prefix);
	}
	log_len += vsnprintf(&pbuf[log_len], buf_len - log_len, format, ap);
	BK_ASSERT(log_len <= buf_len);
	if (log_len == buf_len) {
		log_len = buf_len - 1;
	}
	log_len = transfer_line_end(log_len, pbuf);
	return log_len;
}

// check whether dynamic log available.
static inline bool get_dynamic_log_status(int block_mode, u16 buf_len)
{
	block_mode &= s_block_mode;
	return (s_dynamic_log_num < SHELL_DYM_LOG_NUM_MAX) &&
		   (block_mode & LOG_MALLOC_MASK) &&
		   (buf_len + s_dynamic_log_total_len <= CONFIG_DYM_LOG_MEM_MAX);
}

#define LOG_TRY_ALLOC_COUNT 5
/* alloc log buffer from static memory or dynamic memory */
static u8 * alloc_buffer(int block_mode, u16 *blk_tag, u16 buf_len)
{
	u8 *packet_buf = NULL;
	int try_cnt = LOG_TRY_ALLOC_COUNT;
	do {
		if (buf_len <= SHELL_LOG_BUF1_LEN) {
			packet_buf = alloc_log_blk(buf_len, blk_tag);
			if (packet_buf != NULL) {
				break;
			}
		} else {
			/* while long log is more than static block maximum buffer and cannot use dynamic log, drop it.  */
			if ((block_mode & LOG_MALLOC_MASK) == 0) {
				break;
			}
		}
		if (get_dynamic_log_status(block_mode, buf_len)) {
			packet_buf = alloc_dynamic_log_blk(buf_len, blk_tag);
			if (packet_buf != NULL) {
				break;
			}
		}
		if (check_block_mode(block_mode) == 0) {
			break;
		}
		rtos_get_semaphore(&log_buf_semaphore, SHELL_LOG_BLOCK_TIME);
	} while (try_cnt--);
	return packet_buf;
}

void shell_log_out_port(int block_mode, int level, char *prefix, const char *format, va_list ap)
{
	u8   * packet_buf;
	u16    blk_tag;
	u16    log_len = 0, buf_len;

	if( !log_tx_init_ok )
	{
		shell_log_tx_init();
	}

	if( !shell_level_check_valid(level) )
	{
		return ;
	}

	buf_len = vsnprintf( NULL, 0, format, ap ) + 1;  /* for '\0' */

	if(prefix != NULL)
		buf_len += strlen(prefix);

	if(buf_len == 0)
		return;

	packet_buf = alloc_buffer(block_mode, &blk_tag, buf_len);

	if(packet_buf == NULL)
	{
		if (block_mode & s_block_mode & LOG_BLOCK_MASK)
			output_insert_log(buf_len, prefix, format, ap);
		else
			log_hint_out();
		return;
	}

	log_len = combine_log_with_prefix(prefix, (char *)&packet_buf[0], buf_len, format, ap);

	u32  int_mask = shell_task_enter_critical();

	if ( GET_QUEUE_ID(blk_tag) == SHELL_DYM_QUEUE_ID ) {
		dynamic_list_push_back_by_buffer(packet_buf);
	}

	// push to pending queue.
	push_pending_queue(blk_tag, log_len);

	//set_shell_event(SHELL_EVENT_TX_REQ);  // notify shell task to process the log tx.
	tx_req_process();

	shell_task_exit_critical(int_mask);

	return ;
}

static int shell_assert_out_va(bool bContinue, const char * format, va_list arg_list)
{
	u32         int_mask;
	char       *pbuf;
	u16         data_len, buf_len;

	if( !shell_cpu_check_valid() )
		return 0;

	pbuf = (char *)&shell_assert_buff[0];
	buf_len = sizeof(shell_assert_buff);

	/* just disabled interrupts even when dump out in SMP. */
	/* because other core's dump has been blocked by shell_cpu_check_valid(). */
	/* can't try to get SPINLOCK, because may be it is called from HardFault handler, */
	/* and meanwhile the spinlock is held by other cores. */

	// int_mask = shell_task_enter_critical();
	int_mask = rtos_disable_int();

	data_len = vsnprintf( pbuf, buf_len, format, arg_list );

	if(data_len >= buf_len)
		data_len = buf_len - 1;

	log_dev->dev_drv->write_sync(log_dev, (u8 *)pbuf, data_len);

	if( bContinue )
	{
		// shell_task_exit_critical(int_mask);
		rtos_enable_int(int_mask);
	}
	else
	{
		while(bTRUE)
		{
		}
	}

	return 1;//bTRUE;;

}

int shell_assert_out(bool bContinue, char * format, ...)
{
	int ret;
	va_list     arg_list;
	va_start( arg_list, format );
	ret = shell_assert_out_va(bContinue, format, arg_list);
	va_end( arg_list );
	return ret;
}

int shell_assert_raw(bool bContinue, char * data_buff, u16 data_len)
{
	u32         int_mask;

	if( !shell_cpu_check_valid() )
		return 0;

	/* just disabled interrupts even when dump out in SMP. */
	/* because other core's dump has been blocked by shell_cpu_check_valid(). */
	/* can't try to get SPINLOCK, because may be it is called from HardFault handler, */
	/* and meanwhile the spinlock is held by other cores. */

	// int_mask = shell_task_enter_critical();
	int_mask = rtos_disable_int();

	log_dev->dev_drv->write_sync(log_dev, (u8 *)data_buff, data_len);

	if( bContinue )
	{
		// shell_task_exit_critical(int_mask);
		rtos_enable_int(int_mask);
	}
	else
	{
		while(1)
		{
		}
	}

	return 1;//bTRUE;;

}

#if defined(FWD_CMD_TO_MBOX) || defined(RECV_CMD_LOG_FROM_MBOX)
static int cmd_rsp_fwd(u8 * rsp_msg, u16 msg_len)
{
	u16    rsp_blk_tag = MAKE_BLOCK_TAG(MBOX_RSP_BLK_ID, SHELL_FWD_QUEUE_ID);

	cmd_info_out(rsp_msg, msg_len, rsp_blk_tag);

	return 1;
}

static int cmd_ind_fwd(u8 * ind_msg, u16 msg_len)
{
	u16    ind_blk_tag = MAKE_BLOCK_TAG(MBOX_IND_BLK_ID, SHELL_FWD_QUEUE_ID);

	cmd_info_out(ind_msg, msg_len, ind_blk_tag);

	return 1;
}

static int result_fwd(int blk_id)
{
	log_cmd_t   * log_cmd;
	u32  cmd = MB_CMD_LOG_OUT_OK;
	if(blk_id == MBOX_RSP_BLK_ID)
	{
		log_cmd = &ipc_fwd_data.rsp_buf;
	}
	else if(blk_id == MBOX_IND_BLK_ID)
	{
		log_cmd = &ipc_fwd_data.ind_buf;
	}
	else if(blk_id == MBOX_COMMON_BLK_ID)
	{
		log_cmd = &ipc_fwd_data.common_log_buf;
		cmd = MB_CMD_LOG_UNBLOCK;
	}
	else
	{
		return 0;
	}

	log_cmd->hdr.data = 0;
	log_cmd->hdr.cmd = cmd;
	u32  int_mask = shell_task_enter_critical();
	if (log_cmd->tag == INVALID_LOG_TAG) {
		shell_task_exit_critical(int_mask);
		return 0;
	}
	int ret = ipc_dev->dev_drv->write_cmd(ipc_dev, (mb_chnl_cmd_t *)log_cmd);
	shell_task_exit_critical(int_mask);
	return ret;
}

static void shell_ipc_tx_complete(u16 cmd)
{
	if (cmd == MB_CMD_LOG_OUT_OK || cmd == MB_CMD_LOG_UNBLOCK) {
		set_shell_event(&shell_log_event, SHELL_EVENT_MB_FWD);
	}

	if(cmd == MB_CMD_USER_INPUT) {
		rtos_set_semaphore(&cmd_line_buf.cmd_fwd_semaphore);
	}
}

static u32 shell_ipc_rx_indication(u16 cmd, log_cmd_t *log_cmd, u16 cpu_id)
{
	u32   result = ACK_STATE_FAIL;
	u8  * data = log_cmd->buf;
	u16   data_len = log_cmd->len;

	if(shell_log_owner_cpu != 0)
	{
		if(shell_log_owner_cpu == (0x01 << cpu_id))
			shell_log_req_cpu = shell_log_owner_cpu;
		else
			return result;
	}

	if(cmd == MB_CMD_LOG_OUT)
	{
		u8      queue_id = GET_QUEUE_ID(log_cmd->tag);

		if(queue_id == SHELL_RSP_QUEUE_ID)
		{
			if(cmd_rx_init_ok)
			{
				memcpy(&ipc_fwd_data.rsp_buf, log_cmd, sizeof(ipc_fwd_data.rsp_buf));
				cmd_rsp_fwd(data, data_len);

				result = ACK_STATE_PENDING;
			}
		}
		else if(queue_id == SHELL_IND_QUEUE_ID)
		{
			if(cmd_rx_init_ok)
			{
				memcpy(&ipc_fwd_data.ind_buf, log_cmd, sizeof(ipc_fwd_data.ind_buf));
				cmd_ind_fwd(data, data_len);

				result = ACK_STATE_PENDING;
			}
		}
		else  // no cmd_hint from slave, so must be log from slave.
		{
			result = shell_log_raw_data_internel(0, data, data_len);

			if(result == 0) {
				if (log_handle_init_ok) {
					// tansfer log output to shell task
					memcpy(&ipc_fwd_data.common_log_buf, log_cmd, sizeof(ipc_fwd_data.common_log_buf));
					set_shell_event(&shell_log_event, SHELL_EVENT_MB_LOG);
					result = ACK_STATE_PENDING | ACK_STATE_BLOCK;
				} else {
					result = ACK_STATE_FAIL;
				}
			}
			else
				result = ACK_STATE_COMPLETE;
		}
	}
	else if(cmd == MB_CMD_ASSERT_OUT)
	{
		#if (CONFIG_TASK_WDT)
		bk_task_wdt_feed();
		#endif
		#if (CONFIG_INT_WDT)
		bk_int_wdt_feed();
		#endif

		shell_assert_raw(true, (char *)data, data_len);

		result = ACK_STATE_COMPLETE;
	}

	shell_log_req_cpu = 0;

	/* no cmd handler. */
	return result;
}

int shell_cmd_forward(char *cmd, u16 cmd_len)
{
	mb_chnl_cmd_t	mb_cmd_buf;
	user_cmd_t * user_cmd = (user_cmd_t *)&mb_cmd_buf;
	user_cmd->hdr.data = 0;
	user_cmd->hdr.cmd = MB_CMD_USER_INPUT;
	user_cmd->buf = (u8 *)cmd;
	user_cmd->len = cmd_len;
	int ret_code = 0;
	int try_cnt = 0;
    while(1)
    {
	    u32  int_mask = shell_task_enter_critical();
	    ret_code = ipc_dev->dev_drv->write_cmd(ipc_dev, &mb_cmd_buf);
	    shell_task_exit_critical(int_mask);

	    if(ret_code != 0)
	        break;

	    rtos_delay_milliseconds(10);
	    try_cnt++;
	    if(try_cnt < 4)
	        continue;
	    else
	        return 0;
	}

	rtos_get_semaphore(&cmd_line_buf.cmd_fwd_semaphore, SHELL_WAIT_OUT_TIME);
	return ret_code;
}
#endif

void shell_echo_set(int en_flag)
{
	if(en_flag != 0)
		cmd_line_buf.echo_enable = bTRUE;
	else
		cmd_line_buf.echo_enable = bFALSE;
}

int shell_echo_get(void)
{
	if(cmd_line_buf.echo_enable)
		return 1;

	return 0;
}

void shell_set_log_level(int level)
{
	shell_log_level = level;
}

int shell_get_log_level(void)
{
	return shell_log_level;
}

void shell_set_log_flush(int flush_flag)
{
	log_flush_enabled = flush_flag;
}

int shell_get_log_flush(void)
{
	return log_flush_enabled;
}

int shell_get_log_statist(u32 * info_list, u32 num)
{
	int   cnt = 0;
	if(num > 0)
	{
		info_list[0] = shell_log_overflow;
		cnt++;
	}
	if(num > 1)
	{
		info_list[1] = shell_log_count;
		cnt++;
	}
	if(num > 2)
	{
		info_list[2] = free_queue[0].ovf_cnt;
		cnt++;
	}
	if(num > 3)
	{
		info_list[3] = free_queue[1].ovf_cnt;
		cnt++;
	}
	if(num > 4)
	{
		info_list[4] = free_queue[2].ovf_cnt;
		cnt++;
	}

	return cnt;
}

void shell_log_flush(void)
{
	u32         int_mask;

	/* just disabled interrupts even when dump out in SMP. */
	/* because it is called by power_saving process or by dumping process. */
	/* in any case, only one core is running. */
	/* can't try to get SPINLOCK, because may be it is called from HardFault handler, */
	/* and meanwhile the spinlock is held by other cores. */

	// int_mask = shell_task_enter_critical();
	int_mask = rtos_disable_int();

	log_dev->dev_drv->io_ctrl(log_dev, SHELL_IO_CTRL_FLUSH, NULL);

	// shell_task_exit_critical(int_mask);
	rtos_enable_int(int_mask);
}

void shell_set_uart_port(uint8_t uart_port)
{
#if (LOG_DEV == DEV_UART)
	if(log_dev->dev_type != SHELL_DEV_UART)
	{
		return;
	}

	if ((bk_get_printf_port() != uart_port) && (uart_port < UART_ID_MAX))
	{
		u32  int_mask = shell_task_enter_critical();

		shell_log_flush();
		log_dev->dev_drv->close(log_dev);
		log_dev->dev_drv->io_ctrl(log_dev, SHELL_IO_CTRL_SET_UART_PORT, &uart_port);

		bk_set_printf_port(uart_port);

		log_dev->dev_drv->init(log_dev);
		if(log_dev != cmd_dev)
		{
			log_dev->dev_drv->open(log_dev, shell_log_tx_complete, NULL);  // tx log.
		}
		else
		{
			log_dev->dev_drv->open(log_dev, shell_tx_complete, shell_rx_indicate);
		}

		shell_task_exit_critical(int_mask);
	}
#endif
}

void shell_cmd_ind_out(const char *format, ...)
{
	u16   data_len, buf_len = SHELL_IND_BUF_LEN;
	va_list  arg_list;

	if(!cmd_rx_init_ok)
		return;

	rtos_get_semaphore(&cmd_line_buf.ind_buf_semaphore, SHELL_WAIT_OUT_TIME);

	va_start(arg_list, format);
	data_len = vsnprintf( (char *)&cmd_line_buf.cmd_ind_buff[0], buf_len - 1, format, arg_list );
	va_end(arg_list);

	if(data_len >= buf_len)
		data_len = buf_len - 1;

	cmd_ind_out(cmd_line_buf.cmd_ind_buff, data_len);
}

void shell_set_log_cpu(u8 req_cpu)
{
#if (CONFIG_CPU_CNT > 1) && (LOG_DEV != DEV_MAILBOX)
	if(req_cpu >= CONFIG_CPU_CNT)
	{
		shell_log_owner_cpu = 0;
		shell_log_req_cpu = 0;
	}
	else if(shell_log_owner_cpu == 0)
	{
		shell_log_owner_cpu = 1 << req_cpu;
		shell_log_req_cpu   = 1 << req_cpu;
	}
#endif
}

static void free_list_push_front(dynamic_log_node *dym_node)
{
	if (dym_node == NULL) {
		BK_ASSERT(0);
		return;
	}
	dym_node->next = s_to_free_list;
	s_to_free_list = dym_node;
}

static void dynamic_list_push_back(dynamic_log_node *dym_node)
{
	if (s_curr_node == NULL) {
		s_curr_node = dym_node;
	}
	s_dym_tail_node->next = dym_node;
	s_dym_tail_node = dym_node;

	s_dynamic_log_total_len += dym_node->len;
	if (s_dynamic_log_total_len > s_dynamic_log_mem_max) {
		s_dynamic_log_mem_max = s_dynamic_log_total_len;
	}
	s_dynamic_log_num++;
}

static dynamic_log_node *dynamic_list_pop_front(void)
{
	dynamic_log_node *node = s_dynamic_header.next;
	if (node == NULL) {
		BK_ASSERT(0);
		return NULL;
	}
	BK_ASSERT(s_dynamic_header.next != s_curr_node);
	s_dynamic_header.next = node->next;
	s_dynamic_log_num--;
	node->next = NULL;
	if (node == s_dym_tail_node) {
		s_dym_tail_node = &s_dynamic_header;
		BK_ASSERT(s_curr_node == NULL);
	}
	return node;
}

static void dynamic_node_gc(void)
{
	dynamic_log_node *node;
	dynamic_log_node *temp_node;
	dynamic_log_node *free_list = NULL;
	u32  int_mask = shell_task_enter_critical();
	if (s_to_free_list != NULL) {
		free_list = s_to_free_list;
		s_to_free_list = NULL;
	}
	shell_task_exit_critical(int_mask);
	node = free_list;
	while (node != NULL) {
		temp_node = node;
		s_dynamic_log_total_len -= node->len;
		node = node->next;
		LOG_FREE(temp_node);
		s_dynamic_log_num_in_mem--;
	}
}

static void check_and_free_dynamic_node(void)
{
	if (s_to_free_list != NULL) {
		dynamic_node_gc();
	}
}

static u8 *alloc_dynamic_log_blk(u16 log_len, u16 *blk_tag)
{
	if (rtos_is_in_interrupt_context()) {
		return NULL;
	}
	int total_len = log_len + DYM_NODE_SIZE;
	dynamic_log_node *node = (dynamic_log_node *)LOG_MALLOC(total_len);
	if (node == NULL) {
		return NULL;
	}
	node->len = total_len;
	node->next = NULL;
	*blk_tag = MAKE_BLOCK_TAG(0, SHELL_DYM_QUEUE_ID);
	s_dynamic_log_num_in_mem++;
	return &node->ptr[0];
}

static dynamic_log_node *dynamic_list_switch(void)
{
	dynamic_log_node *node;
	node = s_curr_node;
	s_curr_node = s_curr_node->next;
	return node;
}

static inline void dynamic_list_push_back_by_buffer(u8 *packet_buf)
{
	dynamic_log_node *node = (dynamic_log_node *)&packet_buf[-DYM_NODE_SIZE];
	dynamic_list_push_back(node);
}

static void shell_insert_data( const u8 *data, u16 data_len )
{
	u32         int_mask;
	if( !shell_cpu_check_valid() )
		return;

	int_mask = rtos_disable_int();

	log_dev->dev_drv->write_sync(log_dev, (u8 *)data, data_len);

	rtos_enable_int(int_mask);
}

static void output_insert_data(const u8 *data, u16 data_len)
{
	shell_assert_out(bTRUE, "\r\nINSRT:");
	shell_insert_data(data, data_len);
}

static void output_insert_log(u16 buf_len, char *prefix, const char *format, va_list ap)
{
	shell_assert_out(bTRUE, "\r\nINSRT:%s", prefix);
	shell_assert_out_va(bTRUE, format, ap);
}

void print_dynamic_log_info(void)
{
	os_printf("dynamic log info:\n");
	os_printf("mem_queue:%d, mem_comsume:%d\r\n",
				s_dynamic_log_num_in_mem, s_dynamic_log_total_len);
	os_printf("dynamic log mem max: %d\r\n",s_dynamic_log_mem_max);
}
#endif // CONFIG_TUYA_LOG_OPTIMIZATION
