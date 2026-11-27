#include "bk_uvc_common.h"
#include "uvc_urb_list.h"
#include "uvc_stream_list.h"
#include <modules/pm.h>
#include <components/cherryusb/usb_errno.h>
#include "media_utils.h"
#define TAG "uvc_stream"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

/**
 * UVC帧丢弃控制 - 全局变量
 * 
 * g_uvc_frame_drop_mode:
 *   0 = 不丢帧 (30fps保持30fps)
 *   1 = 每2帧保留1帧 (30fps → 15fps)
 *   2 = 每3帧保留1帧 (30fps → 10fps)
 *   3 = 每4帧保留1帧 (30fps → 7.5fps)
 *   N = 每(N+1)帧保留1帧
 */
static uint8_t g_uvc_frame_drop_mode = 0;
static uint8_t g_uvc_frame_drop_counter = 0;

#define UVC_MAX_PACKET_SIZE (1024)
//#define UVC_DEBUG_TIME

#ifdef UVC_DEBUG_TIME
#define UVC_POWER_ON_START()            GPIO_UP(32);
#define UVC_POWER_ON_END()              GPIO_DOWN(32);

#define UVC_INIT_START()                GPIO_UP(33);
#define UVC_INIT_END()                  GPIO_DOWN(33);

#define UVC_PACKET_PUSH_START()         GPIO_UP(32);
#define UVC_PACKET_PUSH_END()           GPIO_DOWN(32);

#define UVC_EOF_START()                 GPIO_UP(34);
#define UVC_EOF_END()                   GPIO_DOWN(34);

#define UVC_PACKET_START()              GPIO_UP(35);
#define UVC_PACKET_END()                GPIO_DOWN(35);

#define UVC_PACKET_HEAD_START()         GPIO_UP(36);
#define UVC_PACKET_HEAD_END()           GPIO_DOWN(36);

#define UVC_PACKET_COPY_START()         GPIO_UP(37);
#define UVC_PACKET_COPY_END()           GPIO_DOWN(37);

#define UVC_EOF_BIT_START()             GPIO_UP(38);
#define UVC_EOF_BIT_END()               GPIO_DOWN(38);
#else
#define UVC_POWER_ON_START()
#define UVC_POWER_ON_END()

#define UVC_INIT_START()
#define UVC_INIT_END()

#define UVC_PACKET_PUSH_START()
#define UVC_PACKET_PUSH_END()

#define UVC_EOF_START()
#define UVC_EOF_END()

#define UVC_PACKET_START()
#define UVC_PACKET_END()

#define UVC_PACKET_HEAD_START()
#define UVC_PACKET_HEAD_END()

#define UVC_PACKET_COPY_START()
#define UVC_PACKET_COPY_END()

#define UVC_EOF_BIT_START()
#define UVC_EOF_BIT_END()
#endif

uvc_stream_handle_t *s_uvc_stream_handle = NULL;
uvc_separate_config_t uvc_separate_packet_cb = {0};
uvc_separate_info_t uvc_separate_info;
camera_state_cb_t uvc_connect_state_cb = NULL;

extern uint32_t  platform_is_in_interrupt_context(void);
static void uvc_camera_stream_receive_complete_callback(void *pCompleteParam, int nbytes);
static bk_err_t uvc_camera_stream_packet_urb(camera_param_t *camera_param);

#if (MEDIA_DEBUG_TIMER_ENABLE)
static void uvc_camera_stream_timer_handle(void *arg1)
{
    uvc_stream_handle_t *uvc_stream_handle = (uvc_stream_handle_t *)arg1;

    uvc_pro_config_t *pro_config = uvc_stream_handle->pro_config;

    if (pro_config != NULL)
    {
        LOGW("uvc_id1:%d[%d %dKB], uvc_id2: %d[%d %dKB], uvc_id3:%d[%d %dKB], packets:[all:%d, err:%d]\n",
             ((pro_config->frame_id[0] - pro_config->later_id[0]) / UVC_TIME_INTERVAL),
             pro_config->frame_id[0], (pro_config->curr_length[0] / 1024),
             ((pro_config->frame_id[1] - pro_config->later_id[1]) / UVC_TIME_INTERVAL),
             pro_config->frame_id[1], (pro_config->curr_length[1] / 1024),
             ((pro_config->frame_id[2] - pro_config->later_id[2]) / UVC_TIME_INTERVAL),
             pro_config->frame_id[2], (pro_config->curr_length[2] / 1024),
             pro_config->all_packet_num, pro_config->packet_err_num);

        for (uint8_t i = 0; i < CAMERA_ID_MAX; i++)
        {
            pro_config->later_id[i] = pro_config->frame_id[i];
        }
    }
}
#endif

bk_err_t uvc_stream_task_send_msg(uvc_event_t event, uint32_t param)
{
    int ret = BK_OK;
    uvc_stream_handle_t *handle = s_uvc_stream_handle;
    uvc_stream_event_t msg;

    if (handle == NULL
        || handle->stream_queue == NULL)
    {
        LOGV("%s, %d task have closed\r\n", __func__, event);
        return BK_FAIL;
    }

    msg.event = event;
    msg.param = param;

    ret = rtos_push_to_queue(&handle->stream_queue, &msg, BEKEN_NO_WAIT);
    if (BK_OK != ret)
    {
        LOGE("push failed, event:%d\r\n", event);
        return ret;
    }

    return ret;
}

/* uvc_config_t.fps is a media FPSxx bit (FPS5 << n), the frame descriptors
 * list dwFrameInterval values in 100 ns units */
static uint32_t __uvc_fps_bit_to_interval(uint32_t fps)
{
    uint32_t rate = 5;

    while (fps >>= 1)
    {
        rate += 5;
    }

    return 10000000 / rate;
}

static bk_err_t uvc_camera_stream_check_config(camera_param_t *param)
{
    int ret = BK_OK;
    uint8_t frame_num = 0;
    uint8_t index = 0;
    uint8_t resolution_flag = false;
    uint8_t fps_flag = false;
    uint32_t want_interval = 0;

    bk_usb_hub_port_info *uvc_port_info = param->port_info;
    uvc_config_t *user_config = param->info;
    LOGD("%s, %d, port:%d, format:%d, W*H:%d*%d\r\n", __func__, __LINE__, user_config->port, user_config->img_format,
        user_config->width, user_config->height);

    bk_uvc_device_brief_info_t *uvc_device_param = (bk_uvc_device_brief_info_t *)uvc_port_info->usb_device_param;
    bk_uvc_config_t *uvc_device_param_config = (bk_uvc_config_t *)uvc_port_info->usb_device_param_config;

    LOGV("PORT:0x%x\r\n", user_config->port);
    LOGV("VID:0x%x\r\n", uvc_device_param->vendor_id);
    LOGV("PID:0x%x\r\n", uvc_device_param->product_id);
    LOGV("BCD:0x%x\r\n", uvc_device_param->device_bcd);
    uvc_device_param_config->vendor_id = uvc_device_param->vendor_id;
    uvc_device_param_config->product_id = uvc_device_param->product_id;
    want_interval = __uvc_fps_bit_to_interval(user_config->fps);

    switch (user_config->img_format)
    {
        case IMAGE_YUV:
            uvc_device_param_config->format_index = uvc_device_param->format_index.yuv_format_index;
            frame_num = uvc_device_param->all_frame.yuv_frame_num;
            for (index = 0; index < frame_num; index++)
            {
                LOGV("YUV width:%d heigth:%d index:%d\r\n",
                     uvc_device_param->all_frame.yuv_frame[index].width,
                     uvc_device_param->all_frame.yuv_frame[index].height,
                     uvc_device_param->all_frame.yuv_frame[index].index);

                if (uvc_device_param->all_frame.yuv_frame[index].width == user_config->width
                    && uvc_device_param->all_frame.yuv_frame[index].height == user_config->height)
                {
                    uvc_device_param_config->frame_index = uvc_device_param->all_frame.yuv_frame[index].index;
                    uvc_device_param_config->width = uvc_device_param->all_frame.yuv_frame[index].width;
                    uvc_device_param_config->height = uvc_device_param->all_frame.yuv_frame[index].height;
                    resolution_flag = true;
                }

                for (int i = 0; i < uvc_device_param->all_frame.yuv_frame[index].fps_num; i++)
                {
                    LOGV("YUV fps:%d\r\n", uvc_device_param->all_frame.yuv_frame[index].fps[i]);

                    if (resolution_flag
                        && uvc_device_param->all_frame.yuv_frame[index].fps[i] == want_interval)
                    {
                        uvc_device_param_config->fps = uvc_device_param->all_frame.yuv_frame[index].fps[i];
                        fps_flag = true;
                    }
                }

                if (resolution_flag)
                {
                    // have adapt this resolution
                    if (fps_flag == false)
                    {
                        uvc_device_param_config->fps = uvc_device_param->all_frame.yuv_frame[index].fps[0];
                        fps_flag = true;
                    }
                    break;
                }
            }
            break;

        case IMAGE_MJPEG:
            uvc_device_param_config->format_index = uvc_device_param->format_index.mjpeg_format_index;
            frame_num = uvc_device_param->all_frame.mjpeg_frame_num;
            for (index = 0; index < frame_num; index++)
            {
                LOGV("MJPEG width:%d heigth:%d index:%d\r\n",
                     uvc_device_param->all_frame.mjpeg_frame[index].width,
                     uvc_device_param->all_frame.mjpeg_frame[index].height,
                     uvc_device_param->all_frame.mjpeg_frame[index].index);

                if (uvc_device_param->all_frame.mjpeg_frame[index].width == user_config->width
                    && uvc_device_param->all_frame.mjpeg_frame[index].height == user_config->height)
                {
                    uvc_device_param_config->frame_index = uvc_device_param->all_frame.mjpeg_frame[index].index;
                    uvc_device_param_config->width = uvc_device_param->all_frame.mjpeg_frame[index].width;
                    uvc_device_param_config->height = uvc_device_param->all_frame.mjpeg_frame[index].height;
                    resolution_flag = true;
                }

                // iterate all support fps of current resolution
                for (int i = 0; i < uvc_device_param->all_frame.mjpeg_frame[index].fps_num; i++)
                {
                    LOGV("MJPEG fps:%d\r\n", uvc_device_param->all_frame.mjpeg_frame[index].fps[i]);

                    if (resolution_flag
                        && uvc_device_param->all_frame.mjpeg_frame[index].fps[i] == want_interval)
                    {
                        uvc_device_param_config->fps = uvc_device_param->all_frame.mjpeg_frame[index].fps[i];
                        fps_flag = true;
                    }
                }

                if (resolution_flag)
                {
                    // have adapt this resolution
                    if (fps_flag == false)
                    {
                        uvc_device_param_config->fps = uvc_device_param->all_frame.mjpeg_frame[index].fps[0];
                        fps_flag = true;
                    }
                    break;
                }
            }
            break;

        case IMAGE_H264:
            uvc_device_param_config->format_index = uvc_device_param->format_index.h264_format_index;
            frame_num = uvc_device_param->all_frame.h264_frame_num;
            for (index = 0; index < frame_num; index++)
            {
                LOGV("H264 width:%d heigth:%d index:%d\r\n",
                     uvc_device_param->all_frame.h264_frame[index].width,
                     uvc_device_param->all_frame.h264_frame[index].height,
                     uvc_device_param->all_frame.h264_frame[index].index);

                if (uvc_device_param->all_frame.h264_frame[index].width == user_config->width
                    && uvc_device_param->all_frame.h264_frame[index].height == user_config->height)
                {
                    uvc_device_param_config->frame_index = uvc_device_param->all_frame.h264_frame[index].index;
                    uvc_device_param_config->width = uvc_device_param->all_frame.h264_frame[index].width;
                    uvc_device_param_config->height = uvc_device_param->all_frame.h264_frame[index].height;
                    resolution_flag = true;
                }

                // iterate all support fps of current resolution
                for (int i = 0; i < uvc_device_param->all_frame.h264_frame[index].fps_num; i++)
                {
                    LOGV("H264 fps:%d\r\n", uvc_device_param->all_frame.h264_frame[index].fps[i]);

                    if (resolution_flag
                        && uvc_device_param->all_frame.h264_frame[index].fps[i] == want_interval)
                    {
                        uvc_device_param_config->fps = uvc_device_param->all_frame.h264_frame[index].fps[i];
                        fps_flag = true;
                    }
                }

                if (resolution_flag)
                {
                    // have adapt this resolution
                    if (fps_flag == false)
                    {
                        uvc_device_param_config->fps = uvc_device_param->all_frame.h264_frame[index].fps[0];
                        fps_flag = true;
                    }
                    break;
                }
            }
            break;

        case IMAGE_H265:
            uvc_device_param_config->format_index = uvc_device_param->format_index.h265_format_index;
            frame_num = uvc_device_param->all_frame.h265_frame_num;
            for (index = 0; index < frame_num; index++)
            {
                LOGV("H265 width:%d heigth:%d index:%d\r\n",
                     uvc_device_param->all_frame.h265_frame[index].width,
                     uvc_device_param->all_frame.h265_frame[index].height,
                     uvc_device_param->all_frame.h265_frame[index].index);

                if (uvc_device_param->all_frame.h265_frame[index].width == user_config->width
                    && uvc_device_param->all_frame.h265_frame[index].height == user_config->height)
                {
                    uvc_device_param_config->frame_index = uvc_device_param->all_frame.h265_frame[index].index;
                    uvc_device_param_config->width = uvc_device_param->all_frame.h265_frame[index].width;
                    uvc_device_param_config->height = uvc_device_param->all_frame.h265_frame[index].height;
                    resolution_flag = true;
                }

                // iterate all support fps of current resolution
                for (int i = 0; i < uvc_device_param->all_frame.h265_frame[index].fps_num; i++)
                {
                    LOGV("H265 fps:%d\r\n", uvc_device_param->all_frame.h265_frame[index].fps[i]);

                    if (resolution_flag
                        && uvc_device_param->all_frame.h265_frame[index].fps[i] == want_interval)
                    {
                        uvc_device_param_config->fps = uvc_device_param->all_frame.h265_frame[index].fps[i];
                        fps_flag = true;
                    }
                }

                if (resolution_flag)
                {
                    // have adapt this resolution
                    if (fps_flag == false)
                    {
                        uvc_device_param_config->fps = uvc_device_param->all_frame.h265_frame[index].fps[0];
                        fps_flag = true;
                    }
                    break;
                }
            }
            break;

        default:
            ret = BK_FAIL;
            LOGE("%s, please check usb output format:%d\r\n", __func__, user_config->img_format);
            break;
    }

    if (ret != BK_OK)
    {
        LOGE("%s please check usb output format:%d\r\n", __func__, user_config->img_format);
        return ret;
    }

    if (resolution_flag == false)
    {
        LOGE("%s, not support this resolution:%dX%d\r\n", __func__, user_config->width, user_config->height);
        ret = BK_FAIL;
        return ret;
    }

    uvc_device_param_config->ep_desc = uvc_device_param->ep_desc;

    // 初始化丢帧计数器
    g_uvc_frame_drop_counter = 0;
    
    if (g_uvc_frame_drop_mode > 0)
    {
        LOGD("%s, Frame drop enabled: mode=%d (keep 1 per %d frames)\r\n",
             __func__, g_uvc_frame_drop_mode, g_uvc_frame_drop_mode + 1);
    }
    else
    {
        LOGD("%s, Frame drop disabled\r\n", __func__);
    }

    LOGV("%s, %d\r\n", __func__, __LINE__);

    return BK_OK;
}

void uvc_camera_stream_connect_callback(bk_usb_hub_port_info *port_info, void *arg)
{
    uvc_stream_handle_t *uvc_handle = (uvc_stream_handle_t *)arg;

    LOGD("%s, device_index:%d, port:%d\n", __func__, port_info->device_index, port_info->port_index);

    camera_param_t *camera_param = uvc_camera_stream_node_get_by_port_info(uvc_handle, port_info);
    if (camera_param)
    {
        if (camera_param->camera_state == UVC_CONNECT_STATE)
        {
            LOGW("%s, %d, port %d already connected...\r\n", __func__, __LINE__, port_info->port_index);
        }
        else if (camera_param->camera_state == UVC_STREAMING_STATE)
        {
            LOGW("%s, port:%d, already streaming....\r\n", __func__, port_info->port_index);
            camera_param->camera_state = UVC_DISCONNECT_STATE;
            uvc_stream_task_send_msg(UVC_DISCONNECT_IND, port_info->port_index);
        }
        else if (camera_param->camera_state == UVC_CLOSING_STATE)
        {
            // uplayer have send stop cmd, but not respones immediately
            LOGW("%s, port:%d, is closing....\r\n", __func__, port_info->port_index);
            return;
        }

        camera_param->camera_state = UVC_CONNECT_STATE;
    }

    uvc_handle->connect_camera_count++;
    LOGD("%s, port:%d\n", __func__, port_info->port_index);
    rtos_set_event_flags(&uvc_handle->handle, UVC_CONNECT_BIT);

    if (camera_param)
    {
        uvc_stream_task_send_msg(UVC_CONNECT_IND, (uint32_t)camera_param);
    }

    if (uvc_connect_state_cb)
    {
        uvc_connect_state_cb(port_info, BK_UVC_CONNECT);
    }
}

void uvc_camera_stream_disconnect_callback(bk_usb_hub_port_info *port_info, void *arg)
{
    uvc_stream_handle_t *uvc_handle = (uvc_stream_handle_t *)arg;

    LOGD("%s, %d, port:%d\r\n", __func__, __LINE__, port_info->port_index);
    camera_param_t *camera_param = uvc_camera_stream_node_get_by_port_info(uvc_handle, port_info);
    if (camera_param)
    {
        if (camera_param->camera_state == UVC_DISCONNECT_STATE)
        {
            LOGW("%s, %d, this port %d uvc have disconnected\r\n", __func__, __LINE__, port_info->port_index);
            return;
        }

        rtos_lock_mutex(&uvc_handle->mutex);
        camera_param->camera_state = UVC_DISCONNECT_STATE;
        rtos_unlock_mutex(&uvc_handle->mutex);
    }

    uvc_handle->connect_camera_count--;

    rtos_clear_event_flags(&uvc_handle->handle, UVC_CONNECT_BIT);

    if (uvc_connect_state_cb)
    {
        uvc_connect_state_cb(port_info, BK_UVC_DISCONNECT);
    }

    if (camera_param)
    {
        uvc_stream_task_send_msg(UVC_DISCONNECT_IND, (uint32_t)camera_param);
    }
}


static bk_err_t uvc_camera_stream_packet_urb(camera_param_t *camera_param)
{
    LOGV("%s, %d\r\n", __func__, __LINE__);
    struct usbh_video *uvc_device = NULL;
    struct usbh_hubport *hport = NULL;
    struct usbh_urb *urb = camera_param->urb;
    uvc_pro_config_t *pro_config = s_uvc_stream_handle->pro_config;

    if (camera_param->camera_state != UVC_STREAMING_STATE || camera_param->port_info == NULL)
    {
        LOGW("%s, param or state error:%d...\r\n", __func__, camera_param->camera_state);
        return BK_UVC_DISCONNECT;
    }

    uvc_device = (struct usbh_video *)(camera_param->port_info->usb_device);
    hport = camera_param->port_info->hport;
    urb->pipe = (usbh_pipe_t)(uvc_device->isoin);
    if (urb->pipe == NULL)
    {
        LOGE("%s, %d\r\n", __func__, __LINE__);
    }
    urb->complete = (usbh_complete_callback_t)uvc_camera_stream_receive_complete_callback;
    urb->arg = (void *)camera_param;

    if (pro_config->transfer_bulk[camera_param->index]) // bulk transmission
    {
        urb->timeout = 0;
    }
    else // iso transmission
    {
        if (s_uvc_stream_handle->connect_camera_count >= 2)
        {
            if (((struct usbh_hubport *)hport)->speed == 3) //USB_SPEED_HIGH
            {
                urb->timeout = 0;//150us
            }
            if (((struct usbh_hubport *)hport)->speed == 2) //USB_SPEED_FULL
            {
                urb->timeout = 0;//1ms
            }
        }
        else
        {
            urb->timeout = 0;
        }
    }

    return BK_OK;
}

static void uvc_camera_stream_receive_complete_callback(void *pCompleteParam, int nbytes)
{
    UVC_PACKET_PUSH_START();
    LOGV("%s, %d, %d\r\n", __func__, __LINE__, (uint32_t)pCompleteParam);
    struct usbh_urb *urb = NULL;//, *new_urb = NULL;
    camera_param_t *camera_param = (camera_param_t *)pCompleteParam;
    int ret = BK_FAIL;
    urb = camera_param->urb;

    if (urb == NULL)
    {
        LOGW("%s, %d, %d\r\n", __func__, __LINE__, (uint32_t)pCompleteParam);
        UVC_PACKET_PUSH_END();
        return;
    }

    if (nbytes < 0)
    {
        urb->errorcode = nbytes;
    }

    camera_param->urb = NULL;

    uvc_camera_urb_push(urb);

    if (camera_param->camera_state != UVC_STREAMING_STATE)
    {
        LOGV("[%d]%s, %d, %d\r\n", index, __func__, __LINE__, camera_param->camera_state);
        rtos_set_semaphore(&camera_param->sem);
        return;
    }

    urb = uvc_camera_urb_malloc();

    if (urb)
    {
        camera_param->urb = urb;
        ret = uvc_camera_stream_packet_urb(camera_param);
        if (ret != BK_OK)
        {
            camera_param->urb = NULL;
            uvc_camera_urb_free(urb);
            return;
        }

        ret = bk_usbh_hub_dev_request_data(camera_param->info->port, camera_param->port_info->device_index, camera_param->urb);
        if (ret != BK_OK)
        {
            camera_param->urb = NULL;
            uvc_camera_urb_free(urb);
            if (uvc_stream_task_send_msg(UVC_DATA_REQUEST_IND, (uint32_t)camera_param) != BK_OK)
            {
                LOGW("%s, %d send failed...\r\n", __func__, __LINE__);
            }
        }
    }
    else
    {
        if (uvc_stream_task_send_msg(UVC_DATA_REQUEST_IND, (uint32_t)camera_param) != BK_OK)
        {
            LOGW("%s, %d send failed...\r\n", __func__, __LINE__);
        }
    }

    UVC_PACKET_PUSH_END();
}

static bk_err_t uvc_camera_stream_data_request_retry_handle(camera_param_t *param, int value)
{
    int ret = BK_OK;
    uint8_t port = param->info->port;
    switch (-value)
    {
        case EBUSY:
            LOGV("%s port:%d, Urb is EBUSY\r\n", __func__, port);
            ret = uvc_stream_task_send_msg(UVC_DATA_REQUEST_IND, (uint32_t)param);
            break;
        case ENODEV:
            LOGV("%s port:%d, ENODEV Please check device connect\r\n", __func__, port);
            ret = BK_FAIL;
            break;
        case EINVAL:
            LOGV("%s port:%d, EINVAL Please check pipe or urb\r\n", __func__, port);
            //bk_usb_drv_send_msg(USB_DRV_VIDEO_START, id);
            ret = BK_FAIL;
            break;
        case ESHUTDOWN:
            LOGV("%s port:%d, ESHUTDOWN Check device Disconnect\r\n", __func__, port);
            ret = BK_FAIL;
            break;
        case ETIMEDOUT:
            LOGV("%s port:%d, ETIMEDOUT Timeout wait\r\n", __func__, port);
            ret = uvc_stream_task_send_msg(UVC_DATA_REQUEST_IND, (uint32_t)param);
            break;
        default:
            LOGV("%s port:%d, Fail to submit urb:%d\r\n", __func__, port, value);
            ret = BK_FAIL;
            break;
    }

    return ret;
}

static void uvc_camera_stream_data_request_handle(uint32_t param)
{
    struct usbh_urb *new_urb = NULL;
    uvc_stream_handle_t *uvc_handle = s_uvc_stream_handle;
    camera_param_t *uvc_param = (camera_param_t *)param;
    int ret = BK_OK;

    do
    {
        if (uvc_param->camera_state != UVC_STREAMING_STATE)
        {
            LOGW("[%d]%s, %d stream have stoped...\r\n", uvc_param->info->port, __func__, __LINE__);
            rtos_set_semaphore(&uvc_param->sem);
            break;
        }

        if (uvc_param->urb == NULL)
        {
            new_urb = uvc_camera_urb_malloc();

            if (new_urb)
            {
                // apply data
                uvc_param->urb = new_urb;
            }
            else
            {
                // malloc fail, retry
                uvc_handle->pro_config->packet_error[uvc_param->info->port - 1] = true;
                rtos_delay_milliseconds(5);
                LOGD("%s, %d retry.....\r\n", __func__, __LINE__);
                if (uvc_stream_task_send_msg(UVC_DATA_REQUEST_IND, param) != BK_OK)
                {
                    LOGW("%s, %d send fail.\r\n", __func__, __LINE__);
                }
                break;
            }
        }

        ret = uvc_camera_stream_packet_urb(uvc_param);
        if (ret != BK_OK)
        {
            LOGW("%s, %d, port:%d, disconnect.....\r\n", __func__, __LINE__, uvc_param->info->port);
            break;
        }

        ret = bk_usbh_hub_dev_request_data(uvc_param->info->port, uvc_param->port_info->device_index, uvc_param->urb);
        {
            if (ret == BK_OK)
            {
                break;
            }
            else if (ret == BK_FAIL)
            {
                LOGW("%s, %d, port:%d, disconnect.....\r\n", __func__, __LINE__, uvc_param->info->port);
                break;
            }
            else
            {
                ret = uvc_camera_stream_data_request_retry_handle(uvc_param, ret);
                if (ret != BK_OK)
                {
                    LOGW("%s, %d, port:%d, retry error:%d.....\r\n", __func__, __LINE__, uvc_param->info->port, ret);
                    break;
                }
            }
        }

    }
    while (0);


    if (ret != BK_OK && uvc_param->urb)
    {
        uvc_camera_urb_free(uvc_param->urb);
        new_urb = uvc_param->urb = NULL;
    }
}

void uvc_camera_stream_printf_config(bk_usb_hub_port_info *port_info)
{
    bk_uvc_config_t *uvc_device_param_config = (bk_uvc_config_t *)port_info->usb_device_param_config;

    struct s_bk_usb_endpoint_descriptor *ep_desc = uvc_device_param_config->ep_desc;

    LOGD("=========================================================================\r\n");
    LOGD("------------ Endpoint Descriptor -----------\r\n");
    LOGD("bLength					: 0x%x (%d bytes)\r\n", ep_desc->bLength, ep_desc->bLength);
    LOGD("bDescriptorType				: 0x%x (Endpoint Descriptor)\r\n", ep_desc->bDescriptorType);
    LOGD("bEndpointAddress				: 0x%x (Direction=IN  EndpointID=%d)\r\n", ep_desc->bEndpointAddress, (ep_desc->bEndpointAddress & 0x0F));
    LOGD("bmAttributes				: 0x%x\r\n", ep_desc->bmAttributes);
    LOGD("wMaxPacketSize				: 0x%x (%d bytes)\r\n", ep_desc->wMaxPacketSize, ep_desc->wMaxPacketSize);
    LOGD("bInterval 				: 0x%x (%d ms)\r\n", ep_desc->bInterval, ep_desc->bInterval);
    LOGD("%s uvc_set_param VID:0x%x\r\n", __func__, uvc_device_param_config->vendor_id);
    LOGD("%s uvc_set_param PID:0x%x\r\n", __func__, uvc_device_param_config->product_id);
    LOGD("%s uvc_set_param width:%d\r\n", __func__, uvc_device_param_config->width);
    LOGD("%s uvc_set_param height:%d\r\n", __func__, uvc_device_param_config->height);
    LOGD("%s uvc_set_param fps:%d\r\n", __func__, uvc_device_param_config->fps);
    LOGD("%s uvc_set_param frame_index:%d\r\n", __func__, uvc_device_param_config->frame_index);
    LOGD("%s uvc_set_param format_index:%d\r\n", __func__, uvc_device_param_config->format_index);
}

bk_err_t uvc_camera_stream_rx_config(uvc_stream_handle_t *uvc_handle, camera_param_t *uvc_param)
{
    int ret = BK_OK;
    struct usbh_urb *urb = NULL;
    uint8_t index = uvc_param->index;
    frame_buffer_t *new_frame = NULL;

    rtos_lock_mutex(&uvc_handle->mutex);

    if (uvc_param->camera_state != UVC_CONNECT_STATE || uvc_handle->pro_enable == false)
    {
        LOGE("%s, state:%d, pro_task:%d\n", __func__, uvc_param->camera_state, uvc_handle->pro_enable);
        ret = BK_UVC_DISCONNECT;
        goto out;
    }

    // setp 1 check presuppose frame_info
    if (uvc_param->info == NULL)
    {
        LOGE("%s, camera presuppose frame_info is empty....\n", __func__);
        ret = BK_UVC_NO_RESOURCE;
        goto out;
    }

    uvc_param->camera_state = UVC_CONFIGING_STATE;

    // step 2: need compare param support and self defined
    ret = uvc_camera_stream_check_config(uvc_param);
    if (ret != BK_OK)
    {
        LOGE("%s, not support this solution, please retry...\n", __func__);
        ret = BK_UVC_PPI_ERROR;
        goto out;
    }

    // step 3: set endpoint
    bk_usb_hub_port_info *port_info = uvc_param->port_info;
    struct usbh_video *video_class = (struct usbh_video *)(port_info->usb_device);
    bk_uvc_device_brief_info_t *uvc_device_param = (bk_uvc_device_brief_info_t *)port_info->usb_device_param;
    struct usb_endpoint_descriptor *ep_desc = (struct usb_endpoint_descriptor *)uvc_device_param->ep_desc;
    usbh_hport_activate_epx(&video_class->isoin, video_class->hport, ep_desc);

    // step 4: make sure transmission mode
    bk_uvc_config_t *uvc_config = (bk_uvc_config_t *)uvc_param->port_info->usb_device_param_config;
    uvc_handle->pro_config->transfer_bulk[index] = ((uvc_config->ep_desc->bmAttributes & 0x3) == USB_ENDPOINT_BULK_TRANSFER) ? true : false;
    uvc_handle->pro_config->max_packet_size[index] = uvc_config->ep_desc->wMaxPacketSize > 1024 ? 1024 : uvc_config->ep_desc->wMaxPacketSize;
    LOGD("/*****port:%d, transmission mode:%s, max_packet_zise:%d*****/\r\n", uvc_param->info->port, uvc_handle->pro_config->transfer_bulk[index] == 1 ? "BULK" : "ISO",
         uvc_handle->pro_config->max_packet_size[index]);
    uint32_t max_packet_size = uvc_handle->pro_config->max_packet_size[index];
#ifdef CONFIG_PSRAM
    max_packet_size = UVC_MAX_PACKET_SIZE;
#endif
    ret = uvc_camera_urb_list_init(max_packet_size);
    if (ret != BK_OK)
    {
        LOGE("%s, %d\n", __func__, __LINE__);
        ret = BK_UVC_NO_MEMORY;
        goto out;
    }

    if (uvc_separate_packet_cb.uvc_init_packet_cb != NULL && uvc_separate_packet_cb.id == uvc_param->info->port)
    {
        ret = uvc_separate_packet_cb.uvc_init_packet_cb(uvc_param->info, 1, &uvc_handle->callback);
        if (ret != BK_OK)
        {
            LOGE("%s, %d\n", __func__, __LINE__);
            ret = BK_UVC_NO_MEMORY;
            goto out;
        }
    }

    // there can modify to adapt one uvc output two or more stream, there default ues stream
    uvc_param->stream = uvc_handle->callback.frame_init(uvc_param->info->port, UVC_CAMERA, uvc_param->info->img_format);
    if (uvc_param->stream == NULL)
    {
        LOGE("%s, %d not register callback, or stream create fail\n", __func__, __LINE__);
        ret = BK_UVC_NOT_PERMIT;
        goto out;
    }

    ret = bk_usbh_hub_port_dev_open(uvc_param->info->port, uvc_param->port_info->device_index, uvc_param->port_info);
    if (ret != BK_OK)
    {
        // uvc open fail;
        LOGE("%s, %d, port:%d\n", __func__, __LINE__, uvc_param->info->port);
        ret = BK_UVC_NOT_PERMIT;
        goto out;
    }

    // step 5: malloc frame buffer
    if (uvc_param->frame == NULL)
    {
        switch (uvc_param->info->img_format)
        {
            case IMAGE_MJPEG:
                new_frame = uvc_handle->callback.frame_malloc(IMAGE_MJPEG, uvc_param->stream, CONFIG_JPEG_FRAME_SIZE);
                if (new_frame)
                {
                    new_frame->fmt = PIXEL_FMT_JPEG;
                }
                break;

            case IMAGE_H264:
                new_frame = uvc_handle->callback.frame_malloc(IMAGE_H264, uvc_param->stream, CONFIG_H264_FRAME_SIZE);
                if (new_frame)
                {
                    new_frame->fmt = PIXEL_FMT_H264;
                }
                break;

            case IMAGE_H265:
                new_frame = uvc_handle->callback.frame_malloc(IMAGE_H265, uvc_param->stream, CONFIG_H264_FRAME_SIZE);
                if (new_frame)
                {
                    new_frame->fmt = PIXEL_FMT_H265;
                }
                break;

            case IMAGE_YUV:
                new_frame = uvc_handle->callback.frame_malloc(IMAGE_YUV, uvc_param->stream, uvc_param->info->width * uvc_param->info->height * 2);
                if (new_frame)
                {
                    new_frame->fmt = PIXEL_FMT_YUV422;
                }
                break;

            default:
                break;
        }

        if (new_frame == NULL)
        {
            LOGE("%s, %d, %d\r\n", __func__, __LINE__, uvc_param->info->img_format);
            if (uvc_param->urb)
            {
                uvc_camera_urb_free(uvc_param->urb);
                uvc_param->urb = NULL;
            }
            ret = BK_UVC_NO_RESOURCE;
            goto out;
        }

        new_frame->width = uvc_param->info->width;
        new_frame->height = uvc_param->info->height;
        new_frame->sequence = uvc_handle->pro_config->frame_id[index]++;
        uvc_param->frame = new_frame;
    }

    // step 6: malloc urb
    if (uvc_param->urb == NULL)
    {
        urb = uvc_camera_urb_malloc();
        if (urb == NULL)
        {
            uvc_handle->callback.frame_free(uvc_param->info->img_format, uvc_param->stream, uvc_param->frame);
            uvc_param->frame = NULL;
            LOGW("%s, malloc fb failed---%d\r\n", __func__, __LINE__);
            ret = BK_UVC_NO_RESOURCE;
            goto out;
        }

        uvc_param->urb = urb;
    }

    // step 7: config urb
    uvc_param->camera_state = UVC_STREAMING_STATE;
    ret = uvc_camera_stream_packet_urb(uvc_param);
    LOGV("%s, %d, %p, %p, ret:%d\r\n", __func__, __LINE__, urb, uvc_param->urb, ret);

    // step 8: requeset uvc data
    rtos_clear_event_flags(&uvc_handle->handle, UVC_PROCESS_TASK_START_BIT);
    rtos_set_event_flags(&uvc_handle->handle, UVC_PROCESS_TASK_START_BIT);
    ret = bk_usbh_hub_dev_request_data(uvc_param->info->port, uvc_param->port_info->device_index, urb);
    if (ret != BK_OK)
    {
        uvc_param->camera_state = UVC_CONNECT_STATE;
        uvc_handle->callback.frame_free(uvc_param->info->img_format, uvc_param->stream, uvc_param->frame);
        uvc_camera_urb_free(uvc_param->urb);
        uvc_param->frame = NULL;
        uvc_param->urb = NULL;
        LOGE("[%d]%s, %d, ret:%d\r\n", uvc_param->info->port, __func__, __LINE__, ret);
        ret = BK_UVC_NO_RESPON;
    }

out:
    if (uvc_param->camera_state == UVC_CONFIGING_STATE)
    {
        uvc_param->camera_state = UVC_CONNECT_STATE;
    }
    rtos_unlock_mutex(&uvc_handle->mutex);
    LOGD("[%d]%s, %d, state:%d\r\n", uvc_param->info->port, __func__, __LINE__, uvc_param->camera_state);
    return ret;
}

void uvc_camera_stream_stop_handle(uint32_t param)
{
    uvc_stream_handle_t *uvc_handle = s_uvc_stream_handle;
    camera_param_t *uvc_param = (camera_param_t *)param;

    LOGV("%s, %d, camera:%d\r\n", __func__, __LINE__, uvc_param->info->port);

    if (uvc_param->camera_state == UVC_CLOSING_STATE)
    {
        if (rtos_get_semaphore(&uvc_param->sem, 100) != BK_OK)
        {
            LOGE("%s, %d timeout\r\n", __func__, __LINE__);
        }

        uvc_param->camera_state = UVC_CONNECT_STATE;
    }

    if (uvc_param->port_info && uvc_param->camera_state != UVC_DISCONNECT_STATE)
    {
        bk_usbh_hub_port_dev_close(uvc_param->info->port, uvc_param->port_info->device_index, uvc_param->port_info);
    }

    // step 1: free urb
    if (uvc_param->urb)
    {
        uvc_camera_urb_free(uvc_param->urb);
        uvc_param->urb = NULL;
    }

    LOGV("%s, %d\r\n", __func__, __LINE__);

    // step 2: free frame_buffer
    if (uvc_param->frame)
    {
        uvc_handle->callback.frame_free(uvc_param->info->img_format, uvc_param->stream, uvc_param->frame);
        uvc_param->frame = NULL;
    }

    LOGV("%s, %d\r\n", __func__, __LINE__);

    if (uvc_param->stream)
    {
        uvc_handle->callback.frame_clear(uvc_param->stream);
        uvc_handle->callback.frame_deinit(uvc_param->stream);
        uvc_param->stream = NULL;
    }

    if (uvc_separate_packet_cb.uvc_init_packet_cb != NULL && uvc_separate_packet_cb.id == uvc_param->info->port)
    {
        uvc_separate_packet_cb.uvc_init_packet_cb(NULL, 0, NULL);
    }

    LOGD("%s, %d, %d\r\n", __func__, __LINE__, uvc_param->camera_state);

    rtos_set_event_flags(&uvc_handle->handle, UVC_CLOSE_BIT);
}

bk_err_t uvc_camera_stream_start_handle(uint32_t param)
{
    int ret = BK_OK;
    uvc_stream_handle_t *uvc_handle = s_uvc_stream_handle;
    camera_param_t *uvc_param = (camera_param_t *)param;

    ret = uvc_camera_stream_rx_config(uvc_handle, uvc_param);

    if (ret != BK_OK)
    {
        // maybe send notify to user
        if (uvc_param->camera_state != UVC_DISCONNECT_STATE)
        {
            uvc_param->camera_state = UVC_CONNECT_STATE;
        }

        if (uvc_connect_state_cb)
        {
            uvc_connect_state_cb(uvc_param->port_info, ret);
        }

        LOGW("uvc config error, %d, camera_id:%d\n", __LINE__, uvc_param->info->port);
        uvc_camera_stream_stop_handle(param);
    }
    else
    {
        UVC_INIT_END();
    }

    rtos_set_event_flags(&uvc_handle->handle, UVC_STREAM_START_BIT);

    return ret;
}

static void uvc_camera_stream_connect_handle(uint32_t param)
{
    int ret = BK_OK;
    camera_param_t *uvc_param = (camera_param_t *)param;
    uvc_stream_handle_t *uvc_handle = s_uvc_stream_handle;

    ret = uvc_camera_stream_rx_config(uvc_handle, uvc_param);
    if (ret != BK_OK)
    {
        // maybe send notify to user
        if (uvc_param->camera_state != UVC_DISCONNECT_STATE)
        {
            uvc_param->camera_state = UVC_CONNECT_STATE;
        }
        LOGW("uvc config error, camera_id:%d\r\n", uvc_param->info->port);
        if (uvc_connect_state_cb)
        {
            uvc_connect_state_cb(uvc_param->port_info, ret);
        }

        uvc_camera_stream_stop_handle(param);
    }
}

static void uvc_camera_stream_disconnect_handle(uint32_t param)
{
    camera_param_t *uvc_param = (camera_param_t *)param;

    // step 1: free urb
    if (uvc_param->urb)
    {
        uvc_camera_urb_free(uvc_param->urb);
        uvc_param->urb = NULL;
    }

    // step 2: free frame_buffer
    if (uvc_param->frame)
    {
        s_uvc_stream_handle->callback.frame_free(uvc_param->info->img_format, uvc_param->stream, uvc_param->frame);
        uvc_param->frame = NULL;
    }
}

bk_err_t uvc_camera_stream_check_frame_buffer_length(frame_buffer_t *frame, uint32_t total_length)
{
    //! 这里帧大小可以等于总长度
    if (frame->size < total_length)
    {
        return BK_FAIL;
    }

    return BK_OK;
}

int uvc_camera_stream_check_frame_buffer_sof_eof_mask(frame_buffer_t *frame)
{
    int ret = BK_FAIL;
    uint8_t *data = frame->frame;
    uint32_t length = frame->length;

    switch (frame->fmt)
    {
        case PIXEL_FMT_JPEG:
            if (data[0] == 0xFF && data[1] == 0xD8)
            {
                for (uint32_t i = length - 1; i > length - 20; i--)
                {
                    if (data[i - 1] == 0xFF && data[i] == 0xD9)
                    {
                        ret = i + 1;
                        break;
                    }
                }
            }
            break;

        case PIXEL_FMT_H264:
        case PIXEL_FMT_H265:
            if (data[0] == 0x00 && data[1] == 0x00
                && data[2] == 0x00 && data[3] == 0x01)
            {
                ret = BK_OK;
            }
            break;

        case PIXEL_FMT_YUV422:
            ret = BK_OK;
            break;

        default:
            break;
    }

    return ret;
}


static void uvc_camera_stream_eof_handle(camera_param_t *camera_param, uvc_pro_config_t *pro_config)
{
    UVC_EOF_START();
    uvc_stream_handle_t *uvc_handle = s_uvc_stream_handle;
    uint8_t index = camera_param->index;
    frame_buffer_t *new_frame = NULL, *curr_frame_buffer = camera_param->frame;
    rtos_lock_mutex(&uvc_handle->mutex);

    if (pro_config->packet_error[index]
        || curr_frame_buffer->length <= 1024 * 5
        || pro_config->stream_state != UVC_STREAM_STATE_RUNNING)
    {
        LOGV("%s, %d, length:%d, stream_state:%d\n", __func__, __LINE__, curr_frame_buffer->length, pro_config->stream_state);
        pro_config->packet_error[index] = false; // clear packet_error flag
        curr_frame_buffer->length = 0;
        if (pro_config->stream_state != UVC_STREAM_STATE_RUNNING)
        {
            pro_config->stream_state = UVC_STREAM_STATE_RUNNING;
            camera_param->info->drop_num = 1; // more drop 1 frame
        }
        rtos_unlock_mutex(&uvc_handle->mutex);
        goto out;
    }

    rtos_unlock_mutex(&uvc_handle->mutex);

    int check_length = uvc_camera_stream_check_frame_buffer_sof_eof_mask(curr_frame_buffer);

    if (check_length < 0)
    {
        LOGD("%s, %d, frame_length:%d\r\n", __func__, __LINE__, curr_frame_buffer->length);
        curr_frame_buffer->length = 0;
        goto out;
    }
    else if (check_length > 0)
    {
        curr_frame_buffer->length = check_length;
    }
    else
    {
        // h264/h265
    }

#if (MEDIA_DEBUG_TIMER_ENABLE)
    pro_config->curr_length[index] = curr_frame_buffer->length;
#endif

    curr_frame_buffer->timestamp = get_current_timestamp();

    // 全局丢帧控制
    bool should_drop_frame = false;
    
    if (g_uvc_frame_drop_mode > 0)
    {
        // 递增帧计数器
        g_uvc_frame_drop_counter++;
        
        // 计算周期：mode + 1
        // mode=1: 周期2, 保留counter=1, 丢弃counter=2
        // mode=2: 周期3, 保留counter=1, 丢弃counter=2,3
        uint8_t period = g_uvc_frame_drop_mode + 1;
        
        if (g_uvc_frame_drop_counter > period)
        {
            g_uvc_frame_drop_counter = 1;  // 重置计数器
        }
        
        // 只在counter=1时保留帧，其他情况都丢弃
        should_drop_frame = (g_uvc_frame_drop_counter > 1);
        
        if (should_drop_frame)
        {
            LOGV("[%d]%s, frame_drop: mode=%d, counter=%d/%d\r\n", 
                 index, __func__, g_uvc_frame_drop_mode, 
                 g_uvc_frame_drop_counter, period);
        }
    }

    if (camera_param->info->drop_num > 0)
    {
        camera_param->info->drop_num--;
        LOGD("[%d]%s, drop_num:%d\r\n", index, __func__, camera_param->info->drop_num);
    }
    else if (should_drop_frame)
    {
        // 丢帧：不分配新frame，不调用frame_complete
        LOGV("[%d]%s, frame dropped by fps control\r\n", index, __func__);
        curr_frame_buffer->length = 0;  // 清空当前帧
        return;
    }
    else
    {
        new_frame = uvc_handle->callback.frame_malloc(camera_param->info->img_format, camera_param->stream, curr_frame_buffer->size);
        if (new_frame)
        {
            new_frame->fmt = curr_frame_buffer->fmt;
            new_frame->width = curr_frame_buffer->width;
            new_frame->height = curr_frame_buffer->height;
            new_frame->sequence = pro_config->frame_id[index]++;
            new_frame->length = 0;
            uvc_handle->callback.frame_complete(camera_param->info->img_format, camera_param->stream, curr_frame_buffer);
            camera_param->frame = new_frame;
        }
    }

    LOGV("%s, %d, length:%d, fmt:%d\r\n", __func__, __LINE__, curr_frame_buffer->length, curr_frame_buffer->fmt);

    if (new_frame == NULL)
    {
        LOGW("%s, %d malloc frame fail, length:%d\n", __func__, __LINE__, curr_frame_buffer->length);
        curr_frame_buffer->length = 0;
        curr_frame_buffer->sequence = pro_config->frame_id[index]++;
    }

    if (uvc_separate_packet_cb.uvc_eof_packet_cb != NULL && uvc_separate_packet_cb.id == (index + 1))
    {
        uvc_separate_packet_cb.uvc_eof_packet_cb(camera_param->info);
    }

out:
    UVC_EOF_END();
}

static void uvc_camera_stream_packet_process(camera_param_t *camera_param, uint8_t *payload, uint32_t payload_len)
{
    uvc_stream_handle_t *uvc_handle = s_uvc_stream_handle;
    uvc_pro_config_t *pro_config = uvc_handle->pro_config;
    frame_buffer_t *curr_frame_buffer = camera_param->frame;
    uint8_t *data = NULL;
    uint8_t header_info = 0;
    uint8_t header_len = 0;
    uint8_t flag_zlp = 0;
    uint8_t flag_lstp = 0;
    uint8_t index = camera_param->index;
#if 0
    uint8_t variable_offset = 0;
#endif
    uint32_t data_len = 0;
    uint32_t bulk_req_len = 0;

    uint8_t bulk_trans = pro_config->transfer_bulk[index];

    // Check if frame buffer is valid
    if (curr_frame_buffer == NULL || curr_frame_buffer->frame == NULL)
    {
        LOGE("curr_frame_buffer NULL\n");
        return;
    }

    // Handle bulk transfer
    if (bulk_trans)
    {
        bulk_req_len = pro_config->max_packet_size[index];
        if (payload_len == 0)
        {
            flag_zlp = 1;
            LOGV("%s, payload_len == 0\r\n", __func__);
        }
        else
        {
            if (bulk_req_len != payload_len)
            {
                flag_lstp = 1;
            }
        }
    }
    else if (payload_len == 0)
    {
        // Ignore empty payload transfers (for ISO transfer)
        return;
    }

    LOGV("length:%d, index:%d\n", payload_len, index);

    /********************* Process header *******************/
    if (!flag_zlp)
    {
        LOGV("zlp=%d, lstp=%d, payload_len=%d, first=0x%02x, second=0x%02x\r\n", flag_zlp, flag_lstp, payload_len, payload[0], payload_len > 1 ? payload[1] : 0);

        // Check if it's a valid header
        if (payload_len >= payload[0]
            && (payload[0] == 12 || (payload[0] == 2 && !(payload[1] & 0x0C)) || (payload[0] == 6 && !(payload[1] & 0x08)))
            && (payload[1] & 0x80) && !(payload[1] & 0x30)
#ifdef CONFIG_UVC_CHECK_BULK_JPEG_HEADER
            && (!bulk_trans || ((payload[payload[0]] == 0xff) && (payload[payload[0] + 1] == 0xd8)))
#endif
           )
        {
            header_len = payload[0];
            data_len = payload_len - header_len;
            /* checking the end-of-header */
#if 0
            variable_offset = 2;
#endif
            header_info = payload[1];

            LOGV("header=%u info=0x%02x, payload_len = %u\r\n", header_len, header_info, payload_len);

            // Check error bit
            if (header_info & 0x40)
            {
                LOGW("bad packet: %02x, head_len:%d error bit set\r\n", header_info, header_len);
                pro_config->packet_error[index] = true;
#if (MEDIA_DEBUG_TIMER_ENABLE)
                pro_config->packet_err_num++;
#endif
                return;
            }
        }
        else
        {
            LOGV("reassembling %u + %u\r\n", curr_frame_buffer->length, payload_len);
            data_len = payload_len;
        }
    }

    // Handle header info change
    if (header_info)
    {
        if (pro_config->head_bit0[index] != (header_info & 1))
        {
            UVC_PACKET_HEAD_START();
            if (camera_param->frame->length > 0)
            {
                uvc_camera_stream_eof_handle(camera_param, pro_config);
                curr_frame_buffer = camera_param->frame;
            }

            pro_config->head_bit0[index] = (header_info & 1);
            UVC_PACKET_HEAD_END();
        }

#if 0 // do not explain pts and last_scr
        if (header_info & (1 << 2))
        {
            pts = DW_TO_INT(payload + variable_offset);
            variable_offset += 4;
        }

        if (header_info & (1 << 3))
        {
            last_scr = DW_TO_INT(payload + variable_offset);
            variable_offset += 6;
        }
#endif
    }

    /********************* Process data *****************/
    if (data_len >= 1)
    {
        data = payload + header_len;

        // Handle separate packet callback
        if (uvc_separate_packet_cb.uvc_separate_packet_cb && uvc_separate_packet_cb.id == (index + 1))
        {
            uvc_separate_packet_cb.uvc_separate_packet_cb(payload + header_len, data_len, &uvc_separate_info);

            if (uvc_separate_info.data_len != 0)
            {
                data     = uvc_separate_info.data_off;
                data_len = uvc_separate_info.data_len;
            }
            else
            {
                data_len = 0;
            }
        }

        // Copy data to frame buffer
        if (data_len >= 1 && !pro_config->packet_error[index])
        {
            // Fix logic bug: only set error flag when buffer space is insufficient
            if (uvc_camera_stream_check_frame_buffer_length(curr_frame_buffer, (curr_frame_buffer->length + data_len)) != BK_OK)
            {
                LOGE("Frame buffer overflow: current=%d, need=%d\n", curr_frame_buffer->length, data_len);
                pro_config->packet_error[index] = true;
            }
            else
            {
                    UVC_PACKET_COPY_START();
                    LOGV("uvc payload = %02x %02x...%02x %02x\n", payload[header_len], payload[header_len + 1], payload[payload_len - 2], payload[payload_len - 1]);
                    os_memcpy(curr_frame_buffer->frame + curr_frame_buffer->length, data, data_len);
                    curr_frame_buffer->length += data_len;
                    UVC_PACKET_COPY_END();
            }
        }
    }

    // Handle EOF condition
    if (((header_info & (1 << 1)) && !bulk_trans) || flag_zlp || flag_lstp)
    {
        UVC_EOF_BIT_START();
        LOGV("eof:%d, bulk_trans:%d, flag_zlp:%d, flag_lstp:%d\r\n", header_info & 0x2, bulk_trans, flag_zlp, flag_lstp);

        // Publish complete frame
        if (curr_frame_buffer->length != 0)
        {
            if (curr_frame_buffer->fmt == PIXEL_FMT_JPEG)
            {
                // Check SOF and EOF markers for JPEG frame
                if (uvc_camera_stream_check_frame_buffer_sof_eof_mask(curr_frame_buffer) > 0)
                {
                    uvc_camera_stream_eof_handle(camera_param, pro_config);
                }
                else
                {
                    LOGV("[EOF_bit]id:%d, %02x-%02x-%02x-%02x-%02x-%02x\r\n", index,
                         curr_frame_buffer->frame[0],
                         curr_frame_buffer->frame[1],
                         curr_frame_buffer->frame[2],
                         curr_frame_buffer->frame[3],
                         curr_frame_buffer->frame[curr_frame_buffer->length - 2],
                         curr_frame_buffer->frame[curr_frame_buffer->length - 1]);
                }
            }
            else
            {
                // Directly handle EOF for other formats
                uvc_camera_stream_eof_handle(camera_param, pro_config);
            }
        }

        pro_config->packet_error[index] = false;
        UVC_EOF_BIT_END();
    }
}

static void uvc_camera_process_task_main(beken_thread_arg_t data)
{
    struct usbh_urb *urb = NULL;
    uint8_t *payload = NULL;
    uvc_stream_handle_t *uvc_handle = (uvc_stream_handle_t *)data;
    uvc_pro_config_t *pro_config = uvc_handle->pro_config;

    uvc_handle->pro_enable = true;
    rtos_set_event_flags(&uvc_handle->handle, UVC_PROCESS_TASK_ENABLE_BIT);
    rtos_wait_for_event_flags(&uvc_handle->handle, UVC_PROCESS_TASK_START_BIT, true, true, BEKEN_WAIT_FOREVER);

    while (uvc_handle->pro_enable)
    {
        urb = uvc_camera_urb_pop();

        if (urb == NULL)
        {
            continue;
        }

        camera_param_t *camera_param = (camera_param_t *)urb->arg;

        if (camera_param->camera_state != UVC_STREAMING_STATE)
        {
            LOGV("%s, %d\r\n", __func__, __LINE__);
            uvc_camera_urb_free(urb);
            continue;
        }

        // complete urb error, do not need process
        if (uvc_handle->packet_cb)
        {
            UVC_PACKET_START();
            uvc_handle->packet_cb(urb);
            UVC_PACKET_END();
        }
        else
        {
            if (urb->errorcode != 0)
            {
                UVC_PACKET_START();
                pro_config->packet_error[camera_param->index] = true;
                // clear error code
                LOGV("%s, %d, %d\n", __func__, __LINE__, urb->errorcode);
                urb->errorcode = 0;
                UVC_PACKET_END();
#if (MEDIA_DEBUG_TIMER_ENABLE)
                pro_config->packet_err_num += 8;
#endif
            }
            else
            {
                for (uint8_t i = 0; i < urb->num_of_iso_packets; i++)
                {
#if (MEDIA_DEBUG_TIMER_ENABLE)
                    pro_config->all_packet_num++;
#endif
                    UVC_PACKET_START();
                    payload = urb->iso_packet[i].transfer_buffer;
                    if (urb->iso_packet[i].errorcode != BK_OK || pro_config->stream_state == UVC_STREAM_STATE_SUSPEND)
                    {
                        LOGV("[%d]%s, %d packet error:%d...\r\n", camera_param->info->port, __func__, __LINE__, urb->iso_packet[i].errorcode);
                        pro_config->packet_error[camera_param->index] = true;
                        // clear error code
                        urb->iso_packet[i].errorcode = 0;
#if (MEDIA_DEBUG_TIMER_ENABLE)
                        pro_config->packet_err_num++;
#endif
                    }
                    else
                    {
                        uvc_camera_stream_packet_process(camera_param, payload, urb->iso_packet[i].actual_length);
                    }
                    UVC_PACKET_END();
                }
            }
        }

        uvc_camera_urb_free(urb);
    };

    LOGD("%s, %d\r\n", __func__, __LINE__);
    uvc_handle->pro_thread = NULL;
    rtos_set_event_flags(&uvc_handle->handle, UVC_PROCESS_TASK_DISABLE_BIT);
    rtos_delete_thread(NULL);
}

static void uvc_camera_process_task_deinit(uvc_stream_handle_t *handle)
{
    uvc_pro_config_t *pro_config = handle->pro_config;
    if (pro_config && handle->pro_enable)
    {
        handle->pro_enable = false;
        uvc_camera_urb_list_clear();

        rtos_wait_for_event_flags(&handle->handle, UVC_PROCESS_TASK_DISABLE_BIT, true, true, BEKEN_WAIT_FOREVER);

#if (MEDIA_DEBUG_TIMER_ENABLE)
        if (pro_config->timer.handle)
        {
            rtos_stop_timer(&pro_config->timer);
            rtos_deinit_timer(&pro_config->timer);
        }
#endif

        os_free(pro_config);
    }

    handle->pro_config = NULL;
}


bk_err_t uvc_camera_process_task_init(uvc_stream_handle_t *handle, bk_uvc_callback_t *cb)
{
    bk_err_t ret = BK_OK;
    if (handle->pro_thread == NULL)
    {
        handle->pro_config = (uvc_pro_config_t *)os_malloc(sizeof(uvc_pro_config_t));
        if (handle->pro_config == NULL)
        {
            LOGE("s_uvc_pro_config malloc failed\n");
            ret = BK_UVC_NO_MEMORY;
            goto error;
        }

        os_memset(handle->pro_config, 0, sizeof(uvc_pro_config_t));
        os_memcpy(&handle->callback, cb, sizeof(bk_uvc_callback_t));

        rtos_clear_event_flags(&handle->handle, UVC_PROCESS_TASK_ENABLE_BIT);

        ret = rtos_smp_create_thread(&handle->pro_thread,
                                       BEKEN_DEFAULT_WORKER_PRIORITY - 3,
                                       "uvc_pro_task",
                                       (beken_thread_function_t)uvc_camera_process_task_main,
                                       1024 * 2,
                                       (beken_thread_arg_t)handle);

        if (BK_OK != ret)
        {
            LOGE("%s uvc process task init failed\n", __func__);
            os_free(handle->pro_config);
            handle->pro_config = NULL;
            goto error;
        }

        rtos_wait_for_event_flags(&handle->handle, UVC_PROCESS_TASK_ENABLE_BIT, true, true, BEKEN_WAIT_FOREVER);

#if (MEDIA_DEBUG_TIMER_ENABLE)
        ret = rtos_init_timer(&handle->pro_config->timer, UVC_TIME_INTERVAL * 1000,
                              uvc_camera_stream_timer_handle, handle);

        if (ret != BK_OK)
        {
            LOGE("%s, init uvc_handle->pro_config->timer fail....\r\n", __func__);
            ret = BK_UVC_NO_MEMORY;
            goto error;
        }

        rtos_start_timer(&handle->pro_config->timer);
#endif
    }

    return ret;

error:

    uvc_camera_process_task_deinit(handle);
    return ret;
}

void uvc_camera_stream_task_main(beken_thread_arg_t data)
{
    int ret = BK_OK;
    uvc_stream_handle_t *uvc_handle = (uvc_stream_handle_t *)data;
    rtos_set_event_flags(&uvc_handle->handle, UVC_STREAM_TASK_ENABLE_BIT);

    while (1)
    {
        uvc_stream_event_t msg;
        ret = rtos_pop_from_queue(&uvc_handle->stream_queue, &msg, BEKEN_WAIT_FOREVER);
        if (ret == BK_OK)
        {
            LOGV("%s, %d, event:%d\r\n", __func__, __LINE__, msg.event);
            switch (msg.event)
            {
                case UVC_START_IND:
                    uvc_camera_stream_start_handle(msg.param);
                    break;

                case UVC_STOP_IND:
                    uvc_camera_stream_stop_handle(msg.param);
                    break;

                case UVC_CONNECT_IND:
                    uvc_camera_stream_connect_handle(msg.param);
                    break;

                case UVC_DISCONNECT_IND:
                    uvc_camera_stream_disconnect_handle(msg.param);
                    break;

                case UVC_DATA_REQUEST_IND:
                    uvc_camera_stream_data_request_handle(msg.param);
                    break;

                case UVC_EXIT_IND:
                    goto out;
                    break;

                default:
                    break;
            }
        }
    }

out:
    LOGD("%s, exit\r\n", __func__);
    rtos_deinit_queue(&uvc_handle->stream_queue);
    uvc_handle->stream_queue = NULL;
    uvc_handle->stream_thread = NULL;
    rtos_set_event_flags(&uvc_handle->handle, UVC_STREAM_TASK_DISABLE_BIT);
    rtos_delete_thread(NULL);
}

bk_err_t uvc_camera_stream_task_deinit(uvc_stream_handle_t *handle)
{
    if (handle)
    {
        if (handle->stream_thread)
        {
            rtos_clear_event_flags(&handle->handle, UVC_STREAM_TASK_DISABLE_BIT);
            if (uvc_stream_task_send_msg(UVC_EXIT_IND, 0) != BK_OK)
            {
                LOGE("%s, %d\r\n", __func__, __LINE__);
            }

            rtos_wait_for_event_flags(&handle->handle, UVC_STREAM_TASK_DISABLE_BIT, true, true, BEKEN_WAIT_FOREVER);
        }

        if (handle->stream_queue)
        {
            rtos_deinit_queue(&handle->stream_queue);
            handle->stream_queue = NULL;
        }

        if (handle->handle)
        {
            rtos_deinit_event_flags(&handle->handle);
        }

        if (handle->mutex)
        {
            rtos_deinit_mutex(&handle->mutex);
        }

        uvc_camera_stream_node_deinit(handle);
        uvc_camera_urb_list_deinit();

        os_free(handle);
    }

    bk_pm_module_vote_cpu_freq(PM_DEV_ID_USB_1, PM_CPU_FRQ_DEFAULT);

    return BK_OK;
}

bk_err_t uvc_camera_stream_task_init(uvc_stream_handle_t **handle)
{
    bk_err_t ret = BK_FAIL;
    uvc_stream_handle_t *stream_handle = *handle;
    if (stream_handle == NULL)
    {
        stream_handle = (uvc_stream_handle_t *)os_malloc(sizeof(uvc_stream_handle_t));
        if (stream_handle == NULL)
        {
            LOGE("stream_handle malloc failed\n");
            ret = BK_UVC_NO_MEMORY;
            return ret;
        }

        os_memset(stream_handle, 0, sizeof(uvc_stream_handle_t));
        INIT_LIST_HEAD(&stream_handle->list);

        bk_pm_module_vote_cpu_freq(PM_DEV_ID_USB_1, PM_CPU_FRQ_480M);

        ret = rtos_init_event_flags(&stream_handle->handle);

        rtos_init_mutex(&stream_handle->mutex);

        ret = rtos_init_queue(&stream_handle->stream_queue,
                              "uvc_stream_que",
                              sizeof(uvc_stream_event_t),
                              20);
        if (BK_OK != ret)
        {
            LOGE("%s stream_queue init failed\n", __func__);
            goto error;
        }

        rtos_clear_event_flags(&stream_handle->handle, UVC_STREAM_TASK_ENABLE_BIT);

        ret = rtos_create_thread(&stream_handle->stream_thread,
                                       BEKEN_DEFAULT_WORKER_PRIORITY - 2,
                                       "uvc_stream_task",
                                       (beken_thread_function_t)uvc_camera_stream_task_main,
                                       1536,
                                       (beken_thread_arg_t)stream_handle);

        if (BK_OK != ret)
        {
            LOGE("%s uvc stream task init failed\n", __func__);
            goto error;
        }

        rtos_wait_for_event_flags(&stream_handle->handle, UVC_STREAM_TASK_ENABLE_BIT, true, true, BEKEN_WAIT_FOREVER);

        *handle = stream_handle;
        LOGD("%s, %d, %p\n", __func__, __LINE__, *handle);
    }
    else
    {
        ret = BK_OK;
        LOGW("%s, already power on\n", __func__);
        UVC_POWER_ON_END();
        return ret;
    }

    rtos_clear_event_flags(&stream_handle->handle, UVC_CONNECT_BIT);

    return ret;

error:

    uvc_camera_stream_task_deinit(stream_handle);
    stream_handle = NULL;

    return ret;
}

bk_err_t uvc_camera_device_power_on(uvc_stream_handle_t *handle, E_USB_DEVICE_T device, uint32_t timeout)
{
    int ret = BK_FAIL;
    uint8_t port = 1;

    for (port = 1; port < UVC_PORT_MAX; port++)
    {
        // step 2.1: register connect callback
        ret = bk_usbh_hub_port_register_connect_callback(port, device, uvc_camera_stream_connect_callback, handle);
        if (ret != BK_OK)
        {
            LOGE("%s, %d, port:%d\r\n", __func__, __LINE__, port);
            ret = BK_UVC_NOT_PERMIT;
            goto error;
        }

        // step 2.2: register disconect callback
        ret = bk_usbh_hub_port_register_disconnect_callback(port, device, uvc_camera_stream_disconnect_callback, handle);
        if (ret != BK_OK)
        {
            LOGE("%s, %d, port:%d\r\n", __func__, __LINE__, port);
            ret = BK_UVC_NOT_PERMIT;
            goto error;
        }

        // step 2.3: power_on
        ret = bk_usbh_hub_multiple_devices_power_on(USB_HOST_MODE, port, device);
        if (ret != BK_OK)
        {
            LOGE("%s, %d, port:%d\r\n", __func__, __LINE__, port);
            ret = BK_UVC_POWER_ERROR;
            goto error;
        }
    }

    // check already connected
    bk_usb_hub_port_info *port_info = NULL;
    for (port = 1; port < UVC_PORT_MAX; port++)
    {
        ret = bk_usbh_hub_port_check_device(port, device, (bk_usb_hub_port_info **)&port_info);
        if (ret == BK_OK)
        {
            // already connected
            LOGD("%s, port:%d, check connect success\n", __func__, port);
            break;
        }
    }

    if (ret != BK_OK) // need wait connect callback
    {
        ret = rtos_wait_for_event_flags(&handle->handle, UVC_CONNECT_BIT, true, true, timeout);
        if (ret != UVC_CONNECT_BIT)
        {
            LOGE("%s, %d, %x:ret, connect timeout\r\n", __func__, __LINE__, ret);
            ret = BK_UVC_POWER_ERROR;
            goto error;
        }
    }

    ret = BK_OK;

    LOGD("%s, %d, complete....\r\n", __func__, __LINE__);

    UVC_POWER_ON_END();

    return ret;

error:

    for (port = 1; port < UVC_PORT_MAX; port++)
    {
        bk_usbh_hub_port_register_connect_callback(port, device, NULL, NULL);
        bk_usbh_hub_port_register_disconnect_callback(port, device, NULL, NULL);
        bk_usbh_hub_multiple_devices_power_down(USB_HOST_MODE, port, device);
    }

    LOGE("%s, %d failed\r\n", __func__, __LINE__);

    return ret;

}

void uvc_camera_device_power_off(E_USB_DEVICE_T device)
{
    for (uint8_t port = 1; port < UVC_PORT_MAX; port++)
    {
        bk_usbh_hub_port_register_connect_callback(port, device, NULL, NULL);
        bk_usbh_hub_port_register_disconnect_callback(port, device, NULL, NULL);
        bk_usbh_hub_multiple_devices_power_down(USB_HOST_MODE, port, device);
    }
}

bk_err_t bk_uvc_init(camera_handle_t *handle, uvc_config_t *config, bk_uvc_callback_t *cb)
{
    int ret = BK_OK;
    camera_config_t *output_handle = NULL;
    uvc_stream_handle_t *uvc_handle = s_uvc_stream_handle;
    camera_param_t *param = NULL;
    uvc_node_t *node = NULL;

    UVC_INIT_START();

    if (uvc_handle == NULL
        || config == NULL
        || config->port == 0
        || config->port > UVC_PORT_MAX)
    {
        LOGE("%s, %d task or param error.....\r\n", __func__, config->port);
        ret = BK_FAIL;
        goto out;
    }

    param = uvc_camera_stream_node_get_by_port_and_format(uvc_handle, config->port, config->img_format);
    if (param == NULL)
    {
        // create this new stream node
        node = uvc_camera_stream_node_init(uvc_handle);
        if (node == NULL)
        {
            ret = BK_UVC_NO_MEMORY;
            goto out;
        }

        param = node->param;
    }

    if (param->camera_state == UVC_STREAMING_STATE)
    {
        LOGW("%s, camera %d already start\r\n", __func__, config->port);
        ret = BK_FAIL;
        goto out;
    }
    else if (param->camera_state == UVC_CLOSED_STATE)
    {
        // in this situation, this port device may enum slow, or not insert, need wait some secconds
        uint8_t wait_times = 30;

        do {

            if (config->img_format & IMAGE_MJPEG)
            {
                ret = bk_usbh_hub_port_check_device(config->port, USB_UVC_DEVICE, (bk_usb_hub_port_info **)&param->port_info);
            }
            else
            {
                ret = bk_usbh_hub_port_check_device(config->port, USB_UVC_H26X_DEVICE, (bk_usb_hub_port_info **)&param->port_info);
            }
            if (ret == BK_OK)
            {
                // already connected
                param->camera_state = UVC_CONNECT_STATE;
                break;
            }

            wait_times--;
            if (param->camera_state == UVC_CLOSED_STATE)
                rtos_delay_milliseconds(200);
        } while (wait_times);

        if (param->camera_state == UVC_CLOSED_STATE)
        {
            ret = BK_FAIL;
            LOGW("%s, this port:%d not connect\n", __func__, config->port);
            goto out;
        }
    }
    else if (param->camera_state == UVC_CLOSING_STATE ||
                param->camera_state == UVC_DISCONNECT_STATE ||
                param->camera_state == UVC_CONFIGING_STATE)
    {
        ret = BK_FAIL;
        LOGW("%s, this port:%d state:%d cannot open\r\n", __func__, config->port, param->camera_state);
        goto out;
    }

    // step 2: init uvc process task
    ret = uvc_camera_process_task_init(uvc_handle, cb);
    if (ret != BK_OK)
    {
        goto out;
    }

    LOGD("%s, %d, port_id:%d\r\n", __func__, __LINE__, config->port);

    // ensure uvc must be connected
    if (param->camera_state == UVC_CONNECT_STATE)
    {
        output_handle = (camera_config_t *)os_malloc(sizeof(camera_config_t));
        if (output_handle == NULL)
        {
            LOGW("%s, output_handle malloc fail\n", __func__);
            goto out;
        }

        os_memset(output_handle, 0, sizeof(camera_config_t));
        os_memcpy(param->info, config, sizeof(uvc_config_t));
        rtos_clear_event_flags(&uvc_handle->handle, UVC_STREAM_START_BIT);

        //! TODO: 初始化不启动，只进行枚举，在调用start在启动
    #if 0 //! tuya-linch 
        ret = uvc_stream_task_send_msg(UVC_START_IND, (uint32_t)param);
        if (ret != BK_OK)
        {
            goto out;
        }

        rtos_wait_for_event_flags(&uvc_handle->handle, UVC_STREAM_START_BIT, true, true, BEKEN_WAIT_FOREVER);

        if (param->camera_state != UVC_STREAMING_STATE)
        {
            LOGW("%s, start fail....\n", __func__);
            ret = BK_FAIL;
        }
    #endif
    }
    else
    {
        LOGW("%s, this port:%d state:%d cannot open\r\n", __func__, config->port, param->camera_state);
        ret = BK_FAIL;
    }

out:

    if (ret != BK_OK)
    {
        UVC_INIT_END();
        if (output_handle)
        {
            os_free(output_handle);
            output_handle = NULL;
        }
    }
    else
    {
        output_handle->type = UVC_CAMERA;
        output_handle->fps = FPS30;
        output_handle->id = config->port;
        output_handle->image_format = config->img_format;
        output_handle->width = config->width;
        output_handle->height = config->height;
        output_handle->arg = (void *)uvc_handle;
        *handle = (camera_handle_t)output_handle;
    }

    return ret;
}

bk_err_t bk_uvc_deinit(camera_handle_t *handle)
{
    int ret = BK_OK;
    uvc_stream_handle_t *uvc_handle = s_uvc_stream_handle;

    if (*handle == NULL || uvc_handle == NULL)
    {
        LOGW("%s, %d already closed\r\n", __func__, __LINE__);
        return ret;
    }

    camera_config_t *config = (camera_config_t *)*handle;

    ret = bk_uvc_set_stop(handle);
    if (ret == BK_OK)
    {
        os_free(config);
        *handle = NULL;
    }

    LOGD("%s, %d complete\r\n", __func__, __LINE__);

    return ret;
}

bk_err_t bk_uvc_power_on(uint32_t format, uint32_t timeout)
{
    bk_err_t ret = BK_FAIL;
    UVC_POWER_ON_START();

    LOGD("%s, %d, %p\n", __func__, __LINE__, s_uvc_stream_handle);

    // step 1: stream task init
    ret = uvc_camera_stream_task_init(&s_uvc_stream_handle);
    if (ret != BK_OK)
    {
        UVC_POWER_ON_END();
        return ret;
    }

    if (format & IMAGE_MJPEG)
    {
        ret = uvc_camera_device_power_on(s_uvc_stream_handle, USB_UVC_DEVICE, timeout);
    }
    else
    {
        ret = uvc_camera_device_power_on(s_uvc_stream_handle, USB_UVC_H26X_DEVICE, timeout);
    }

    if (ret != BK_OK)
    {
        if (uvc_camera_stream_check_all_uvc_closed(s_uvc_stream_handle))
        {
            uvc_camera_stream_task_deinit(s_uvc_stream_handle);
            s_uvc_stream_handle = NULL;
        }
    }

    UVC_POWER_ON_END();

    return ret;
}

bk_err_t bk_uvc_power_off(void)
{
    uvc_stream_handle_t *handle = s_uvc_stream_handle;

    if (handle == NULL)
    {
        LOGE("%s, %d task have closed\r\n", __func__, __LINE__);
        return BK_OK;
    }

    if (uvc_camera_stream_check_all_uvc_closed(handle))
    {
        uvc_camera_process_task_deinit(handle);

        uvc_camera_device_power_off(USB_UVC_DEVICE);

        uvc_camera_device_power_off(USB_UVC_H26X_DEVICE);

        uvc_camera_stream_task_deinit(handle);
        s_uvc_stream_handle = NULL;
    }
    else
    {
        LOGE("%s, %d all camera not been closed\r\n", __func__, __LINE__);
    }

    return BK_OK;
}


bk_err_t bk_uvc_register_packet_cb(void *cb)
{
    uvc_stream_handle_t *uvc_handle = s_uvc_stream_handle;
    if (uvc_handle == NULL)
    {
        LOGE("%s, %d uvc stream task not init\r\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (uvc_handle->packet_cb)
    {
        LOGE("%s, %d packet cb have been register\r\n", __func__, __LINE__);
        return BK_FAIL;
    }

    uvc_handle->packet_cb = cb;

    return BK_OK;
}

bk_err_t bk_uvc_register_connect_state_cb(camera_state_cb_t cb)
{
    if (!uvc_connect_state_cb)
    {
        uvc_connect_state_cb = cb;
    }
    return BK_OK;
}

bk_err_t bk_uvc_register_separate_packet_callback(uvc_separate_config_t *cb)
{
    uvc_separate_packet_cb.id                     = cb->id;
    uvc_separate_packet_cb.uvc_eof_packet_cb      = cb->uvc_eof_packet_cb;
    uvc_separate_packet_cb.uvc_init_packet_cb     = cb->uvc_init_packet_cb;
    uvc_separate_packet_cb.uvc_separate_packet_cb = cb->uvc_separate_packet_cb;

    return BK_OK;
}

bk_usb_hub_port_info *bk_uvc_get_enum_info(uint8_t port, uint16_t format)
{
    bk_err_t ret = BK_FAIL;
    uint8_t count = 10;
    bk_usb_hub_port_info *port_info = NULL;
    if (s_uvc_stream_handle == NULL
        || port == 0
        || port >= UVC_PORT_MAX)
    {
        LOGE("%s, %d %p param error\r\n", __func__, port, s_uvc_stream_handle);
        return NULL;
    }

    camera_param_t *camera_param = uvc_camera_stream_node_get_by_port_and_format(s_uvc_stream_handle, port, format);
    if (camera_param)
    {
        return camera_param->port_info;
    }
    else
    {
        do
        {
            if (format & IMAGE_MJPEG)
            {
                ret = bk_usbh_hub_port_check_device(port, USB_UVC_DEVICE, (bk_usb_hub_port_info **)&port_info);
            }
            else
            {
                ret = bk_usbh_hub_port_check_device(port, USB_UVC_H26X_DEVICE, (bk_usb_hub_port_info **)&port_info);
            }

            if (ret != BK_OK)
            {
                rtos_delay_milliseconds(500);
                count--;
            }
            else
            {
                break;
            }
        }
        while (count);
    }

    if (count == 0)
    {
        return NULL;
    }

    return port_info;
}

bk_err_t bk_uvc_set_start(camera_handle_t *handle, uvc_config_t *config)
{
    int ret = BK_FAIL;
    camera_config_t *cam_config = (camera_config_t *)*handle;

    if (cam_config == NULL)
    {
        LOGW("%s, %d\r\n", __func__, __LINE__);
        return ret;
    }
    uvc_stream_handle_t *uvc_handle = (uvc_stream_handle_t *)cam_config->arg;

    LOGD("%s, %d\r\n", __func__, __LINE__);

    if (uvc_handle == NULL
        || config->port == 0
        || config->port >= UVC_PORT_MAX)
    {
        LOGE("%s, %d task or param error...\r\n", __func__, __LINE__);
        return ret;
    }

    camera_param_t *uvc_param = uvc_camera_stream_node_get_by_port_and_format(uvc_handle, config->port, config->img_format);
    if (uvc_param == NULL || uvc_param->camera_state != UVC_CONNECT_STATE)
    {
        LOGE("[%d]%s, %d  state:%d, not init or connect!\r\n",
             config->port, __func__, __LINE__, uvc_param->camera_state);
        return ret;
    }

    os_memcpy(uvc_param->info, config, sizeof(uvc_config_t));
    rtos_clear_event_flags(&uvc_handle->handle, UVC_STREAM_START_BIT);
    ret = uvc_stream_task_send_msg(UVC_START_IND, (uint32_t)uvc_param);
    if (ret != BK_OK)
    {
        LOGD("%s, %d\r\n", __func__, __LINE__);
        return ret;
    }

    rtos_wait_for_event_flags(&uvc_handle->handle, UVC_STREAM_START_BIT, true, true, BEKEN_WAIT_FOREVER);

    /* the stream task only reports a failed start through the state */
    if (uvc_param->camera_state != UVC_STREAMING_STATE)
    {
        ret = BK_FAIL;
    }

    return ret;
}

bk_err_t bk_uvc_set_stop(camera_handle_t *handle)
{
    int ret = BK_OK;
    camera_config_t *cam_config = (camera_config_t *)*handle;

    if (cam_config == NULL)
    {
        LOGW("%s, %d\r\n", __func__, __LINE__);
        return ret;
    }

    uvc_stream_handle_t *uvc_handle = (uvc_stream_handle_t *)cam_config->arg;

    LOGD("%s, %d\r\n", __func__, __LINE__);

    camera_param_t *uvc_param = uvc_camera_stream_node_get_by_port_and_format(uvc_handle, cam_config->id, cam_config->image_format);
    if (uvc_param)
    {
        if (uvc_param->camera_state == UVC_CLOSED_STATE)
        {
            LOGE("%s, %d camera have been closed\r\n", __func__, __LINE__);
            return ret;
        }
        else if (uvc_param->camera_state == UVC_CONNECT_STATE || uvc_param->camera_state == UVC_DISCONNECT_STATE)
        {
            LOGE("%s, %d camera not start\r\n", __func__, __LINE__);
        }
        else if (uvc_param->camera_state == UVC_STREAMING_STATE || uvc_param->camera_state == UVC_CONFIGING_STATE)
        {
            uvc_param->camera_state = UVC_CLOSING_STATE;
        }

        rtos_clear_event_flags(&uvc_handle->handle, UVC_CLOSE_BIT);

        if (uvc_stream_task_send_msg(UVC_STOP_IND, (uint32_t)uvc_param) != BK_OK)
        {
            ret = BK_FAIL;
        }
        else
        {
            rtos_wait_for_event_flags(&uvc_handle->handle, UVC_CLOSE_BIT, true, true, BEKEN_NEVER_TIMEOUT);
            uvc_param->camera_state = UVC_CONNECT_STATE;
        }
    }

    return ret;
}

bk_err_t bk_uvc_set_stream_state(uint32_t state)
{
    uvc_stream_handle_t *uvc_handle = s_uvc_stream_handle;
    uint32_t isr_context = platform_is_in_interrupt_context();

    if (uvc_handle == NULL || uvc_handle->pro_config == NULL)
    {
        LOGW("%s, not open...\n", __func__);
        return BK_FAIL;
    }

    if (!isr_context)
    {
        rtos_lock_mutex(&uvc_handle->mutex);
    }

    if (state == 0)
    {
        uvc_handle->pro_config->stream_state = UVC_STREAM_STATE_RESUME;
    }
    else if (state == 1)
    {
        uvc_handle->pro_config->stream_state = UVC_STREAM_STATE_SUSPEND;
    }
    else
    {
        LOGW("%s, state error:%d\n", __func__, state);
    }

    if (!isr_context)
    {
        rtos_unlock_mutex(&uvc_handle->mutex);
    }

    LOGV("%s, set stream state:%d\n", __func__, uvc_handle->pro_config->stream_state);
    return BK_OK;
}

/**
 * @brief 设置UVC帧丢弃模式
 * 
 * @param mode 丢帧模式
 *   0: 不丢帧 (30fps → 30fps)
 *   1: 每2帧保留1帧 (30fps → 15fps)
 *   2: 每3帧保留1帧 (30fps → 10fps)
 *   3: 每4帧保留1帧 (30fps → 7.5fps)
 *   N: 每(N+1)帧保留1帧
 * 
 * @return bk_err_t BK_OK成功
 * 
 * @note 该函数可在运行时动态调整，立即生效
 */
bk_err_t bk_uvc_set_frame_drop_mode(uint8_t mode)
{
    if (mode != g_uvc_frame_drop_mode)
    {
        g_uvc_frame_drop_mode = mode;
        g_uvc_frame_drop_counter = 0;  // 重置计数器
        
        if (mode > 0)
        {
            LOGI("UVC frame drop mode set to %d (keep 1 per %d frames)\r\n", mode, mode + 1);
        }
        else
        {
            LOGI("UVC frame drop mode disabled\r\n");
        }
    }
    
    return BK_OK;
}
