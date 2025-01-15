
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

#if CONFIG_AT
#include "atsvr_unite.h"
#if CONFIG_AT_DATA_MODE
#include "../../../components/at/inc/at_sal_ex.h"
#endif
#endif

#if 0//def CONFIG_SYS_CPU0

const uart_config_t at_config  = 
{
	.baud_rate = UART_BAUD_RATE,
	.data_bits = UART_DATA_8_BITS,
	.parity = UART_PARITY_NONE,
	.stop_bits = UART_STOP_BITS_1,
	.flow_ctrl = UART_FLOWCTRL_DISABLE,
	.src_clk = UART_SCLK_XTAL_26M,

};
#endif
//#define ATSVR_TAG       "atsvr"

#define DEV_UART        1
#define DEV_MAILBOX     2

#if CONFIG_SYS_PRINT_DEV_UART
#define CMD_DEV    DEV_UART
#elif CONFIG_SYS_PRINT_DEV_MAILBOX
#define CMD_DEV    DEV_MAILBOX
#endif

#if (CONFIG_SYS_CPU0)
#if CONFIG_MAILBOX
#define FWD_CMD_TO_SLAVE
#define RECV_CMD_LOG_FROM_SLAVE
#endif
#endif

#if defined(FWD_CMD_TO_SLAVE)
#define SLAVE_RSP_BLK_ID		0
#define SLAVE_IND_BLK_ID		1
#endif

#define ATSVR_WAIT_OUT_TIME		(2000) 		// 2s.

#define ATSVR_TASK_WAIT_TIME	(100)		// 100ms
#define ATSVR_TASK_WAKE_CYCLE	(20)		// 2s

#define ATSVR_EVENT_RX_IND  	0x02

#if (CMD_DEV == DEV_MAILBOX)
#define ATSVR_RX_BUF_LEN		140
#endif
#if (CMD_DEV == DEV_UART)
#define ATSVR_RX_BUF_LEN		4
#endif

#if CONFIG_AT_DATA_MODE
#define ATSVR_CMD_BUF_LEN		4096
#else
#define ATSVR_CMD_BUF_LEN		200
#endif
#define ATSVR_RSP_BUF_LEN		140
#define ATSVR_IND_BUF_LEN		132

#define ATSVR_RSP_QUEUE_ID	    (7)
#define ATSVR_FWD_QUEUE_ID      (8)
#define ATSVR_IND_QUEUE_ID		(10)

enum
{
	CMD_TYPE_TEXT = 0,
	CMD_TYPE_INVALID,
};

typedef struct
{
	u8     rsp_buff[ATSVR_RSP_BUF_LEN];
	beken_semaphore_t   rsp_buf_semaphore;

	u8     rx_buff[ATSVR_RX_BUF_LEN];

	u8     cur_cmd_type;
	u8     cmd_buff[ATSVR_CMD_BUF_LEN];
	u16    cmd_data_len;


	u8     echo_enable;

	/* patch for AT cmd handling. */
	u8     cmd_ind_buff[ATSVR_IND_BUF_LEN];
	beken_semaphore_t   ind_buf_semaphore;
} cmd_line_t;

#define GET_BLOCK_ID(blocktag)          ((blocktag) & 0xFF)
#define GET_QUEUE_ID(blocktag)          (((blocktag) & 0x0F00) >> 8)
#define MAKE_BLOCK_TAG(blk_id, q_id)    (((blk_id) & 0xFF) | (((q_id) & 0x0F) << 8) )

static cmd_line_t  cmd_line_buf;


#if (CMD_DEV == DEV_UART)
#if (CONFIG_UART_PRINT_PORT == AT_UART_PORT_CFG)
extern shell_dev_t	   shell_uart;
static shell_dev_t * cmd_dev = &shell_uart;
#else
extern shell_dev_t	   atsvr_shell_uart;
static shell_dev_t * cmd_dev = &atsvr_shell_uart;

#endif
#endif
#if (CMD_DEV == DEV_MAILBOX)
//#if (0 == CONFIG_DEFAULT_AT_PORT)
//extern shell_dev_t     shell_dev_mb;
//static shell_dev_t * cmd_dev = &shell_dev_mb;
//#else
extern shell_dev_t     atsvr_shell_dev_mb;
static shell_dev_t * cmd_dev = &atsvr_shell_dev_mb;
//#endif
#endif


#if defined(FWD_CMD_TO_SLAVE) || defined(RECV_CMD_LOG_FROM_SLAVE)

typedef struct
{
	log_cmd_t	rsp_buf;
	log_cmd_t   ind_buf;
} fwd_slave_data_t;

static fwd_slave_data_t  ipc_fwd_data;
extern shell_dev_ipc_t atsvr_shell_dev_ipc;
static shell_dev_ipc_t * ipc_dev = &atsvr_shell_dev_ipc;

static int result_fwd(int blk_id);
static u32 atsvr_ipc_rx_indication(u16 cmd, log_cmd_t *data, u16 cpu_id);

#endif

static inline uint32_t atsvr_task_enter_critical()
{
	return rtos_enter_critical();
}

static inline void atsvr_task_exit_critical(uint32_t flags)
{
	rtos_exit_critical(flags);
}

static const char    atsvr_prompt_str[] = "\r\n>";

static u8     atsvr_init_ok = bFALSE;

static u32    atsvr_pm_wake_time = ATSVR_TASK_WAKE_CYCLE;   // wait cycles before enter sleep.
static u8     atsvr_pm_wake_flag = 1;

#if 0

typedef struct
{
	beken_semaphore_t   event_semaphore;  // will release from ISR.
	u32       event_flag;
} os_ext_event_t;

static os_ext_event_t   atsvr_task_event;

static bool_t atsvr_create_event(void)
{
	atsvr_task_event.event_flag = 0;
	rtos_init_semaphore(&atsvr_task_event.event_semaphore, 1);

	return bTRUE;
}

/* this API may be called from ISR. */
bool_t atsvr_set_event(u32 event_flag)
{
	u32  int_mask;

	int_mask = atsvr_task_enter_critical();

	atsvr_task_event.event_flag |= event_flag;

	atsvr_task_exit_critical(int_mask);

	rtos_set_semaphore(&atsvr_task_event.event_semaphore);

	return bTRUE;
}

u32 atsvr_wait_any_event(u32 timeout)
{
	u32  int_mask;
	u32  event_flag;

	int  result;

	while(bTRUE)
	{
		int_mask = atsvr_task_enter_critical();

		event_flag = atsvr_task_event.event_flag;
		atsvr_task_event.event_flag = 0;

		atsvr_task_exit_critical(int_mask);

		if((event_flag != 0) || (timeout == 0))
		{
			return event_flag;
		}
		else
		{
			result = rtos_get_semaphore(&atsvr_task_event.event_semaphore, timeout);

			if(result == kTimeoutErr)
				return 0;
		}
	}

}

#else

static beken_semaphore_t   atsvr_semaphore;  // will release from ISR.

static bool_t atsvr_create_event(void)
{
	rtos_init_semaphore(&atsvr_semaphore, 1);

	return bTRUE;
}

static bool_t atsvr_set_event(u32 event_flag)
{
	(void)event_flag;

	rtos_set_semaphore(&atsvr_semaphore);

	return bTRUE;
}

static u32 atsvr_wait_any_event(u32 timeout)
{
	int result;

	result = rtos_get_semaphore(&atsvr_semaphore, timeout);

	if(result == kTimeoutErr)
		return 0;
	
	return ATSVR_EVENT_RX_IND;
}
#endif


/* call from cmd TX ISR. */
static int cmd_tx_complete(u8 *pbuf, u16 buf_tag)
{
	u8      queue_id = GET_QUEUE_ID(buf_tag);
	u16     blk_id = GET_BLOCK_ID(buf_tag);

	/* rsp ok ?? */
	if( queue_id == ATSVR_RSP_QUEUE_ID )    /* rsp. */
	{
		/* it is called from cmd_dev tx ISR. */

		if ( (pbuf != cmd_line_buf.rsp_buff) || (blk_id != 0) )
		{
			/* something wrong!!! */
			ATSVRLOGE("FAULT: in rsp.\r\n");
		}

		/* rsp compelete, rsp_buff can be used for next cmd/response. */
		rtos_set_semaphore(&cmd_line_buf.rsp_buf_semaphore);

		return 1;
	}

	if( queue_id == ATSVR_IND_QUEUE_ID )    /* cmd_ind. */
	{
		/* it is called from cmd_dev tx ISR. */

		if ( (pbuf != cmd_line_buf.cmd_ind_buff) || (blk_id != 0) )
		{
			/* something wrong!!! */
			ATSVRLOGE("FAULT: indication.\r\n");
		}

		/* indication tx compelete, cmd_ind_buff can be used for next cmd_indication. */
		rtos_set_semaphore(&cmd_line_buf.ind_buf_semaphore);

		return 1;
	}

	if( queue_id == ATSVR_FWD_QUEUE_ID )    /* slave buffer. */
	{
		#if defined(FWD_CMD_TO_SLAVE)
		result_fwd(blk_id);
		return 1;
		#endif
	}

	return 0;
}

/* call from TX ISR. */
static void atsvr_cmd_tx_complete(u8 *pbuf, u16 buf_tag)
{
	u32  int_mask = atsvr_task_enter_critical();
	int  tx_handled = cmd_tx_complete(pbuf, buf_tag);

	if(tx_handled == 0)  /* not handled. */
	{
		/*        FAULT !!!!      */
		ATSVRLOGE("FATAL:%x,\r\n", buf_tag);
	}

	atsvr_task_exit_critical(int_mask);
}

/* call from RX ISR. */
static void atsvr_rx_indicate(void)
{
	atsvr_set_event(ATSVR_EVENT_RX_IND);

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
	u32  int_mask = atsvr_task_enter_critical();

	/* should have a count semaphore for write_asyn calls for rsp/ind/cmd_hint & slave rsp/ind. *
	 * otherwise there will be coupled with driver, drv tx_queue_len MUST be >= 5. */
	cmd_dev->dev_drv->write_async(cmd_dev, msg_buf, msg_len, blk_tag);

	atsvr_task_exit_critical(int_mask);
}

/*    NOTICE:  this can only be called by shell task internally (cmd handler). */
/*             it is not a re-enterance function becaue of using rsp_buff. */
static bool_t cmd_rsp_out(u8 * rsp_msg, u16 msg_len)
{
	u16    rsp_blk_tag = MAKE_BLOCK_TAG(0, ATSVR_RSP_QUEUE_ID);

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
	u16    ind_blk_tag = MAKE_BLOCK_TAG(0, ATSVR_IND_QUEUE_ID);

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
extern int get_data_len(void);
static void rx_ind_process(void)
{
	u16   read_cnt, buf_len;
	u8    cmd_rx_done = bFALSE;
	#if CONFIG_SYS_CPU0
	u16   echo_len;
	u16   i = 0;
	u8    need_backspace = bFALSE;
	#if CONFIG_AT_DATA_MODE
	int   at_data_len = 0;
	#endif
	#endif

	if(cmd_dev->dev_type == SHELL_DEV_MAILBOX)
	{
		buf_len = ATSVR_RX_BUF_LEN;
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
#if CONFIG_SYS_CPU1
		memcpy(cmd_line_buf.cmd_buff,cmd_line_buf.rx_buff,read_cnt);
		cmd_line_buf.cmd_data_len = read_cnt;
		cmd_line_buf.cur_cmd_type = CMD_TYPE_TEXT;
		cmd_rx_done = 1;
		goto cpu1_handle;
#endif
#if CONFIG_SYS_CPU0
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

		}

		if( cmd_rx_done )
		{
			if(cmd_line_buf.echo_enable)
			{
				echo_out(&rx_temp_buff[0], echo_len);
				echo_out((u8 *)"\r\n", 2);
			}

			break;
		}
		else
		{
			if(cmd_line_buf.echo_enable)
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
						echo_out((u8 *)&atsvr_prompt_str[0], 3);
						echo_len = 0;
					}
				}
				
				echo_out(rx_temp_buff, echo_len);
			}
		}

		if(read_cnt < buf_len) /* all data are read out. */
			break;
#endif
	}

	if(read_cnt < buf_len) /* all data are read out. */
	{
	}
	else  /* cmd pends in buffer, handle it in new loop cycle. */
	{
		atsvr_set_event(ATSVR_EVENT_RX_IND);
	}
#if CONFIG_SYS_CPU1
cpu1_handle:
#endif
	/* can re-use *buf_len*. */
	if( cmd_rx_done )
	{
	#if CONFIG_SYS_CPU0
		if(cmd_line_buf.cur_cmd_type == CMD_TYPE_TEXT)
		{
	#endif
			u16		rx_ovf = 0;
			cmd_dev->dev_drv->io_ctrl(cmd_dev, SHELL_IO_CTRL_GET_RX_STATUS, &rx_ovf);

			if(rx_ovf != 0)
			{
				ATSVRLOGE("AT Commands lost!\r\n");
			}

			rtos_get_semaphore(&cmd_line_buf.rsp_buf_semaphore, ATSVR_WAIT_OUT_TIME);

			cmd_line_buf.rsp_buff[0] = 0;

			/* handle command. */
			if( cmd_line_buf.cmd_data_len > 0 )
				//atsvr_handle_shell_input( (char *)cmd_line_buf.cmd_buff, cmd_line_buf.cmd_data_len, (char *)cmd_line_buf.rsp_buff, ATSVR_RSP_BUF_LEN - 4 );
				atsvr_msg_get_input((char *)cmd_line_buf.cmd_buff, cmd_line_buf.cmd_data_len, (char *)cmd_line_buf.rsp_buff, ATSVR_RSP_BUF_LEN - 4);

			cmd_line_buf.rsp_buff[ATSVR_RSP_BUF_LEN - 4] = 0;

			buf_len = strlen((char *)cmd_line_buf.rsp_buff);
			if(buf_len > (ATSVR_RSP_BUF_LEN - 4))
				buf_len = (ATSVR_RSP_BUF_LEN - 4);
			buf_len += sprintf((char *)&cmd_line_buf.rsp_buff[buf_len], &atsvr_prompt_str[0]);

			cmd_rsp_out(cmd_line_buf.rsp_buff, buf_len);
	#if CONFIG_SYS_CPU0
		}
	#endif

		cmd_line_buf.cur_cmd_type = CMD_TYPE_INVALID;  /* reset cmd line to interpret new cmd. */
		cmd_line_buf.cmd_data_len = 0;
	}

	return;
}

extern gpio_id_t bk_uart_get_rx_gpio(uart_id_t id);

static void atsvr_rx_wakeup(int gpio_id);

static void atsvr_power_save_enter(void)
{
	//u32		flush_log = cmd_line_buf.log_flush;

	cmd_dev->dev_drv->io_ctrl(cmd_dev, SHELL_IO_CTRL_RX_SUSPEND, NULL);

	if(cmd_dev->dev_type == SHELL_DEV_UART)
	{
		u8   uart_port = UART_ID_MAX;

		cmd_dev->dev_drv->io_ctrl(cmd_dev, SHELL_IO_CTRL_GET_UART_PORT, &uart_port);
		u32  gpio_id = bk_uart_get_rx_gpio(uart_port);

		bk_gpio_register_isr(gpio_id, (gpio_isr_t)atsvr_rx_wakeup);
	}
}

static void atsvr_power_save_exit(void)
{
	cmd_dev->dev_drv->io_ctrl(cmd_dev, SHELL_IO_CTRL_RX_RESUME, NULL);
}

static void wakeup_process(void)
{
	bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_AT, 0, 0);
	atsvr_pm_wake_flag = 1;
	atsvr_pm_wake_time = ATSVR_TASK_WAKE_CYCLE;
}

static void atsvr_rx_wakeup(int gpio_id)
{
	wakeup_process();

	ATSVRLOG("ATSVR_wakeup\r\n");

	if(cmd_dev->dev_type == SHELL_DEV_UART)
	{
		bk_gpio_register_isr(gpio_id, NULL);
	}
}

static void atsvr_task_init(void)
{
	//u16		i;

	cmd_line_buf.cur_cmd_type = CMD_TYPE_INVALID;
	cmd_line_buf.cmd_data_len = 0;
	cmd_line_buf.echo_enable = bTRUE;

	rtos_init_semaphore_ex(&cmd_line_buf.rsp_buf_semaphore, 1, 1);  // one buffer for cmd_rsp.
	rtos_init_semaphore_ex(&cmd_line_buf.ind_buf_semaphore, 1, 1);  // one buffer for cmd_ind.

	atsvr_create_event();

	cmd_dev->dev_drv->init(cmd_dev);

	cmd_dev->dev_drv->open(cmd_dev, atsvr_cmd_tx_complete, atsvr_rx_indicate); // rx cmd, tx rsp.

	#if defined(FWD_CMD_TO_SLAVE) || defined(RECV_CMD_LOG_FROM_SLAVE)
	ipc_dev->dev_drv->init(ipc_dev);
	ipc_dev->dev_drv->open(ipc_dev, (shell_ipc_rx_t)atsvr_ipc_rx_indication, NULL);   /* register rx-callback to copy log data to buffer. */
	#endif

	atsvr_init_ok = bTRUE;

	pm_cb_conf_t enter_config;
	enter_config.cb = (pm_cb)atsvr_power_save_enter;
	enter_config.args = NULL;

	pm_cb_conf_t exit_config;
	exit_config.cb = (pm_cb)atsvr_power_save_exit;
	exit_config.args = NULL;

	#if 1
	bk_pm_sleep_register_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_UART2, &enter_config, &exit_config);

	u8   uart_port = UART_ID_MAX;

	cmd_dev->dev_drv->io_ctrl(cmd_dev, SHELL_IO_CTRL_GET_UART_PORT, &uart_port);

	atsvr_rx_wakeup(bk_uart_get_rx_gpio(uart_port));
	#else

	u8 uart_port = UART_ID_MAX;
	cmd_dev->dev_drv->io_ctrl(cmd_dev, SHELL_IO_CTRL_GET_UART_PORT, &uart_port);

	u8 pm_uart_port = uart_id_to_pm_uart_id(uart_port);
	bk_pm_sleep_register_cb(PM_MODE_LOW_VOLTAGE, pm_uart_port, &enter_config, &exit_config);

	atsvr_rx_wakeup(bk_uart_get_rx_gpio(uart_port));
	#endif


}

void atsvr_task( void *para )
{
	u32    Events;
	u32    timeout = ATSVR_TASK_WAIT_TIME;

	atsvr_task_init();

	echo_out((u8 *)&atsvr_prompt_str[0], 3);

	atsvr_notice_ready();

	while(bTRUE)
	{
		Events = atsvr_wait_any_event(timeout);  // WAIT_EVENT;


		if(Events & ATSVR_EVENT_RX_IND)
		{
			wakeup_process();
			rx_ind_process();
		}

		if(Events == 0)
		{
			if(atsvr_pm_wake_time > 0)
				atsvr_pm_wake_time--;

			if(atsvr_pm_wake_time == 0)
			{
				if(atsvr_pm_wake_flag != 0)
				{
					atsvr_pm_wake_flag = 0;
					bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_AT, 1, 0);
					ATSVRLOG("ATSVR sleep\r\n");
				}
			}
		}

		if(atsvr_pm_wake_flag)
		{
			timeout = ATSVR_TASK_WAIT_TIME;
		}
		else
		{
			timeout = BEKEN_WAIT_FOREVER;
		}
	}
}

#if defined(FWD_CMD_TO_SLAVE) || defined(RECV_CMD_LOG_FROM_SLAVE)
static int cmd_rsp_fwd(u8 * rsp_msg, u16 msg_len)
{
	u16    rsp_blk_tag = MAKE_BLOCK_TAG(SLAVE_RSP_BLK_ID, ATSVR_FWD_QUEUE_ID);

	cmd_info_out(rsp_msg, msg_len, rsp_blk_tag);

	return 1;
}

static int cmd_ind_fwd(u8 * ind_msg, u16 msg_len)
{
	u16    ind_blk_tag = MAKE_BLOCK_TAG(SLAVE_IND_BLK_ID, ATSVR_FWD_QUEUE_ID);
	
	if(0 == strcmp(ATSVR_READY_MSG,(char*)(ind_msg)))
	{
		extern _at_svr_ctrl_env_t _at_svr_env;
		_at_svr_env.cpu1_ready = true;
		ATSVRLOGE("ATSVR:CPU0 Notice CPU1 is ready\r\n");
	}
	cmd_info_out(ind_msg, msg_len, ind_blk_tag);

	return 1;
}

static int result_fwd(int blk_id)
{
	log_cmd_t   * log_cmd;

	if(blk_id == SLAVE_RSP_BLK_ID)
	{
		log_cmd = &ipc_fwd_data.rsp_buf;
	}
	else if(blk_id == SLAVE_IND_BLK_ID)
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

static u32 atsvr_ipc_rx_indication(u16 cmd, log_cmd_t *log_cmd, u16 cpu_id)
{
	u32   result = ACK_STATE_FAIL;
	u8  * data = log_cmd->buf;
	u16   data_len = log_cmd->len;
	if(cmd == MB_CMD_LOG_OUT)
	{
		u8      queue_id = GET_QUEUE_ID(log_cmd->tag);

		if(queue_id == ATSVR_RSP_QUEUE_ID)
		{
			memcpy(&ipc_fwd_data.rsp_buf, log_cmd, sizeof(ipc_fwd_data.rsp_buf));
			cmd_rsp_fwd(data, data_len);

			return ACK_STATE_PENDING;
		}
		else if(queue_id == ATSVR_IND_QUEUE_ID)
		{
			memcpy(&ipc_fwd_data.ind_buf, log_cmd, sizeof(ipc_fwd_data.ind_buf));
			cmd_ind_fwd(data, data_len);

			return ACK_STATE_PENDING;
		}
	}

	/* no cmd handler. */
	return result;
}

int atsvr_cmd_forward(char *cmd, u16 cmd_len)
{
	mb_chnl_cmd_t	mb_cmd_buf;
	user_cmd_t * user_cmd = (user_cmd_t *)&mb_cmd_buf;

	user_cmd->hdr.data = 0;
	user_cmd->hdr.cmd = MB_CMD_USER_INPUT;
	user_cmd->buf = (u8 *)cmd;
	user_cmd->len = cmd_len;

	u32  int_mask = atsvr_task_enter_critical();

	int ret_code = ipc_dev->dev_drv->write_cmd(ipc_dev, &mb_cmd_buf);

	atsvr_task_exit_critical(int_mask);

	return ret_code;
}
#endif

void atsvr_echo_set(int en_flag)
{
	if(en_flag != 0)
		cmd_line_buf.echo_enable = bTRUE;
	else
		cmd_line_buf.echo_enable = bFALSE;
}

int atsvr_echo_get(void)
{
	if(cmd_line_buf.echo_enable)
		return 1;

	return 0;
}

void atsvr_cmd_ind_out(const char *format, ...)
{
	u16   data_len, buf_len = ATSVR_IND_BUF_LEN;
	va_list  arg_list;
	rtos_get_semaphore(&cmd_line_buf.ind_buf_semaphore, ATSVR_WAIT_OUT_TIME);
	va_start(arg_list, format);
	data_len = vsnprintf( (char *)&cmd_line_buf.cmd_ind_buff[0], buf_len - 1, format, arg_list );
	va_end(arg_list);

	if(data_len >= buf_len)
		data_len = buf_len - 1;

	cmd_ind_out(cmd_line_buf.cmd_ind_buff, data_len);
}

int atsvr_get_cpu_id(void)
{
#ifdef CONFIG_FREERTOS_SMP
	return rtos_get_core_id();
#elif (CONFIG_CPU_CNT > 1)

	if(cmd_dev->dev_type == SHELL_DEV_MAILBOX)
		return SELF_CPU;

#endif

	return -1;
}


