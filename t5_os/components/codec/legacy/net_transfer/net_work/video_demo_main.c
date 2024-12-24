#include <common/bk_include.h>
#include "video_demo_config.h"
#include <components/video_transfer.h>
#include "video_transfer_udp.h"
#include "video_transfer_tcp.h"
#include <components/video_transfer.h>
#include "cli.h"

#if (CONFIG_NET_WORK_VIDEO_TRANSFER == 1)
#include <os/str.h>
#include "bk_uart.h"
#include <os/mem.h>

#include <os/os.h>

#define VIDE_MODE_STATION        (0x1)
#define VIDE_MODE_SOFTAP         (0x2)
#define VIDE_MODE_P2P            (0x4)
//extern void app_demo_sta_start(char *oob_ssid, char *connect_key);
extern void app_demo_sta_start(char *oob_ssid, char *connect_key, int argc, char **argv);
extern void app_demo_sta_exit(void);

extern void app_demo_softap_start(char *oob_ssid, char *connect_key);
extern void app_demo_softap_exit(void);

extern void app_demo_p2p_start(char *oob_ssid, char *connect_key);
extern void app_demo_p2p_exit(void);

extern bk_err_t bk_dvp_camera_init(void *data);
extern bk_err_t _bk_uvc_camera_init(void *data);

media_camera_device_t device =
{
	.type = DVP_CAMERA,
	.mode = H264_MODE,
	.fmt = PIXEL_FMT_H264,
	.info.resolution.width = 640,
	.info.resolution.height = 480,
	.info.fps = FPS20,
}; /**< config of camera */

media_camera_device_t *get_camera_device(void)
{
	os_printf("get camera \r\n");
	return &device;
}

void media_trans_set_device_type(camera_type_t type, yuv_mode_t mode, pixel_format_t fmt)
{
	device.type = type;
	device.mode = mode;
	device.fmt = fmt;
}

void media_trans_set_device_stream(uint16_t width, uint16_t height, frame_fps_t fps)
{
	device.info.resolution.width = width;
	device.info.resolution.height = height;
	device.info.fps = fps;
}

void cmd_camera_type(int argc, char **argv)
{
	int i;

	for (i = 0; i < argc; i++)
	{
		if (os_strcmp(argv[i], "uvc_jpg") == 0)
		{
			device.type = UVC_CAMERA;
			device.fmt = PIXEL_FMT_JPEG;
		}

		if (os_strcmp(argv[i], "uvc_h264") == 0)
		{
			device.type = UVC_CAMERA;
			device.fmt = PIXEL_FMT_H264;
		}

		if (os_strcmp(argv[i], "dvp_jpg") == 0)
		{
			device.type = DVP_CAMERA;
			device.fmt = PIXEL_FMT_JPEG;
		}

		if (os_strcmp(argv[i], "dvp_h264") == 0)
		{
			device.type = DVP_CAMERA;
			device.fmt = PIXEL_FMT_H264;
		}

		if (device.type != UNKNOW_CAMERA)
			break;
	}

	if (device.type == UNKNOW_CAMERA)
		os_printf("error : unknown camera! \r\n");
}

void cmd_camera_ppi(int argc, char **argv)
{
	int i;
	media_ppi_t ppi = PPI_DEFAULT;

	for (i = 0; i < argc; i++)
	{
		ppi = get_string_to_ppi(argv[i]);
		if (ppi != PPI_DEFAULT)
			break;
	}

	if (ppi == PPI_DEFAULT)
	{
		return;
	}

	device.info.resolution.width = ppi >> 16;
	device.info.resolution.height = ppi & 0xFFFF;
}

void cmd_camera_fps(int argc, char **argv)
{
	int i;
	frame_fps_t fps = FPS0;

	for (i = 0; i < argc; i++)
	{
		fps = get_string_to_fps(argv[i]);
		if (fps != FPS0)
			break;
	}

	if (fps == FPS0)
	{
		return;
	}

	device.info.fps = fps;
}

void cmd_video_transfer(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = NULL;

	if (argc > 3)
	{
		cmd_camera_type(argc - 2, argv + 2);

		cmd_camera_fps(argc - 2, argv + 2);

		cmd_camera_ppi(argc - 2, argv + 2);
	}

	if (argc == 1)
	{
		goto __usage;
	}
	else if (os_strcmp(argv[1], "-h") == 0)
	{
		goto __usage;
	}
	else if (os_strcmp(argv[1], "-stop") == 0)
	{
		os_printf("not implement!\n");
	}
	else if (os_strcmp(argv[1], "udp") == 0)
	{
#if APP_DEMO_CFG_USE_UDP
		app_demo_udp_init();
#endif
	}
	else if (os_strcmp(argv[1], "tcp") == 0)
	{
#if APP_DEMO_CFG_USE_UDP
		app_demo_tcp_init();
#endif
	}
	else if (os_strcmp(argv[1], "start") == 0)
	{
		bk_video_transfer_start();
	}
	else if (os_strcmp(argv[1], "stop") == 0)
	{
		bk_video_transfer_stop();
	}
	else
	{
		goto __usage;
	}

	msg = CLI_CMD_RSP_SUCCEED;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return;

__usage:
	// video_transfer_usage();
	msg = CLI_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

#endif

