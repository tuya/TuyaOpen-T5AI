// Copyright 2020-2021 Beken
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

#include <driver/int.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>

#include <driver/dma.h>
#include <driver/i2c.h>
#include <driver/jpeg_enc.h>
#include <driver/jpeg_enc_types.h>

#include <driver/media_types.h>
#include <driver/dvp_camera.h>
#include <driver/dvp_camera_types.h>
#include <driver/gpio_types.h>
#include <driver/gpio.h>
#include <driver/h264.h>
#include "bk_general_dma.h"
#include <driver/psram.h>
#include <driver/yuv_buf.h>
#include <driver/video_common_driver.h>
#include <driver/aon_rtc.h>
#include <modules/image_scale.h>

#include <bk_list.h>
#include "FreeRTOS.h"
#include "event_groups.h"

#include "bk_misc.h"
#include "gpio_driver.h"
#include "mux_dvp.h"

#define TAG "dvp_drv"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define DVP_YUV_NODE_NUM  (4)
#define H264_SELF_DEFINE_SEI_SIZE (96)
#define JPEG_CRC_SIZE (5)
#define FRAME_BUFFER_CACHE (1024 * 5)

//#define YUV_DEBUG_ENABLE
#define YUV_ENCODE_EMR_USE_SRAM        (1)
#define YUV_ENCODE_ENABLE              (1)
#define YUV_ENCODE_MEM_CPY_ENABLE      (1)
#define YUV_ENCODE_CPU2_ENABLE         (1)
#define YUV_ROTATE_IN_SRAM_ENABLE      (1)

#define DVP_YUV_OUTPUT_COMPLETE_EVENT  (1 << 0)
#define DVP_YUV_ROTATE_COMPLETE_EVENT  (1 << 1)
#define DVP_YUV_ENCODE_COMPLETE_EVENT  (1 << 2)
#define DVP_YUV_ENCODE_STOP_EVENT      (1 << 3)
#define DVP_YUV_ROTATE_START_EVENT     (1 << 4)
#define DVP_YUV_ROTATE_STOP_EVENT      (1 << 5)

#define DVP_YUV_ROTATE_TASK_EXIT_EVENT (1 << 7)
#define DVP_YUV_ENCODE_TASK_EXIT_EVENT (1 << 8)
#define DVP_YUV_ENCODE_COMPLETE_BIT    (DVP_YUV_ENCODE_COMPLETE_EVENT | DVP_YUV_ENCODE_STOP_EVENT)

#define list_for_each_safe_edge(pos, n, head) \
    for (pos = (head)->next, n = pos->next; (pos != (head)) && (pos->next != pos); \
         pos = n, n = pos->next)

typedef enum
{
	DVP_CAMERA_YUV_FREE = 0,
	DVP_CAMERA_YUV_READY,
	DVP_CAMERA_YUV_BUSY,
} dvp_yuv_state_t;

typedef struct
{
	uint32_t yuv_em_addr;
	uint32_t yuv_pingpong_length;
	uint32_t yuv_data_offset;
	uint8_t dma_collect_yuv;
}encode_yuv_config_t;

typedef struct
{
	uint8_t rotate_state;
	rotate_nofity_t rot_notify;
} yuv_rotate_info_t;

typedef struct
{
	uint8_t eof : 1;
	uint8_t error : 1;
	uint8_t i_frame : 1;
	uint8_t not_free : 1;
	uint8_t regenerate_idr : 1;
	uint8_t node_init : 1;
	uint8_t sequence;

#ifdef CONFIG_H264_ADD_SELF_DEFINE_SEI
	uint8_t sei[H264_SELF_DEFINE_SEI_SIZE]; // save frame infomation
#endif

	uint8_t yuv_node_count; // save yuv data node cnt, DVP_YUV_NODE_NUM
#if YUV_ENCODE_MEM_CPY_ENABLE
	uint8_t mem_cpy_channel; // memcpy yuv data to emr_base_addr to encode
#endif
	uint8_t dma_channel; // encode fifo data cpy to psram
	uint8_t line_done_cnt; // urrent frame resolution total line done number = width / 16;
	uint8_t line_done_index; // current encode line index
	uint8_t yuv_line_cnt; // dvp output yuv frame total line, height/8(16)
	uint8_t yuv_line_index; // sram0/sram1 output index
	uint32_t frame_id; // frame id
	uint32_t yuv_pingpong_length; // dvp output pingpong buffer length
	uint32_t encode_pingpong_length; // dvp encode by line, pingpang buffer length
	uint32_t yuv_frame_length; // yuv frame buffer length
	uint32_t dma_length; // dma cpy total length from encode fifo
	uint8_t *em_base_addr; // dvp output yuv_pingpong buffer addr
	uint8_t *emr_base_addr;// dvp input yuv_pingpong buffer addr
	uint8_t *rot_rx; // rotate rx buf
	uint8_t *rot_tx; // rotae tx buf
	media_state_t dvp_state;
	beken_semaphore_t sem; // control while output complete frame, then close dvp
	frame_buffer_t *encode_frame; // output encode frame buffer
	frame_buffer_t *yuv_frame; // output yuv frame buffer
	frame_buffer_t *in_encode_frame; // input need encode frame buffer
	LIST_HEADER_T yuv_node; // save yuv_frame list
#if (YUV_ENCODE_CPU2_ENABLE)
	LIST_HEADER_T encode_reay_node;
	yuv_rotate_info_t cp1_rot_info;
	yuv_rotate_info_t cp2_rot_info;
	beken_thread_t rot_thread;
#endif
	beken_mutex_t yuv_lock;
	beken_thread_t yuv_thread; // task process rotate->h264/jpeg
	EventGroupHandle_t waiting_event; // wait event index
	uint8_t *yuv_node_buff;
	dvp_camera_config_t config;
	encode_yuv_config_t *encode_yuv_config;
	const dvp_sensor_config_t *sensor;
} dvp_driver_handle_t;

typedef struct
{
	LIST_HEADER_T list;
	frame_buffer_t frame;
	dvp_yuv_state_t state;
} dvp_yuv_node_t;

typedef struct
{
	LIST_HEADER_T list;
	frame_buffer_t *frame;
	dvp_yuv_state_t state;
} dvp_enc_node_t;

//#define DVP_DIAG_DEBUG

#ifdef DVP_DIAG_DEBUG

#define DVP_DIAG_DEBUG_INIT()                   \
	do {                                        \
		gpio_dev_unmap(GPIO_2);                 \
		bk_gpio_disable_pull(GPIO_2);           \
		bk_gpio_enable_output(GPIO_2);          \
		bk_gpio_set_output_low(GPIO_2);         \
		\
		gpio_dev_unmap(GPIO_3);                 \
		bk_gpio_disable_pull(GPIO_3);           \
		bk_gpio_enable_output(GPIO_3);          \
		bk_gpio_set_output_low(GPIO_3);         \
		\
		gpio_dev_unmap(GPIO_4);                 \
		bk_gpio_disable_pull(GPIO_4);           \
		bk_gpio_enable_output(GPIO_4);          \
		bk_gpio_set_output_low(GPIO_4);         \
		\
		gpio_dev_unmap(GPIO_5);                 \
		bk_gpio_disable_pull(GPIO_5);           \
		bk_gpio_enable_output(GPIO_5);          \
		bk_gpio_set_output_low(GPIO_5);         \
		\
		gpio_dev_unmap(GPIO_12);                \
		bk_gpio_disable_pull(GPIO_12);          \
		bk_gpio_enable_output(GPIO_12);         \
		bk_gpio_set_output_low(GPIO_12);        \
		\
		gpio_dev_unmap(GPIO_13);                \
		bk_gpio_disable_pull(GPIO_13);          \
		bk_gpio_enable_output(GPIO_13);         \
		bk_gpio_set_output_low(GPIO_13);        \
		\
	} while (0)

#define DVP_JPEG_VSYNC_ENTRY()          bk_gpio_set_output_high(GPIO_2)
#define DVP_JPEG_VSYNC_OUT()            bk_gpio_set_output_low(GPIO_2)

#define DVP_JPEG_EOF_ENTRY()            bk_gpio_set_output_high(GPIO_3)
#define DVP_JPEG_EOF_OUT()              bk_gpio_set_output_low(GPIO_3)

#define DVP_YUV_EOF_ENTRY()             bk_gpio_set_output_high(GPIO_4)
#define DVP_YUV_EOF_OUT()               bk_gpio_set_output_low(GPIO_4)

#define DVP_JPEG_START_ENTRY()          bk_gpio_set_output_high(GPIO_5)
#define DVP_JPEG_START_OUT()            bk_gpio_set_output_low(GPIO_5)

#define DVP_JPEG_HEAD_ENTRY()           bk_gpio_set_output_high(GPIO_8)
#define DVP_JPEG_HEAD_OUT()             bk_gpio_set_output_low(GPIO_8)

#define DVP_PPI_ERROR_ENTRY()           DVP_YUV_EOF_ENTRY()
#define DVP_PPI_ERROR_OUT()             DVP_YUV_EOF_OUT()

#define DVP_H264_EOF_ENTRY()            DVP_JPEG_EOF_ENTRY()
#define DVP_H264_EOF_OUT()              DVP_JPEG_EOF_OUT()

#define DVP_JPEG_SDMA_ENTRY()           bk_gpio_set_output_high(GPIO_13)
#define DVP_JPEG_SDMA_OUT()             bk_gpio_set_output_low(GPIO_13)

#define DVP_DEBUG_IO()                      \
	do {                                    \
		bk_gpio_set_output_high(GPIO_6);    \
		bk_gpio_set_output_low(GPIO_6);     \
	} while (0)

#else
#define DVP_DIAG_DEBUG_INIT()

#define DVP_JPEG_EOF_ENTRY()
#define DVP_JPEG_EOF_OUT()

#define DVP_YUV_EOF_ENTRY()
#define DVP_YUV_EOF_OUT()

#define DVP_JPEG_START_ENTRY()
#define DVP_JPEG_START_OUT()

#define DVP_JPEG_HEAD_ENTRY()
#define DVP_JPEG_HEAD_OUT()

#define DVP_PPI_ERROR_ENTRY()
#define DVP_PPI_ERROR_OUT()

#define DVP_H264_EOF_ENTRY()
#define DVP_H264_EOF_OUT()

#define DVP_JPEG_SDMA_ENTRY()
#define DVP_JPEG_SDMA_OUT()

#define DVP_JPEG_VSYNC_ENTRY()
#define DVP_JPEG_VSYNC_OUT()

#endif

extern media_debug_t *media_debug;
extern uint8_t *media_dtcm_share_buff;
extern uint8_t *media_bt_share_buffer;

dvp_sram_buffer_t *dvp_camera_encode = NULL;
static dvp_driver_handle_t *s_dvp_camera_handle = NULL;
static const dvp_sensor_config_t **devices_list = NULL;
static uint16_t devices_size = 0;


static const uint8 crc8_table[256] =
{
	0x00, 0xF7, 0xB9, 0x4E, 0x25, 0xD2, 0x9C, 0x6B,
	0x4A, 0xBD, 0xF3, 0x04, 0x6F, 0x98, 0xD6, 0x21,
	0x94, 0x63, 0x2D, 0xDA, 0xB1, 0x46, 0x08, 0xFF,
	0xDE, 0x29, 0x67, 0x90, 0xFB, 0x0C, 0x42, 0xB5,
	0x7F, 0x88, 0xC6, 0x31, 0x5A, 0xAD, 0xE3, 0x14,
	0x35, 0xC2, 0x8C, 0x7B, 0x10, 0xE7, 0xA9, 0x5E,
	0xEB, 0x1C, 0x52, 0xA5, 0xCE, 0x39, 0x77, 0x80,
	0xA1, 0x56, 0x18, 0xEF, 0x84, 0x73, 0x3D, 0xCA,
	0xFE, 0x09, 0x47, 0xB0, 0xDB, 0x2C, 0x62, 0x95,
	0xB4, 0x43, 0x0D, 0xFA, 0x91, 0x66, 0x28, 0xDF,
	0x6A, 0x9D, 0xD3, 0x24, 0x4F, 0xB8, 0xF6, 0x01,
	0x20, 0xD7, 0x99, 0x6E, 0x05, 0xF2, 0xBC, 0x4B,
	0x81, 0x76, 0x38, 0xCF, 0xA4, 0x53, 0x1D, 0xEA,
	0xCB, 0x3C, 0x72, 0x85, 0xEE, 0x19, 0x57, 0xA0,
	0x15, 0xE2, 0xAC, 0x5B, 0x30, 0xC7, 0x89, 0x7E,
	0x5F, 0xA8, 0xE6, 0x11, 0x7A, 0x8D, 0xC3, 0x34,
	0xAB, 0x5C, 0x12, 0xE5, 0x8E, 0x79, 0x37, 0xC0,
	0xE1, 0x16, 0x58, 0xAF, 0xC4, 0x33, 0x7D, 0x8A,
	0x3F, 0xC8, 0x86, 0x71, 0x1A, 0xED, 0xA3, 0x54,
	0x75, 0x82, 0xCC, 0x3B, 0x50, 0xA7, 0xE9, 0x1E,
	0xD4, 0x23, 0x6D, 0x9A, 0xF1, 0x06, 0x48, 0xBF,
	0x9E, 0x69, 0x27, 0xD0, 0xBB, 0x4C, 0x02, 0xF5,
	0x40, 0xB7, 0xF9, 0x0E, 0x65, 0x92, 0xDC, 0x2B,
	0x0A, 0xFD, 0xB3, 0x44, 0x2F, 0xD8, 0x96, 0x61,
	0x55, 0xA2, 0xEC, 0x1B, 0x70, 0x87, 0xC9, 0x3E,
	0x1F, 0xE8, 0xA6, 0x51, 0x3A, 0xCD, 0x83, 0x74,
	0xC1, 0x36, 0x78, 0x8F, 0xE4, 0x13, 0x5D, 0xAA,
	0x8B, 0x7C, 0x32, 0xC5, 0xAE, 0x59, 0x17, 0xE0,
	0x2A, 0xDD, 0x93, 0x64, 0x0F, 0xF8, 0xB6, 0x41,
	0x60, 0x97, 0xD9, 0x2E, 0x45, 0xB2, 0xFC, 0x0B,
	0xBE, 0x49, 0x07, 0xF0, 0x9B, 0x6C, 0x22, 0xD5,
	0xF4, 0x03, 0x4D, 0xBA, 0xD1, 0x26, 0x68, 0x9F
};

static uint8 hnd_crc8(
    uint8 *pdata,   /* pointer to array of data to process */
    uint32  nbytes,   /* number of input data bytes to process */
    uint8 crc   /* either CRC8_INIT_VALUE or previous return value */
)
{
	/* hard code the crc loop instead of using CRC_INNER_LOOP macro
	 * to avoid the undefined and unnecessary (uint8 >> 8) operation.
	 */
	while (nbytes-- > 0)
	{
		crc = crc8_table[(crc ^ *pdata++) & 0xff];
	}

	return crc;
}

const dvp_sensor_config_t **get_sensor_config_devices_list(void)
{
	return devices_list;
}

int get_sensor_config_devices_num(void)
{
	return devices_size;
}

void bk_dvp_camera_set_devices_list(const dvp_sensor_config_t **list, uint16_t size)
{
	devices_list = list;
	devices_size = size;
}

const dvp_sensor_config_t *get_sensor_config_interface_by_id(sensor_id_t id)
{
	uint32_t i;

	for (i = 0; i < devices_size; i++)
	{
		if (devices_list[i]->id == id)
		{
			return devices_list[i];
		}
	}

	return NULL;
}

const dvp_sensor_config_t *bk_dvp_get_sensor_auto_detect(void)
{
	uint32_t i;
	uint8_t count = 3;

	do {
		for (i = 0; i < devices_size; i++)
		{
			if (devices_list[i]->detect() == true)
			{
				return devices_list[i];
			}
		}

		count--;

		rtos_delay_milliseconds(5);

	} while (count > 0);

	return NULL;
}

static uint32_t dvp_camera_get_milliseconds(void)
{
	uint32_t time = 0;

#if CONFIG_ARCH_RISCV
	extern u64 riscv_get_mtimer(void);

	time = (riscv_get_mtimer() / 26000) & 0xFFFFFFFF;
#elif CONFIG_ARCH_CM33

#if CONFIG_AON_RTC
	time = (bk_aon_rtc_get_us() / 1000) & 0xFFFFFFFF;
#endif

#endif

	return time;
}

static bk_err_t dvp_camera_yuv_frame_buffer_node_init(dvp_driver_handle_t *handle)
{
	int ret = BK_OK;

	handle->yuv_node_count = DVP_YUV_NODE_NUM;

	uint32_t total_length = (sizeof(dvp_yuv_node_t) + handle->yuv_frame_length) * handle->yuv_node_count;

	handle->yuv_node_buff = (uint8_t *)psram_malloc(total_length);

	if (handle->yuv_node_buff == NULL)
	{
		LOGE("%s, malloc handle->yuv_node_buff error\n", __func__);
		ret = BK_ERR_NO_MEM;
		return ret;
	}

	if (handle->yuv_frame_length == 0)
	{
		BK_ASSERT_EX(0, "%s, %d\n", __func__, __LINE__);
	}
	else
	{
		LOGI("%s, yuv frame size:%d, %d\n", __func__, handle->yuv_frame_length, total_length);
	}

	INIT_LIST_HEAD(&handle->yuv_node);
	INIT_LIST_HEAD(&handle->encode_reay_node);
#if (YUV_ENCODE_CPU2_ENABLE)
	// start cp2
	vote_start_cpu2_core(CPU2_USER_JPEG_SW_DEC);
#endif

	ret = rtos_init_mutex(&handle->yuv_lock);
	if (ret != BK_OK)
	{
		LOGE("%s, handle->yuv_lock malloc failed!\r\n", __func__);
		return ret;
	}

	handle->waiting_event = xEventGroupCreate();

	if (handle->waiting_event == NULL)
	{
		LOGE("%s , create waiting_event failed\n", __func__);
		ret = BK_FAIL;
		return ret;
	}

	uint8_t *yuv_start = handle->yuv_node_buff + sizeof(dvp_yuv_node_t) * handle->yuv_node_count;

	for (uint8_t i = 0; i < handle->yuv_node_count; i++)
	{
		dvp_yuv_node_t * node = (dvp_yuv_node_t *)(handle->yuv_node_buff + sizeof(dvp_yuv_node_t) * i);

		os_memset(node, 0, sizeof(dvp_yuv_node_t));

		node->frame.frame = yuv_start + handle->yuv_frame_length * i;
		node->frame.size = handle->yuv_frame_length;
		node->state = DVP_CAMERA_YUV_FREE;

		list_add_tail(&node->list, &handle->yuv_node);
	}

	handle->node_init = true;

	return ret;
}

static bk_err_t dvp_camera_yuv_frame_buffer_node_deinit(dvp_driver_handle_t *handle)
{
	dvp_yuv_node_t *tmp = NULL;

	LIST_HEADER_T *pos, *n;

	if (handle == NULL || handle->node_init == false)
	{
		LOGW("%s, not init:%p\n", __func__, handle);
		return BK_FAIL;
	}

	GLOBAL_INT_DECLARATION() = 0;

	uint32_t isr_context = platform_is_in_interrupt_context();

	if (!isr_context)
	{
		rtos_lock_mutex(&handle->yuv_lock);
		GLOBAL_INT_DISABLE();
	}

	if (!list_empty(&handle->yuv_node))
	{
		list_for_each_safe(pos, n, &handle->yuv_node)
		{
			tmp = list_entry(pos, dvp_yuv_node_t, list);
			if (tmp != NULL)
			{
				list_del(pos);
				tmp = NULL;
			}
		}

		INIT_LIST_HEAD(&handle->yuv_node);
	}

	if (!isr_context)
	{
		GLOBAL_INT_RESTORE();
		rtos_unlock_mutex(&handle->yuv_lock);
	}

	dvp_enc_node_t *tmp1 = NULL;
	if (!list_empty(&handle->encode_reay_node))
	{
		list_for_each_safe(pos, n, &handle->encode_reay_node)
		{
			tmp1 = list_entry(pos, dvp_enc_node_t, list);
			if (tmp1 != NULL)
			{
				handle->config.fb_free(tmp1->frame);
				list_del(pos);
				os_free(tmp1);
				tmp = NULL;
			}
		}

		INIT_LIST_HEAD(&handle->encode_reay_node);
	}

	os_free(handle->yuv_node_buff);
	handle->yuv_node_buff = NULL;

#if (YUV_ENCODE_CPU2_ENABLE)
	// start cp2
	vote_stop_cpu2_core(CPU2_USER_JPEG_SW_DEC);
#endif

	handle->node_init = false;

	return BK_OK;
}

static frame_buffer_t *dvp_camera_yuv_frame_buffer_node_malloc(dvp_driver_handle_t *handle)
{
	dvp_yuv_node_t *node = NULL, *tmp = NULL;

	LIST_HEADER_T *pos, *n;

	if (handle == NULL || handle->node_init == false)
	{
		LOGW("%s, not init:%p\n", __func__, handle);
		return NULL;
	}

	GLOBAL_INT_DECLARATION() = 0;

	uint32_t isr_context = platform_is_in_interrupt_context();

	if (!isr_context)
	{
		rtos_lock_mutex(&handle->yuv_lock);
		GLOBAL_INT_DISABLE();
	}

	list_for_each_safe(pos, n, &handle->yuv_node)
	{
		tmp = list_entry(pos, dvp_yuv_node_t, list);
		if (tmp != NULL && tmp->state == DVP_CAMERA_YUV_FREE)
		{
			node = tmp;
			list_del(pos);
			break;
		}
	}

	if (node == NULL)
	{
		list_for_each_safe(pos, n, &handle->yuv_node)
		{
			tmp = list_entry(pos, dvp_yuv_node_t, list);
			if (tmp != NULL && tmp->state == DVP_CAMERA_YUV_READY)
			{
				node = tmp;
				list_del(pos);
				break;
			}
		}
	}

	if (!isr_context)
	{
		GLOBAL_INT_RESTORE();
		rtos_unlock_mutex(&handle->yuv_lock);
	}

	if (node)
	{
		node->state = DVP_CAMERA_YUV_BUSY;
		return &node->frame;
	}

	return NULL;
}

static void dvp_camera_yuv_frame_buffer_node_free(dvp_driver_handle_t *handle, frame_buffer_t *frame)
{
	GLOBAL_INT_DECLARATION();

	if (handle == NULL || handle->node_init == false)
	{
		LOGW("%s, not init:%p\n", __func__, handle);
		return;
	}

	uint32_t isr_context = platform_is_in_interrupt_context();

	if (!isr_context)
	{
		rtos_lock_mutex(&handle->yuv_lock);
		GLOBAL_INT_DISABLE();
	}

	dvp_yuv_node_t *node = list_entry(frame, dvp_yuv_node_t, frame);
	node->state = DVP_CAMERA_YUV_FREE;
	list_add_tail(&node->list, &handle->yuv_node);

	if (!isr_context)
	{
		GLOBAL_INT_RESTORE();
		rtos_unlock_mutex(&handle->yuv_lock);
	}
}

static void dvp_camera_yuv_frame_buffer_node_push(dvp_driver_handle_t *handle, frame_buffer_t *frame)
{
	if (handle == NULL || handle->node_init == false)
	{
		LOGW("%s, not init:%p\n", __func__, handle);
		return;
	}

	GLOBAL_INT_DECLARATION();

	uint32_t isr_context = platform_is_in_interrupt_context();

	if (!isr_context)
	{
		rtos_lock_mutex(&handle->yuv_lock);
		GLOBAL_INT_DISABLE();
	}

	dvp_yuv_node_t *node = list_entry(frame, dvp_yuv_node_t, frame);
	node->state = DVP_CAMERA_YUV_READY;
	list_add_tail(&node->list, &handle->yuv_node);

	if (!isr_context)
	{
		GLOBAL_INT_RESTORE();
		rtos_unlock_mutex(&handle->yuv_lock);
	}

	if (platform_is_in_interrupt_context())
	{
		BaseType_t token = pdTRUE;
		xEventGroupSetBitsFromISR(handle->waiting_event, DVP_YUV_OUTPUT_COMPLETE_EVENT, &token);
	}
	else
	{
		xEventGroupSetBits(handle->waiting_event, DVP_YUV_OUTPUT_COMPLETE_EVENT);
	}
}

static frame_buffer_t *dvp_camera_yuv_frame_buffer_node_pop(dvp_driver_handle_t *handle)
{
	LIST_HEADER_T *pos, *n;
	dvp_yuv_node_t *node = NULL, *tmp = NULL;

	if (handle == NULL || handle->node_init == false)
	{
		LOGW("%s, not init:%p\n", __func__, handle);
		return NULL;
	}

	GLOBAL_INT_DECLARATION();

	uint32_t isr_context = platform_is_in_interrupt_context();

	if (!isr_context)
	{
		rtos_lock_mutex(&handle->yuv_lock);
		GLOBAL_INT_DISABLE();
	}

	list_for_each_safe_edge(pos, n, &handle->yuv_node)
	{
		tmp = list_entry(pos, dvp_yuv_node_t, list);
		if (tmp != NULL && tmp->state == DVP_CAMERA_YUV_READY)
		{
			node = tmp;
			list_del(pos);
			break;
		}
	}

	if (!isr_context)
	{
		GLOBAL_INT_RESTORE();
		rtos_unlock_mutex(&handle->yuv_lock);
	}

	if (node)
	{
		node->state = DVP_CAMERA_YUV_BUSY;
		return &node->frame;
	}

	return NULL;
}

static bk_err_t dvp_camera_encode_node_push(dvp_driver_handle_t *handle, frame_buffer_t *frame)
{
	if (handle == NULL || handle->node_init == false)
	{
		LOGW("%s, not init:%p\n", __func__, handle);
		return BK_FAIL;
	}

	dvp_enc_node_t *node = (dvp_enc_node_t *)os_malloc(sizeof(dvp_enc_node_t));

	if (node == NULL)
	{
		LOGE("%s, %d\n", __func__, __LINE__);
		return BK_ERR_NO_MEM;
	}
	else
	{
		LOGD("%s, %p\n", __func__, node);
	}

	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();
	node->frame = frame;
	list_add_tail(&node->list, &handle->encode_reay_node);
	GLOBAL_INT_RESTORE();

	return BK_OK;
}

static frame_buffer_t *dvp_camera_encode_node_pop(dvp_driver_handle_t *handle)
{
	if (handle == NULL || handle->node_init == false)
	{
		LOGW("%s, not init:%p\n", __func__, handle);
		return NULL;
	}

	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();

	LIST_HEADER_T *pos, *n;

	dvp_enc_node_t *tmp = NULL;
	frame_buffer_t *frame = NULL;

	list_for_each_safe_edge(pos, n, &handle->encode_reay_node)
	{
		tmp = list_entry(pos, dvp_enc_node_t, list);
		if (tmp != NULL)
		{
			LOGD("%s, %p\n", __func__, tmp);
			frame = tmp->frame;
			list_del(pos);
			os_free(tmp);
			break;
		}
	}

	GLOBAL_INT_RESTORE();

	return frame;
}

static void dvp_camera_yuv_complete_handler(void *param)
{
	dvp_driver_handle_t *handle = (dvp_driver_handle_t *)param;

	DVP_YUV_EOF_ENTRY();

	if (handle == NULL)
	{
		DVP_YUV_EOF_OUT();
		return;
	}

#ifdef YUV_DEBUG_ENABLE
	LOGI("%s, %d\n", __func__, __LINE__);
#endif

	handle->yuv_line_index = 0;

	BK_WHILE(bk_dma_get_enable_status(handle->encode_yuv_config->dma_collect_yuv));

	handle->encode_yuv_config->yuv_data_offset = 0;
	bk_dma_stop(handle->encode_yuv_config->dma_collect_yuv);
	bk_dma_flush_src_buffer(handle->encode_yuv_config->dma_collect_yuv);
	frame_buffer_t *new_frame = NULL;

	if (handle->config.device->rot_angle == ROTATE_NONE)
	{
		new_frame = handle->config.fb_malloc(FB_INDEX_DISPLAY, handle->yuv_frame_length);
	}
	else
	{
		new_frame = dvp_camera_yuv_frame_buffer_node_malloc(handle);
	}

	if (new_frame)
	{
		new_frame->fmt = handle->sensor->fmt;
		new_frame->width = handle->yuv_frame->width;
		new_frame->height = handle->yuv_frame->height;
		new_frame->fmt = handle->yuv_frame->fmt;
		new_frame->length = handle->yuv_frame_length;

		if (handle->config.device->rot_angle == ROTATE_NONE)
		{
			handle->yuv_frame->sequence = handle->frame_id++;
			handle->config.fb_complete(handle->yuv_frame);
		}
		else
		{
			handle->yuv_frame->sequence = handle->frame_id++;
			dvp_camera_yuv_frame_buffer_node_push(handle, handle->yuv_frame);
		}

		handle->yuv_frame = new_frame;
	}

	DVP_YUV_EOF_OUT();
}

static bk_err_t dvp_camera_init_device(dvp_driver_handle_t *handle)
{
	dvp_camera_config_t *config = &handle->config;
	const dvp_sensor_config_t *sensor = handle->sensor;

	if (config->device->info.resolution.width != (sensor->def_ppi >> 16) ||
		config->device->info.resolution.height != (sensor->def_ppi & 0xFFFF))
	{
		if (!(pixel_ppi_to_cap((config->device->info.resolution.width << 16)
			| config->device->info.resolution.height) & (sensor->ppi_cap)))
		{
			LOGE("%s, %d, not support this resolution...\r\n", __func__, __LINE__);
			return BK_FAIL;
		}
	}

	return BK_OK;
}

static void dvp_camera_dma_finish_callback(dma_id_t id)
{
	DVP_JPEG_SDMA_ENTRY();

	dvp_driver_handle_t *handle = s_dvp_camera_handle;

	if (handle && handle->dvp_state == MASTER_TURN_ON)
	{
		handle->dma_length += FRAME_BUFFER_CACHE;
	}

	DVP_JPEG_SDMA_OUT();
}

static bk_err_t dvp_camera_dma_config(dvp_driver_handle_t *handle)
{
	bk_err_t ret = BK_OK;
	dma_config_t dma_config = {0};
	uint32_t encode_fifo_addr;

	dvp_camera_config_t *config = &handle->config;

	if (config->device->mode == H264_MODE || config->device->mode == H264_YUV_MODE)
	{
		handle->encode_frame = config->fb_malloc(FB_INDEX_H264, CONFIG_H264_FRAME_SIZE);
	}
	else // MJPEG || MIX
	{
		handle->encode_frame = config->fb_malloc(FB_INDEX_JPEG, CONFIG_JPEG_FRAME_SIZE);
	}

	if (handle->encode_frame == NULL)
	{
		LOGE("malloc frame fail \r\n");
		ret = BK_FAIL;
		return ret;
	}

	handle->encode_frame->type = config->device->type;
	handle->encode_frame->fmt = config->device->fmt;
	handle->encode_frame->width = config->device->info.resolution.width;
	handle->encode_frame->height = config->device->info.resolution.height;

	if (config->device->mode == H264_MODE || config->device->mode == H264_YUV_MODE)
	{
		bk_h264_get_fifo_addr(&encode_fifo_addr);
		handle->dma_channel = bk_fixed_dma_alloc(DMA_DEV_H264, DMA_ID_8);
	}
	else // JPEG || MIX
	{
		bk_jpeg_enc_get_fifo_addr(&encode_fifo_addr);
		handle->dma_channel = bk_fixed_dma_alloc(DMA_DEV_JPEG, DMA_ID_8);
	}

	if ((handle->dma_channel < DMA_ID_0) || (handle->dma_channel >= DMA_ID_MAX))
	{
		LOGE("malloc dma fail \r\n");
		config->fb_free(handle->encode_frame);
		handle->encode_frame = NULL;
		ret = BK_FAIL;
		return ret;
	}

	LOGI("dvp_dma id:%d \r\n", handle->dma_channel);

	dma_config.mode = DMA_WORK_MODE_REPEAT;
	dma_config.chan_prio = 0;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = encode_fifo_addr;
	dma_config.dst.dev = DMA_DEV_DTCM;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;

	if (config->device->mode == H264_MODE || config->device->mode == H264_YUV_MODE)
	{
		dma_config.src.dev = DMA_DEV_H264;
	}
	else // JPEG || MIX
	{
		dma_config.src.dev = DMA_DEV_JPEG;
	}

	dma_config.dst.start_addr = (uint32_t)handle->encode_frame->frame;
	dma_config.dst.end_addr = (uint32_t)(handle->encode_frame->frame + handle->encode_frame->size);
	BK_RETURN_ON_ERR(bk_dma_init(handle->dma_channel, &dma_config));
	BK_RETURN_ON_ERR(bk_dma_set_transfer_len(handle->dma_channel, FRAME_BUFFER_CACHE));

	BK_RETURN_ON_ERR(bk_dma_register_isr(handle->dma_channel, NULL, dvp_camera_dma_finish_callback));
	BK_RETURN_ON_ERR(bk_dma_enable_finish_interrupt(handle->dma_channel));
#if (CONFIG_SPE)
	BK_RETURN_ON_ERR(bk_dma_set_src_burst_len(handle->dma_channel, BURST_LEN_SINGLE));
	BK_RETURN_ON_ERR(bk_dma_set_dest_burst_len(handle->dma_channel, BURST_LEN_INC16));
	BK_RETURN_ON_ERR(bk_dma_set_dest_sec_attr(handle->dma_channel, DMA_ATTR_SEC));
	BK_RETURN_ON_ERR(bk_dma_set_src_sec_attr(handle->dma_channel, DMA_ATTR_SEC));
#endif
	BK_RETURN_ON_ERR(bk_dma_start(handle->dma_channel));

	return ret;
}

static bk_err_t dvp_camera_yuv_dma_cpy(void *out, const void *in, uint32_t len, dma_id_t cpy_chnl)
{
	dma_config_t dma_config = {0};
	os_memset(&dma_config, 0, sizeof(dma_config_t));

	dma_config.mode = DMA_WORK_MODE_SINGLE;
	dma_config.chan_prio = 1;

	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.src.start_addr = (uint32_t)in;
	dma_config.src.end_addr = (uint32_t)(in + len);

	dma_config.dst.dev = DMA_DEV_DTCM;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.start_addr = (uint32_t)out;
	dma_config.dst.end_addr = (uint32_t)(out + len);

	BK_RETURN_ON_ERR(bk_dma_init(cpy_chnl, &dma_config));
	BK_RETURN_ON_ERR(bk_dma_set_transfer_len(cpy_chnl, len));
#if (CONFIG_SPE && CONFIG_GDMA_HW_V2PX)
	BK_RETURN_ON_ERR(bk_dma_set_src_burst_len(cpy_chnl, 3));
	BK_RETURN_ON_ERR(bk_dma_set_dest_burst_len(cpy_chnl, 3));
	BK_RETURN_ON_ERR(bk_dma_set_dest_sec_attr(cpy_chnl, DMA_ATTR_SEC));
	BK_RETURN_ON_ERR(bk_dma_set_src_sec_attr(cpy_chnl, DMA_ATTR_SEC));
#endif

	return BK_OK;
}

static void dvp_camera_reset_hardware_modules_handler(dvp_driver_handle_t *handle)
{
	if (handle == NULL)
	{
		return;
	}

	yuv_mode_t mode = handle->config.device->mode;

	if (mode == JPEG_MODE || mode == JPEG_YUV_MODE)
	{
		bk_jpeg_enc_soft_reset();
	}

	if (mode == H264_MODE || mode == H264_YUV_MODE)
	{
		bk_h264_config_reset();
		bk_video_encode_start(H264_MODE);
	}

	//if (handle->node_init == false)
	{
		bk_yuv_buf_soft_reset();
	}

	if (handle->dma_channel < DMA_ID_MAX)
	{
		bk_dma_stop(handle->dma_channel);
		handle->dma_length = 0;
		if (handle->encode_frame)
		{
			handle->encode_frame->length = 0;
		}
		bk_dma_start(handle->dma_channel);
	}
}

static void dvp_camera_sensor_ppi_err_handler(yuv_buf_unit_t id, void *param)
{
	DVP_PPI_ERROR_ENTRY();

	dvp_driver_handle_t *handle = (dvp_driver_handle_t *)param;

	if (!handle->error)
	{
		handle->error = true;
	}

	DVP_PPI_ERROR_OUT();
}

static void dvp_camera_yuv_sm0_line_done(yuv_buf_unit_t id, void *param)
{
	dvp_driver_handle_t *handle = (dvp_driver_handle_t *)param;

	if (handle == NULL || handle->dvp_state != MASTER_TURN_ON)
	{
		return;
	}

	if ((handle->encode_yuv_config->yuv_data_offset + handle->encode_yuv_config->yuv_pingpong_length) > handle->yuv_frame->length)
	{
		handle->encode_yuv_config->yuv_data_offset = 0;
	}

	BK_WHILE(bk_dma_get_enable_status(handle->encode_yuv_config->dma_collect_yuv));
	bk_dma_stop(handle->encode_yuv_config->dma_collect_yuv);
	bk_dma_set_src_start_addr(handle->encode_yuv_config->dma_collect_yuv,
							  (uint32_t)handle->encode_yuv_config->yuv_em_addr);
	bk_dma_set_dest_start_addr(handle->encode_yuv_config->dma_collect_yuv,
							   (uint32_t)(handle->yuv_frame->frame + handle->encode_yuv_config->yuv_data_offset));
	bk_dma_start(handle->encode_yuv_config->dma_collect_yuv);
	handle->encode_yuv_config->yuv_data_offset += handle->encode_yuv_config->yuv_pingpong_length;

	handle->yuv_line_index++;
	if (handle->yuv_line_cnt == handle->yuv_line_index)
	{
		dvp_camera_yuv_complete_handler(handle);
	}
}

static void dvp_camera_yuv_sm1_line_done(yuv_buf_unit_t id, void *param)
{
	dvp_driver_handle_t *handle = (dvp_driver_handle_t *)param;

	if (handle == NULL || handle->dvp_state != MASTER_TURN_ON)
	{
		return;
	}

	if ((handle->encode_yuv_config->yuv_data_offset + handle->encode_yuv_config->yuv_pingpong_length) > handle->yuv_frame->length)
	{
		handle->encode_yuv_config->yuv_data_offset = 0;
	}

	BK_WHILE(bk_dma_get_enable_status(handle->encode_yuv_config->dma_collect_yuv));
	bk_dma_stop(handle->encode_yuv_config->dma_collect_yuv);
	bk_dma_set_src_start_addr(handle->encode_yuv_config->dma_collect_yuv,
							  (uint32_t)(handle->encode_yuv_config->yuv_em_addr + handle->encode_yuv_config->yuv_pingpong_length));
	bk_dma_set_dest_start_addr(handle->encode_yuv_config->dma_collect_yuv,
							   (uint32_t)(handle->yuv_frame->frame + handle->encode_yuv_config->yuv_data_offset));
	bk_dma_start(handle->encode_yuv_config->dma_collect_yuv);
	handle->encode_yuv_config->yuv_data_offset += handle->encode_yuv_config->yuv_pingpong_length;

	handle->yuv_line_index++;
	if (handle->yuv_line_cnt == handle->yuv_line_index)
	{
		dvp_camera_yuv_complete_handler(handle);
	}
}

static void dvp_camera_vsync_negedge_handler(jpeg_unit_t id, void *param)
{
	DVP_JPEG_VSYNC_ENTRY();

	dvp_driver_handle_t *handle = (dvp_driver_handle_t *)param;

	if (handle && handle->dvp_state == MASTER_TURNING_OFF)
	{
		bk_video_encode_stop(YUV_MODE);
		bk_video_encode_stop(JPEG_MODE);
		bk_video_encode_stop(H264_MODE);

		if (handle->sem != NULL)
		{
			rtos_set_semaphore(&handle->sem);
		}

		goto out;
	}

	if (handle->error)
	{
		handle->error = false;
		handle->sequence = 0;
		dvp_camera_reset_hardware_modules_handler(handle);
		LOGI("reset OK \r\n");
		goto out;
	}

out:
	DVP_JPEG_VSYNC_OUT();
}

static void dvp_camera_yuv_eof_handler(jpeg_unit_t id, void *param)
{
	frame_buffer_t *new_frame = NULL;

	dvp_driver_handle_t *handle = (dvp_driver_handle_t *)param;

	DVP_YUV_EOF_ENTRY();

	if (handle == NULL || handle->dvp_state != MASTER_TURN_ON)
	{
		goto out;
	}

	media_debug->isr_jpeg++;

	handle->yuv_frame->sequence = handle->frame_id++;

	LOGI("%s, seq:%d\n", __func__, handle->yuv_frame->sequence);

	new_frame = handle->config.fb_malloc(FB_INDEX_DISPLAY, handle->yuv_frame_length);

	if (new_frame)
	{
		new_frame->width = handle->yuv_frame->width;
		new_frame->height = handle->yuv_frame->height;
		new_frame->fmt = handle->sensor->fmt;
		new_frame->type = DVP_CAMERA;
		new_frame->length = handle->yuv_frame_length;
		handle->config.fb_complete(handle->yuv_frame);
		handle->yuv_frame = new_frame;
	}

	bk_yuv_buf_set_em_base_addr((uint32_t)handle->yuv_frame->frame);

out:

	DVP_YUV_EOF_OUT();
}

static void dvp_camera_jpeg_eof_handler(jpeg_unit_t id, void *param)
{
	DVP_JPEG_EOF_ENTRY();

	uint32_t real_length = 0, recv_length = 0;

	dvp_driver_handle_t *handle = (dvp_driver_handle_t *)param;

	if (handle == NULL || handle->dvp_state != MASTER_TURN_ON)
	{
		goto out;
	}

	media_debug->isr_jpeg++;

	bk_dma_flush_src_buffer(handle->dma_channel);

	real_length = bk_jpeg_enc_get_frame_size();

	recv_length = FRAME_BUFFER_CACHE - bk_dma_get_remain_len(handle->dma_channel);

	bk_dma_stop(handle->dma_channel);

	handle->dma_length += recv_length - JPEG_CRC_SIZE;

	if (handle->dma_length != real_length)
	{
		LOGW("%s size no match:%d-%d=%d\r\n", __func__, real_length, handle->dma_length, real_length - handle->dma_length);
	}

	handle->dma_length = 0;

	/*there not compare dma copy total length and read from jpeg register length, because register count is error*/
	if (handle->config.device->mode == JPEG_YUV_MODE)
	{
		handle->encode_frame->mix = true;
	}

	for (uint32_t i = real_length; i > real_length - 10; i--)
	{
		if (handle->encode_frame->frame[i - 1] == 0xD9
			&& handle->encode_frame->frame[i - 2] == 0xFF)
		{
			real_length = i + 1;
			handle->eof = true;
			break;
		}

		handle->eof = false;
	}

	if (handle->eof)
	{
		handle->encode_frame->length = real_length;
		if (handle->node_init)
		{
			handle->encode_frame->sequence = handle->in_encode_frame->sequence;
		}
		else
		{
			if (handle->config.device->mode == JPEG_MODE)
			{
				handle->encode_frame->sequence = handle->frame_id++;
			}
			else
			{
				handle->encode_frame->sequence = handle->frame_id;
			}
		}

		LOGD("%s, seq:%d\n", __func__, handle->encode_frame->sequence);

		frame_buffer_t *frame_buffer = handle->config.fb_malloc(FB_INDEX_JPEG, CONFIG_JPEG_FRAME_SIZE);

		if (frame_buffer == NULL)
		{
			handle->encode_frame->length = 0;
		}
		else
		{
			handle->encode_frame->length = real_length;
			frame_buffer->width = handle->encode_frame->width;
			frame_buffer->height = handle->encode_frame->height;
			frame_buffer->fmt = PIXEL_FMT_JPEG;
			frame_buffer->type = DVP_CAMERA;
			handle->config.fb_complete(handle->encode_frame);
			handle->encode_frame = frame_buffer;
		}

		handle->eof = false;
	}
	else
	{
		handle->encode_frame->length = 0;
	}

	bk_dma_set_dest_addr(handle->dma_channel, (uint32_t)handle->encode_frame->frame, (uint32_t)(handle->encode_frame->frame + handle->encode_frame->size));
	bk_dma_start(handle->dma_channel);

out:

	if (handle->node_init)
	{
		BaseType_t token = pdTRUE;
		xEventGroupSetBitsFromISR(handle->waiting_event, DVP_YUV_ENCODE_COMPLETE_EVENT, &token);
	}

	DVP_JPEG_EOF_OUT();
}

static void dvp_camera_h264_line_down_handler(h264_unit_t id, void *param)
{
	dvp_driver_handle_t *handle = (dvp_driver_handle_t *)param;

	if (handle == NULL || handle->dvp_state != MASTER_TURN_ON)
	{
		return;
	}

	if (handle->config.device->rot_angle == ROTATE_NONE)
	{
		return;
	}

	handle->line_done_index++;

	uint32_t offset = handle->encode_pingpong_length >> 1;

	LOGD("%s, %d, length:%d, line:%d-%d\n", __func__, __LINE__, offset, handle->line_done_index, handle->line_done_cnt);

	if (handle->line_done_index < handle->line_done_cnt)
	{
		if (handle->line_done_index % 2 == 0)
		{
#if YUV_ENCODE_MEM_CPY_ENABLE
			bk_dma_set_src_start_addr(handle->mem_cpy_channel,
								  (uint32_t)(handle->in_encode_frame->frame + handle->line_done_index * offset));
			bk_dma_set_dest_start_addr(handle->mem_cpy_channel,
								   (uint32_t)handle->emr_base_addr);
			bk_dma_start(handle->mem_cpy_channel);
			BK_WHILE(bk_dma_get_enable_status(handle->mem_cpy_channel));
#else
			os_memcpy(handle->emr_base_addr, handle->in_encode_frame->frame + offset * handle->line_done_index, offset);
#endif
		}
		else
		{
#if YUV_ENCODE_MEM_CPY_ENABLE
			bk_dma_set_src_start_addr(handle->mem_cpy_channel,
								  (uint32_t)(handle->in_encode_frame->frame + handle->line_done_index * offset));
			bk_dma_set_dest_start_addr(handle->mem_cpy_channel,
								   (uint32_t)handle->emr_base_addr + offset);
			bk_dma_start(handle->mem_cpy_channel);
			BK_WHILE(bk_dma_get_enable_status(handle->mem_cpy_channel));
#else
			os_memcpy(handle->emr_base_addr + offset, handle->in_encode_frame->frame + offset * handle->line_done_index, offset);
#endif
		}

		bk_yuv_buf_rencode_start();
	}
}

static void dvp_camera_h264_eof_handler(h264_unit_t id, void *param)
{
	uint32_t real_length = 0, remain_length = 0;
	frame_buffer_t *new_frame = NULL;

	dvp_driver_handle_t *handle = (dvp_driver_handle_t *)param;

	if (handle == NULL || handle->dvp_state != MASTER_TURN_ON)
	{
		return;
	}

	DVP_H264_EOF_ENTRY();

	handle->sequence++;

	if (handle->sequence > H264_GOP_FRAME_CNT)
	{
		handle->sequence = 1;
	}

	if (handle->sequence == 1)
	{
		handle->i_frame = 1;
	}
	else
	{
		handle->i_frame = 0;
	}


#if (CONFIG_H264_GOP_START_IDR_FRAME)
	if (handle->sequence == H264_GOP_FRAME_CNT)
	{
		handle->regenerate_idr = true;
	}
#endif

	bk_dma_flush_src_buffer(handle->dma_channel);

	remain_length = FRAME_BUFFER_CACHE - bk_dma_get_remain_len(handle->dma_channel);

	bk_dma_stop(handle->dma_channel);

	handle->dma_length += remain_length;

	real_length = bk_h264_get_encode_count() * 4;

	if (real_length > CONFIG_H264_FRAME_SIZE - 0x20)
	{
		LOGE("%s size over h264 buffer range, %d\r\n", __func__, real_length);
		handle->error = true;
	}
	else
	{
		if (handle->dma_length != real_length)
		{
			int left_length = real_length - handle->dma_length;
			if (left_length != FRAME_BUFFER_CACHE)
			{
				LOGW("%s size no match:%d-%d=%d\r\n", __func__, real_length, handle->dma_length, left_length);
				handle->error = true;
			}
		}
	}

	handle->dma_length = 0;

	if (handle->error)
	{
		handle->encode_frame->length = 0;
		handle->sequence = 0;
		handle->regenerate_idr = true;
		goto out;
	}

	media_debug->isr_h264++;

	if (handle->config.device->mode == H264_YUV_MODE)
	{
		handle->encode_frame->mix = true;
	}

	handle->encode_frame->length = real_length;

	if (handle->node_init)
	{
		handle->encode_frame->sequence = handle->in_encode_frame->sequence;
	}
	else
	{
		if (handle->config.device->mode == H264_MODE)
		{
			handle->encode_frame->sequence = handle->frame_id++;
		}
		else
		{
			handle->encode_frame->sequence = handle->frame_id;
		}
	}

	media_debug->h264_length = handle->encode_frame->length;
	media_debug->h264_kbps += handle->encode_frame->length;

	if (handle->i_frame)
	{
		handle->encode_frame->h264_type |= 1 << H264_NAL_I_FRAME;
#if (CONFIG_H264_GOP_START_IDR_FRAME)
		handle->encode_frame->h264_type |= (1 << H264_NAL_SPS) | (1 << H264_NAL_PPS) | (1 << H264_NAL_IDR_SLICE);
#endif
	}
	else
	{
		handle->encode_frame->h264_type |= 1 << H264_NAL_P_FRAME;
	}

	handle->encode_frame->timestamp = dvp_camera_get_milliseconds();

#ifdef CONFIG_H264_ADD_SELF_DEFINE_SEI
	handle->encode_frame->crc = hnd_crc8(handle->encode_frame->frame, handle->encode_frame->length, 0xFF);
	handle->encode_frame->length += H264_SELF_DEFINE_SEI_SIZE;
	os_memcpy(&handle->sei[23], (uint8_t *)handle->encode_frame, sizeof(frame_buffer_t));
	os_memcpy(&handle->encode_frame->frame[handle->encode_frame->length - H264_SELF_DEFINE_SEI_SIZE], &handle->sei[0], H264_SELF_DEFINE_SEI_SIZE);
#endif

	LOGD("%d, I:%d, p:%d\r\n", handle->encode_frame->length, (handle->encode_frame->h264_type & 0x1000020) > 0 ? 1 : 0, (handle->encode_frame->h264_type >> 23) & 0x1);

	new_frame = handle->config.fb_malloc(FB_INDEX_H264, CONFIG_H264_FRAME_SIZE);
	if (new_frame)
	{
		new_frame->width = handle->encode_frame->width;
		new_frame->height = handle->encode_frame->height;
		new_frame->fmt = PIXEL_FMT_H264;
		new_frame->type = DVP_CAMERA;
		handle->config.fb_complete(handle->encode_frame);
		handle->encode_frame = new_frame;
	}
	else
	{
		handle->regenerate_idr = true;
	}

	handle->encode_frame->length = 0;
	handle->encode_frame->h264_type = 0;

	bk_dma_set_dest_addr(handle->dma_channel, (uint32_t)(handle->encode_frame->frame), (uint32_t)(handle->encode_frame->frame + handle->encode_frame->size));
	bk_dma_start(handle->dma_channel);

out:

	if (handle->regenerate_idr)
	{
		handle->regenerate_idr = false;
		handle->sequence = 0;
		bk_h264_soft_reset();
	}

	if (handle->node_init)
	{
#ifdef YUV_DEBUG_ENABLE
		LOGI("%s, %d\n", __func__, __LINE__);
#endif
		BaseType_t token = pdTRUE;
		xEventGroupSetBitsFromISR(handle->waiting_event, DVP_YUV_ENCODE_COMPLETE_EVENT, &token);
	}

	DVP_H264_EOF_OUT();
}

static bk_err_t dvp_camera_jpeg_config_init(dvp_driver_handle_t *handle)
{
	int ret = BK_OK;
	jpeg_config_t jpeg_config = {0};

	dvp_camera_config_t *config = &handle->config;
	const dvp_sensor_config_t *sensor = handle->sensor;

	switch (handle->config.device->rot_angle)
	{
		case ROTATE_90:
		case ROTATE_270:
			jpeg_config.x_pixel = config->device->info.resolution.height / 8;
			jpeg_config.y_pixel = config->device->info.resolution.width / 8;
			break;

		case ROTATE_NONE:
		case ROTATE_180:
			jpeg_config.x_pixel = config->device->info.resolution.width / 8;
			jpeg_config.y_pixel = config->device->info.resolution.height / 8;
			break;
	}

	jpeg_config.vsync = sensor->vsync;
	jpeg_config.hsync = sensor->hsync;
	jpeg_config.clk = sensor->clk;
	jpeg_config.mode = JPEG_MODE;

	switch (sensor->fmt)
	{
		case PIXEL_FMT_YUYV:
			jpeg_config.sensor_fmt = YUV_FORMAT_YUYV;
			break;

		case PIXEL_FMT_UYVY:
			jpeg_config.sensor_fmt = YUV_FORMAT_UYVY;
			break;

		case PIXEL_FMT_YYUV:
			jpeg_config.sensor_fmt = YUV_FORMAT_YYUV;
			break;

		case PIXEL_FMT_UVYY:
			jpeg_config.sensor_fmt = YUV_FORMAT_UVYY;
			break;

		default:
			LOGE("JPEG MODULE not support this sensor input format\r\n");
			ret = BK_FAIL;
			return ret;
	}

	ret = bk_jpeg_enc_init(&jpeg_config);
	if (ret != BK_OK)
	{
		LOGE("jpeg init error\n");
	}

	return ret;
}

bk_err_t dvp_camera_yuv_buf_config_init(dvp_driver_handle_t *handle, yuv_mode_t mode)
{
	int ret = BK_OK;

	dvp_camera_config_t *config = &handle->config;

	yuv_buf_config_t yuv_mode_config = {0};

	yuv_mode_config.work_mode = mode;
	yuv_mode_config.mclk_div = YUV_MCLK_DIV_3;

	if (mode != GRAY_MODE)
	{
		yuv_mode_config.x_pixel = config->device->info.resolution.width / 8;
	}
	else
	{
		yuv_mode_config.x_pixel = config->device->info.resolution.width / 8 / 2;
	}

	yuv_mode_config.y_pixel = config->device->info.resolution.height / 8;
	yuv_mode_config.yuv_mode_cfg.vsync = handle->sensor->vsync;
	yuv_mode_config.yuv_mode_cfg.hsync = handle->sensor->hsync;

	LOGI("%s, %d-%d, mode:%d\r\n", __func__, config->device->info.resolution.width, config->device->info.resolution.height, mode);
	switch (handle->sensor->fmt)
	{
		case PIXEL_FMT_YUYV:
			yuv_mode_config.yuv_mode_cfg.yuv_format = YUV_FORMAT_YUYV;
			break;

		case PIXEL_FMT_UYVY:
			yuv_mode_config.yuv_mode_cfg.yuv_format = YUV_FORMAT_UYVY;
			break;

		case PIXEL_FMT_YYUV:
			yuv_mode_config.yuv_mode_cfg.yuv_format = YUV_FORMAT_YYUV;
			break;

		case PIXEL_FMT_UVYY:
			yuv_mode_config.yuv_mode_cfg.yuv_format = YUV_FORMAT_UVYY;
			break;

		default:
			LOGE("YUV_BUF MODULE not support this sensor input format\r\n");
			ret = BK_FAIL;
	}

	if (ret != BK_OK)
	{
		return ret;
	}

	yuv_mode_config.base_addr = handle->em_base_addr;
	yuv_mode_config.emr_base_addr = handle->emr_base_addr;

	ret = bk_yuv_buf_init(&yuv_mode_config);
	if (ret != BK_OK)
	{
		LOGE("yuv_buf yuv mode init error\n");
		return ret;
	}

	if (config->device->rot_angle != ROTATE_NONE)
	{
		yuv_buf_resize_config_t resize_config = {0};
		switch (config->device->rot_angle)
		{
			case ROTATE_90:
			case ROTATE_270:
				resize_config.x_pixel_resize = config->device->info.resolution.height;
				resize_config.y_pixel_resize = config->device->info.resolution.width;
				break;

			case ROTATE_180:
				resize_config.x_pixel_resize = config->device->info.resolution.width;
				resize_config.y_pixel_resize = config->device->info.resolution.height;
				break;

			default:
				LOGE("%s, param error\n", __func__);
				ret = BK_FAIL;
		}

		if (ret != BK_OK)
		{
			return ret;
		}

		resize_config.emr_base_addr = handle->emr_base_addr;
		ret = bk_yuv_buf_set_resize(&resize_config);
	}

	return ret;
}

static bk_err_t dvp_camera_yuv_mode(dvp_driver_handle_t *handle)
{
	LOGI("%s, %d\r\n", __func__, __LINE__);

	int ret = BK_OK;

	dvp_camera_config_t *config = &handle->config;

	ret = dvp_camera_yuv_buf_config_init(handle, YUV_MODE);
	if (ret != BK_OK)
	{
		return ret;
	}

	config->fb_init(FB_INDEX_DISPLAY);

	if (config->device->rot_angle == ROTATE_NONE)
	{
		handle->yuv_frame = config->fb_malloc(FB_INDEX_DISPLAY, handle->yuv_frame_length);
	}
	else
	{
		handle->yuv_frame = NULL;
	}

	if (handle->yuv_frame == NULL)
	{
		LOGE("malloc frame fail \r\n");
		ret = BK_FAIL;
		return ret;
	}

	handle->yuv_frame->width = config->device->info.resolution.width;
	handle->yuv_frame->height = config->device->info.resolution.height;
	handle->yuv_frame->fmt = handle->sensor->fmt;
	handle->yuv_frame->length = handle->yuv_frame_length;

	bk_yuv_buf_set_em_base_addr((uint32_t)handle->yuv_frame->frame);

	return ret;
}

static bk_err_t dvp_camera_jpeg_mode(dvp_driver_handle_t *handle)
{
	LOGI("%s, %d\r\n", __func__, __LINE__);
	int ret = BK_OK;

	dvp_camera_config_t *config = &handle->config;

	config->fb_init(FB_INDEX_JPEG);

	ret = dvp_camera_dma_config(handle);

	if (ret != BK_OK)
	{
		LOGE("dma init failed\n");
		return ret;
	}

	ret = dvp_camera_yuv_buf_config_init(handle, JPEG_MODE);
	if (ret != BK_OK)
	{
		return ret;
	}

	ret = dvp_camera_jpeg_config_init(handle);

	return ret;
}

static bk_err_t dvp_camera_h264_mode(dvp_driver_handle_t *handle)
{
	LOGI("%s, %d\r\n", __func__, __LINE__);
	int ret = BK_OK;

	dvp_camera_config_t *config = &handle->config;

	ret = dvp_camera_yuv_buf_config_init(handle, H264_MODE);
	if (ret != BK_OK)
	{
		return ret;
	}

	switch (handle->config.device->rot_angle)
	{
		case ROTATE_90:
		case ROTATE_270:
			ret = bk_h264_init((config->device->info.resolution.height << 16) | config->device->info.resolution.width);
			break;

		case ROTATE_NONE:
		case ROTATE_180:
			ret = bk_h264_init((config->device->info.resolution.width << 16) | config->device->info.resolution.height);
			break;
	}

	if (ret != BK_OK)
	{
		return ret;
	}

	config->fb_init(FB_INDEX_H264);

	ret = dvp_camera_dma_config(handle);
	if (ret != BK_OK)
	{
		LOGE("dma init failed\n");
		return ret;
	}

#ifdef CONFIG_H264_ADD_SELF_DEFINE_SEI
	os_memset(&handle->sei[0], 0xFF, H264_SELF_DEFINE_SEI_SIZE);

	h264_encode_sei_init(&handle->sei[0]);
#endif

	return ret;
}

static void dvp_camera_register_isr_function(dvp_driver_handle_t *handle)
{
	yuv_mode_t mode = handle->config.device->mode;

	LOGI("%s, %d, mode:%d\r\n", __func__, __LINE__, mode);

	switch (mode)
	{
		case GRAY_MODE:
		case YUV_MODE:
			bk_yuv_buf_register_isr(YUV_BUF_YUV_ARV, dvp_camera_yuv_eof_handler, (void *)handle);
			break;

		case JPEG_YUV_MODE:
		case JPEG_MODE:
			bk_jpeg_enc_register_isr(JPEG_EOF, dvp_camera_jpeg_eof_handler, (void *)handle);
			bk_jpeg_enc_register_isr(JPEG_FRAME_ERR, dvp_camera_sensor_ppi_err_handler, (void *)handle);
			if (handle->config.device->rot_angle == ROTATE_NONE)
			{
				//bk_yuv_buf_register_isr(YUV_BUF_ENC_SLOW, dvp_camera_sensor_ppi_err_handler, (void *)handle);
			}
			break;

		case H264_YUV_MODE:
		case H264_MODE:
			bk_h264_register_isr(H264_FINAL_OUT, dvp_camera_h264_eof_handler, (void *)handle);
			if (handle->config.device->rot_angle != ROTATE_NONE)
			{
				bk_h264_register_isr(H264_LINE_DONE, dvp_camera_h264_line_down_handler, (void *)handle);
			}
			break;

		default:
			break;
	}

	if (mode == JPEG_YUV_MODE || mode == H264_YUV_MODE)
	{
		bk_yuv_buf_register_isr(YUV_BUF_SM0_WR, dvp_camera_yuv_sm0_line_done, (void *)handle);
		bk_yuv_buf_register_isr(YUV_BUF_SM1_WR, dvp_camera_yuv_sm1_line_done, (void *)handle);
	}

	bk_yuv_buf_register_isr(YUV_BUF_VSYNC_NEGEDGE, dvp_camera_vsync_negedge_handler, (void *)handle);
	bk_yuv_buf_register_isr(YUV_BUF_SEN_RESL, dvp_camera_sensor_ppi_err_handler, (void *)handle);
	if (handle->config.device->rot_angle == ROTATE_NONE)
	{
		bk_yuv_buf_register_isr(YUV_BUF_FULL, dvp_camera_sensor_ppi_err_handler, (void *)handle);
	}
}

// Modified by TUYA Start
#include "tuya_cloud_types.h"
// Modified by TUYA End
const dvp_sensor_config_t *bk_dvp_camera_enumerate(void)
{
	const dvp_sensor_config_t *sensor = NULL;

	// step 1: power on video modules
    // Modified by TUYA Start
#if CONFIG_TUYA_LOGIC_MODIFY
    extern OPERATE_RET tkl_vi_get_power_info(uint8_t device_type, uint8_t *io, uint8_t *active);
    uint8_t dvp_ldo = 56, dvp_active_level = 0;
    tkl_vi_get_power_info(DVP_CAMERA, &dvp_ldo, &dvp_active_level);
	bk_video_power_on(dvp_ldo, dvp_active_level);
#else
	bk_video_power_on(CONFIG_CAMERA_CTRL_POWER_GPIO_ID, 1);
#endif
    // Modified by TUYA End

	// step 2: map gpio as MCLK, PCLK for i2c communicate with dvp
	bk_video_gpio_init(DVP_GPIO_ALL);

	// step 3: enable mclk for i2c communicate with dvp
	bk_video_dvp_mclk_enable(YUV_MODE);

    // Modified by TUYA Start
#if CONFIG_TUYA_LOGIC_MODIFY
    // do nothing
#else
	i2c_config_t i2c_config = {0};

	// step 4: init i2c
	i2c_config.baud_rate = I2C_BAUD_RATE_100KHZ;
	i2c_config.addr_mode = I2C_ADDR_MODE_7BIT;

	bk_i2c_init(CONFIG_DVP_CAMERA_I2C_ID, &i2c_config);
#endif
    // Modified by TUYA End

	// step 5: detect sensor
	sensor = bk_dvp_get_sensor_auto_detect();
	if (sensor == NULL)
	{
		LOGE("%s no dvp camera found\n", __func__);
	}
	else
	{
		LOGI("auto detect success, dvp camera name:%s\r\n", sensor->name);
	}

	bk_video_encode_stop(YUV_MODE);

    // Modified by TUYA Start
#if CONFIG_TUYA_LOGIC_MODIFY
	bk_video_power_off(dvp_ldo, !dvp_active_level);
#else
	bk_video_power_off(CONFIG_CAMERA_CTRL_POWER_GPIO_ID, 0);
#endif
    // Modified by TUYA End

	return sensor;
}

static bk_err_t dvp_camera_hardware_init(dvp_driver_handle_t *handle)
{
	int ret = BK_OK;

	dvp_camera_config_t *config = &handle->config;

	switch (handle->config.device->mode)
	{
		case YUV_MODE:
			ret = dvp_camera_yuv_mode(handle);
			break;

		case JPEG_MODE:
		case JPEG_YUV_MODE:
			ret = dvp_camera_jpeg_mode(handle);
			break;

		case H264_MODE:
		case H264_YUV_MODE:
			ret = dvp_camera_h264_mode(handle);
			break;

		default:
			ret = BK_FAIL;
			LOGE("%s, mode:%d not support\n", __func__, handle->config.device->mode);
			break;
	}

	if (handle->config.device->mode == JPEG_YUV_MODE || handle->config.device->mode == H264_YUV_MODE)
	{
		config->fb_init(FB_INDEX_DISPLAY);

		if (config->device->rot_angle == ROTATE_NONE)
		{
			handle->yuv_frame = config->fb_malloc(FB_INDEX_DISPLAY, handle->yuv_frame_length);
		}
		else
		{
			handle->yuv_frame = dvp_camera_yuv_frame_buffer_node_malloc(handle);
		}

		if(handle->yuv_frame == NULL)
		{
			LOGE("%s, yuv_frame malloc failed!\n", __func__);
			ret = BK_ERR_NO_MEM;
			return ret;
		}

		handle->yuv_frame->width = config->device->info.resolution.width;
		handle->yuv_frame->height = config->device->info.resolution.height;
		handle->yuv_frame->fmt = handle->sensor->fmt;
		handle->yuv_frame->type = config->device->type;
		handle->yuv_frame->length = handle->yuv_frame_length;

		if (handle->encode_yuv_config == NULL)
		{
			handle->encode_yuv_config = (encode_yuv_config_t *)os_malloc(sizeof(encode_yuv_config_t));
			if (handle->encode_yuv_config == NULL)
			{
				LOGE("encode_lcd_config malloc error! \r\n");
				ret = BK_ERR_NO_MEM;
				return ret;
			}
		}

		handle->encode_yuv_config->yuv_em_addr = bk_yuv_buf_get_em_base_addr();
		LOGI("yuv buffer base addr:%08x\r\n", handle->encode_yuv_config->yuv_em_addr);
		handle->encode_yuv_config->dma_collect_yuv = bk_dma_alloc(DMA_DEV_DTCM);
		handle->encode_yuv_config->yuv_pingpong_length = handle->yuv_pingpong_length >> 1;
		handle->encode_yuv_config->yuv_data_offset = 0;
		LOGI("dma_collect_yuv id is %d length:%d\r\n", handle->encode_yuv_config->dma_collect_yuv,
				handle->encode_yuv_config->yuv_pingpong_length);

		ret = dvp_camera_yuv_dma_cpy(handle->yuv_frame->frame,
						   (uint32_t *)handle->encode_yuv_config->yuv_em_addr,
						   handle->encode_yuv_config->yuv_pingpong_length,
						   handle->encode_yuv_config->dma_collect_yuv);
	}

	return ret;
}

void memcpy_word(uint32_t *dst, uint32_t *src, uint32_t size)
{
	uint32_t i = 0;

	for (i = 0; i < size; i++)
	{
		dst[i] = src[i];
	}
}

static void dvp_camera_yuv_rotate_task(beken_thread_arg_t data)
{
	dvp_driver_handle_t *handle = (dvp_driver_handle_t *)data;

	int ret = BK_OK;

	xEventGroupClearBits(handle->waiting_event, DVP_YUV_ROTATE_START_EVENT | DVP_YUV_ROTATE_STOP_EVENT);

	while (handle->dvp_state == MASTER_TURNING_ON || handle->dvp_state == MASTER_TURN_ON)
	{
		EventBits_t bits = xEventGroupWaitBits(handle->waiting_event, DVP_YUV_ROTATE_STOP_EVENT | DVP_YUV_ROTATE_START_EVENT,
							true, false, BEKEN_NEVER_TIMEOUT);

		if (bits & DVP_YUV_ROTATE_START_EVENT)
		{
			int (*func)(unsigned char *vuyy, unsigned char *rotatedVuyy, int width, int height);

			frame_buffer_t *src_yuv = handle->cp1_rot_info.rot_notify.src_yuv;
			frame_buffer_t *rotate_yuv = handle->cp1_rot_info.rot_notify.dst_yuv;
			func = NULL;
			switch (handle->cp1_rot_info.rot_notify.rot_angle)
			{
				case ROTATE_90:
					func = yuyv_rotate_degree90_to_yuyv;
					rotate_yuv->width = src_yuv->height;
					rotate_yuv->height = src_yuv->width;
					break;

				case ROTATE_270:
					func = yuyv_rotate_degree270_to_yuyv;
					rotate_yuv->width = src_yuv->height;
					rotate_yuv->height = src_yuv->width;
					break;

				case ROTATE_180:
					func = yuyv_rotate_degree180_to_yuyv;
					rotate_yuv->width = src_yuv->width;
					rotate_yuv->height = src_yuv->height;
					break;

				default:
					break;
			}

			// only support 864X480
			int i = 0, j = 0, k = 0;
			int src_width = 864, src_height = 480;
			int block_width = 54, block_height = 40;
			uint8_t *rx_block = handle->rot_rx;
			uint8_t *tx_block = handle->rot_tx;
			register uint8_t *cp_ptr = NULL;
			uint8_t *src_frame_temp = src_yuv->frame;
			uint8_t *dst_frame_temp = rotate_yuv->frame;
			for (j = 0; j < (src_height / block_height); j++)
			{

				for (i = 0; i < (src_width / block_width); i++)
				{
					for (k = 0; k < block_height; k++)
					{
						cp_ptr = src_frame_temp + i * block_width * 2 + j * block_height * src_width * 2 + k * src_width * 2;
						memcpy_word((uint32_t *)(rx_block + block_width * 2 * k), (uint32_t *)cp_ptr, block_width * 2 / 4);
					}

					func(rx_block, tx_block, block_width, block_height);

					for (k = 0; k < block_width; k++)
					{
						if (handle->config.device->rot_angle == ROTATE_90)
						{
							cp_ptr = dst_frame_temp + (src_height / block_height - j - 1) * block_height * 2 + (i) * block_width * src_height * 2 + k * src_height * 2;
							memcpy_word((uint32_t *)cp_ptr, (uint32_t *)(tx_block + block_height * 2 * k), block_height * 2 / 4);
						}
						else //270
						{
							cp_ptr = dst_frame_temp + (src_width / block_width - 1 - i) * block_width * src_height * 2 + block_height * j * 2 + k * src_height * 2;
							memcpy_word((uint32_t *)cp_ptr, (uint32_t *)(tx_block + block_height * 2 * k), block_height * 2 / 4);
						}
					}
				}
			}

			// push to encode list
			ret = dvp_camera_encode_node_push(handle, rotate_yuv);
			if (ret != BK_OK)
			{
				// free dst_yuv
				handle->config.fb_free(rotate_yuv);
			}
			dvp_camera_yuv_frame_buffer_node_free(handle, src_yuv);
			handle->cp1_rot_info.rot_notify.src_yuv = NULL;
			handle->cp1_rot_info.rot_notify.dst_yuv = NULL;
			handle->cp1_rot_info.rotate_state = false;
#ifdef YUV_DEBUG_ENABLE
			LOGI("%s, %d\n", __func__, __LINE__);
#endif
			xEventGroupSetBits(handle->waiting_event, DVP_YUV_ROTATE_COMPLETE_EVENT);
		}

		if (bits & DVP_YUV_ROTATE_STOP_EVENT)
		{
			break;
		}
	}

	handle->rot_thread = NULL;
	xEventGroupClearBits(handle->waiting_event, DVP_YUV_ROTATE_TASK_EXIT_EVENT);
	xEventGroupSetBits(handle->waiting_event, DVP_YUV_ROTATE_TASK_EXIT_EVENT);
	rtos_delete_thread(NULL);
}

static void dvp_camera_encode_task(beken_thread_arg_t data)
{
	dvp_driver_handle_t *handle = (dvp_driver_handle_t *)data;

	frame_buffer_t *src_yuv = NULL;

	while (handle->dvp_state == MASTER_TURNING_ON || handle->dvp_state == MASTER_TURN_ON)
	{
		if (handle->cp1_rot_info.rotate_state && handle->cp2_rot_info.rotate_state)
		{
			xEventGroupWaitBits(handle->waiting_event, DVP_YUV_ROTATE_COMPLETE_EVENT,
									true, false, 100);
		}

		src_yuv = dvp_camera_yuv_frame_buffer_node_pop(handle);

		if (src_yuv == NULL)
		{
			xEventGroupWaitBits(handle->waiting_event, DVP_YUV_OUTPUT_COMPLETE_EVENT, true, false, BEKEN_NEVER_TIMEOUT);
			continue;
		}

		frame_buffer_t *rotate_yuv = handle->config.fb_malloc(FB_INDEX_DISPLAY, handle->yuv_frame_length);

		if (rotate_yuv)
		{
			rotate_yuv->width = src_yuv->width;
			rotate_yuv->height = src_yuv->height;
			rotate_yuv->sequence = src_yuv->sequence;
			rotate_yuv->fmt = src_yuv->fmt;
			rotate_yuv->length = src_yuv->length;
		}
		else
		{
			LOGW("%s, malloc rotate frame fail\n", __func__);
			dvp_camera_yuv_frame_buffer_node_free(handle, src_yuv);
			rtos_delay_milliseconds(50);
			continue;
		}

		if (handle->cp1_rot_info.rotate_state == false)
		{
			handle->cp1_rot_info.rotate_state = true;
			handle->cp1_rot_info.rot_notify.rot_angle = handle->config.device->rot_angle;
			handle->cp1_rot_info.rot_notify.src_yuv = src_yuv;
			handle->cp1_rot_info.rot_notify.dst_yuv = rotate_yuv;
			xEventGroupClearBits(handle->waiting_event, DVP_YUV_ROTATE_START_EVENT);
			xEventGroupSetBits(handle->waiting_event, DVP_YUV_ROTATE_START_EVENT);
		}
		else if (handle->cp2_rot_info.rotate_state == false)
		{
			handle->cp2_rot_info.rotate_state = true;
			handle->cp2_rot_info.rot_notify.rot_angle = handle->config.device->rot_angle;
			handle->cp2_rot_info.rot_notify.src_yuv = src_yuv;
			handle->cp2_rot_info.rot_notify.dst_yuv = rotate_yuv;
			if (handle->config.frame_ops((uint32_t)&handle->cp2_rot_info.rot_notify) != BK_OK)
			{
				LOGE("%s, %d\n", __func__, __LINE__);
			}
		}
		else
		{
			// rotate busy
			dvp_camera_yuv_frame_buffer_node_free(handle, src_yuv);
			src_yuv = NULL;
			handle->config.fb_free(rotate_yuv);
			rotate_yuv = NULL;
		}

#ifdef YUV_DEBUG_ENABLE
		LOGI("%s, %d\n", __func__, __LINE__);
#endif

		while (1)
		{
			frame_buffer_t * encode_yuv = dvp_camera_encode_node_pop(handle);

			if (encode_yuv == NULL)
			{
				break;
			}
			else
			{
				handle->in_encode_frame = encode_yuv;
				handle->line_done_index = 0;

#if YUV_ENCODE_ENABLE
				bk_yuv_buf_set_emr_base_addr((uint32_t)handle->emr_base_addr);

				uint32_t offset = handle->encode_pingpong_length >> 1;
#if YUV_ENCODE_MEM_CPY_ENABLE
				bk_dma_set_src_start_addr(handle->mem_cpy_channel,
									  (uint32_t)(handle->in_encode_frame->frame + handle->line_done_index * offset));
				bk_dma_set_dest_start_addr(handle->mem_cpy_channel,
									   (uint32_t)handle->emr_base_addr);
				bk_dma_start(handle->mem_cpy_channel);
				BK_WHILE(bk_dma_get_enable_status(handle->mem_cpy_channel));
#else
				os_memcpy(handle->emr_base_addr, handle->in_encode_frame->frame, offset);
#endif
				bk_h264_encode_enable();

#ifdef YUV_DEBUG_ENABLE
				LOGI("%s, %d\n", __func__, __LINE__);
#endif

				bk_yuv_buf_rencode_start();

				xEventGroupClearBits(handle->waiting_event, DVP_YUV_ENCODE_COMPLETE_EVENT);
				EventBits_t bits = xEventGroupWaitBits(handle->waiting_event, DVP_YUV_ENCODE_COMPLETE_BIT,
					true, false, 100);//BEKEN_NEVER_TIMEOUT

				if (bits & DVP_YUV_ENCODE_STOP_EVENT)
				{
					handle->config.fb_free(encode_yuv);
					handle->in_encode_frame = NULL;
					xEventGroupClearBits(handle->waiting_event, DVP_YUV_ROTATE_STOP_EVENT);
					xEventGroupSetBits(handle->waiting_event, DVP_YUV_ROTATE_STOP_EVENT);
					LOGI("%s, yuv task exit\n", __func__);
					goto exit;
				}
				else
				{
					if (bits & DVP_YUV_ENCODE_COMPLETE_EVENT)
					{
						if (handle->error)
						{
							// encode error
							LOGW("%s, encode error\n", __func__);
							handle->error = false;
							dvp_camera_reset_hardware_modules_handler(handle);
						}
					}
					else
					{
						// encode timeout
						LOGW("%s, encode timeout\n", __func__);
						dvp_camera_reset_hardware_modules_handler(handle);
					}
				}
#endif

#ifdef YUV_DEBUG_ENABLE
				LOGI("%s, seq:%d\n", __func__, encode_yuv->sequence);
#endif
				handle->config.fb_complete(encode_yuv);
				handle->in_encode_frame = NULL;
			}

		}
	}

exit:

	handle->yuv_thread = NULL;
	xEventGroupClearBits(handle->waiting_event, DVP_YUV_ENCODE_TASK_EXIT_EVENT);
	xEventGroupSetBits(handle->waiting_event, DVP_YUV_ENCODE_TASK_EXIT_EVENT);
	rtos_delete_thread(NULL);
}

static bk_err_t dvp_camera_pingpang_buf_init(dvp_driver_handle_t *handle)
{
	int ret = BK_OK;

	dvp_camera_config_t *config = &handle->config;

	if (config->device->mode == YUV_MODE
		|| config->device->mode == JPEG_MODE
		|| config->device->mode == H264_MODE)
	{
		if (config->device->rot_angle != ROTATE_NONE)
		{
			LOGE("%s, param error, mode:%d, angle:%d\n", __func__, config->device->mode, config->device->rot_angle);
			ret = BK_ERR_PARAM;
			return ret;
		}
	}

	if (config->device->rot_angle != ROTATE_NONE)
	{
		if (handle->yuv_frame_length > 864 * 480 * 2)
		{
			LOGE("%s, not support this resolution rotate, most support 864X480...\n", __func__);
			ret = BK_ERR_PARAM;
			return ret;
		}

#if YUV_ENCODE_MEM_CPY_ENABLE
		handle->mem_cpy_channel = bk_dma_alloc(DMA_DEV_DTCM);
		if (handle->mem_cpy_channel >= DMA_ID_MAX)
		{
			LOGE("%s, mem cpy dma channel alloc fail\n", __func__);
			ret = BK_ERR_PARAM;
			return ret;
		}
#endif
	}

	if (config->device->mode == JPEG_MODE || config->device->mode == JPEG_YUV_MODE)
	{
		handle->yuv_pingpong_length = config->device->info.resolution.width * 16 * 2;
		handle->yuv_line_cnt = config->device->info.resolution.height / 8;
	}
	else if (config->device->mode == H264_MODE || config->device->mode == H264_YUV_MODE)
	{
		handle->yuv_pingpong_length = config->device->info.resolution.width * 32 * 2;
		handle->yuv_line_cnt = config->device->info.resolution.height / 16;
	}
	else
	{
		handle->yuv_pingpong_length = 0;
		handle->yuv_line_cnt = 0;
	}

	switch (handle->config.device->rot_angle)
	{
		case ROTATE_NONE:
			handle->encode_pingpong_length = 0;
			handle->line_done_cnt = 0;
			break;

		case ROTATE_90:
		case ROTATE_270:
			handle->encode_pingpong_length = config->device->info.resolution.height * 32 * 2;
			handle->line_done_cnt = config->device->info.resolution.width / 16;
			break;

		case ROTATE_180:
			handle->encode_pingpong_length = handle->yuv_pingpong_length;
			handle->line_done_cnt = handle->yuv_line_cnt;
			break;
	}

	LOGI("%s, mode:%d, angle:%d, yuv:%d-%d, encode:%d-%d\n",
			__func__,
			config->device->mode,
			config->device->rot_angle,
			handle->yuv_pingpong_length, handle->yuv_line_cnt,
			handle->encode_pingpong_length, handle->line_done_cnt);

	dvp_camera_encode = (dvp_sram_buffer_t *)media_bt_share_buffer;

	if (dvp_camera_encode)
	{
		handle->em_base_addr = dvp_camera_encode->yuv_pingpang;
		handle->emr_base_addr = dvp_camera_encode->enc_pingpang;

		handle->rot_rx = media_dtcm_share_buff;
		handle->rot_tx = media_dtcm_share_buff + 5 * 1024;

		if (handle->rot_rx == NULL)
		{
			handle->rot_rx = dvp_camera_encode->rot_rx;
			handle->rot_tx = dvp_camera_encode->rot_tx;
		}
#ifdef YUV_DEBUG_ENABLE
		else
		{
			LOGI("%s, %d, %p\n", __func__, __LINE__, handle->rot_rx);
		}
#endif

		if (handle->encode_pingpong_length == 0)
		{
			handle->emr_base_addr = handle->em_base_addr;
		}
	}
	else
	{
		handle->rot_rx = (uint8_t *)os_malloc(1024 * 10);
		if (handle->rot_rx == NULL)
		{
			LOGE("%s, rot_rx malloc fail\n", __func__);
			ret = BK_ERR_NO_MEM;
			return ret;
		}

		handle->rot_tx = handle->rot_rx + 1024 * 5;

		if (handle->yuv_pingpong_length)
		{
			handle->em_base_addr = (uint8_t *)os_malloc(handle->yuv_pingpong_length);

			if (handle->em_base_addr == NULL)
			{
				LOGE("%s, em_base_addr malloc fail\n", __func__);
				ret = BK_ERR_NO_MEM;
				return ret;
			}
		}

		if (handle->encode_pingpong_length)
		{
			handle->emr_base_addr = (uint8_t *)os_malloc(handle->encode_pingpong_length);

			if (handle->emr_base_addr == NULL)
			{
				LOGE("%s, emr_base_addr malloc fail\n", __func__);
				ret = BK_ERR_NO_MEM;
				return ret;
			}
		}
		else
		{
			handle->emr_base_addr = handle->em_base_addr;
		}
	}

#if YUV_ENCODE_MEM_CPY_ENABLE
	if (config->device->rot_angle != ROTATE_NONE)
	{
		// attation: when start this dma, must modify src->start_addr and dst->src_addr
		ret = dvp_camera_yuv_dma_cpy(handle->em_base_addr,
							handle->emr_base_addr,
							handle->encode_pingpong_length >> 1,
							handle->mem_cpy_channel);

		if (ret != BK_OK)
		{
			LOGE("%s, %d\n", __func__, __LINE__);
		}
	}
#endif

	return ret;
}

static bk_err_t dvp_camera_pingpang_buf_deinit(dvp_driver_handle_t *handle)
{
	if (dvp_camera_encode == NULL)
	{
		// not share with bt
		if (handle->yuv_pingpong_length)
		{
			if (handle->em_base_addr)
			{
				os_free(handle->em_base_addr);
				handle->em_base_addr = NULL;
			}
		}

		if (handle->encode_pingpong_length)
		{
			if (handle->emr_base_addr)
			{
				os_free(handle->emr_base_addr);
				handle->emr_base_addr = NULL;
			}
		}

		if (handle->rot_rx)
		{
			os_free(handle->rot_rx);
			handle->rot_rx = NULL;
			handle->rot_tx = NULL;
		}
	}

	return BK_OK;
}

static bk_err_t dvp_camera_init(dvp_driver_handle_t *handle)
{
	handle->sensor = bk_dvp_camera_enumerate();
	if (handle->sensor == NULL)
	{
		return BK_FAIL;
	}

    // Modified by TUYA Start
#if CONFIG_TUYA_LOGIC_MODIFY
    extern OPERATE_RET tkl_vi_get_power_info(uint8_t device_type, uint8_t *io, uint8_t *active);
    uint8_t dvp_ldo = 56, dvp_active_level = 0;
    tkl_vi_get_power_info(DVP_CAMERA, &dvp_ldo, &dvp_active_level);
	bk_video_power_on(dvp_ldo, dvp_active_level);
#else
	bk_video_power_on(CONFIG_CAMERA_CTRL_POWER_GPIO_ID, 1);
#endif
    // Modified by TUYA End

	/* set current used camera config */
	BK_RETURN_ON_ERR(dvp_camera_init_device(handle));

	return BK_OK;
}

static bk_err_t dvp_camera_deinit(dvp_driver_handle_t *handle)
{
	// step 1: deinit dvp gpio, data cannot transfer
	bk_video_gpio_deinit(DVP_GPIO_ALL);

	// step 2: deinit i2c
	bk_i2c_deinit(CONFIG_DVP_CAMERA_I2C_ID);

	// step 3: deinit hardware
	bk_yuv_buf_deinit();
	bk_h264_encode_disable();
	bk_h264_deinit();
	bk_jpeg_enc_deinit();

	bk_video_dvp_mclk_disable();

	// step 4: power off
	bk_video_power_off(CONFIG_CAMERA_CTRL_POWER_GPIO_ID, 1);

	// free config buffer
	if (handle)
	{
		if (handle->encode_yuv_config)
		{
			bk_dma_stop(handle->encode_yuv_config->dma_collect_yuv);
			bk_dma_deinit(handle->encode_yuv_config->dma_collect_yuv);
			bk_dma_free(DMA_DEV_DTCM, handle->encode_yuv_config->dma_collect_yuv);

			os_free(handle->encode_yuv_config);
			handle->encode_yuv_config = NULL;
		}

		if (handle->sem)
		{
			rtos_deinit_semaphore(&handle->sem);
		}

		if (handle->config.device)
		{
			yuv_mode_t mode = handle->config.device->mode;

			if (handle->config.device->rot_angle != ROTATE_NONE)
			{
				// step 1:close encode task
				if (handle->yuv_thread)
				{
					xEventGroupSetBits(handle->waiting_event, DVP_YUV_OUTPUT_COMPLETE_EVENT);
					xEventGroupSetBits(handle->waiting_event, DVP_YUV_ENCODE_STOP_EVENT);

					xEventGroupWaitBits(handle->waiting_event, DVP_YUV_ENCODE_TASK_EXIT_EVENT,
						true, false, BEKEN_NEVER_TIMEOUT);
				}

#ifdef YUV_DEBUG_ENABLE
				LOGI("%s, %d\n", __func__, __LINE__);
#endif


				// step 2:close rotate task
				if (handle->rot_thread)
				{
					xEventGroupSetBits(handle->waiting_event, DVP_YUV_ROTATE_STOP_EVENT);
					xEventGroupWaitBits(handle->waiting_event, DVP_YUV_ROTATE_TASK_EXIT_EVENT,
						true, false, BEKEN_NEVER_TIMEOUT);
				}

				// step 3:free rotate buffer
				if (handle->cp1_rot_info.rotate_state)
				{
					if (dvp_camera_encode_node_push(handle, handle->cp1_rot_info.rot_notify.dst_yuv) != BK_OK)
					{
						// free dst_yuv
						handle->config.fb_free(handle->cp1_rot_info.rot_notify.dst_yuv);
					}

					dvp_camera_yuv_frame_buffer_node_free(handle, handle->cp1_rot_info.rot_notify.src_yuv);
					handle->cp1_rot_info.rotate_state = false;
				}

				if (handle->cp2_rot_info.rotate_state)
				{
					if (dvp_camera_encode_node_push(handle, handle->cp2_rot_info.rot_notify.dst_yuv) != BK_OK)
					{
						// free dst_yuv
						handle->config.fb_free(handle->cp2_rot_info.rot_notify.dst_yuv);
					}

					dvp_camera_yuv_frame_buffer_node_free(handle, handle->cp2_rot_info.rot_notify.src_yuv);
					handle->cp2_rot_info.rotate_state = false;
				}

#ifdef YUV_DEBUG_ENABLE
				LOGI("%s, %d\n", __func__, __LINE__);
#endif

				if (handle->yuv_frame)
				{
					dvp_camera_yuv_frame_buffer_node_free(handle, handle->yuv_frame);
					handle->yuv_frame = NULL;
				}

				dvp_camera_yuv_frame_buffer_node_deinit(handle);

				if (handle->yuv_lock)
				{
					rtos_deinit_mutex(&handle->yuv_lock);
				}

				if (handle->waiting_event)
				{
					vEventGroupDelete(handle->waiting_event);
					handle->waiting_event = NULL;
				}
			}

			if (mode != YUV_MODE && mode != GRAY_MODE
				&& handle->dma_channel != DMA_ID_MAX)
			{
				bk_dma_stop(handle->dma_channel);
				bk_dma_deinit(handle->dma_channel);

				if (mode == H264_MODE || mode == H264_YUV_MODE)
				{
					bk_dma_free(DMA_DEV_H264, handle->dma_channel);
				}
				else
				{
					bk_dma_free(DMA_DEV_JPEG, handle->dma_channel);
				}

#if YUV_ENCODE_MEM_CPY_ENABLE
				if (handle->mem_cpy_channel != DMA_ID_MAX)
				{
					bk_dma_stop(handle->mem_cpy_channel);
					bk_dma_deinit(handle->mem_cpy_channel);
					bk_dma_free(DMA_DEV_DTCM, handle->mem_cpy_channel);
				}
#endif
			}

			dvp_camera_pingpang_buf_deinit(handle);

			// free yuv frame buffer
			if (handle->yuv_frame)
			{
				handle->config.fb_free(handle->yuv_frame);
				handle->yuv_frame = NULL;
			}

			// free roate frame buffer
			if (handle->in_encode_frame)
			{
				handle->config.fb_free(handle->in_encode_frame);
				handle->in_encode_frame = NULL;
			}

			// free h264/jpeg frame buffer
			if (handle->encode_frame)
			{
				handle->config.fb_free(handle->encode_frame);
				handle->encode_frame = NULL;
			}

			// clear h264/jpeg frame buffer ready list
			if (mode == H264_MODE || mode == H264_YUV_MODE)
			{
				handle->config.fb_clear(FB_INDEX_H264);
			}
			else
			{
				handle->config.fb_clear(FB_INDEX_JPEG);
			}

			// free handle->config.device
			os_free(handle->config.device);
			handle->config.device = NULL;
		}

		if (handle->sem)
		{
			rtos_deinit_semaphore(&handle->sem);
		}

		handle->dvp_state = MASTER_TURN_OFF;

		os_free(handle);

		s_dvp_camera_handle = NULL;
	}

	LOGI("%s complete!\r\n", __func__);

	return BK_OK;
}

void bk_dvp_camera_cp2_rotate_finish(void)
{
	dvp_driver_handle_t *handle = s_dvp_camera_handle;

	if (handle == NULL)
	{
		LOGE("%s, %d\n", __func__, __LINE__);
		return;
	}

#ifdef YUV_DEBUG_ENABLE
	LOGI("%s, %d\n", __func__, __LINE__);
#endif

	// push to encode list
	int ret = dvp_camera_encode_node_push(handle, handle->cp2_rot_info.rot_notify.dst_yuv);
	if (ret != BK_OK)
	{
		// free dst_yuv
		handle->config.fb_free(handle->cp2_rot_info.rot_notify.dst_yuv);
		handle->cp2_rot_info.rot_notify.dst_yuv = NULL;
	}

	// free src_yuv
	dvp_camera_yuv_frame_buffer_node_free(handle, handle->cp2_rot_info.rot_notify.src_yuv);
	handle->cp2_rot_info.rot_notify.src_yuv = NULL;
	handle->cp2_rot_info.rotate_state = false;
	xEventGroupSetBits(handle->waiting_event, DVP_YUV_ROTATE_COMPLETE_EVENT);
}

bk_err_t bk_dvp_camera_driver_init(dvp_camera_config_t *config)
{
	int ret = BK_OK;

	LOGI("%s\r\n", __func__);

	dvp_driver_handle_t *handle = s_dvp_camera_handle;

	if (handle == NULL)
	{
		handle = (dvp_driver_handle_t *)os_malloc(sizeof(dvp_driver_handle_t));

		if (handle == NULL)
		{
			LOGI("%s, s_dvp_camera_handle malloc failed\n", __func__);
			ret = BK_ERR_NO_MEM;
			goto error;
		}

		os_memset(handle, 0, sizeof(dvp_driver_handle_t));

		handle->dvp_state = MASTER_TURNING_ON;

		DVP_DIAG_DEBUG_INIT();

		os_memcpy(&handle->config, config, sizeof(dvp_camera_config_t));

		handle->config.device = (media_camera_device_t *)os_malloc(sizeof(media_camera_device_t));

		if (handle->config.device == NULL)
		{
			LOGE("%s, handle->config.device malloc failed!\r\n", __func__);
			goto error;
		}

		os_memcpy(handle->config.device, config->device, sizeof(media_camera_device_t));

		handle->yuv_frame_length = config->device->info.resolution.width * config->device->info.resolution.height * 2;

		handle->dma_channel = DMA_ID_MAX;
#if YUV_ENCODE_MEM_CPY_ENABLE
		handle->mem_cpy_channel = DMA_ID_MAX;
#endif

		ret = dvp_camera_pingpang_buf_init(handle);
		if (ret != BK_OK)
		{
			LOGE("%s, pingpang_buf malloc failed!\r\n", __func__);
			goto error;
		}

		ret = rtos_init_semaphore(&handle->sem, 1);
		if (ret != BK_OK)
		{
			LOGE("%s, handle->sem malloc failed!\r\n", __func__);
			goto error;
		}

		if (config->device->rot_angle != ROTATE_NONE)
		{
			ret = dvp_camera_yuv_frame_buffer_node_init(handle);

			if (ret != BK_OK)
			{
				LOGE("%s, %d\n", __func__, __LINE__);
				goto error;
			}

			ret = rtos_create_thread(&handle->rot_thread,
							BEKEN_DEFAULT_WORKER_PRIORITY,
							"yuv_rotate",
							(beken_thread_function_t)dvp_camera_yuv_rotate_task,
							1024,
							(beken_thread_arg_t)handle);

			if (ret != BK_OK)
			{
				LOGE("%s, create yuv_rotate fail\n", __func__);
				goto error;
			}

			ret = rtos_create_thread(&handle->yuv_thread,
							BEKEN_DEFAULT_WORKER_PRIORITY - 1,
							"yuv_task",
							(beken_thread_function_t)dvp_camera_encode_task,
							1024,
							(beken_thread_arg_t)handle);

			if (ret != BK_OK)
			{
				LOGE("%s, create yuv_task fail\n", __func__);
				goto error;
			}
		}
	}
	else
	{
		LOGE("dvp init state error\r\n");
		ret = BK_FAIL;
		return ret;
	}

	// step 1: for camera sensor, init other device
	ret = dvp_camera_init(handle);
	if (ret != BK_OK)
	{
		goto error;
	}

	// step 2: init yuv_buf/h264/jpeg
	ret = dvp_camera_hardware_init(handle);
	if (ret != BK_OK)
	{
		goto error;
	}

	// step 3: maybe need register isr_func
	dvp_camera_register_isr_function(handle);

	// step 4: start hardware function in different mode
	bk_video_set_mclk(handle->sensor->clk);

	bk_video_encode_start(handle->config.device->mode);

	if (handle->config.device->rot_angle != ROTATE_NONE)
	{
		if (handle->config.device->mode == JPEG_YUV_MODE)
		{
			bk_yuv_buf_stop(JPEG_MODE);
		}
		else if (handle->config.device->mode == H264_YUV_MODE)
		{
			bk_h264_encode_disable();
		}
		else
		{
			ret = BK_ERR_PARAM;
			LOGE("%s, param error, mode:%d, angle:%d\n", __func__, handle->config.device->mode, handle->config.device->rot_angle);
			goto error;
		}
	}

	// step 5: init dvp camera sensor register
	handle->sensor->init();
	handle->sensor->set_ppi((handle->config.device->info.resolution.width << 16) | handle->config.device->info.resolution.height);
	handle->sensor->set_fps(handle->config.device->info.fps);

	media_debug->isr_jpeg = 0;
	media_debug->isr_h264 = 0;
	media_debug->psram_busy = 0;
	media_debug->jpeg_length = 0;
	media_debug->h264_length = 0;
	media_debug->jpeg_kbps = 0;
	media_debug->h264_kbps = 0;

	handle->dvp_state = MASTER_TURN_ON;

	s_dvp_camera_handle = handle;

	return ret;

error:

	if (handle)
	{
		handle->dvp_state = MASTER_TURNING_OFF;
	}

	dvp_camera_deinit(handle);

	handle = NULL;

	return ret;
}

bk_err_t bk_dvp_camera_driver_deinit(void)
{
	dvp_driver_handle_t *handle = s_dvp_camera_handle;

	if (handle == NULL || handle->dvp_state == MASTER_TURN_OFF)
	{
		LOGI("%s, dvp have been closed!\r\n", __func__);
		return BK_FAIL;
	}

	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();
	handle->dvp_state = MASTER_TURNING_OFF;
	GLOBAL_INT_RESTORE();

	if (BK_OK != rtos_get_semaphore(&handle->sem, 500))
	{
		LOGI("Not wait yuv vsync negedge!\r\n");
	}

	dvp_camera_deinit(handle);

	handle = NULL;

	return BK_OK;
}

media_camera_device_t *bk_dvp_camera_get_device(void)
{
	dvp_driver_handle_t *handle = s_dvp_camera_handle;

	if (handle != NULL && handle->dvp_state == MASTER_TURN_ON)
	{
		return handle->config.device;
	}

	return NULL;
}

bk_err_t bk_dvp_camera_h264_regenerate_idr_frame(void)
{
	dvp_driver_handle_t *handle = s_dvp_camera_handle;

	if (handle == NULL || handle->dvp_state != MASTER_TURN_ON)
	{
		LOGW("%s, %d dvp not open!\r\n", __func__, __LINE__);
		return BK_FAIL;
	}

	yuv_mode_t mode = handle->config.device->mode;

	if (mode != H264_MODE && mode != H264_YUV_MODE)
	{
		LOGW("%s, %d not support this encode mode!\r\n", __func__, __LINE__);
		return BK_FAIL;
	}

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	handle->regenerate_idr = true;
	GLOBAL_INT_RESTORE();

	return BK_OK;
}
