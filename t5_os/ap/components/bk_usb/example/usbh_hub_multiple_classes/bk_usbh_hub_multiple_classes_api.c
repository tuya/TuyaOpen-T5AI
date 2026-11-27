#include "bk_usbh_hub_multiple_classes_api.h"

typedef struct
{
	uint8_t usbh_hub_power_flag;

	usbh_hub_connect_disconnect_cb_t usbh_hub_connect_cb[CONFIG_USBHOST_HUB_MAX_EHPORTS + 1][CONFIG_USBHOST_HUB_PORT_SUPPORT_MAX_DEVICE];//port0 for debug
	usbh_hub_connect_disconnect_cb_t usbh_hub_disconnect_cb[CONFIG_USBHOST_HUB_MAX_EHPORTS + 1][CONFIG_USBHOST_HUB_PORT_SUPPORT_MAX_DEVICE];//port0 for debug
	void *usbh_hub_connect_cb_arg[CONFIG_USBHOST_HUB_MAX_EHPORTS + 1][CONFIG_USBHOST_HUB_PORT_SUPPORT_MAX_DEVICE];//port0 for debug
	void *usbh_hub_disconnect_cb_arg[CONFIG_USBHOST_HUB_MAX_EHPORTS + 1][CONFIG_USBHOST_HUB_PORT_SUPPORT_MAX_DEVICE];//port0 for debug
	uint32_t usbh_hub_connect_class_device_flag[CONFIG_USBHOST_HUB_MAX_EHPORTS + 1];//uvc: 0x1 << USB_UVC_DEVICE

	uint32_t usbh_hub_class_device_vote_power_flag[CONFIG_USBHOST_HUB_MAX_EHPORTS + 1];
	bk_usb_hub_port_info usbh_hub_port_info[CONFIG_USBHOST_HUB_MAX_EHPORTS + 1][CONFIG_USBHOST_HUB_PORT_SUPPORT_MAX_DEVICE];
} bk_usbh_hub_class_dev_info;

static bk_usbh_hub_class_dev_info s_usb_hub_class_dev;

/*

*/
bk_err_t bk_usbh_hub_port_register_connect_callback(E_USB_HUB_PORT_INDEX port_index,
																				E_USB_DEVICE_T class_dev_index,
																				void *connect_cb,
																				void *arg)
{
	uint32_t ret = BK_OK;
	bk_usbh_hub_class_dev_info *usb_hub_class_dev = &s_usb_hub_class_dev;

	if(port_index > CONFIG_USBHOST_HUB_MAX_EHPORTS) {
		USB_HUB_MD_LOGE("%s PORT_INDEX IS ERROR\r\n", __func__);
		return BK_FAIL;
	}

	if(class_dev_index > USB_DEVICE_MAX) {
		USB_HUB_MD_LOGE("%s CLASS_DEV_INDEX IS ERROR\r\n", __func__);
		return BK_FAIL;
	}

	if(connect_cb) {
		ret = BK_OK;
	} else {
		ret = BK_ERR_USB_OPERATION_NULL_POINTER;
	}
	usb_hub_class_dev->usbh_hub_connect_cb[port_index][class_dev_index] = (usbh_hub_connect_disconnect_cb_t)connect_cb;
	usb_hub_class_dev->usbh_hub_connect_cb_arg[port_index][class_dev_index] = arg;

	return ret;
}

/*

*/
bk_err_t bk_usbh_hub_port_register_disconnect_callback(E_USB_HUB_PORT_INDEX port_index,
																				E_USB_DEVICE_T class_dev_index,
																				void *disconnect_cb,
																				void *arg)
{
	uint32_t ret = BK_OK;
	bk_usbh_hub_class_dev_info *usb_hub_class_dev = &s_usb_hub_class_dev;

	if(port_index > CONFIG_USBHOST_HUB_MAX_EHPORTS) {
		USB_HUB_MD_LOGE("%s PORT_INDEX IS ERROR\r\n", __func__);
		return BK_FAIL;
	}

	if(class_dev_index > USB_DEVICE_MAX) {
		USB_HUB_MD_LOGE("%s CLASS_DEV_INDEX IS ERROR\r\n", __func__);
		return BK_FAIL;
	}

	if(disconnect_cb) {
		ret = BK_OK;
	} else {
		ret = BK_ERR_USB_OPERATION_NULL_POINTER;
	}
	usb_hub_class_dev->usbh_hub_disconnect_cb[port_index][class_dev_index] = (usbh_hub_connect_disconnect_cb_t)disconnect_cb;
	usb_hub_class_dev->usbh_hub_disconnect_cb_arg[port_index][class_dev_index] = arg;

	return ret;
}

static bk_err_t bk_usbh_hub_port_power_ops(uint32_t gpio_id, bool ops)
{
	if(ops) {
		gpio_dev_unmap(gpio_id);
		bk_gpio_set_capacity(gpio_id, 0);
		BK_LOG_ON_ERR(bk_gpio_disable_input(gpio_id));
		BK_LOG_ON_ERR(bk_gpio_enable_output(gpio_id));
		bk_gpio_set_output_high(gpio_id);
	} else {
		bk_gpio_set_output_low(gpio_id);
	}

	return BK_OK;
}

static uint32_t bk_usbh_hub_get_port_vbus_control_gpio_id(E_USB_HUB_PORT_INDEX port_index)
{
	uint32_t gpio_id;
	switch(port_index){
		case USB_HUB_PORT_1:
		case USB_HUB_PORT_2:
		case USB_HUB_PORT_3:
		case USB_HUB_PORT_4:
		case USB_HUB_PORT_5:
		case USB_HUB_PORT_6:
		case USB_HUB_PORT_7:
		case USB_HUB_PORT_8:
			gpio_id = CONFIG_USBHOST_HUB_PORT_1_VBAT_CONTROL_GPIO_ID;
			break;
		default:
			break;
	}
	return gpio_id;
}

static void bk_usbh_hub_port_sw_init()
{
	bk_usbh_hub_class_dev_info *usb_hub_class_dev = &s_usb_hub_class_dev;

	for(E_USB_HUB_PORT_INDEX port_index = USB_HUB_PORT_1; port_index <= CONFIG_USBHOST_HUB_MAX_EHPORTS; port_index++)
	{
		for(E_USB_DEVICE_T dev_index = USB_UVC_DEVICE; dev_index < CONFIG_USBHOST_HUB_PORT_SUPPORT_MAX_DEVICE; dev_index++)
		{
			usb_hub_class_dev->usbh_hub_port_info[port_index][dev_index].port_index = 0;
			usb_hub_class_dev->usbh_hub_port_info[port_index][dev_index].device_index = USB_DEVICE_MAX;
			usb_hub_class_dev->usbh_hub_port_info[port_index][dev_index].hport = NULL;
			usb_hub_class_dev->usbh_hub_port_info[port_index][dev_index].usb_device = NULL;
			usb_hub_class_dev->usbh_hub_port_info[port_index][dev_index].interface_num = 0;
			usb_hub_class_dev->usbh_hub_port_info[port_index][dev_index].usb_device_param = NULL;
			usb_hub_class_dev->usbh_hub_port_info[port_index][dev_index].usb_device_param_config = NULL;
		}
		usb_hub_class_dev->usbh_hub_connect_class_device_flag[port_index] = 0;
		usb_hub_class_dev->usbh_hub_class_device_vote_power_flag[port_index] = 0;
	}
}

static void bk_usbh_hub_port_sw_deinit()
{
	bk_usbh_hub_class_dev_info *usb_hub_class_dev = &s_usb_hub_class_dev;
	void *usb_device_param = NULL;
	void *usb_device_param_config = NULL;

	for(E_USB_HUB_PORT_INDEX port_index = USB_HUB_PORT_1; port_index <= CONFIG_USBHOST_HUB_MAX_EHPORTS; port_index++)
	{
		for(E_USB_DEVICE_T dev_index = USB_UVC_DEVICE; dev_index < CONFIG_USBHOST_HUB_PORT_SUPPORT_MAX_DEVICE; dev_index++)
		{
			usb_hub_class_dev->usbh_hub_connect_cb[port_index][dev_index] = NULL;
			usb_hub_class_dev->usbh_hub_disconnect_cb[port_index][dev_index] = NULL;
			if(usb_hub_class_dev->usbh_hub_port_info[port_index][dev_index].port_index == 0){break;}

			usb_device_param = usb_hub_class_dev->usbh_hub_port_info[port_index][dev_index].usb_device_param;
			if(usb_device_param) {
				os_free(usb_device_param);
			}
			usb_hub_class_dev->usbh_hub_port_info[port_index][dev_index].usb_device_param = NULL;

			usb_device_param_config = usb_hub_class_dev->usbh_hub_port_info[port_index][dev_index].usb_device_param_config;
			if(usb_device_param_config) {
				os_free(usb_device_param_config);
			}

			usb_hub_class_dev->usbh_hub_port_info[port_index][dev_index].usb_device_param_config = NULL;
		}
		usb_hub_class_dev->usbh_hub_connect_class_device_flag[port_index] = 0;
		usb_hub_class_dev->usbh_hub_class_device_vote_power_flag[port_index] = 0;
	}
}

bk_err_t bk_usbh_hub_multiple_devices_power_on(E_USB_MODE mode, E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T class_dev_index)
{
	uint32_t ret = BK_OK;
	bk_usbh_hub_class_dev_info *usb_hub_class_dev = &s_usb_hub_class_dev;
	uint32_t gpio_id;

	if(port_index > CONFIG_USBHOST_HUB_MAX_EHPORTS) {
		USB_HUB_MD_LOGE("%s PORT_INDEX IS ERROR\r\n", __func__);
		return BK_FAIL;
	}

	if(class_dev_index > USB_DEVICE_MAX) {
		USB_HUB_MD_LOGE("%s CLASS_DEV_INDEX IS ERROR\r\n", __func__);
		return BK_FAIL;
	}

	bk_usb_driver_task_lock_mutex();

	if(!usb_hub_class_dev->usbh_hub_class_device_vote_power_flag[port_index]) {
		gpio_id = bk_usbh_hub_get_port_vbus_control_gpio_id(port_index);
		USB_HUB_MD_LOGD("bk_usbh_hub_multiple_devices_power_on: %d, %d\r\n", port_index, gpio_id);
		bk_usbh_hub_port_power_ops(gpio_id, 1);
	}

	if(usb_hub_class_dev->usbh_hub_power_flag) {
		ret = BK_OK;
	} else {
		USB_HUB_MD_LOGD("bk_usb_open: %d, %d\r\n", port_index, CONFIG_USB_VBAT_CONTROL_GPIO_ID);
		bk_usb_power_ops(CONFIG_USB_VBAT_CONTROL_GPIO_ID, 1);
		ret = bk_usb_open(mode);
		if(ret == BK_OK) {
			bk_usbh_hub_port_sw_init();
			usb_hub_class_dev->usbh_hub_power_flag = 1;
		}
	}

	usb_hub_class_dev->usbh_hub_class_device_vote_power_flag[port_index] |= (0x1 << class_dev_index);
	bk_usb_driver_task_unlock_mutex();
	return ret;

}

bk_err_t bk_usbh_hub_multiple_devices_power_down(E_USB_MODE mode, E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T class_dev_index)
{
	uint32_t ret = BK_OK;
	bk_usbh_hub_class_dev_info *usb_hub_class_dev = &s_usb_hub_class_dev;
	uint32_t gpio_id;
	uint8_t port_i = USB_HUB_PORT_1;
	uint8_t need_power_down_flag = 0;

	if(port_index > CONFIG_USBHOST_HUB_MAX_EHPORTS) {
		USB_HUB_MD_LOGE("%s PORT_INDEX IS ERROR\r\n", __func__);
		return BK_FAIL;
	}

	if(class_dev_index > USB_DEVICE_MAX) {
		USB_HUB_MD_LOGE("%s CLASS_DEV_INDEX IS ERROR\r\n", __func__);
		return BK_FAIL;
	}

	bk_usb_driver_task_lock_mutex();

	usb_hub_class_dev->usbh_hub_class_device_vote_power_flag[port_index] &= ~(0x1 << class_dev_index);
	if(!usb_hub_class_dev->usbh_hub_class_device_vote_power_flag[port_index]) {
		gpio_id = bk_usbh_hub_get_port_vbus_control_gpio_id(port_index);
		bk_usbh_hub_port_power_ops(gpio_id, 0);
	}

	if(usb_hub_class_dev->usbh_hub_power_flag) {
		port_i = USB_HUB_PORT_1;
		while(!(port_i > CONFIG_USBHOST_HUB_MAX_EHPORTS)){
			USB_HUB_MD_LOGV("%s usbh_hub_class_device_vote_power_flag[%d]:%d\r\n", __func__, port_i, usb_hub_class_dev->usbh_hub_class_device_vote_power_flag[port_i]);
			if(usb_hub_class_dev->usbh_hub_class_device_vote_power_flag[port_i]){
				need_power_down_flag = 0;
				break;
			} else {
				need_power_down_flag = 1;
			}
			port_i++;
		}
		USB_HUB_MD_LOGV("%s need_power_down_flag:%d\r\n", __func__, need_power_down_flag);
		if(need_power_down_flag) {
			ret = bk_usb_close();
			if(ret == BK_OK) {
				usb_hub_class_dev->usbh_hub_power_flag = 0;
				bk_usbh_hub_port_sw_deinit();
			}
			bk_usb_power_ops(CONFIG_USB_VBAT_CONTROL_GPIO_ID, 0);
		}
	}
	bk_usb_driver_task_unlock_mutex();
	return ret;
}

static void bk_usbh_hub_uac_parse_param(struct usbh_hubport *hport, uint8_t interface_num, void *audio_class)
{
	bk_usbh_hub_class_dev_info *usb_hub_class_dev = &s_usb_hub_class_dev;
	struct usbh_audio *uac_device = (struct usbh_audio *)audio_class;

	bk_usb_hub_port_info *usbh_hub_port_mic_info = &usb_hub_class_dev->usbh_hub_port_info[hport->port][USB_UAC_MIC_DEVICE];
	bk_usb_hub_port_info *usbh_hub_port_spk_info = &usb_hub_class_dev->usbh_hub_port_info[hport->port][USB_UAC_SPEAKER_DEVICE];

	uint32_t malloc_fail_flag = 0;
	bk_uac_device_brief_info_t *uac_mic_device_info = NULL;
	bk_uac_mic_config_t *mic_config = NULL;

	bk_uac_device_brief_info_t *uac_spk_device_info = NULL;
	bk_uac_spk_config_t *spk_config = NULL;

	bk_uac_device_brief_info_t uac_device_info;

	do{
		uac_mic_device_info = os_malloc(sizeof(bk_uac_device_brief_info_t));
		if(!uac_mic_device_info) {
			malloc_fail_flag |= (0x1 << 0);
			break;
		} else {
			os_memset((void *)uac_mic_device_info, 0x0, sizeof(bk_uac_device_brief_info_t));
		}

		mic_config = os_malloc(sizeof(bk_uac_mic_config_t));
		if(!mic_config) {
			malloc_fail_flag |= (0x1 << 1);
			break;
		} else {
			os_memset((void *)mic_config, 0x0, sizeof(bk_uac_mic_config_t));
		}

		uac_spk_device_info = os_malloc(sizeof(bk_uac_device_brief_info_t));
		if(!uac_spk_device_info) {
			malloc_fail_flag |= (0x1 << 2);
			break;
		} else {
			os_memset((void *)uac_spk_device_info, 0x0, sizeof(bk_uac_device_brief_info_t));
		}

		spk_config = os_malloc(sizeof(bk_uac_spk_config_t));
		if(!spk_config) {
			malloc_fail_flag |= (0x1 << 3);
			break;
		} else {
			os_memset((void *)spk_config, 0x0, sizeof(bk_uac_spk_config_t));
		}

	} while(0);

	if(malloc_fail_flag) {

		if(uac_mic_device_info) {
			os_free(uac_mic_device_info);
			uac_mic_device_info = NULL;
		}

		if(mic_config) {
			os_free(mic_config);
			mic_config = NULL;
		}

		if(uac_spk_device_info) {
			os_free(uac_spk_device_info);
			uac_spk_device_info = NULL;
		}

		if(spk_config) {
			os_free(spk_config);
			spk_config = NULL;
		}

		USB_HUB_MD_LOGE("[=]%s malloc Fail FLAG:0x%x\r\n", __func__, malloc_fail_flag);
		usb_hub_class_dev->usbh_hub_connect_class_device_flag[hport->port] &= ~(0x1 << USB_UAC_MIC_DEVICE);
		usb_hub_class_dev->usbh_hub_connect_class_device_flag[hport->port] &= ~(0x1 << USB_UAC_SPEAKER_DEVICE);
		return;
	}

	usbh_hub_port_mic_info->port_index = hport->port;
	usbh_hub_port_spk_info->port_index = hport->port;

	usbh_hub_port_mic_info->device_index = USB_UAC_MIC_DEVICE;
	usbh_hub_port_spk_info->device_index = USB_UAC_SPEAKER_DEVICE;

	usbh_hub_port_mic_info->hport = hport;
	usbh_hub_port_spk_info->hport = hport;

	usbh_hub_port_mic_info->usb_device = uac_device;
	usbh_hub_port_spk_info->usb_device = uac_device;

	usbh_hub_port_mic_info->interface_num = interface_num;
	usbh_hub_port_spk_info->interface_num = interface_num;

	uac_device_info.vendor_id = hport->device_desc.idVendor;
	uac_device_info.product_id = hport->device_desc.idProduct;

	const char *mic_name = "mic";
	const char *spk_name = "speaker";
	uint8_t intf = 0xff;
	uint8_t mic_intf = 0xff;
	uint8_t spk_intf = 0xff;
	struct audio_ep_descriptor *audio_ep_desc;
    for (size_t i = 0; i < uac_device->module_num; i++) {
		if (strcmp(mic_name, uac_device->module[i].name) == 0) {
			intf = uac_device->module[i].data_intf;
			uac_device_info.mic_format_tag = uac_device->module[i].altsetting[0].format_type;
			uac_device_info.mic_samples_frequence_num = uac_device->module[i].altsetting[0].sampfreq_num;
			uac_device_info.mic_samples_frequence = &(uac_device->module[i].altsetting[0].sampfreq[0]);
			audio_ep_desc = (struct audio_ep_descriptor *)&uac_device->hport->config.intf[intf].altsetting[1].ep[0].ep_desc;
			if(audio_ep_desc->bEndpointAddress & 0x80) {
				uac_device_info.mic_ep_desc = (struct audio_ep_descriptor *)audio_ep_desc;
				mic_intf = intf;
			} else {
				uac_device_info.spk_ep_desc = (struct audio_ep_descriptor *)audio_ep_desc;
				spk_intf = intf;
			}
			usb_hub_class_dev->usbh_hub_connect_class_device_flag[hport->port] |= (0x1 << USB_UAC_MIC_DEVICE);
		}
		if (strcmp(spk_name, uac_device->module[i].name) == 0) {
			intf = uac_device->module[i].data_intf;
			uac_device_info.spk_format_tag = uac_device->module[i].altsetting[0].format_type;
			uac_device_info.spk_samples_frequence_num = uac_device->module[i].altsetting[0].sampfreq_num;
			uac_device_info.spk_samples_frequence = &(uac_device->module[i].altsetting[0].sampfreq[0]);
			audio_ep_desc = (struct audio_ep_descriptor *)&uac_device->hport->config.intf[intf].altsetting[1].ep[0].ep_desc;
			if(audio_ep_desc->bEndpointAddress & 0x80) {
				uac_device_info.mic_ep_desc = (struct audio_ep_descriptor *)audio_ep_desc;
				mic_intf = intf;
			} else {
				uac_device_info.spk_ep_desc = (struct audio_ep_descriptor *)audio_ep_desc;
				spk_intf = intf;
			}
			usb_hub_class_dev->usbh_hub_connect_class_device_flag[hport->port] |= (0x1 << USB_UAC_SPEAKER_DEVICE);
		}
	}
	for (size_t i = 0; i < uac_device->module_num; i++) {
        if (strcmp(mic_name, uac_device->module[i].name) == 0) {
			uac_device->module[i].data_intf = mic_intf;
			mic_config->mic_format_tag = uac_device_info.mic_format_tag;
			mic_config->mic_samples_frequence = uac_device_info.mic_samples_frequence[0];
			mic_config->mic_ep_desc = uac_device_info.mic_ep_desc;
		}
		if (strcmp(spk_name, uac_device->module[i].name) == 0) {
			uac_device->module[i].data_intf = spk_intf;
			spk_config->spk_format_tag = uac_device_info.spk_format_tag;
			spk_config->spk_samples_frequence = uac_device_info.spk_samples_frequence[0];
			spk_config->spk_ep_desc = uac_device_info.spk_ep_desc;
		}
	}

	os_memcpy((void *)uac_mic_device_info, (void *)&uac_device_info, sizeof(bk_uac_device_brief_info_t));
	usbh_hub_port_mic_info->usb_device_param = (void *)uac_mic_device_info;
	usbh_hub_port_mic_info->usb_device_param_config = (void *)mic_config;

	os_memcpy((void *)uac_spk_device_info, (void *)&uac_device_info, sizeof(bk_uac_device_brief_info_t));
	usbh_hub_port_spk_info->usb_device_param = (void *)uac_spk_device_info;
	usbh_hub_port_spk_info->usb_device_param_config = (void *)spk_config;

}

static uint32_t bk_usbh_hub_uvc_parse_param(struct usbh_hubport *hport, uint8_t interface_num, void *video_class)
{
	USB_HUB_MD_LOGV("[+]%s\r\n", __func__);

	bk_usbh_hub_class_dev_info *usb_hub_class_dev = &s_usb_hub_class_dev;
	bk_usb_hub_port_info *usbh_hub_port_info = NULL;
	struct usbh_video *uvc_device = (struct usbh_video *)video_class;

	uint32_t device_index = 0;
	uint8_t video_format_type = 0;
	uint32_t k = 0;

	//! TODO: 需要优化，根据应用选择的格式和实际格式进行判断
	for (; k < uvc_device->num_of_formats; k++)
	{
		video_format_type = uvc_device->format[k].format_type;
		if (video_format_type != USBH_VIDEO_FORMAT_UNCOMPRESSED)
		{
			if (video_format_type == USBH_VIDEO_FORMAT_MJPEG) {
				device_index = USB_UVC_DEVICE;
				break;
			}
			else if (video_format_type == USBH_VIDEO_FORMAT_H264 || video_format_type == USBH_VIDEO_FORMAT_H265)
			{
				device_index = USB_UVC_H26X_DEVICE;
				break;
			}
		}

	}
	uint32_t malloc_fail_flag = 0;
	bk_uvc_device_brief_info_t *uvc_device_info = NULL;
	bk_uvc_config_t *uvc_device_config = NULL;

	do{
		uvc_device_info = os_malloc(sizeof(bk_uvc_device_brief_info_t));
		if(!uvc_device_info) {
			malloc_fail_flag |= (0x1 << 0);
			break;
		} else {
			os_memset((void *)uvc_device_info, 0x0, sizeof(bk_uvc_device_brief_info_t));
		}

		uvc_device_config = os_malloc(sizeof(bk_uvc_config_t));
		if(!uvc_device_config) {
			malloc_fail_flag |= (0x1 << 1);
			break;
		} else {
			os_memset((void *)uvc_device_config, 0x0, sizeof(bk_uvc_config_t));
		}

	} while(0);

	if(malloc_fail_flag) {

		if(uvc_device_info) {
			os_free(uvc_device_info);
			uvc_device_info = NULL;
		}

		if(uvc_device_config) {
			os_free(uvc_device_config);
			uvc_device_config = NULL;
		}

		USB_HUB_MD_LOGE("[=]%s malloc Fail FLAG:0x%x\r\n", __func__, malloc_fail_flag);
		usb_hub_class_dev->usbh_hub_connect_class_device_flag[hport->port] &= ~(0x1 << device_index);
		return device_index;
	}

	usbh_hub_port_info = &usb_hub_class_dev->usbh_hub_port_info[hport->port][device_index];

	usbh_hub_port_info->port_index    = hport->port;
	usbh_hub_port_info->device_index  = device_index;
	usbh_hub_port_info->hport         = hport;
	usbh_hub_port_info->usb_device    = video_class;
	usbh_hub_port_info->interface_num = interface_num;

	uvc_device_info->vendor_id    = hport->device_desc.idVendor;
	uvc_device_info->product_id   = hport->device_desc.idProduct;
	uvc_device_info->device_bcd   = hport->device_desc.bcdDevice;
	uvc_device_info->support_devs = 0;

	USB_HUB_MD_LOGV("[=]%s begin format %d\r\n", __func__, uvc_device->num_of_formats);
	for(int index = 0; index < uvc_device->num_of_formats; index++)
	{
		switch(uvc_device->format[index].format_type)
		{
			case USBH_VIDEO_FORMAT_UNCOMPRESSED:
				uvc_device_info->all_frame.yuv_frame = (bk_uvc_frame *)uvc_device->format[index].frame;
				uvc_device_info->all_frame.yuv_frame_num = uvc_device->format[index].num_of_frames;
				uvc_device_info->format_index.yuv_format_index = index + 1;
				break;
			case USBH_VIDEO_FORMAT_MJPEG:
				uvc_device_info->all_frame.mjpeg_frame = (bk_uvc_frame *)uvc_device->format[index].frame;
				uvc_device_info->all_frame.mjpeg_frame_num = uvc_device->format[index].num_of_frames;
				uvc_device_info->format_index.mjpeg_format_index = index + 1;
				break;
			case USBH_VIDEO_FORMAT_H264:
				uvc_device_info->all_frame.h264_frame = (bk_uvc_frame *)uvc_device->format[index].frame;
				uvc_device_info->all_frame.h264_frame_num = uvc_device->format[index].num_of_frames;
				uvc_device_info->format_index.h264_format_index = index + 1;
				break;
			case USBH_VIDEO_FORMAT_H265:
				uvc_device_info->all_frame.h265_frame = (bk_uvc_frame *)uvc_device->format[index].frame;
				uvc_device_info->all_frame.h265_frame_num = uvc_device->format[index].num_of_frames;
				uvc_device_info->format_index.h265_format_index = index + 1;
				break;
			default:
				break;
		}
	}
	//! TODO: 这里在干嘛？？？端点数量不累计吗？
	for (uint8_t i = 0; i < uvc_device->num_of_intf_altsettings; i++) {
		uvc_device_info->endpoints_num = uvc_device->hport->config.intf[uvc_device->data_intf].altsetting[i].intf_desc.bNumEndpoints;
		uvc_device_info->ep_desc       = (struct s_bk_usb_endpoint_descriptor *)&uvc_device->hport->config.intf[uvc_device->data_intf].altsetting[i].ep[0].ep_desc;
	}

	usbh_hub_port_info->usb_device_param        = (void *)uvc_device_info;
	usbh_hub_port_info->usb_device_param_config = (void *)uvc_device_config;

	usb_hub_class_dev->usbh_hub_connect_class_device_flag[hport->port] |= (0x1 << device_index);
	USB_HUB_MD_LOGV("[-]%s\r\n", __func__);
	return device_index;
}

#if (CONFIG_USB_CDC)
static void bk_usbh_hub_cdc_parse_param(struct usbh_hubport *hport, uint8_t interface_num, void *cdc_class)
{
	USB_HUB_MD_LOGD("[+]%s, %d\r\n", __func__, hport->port);
	bk_usbh_hub_class_dev_info *usb_hub_class_dev = &s_usb_hub_class_dev;
	bk_usb_hub_port_info *usbh_hub_port_info = NULL; __maybe_unused_var(usbh_hub_port_info);
	struct usbh_cdc_acm *cdc_device = (struct usbh_cdc_acm *)cdc_class; __maybe_unused_var(cdc_device);

	usbh_hub_port_info = &usb_hub_class_dev->usbh_hub_port_info[hport->port][USB_CDC_DEVICE];

	usbh_hub_port_info->hport         = hport;
	usbh_hub_port_info->port_index    = hport->port;
	usbh_hub_port_info->usb_device    = cdc_class;
	usbh_hub_port_info->interface_num = interface_num;
	usbh_hub_port_info->device_index  = USB_CDC_DEVICE;

	usbh_hub_port_info->usb_device_param		= NULL;
	usbh_hub_port_info->usb_device_param_config = NULL;
	usb_hub_class_dev->usbh_hub_connect_class_device_flag[hport->port] |= (0x1 << USB_CDC_DEVICE);
	USB_HUB_MD_LOGV("[-]%s, %d %x\r\n", __func__, hport->port, usb_hub_class_dev->usbh_hub_connect_class_device_flag[hport->port]);
}
#endif

void bk_usbh_hub_class_connect_notification(struct usbh_hubport *hport, uint8_t intf, uint32_t class)
{
	bk_usbh_hub_class_dev_info *usb_hub_class_dev = &s_usb_hub_class_dev;
	bk_usb_hub_port_info *usbh_hub_port_info = NULL;
	void *usb_device = NULL;
	void *connect_cb_arg = NULL;
	switch (class)
	{
#if CONFIG_USBH_UAC
		case USB_DEVICE_CLASS_AUDIO:
			usb_device = usbh_find_class_instance(hport->config.intf[intf].devname);
			if(usb_device) {
				bk_usbh_hub_uac_parse_param(hport, intf, usb_device);
			}

			usbh_hub_port_info = &usb_hub_class_dev->usbh_hub_port_info[hport->port][USB_UAC_MIC_DEVICE];
			USB_HUB_MD_LOGD("%s connect_device_flag:0x%x\r\n", __func__, usb_hub_class_dev->usbh_hub_connect_class_device_flag[hport->port]);
			if((usb_hub_class_dev->usbh_hub_connect_class_device_flag[hport->port] & (0x1 << USB_UAC_MIC_DEVICE))
				&& usb_hub_class_dev->usbh_hub_connect_cb[hport->port][USB_UAC_MIC_DEVICE])
			{
				connect_cb_arg = usb_hub_class_dev->usbh_hub_connect_cb_arg[hport->port][USB_UAC_MIC_DEVICE];
				usb_hub_class_dev->usbh_hub_connect_cb[hport->port][USB_UAC_MIC_DEVICE](usbh_hub_port_info, connect_cb_arg);
			}

			usbh_hub_port_info = &usb_hub_class_dev->usbh_hub_port_info[hport->port][USB_UAC_SPEAKER_DEVICE];
			if((usb_hub_class_dev->usbh_hub_connect_class_device_flag[hport->port] & (0x1 << USB_UAC_SPEAKER_DEVICE))
				&& usb_hub_class_dev->usbh_hub_connect_cb[hport->port][USB_UAC_SPEAKER_DEVICE])
			{
				connect_cb_arg = usb_hub_class_dev->usbh_hub_connect_cb_arg[hport->port][USB_UAC_SPEAKER_DEVICE];
				usb_hub_class_dev->usbh_hub_connect_cb[hport->port][USB_UAC_SPEAKER_DEVICE](usbh_hub_port_info, connect_cb_arg);
			}
			break;
#endif
#if CONFIG_USBH_UVC
		case USB_DEVICE_CLASS_VIDEO:
			{
				uint32_t device_index = 0;
				usb_device = usbh_find_class_instance(hport->config.intf[intf].devname);
				if(usb_device) {
					device_index = bk_usbh_hub_uvc_parse_param(hport, intf, usb_device);
				}
				USB_HUB_MD_LOGD("%s connect_device_flag:0x%x\r\n", __func__, usb_hub_class_dev->usbh_hub_connect_class_device_flag[hport->port]);

				usbh_hub_port_info = &usb_hub_class_dev->usbh_hub_port_info[hport->port][device_index];

				if((usb_hub_class_dev->usbh_hub_connect_class_device_flag[hport->port] & (0x1 << device_index))
					&& usb_hub_class_dev->usbh_hub_connect_cb[hport->port][device_index])
				{
					connect_cb_arg = usb_hub_class_dev->usbh_hub_connect_cb_arg[hport->port][device_index];
					USB_HUB_MD_LOGV("%s port_dev_info:0x%x port_index:%d dev_index:%d\r\n", __func__, usbh_hub_port_info, hport->port, device_index);
					usb_hub_class_dev->usbh_hub_connect_cb[hport->port][device_index](usbh_hub_port_info, connect_cb_arg);
				}
			}
			break;
#endif

#if CONFIG_USB_CDC
		case USB_DEVICE_CLASS_CDC:
			{
				usb_device = usbh_find_class_instance(hport->config.intf[intf].devname);
				if (usb_device) {
					bk_usbh_hub_cdc_parse_param(hport, intf, usb_device);
				}
				usbh_hub_port_info = &usb_hub_class_dev->usbh_hub_port_info[hport->port][USB_CDC_DEVICE];
				USB_HUB_MD_LOGD("%s, %s, connect_device_flag : 0x%x\r\n", __func__, hport->config.intf[intf].devname, usb_hub_class_dev->usbh_hub_connect_class_device_flag[hport->port]);
				if ((usb_hub_class_dev->usbh_hub_connect_class_device_flag[hport->port] & (0x1 << USB_CDC_DEVICE))
					&& usb_hub_class_dev->usbh_hub_connect_cb[hport->port][USB_CDC_DEVICE])
				{
					connect_cb_arg = (usb_hub_class_dev->usbh_hub_connect_class_device_flag + hport->port);
					usb_hub_class_dev->usbh_hub_connect_cb[hport->port][USB_CDC_DEVICE](usbh_hub_port_info, connect_cb_arg);
				}
			}
			break;
#endif

		default:
			USB_HUB_MD_LOGW("%s dev:%x info:%x arg:%x usb_device:%x\r\n", __func__,
							usb_hub_class_dev,
							usbh_hub_port_info,
							connect_cb_arg,
							usb_device);

			break;
	}

}

void bk_usbh_hub_class_disconnect_notification(struct usbh_hubport *hport, uint8_t intf, uint32_t class)
{
	bk_usbh_hub_class_dev_info *usb_hub_class_dev = &s_usb_hub_class_dev;
	bk_usb_hub_port_info *usbh_hub_port_info = NULL;
	void *disconnect_cb_arg = NULL;
	void *usb_device_param = NULL;
	void *usb_device_param_config = NULL;
	USB_HUB_MD_LOGD("%s intf:0x%x class:0x%x\r\n", __func__, intf, class);
	switch (class)
	{
#if CONFIG_USBH_UAC
		case USB_DEVICE_CLASS_AUDIO:
			usbh_hub_port_info = &usb_hub_class_dev->usbh_hub_port_info[hport->port][USB_UAC_MIC_DEVICE];
			
			if((usb_hub_class_dev->usbh_hub_connect_class_device_flag[hport->port] & (0x1 << USB_UAC_MIC_DEVICE))
				&& usb_hub_class_dev->usbh_hub_disconnect_cb[hport->port][USB_UAC_MIC_DEVICE])
			{
				disconnect_cb_arg = usb_hub_class_dev->usbh_hub_connect_cb_arg[hport->port][USB_UAC_MIC_DEVICE];
				usb_hub_class_dev->usbh_hub_disconnect_cb[hport->port][USB_UAC_MIC_DEVICE](usbh_hub_port_info, disconnect_cb_arg);
			}
			usb_hub_class_dev->usbh_hub_connect_class_device_flag[hport->port] &= ~(0x1 << USB_UAC_MIC_DEVICE);
			//free param & param_config
			usb_device_param = usbh_hub_port_info->usb_device_param;
			usb_device_param_config = usbh_hub_port_info->usb_device_param_config;
			usbh_hub_port_info->usb_device_param = NULL;
			usbh_hub_port_info->usb_device_param_config = NULL;
			if(usb_device_param) {
				os_free(usb_device_param);
			}
			if(usb_device_param_config) {
				os_free(usb_device_param_config);
			}

			usbh_hub_port_info = &usb_hub_class_dev->usbh_hub_port_info[hport->port][USB_UAC_SPEAKER_DEVICE];
			if((usb_hub_class_dev->usbh_hub_connect_class_device_flag[hport->port] & (0x1 << USB_UAC_SPEAKER_DEVICE))
				&& usb_hub_class_dev->usbh_hub_disconnect_cb[hport->port][USB_UAC_SPEAKER_DEVICE])
			{
				disconnect_cb_arg = usb_hub_class_dev->usbh_hub_connect_cb_arg[hport->port][USB_UAC_SPEAKER_DEVICE];
				usb_hub_class_dev->usbh_hub_disconnect_cb[hport->port][USB_UAC_SPEAKER_DEVICE](usbh_hub_port_info, disconnect_cb_arg);
			}
			usb_hub_class_dev->usbh_hub_connect_class_device_flag[hport->port] &= ~(0x1 << USB_UAC_SPEAKER_DEVICE);
			//free param & param_config
			usb_device_param = usbh_hub_port_info->usb_device_param;
			usb_device_param_config = usbh_hub_port_info->usb_device_param_config;
			usbh_hub_port_info->usb_device_param = NULL;
			usbh_hub_port_info->usb_device_param_config = NULL;
			if(usb_device_param) {
				os_free(usb_device_param);
			}
			if(usb_device_param_config) {
				os_free(usb_device_param_config);
			}

			break;
#endif
#if CONFIG_USBH_UVC
		case USB_DEVICE_CLASS_VIDEO:
			usbh_hub_port_info = &usb_hub_class_dev->usbh_hub_port_info[hport->port][USB_UVC_DEVICE];
			if((usb_hub_class_dev->usbh_hub_connect_class_device_flag[hport->port] & (0x1 << USB_UVC_DEVICE))
				&& usb_hub_class_dev->usbh_hub_disconnect_cb[hport->port][USB_UVC_DEVICE])
			{
				disconnect_cb_arg = usb_hub_class_dev->usbh_hub_connect_cb_arg[hport->port][USB_UVC_DEVICE];
				usb_hub_class_dev->usbh_hub_disconnect_cb[hport->port][USB_UVC_DEVICE](usbh_hub_port_info, disconnect_cb_arg);
			}
			usb_hub_class_dev->usbh_hub_connect_class_device_flag[hport->port] &= ~(0x1 << USB_UVC_DEVICE);
			//free param & param_config
			usb_device_param        = usbh_hub_port_info->usb_device_param;
			usb_device_param_config = usbh_hub_port_info->usb_device_param_config;
			usbh_hub_port_info->usb_device_param        = NULL;
			usbh_hub_port_info->usb_device_param_config = NULL;
			if(usb_device_param) {
				os_free(usb_device_param);
			}
			if(usb_device_param_config) {
				os_free(usb_device_param_config);
			}
			break;
#endif

#if CONFIG_USB_CDC
		case USB_DEVICE_CLASS_CDC:
			{
				usbh_hub_port_info = &usb_hub_class_dev->usbh_hub_port_info[hport->port][USB_CDC_DEVICE];
				if ((usb_hub_class_dev->usbh_hub_connect_class_device_flag[hport->port] & (0x1 << USB_CDC_DEVICE))
					&& usb_hub_class_dev->usbh_hub_disconnect_cb[hport->port][USB_CDC_DEVICE])
				{
					disconnect_cb_arg = (usb_hub_class_dev->usbh_hub_connect_class_device_flag + hport->port);
					usb_hub_class_dev->usbh_hub_disconnect_cb[hport->port][USB_CDC_DEVICE](usbh_hub_port_info, disconnect_cb_arg);
				}
				usb_hub_class_dev->usbh_hub_connect_class_device_flag[hport->port] &= ~(0x1 << USB_CDC_DEVICE);
				//free param & param config
				usb_device_param        = usbh_hub_port_info->usb_device_param;
				usb_device_param_config = usbh_hub_port_info->usb_device_param_config;
				usbh_hub_port_info->usb_device_param        = NULL;
				usbh_hub_port_info->usb_device_param_config = NULL;
				if (usb_device_param) {
					os_free(usb_device_param);
				}
				if (usb_device_param_config) {
					os_free(usb_device_param_config);
				}
			}
			break;
#endif
		default:
			USB_HUB_MD_LOGW("%s dev:%x info:%x arg:%x param:%x config:%x\r\n", __func__,
							usb_hub_class_dev,
							usbh_hub_port_info,
							disconnect_cb_arg,
							usb_device_param,
							usb_device_param_config);
			break;
	}

}


bk_err_t bk_usbh_hub_port_check_device(E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T device_index, bk_usb_hub_port_info **port_dev_info)
{
	if(port_index > CONFIG_USBHOST_HUB_MAX_EHPORTS) {
		USB_HUB_MD_LOGE("%s PORT_INDEX IS ERROR\r\n", __func__);
		return BK_FAIL;
	}

	if(device_index > USB_DEVICE_MAX) {
		USB_HUB_MD_LOGE("%s CLASS_DEV_INDEX IS ERROR\r\n", __func__);
		return BK_FAIL;
	}

	bk_usbh_hub_class_dev_info *usb_hub_class_dev = &s_usb_hub_class_dev;

	USB_HUB_MD_LOGV("%s connect_device_flag:0x%x\r\n", __func__, usb_hub_class_dev->usbh_hub_connect_class_device_flag[port_index]);
	if(usb_hub_class_dev->usbh_hub_connect_class_device_flag[port_index] & (0x1 << device_index)) {

		*port_dev_info = (bk_usb_hub_port_info *)&usb_hub_class_dev->usbh_hub_port_info[port_index][device_index];
		USB_HUB_MD_LOGV("%s port_dev_info:0x%x port_index:%d dev_index:%d\r\n", __func__, port_dev_info, port_index, device_index);
		return BK_OK;
	} else {
		USB_HUB_MD_LOGV("%s NULL port_index:%d dev_index:%d\r\n", __func__, port_index, device_index);
		return BK_ERR_USB_OPERATION_NULL_POINTER;
	}

}

bk_err_t bk_usbh_hub_port_video_open_handle(bk_usb_hub_port_info *port_dev_info)
{
	if(!port_dev_info)
		return BK_ERR_USB_OPERATION_NULL_POINTER;

	bk_uvc_config_t *config       = port_dev_info->usb_device_param_config;
	struct usbh_video *uvc_device = port_dev_info->usb_device;

	uint8_t formatindex = config->format_index;
	uint8_t frameindex  = config->frame_index;
	uint32_t dwMaxVideoFrameSize      = (config->width) * (config->height) * 2;
	uint32_t dwMaxPayloadTransferSize = config->ep_desc->wMaxPacketSize;
	int probe_ret = 0;
	uint8_t altsettings = 0;
	int ret = BK_OK;
	usbh_ep0_pipe_reconfigure(port_dev_info->hport->ep0, port_dev_info->hport->dev_addr, 0x40, port_dev_info->hport->speed);

	probe_ret |= usbh_videostreaming_get_cur_probe(uvc_device);
	dwMaxVideoFrameSize = uvc_device->probe.dwMaxVideoFrameSize;
	dwMaxPayloadTransferSize = uvc_device->probe.dwMaxPayloadTransferSize;
	/* config->fps is the dwFrameInterval of the frame descriptor in 100 ns units,
	 * 10000000 / fps would send an unsupported interval to the camera */
	uvc_device->probe.dwFrameInterval = config->fps;
	probe_ret |= usbh_videostreaming_set_cur_probe(uvc_device, formatindex, frameindex, dwMaxVideoFrameSize, dwMaxPayloadTransferSize);

	//! 实测更新FPS后，虽然USB返回生效了，但实际还是按照格式里指定的FPS
	probe_ret |= usbh_videostreaming_get_cur_probe(uvc_device);
	dwMaxVideoFrameSize = uvc_device->probe.dwMaxVideoFrameSize;
	dwMaxPayloadTransferSize = uvc_device->probe.dwMaxPayloadTransferSize;
	USB_HUB_MD_LOGD("after get cur probe, dwMaxVideoFrameSize: %d, dwMaxPayloadTransferSize: %d, dwFrameInterval: %d\r\n", 
		uvc_device->probe.dwMaxVideoFrameSize, uvc_device->probe.dwMaxPayloadTransferSize, uvc_device->probe.dwFrameInterval);

	probe_ret |= usbh_videostreaming_set_cur_commit(uvc_device, formatindex, frameindex, dwMaxVideoFrameSize, dwMaxPayloadTransferSize); /* select resolution from list */
	probe_ret |= usbh_videostreaming_get_cur_probe(uvc_device);
	USB_HUB_MD_LOGD("after set cur commit, dwMaxVideoFrameSize: %d, dwMaxPayloadTransferSize: %d, dwFrameInterval: %d\r\n", 
		uvc_device->probe.dwMaxVideoFrameSize, uvc_device->probe.dwMaxPayloadTransferSize, uvc_device->probe.dwFrameInterval);
	
	//! FIXME: 是否是根据分辨率获取接口的备用号
	for (uint8_t i = 1; i < uvc_device->num_of_intf_altsettings; i++) {
		altsettings = uvc_device->hport->config.intf[uvc_device->data_intf].altsetting[i].intf_desc.bAlternateSetting;
	}
	/* frameindex is the index of the frame descriptor (resolution), not the
	 * alternate setting of the streaming interface: SET_INTERFACE stalls with it.
	 * Keep the largest alternate setting found above. */
	/* altsettings = frameindex - 1; */
	USB_HUB_MD_LOGD("altsettings: %d, frameindex: %d\r\n", altsettings, frameindex);

	ret = usbh_video_open(uvc_device, altsettings); /* select ep mps from altsettings ,just for reference now */
	if(ret < 0) {
		if (ret == -EMFILE)
			USB_HUB_MD_LOGW("usbh video has been opened.\n");
		else {
			USB_HUB_MD_LOGE("usbh_video_open:%d\r\n", ret);
			USB_HUB_MD_LOGE("CHECK VIDEO OPEN FAIL\n");
		}
	} else {
		ret = BK_OK;
	}
	return ret;
}

bk_err_t bk_usbh_hub_port_dev_open(E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T device_index, bk_usb_hub_port_info *port_dev_info)
{
	uint32_t ret = BK_OK;
	bk_usbh_hub_class_dev_info *usb_hub_class_dev = &s_usb_hub_class_dev;

	if(port_index > CONFIG_USBHOST_HUB_MAX_EHPORTS) {
		USB_HUB_MD_LOGE("%s PORT_INDEX IS ERROR\r\n", __func__);
		return BK_FAIL;
	}

	if(device_index > USB_DEVICE_MAX) {
		USB_HUB_MD_LOGE("%s CLASS_DEV_INDEX IS ERROR\r\n", __func__);
		return BK_FAIL;
	}

	if(!port_dev_info) {
		USB_HUB_MD_LOGE("%s port_dev_info IS NULL\r\n", __func__);
		return BK_FAIL;
	}

	if(!port_dev_info->hport) {
		USB_HUB_MD_LOGE("%s port_dev_info->hport IS NULL\r\n", __func__);
		return BK_FAIL;
	}

	usbh_hub_event_lock_mutex();
	usbh_ep0_pipe_reconfigure(port_dev_info->hport->ep0, port_dev_info->hport->dev_addr, 0x40, port_dev_info->hport->speed);

	switch (device_index)
	{
#if CONFIG_USBH_UVC
		case USB_UVC_DEVICE:
			if(usb_hub_class_dev->usbh_hub_connect_class_device_flag[port_index] & (0x1 << USB_UVC_DEVICE)) {
				ret = bk_usbh_hub_port_video_open_handle(port_dev_info);
			} else {
				ret = BK_ERR_USB_NOT_CONNECT;
			}
			break;
		case USB_UVC_H26X_DEVICE:
			if(usb_hub_class_dev->usbh_hub_connect_class_device_flag[port_index] & (0x1 << USB_UVC_H26X_DEVICE)) {
				ret = bk_usbh_hub_port_video_open_handle(port_dev_info);
			} else {
				ret = BK_ERR_USB_NOT_CONNECT;
			}
			break;
#endif //end CONFIG_USBH_UVC
#if CONFIG_USBH_UAC
		case USB_UAC_MIC_DEVICE:
			if(usb_hub_class_dev->usbh_hub_connect_class_device_flag[port_index] & (0x1 << USB_UAC_MIC_DEVICE)) {
				ret = usbh_audio_open((struct usbh_audio *)port_dev_info->usb_device, "mic", ((bk_uac_mic_config_t *)port_dev_info->usb_device_param_config)->mic_samples_frequence);
				if(ret < 0) {
					USB_HUB_MD_LOGE("usbh_mic_open:%d\r\n", ret);
				} else {
					ret = BK_OK;
				}
			} else {
				ret = BK_ERR_USB_NOT_CONNECT;
			}
			break;
		case USB_UAC_SPEAKER_DEVICE:
			if(usb_hub_class_dev->usbh_hub_connect_class_device_flag[port_index] & (0x1 << USB_UAC_SPEAKER_DEVICE)) {
				ret = usbh_audio_open((struct usbh_audio *)port_dev_info->usb_device, "speaker", ((bk_uac_spk_config_t *)port_dev_info->usb_device_param_config)->spk_samples_frequence);
				if(ret < 0) {
					USB_HUB_MD_LOGE("usbh_speaker_open:%d\r\n", ret);
				} else {
					ret = BK_OK;
				}
			} else {
				ret = BK_ERR_USB_NOT_CONNECT;
			}
			break;
#endif
		default:
			USB_HUB_MD_LOGW("%s usb_hub_class_dev:%x\r\n", __func__, usb_hub_class_dev);
			ret = BK_FAIL;
			break;
	}
	usbh_hub_event_unlock_mutex();

	return ret;
}

bk_err_t bk_usbh_hub_port_dev_close(E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T device_index, bk_usb_hub_port_info *port_dev_info)
{
	uint32_t ret = BK_OK;

	if(port_index > CONFIG_USBHOST_HUB_MAX_EHPORTS) {
		USB_HUB_MD_LOGE("%s PORT_INDEX IS ERROR\r\n", __func__);
		return BK_FAIL;
	}

	if(device_index > USB_DEVICE_MAX) {
		USB_HUB_MD_LOGE("%s CLASS_DEV_INDEX IS ERROR\r\n", __func__);
		return BK_FAIL;
	}

	if(!port_dev_info) {
		USB_HUB_MD_LOGE("%s port_dev_info IS NULL\r\n", __func__);
		return BK_FAIL;
	}

	if(!port_dev_info->hport) {
		USB_HUB_MD_LOGE("%s port_dev_info->hport IS NULL\r\n", __func__);
		return BK_FAIL;
	}

	usbh_hub_event_lock_mutex();
	usbh_ep0_pipe_reconfigure(port_dev_info->hport->ep0, port_dev_info->hport->dev_addr, 0x40, port_dev_info->hport->speed);

	switch (device_index)
	{
#if CONFIG_USBH_UVC
		case USB_UVC_DEVICE:
		case USB_UVC_H26X_DEVICE:
			ret = usbh_video_close((struct usbh_video *)port_dev_info->usb_device);
			break;
#endif
#if CONFIG_USBH_UAC
		case USB_UAC_MIC_DEVICE:
			ret = usbh_audio_close((struct usbh_audio *)port_dev_info->usb_device, "mic");
			break;
		case USB_UAC_SPEAKER_DEVICE:
			ret = usbh_audio_close((struct usbh_audio *)port_dev_info->usb_device, "speaker");
			break;
#endif
		default:
			break;
	}
	usbh_hub_event_unlock_mutex();

	return ret;
}

bk_err_t bk_usbh_hub_dev_request_data(E_USB_HUB_PORT_INDEX port_index, E_USB_DEVICE_T device_index, struct usbh_urb *urb)
{
	uint32_t ret = BK_OK;
	USB_HUB_MD_LOGV("[+]%s\r\n", __func__);

	if(port_index > CONFIG_USBHOST_HUB_MAX_EHPORTS) {
		USB_HUB_MD_LOGE("%s PORT_INDEX IS ERROR\r\n", __func__);
		return BK_FAIL;
	}

	if(device_index > USB_DEVICE_MAX) {
		USB_HUB_MD_LOGE("%s CLASS_DEV_INDEX IS ERROR\r\n", __func__);
		return BK_FAIL;
	}

	if(!urb) {
		USB_HUB_MD_LOGE("%s urb IS NULL port_index:%d device_index:%d\r\n", __func__, port_index, device_index);
		return BK_FAIL;
	}

	ret = usbh_submit_urb(urb);
	if (ret < 0) {
		switch(-ret)
		{
			case EBUSY:
				USB_HUB_MD_LOGE("%s Urb is busy ret:%d\r\n", __func__, ret);
				break;
			case ENODEV:
				USB_HUB_MD_LOGE("%s Please check device connect ret:%d\r\n", __func__, ret);
				break;
			case EINVAL:
				USB_HUB_MD_LOGE("%s Please check pipe or urb ret:%d\r\n", __func__, ret);
				break;
			case ESHUTDOWN:
				USB_HUB_MD_LOGE("%s Check device Disconnect ret:%d\r\n", __func__, ret);
				break;
			case ETIMEDOUT:
				USB_HUB_MD_LOGE("%s Timeout wait ret:%d\r\n", __func__, ret);
				break;
			default:
				USB_HUB_MD_LOGE("%s Fail to submit urb ret:%d\r\n", __func__, ret);
				break;
		}
	}
	USB_HUB_MD_LOGV("[-]%s\r\n", __func__);

	return ret;
}

