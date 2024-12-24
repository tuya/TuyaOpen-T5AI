#include <string.h>
#include "mbox0_adapter.h"

enum
{
	TX_CMD_BOX = 0,
	TX_ACK_BOX,
	TX_BOX_NUM,
} ;

static mailbox_data_t	mailbox_buff[MBOX_CHNL_NUM][TX_BOX_NUM];
static mailbox_callback_t  mailbox_callback[MBOX_CHNL_NUM] = {0};
static uint8_t	mailbox_tx_box[MBOX_CHNL_NUM] = {0};

static void mbox0_rx_isr(mbox0_message_t *msg)
{
	mailbox_data_t data;
	u32 chnl = msg->src_cpu;

	if(chnl >= MBOX_CHNL_NUM)
		return;

	if(msg->data[1] == sizeof(data))
	{
#if CONFIG_CACHE_ENABLE
	  	extern void flush_dcache(void *va, long size);
		flush_dcache((void *)msg->data[0], sizeof(data));
#endif
		memcpy(&data, (u8 *)msg->data[0], sizeof(data));
	}
	else
	{
		return;
	}

	if(mailbox_callback[chnl] != NULL)
		mailbox_callback[chnl](&data);
}

bk_err_t bk_mailbox_init(void)
{
	int ret_code;

	ret_code = mbox0_drv_init();
	if(ret_code != 0)
		return BK_FAIL;

	ret_code = mbox0_drv_callback_register(mbox0_rx_isr);

	return ret_code;
}

bk_err_t bk_mailbox_deinit(void)
{
	int ret_code;

	ret_code = mbox0_drv_deinit();
	if(ret_code != 0)
		return BK_FAIL;

	return BK_OK;
}

bk_err_t bk_mailbox_ready(mailbox_endpoint_t src, mailbox_endpoint_t dst, uint32_t box_id)
{
	if(dst >= MBOX_CHNL_NUM)
		return BK_ERR_MAILBOX_SRC_DST;

	return BK_OK;

}

bk_err_t bk_mailbox_send(mailbox_data_t *data, mailbox_endpoint_t src, mailbox_endpoint_t dst, void *arg)
{
	mbox0_message_t message;

	if(dst >= MBOX_CHNL_NUM)
		return BK_ERR_MAILBOX_SRC_DST;

	(void)arg;

	/*
	int box = TX_CMD_BOX;

	if(arg != NULL)
	{
		box = (*(uint16_t *)arg) & 0xFF;
	}

	if(box >= TX_BOX_NUM)
		return BK_ERR_MAILBOX_ID;
	*/

	int ret_code = mbox0_drv_get_send_stat((uint32_t)dst);

	if(ret_code != MBOX0_HAL_OK)
	{
		if(ret_code == MBOX0_HAL_SW_FIFO_FULL)
			return BK_ERR_MAILBOX_TIMEOUT;  // mailbox busy

		if(ret_code == MBOX0_HAL_SW_PARAM_ERR)
			return BK_ERR_MAILBOX_SRC_DST;

		return BK_ERR_MAILBOX_NOT_INIT;
	}

	mailbox_tx_box[dst] = (mailbox_tx_box[dst] + 1) % TX_BOX_NUM;

	int box = mailbox_tx_box[dst];

	if(data != NULL)
	{
		memcpy(&mailbox_buff[dst][box], data, sizeof(mailbox_data_t));
		
		message.data[0] = (u32)(&mailbox_buff[dst][box]);
		message.data[1] = sizeof(mailbox_data_t);
	}
	else
	{
		message.data[0] = 0;
		message.data[1] = 0;
	}

	message.dest_cpu = (uint8_t)dst;

	ret_code = mbox0_drv_send_message(&message);

	if(ret_code != MBOX0_HAL_OK)
		return BK_ERR_MAILBOX_NOT_INIT;

	return BK_OK;
}

bk_err_t bk_mailbox_recv_callback_register(mailbox_endpoint_t src, mailbox_endpoint_t dst, mailbox_callback_t callback)
{
	if(src >= MBOX_CHNL_NUM)
		return BK_ERR_MAILBOX_SRC_DST;

	mailbox_callback[src] = callback;
	
	return BK_OK;
}

bk_err_t bk_mailbox_recv_callback_unregister(mailbox_endpoint_t src, mailbox_endpoint_t dst)
{
	if(src >= MBOX_CHNL_NUM)
		return BK_ERR_MAILBOX_SRC_DST;

	mailbox_callback[src] = NULL;

	return BK_OK;
}

