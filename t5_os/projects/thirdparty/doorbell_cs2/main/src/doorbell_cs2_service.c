#include <common/bk_include.h>

#include "lwip/tcp.h"
#include "bk_uart.h"
#include <os/mem.h>
#include <os/os.h>
#include <os/str.h>
#include <common/bk_kernel_err.h>
#include <components/video_types.h>
//#include "av_audio.h"
#include "aud_intf.h"
//#include "av_audio_common.h"

#include "lwip/sockets.h"

#include "lcd_act.h"

#include "media_app.h"
#include <stdlib.h>

#include "doorbell_comm.h"
#include "doorbell_transmission.h"
#include "doorbell_cs2_service.h"
#include "doorbell_devices.h"
#include "doorbell_cmd.h"


#define TAG "db-cs2"

#include "PPCS_API.h"
#include "PPCS_Error.h"
#include "PPCS_Type.h"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define THROUGHPUT_DEBUG 1

static int32_t doorbell_cs2_p2p_write(int SessionID, uint8_t Channel, uint8_t *buff, uint32_t size);

typedef struct
{
	uint8_t is_running;
	uint8_t aud_running;
	beken_mutex_t tx_lock;
	db_channel_t *db_channel;

	uint16_t img_status : 1;
	uint16_t aud_status : 1;
	uint16_t device_connected : 1;
	db_channel_t *img_channel;
	db_channel_t *aud_channel;

	beken_mutex_t mutex;

	beken_thread_t thd;
	p2p_cs2_key_t *cs2_key;

	beken_semaphore_t aud_sem;

} doorbell_cs2_info_t;


static st_PPCS_NetInfo s_cs2_p2p_networkinfo;

doorbell_cs2_info_t *doorbell_cs2_info = NULL;

static volatile int s_current_sessionid = -1;

static const char *get_p2p_error_code_info(int err);

#define RECV_TMP_BUFF_SIZE   (1472)
#define RECV_TMP_CMD_BUFF_SIZE   (200)

#if THROUGHPUT_DEBUG
#define THROUGHPUT_ANLAYSE_MS 2000
static beken_timer_t s_throughput_timer;
static volatile uint32_t s_send_video_count = 0;
static volatile uint32_t s_send_video_bytes = 0;
static volatile uint32_t s_send_audio_bytes = 0;
static volatile uint32_t s_recv_audio_bytes = 0;
static volatile uint32_t s_recv_total_bytes = 0;
static volatile uint32_t s_send_time = 0;
static volatile uint32_t s_delay_count = 0;
#if CONFIG_MEDIA_DROP_STRATEGY_ENABLE
doorbell_cs2_h264_drop_info_t s_h264_drop_info = {0};
#endif
static void throughput_anlayse_timer_hdl(void *param)
{
    os_printf("cs2_tp, send bytes %d, count %d, %.3f count/s, %.3f KB/s, %.3f ms/perc, %.3fB/perc, delay count %d\n",
            s_send_video_bytes,
            s_send_video_count,
            1.0 * s_send_video_count / (THROUGHPUT_ANLAYSE_MS / 1000),
            (float)(s_send_video_bytes / (THROUGHPUT_ANLAYSE_MS / 1000) / 1024.0),
            1.0 * s_send_time / s_send_video_count,
            1.0 * s_send_video_bytes / s_send_video_count,
            s_delay_count);

    s_delay_count = s_send_time = s_send_video_count = s_recv_total_bytes = s_send_video_bytes = s_send_audio_bytes = s_recv_audio_bytes = 0;

    return;

}
#endif

int doorbell_cs2_img_send_packet(uint8_t *data, uint32_t len, uint16_t *retry_cnt)
{
	int send_byte = 0;
	uint32_t index = 0;
	uint8_t *ptr = data - sizeof(db_trans_head_t);
	uint16_t size = len + sizeof(db_trans_head_t);
	db_trans_head_t *tmp = (typeof(tmp))ptr;

    if (!rtos_is_timer_init(&s_throughput_timer))
    {
        //        rtos_init_timer(&bt_a2dp_source_write_timer, inter, bt_a2dp_source_write_from_file_timer_hdl, (void *)&pcm_file_fd);
        rtos_init_timer(&s_throughput_timer, THROUGHPUT_ANLAYSE_MS, throughput_anlayse_timer_hdl, NULL);

        rtos_change_period(&s_throughput_timer, THROUGHPUT_ANLAYSE_MS);
        rtos_start_timer(&s_throughput_timer);
    }

    //LOGE("%s %d\n", __func__, len);


	if (!doorbell_cs2_info->img_status)
	{
		LOGE("video not ready\n");
		return -1;
	}

	if (s_current_sessionid < 0)
	{
		LOGE("img send error, session lose\n");
		return -1;
	}

#if THROUGHPUT_DEBUG
	uint32_t start_time = rtos_get_time();
#endif

	do
	{
		if (s_current_sessionid < 0)
		{
			return -1;
		}

		send_byte = doorbell_cs2_p2p_write(s_current_sessionid, IMG_P2P_CHANNEL, ptr + index, size - index);

		if (send_byte < 0)
		{
			LOGE("send return fd:%d\n", send_byte);
			rtos_delay_milliseconds(CS2_P2P_TRANSFER_DELAY);
			return -1;
		}

		index += send_byte;

		if (index < size)
		{
#if THROUGHPUT_DEBUG
		    s_delay_count++;
#endif
			//LOGI("%s delay %d, %d\n", __func__, index, size);
			rtos_delay_milliseconds(CS2_P2P_TRANSFER_DELAY);
		}
	}
	while (index < size);

#if THROUGHPUT_DEBUG
	uint32_t end_time = rtos_get_time();

	//LOGE("send: %d, %d\n", index - sizeof(db_trans_head_t), len);
	s_send_video_count++;
	s_send_video_bytes += size;
	s_send_time += end_time - start_time;
#endif

	return index - sizeof(db_trans_head_t);
}

void doorbell_cs2_img_timer_deinit(void)
{
	if (doorbell_cs2_info)
	{
		rtos_lock_mutex(&doorbell_cs2_info->mutex);

		if (rtos_is_timer_init(&s_throughput_timer))
		{
			if (rtos_is_timer_running(&s_throughput_timer))
			{
				rtos_stop_timer(&s_throughput_timer);
			}

			rtos_deinit_timer(&s_throughput_timer);
		}

		rtos_unlock_mutex(&doorbell_cs2_info->mutex);
	}
}

int doorbell_cs2_img_send_prepare(uint8_t *data, uint32_t length)
{
	doorbell_transmission_pack(doorbell_cs2_info->img_channel, data, length);
//	db_trans_head_t *head = doorbell_cs2_info->img_channel->tbuf;

	return 0;
}

#if CONFIG_MEDIA_DROP_STRATEGY_ENABLE
void doorbell_cs2_img_h264_record_map(bool is_I_frame)
{
	LOGD("[map] is_I_frame %d I_num %d P_num %d drop_level %d drop_period %d\n",is_I_frame,
			s_h264_drop_info.I_num,
			s_h264_drop_info.P_num,
			s_h264_drop_info.drop_level,
			s_h264_drop_info.drop_period);

	if (is_I_frame && s_h264_drop_info.drop_level)
	{
		s_h264_drop_info.drop_period++;
	}

	if (is_I_frame && (s_h264_drop_info.I_num >= MEDIA_H264_I_FRAME_MAX_NUM))
	{
		s_h264_drop_info.I_num = 0;
	}

	if (is_I_frame)
	{
		s_h264_drop_info.I_num++;
		s_h264_drop_info.P_num = 0;
	}
	else
	{
		s_h264_drop_info.P_num++;
	}
}

void doorbell_cs2_h264_down_drop_level(uint32_t cur_drop_level,bool recovery,bool is_I_frame)
{
	uint32_t status_avg_size;
	int32_t avg_drop_level;

	LOGD("[dec] cur_drop_level %d recovery %d is_I_frame %d\n", cur_drop_level, recovery, is_I_frame);
	LOGD("[dec] drop period %d total_size %d stats cnt %d\n",s_h264_drop_info.drop_period,s_h264_drop_info.status_total_size,s_h264_drop_info.status_cnt);

	if ((s_h264_drop_info.drop_period <= MEDIA_H264_DOWN_DROP_LEVEL_MIN_PERIOD) || !is_I_frame || !s_h264_drop_info.status_cnt)
	{
		return;
	}

	status_avg_size = (s_h264_drop_info.status_total_size * 1024) / s_h264_drop_info.status_cnt;

	LOGD("status_avg_size %d\n", status_avg_size);

	if (status_avg_size < MEDIA_H264_START_DROP_THD)
	{
		s_h264_drop_info.drop_level = recovery ? 0 : cur_drop_level;
	}
	else
	{
		avg_drop_level = (status_avg_size - MEDIA_H264_START_DROP_THD) / MEDIA_H264_DROP_LEVEL_INTERVAL + 1;
		if (avg_drop_level >= MEDIA_H264_MAX_DROP_LEVEL)
			avg_drop_level = MEDIA_H264_MAX_DROP_LEVEL - 1;
		s_h264_drop_info.drop_level = (avg_drop_level > cur_drop_level) ? avg_drop_level : cur_drop_level;
		LOGD("[dec] avg_drop_level %d drop_level %d\n",avg_drop_level,s_h264_drop_info.drop_level);
	}

	s_h264_drop_info.drop_period = 0;
	s_h264_drop_info.status_total_size = 0;
	s_h264_drop_info.status_cnt = 0;
}

bool doorbell_cs2_img_h264_drop_level_check(uint32_t pre_size,UINT32 WriteSize,bool is_I_frame)
{
	uint32_t cur_size = pre_size + WriteSize;
	uint32_t cur_drop_level;
	bool need_drop = (cur_size > MEDIA_H264_START_DROP_THD) ? true : false;

	LOGD("pre_size %d WriteSize %d is_I_frame %d need_drop %d\n",pre_size,WriteSize,is_I_frame,need_drop);
	LOGD("drop_level %d drop_period %d is_I_frame %d need_drop %d\n",s_h264_drop_info.drop_level,s_h264_drop_info.drop_period);
	LOGD("drop thd %d offset %d\n",MEDIA_H264_START_DROP_THD,MEDIA_H264_DROP_LEVEL_INTERVAL);

	if ((!s_h264_drop_info.drop_level) && !need_drop)
	{
		return false;
	}

	if (s_h264_drop_info.drop_period > MEDIA_H264_EVERY_DROP_LEVEL_MAX_STATUS_CNT)
	{
		s_h264_drop_info.drop_period = 0;
		s_h264_drop_info.status_total_size = 0;
		s_h264_drop_info.status_cnt = 0;
	}

	if (s_h264_drop_info.drop_period)
	{
		s_h264_drop_info.status_total_size += WriteSize / 1024;
		s_h264_drop_info.status_cnt++;
	}

	if (need_drop)
	{
		cur_drop_level = (cur_size - MEDIA_H264_START_DROP_THD) / MEDIA_H264_DROP_LEVEL_INTERVAL + 1;

		if (cur_drop_level >= MEDIA_H264_MAX_DROP_LEVEL)
		{
			cur_drop_level = MEDIA_H264_MAX_DROP_LEVEL - 1;
		}

		if (s_h264_drop_info.drop_level > cur_drop_level)
		{
			doorbell_cs2_h264_down_drop_level(cur_drop_level,false,is_I_frame);
		}
		else
		{
			s_h264_drop_info.drop_level = cur_drop_level;
		}
	}
	else
	{
		doorbell_cs2_h264_down_drop_level(s_h264_drop_info.drop_level,true,is_I_frame);
	}

	if (s_h264_drop_info.drop_level && !s_h264_drop_info.drop_period)
	{
		s_h264_drop_info.drop_period++;
	}

	if (is_I_frame)
	{
		if((s_h264_drop_info.drop_level >= MEDIA_H264_MAX_DROP_LEVEL) && (s_h264_drop_info.I_num == MEDIA_H264_I_FRAME_MAX_NUM))
		{
			LOGD("Drop cur I frame\n");
			s_h264_drop_info.I_frame_droped = true;
			return true;
		}
		s_h264_drop_info.I_frame_droped = false;
	}
	else
	{
		if ((s_h264_drop_info.P_num >= (MEDIA_H264_MAX_DROP_LEVEL - s_h264_drop_info.drop_level)))
		{
			LOGD("Drop cur P frame\n");
			return true;
		}
	}

	return false;
}

bool doorbell_cs2_img_h264_drop_check(frame_buffer_t *frame,uint32_t count, uint16_t ext_size)
{
	int32_t Check_ret = 0;
	UINT32 WriteSize = 0;
	bool is_NAL_I_frame = false;
	static bool drop_other_gop_frame = false;
	uint32_t pre_size = 0;

	if (!s_h264_drop_info.check_type)
	{
		h264_base_config_t media_config;
		int ret = 0;

		ret = media_app_get_h264_encode_config(&media_config);
		if (ret == BK_OK)
		{
			s_h264_drop_info.check_type = true;
			s_h264_drop_info.support_type = (media_config.p_frame_cnt == MEDIA_H264_SUPPORT_MAX_P_FRAME_NUM) ? true : false;
			LOGI("check support cnt %d support type %d\n",media_config.p_frame_cnt,s_h264_drop_info.support_type);
		}
	}

	if (frame == NULL || !s_h264_drop_info.support_type)
	{
		LOGE("frame %d support_type %d\n",frame,s_h264_drop_info.support_type);
		return false;
	}

	if (frame->h264_type & (0x1 << H264_NAL_I_FRAME))
	{
		is_NAL_I_frame = true;
		drop_other_gop_frame = false;
	}
	else
	{
		is_NAL_I_frame = false;
	}

	LOGD("drop_check seq:%d len:%d type:%s\n", frame->sequence, frame->length, is_NAL_I_frame?"I":"P");

	if (s_current_sessionid < 0)
	{
		LOGE("%s Invalid sessionid:%d\n", __func__, s_current_sessionid);
		drop_other_gop_frame = false;
		return false;
	}

	doorbell_cs2_img_h264_record_map(is_NAL_I_frame);


	Check_ret = PPCS_Check_Buffer(s_current_sessionid, IMG_P2P_CHANNEL, &WriteSize, NULL);

	// st_debug("ThreadWrite PPCS_Check_Buffer: Session=%d,CH=%d,WriteSize=%d,ret=%d %s\n",
	//    SessionID, Channel, WriteSize, Check_ret, get_p2p_error_code_info(Check_ret));
	if (0 > Check_ret)
	{
		LOGE("%s PPCS_Check_Buffer: Session=%d,CH=%d,WriteSize=%d,ret=%d %s\n", __func__, 
			s_current_sessionid, IMG_P2P_CHANNEL, WriteSize, Check_ret, get_p2p_error_code_info(Check_ret));
		drop_other_gop_frame = false;
		return false;
	}

	//Recovery send frame immditely when write size is less than THD/2 and one gop no drop I frame
	if(!s_h264_drop_info.I_frame_droped && drop_other_gop_frame && (WriteSize < MEDIA_H264_START_DROP_THD / 2))
	{
		LOGD("[drop] restore p frame\r\n");
		drop_other_gop_frame = false;
	}

	if (drop_other_gop_frame)
	{
		LOGD("GOP Drop frame[%s]\n", is_NAL_I_frame ? "I" : "P");
		return true;
	}

	pre_size = frame->length + count * (ext_size + sizeof(db_trans_head_t));
	drop_other_gop_frame = doorbell_cs2_img_h264_drop_level_check(pre_size,WriteSize,is_NAL_I_frame);

	return drop_other_gop_frame;

}
bool doorbell_cs2_img_drop_check(frame_buffer_t *frame,uint32_t count, uint16_t ext_size)
{
	bool ret = false;

	if (frame == NULL)
	{
		return false;
	}

	switch(frame->fmt)
	{
		case PIXEL_FMT_H264:
			ret = doorbell_cs2_img_h264_drop_check(frame,count,ext_size);
			break;
		default:
			break;
	}

	return ret;
}
#endif

void *doorbell_cs2_img_get_tx_buf(void)
{
	if (doorbell_cs2_info == NULL)
	{
		LOGE("doorbell_udp_img_get_tx_buf, service null\n");
		return NULL;
	}

	if (doorbell_cs2_info->img_channel == NULL)
	{
		LOGE("doorbell_udp_img_get_tx_buf, img_channel null\n");
		return NULL;
	}

	LOGI("doorbell_udp_img_get_tx_buf, tbuf %p\n", doorbell_cs2_info->img_channel->tbuf);

	return doorbell_cs2_info->img_channel->tbuf + 1;
}

int doorbell_cs2_img_get_tx_size(void)
{
	if (doorbell_cs2_info == NULL)
	{
		LOGE("doorbell_udp_img_get_tx_size, service null\n");
		return 0;
	}

	if (doorbell_cs2_info->img_channel == NULL)
	{
		LOGE("doorbell_udp_img_get_tx_size, img_channel null\n");
		return 0;
	}

	return doorbell_cs2_info->img_channel->tsize - sizeof(db_trans_head_t);
}

int doorbell_cs2_aud_send_packet(uint8_t *data, uint32_t len, uint16_t *retry_cnt)
{
	int send_byte = 0;
	uint32_t index = 0;
	uint8_t *ptr = data - sizeof(db_trans_head_t);
	uint16_t size = len + sizeof(db_trans_head_t);

	if (!doorbell_cs2_info->aud_status)
	{
		LOGE("audio not ready\n");
		return -1;
	}

	if (s_current_sessionid < 0)
	{
		LOGE("aud send error, session lose\n");
		return -1;
	}

	do
	{
		send_byte = doorbell_cs2_p2p_write(s_current_sessionid, AUD_P2P_CHANNEL, ptr + index, size - index);

		if (send_byte < 0)
		{
			LOGE("send return fd:%d\n", send_byte);
			rtos_delay_milliseconds(CS2_P2P_TRANSFER_DELAY);
			return -1;
		}

		index += send_byte;

		if (index < size)
		{
			//LOGI("%s delay %d, %d\n", __func__, index, size);
			rtos_delay_milliseconds(CS2_P2P_TRANSFER_DELAY);
		}

	}
	while (index < size && s_current_sessionid > 0);

	//LOGE("send: %d, %d\n", index - sizeof(db_trans_head_t), len);

	return index - sizeof(db_trans_head_t);
}




int doorbell_cs2_aud_send_prepare(uint8_t *data, uint32_t length)
{
	doorbell_transmission_pack(doorbell_cs2_info->aud_channel, data, length);
	return 0;
}


void *doorbell_cs2_aud_get_tx_buf(void)
{
	if (doorbell_cs2_info == NULL)
	{
		LOGE("doorbell_cs2_aud_get_tx_buf, service null\n");
		return NULL;
	}

	if (doorbell_cs2_info->aud_channel == NULL)
	{
		LOGE("doorbell_cs2_aud_get_tx_buf, aud_channel null\n");
		return NULL;
	}

	//LOGI("doorbell_cs2_aud_get_tx_buf, tbuf %p\n", doorbell_cs2_info->aud_channel->tbuf);

	return doorbell_cs2_info->aud_channel->tbuf + 1;
}


int doorbell_cs2_aud_get_tx_size(void)
{
	if (doorbell_cs2_info == NULL)
	{
		LOGE("doorbell_cs2_aud_get_tx_size, service null\n");
		return 0;
	}

	if (doorbell_cs2_info->aud_channel == NULL)
	{
		LOGE("doorbell_cs2_aud_get_tx_size, aud_channel null\n");
		return 0;
	}

	return doorbell_cs2_info->aud_channel->tsize - sizeof(db_trans_head_t);
}

static media_transfer_cb_t doorbell_cs2_img_channel =
{
	.send = doorbell_cs2_img_send_packet,
	.prepare = doorbell_cs2_img_send_prepare,
	.get_tx_buf = doorbell_cs2_img_get_tx_buf,
	.get_tx_size = doorbell_cs2_img_get_tx_size,
	.fmt = PIXEL_FMT_UNKNOW,
	#if CONFIG_MEDIA_DROP_STRATEGY_ENABLE
	.drop_check = doorbell_cs2_img_drop_check,
	#else
	.drop_check = NULL,
	#endif
};


static const media_transfer_cb_t doorbell_cs2_aud_channel =
{
	.send = doorbell_cs2_aud_send_packet,
	.prepare = doorbell_cs2_aud_send_prepare,
	.get_tx_buf = doorbell_cs2_aud_get_tx_buf,
	.get_tx_size = doorbell_cs2_aud_get_tx_size,
	.fmt = PIXEL_FMT_UNKNOW,
};

static void doorbell_cs2_get_time(time_info_t *pt)
{

	struct timeval tmv;
	int ret = gettimeofday(&tmv, NULL);

	if (0 != ret)
	{
		printf("gettimeofday failed!! errno=%d\n", errno);
		memset(pt, 0, sizeof(time_info_t));
		return ;
	}

	//struct tm *ptm = localtime((const time_t *)&tmv.tv_sec);
	struct tm stm = {0};
	struct tm *ptm = localtime_r((const time_t *)&tmv.tv_sec, &stm);

	if (!ptm)
	{
		printf("localtime_r failed!!\n");
		os_memset(pt, 0, sizeof(time_info_t));
		pt->tick_msec = ((unsigned long long)tmv.tv_sec) * 1000 + tmv.tv_usec / 1000; // ->ms
	}
	else
	{
		pt->year = stm.tm_year + 1900;
		pt->mon = stm.tm_mon + 1;
		pt->day = stm.tm_mday;
		pt->week = stm.tm_wday;
		pt->hour = stm.tm_hour;
		pt->min = stm.tm_min;
		pt->sec = stm.tm_sec;
		pt->msec = (int)(tmv.tv_usec / 1000);
		pt->tick_sec = tmv.tv_sec; // 1970�?1�?1�?0点至今的秒数�?
		pt->tick_msec = ((unsigned long long)tmv.tv_sec) * 1000 + tmv.tv_usec / 1000; // ->ms
		os_memset(pt->date, 0, sizeof(pt->date));
		snprintf(pt->date, sizeof(pt->date) - 1, "%04d-%02d-%02d %02d:%02d:%02d.%03d", pt->year, pt->mon, pt->day, pt->hour, pt->min, pt->sec, pt->msec);
	}
}

static const char *get_p2p_error_code_info(int err)
{
	switch (err)
	{
		case 0:
			return "ERROR_PPCS_SUCCESSFUL";

		case -1:
			return "ERROR_PPCS_NOT_INITIALIZED"; // API didn't initialized

		case -2:
			return "ERROR_PPCS_ALREADY_INITIALIZED";

		case -3:
			return "ERROR_PPCS_TIME_OUT";

		case -4:
			return "ERROR_PPCS_INVALID_ID";//Invalid Device ID !!

		case -5:
			return "ERROR_PPCS_INVALID_PARAMETER";

		case -6:
			return "ERROR_PPCS_DEVICE_NOT_ONLINE";

		case -7:
			return "ERROR_PPCS_FAIL_TO_RESOLVE_NAME";

		case -8:
			return "ERROR_PPCS_INVALID_PREFIX";//Prefix of Device ID is not accepted by Server !!

		case -9:
			return "ERROR_PPCS_ID_OUT_OF_DATE";

		case -10:
			return "ERROR_PPCS_NO_RELAY_SERVER_AVAILABLE";

		case -11:
			return "ERROR_PPCS_INVALID_SESSION_HANDLE";

		case -12:
			return "ERROR_PPCS_SESSION_CLOSED_REMOTE";

		case -13:
			return "ERROR_PPCS_SESSION_CLOSED_TIMEOUT";

		case -14:
			return "ERROR_PPCS_SESSION_CLOSED_CALLED";

		case -15:
			return "ERROR_PPCS_REMOTE_SITE_BUFFER_FULL";

		case -16:
			return "ERROR_PPCS_USER_LISTEN_BREAK";//Listen break is called !!

		case -17:
			return "ERROR_PPCS_MAX_SESSION";//Exceed max session !!

		case -18:
			return "ERROR_PPCS_UDP_PORT_BIND_FAILED";//The specified UDP port can not be binded !!

		case -19:
			return "ERROR_PPCS_USER_CONNECT_BREAK";

		case -20:
			return "ERROR_PPCS_SESSION_CLOSED_INSUFFICIENT_MEMORY";

		case -21:
			return "ERROR_PPCS_INVALID_APILICENSE";//API License code is not correct !!

		case -22:
			return "ERROR_PPCS_FAIL_TO_CREATE_THREAD";//Fail to Create Thread !!

		case -23:
			return "ERROR_PPCS_INVALID_DSK";

		case -24:
			return "ERROR_PPCS_FAILED_TO_CONNECT_TCP_RELAY";

		case -25:
			return "ERROR_PPCS_FAIL_TO_ALLOCATE_MEMORY"; // only availeable since P2P API ver: 4.2.0.0

		default:
			return "Unknown, something is wrong";
	}
} // get_p2p_error_code_info

static void show_network(st_PPCS_NetInfo NetInfo)
{
	LOGI("-------------- NetInfo: -------------------\n");
	LOGI("Internet Reachable     : %s\n", (NetInfo.bFlagInternet == 1) ? "YES" : "NO");
	LOGI("P2P Server IP resolved : %s\n", (NetInfo.bFlagHostResolved == 1) ? "YES" : "NO");
	LOGI("P2P Server Hello Ack   : %s\n", (NetInfo.bFlagServerHello == 1) ? "YES" : "NO");

	switch (NetInfo.NAT_Type)
	{
		case 0:
			LOGI("Local NAT Type         : Unknow\n");
			break;

		case 1:
			LOGI("Local NAT Type         : IP-Restricted Cone\n");
			break;

		case 2:
			LOGI("Local NAT Type         : Port-Restricted Cone\n");
			break;

		case 3:
			LOGI("Local NAT Type         : Symmetric\n");
			break;

		case 4:
			LOGI("Local NAT Type         : Different Wan IP Detected!!\n");
			break;
	}

	LOGI("My Wan IP : %s\n", NetInfo.MyWanIP);
	LOGI("My Lan IP : %s\n", NetInfo.MyLanIP);
	LOGI("-------------------------------------------\n");
}

static const char *get_listen_error_info(int ret)
{
	switch (ret)
	{
		case ERROR_PPCS_NOT_INITIALIZED:
			return "API didn't initialized";

		case ERROR_PPCS_TIME_OUT:
			return "Listen time out, No client connect me !!";

		case ERROR_PPCS_INVALID_ID:
			return "Invalid Device ID !!";

		case ERROR_PPCS_INVALID_PREFIX:
			return "Prefix of Device ID is not accepted by Server !!";

		case ERROR_PPCS_UDP_PORT_BIND_FAILED:
			return "The specified UDP port can not be binded !!";

		case ERROR_PPCS_MAX_SESSION:
			return "Exceed max session !!";

		case ERROR_PPCS_USER_LISTEN_BREAK:
			return "Listen break is called !!";

		case ERROR_PPCS_INVALID_APILICENSE:
			return "API License code is not correct !!";

		case ERROR_PPCS_FAIL_TO_CREATE_THREAD:
			return "Fail tO Create Thread !!";

		default:
			return get_p2p_error_code_info(ret);
	}
}

static INT32 get_socket_type(INT32 skt)
{
	socklen_t length = sizeof(unsigned int);
	int type;
	getsockopt(skt, SOL_SOCKET, SO_TYPE, (char *)&type, &length);

	if (type == SOCK_STREAM)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

static int is_lan_cmp(const char *IP1, const char *IP2)
{
#define   YES   1
#define   NO   0
	short Len_IP1 = strlen(IP1);
	short Len_IP2 = strlen(IP2);

	if (!IP1 || 7 > Len_IP1 || !IP2 || 7 > Len_IP2)
	{
		return -1;
	}

	if (0 == strcmp(IP1, IP2))
	{
		return YES;
	}

	const char *pIndex = IP1 + Len_IP1 - 1;

	while (1)
	{
		if ('.' == *pIndex || pIndex == IP1)
		{
			break;
		}
		else
		{
			pIndex--;
		}
	}

	if (0 == strncmp(IP1, IP2, (int)(pIndex - IP1)))
	{
		return YES;
	}

	return NO;
}

static int get_session_info(int SessionID, session_info_t *MySInfo)
{
	memset(MySInfo, 0, sizeof(session_info_t));

	st_PPCS_Session Sinfo;

	int ret = PPCS_Check(SessionID, &Sinfo);

	if (ERROR_PPCS_SUCCESSFUL == ret)
	{
		MySInfo->skt = Sinfo.Skt;
		// Remote addr
		snprintf(MySInfo->remote_ip, sizeof(MySInfo->remote_ip), "%s", inet_ntoa(Sinfo.RemoteAddr.sin_addr));
		MySInfo->remote_port = ntohs(Sinfo.RemoteAddr.sin_port);
		// Lan addr
		snprintf(MySInfo->local_ip, sizeof(MySInfo->local_ip), "%s", inet_ntoa(Sinfo.MyLocalAddr.sin_addr));
		MySInfo->local_port = ntohs(Sinfo.MyLocalAddr.sin_port);
		// Wan addr
		snprintf(MySInfo->wan_ip, sizeof(MySInfo->wan_ip), "%s", inet_ntoa(Sinfo.MyWanAddr.sin_addr));
		MySInfo->wan_port = ntohs(Sinfo.MyWanAddr.sin_port);

		MySInfo->connect_time = Sinfo.ConnectTime;
		memcpy(MySInfo->did, Sinfo.DID, strlen(Sinfo.DID));
		MySInfo->bcord = Sinfo.bCorD;

		if (0 == Sinfo.bMode)
		{
			if (Sinfo.RemoteAddr.sin_addr.s_addr == Sinfo.MyLocalAddr.sin_addr.s_addr || 1 == is_lan_cmp(MySInfo->local_ip, MySInfo->remote_ip))
			{
				MySInfo->bmode = 0;
				1 == get_socket_type(Sinfo.Skt) ? memcpy(MySInfo->mode, "LAN.", 4) : memcpy(MySInfo->mode, "LAN", 3);
			}
			else
			{
				MySInfo->bmode = 1;
				memcpy(MySInfo->mode, "P2P", 3);
			}
		}
		else if (1 == Sinfo.bMode)
		{
			MySInfo->bmode = 2;
			memcpy(MySInfo->mode, "RLY", 3);
		}
		else if (2 == Sinfo.bMode)
		{
			MySInfo->bmode = 3;
			memcpy(MySInfo->mode, "TCP", 3);
		}
		else if (3 == Sinfo.bMode)
		{
			MySInfo->bmode = 4;    //// support by P2P API 5.0.8
			memcpy(MySInfo->mode, "RP2P", 4);
		}
	}

	// else LOGI("PPCS_Check(SessionID=%d) ret=%d [%s]\n", SessionID, ret, get_p2p_error_code_info(ret));
	return ret;
}



static int cs2_p2p_listen(const char *did, const char *APILicense, unsigned long Repeat, uint8_t *is_run)
{
#ifdef TIME_SHOW
	time_info_t t1, t2;
#endif

	int ret = -1;
	int SessionID = -99;

	unsigned int TimeOut_Sec = 120;
	unsigned short UDP_Port = 0;// PPCS_Listen 端口�? 0 让底层自动分配�??
	char bEnableInternet = 2;

#ifdef TIME_SHOW
	os_memset(&t2, 0, sizeof(t2));

	doorbell_cs2_get_time(&t1);
#endif

	do
	{
		ret = PPCS_Listen(did, TimeOut_Sec, UDP_Port, bEnableInternet, APILicense);

		LOGI("PPCS_Listen timeout\n");
	}
	while (ERROR_PPCS_TIME_OUT == ret && *is_run);

	if (1 == Repeat)
	{
		LOGI("%02lu-PPCS_Listen(%s,%d,%d,%d,%s) ...\n", Repeat, did, TimeOut_Sec, UDP_Port, bEnableInternet, APILicense);
	}
	else
	{
		LOGI("%02lu-PPCS_Listen(%s,%d,%d,%d,%s) ...\n", Repeat, did, TimeOut_Sec, UDP_Port, bEnableInternet, APILicense);
	}

#ifdef TIME_SHOW
	doorbell_cs2_get_time(&t2);
#endif

	if (*is_run == 0)
	{
		LOGI("p2p", "%s is_run is 0, exit\n", __func__);

		if (ret >= 0)
		{
			PPCS_ForceClose(ret);
		}

		return -1;
	}

	if (0 > ret)
	{
#ifdef TIME_SHOW
		LOGI("[%s] %02lu-PPCS_Listen failed:%d ms, ret=%d %s\n", t2.date, Repeat, TU_MS(t1, t2), ret, get_listen_error_info(ret));
#else
		LOGI("%02lu-PPCS_Listen failed, ret=%d %s\n", Repeat, ret, get_listen_error_info(ret));
#endif
		return ret;
	}
	else //// ret >= 0, Listen OK, new client connect in.
	{
		SessionID = ret; // 每个 >=0 �? SessionID 都是�?个正常的连接，本 sample 是单用户连接范例，多用户端连接注意要保留区分每一�? PPCS_Listen >=0 �? SessionID, 当连接断�?或�?? SessionID 不用时，必须要调 PPCS_Close(SessionID)/PPCS_ForceClose(SessionID) 关闭连线释放资源�?
//		PPCS_Share_Bandwidth(0); // 当有连接进来，关闭设备转发功能�??

		LOGI("PPCS_Share_Bandwidth(0) is Called!!\n");
	}

#ifdef SESSION_PRINT
	{
		session_info_t session_info;

		if (ERROR_PPCS_SUCCESSFUL != (ret = get_session_info(SessionID, &session_info)))
		{
			LOGI("%02lu-did=%s,Session=%d,RmtAddr=Unknown (PPCS_Check:%d)\n", Repeat, did, SessionID, ret);
			return SessionID;
		}
	}
#endif
	return SessionID;
}


static int32_t doorbell_cs2_p2p_write(int SessionID, uint8_t Channel, uint8_t *buff, uint32_t size)
{
	int32_t ret = 0;
	int32_t Check_ret = 0;
	UINT32 WriteSize = size;
	uint32_t write_index = 0;
	uint32_t write_not_send_thr = PPCS_TX_BUFFER_THD;
	const uint32_t write_per_count_thr = size;///write_not_send_thr / 10;//1024 * 2;

	if (size == 0)
	{
		LOGE("%s, size: 0\n", __func__);
		return 0;
	}

	do
	{
		uint32_t will_write_size = ((size - write_index < write_per_count_thr) ? (size - write_index) : write_per_count_thr);
		// 在调�? PPCS_Write 之前�?定要调用 PPCS_Check_Buffer �?测写缓存还有多少数据尚未发出去，�?控制在一个合理范围，�?般控制在 128KB/256KB 左右�?
		Check_ret = PPCS_Check_Buffer(SessionID, Channel, &WriteSize, NULL);

		// st_debug("ThreadWrite PPCS_Check_Buffer: Session=%d,CH=%d,WriteSize=%d,ret=%d %s\n", SessionID, Channel, WriteSize, Check_ret, get_p2p_error_code_info(Check_ret));
		if (0 > Check_ret)
		{
			LOGE("%s PPCS_Check_Buffer: Session=%d,CH=%d,WriteSize=%d,ret=%d %s\n", __func__, SessionID, Channel, WriteSize, Check_ret, get_p2p_error_code_info(Check_ret));
			ret = Check_ret;
			goto WRITE_FAIL;
			break;
		}

		if (Channel == IMG_P2P_CHANNEL)
		{
			write_not_send_thr = CS2_IMG_MAX_TX_BUFFER_THD;
		}
		else if(Channel == AUD_P2P_CHANNEL)
		{
			write_not_send_thr = CS2_AUD_MAX_TX_BUFFER_THD;
		}

		// 写缓存的数据大小超过128KB/256KB，则�?考虑延时缓一缓�??
		// 如果发现 wsize 越来越大，可能网络状态很差，�?要�?�虑�?下丢帧或降码率，这是�?个动态调整策略，非常重要!!
		// On device, Recommended CHECK_WRITE_THRESHOLD_SIZE == (128 or 256) * 1024 Byte. this sample set 1MB.

		if (WriteSize <= write_not_send_thr)
		{
			//            os_printf("%s start write %d WriteSize %d SessionID %d channel %d\n", __func__, will_write_size, WriteSize, SessionID, Channel);

			//LOGI("channel: %d, write size: %d\n", Channel, will_write_size);
			ret = PPCS_Write(SessionID, Channel, (CHAR *)(buff + write_index), will_write_size);
			//LOGI("channel: %d, return size: %d\n", Channel, ret);

			if (0 > ret)
			{
				if (ERROR_PPCS_SESSION_CLOSED_TIMEOUT == ret)
				{
					LOGE("%s Session=%d,CH=%d,ret=%d, Session Closed TimeOUT!!\n", __func__, SessionID, Channel, ret);
				}
				else if (ERROR_PPCS_SESSION_CLOSED_REMOTE == ret)
				{
					LOGE("%s Session=%d,CH=%d,ret=%d, Session Remote Closed!!\n", __func__, SessionID, Channel, ret);
				}
				else if (ERROR_PPCS_INVALID_PARAMETER == ret)
				{
					LOGE("%s Session=%d,CH=%d,ret=%d, ERROR_PPCS_INVALID_PARAMETER %d!!\n", __func__, SessionID, Channel, ret, will_write_size);
				}
				else
				{
					LOGE("%s Session=%d,CH=%d,ret=%d %s\n", __func__, SessionID, Channel, ret, get_p2p_error_code_info(ret));
				}

				goto WRITE_FAIL;
			}

			write_index += ret;
		}
		else
		{
			//LOGE("%s, full\n", __func__);
			break;
		}
	}
	while (write_index < size);

	return write_index;

WRITE_FAIL:

	return ret;
}


static int32_t doorbell_cs2_p2p_cmd_handle(uint8_t *data, uint32_t length)
{
	LOGI("%s, length: %d\n", __func__, length);
	doorbell_transmission_unpack(doorbell_cs2_info->db_channel, data, length, doorbell_transmission_cmd_recive_callback);
	return 0;
}

static int doorbell_cs2_p2p_event_send(uint8_t *data, uint16_t length)
{
	int send_byte = 0;
	uint32_t index = 0;

	do
	{
		send_byte = doorbell_cs2_p2p_write(s_current_sessionid, CMD_P2P_CHANNEL, data + index, length - index);

		if (send_byte < 0)
		{
			LOGE("send return fd:%d\n", send_byte);
			rtos_delay_milliseconds(CS2_P2P_TRANSFER_DELAY);
			return -1;
		}

		index += send_byte;

		if (index < length)
		{
			LOGI("%s delay %d, %d\n", __func__, index, length);
			rtos_delay_milliseconds(CS2_P2P_TRANSFER_DELAY);
		}

	}
	while (index < length);


	return index;
}

static inline void doorbell_cs2_voice_receiver(db_channel_t *channel, uint16_t sequence, uint16_t flags, uint32_t timestamp, uint8_t sequences, uint8_t *data, uint16_t length)
{
	doorbell_audio_data_callback(data, length);
}


static void doorbell_cs2_session_close(void)
{
	time_info_t t1, t2;

	memset(&t2, 0, sizeof(t2));

	rtos_lock_mutex(&doorbell_cs2_info->mutex);

	if (s_current_sessionid == -1)
	{
		LOGW("%s already close\n", __func__);
		goto out;
	}

	doorbell_cs2_get_time(&t1);

	PPCS_ForceClose(s_current_sessionid);// PPCS_Close(SessionID);// 不能多线程对同一�? SessionID �? PPCS_Close(SessionID)/PPCS_ForceClose(SessionID) 的动作，否则可能导致崩溃�?

	doorbell_cs2_get_time(&t2);

	LOGI("%s: (%d) done!! t:%d ms\n", __func__, s_current_sessionid, TU_MS(t1, t2));

	s_current_sessionid = -1;

out:

	if (doorbell_cs2_info->device_connected == BK_TRUE)
	{
		doorbell_msg_t msg;

		msg.event = DBEVT_REMOTE_DEVICE_DISCONNECTED;
		msg.param = BK_OK;
		doorbell_send_msg(&msg);

		doorbell_cs2_info->device_connected = BK_FALSE;
	}

	rtos_unlock_mutex(&doorbell_cs2_info->mutex);

}

static int doorbell_cs2_p2p_audio_receiver(beken_thread_arg_t arg)
{
	int32_t ret = 0;

	uint8_t *tmp_read_buf = NULL;

	tmp_read_buf = os_malloc(RECV_TMP_BUFF_SIZE);

	if (!tmp_read_buf)
	{
		LOGE("p2p", "%s alloc err\n", __func__);
		return -1;
	}

	rtos_set_semaphore(&doorbell_cs2_info->aud_sem);

	while (doorbell_cs2_info->aud_running == BK_TRUE)//Repeat < Total_Times)
	{

		if (0 <= s_current_sessionid)
		{
			LOGE("%s listen Sid %d\n", __func__, s_current_sessionid);

			rtos_lock_mutex(&doorbell_cs2_info->mutex);
			doorbell_cs2_info->aud_status = BK_TRUE;
			rtos_unlock_mutex(&doorbell_cs2_info->mutex);

			do
			{
				INT32 ReadSize = 0;

				ReadSize = RECV_TMP_BUFF_SIZE;

				ret = PPCS_Read(s_current_sessionid, AUD_P2P_CHANNEL, (char *)tmp_read_buf, &ReadSize, 1000 * 2);

				if (ReadSize)
				{
					LOGD("got audio count: %d\n", ReadSize);
					doorbell_transmission_unpack(doorbell_cs2_info->aud_channel, tmp_read_buf, ReadSize, doorbell_cs2_voice_receiver);

					continue;
				}

				if (ret == ERROR_PPCS_TIME_OUT)
				{
					LOGD("got audio data timeout\n");
					continue;
				}

				if (ret < 0 && ERROR_PPCS_TIME_OUT != ret)
				{
					LOGE("%s PPCS_Read ret err %d %s\n", __func__, ret, get_p2p_error_code_info(ret));
					doorbell_cs2_session_close();
					break;
				}
			}
			while (doorbell_cs2_info->aud_running == BK_TRUE);

			doorbell_cs2_info->aud_status = BK_FALSE;
			doorbell_cs2_info->aud_running = false;

			rtos_delay_milliseconds(300);
			break;
		}
		else
		{
			LOGE("session error wait\n");
			rtos_delay_milliseconds(300);
		}
	}

	//rtos_set_semaphore(&doorbell_cs2_info->aud_sem);

	if (tmp_read_buf)
	{
		os_free(tmp_read_buf);
	}

	LOGE("audio thread exit\n");

	rtos_delete_thread(NULL);

	return 0;
}


static int doorbell_cs2_p2p_interface_core(p2p_cs2_key_t *key)
{
	int Repeat = 0;
	int32_t ret = 0;

	int session = -99;
	const unsigned long Total_Times = Repeat;
	uint8_t *tmp_read_buf = NULL;

	(void)Total_Times;
	Repeat = 0;

	ret = PPCS_NetworkDetect(&s_cs2_p2p_networkinfo, 0);
	show_network(s_cs2_p2p_networkinfo);

	tmp_read_buf = os_malloc(RECV_TMP_CMD_BUFF_SIZE);

	if (!tmp_read_buf)
	{
		LOGE("p2p", "%s alloc err\n", __func__);
		return -1;
	}

	{
		doorbell_msg_t msg;

		msg.event = DBEVT_P2P_CS2_SERVICE_START_RESPONSE;
		msg.param = BK_OK;
		doorbell_send_msg(&msg);
	}


	while (doorbell_cs2_info->is_running)//Repeat < Total_Times)
	{
		Repeat++;

		ret = PPCS_NetworkDetect(&s_cs2_p2p_networkinfo, 0);

		if (ret < 0)
		{
			LOGE("p2p", "%s PPCS_NetworkDetect err %d %s\n", __func__, ret, get_p2p_error_code_info(ret));
			return -1;
		}

		s_current_sessionid = session = cs2_p2p_listen(key->did, key->apilicense, Repeat, &doorbell_cs2_info->is_running);

		if (0 <= session)
		{
			BK_LOGI("p2p", "%s listen Sid %d\n", __func__, s_current_sessionid);

			doorbell_cs2_info->device_connected = BK_TRUE;

			doorbell_cs2_info->img_status = BK_TRUE;
			doorbell_cs2_info->aud_status = BK_TRUE;

			do
			{
				INT32 ReadSize = 0;

				ReadSize = RECV_TMP_CMD_BUFF_SIZE;

				ret = PPCS_Read(session, CMD_P2P_CHANNEL, (char *)tmp_read_buf, &ReadSize, 1000 * 2);

				if (ReadSize)
				{
					LOGI("got cmd count: %d\n", ReadSize);
					doorbell_cs2_p2p_cmd_handle(tmp_read_buf, ReadSize);
					continue;
				}

				if (ret == ERROR_PPCS_TIME_OUT)
				{
					LOGD("got cmd data timeout, %d\n", ReadSize);
					continue;
				}

				if (ret < 0 && ERROR_PPCS_TIME_OUT != ret)
				{
					LOGI("%s PPCS_Read ret err %d %s\n", __func__, ret, get_p2p_error_code_info(ret));
					goto READ_ERR;
				}
			}
			while (doorbell_cs2_info->is_running);
READ_ERR:

			LOGE("\ncmd session thread error, close");

			doorbell_cs2_session_close();

			session = -1;

			rtos_delay_milliseconds(300); // 两次 PPCS_Listen 之间�?要保持间隔�??

			continue;
		}
		else if (ERROR_PPCS_MAX_SESSION == session)
		{
			rtos_delay_milliseconds(1 * 1000);
		}
		else if (session == -1)
		{
			break;
		}

		doorbell_cs2_info->img_status = BK_FALSE;
		doorbell_cs2_info->aud_status = BK_FALSE;

	}


	if (tmp_read_buf)
	{
		os_free(tmp_read_buf);
	}

	if (s_current_sessionid >= 0)
	{
		PPCS_ForceClose(s_current_sessionid);
		s_current_sessionid = -1;
	}

	return 0;
}


static int doorbell_cs2_p2p_interface_init(p2p_cs2_key_t *key)
{
	int ret = 0;
	UINT32 APIVersion = PPCS_GetAPIVersion();
	char VerBuf[64] = {0};

	snprintf(VerBuf, sizeof(VerBuf), "%d.%d.%d.%d",
	         (APIVersion & 0xFF000000) >> 24,
	         (APIVersion & 0x00FF0000) >> 16,
	         (APIVersion & 0x0000FF00) >> 8,
	         (APIVersion & 0x000000FF) >> 0);

	if (0 > strncmp(VerBuf, "3.5.0.0", 5))
	{
		LOGI("PPCS P2P API Version: %d.%d.%d.%d\n",
		     (APIVersion & 0xFF000000) >> 24,
		     (APIVersion & 0x00FF0000) >> 16,
		     (APIVersion & 0x0000FF00) >> 8,
		     (APIVersion & 0x000000FF) >> 0);
	}
	else
	{
		const char *pVer = PPCS_GetAPIInformation();// PPCS_GetAPIInformation: support by Version >= 3.5.0
		LOGI("PPCS_GetAPIInformation(%u Byte):\n%s\n", (unsigned)strlen(pVer), pVer);
	}

	time_info_t t1, t2;
	os_memset(&t1, 0, sizeof(t1));
	os_memset(&t2, 0, sizeof(t2));

	if (0 <= strncmp(VerBuf, "4.2.0.0", 5)) // PPCS_Initialize JsonString support by Version>=4.2.0
	{
		int MaxNumSess = 1; // Max Number Session: 1~256.
		int SessAliveSec = 15; // session timeout close alive: 6~30.


		char InitJsonString[256] = {0};
		snprintf(InitJsonString, sizeof(InitJsonString), "{\"InitString\":\"%s\",\"MaxNumSess\":%d,\"SessAliveSec\":%d}", key->initstring, MaxNumSess, SessAliveSec);
		// st_debug("InitJsonString=%s\n",InitJsonString);
		doorbell_cs2_get_time(&t1);
		LOGI("[%s] PPCS_Initialize1(%s) ...\n", t1.date, InitJsonString);


		// 如果Parameter 不是正确的JSON字串则会被当成InitString[:P2PKey]来处�?, 如此以兼容旧�?.
		ret = PPCS_Initialize((char *)InitJsonString);

		doorbell_cs2_get_time(&t2);
		LOGI("[%s] PPCS_Initialize2 len(%d): ret=%d, t:%d ms\n", t2.date, strlen(key->initstring), ret, TU_MS(t1, t2));


		if (ERROR_PPCS_SUCCESSFUL != ret && ERROR_PPCS_ALREADY_INITIALIZED != ret)
		{
			LOGI("[%s] PPCS_Initialize: ret=%d\n", t2.date, ret);
			return 0;
		}
	}
	else
	{
		doorbell_cs2_get_time(&t1);
		LOGI("[%s] PPCS_Initialize3(%s) ...\n", t1.date, key->initstring);
		ret = PPCS_Initialize((char *)key->initstring);
		doorbell_cs2_get_time(&t2);
		LOGI("[%s] PPCS_Initialize4(%s): ret=%d, t:%d ms\n", t2.date, key->initstring, ret, TU_MS(t1, t2));

		if (ERROR_PPCS_SUCCESSFUL != ret && ERROR_PPCS_ALREADY_INITIALIZED != ret)
		{
			LOGI("[%s] PPCS_Initialize: ret=%d\n", t2.date, ret);
			return 0;
		}
	}

	return 0;
}


static void doorbell_cs2_p2p_interface_deinit(void)
{
	int ret = PPCS_DeInitialize();

	if (ERROR_PPCS_SUCCESSFUL != ret)
	{
		LOGE("%s PPCS_DeInitialize: ret=%d %s\n", __func__, ret, get_p2p_error_code_info(ret));
	}

	os_memset(&s_cs2_p2p_networkinfo, 0, sizeof(s_cs2_p2p_networkinfo));
}

static void doorbell_cs2_service_main(beken_thread_arg_t arg)
{
	doorbell_cs2_p2p_interface_init(doorbell_cs2_info->cs2_key);

	doorbell_cs2_info->is_running = 1;

	doorbell_cs2_p2p_interface_core(doorbell_cs2_info->cs2_key);

	doorbell_cs2_p2p_interface_deinit();
}


static const db_channel_cb_t db_channel_callback =
{
	.tsend = doorbell_cs2_p2p_event_send,
};


int doorbell_cs2_service_init(void *param)
{
	int ret;

	LOGI("%s\n", __func__);

	p2p_cs2_key_t *p2p_cs2_key = (p2p_cs2_key_t *)param;

	if (p2p_cs2_key == NULL
	    || p2p_cs2_key->did == NULL
	    || p2p_cs2_key->apilicense == NULL
	    || p2p_cs2_key->initstring == NULL)
	{
		LOGE("%s p2p_cs2_key null\n", __func__);
		return BK_FAIL;
	}

	if (doorbell_cs2_info == NULL)
	{
		doorbell_cs2_info = os_malloc(sizeof(doorbell_cs2_info_t));

		if (doorbell_cs2_info == NULL)
		{
			LOGE("%s doorbell_cs2_info malloc failed\n", __func__);
			return BK_FAIL;
		}

	}

	os_memset(doorbell_cs2_info, 0, sizeof(doorbell_cs2_info_t));

	doorbell_cs2_info->cs2_key = p2p_cs2_key;

	rtos_init_mutex(&doorbell_cs2_info->tx_lock);

	rtos_init_mutex(&doorbell_cs2_info->mutex);

	doorbell_cs2_info->db_channel = doorbell_transmission_malloc(1600, 1600);
	doorbell_cs2_info->db_channel->cb = &db_channel_callback;


	doorbell_cs2_info->img_channel = doorbell_transmission_malloc(1600, DOORBELL_NETWORK_MAX_SIZE);

	if (doorbell_cs2_info->img_channel == NULL)
	{
		LOGE("img_channel malloc failed\n");
		goto error;
	}

	doorbell_cs2_info->aud_channel = doorbell_transmission_malloc(1600, DOORBELL_NETWORK_MAX_SIZE);

	if (doorbell_cs2_info->aud_channel == NULL)
	{
		LOGE("aud_channel malloc failed\n");
		goto error;
	}

	ret = rtos_init_semaphore_ex(&doorbell_cs2_info->aud_sem, 1, 0);

	doorbell_devices_set_camera_transfer_callback(&doorbell_cs2_img_channel);
	doorbell_devices_set_audio_transfer_callback(&doorbell_cs2_aud_channel);

	ret = rtos_create_thread(&doorbell_cs2_info->thd,
	                         4,
	                         "doorbell",
	                         (beken_thread_function_t)doorbell_cs2_service_main,
	                         1024 * 8,
	                         (beken_thread_arg_t)NULL);
	if (ret != BK_OK)
	{
		LOGE("Error: Failed to create doorbell udp service: %d\n", ret);
		return BK_FAIL;
	}

	return BK_OK;

error:

	return BK_FAIL;
}

void demo_doorbell_cs2_p2p_server_deinit(void)
{
	LOGI("%s\n", __func__);
}


static int doorbell_cs2_camera_state_changed(doorbell_state_t state)
{

	return BK_OK;
}

static int doorbell_cs2_audio_state_changed(doorbell_state_t state)
{
	int ret;

	LOGI("%s: %d\n", __func__, state);

	if (state == DB_TURN_ON)
	{
		if (doorbell_cs2_info->aud_running == BK_TRUE)
		{
			LOGW("%s already open\n", __func__);
			return BK_OK;
		}


		doorbell_cs2_info->aud_running = BK_TRUE;

		ret = rtos_create_thread(&doorbell_cs2_info->thd,
		                         4,
		                         "audo",
		                         (beken_thread_function_t)doorbell_cs2_p2p_audio_receiver,
		                         1024 * 4,
		                         (beken_thread_arg_t)NULL);


		if (ret != BK_OK)
		{
			LOGE("%s init thread failed\n", __func__);
			goto error;
		}

		ret = rtos_get_semaphore(&doorbell_cs2_info->aud_sem, BEKEN_WAIT_FOREVER);

		if (ret != BK_OK)
		{
			LOGE("%s wait semaphore failed\n", __func__);
			goto error;
		}
	}
	else if (state == DB_TURN_OFF)
	{
		#if 0
		if (doorbell_cs2_info->aud_running == BK_FALSE)
		{
			LOGW("%s already close\n", __func__);
			return BK_OK;
		}


		doorbell_cs2_info->aud_running = BK_FALSE;

		ret = rtos_get_semaphore(&doorbell_cs2_info->aud_sem, BEKEN_WAIT_FOREVER);

		if (ret != BK_OK)
		{
			LOGE("%s wait semaphore failed\n", __func__);
			goto error;
		}
		#endif
	}

	return BK_OK;

error:


	return BK_FAIL;
}

const doorbell_service_interface_t doorbell_cs2_service =
{
	.init = doorbell_cs2_service_init,
	.deinit = demo_doorbell_cs2_p2p_server_deinit,
	.camera_state_changed = doorbell_cs2_camera_state_changed,
	.audio_state_changed = doorbell_cs2_audio_state_changed,
	.camera_transfer_cb = &doorbell_cs2_img_channel,
	.audio_transfer_cb = &doorbell_cs2_aud_channel,
};

const doorbell_service_interface_t *get_doorbell_cs2_service_interface(void)
{
	return &doorbell_cs2_service;
}

