#include "mbox0_drv.h"
#include "sys_driver.h"
#include "mbox0_fifo_cfg.h"

static mbox0_dev_t       mbox0_dev;
static u8                mbox0_init = 0;

static mbox0_fifo_cfg_t  fifo_cfg[] = MBOX0_FIFO_CFG_TABLE;
	
static int mbox0_drv_cfg_fifo(uint8_t fifo_start, uint8_t fifo_length)
{
	return mbox0_dev.chn_drv[SELF_CHNL]->chn_cfg_fifo(&mbox0_dev.hal, fifo_start, fifo_length);
}

static int mbox0_drv_recieve_message(mbox0_message_t* message)
{
	return mbox0_dev.chn_drv[SELF_CHNL]->chn_recv(&mbox0_dev.hal, message);
}

static void mbox0_drv_isr_handler(void)
{
	uint32_t int_status;
	uint32_t fifo_status;
	mbox0_message_t message;

	int_status = mbox0_dev.chn_drv[SELF_CHNL]->chn_get_int_status(&mbox0_dev.hal);

	if(int_status == 0)
		return;

	do
	{
		fifo_status = mbox0_dev.chn_drv[SELF_CHNL]->chn_get_rx_fifo_stat(&mbox0_dev.hal);
		
		if(fifo_status & RX_FIFO_STAT_NOT_EMPTY)
		{
			mbox0_drv_recieve_message(&message);

			if(mbox0_dev.rx_callback != NULL)
				mbox0_dev.rx_callback(&message);
		}
		else
		{
			break;
		}
	} while(1);
	
}

int mbox0_drv_get_send_stat(uint32_t dest_cpu)
{
	uint32_t fifo_status;

	if(dest_cpu >= MBOX_CHNL_NUM)
	{
		return MBOX0_HAL_SW_PARAM_ERR;
	}

	fifo_status = mbox0_dev.chn_drv[dest_cpu]->chn_get_rx_fifo_stat(&mbox0_dev.hal);

	if(RX_FIFO_STAT_FULL & fifo_status)
	{
		return MBOX0_HAL_SW_FIFO_FULL;
	}

	return MBOX0_HAL_OK;
}

int mbox0_drv_send_message(mbox0_message_t* message)
{
	return mbox0_dev.chn_drv[SELF_CHNL]->chn_send(&mbox0_dev.hal, message);
}

int mbox0_drv_callback_register(mbox0_rx_callback_t callback)
{
	mbox0_dev.rx_callback = callback;

	return 0;
}

int mbox0_drv_init(void)
{
	if(mbox0_init == 1)
		return 0;

	mbox0_dev.chn_drv[0] = &hal_chn0_drv;
	mbox0_dev.chn_drv[1] = &hal_chn1_drv;
	mbox0_dev.chn_drv[2] = &hal_chn2_drv;

	mbox0_dev.rx_callback = NULL;

	mbox0_hal_init(&mbox0_dev.hal);

	if(SELF_CHNL == 0)
	{
		mbox0_hal_dev_init(&mbox0_dev.hal);  /* can only be initialized by CPU0. */
		mbox0_hal_set_reg_0x2_chn_pro_disable(&mbox0_dev.hal, 1); /* channel unprotect*/
	}

	mbox0_drv_cfg_fifo(fifo_cfg[SELF_CHNL].start, fifo_cfg[SELF_CHNL].len);

	/* clear/empty RX FIFO. */
	while(1)
	{
		mbox0_message_t message;

		uint32_t fifo_status = mbox0_dev.chn_drv[SELF_CHNL]->chn_get_rx_fifo_stat(&mbox0_dev.hal);
		
		if(fifo_status & RX_FIFO_STAT_NOT_EMPTY)
		{
			mbox0_drv_recieve_message(&message);
		}
		else
		{
			break;
		}
	}

	int int_src = INT_SRC_MAILBOX;
	
	bk_int_isr_register(int_src, mbox0_drv_isr_handler, NULL);

	mbox0_dev.chn_drv[SELF_CHNL]->chn_int_enable(&mbox0_dev.hal, 1);
	
	sys_drv_int_group2_enable(1 << (int_src - 32));

	mbox0_init = 1;

	return 0;
}

int mbox0_drv_deinit(void)
{
	if(mbox0_init == 0)
		return -1;

	uint8_t int_src = INT_SRC_MAILBOX;

	sys_drv_int_group2_disable(1 << (int_src - 32));
	
	mbox0_dev.chn_drv[SELF_CHNL]->chn_int_enable(&mbox0_dev.hal, 0);

	bk_int_isr_unregister(int_src);

	if(SELF_CHNL == 0)
		mbox0_hal_dev_deinit(&mbox0_dev.hal);  /* can only be de-initialized by CPU0. */

	mbox0_hal_deinit(&mbox0_dev.hal);
	
	mbox0_dev.rx_callback = NULL;

	mbox0_init = 0;

	return 0;
}

