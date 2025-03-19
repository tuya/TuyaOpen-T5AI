#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <string.h>

#include <common/bk_include.h>
#include <components/log.h>

#include <driver/mailbox_channel.h>

#include "media_ipc.h"

#include "test/media_ipc_test.h"

#if (CONFIG_CACHE_ENABLE)
#include "cache.h"
#endif


#define TAG "MIPC"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

media_ipc_info_t *media_ipc_info = NULL;

media_ipc_core_t media_ipc_cpu_id_get(void)
{
	media_ipc_core_t id = 0xFF;

#if (CONFIG_SYS_CPU0)
	id = MIPC_CORE_CPU0;
#endif

#if (CONFIG_SYS_CPU1)
	id = MIPC_CORE_CPU1;
#endif

#if (CONFIG_SYS_CPU2)
	id = MIPC_CORE_CPU2;
#endif

	return id;
}

media_ipc_mailbox_state_t media_ipc_mailbox_state_get(void)
{
	bk_err_t ret = BK_OK;
	uint8_t state = 0;
	uint8_t channel_id = 0;

#if (CONFIG_SYS_CPU0 || CONFIG_SYS_CPU1)
	channel_id  = MB_CHNL_MIPC_SYNC;
#endif

	ret = mb_chnl_ctrl(channel_id, MB_CHNL_GET_STATUS, &state);

	if (ret != BK_OK)
	{
		LOGE("%s, get state error: %X\n", __func__, ret);
		return MIPC_MSTATE_ERROR;
	}

	if (state == 0 /*CHNL_STATE_ILDE*/)
	{
		return MIPC_MSTATE_IDLE;
	}
	else if (state == 1/*CHNL_STATE_BUSY*/)
	{
		return MIPC_MSTATE_BUSY;
	}
	else
	{
		LOGE("%s, get unknow state: %X\n", __func__, state);
	}

	return MIPC_MSTATE_ERROR;
}

static void media_ipc_event_notify(void)
{
	bk_err_t ret = rtos_set_semaphore(&media_ipc_info->event);

	if (ret != BK_OK)
	{
		LOGE("%s, set event failed\n", __func__);
	}
}

media_ipc_data_t *media_ipc_data_pop(LIST_HEADER_T *list)
{
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	LIST_HEADER_T *pos, *n;
	media_ipc_data_t *node = NULL;
	media_ipc_data_t *tmp = NULL;

	if (!list_empty(list))
	{
		list_for_each_safe(pos, n, list)
		{
			tmp = list_entry(pos, media_ipc_data_t, list);
			if (tmp != NULL)
			{
				node = tmp;
				list_del(pos);
				break;
			}
		}
	}
	GLOBAL_INT_RESTORE();

	return node;
}

void media_ipc_data_clear(LIST_HEADER_T *list)
{
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	LIST_HEADER_T *pos, *n;
	media_ipc_data_t *tmp = NULL;

	if (!list_empty(list))
	{
		list_for_each_safe(pos, n, list)
		{
			tmp = list_entry(pos, media_ipc_data_t, list);
			if (tmp != NULL)
			{
				list_del(pos);
				os_free(tmp);
			}
		}
	}
	GLOBAL_INT_RESTORE();
}

bk_err_t media_ipc_list_insert(LIST_HEADER_T *list, LIST_HEADER_T *node)
{
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	list_add_tail(node, list);
	GLOBAL_INT_RESTORE();
	return BK_OK;
}


bk_err_t media_ipc_data_push(LIST_HEADER_T *list, media_ipc_data_t *data)
{
	bk_err_t ret = BK_OK;

	if (data == NULL)
	{
		LOGE("%s media_ipc_data_t NULL\n", __func__);
		return -1;
	}

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	list_add_tail(&data->list, list);
	GLOBAL_INT_RESTORE();
	return ret;
}

media_ipc_handle_t *media_ipc_get_handle_by_name(LIST_HEADER_T *list, char *name)
{
	if (name == NULL || strlen(name) == 0)
	{
		LOGE("%s invalid channel: %s\n", __func__, name);
		return NULL;
	}

	if (list == NULL || list_empty(list))
	{
		LOGD("%s invalid list\n", __func__);
		return NULL;
	}

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	LIST_HEADER_T *pos, *n;
	media_ipc_handle_t *node = NULL;
	media_ipc_handle_t *tmp = NULL;

	list_for_each_safe(pos, n, list)
	{
		tmp = list_entry(pos, media_ipc_handle_t, list);
		if (tmp != NULL && tmp->cfg.name)
		{
			if (0 == strcmp(tmp->cfg.name, name))
			{
				node = tmp;
				break;
			}
		}
	}

	GLOBAL_INT_RESTORE();
	return node;
}


bk_err_t media_ipc_channel_list_remove(LIST_HEADER_T *list, char *name)
{
	bk_err_t ret = BK_FAIL;

	if (name == NULL || strlen(name) == 0)
	{
		LOGE("%s invalid channel: %s\n", __func__, name);
		return ret;
	}

	if (list == NULL || list_empty(list))
	{
		LOGE("%s invalid list\n", __func__);
		return ret;
	}

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	LIST_HEADER_T *pos, *n;
	media_ipc_handle_t *tmp = NULL;

	list_for_each_safe(pos, n, list)
	{
		tmp = list_entry(pos, media_ipc_handle_t, list);
		if (tmp != NULL && tmp->cfg.name)
		{
			if (0 == strcmp(tmp->cfg.name, name))
			{
				list_del(pos);
				ret = BK_OK;
				break;
			}
		}
	}

	GLOBAL_INT_RESTORE();
	return ret;
}


int media_ipc_channel_open(meida_ipc_t *ipc, media_ipc_chan_cfg_t *cfg)
{
	media_ipc_handle_t *handle = NULL;

	if (cfg == NULL)
	{
		LOGE("%s invalid cfg: %p\n", __func__, cfg);
		return -1;
	}

	if (cfg->name == NULL || strlen(cfg->name) == 0)
	{
		LOGE("%s invalid channel: %s\n", __func__, cfg->name);
		return -1;
	}

	if (media_ipc_info == NULL)
	{
		LOGE("%s media ipc not init\n", __func__);
		return -1;
	}

	handle = media_ipc_get_handle_by_name(&media_ipc_info->channel_list, cfg->name);

	if (handle != NULL)
	{
		LOGE("%s channel %s already opened\n", __func__, cfg->name);
		return -1;
	}

	handle = (media_ipc_handle_t *)os_malloc(sizeof(media_ipc_handle_t));

	if (handle == NULL)
	{
		LOGE("%s channel %s malloc failed\n", __func__, cfg->name);
		return -1;
	}

	os_memset(handle, 0, sizeof(media_ipc_handle_t));
	os_memcpy(&handle->cfg, cfg, sizeof(media_ipc_chan_cfg_t));

	handle->cfg.name = os_malloc(strlen(cfg->name) + 1);
	os_memset(handle->cfg.name, 0, strlen(cfg->name) + 1);
	os_memcpy(handle->cfg.name, cfg->name, strlen(cfg->name));

	INIT_LIST_HEAD(&handle->local_list);
	INIT_LIST_HEAD(&handle->remote_list);
	INIT_LIST_HEAD(&handle->free_list);
	handle->ipc = ipc;

	media_ipc_list_insert(&media_ipc_info->channel_list, &handle->list);

	*ipc = handle;

	LOGI("%s success\n", __func__);

	return 0;
}

int media_ipc_channel_close(meida_ipc_t *ipc)
{
	media_ipc_handle_t *handle = (media_ipc_handle_t *)*ipc;

	if (handle == NULL)
	{
		LOGE("%s channel already free\n", __func__);
		return -1;
	}

	if (media_ipc_info == NULL)
	{
		LOGE("%s media ipc not init\n", __func__);
		return -1;
	}

	if (media_ipc_get_handle_by_name(&media_ipc_info->channel_list, handle->cfg.name) == NULL)
	{
		LOGE("%s channel list already list free\n", __func__);
		return -1;
	}

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	*(handle->ipc) = NULL;
	media_ipc_channel_list_remove(&media_ipc_info->channel_list, handle->cfg.name);
	GLOBAL_INT_RESTORE();

	media_ipc_data_clear(&handle->local_list);
	media_ipc_data_clear(&handle->free_list);
	os_free(handle->cfg.name);
	os_free(handle);
	return 0;
}

static int media_ipc_send_async(media_ipc_data_t *ipc_data)
{
	mb_chnl_cmd_t mb_cmd;
	bk_err_t ret = BK_OK;
	uint8_t channel = 0;

	mb_cmd.hdr.cmd = media_ipc_cpu_id_get();
	mb_cmd.param1 = (uint32_t)ipc_data;
	mb_cmd.param2 = 0;
	mb_cmd.param3 = 0;

#if (CONFIG_SYS_CPU0 || CONFIG_SYS_CPU1)
	channel = MB_CHNL_MIPC_SYNC;
#endif


	ret = mb_chnl_write(channel, &mb_cmd);

	rtos_deinit_semaphore(&ipc_data->sem);
	ipc_data->sem = NULL;

	return ret;
}

static inline int media_ipc_send_mailbox(uint8_t flags,
	media_ipc_data_t *ipc_data, uint8_t *data, uint32_t size)
{
	mb_chnl_cmd_t mb_cmd;
	uint8_t channel = 0;
	int ret = BK_OK;

	mb_cmd.hdr.cmd = flags;
	mb_cmd.param1 = (uint32_t)ipc_data;
	mb_cmd.param2 = (uint32_t)data;
	mb_cmd.param3 = size;

#if (CONFIG_SYS_CPU0 || CONFIG_SYS_CPU1)
	channel = MB_CHNL_MIPC_SYNC;
#endif

	ret = rtos_get_semaphore(&media_ipc_info->sem, BEKEN_WAIT_FOREVER);

	if (ret != BK_OK)
	{
		LOGE("%s wait comm semaphore failed\n", __func__);
		return -1;
	}

	return mb_chnl_write(channel, &mb_cmd);
}

static int media_ipc_send_sync(media_ipc_data_t *ipc_data)
{
	bk_err_t ret = BK_OK;

	ret = rtos_init_semaphore_ex(&ipc_data->sem, 1, 0);

	if (ret != BK_OK)
	{
		LOGE("%s init semaphore failed 0x%x\n", __func__, ret);
		goto out;
	}

	ret = media_ipc_send_mailbox(media_ipc_cpu_id_get(), ipc_data, ipc_data->data, ipc_data->size);

	if (ret != BK_OK)
	{
		LOGE("%s write mailbox failed\n", __func__);
		goto out;
	}

	ret = rtos_get_semaphore(&ipc_data->sem, BEKEN_WAIT_FOREVER);

	if (ret != BK_OK)
	{
		LOGE("%s wait local semaphore failed\n", __func__);
		goto out;
	}

out:

	rtos_deinit_semaphore(&ipc_data->sem);
	ipc_data->sem = NULL;

	return ret;
}

int media_ipc_send(meida_ipc_t *ipc, void *data, uint32_t size, uint32_t flags)
{
	bk_err_t ret = BK_OK;
	media_ipc_data_t *ipc_data;

	LOGD("%s %d ++\n", __func__, __LINE__);

	if (media_ipc_info == NULL)
	{
		LOGE("%s ipc not ready\n", __func__);
		return -1;
	}

	ipc_data = (media_ipc_data_t *)os_malloc(sizeof(media_ipc_data_t));

	if (ipc_data == NULL)
	{
		LOGE("%s ipc_data malloc failed\n", __func__);
		return -1;
	}

	os_memset(ipc_data, 0, sizeof(media_ipc_data_t));

	ipc_data->data = data;
	ipc_data->size = size;
	ipc_data->handle = (media_ipc_handle_t *)(*ipc);
	ipc_data->flags = flags;

	if (flags & MIPC_CHAN_SEND_FLAG_SYNC)
	{
		LOGD("%s mailbox send wait +++\n", __func__);
		ret = media_ipc_send_sync(ipc_data);
		LOGD("%s mailbox send wait ---\n", __func__);

		if (ret != BK_OK)
		{
			LOGE("%s ipc send sync failed 0x%x\n", __func__, ret);
			goto out;
		}
	}
	else
	{
		media_ipc_data_push(&ipc_data->handle->local_list, ipc_data);
		media_ipc_event_notify();
	}

out:

	if ((flags & MIPC_CHAN_SEND_FLAG_SYNC) && (ipc_data != NULL))
	{
		os_free(ipc_data);
		ipc_data = NULL;
	}

	LOGD("%s %d --\n", __func__, __LINE__);

	return ret;
}

static void media_ipc_mailbox_rx_isr(void *param, mb_chnl_cmd_t *cmd_buf)
{
	//media_ipc_core_t  id = media_ipc_cpu_id_get();
	media_ipc_data_t *data = (media_ipc_data_t *)cmd_buf->param1;
	media_ipc_handle_t *handle;

#if (CONFIG_CACHE_ENABLE)
    flush_all_dcache();
#endif

	LOGD("%s %d\n", __func__, __LINE__);

	if (data == NULL)
	{
		LOGE("%s, ipc data NULL\n", __func__);
		return;
	}

	handle = data->handle;

	if (data == NULL)
	{
		LOGE("%s, ipc handle NULL\n", __func__);
		return;
	}

	if (cmd_buf->hdr.cmd & MIPC_FLAGS_ACK)
	{
		if (data->flags & MIPC_CHAN_SEND_FLAG_SYNC)
		{
			LOGD("%s set sync sem\n", __func__);
			bk_err_t ret = rtos_set_semaphore(&data->sem);

			if (ret != BK_OK)
			{
				LOGE("%s, set semaphore failed\n", __func__);
			}
		}
		else
		{
			media_ipc_data_push(&handle->free_list, data);
			media_ipc_event_notify();
		}
	}
	else
	{
		media_ipc_handle_t *local_handle = media_ipc_get_handle_by_name(&media_ipc_info->channel_list, handle->cfg.name);

		if (local_handle == NULL)
		{
			LOGE("%s, not register local channel: %s\n", __func__, handle->cfg.name);
			return;
		}

		LOGD("%s got data from CPU %d\n", __func__, cmd_buf->hdr.cmd & 0x3);

		media_ipc_data_push(&local_handle->remote_list, data);
		media_ipc_event_notify();
	}
}


static void media_ipc_mailbox_tx_isr(void *param)
{
}

static void media_ipc_mailbox_tx_cmpl_isr(beken_semaphore_t msg, mb_chnl_ack_t *ack_buf)
{
	LOGD("%s %d\n", __func__, __LINE__);

	if (media_ipc_info)
	{
		rtos_set_semaphore(&media_ipc_info->sem);
	}
}

static void media_ipc_thread_entry(void)
{
	bk_err_t ret = BK_OK;
	media_ipc_data_t *data = NULL;

	media_ipc_info->thread_running = true;

	do
	{
		ret = rtos_get_semaphore(&media_ipc_info->event, 2 * 1000);

		if (ret == kTimeoutErr)
		{
			//LOGE("%s wait event timeout, %d\n", __func__, ret);
			continue;
		}

		LIST_HEADER_T *pos, *n;
		media_ipc_handle_t *handle = NULL;
		LIST_HEADER_T *list = &media_ipc_info->channel_list;

		list_for_each_safe(pos, n, list)
		{
			handle = list_entry(pos, media_ipc_handle_t, list);

			if (handle != NULL)
			{
				data = media_ipc_data_pop(&handle->remote_list);

				if (data)
				{
					if (handle->cfg.cb)
					{
						handle->cfg.cb(data->data, data->size, handle->cfg.param);
					}

					LOGD("send ack\n");
					ret = media_ipc_send_mailbox(media_ipc_cpu_id_get() | MIPC_FLAGS_ACK, data, data->data, data->size);

					if (ret != BK_OK)
					{
						LOGE("%s send mailbox ack failed\n", __func__);
					}
				}

				data = media_ipc_data_pop(&handle->local_list);

				if (data)
				{
					ret = media_ipc_send_mailbox(media_ipc_cpu_id_get(), data, data->data, data->size);

					if (ret != BK_OK)
					{
						LOGE("%s send async mailbox message to cpu %d failed\n", __func__, media_ipc_cpu_id_get());
					}
				}

				data = media_ipc_data_pop(&handle->free_list);

				if (data)
				{
					os_free(data);
					data = NULL;
				}
			}
		}
	}
	while (media_ipc_info->thread_running);

	media_ipc_info->thread = NULL;
	rtos_delete_thread(NULL);
}

int media_ipc_init(void)
{
	bk_err_t ret = BK_OK;

	if (media_ipc_info)
	{
		LOGE("%s already init\n", __func__);
		return -1;
	}

	media_ipc_info = (media_ipc_info_t *)os_malloc(sizeof(media_ipc_info_t));

	if (media_ipc_info == NULL)
	{
		LOGE("%s media_ipc_info malloc failed\n", __func__);
		goto out;
	}

	os_memset(media_ipc_info, 0, sizeof(media_ipc_info_t));

	media_ipc_info->state = MIPC_MSTATE_IDLE;
	INIT_LIST_HEAD(&media_ipc_info->channel_list);

	ret = rtos_init_semaphore_ex(&media_ipc_info->sem, 1, 1);

	if (ret != BK_OK)
	{
		LOGE("%s init semaphore failed 0x%x\n", __func__, ret);
		goto out;
	}

	ret = rtos_init_semaphore_ex(&media_ipc_info->event, 2048, 0);

	if (ret != BK_OK)
	{
		LOGE("%s init event failed 0x%x\n", __func__, ret);
		goto out;
	}


#if (CONFIG_SYS_CPU0 || CONFIG_SYS_CPU1)
	LOGI("open channel: %d on CPU: %d\n", MB_CHNL_MIPC_SYNC, media_ipc_cpu_id_get());
	mb_chnl_open(MB_CHNL_MIPC_SYNC, media_ipc_info);
	mb_chnl_ctrl(MB_CHNL_MIPC_SYNC, MB_CHNL_SET_RX_ISR, media_ipc_mailbox_rx_isr);
	mb_chnl_ctrl(MB_CHNL_MIPC_SYNC, MB_CHNL_SET_TX_ISR, media_ipc_mailbox_tx_isr);
	mb_chnl_ctrl(MB_CHNL_MIPC_SYNC, MB_CHNL_SET_TX_CMPL_ISR, media_ipc_mailbox_tx_cmpl_isr);
#endif

	ret = rtos_create_thread(&media_ipc_info->thread,
								9,
								"ipc thread",
								(beken_thread_function_t)media_ipc_thread_entry,
								8192,
								NULL);


#if MEDIA_IPC_UT_TEST
	media_ipc_test_init();
#endif
	return ret;

out:

	if (media_ipc_info)
	{
		os_free(media_ipc_info);
		media_ipc_info = NULL;
	}

	return -1;
}

int media_ipc_deinit(void)
{
	return 0;
}

// Modified by TUYA Start
int media_ipc_status(void)
{
    return media_ipc_info->thread_running;
}
// Modified by TUYA End

