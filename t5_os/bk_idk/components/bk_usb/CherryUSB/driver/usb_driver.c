#include <os/os.h>
#include <os/mem.h>
#include <common/bk_err.h>
#include <driver/int_types.h>
#include <driver/int.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"
// Modified by TUYA Start
#include "gpio_map.h"
// Modified by TUYA End
#include "sys_driver.h"
#include "sys_types.h"
#include "sys_rtos.h"
#include "usb_driver.h"
#include "usb_regs_address.h"
#if CONFIG_USB_HOST
#include "usbh_core.h"
#endif
#if CONFIG_USB_DEVICE
#include "usbd_core.h"
#endif

#if CONFIG_USB_UAC
#include "usbh_audio.h"
#endif
#if CONFIG_USB_UVC
#include "usbh_video.h"
#endif
#if CONFIG_UVC_UAC_DEMO
#include "bk_uvc_uac_api.h"
#endif

static beken_thread_t  s_usb_drv_thread_hdl = NULL;
static beken_queue_t s_usb_drv_msg_que = NULL;
static beken_mutex_t s_usb_drv_task_mutex = NULL;
static bk_usb_driver_comprehensive_ops *s_usb_driver_ops = NULL;

static bool s_usb_driver_init_flag = 0;
static bool s_usb_power_on_flag = 0;
static bool s_usb_open_close_flag = 0;
static bool s_usbh_device_connect_flag = 0;
static bool s_usbh_control_transfer_init_flag = 0;
static uint8_t s_usbh_class_using_status = 0;

static bk_err_t usb_driver_sw_deinit();

#define USB_DRIVER_RETURN_NOT_INIT() do {\
	if(!s_usb_driver_init_flag) {\
			return BK_FAIL;\
		}\
	} while(0)


#define USB_DRIVER_RETURN_NOT_DEINIT() do {\
	if(s_usb_driver_init_flag) {\
			return BK_FAIL;\
		}\
	} while(0)

#define USB_RETURN_NOT_POWERED_ON() do {\
		if(!s_usb_power_on_flag) {\
			return BK_ERR_USB_NOT_POWER;\
		}\
	} while(0)


#define USB_RETURN_NOT_POWERED_DOWN() do {\
		if(s_usb_power_on_flag) {\
			return BK_ERR_USB_NOT_POWER;\
		}\
	} while(0)


#define USB_RETURN_NOT_OPENED() do {\
		if(!s_usb_open_close_flag) {\
			return BK_ERR_USB_NOT_OPEN;\
		}\
	} while(0)

#define USB_RETURN_NOT_CLOSED() do {\
		if(s_usb_open_close_flag) {\
			return BK_ERR_USB_NOT_CLOSE;\
		}\
	} while(0)

#define USB_CONTROL_TRANSFER_RETURN_NOT_INIT() do {\
		if(!s_usbh_control_transfer_init_flag) {\
			return BK_FAIL;\
		}\
	} while(0)


#define USB_CONTROL_TRANSFER_RETURN_NOT_DEINIT() do {\
		if(s_usbh_control_transfer_init_flag) {\
			return BK_FAIL;\
		}\
	} while(0)

#define USB_RETURN_CLASS_IS_WORKING() do {\
		if(s_usbh_class_using_status) {\
			return BK_ERR_USB_NOT_CLOSE;\
		}\
	} while(0)

/*
#define USB_BREAK_NOT_OPENED() do {\
		if (!s_usb_open_close_flag) {\
			break;\
		}\
	} while(0)
*/
static uint32_t bk_usb_set_config(bk_usb_driver_comprehensive_ops *usb_driver)
{
	USB_DRIVER_LOGD("[+]%s\r\n", __func__);
	USB_CONTROL_TRANSFER_RETURN_NOT_INIT();
	USB_DRIVER_LOGD("[-]%s\r\n", __func__);

    return BK_OK;
}

static void bk_usb_init_all_device_driver_sw(void)
{
#if CONFIG_UVC_UAC_DEMO
	bk_usb_uvc_uac_sw_init(NULL);
#endif

#if CONFIG_USB_MSD
	extern void usbh_msc_register();
	usbh_msc_register();
#endif
}

static void bk_usb_deinit_all_device_driver_sw(void)
{
#if CONFIG_UVC_UAC_DEMO
	bk_usb_uvc_uac_sw_deinit();
#endif
}

bk_err_t bk_usb_power_ops(uint32_t gpio_id, bool ops)
{
	if(ops) {
		USB_RETURN_NOT_POWERED_DOWN();
		USB_DRIVER_LOGI("%s gpio_id:%d ops:%d s_usb_power_on_flag:%d\r\n", __func__,gpio_id, ops, s_usb_power_on_flag);
		gpio_dev_unprotect_unmap(gpio_id);
		bk_gpio_set_capacity(gpio_id, 0);
		BK_LOG_ON_ERR(bk_gpio_disable_input(gpio_id));
		BK_LOG_ON_ERR(bk_gpio_enable_output(gpio_id));
		bk_gpio_set_output_high(gpio_id);
		s_usb_power_on_flag = ops;
	} else {
		USB_DRIVER_LOGD("%s s_usb_power_on_flag:%d\r\n", __func__, s_usb_power_on_flag);
		USB_RETURN_NOT_POWERED_ON();
		if(s_usb_driver_ops) {
			if(s_usbh_class_using_status) {
				USB_DRIVER_LOGE("%s warking:0x%x!\r\n", __func__, s_usbh_class_using_status);
				return BK_ERR_USB_NOT_POWER;
			}
		}
		bk_gpio_set_output_low(gpio_id);
		USB_DRIVER_LOGD("%s setup.last POWER DOWN\r\n", __func__);
		s_usb_power_on_flag = ops;
	}

	return BK_OK;
}

bk_err_t bk_usb_driver_init(void)
{
	USB_DRIVER_RETURN_NOT_DEINIT();
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);

	bk_usb_init_all_device_driver_sw();

#if CONFIG_USB_MAILBOX_SLAVE
	bk_usb_mailbox_sw_slave_init();
#endif
	s_usb_driver_init_flag = 1;

	USB_DRIVER_LOGD("[-]%s\r\n",__func__);

	return BK_OK;
}

bk_err_t bk_usb_driver_deinit(void)
{
	USB_DRIVER_RETURN_NOT_INIT();

	sys_drv_int_disable(USB_INTERRUPT_CTRL_BIT);
	bk_int_isr_unregister(INT_SRC_USB);
	sys_drv_dev_clk_pwr_up(CLK_PWR_ID_USB_1, CLK_PWR_CTRL_PWR_DOWN);
	bk_usb_deinit_all_device_driver_sw();
	s_usb_driver_init_flag = 0;
	return BK_OK;
}

static void bk_analog_layer_usb_sys_related_ops(uint32_t usb_mode, bool ops)
{
	extern void bk_delay(INT32 num);

#if 0
	sys_drv_usb_analog_phy_en(ops, NULL);
	sys_drv_usb_analog_speed_en(ops, NULL);
	sys_drv_usb_analog_ckmcu_en(ops, NULL);
#endif
	if(ops){
		sys_drv_usb_clock_ctrl(true, NULL);
		bk_delay(100);
#if 0
		sys_drv_usb_analog_deepsleep_en(false);
#endif
		sys_drv_usb_analog_dp_capability(USB_DP_CAPABILITY_VALUE);
		sys_drv_usb_analog_dn_capability(USB_DN_CAPABILITY_VALUE);
		if(!sys_hal_psram_ldo_status()) {
			sys_drv_psram_ldo_enable(1);
		}
		sys_drv_usb_analog_phy_en(1, NULL);

		if(usb_mode == USB_HOST_MODE) {
			REG_USB_USR_708 = 0x0;
			REG_USB_USR_710 &= ~(0x1<< 7);
			bk_delay(100);

			REG_USB_USR_710 |= (0x1<<15);
			//REG_USB_USR_710 |= (0x1<<14);
			REG_USB_USR_710 |= (0x1<<16);
			REG_USB_USR_710 |= (0x1<<17);
			REG_USB_USR_710 |= (0x1<<18);
			REG_USB_USR_710 |= (0x1<<19);
			REG_USB_USR_710 &=~(0x1<<20);
			REG_USB_USR_710 |= (0x1<<21);
			REG_USB_USR_710 |= (0x0<< 0);
			REG_USB_USR_710 |= (0x1<< 5);
			REG_USB_USR_710 |= (0x1<< 6);
			REG_USB_USR_710 |= (0x1<< 9);
			REG_USB_USR_710 |= (0x1<<10);
			REG_USB_USR_710 |= (0x1<< 1);

			REG_USB_USR_710 |= (0x1<< 7);
			REG_USB_USR_708  =	0x1;
#if 0
			REG_USB_PHY_00   = 0x08;     
			REG_USB_PHY_01   = 0x02;
			REG_USB_USR_710 |= (0x1<< 8);
        	while(1){
				reg = REG_USB_USR_70C;
				if(reg & 0x100){
					USB_DRIVER_LOGI("SelfTest Fin!\r\n");
					USB_DRIVER_LOGI("test end!\r\n");
					break;
				} else {
					USB_DRIVER_LOGI("70c_reg:0x%x\r\n", reg);
					bk_delay(10000);
				}
        	}
			REG_USB_PHY_00   &= ~0x08;     
			REG_USB_PHY_01   &= ~0x02;
			REG_USB_USR_710 &= ~(0x1<< 8);
#endif
		} else {
					REG_USB_USR_710 |= (0x1<<15);
			//REG_USB_USR_710 |= (0x1<<14);
			REG_USB_USR_710 |= (0x1<<16);
			REG_USB_USR_710 |= (0x1<<17);
			REG_USB_USR_710 |= (0x1<<18);
			REG_USB_USR_710 |= (0x1<<19);
			REG_USB_USR_710 |= (0x1<<20);
			REG_USB_USR_710 &=~(0x1<<21);
			REG_USB_USR_710 |= (0x0<< 0);
			REG_USB_USR_710 |= (0x1<< 5);
			REG_USB_USR_710 |= (0x1<< 6);
			REG_USB_USR_710 |= (0x1<< 9);
			REG_USB_USR_710 |= (0x1<<10);
			REG_USB_USR_710 |= (0x1<< 7);
			REG_USB_USR_710 |= (0x1<< 1);

			REG_USB_USR_708  =	0x1;

		}
	} else {
		sys_drv_usb_analog_phy_en(0, NULL);
		sys_drv_usb_clock_ctrl(false, NULL);
	}
}

// Modified by TUYA Start
#include "tuya_cloud_types.h"
// Modified by TUYA End
static void bk_usb_phy_register_refresh()
{
#if CONFIG_USB_HOST
	sys_drv_int_disable(USB_INTERRUPT_CTRL_BIT);
    // Modified by TUYA Start
#ifdef CONFIG_TUYA_GPIO_MAP
    uint8_t usb_ldo, active_level;
    extern OPERATE_RET tkl_vi_get_power_info(uint8_t device_type, uint8_t *io, uint8_t *active);
    tkl_vi_get_power_info(UVC_CAMERA, &usb_ldo, &active_level);
	bk_gpio_set_output_low(usb_ldo);
#else
	bk_gpio_set_output_low(CONFIG_USB_VBAT_CONTROL_GPIO_ID);
#endif // CONFIG_TUYA_GPIO_MAP
    // Modified by TUYA End

	bk_analog_layer_usb_sys_related_ops(USB_HOST_MODE, false);
	bk_analog_layer_usb_sys_related_ops(USB_HOST_MODE, true);
	extern int usb_hc_mhdrc_register_init(void);
	usb_hc_mhdrc_register_init();
	sys_drv_int_enable(USB_INTERRUPT_CTRL_BIT);
	if(s_usb_power_on_flag) {
    // Modified by TUYA Start
#ifdef CONFIG_TUYA_GPIO_MAP
		bk_gpio_set_output_high(usb_ldo);
#else
		bk_gpio_set_output_high(CONFIG_USB_VBAT_CONTROL_GPIO_ID);
#endif // CONFIG_TUYA_GPIO_MAP
    // Modified by TUYA End
	}
#endif

}

void bk_usb_enum_fail_all_reg_reset()
{
#if CONFIG_USB_HOST
	sys_drv_int_disable(USB_INTERRUPT_CTRL_BIT);
    // Modified by TUYA Start
#ifdef CONFIG_TUYA_GPIO_MAP
    uint8_t usb_ldo, active_level;
    extern OPERATE_RET tkl_vi_get_power_info(uint8_t device_type, uint8_t *io, uint8_t *active);
    tkl_vi_get_power_info(UVC_CAMERA, &usb_ldo, &active_level);
	bk_gpio_set_output_low(usb_ldo);
#else
	bk_gpio_set_output_low(CONFIG_USB_VBAT_CONTROL_GPIO_ID);
#endif // CONFIG_TUYA_GPIO_MAP
    // Modified by TUYA End
	rtos_delay_milliseconds(10);
	bk_analog_layer_usb_sys_related_ops(USB_HOST_MODE, false);
	rtos_delay_milliseconds(10);
	bk_analog_layer_usb_sys_related_ops(USB_HOST_MODE, true);
	rtos_delay_milliseconds(10);
	extern int usb_hc_mhdrc_register_init(void);
	usb_hc_mhdrc_register_init();
	sys_drv_int_enable(USB_INTERRUPT_CTRL_BIT);
#ifdef CONFIG_TUYA_GPIO_MAP
	bk_gpio_set_output_high(usb_ldo);
#else
	bk_gpio_set_output_high(CONFIG_USB_VBAT_CONTROL_GPIO_ID);
#endif // CONFIG_TUYA_GPIO_MAP
    // Modified by TUYA End
#endif

}

static s_bk_usb_descriptor_header  *usb_get_next_desc (uint8_t   *pbuf, uint16_t  *ptr)
{
    s_bk_usb_descriptor_header  *pnext;

    *ptr += ((s_bk_usb_descriptor_header *)pbuf)->bLength;
    pnext = (s_bk_usb_descriptor_header *)((uint8_t *)pbuf + \
           ((s_bk_usb_descriptor_header *)pbuf)->bLength);

    return (pnext);
}

static void bk_usb_parse_descriptors_type_cs_interface(uint8_t interface_class, uint8_t interface_sub_class, uint8_t *descriptors_buffer)
{
	switch (interface_class)
	{
#if CONFIG_USB_UAC
		case USB_DEVICE_CLASS_AUDIO:
			switch (interface_sub_class)
			{
				case BK_USB_SUBCLASS_CONTROL:
					//bk_usb_parse_audio_control_descriptors(descriptors_buffer);
					break;
				case BK_USB_SUBCLASS_STREAMING:
					//bk_usb_parse_audio_streaming_descriptors(descriptors_buffer);
					break;
				default:
					break;
			}
			break;
#endif

#if CONFIG_USB_UVC
		case USB_DEVICE_CLASS_VIDEO:
			switch (interface_sub_class)
			{
				case BK_USB_SUBCLASS_CONTROL:
					//bk_usb_parse_video_control_descriptors(descriptors_buffer);
					break;
				case BK_USB_SUBCLASS_STREAMING:
					//bk_usb_parse_video_streaming_descriptors(descriptors_buffer);
					break;
				default:
					break;
			}
			break;
#endif
		default:
			break;

	}
}

static void bk_usb_updata_endpoint(uint8_t interface_class, uint8_t interface_sub_class, uint8_t *descriptors_buffer)
{
	switch (interface_class)
	{
#if CONFIG_USB_UAC
		case USB_DEVICE_CLASS_AUDIO:
			//bk_usb_updata_audio_endpoint(interface_sub_class, descriptors_buffer);
			break;
#endif

#if CONFIG_USB_UVC
		case USB_DEVICE_CLASS_VIDEO:
			//bk_usb_updata_video_endpoint(interface_sub_class, descriptors_buffer);
			break;
#endif
		default:
			break;

	}

}

static void bk_usb_parse_descriptors_type_cs_endpoint(uint8_t interface_class, uint8_t *descriptors_buffer)
{
	struct usb_audio_endpoint_general_descriptor_t *endpoint_general_desc;

	switch (interface_class)
	{
		case USB_DEVICE_CLASS_AUDIO:
			endpoint_general_desc = (struct usb_audio_endpoint_general_descriptor_t *)descriptors_buffer;
			USB_DRIVER_LOGD("     ------------ Audio Data Endpoint Descriptor -----------  \r\n");
			USB_DRIVER_LOGD("bLength                        : 0x%x (%d bytes)\r\n", endpoint_general_desc->bLength, endpoint_general_desc->bLength);
			USB_DRIVER_LOGD("bDescriptorType                : 0x%x (Audio Endpoint Descriptor)\r\n", endpoint_general_desc->bDescriptorType);
			USB_DRIVER_LOGD("bDescriptorSubtype             : 0x%x (General)\r\n", endpoint_general_desc->bDescriptorSubtype);
			USB_DRIVER_LOGD("bmAttributes                   : 0x%x\r\n", endpoint_general_desc->bmAttributes);
			USB_DRIVER_LOGD("bLockDelayUnits                : 0x%x\r\n", endpoint_general_desc->bLockDelayUnits);
			USB_DRIVER_LOGD("wLockDelay                     : 0x%x\r\n", endpoint_general_desc->wLockDelay);
			//bk_usb_updata_audio_data_endpoint_descriptor(descriptors_buffer);
			break;
		case USB_DEVICE_CLASS_VIDEO:
			break;
		default:
			break;

	}

}

static void bk_usb_updata_interface(bk_usb_driver_comprehensive_ops *usb_driver, uint8_t bInterfaceNumber, uint8_t interface_class, uint8_t interface_sub_class)
{
	switch (interface_class)
	{
#if CONFIG_USB_UAC
		case USB_DEVICE_CLASS_AUDIO:
			bk_usb_updata_audio_interface(usb_driver->hport, bInterfaceNumber, interface_sub_class);
			break;
#endif

#if CONFIG_USB_UVC
		case USB_DEVICE_CLASS_VIDEO:
			bk_usb_updata_video_interface(usb_driver->hport, bInterfaceNumber, interface_sub_class);
			break;
#endif
		default:
			break;
	}

	return;
}

static void bk_usb_parse_all_descriptors(bk_usb_driver_comprehensive_ops *usb_driver)
{
	struct usbh_hubport *hport = usb_driver->hport;
	if(!hport->raw_config_desc) {
		USB_DRIVER_LOGE("descriptors_buffer IS NULL\r\n");
		return;
	} else
		USB_DRIVER_LOGD("Descriptors_buffer:%x\r\n", hport->raw_config_desc);
	struct usb_device_descriptor_t dev_descriptor;
	bk_usb_get_device_descriptor(&dev_descriptor);

	s_bk_usb_descriptor_header *pdesc;
	const uint32_t TotalLength = ((struct usb_configuration_descriptor_t *)hport->raw_config_desc)->wTotalLength;
	uint16_t ptr = USB_SIZEOF_CONFIG_DESC;
	pdesc = (s_bk_usb_descriptor_header *)hport->raw_config_desc;

    uint8_t interface_class = USB_DEVICE_CLASS_RESERVED;
	uint8_t interface_sub_class =BK_USB_SUBCLASS_UNDEFINED;

	struct usb_configuration_descriptor_t *con_desc;
	struct usb_interface_association_descriptor *iad_desc;
	struct usb_interface_descriptor *interface_desc;
	struct usb_endpoint_descriptor *ep_desc;

	while(ptr < (TotalLength + USB_SIZEOF_CONFIG_DESC))
	{
		USB_DRIVER_LOGD("[=] %s while TotalLength:%d ptr:%d pdesc->bDescriptorType:0x%x\r\n", __func__, TotalLength, ptr, pdesc->bDescriptorType);
		if(pdesc->bDescriptorType == 0)
			break;

		switch (pdesc->bDescriptorType)
		{
			case USB_DESCRIPTOR_TYPE_CONFIGURATION:
				con_desc = (struct usb_configuration_descriptor_t *)pdesc;
				USB_DRIVER_LOGD("   ------------- Configuration Descriptor ------------\r\n");
				USB_DRIVER_LOGD("bLength              : 0x%x (%d bytes)\r\n", con_desc->bLength, con_desc->bLength);
				USB_DRIVER_LOGD("bDescriptorType      : 0x%x (Configuration Descriptor)\r\n", con_desc->bDescriptorType);
				USB_DRIVER_LOGD("wTotalLength         : 0x%x (%d bytes)\r\n", con_desc->wTotalLength, con_desc->wTotalLength);
				USB_DRIVER_LOGD("bNumInterfaces       : 0x%x (%d Interfaces)\r\n", con_desc->bNumInterfaces, con_desc->bNumInterfaces);
				USB_DRIVER_LOGD("bConfigurationValue  : 0x%x (Configuration %d)\r\n", con_desc->bConfigurationValue, con_desc->bConfigurationValue);
				USB_DRIVER_LOGD("iConfiguration       : 0x%x\r\n", con_desc->iConfiguration);
				USB_DRIVER_LOGD("bmAttributes         : 0x%x\r\n", con_desc->bmAttributes);
				USB_DRIVER_LOGD("bMaxPower            : 0x%x (%d mA)\r\n", con_desc->bMaxPower, con_desc->bMaxPower * 2);
				break;

			case USB_DESCRIPTOR_TYPE_INTERFACE_ASSOCIATION:
				iad_desc = (struct usb_interface_association_descriptor *)pdesc;
				USB_DRIVER_LOGD("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\r\n");
				USB_DRIVER_LOGD("  -------------------- IAD Descriptor ----------------- \r\n");
				USB_DRIVER_LOGD("bLength                        : 0x%x (%d bytes)\r\n", iad_desc->bLength, iad_desc->bLength);
				USB_DRIVER_LOGD("bDescriptorType                : 0x%x (Interface Association Descriptor)\r\n",iad_desc->bDescriptorType);
				USB_DRIVER_LOGD("bFirstInterface                : 0x%x (Interface %d)\r\n", iad_desc->bFirstInterface, iad_desc->bFirstInterface);
				USB_DRIVER_LOGD("bInterfaceCount                : 0x%x (%d Interfaces)\r\n", iad_desc->bInterfaceCount, iad_desc->bInterfaceCount);
				USB_DRIVER_LOGD("bFunctionClass                 : 0x%x\r\n",iad_desc->bFunctionClass);
				USB_DRIVER_LOGD("bFunctionSubClass              : 0x%x\r\n",iad_desc->bFunctionSubClass);
				USB_DRIVER_LOGD("bFunctionProtocol              : 0x%x\r\n",iad_desc->bFunctionProtocol);
				USB_DRIVER_LOGD("iFunction                      : 0x%x\r\n",iad_desc->iFunction);
				break;
			case USB_DESCRIPTOR_TYPE_INTERFACE:
				interface_desc = (struct usb_interface_descriptor *)pdesc;
				USB_DRIVER_LOGD("*************************************************************************\r\n");
				USB_DRIVER_LOGD("     ------------ Interface Descriptor -----------  \r\n");
				USB_DRIVER_LOGD("bLength                        : 0x%x (%d bytes)\r\n", interface_desc->bLength, interface_desc->bLength);
				USB_DRIVER_LOGD("bDescriptorType                : 0x%x (Interface Descriptor)\r\n",interface_desc->bDescriptorType);
				USB_DRIVER_LOGD("bInterfaceNumber               : 0x%x (Interface %d)\r\n", interface_desc->bInterfaceNumber, interface_desc->bInterfaceNumber);
				USB_DRIVER_LOGD("bAlternateSetting              : 0x%x\r\n",interface_desc->bAlternateSetting);
				USB_DRIVER_LOGD("bNumEndpoints                  : 0x%x\r\n",interface_desc->bNumEndpoints);
				USB_DRIVER_LOGD("bFunctionSubClass              : 0x%x\r\n",interface_desc->bInterfaceClass);
				USB_DRIVER_LOGD("bInterfaceSubClass             : 0x%x\r\n",interface_desc->bInterfaceSubClass);
				USB_DRIVER_LOGD("bInterfaceProtocol             : 0x%x\r\n",interface_desc->bInterfaceProtocol);
				USB_DRIVER_LOGD("iInterface                     : 0x%x\r\n",interface_desc->iInterface);
				interface_class = interface_desc->bInterfaceClass;
				interface_sub_class = interface_desc->bInterfaceSubClass;
				bk_usb_updata_interface(usb_driver, interface_desc->bInterfaceNumber, interface_class, interface_sub_class);
				break;

			case USB_DESCRIPTOR_TYPE_ENDPOINT:
				ep_desc = (struct usb_endpoint_descriptor *)pdesc;
				USB_DRIVER_LOGD("=========================================================================\r\n");
				USB_DRIVER_LOGD("     ------------ Endpoint Descriptor -----------  \r\n");
				USB_DRIVER_LOGD("bLength                        : 0x%x (%d bytes)\r\n", ep_desc->bLength, ep_desc->bLength);
				USB_DRIVER_LOGD("bDescriptorType                : 0x%x (Endpoint Descriptor)\r\n", ep_desc->bDescriptorType);
				if(ep_desc->bEndpointAddress & USB_EP_DIR_IN)
					USB_DRIVER_LOGD("bEndpointAddress               : 0x%x (Direction=IN  EndpointID=%d)\r\n", ep_desc->bEndpointAddress, (ep_desc->bEndpointAddress & 0x0F));
				else
					USB_DRIVER_LOGD("bEndpointAddress               : 0x%x (Direction=OUT  EndpointID=%d)\r\n", ep_desc->bEndpointAddress, (ep_desc->bEndpointAddress & 0x0F));
				USB_DRIVER_LOGD("bmAttributes                   : 0x%x\r\n", ep_desc->bmAttributes);
				USB_DRIVER_LOGD("wMaxPacketSize                 : 0x%x (%d bytes)\r\n", ep_desc->wMaxPacketSize, ep_desc->wMaxPacketSize);
				USB_DRIVER_LOGD("bInterval                      : 0x%x (%d ms)\r\n", ep_desc->bInterval, ep_desc->bInterval);
				bk_usb_updata_endpoint(interface_class, interface_sub_class, (uint8_t *)pdesc);
				break;

			case USB_CS_DESCRIPTOR_TYPE_INTERFACE:
				bk_usb_parse_descriptors_type_cs_interface(interface_class, interface_sub_class, (uint8_t *)pdesc);
				break;
			case USB_CS_DESCRIPTOR_TYPE_ENDPOINT:
				bk_usb_parse_descriptors_type_cs_endpoint(interface_class, (uint8_t *)pdesc);
				break;

			default:
				break;
		}

		pdesc = usb_get_next_desc((uint8_t*) pdesc, &ptr);
	}
	USB_DRIVER_LOGD("[-] %s\r\n", __func__);

}

void bk_usb_device_set_using_status(bool use_or_no, E_USB_DEVICE_T dev)
{
    uint32_t int_level;

    int_level = rtos_disable_int();

    if(use_or_no){
        s_usbh_class_using_status |= (1 << dev);
        USB_DRIVER_LOGD("%s device is working %d\r\n", __func__, s_usbh_class_using_status);
    } else {
        s_usbh_class_using_status &= ~(1 << dev);
        USB_DRIVER_LOGD("%s device is working %d\r\n", __func__, s_usbh_class_using_status);
    }

    rtos_enable_int(int_level);
}

bk_err_t bk_usb_drv_send_msg(int op, void *param)
{
	bk_err_t ret;
	bk_usb_drv_msg_t msg;

	msg.op = op;
	msg.param = param;
	if (s_usb_drv_msg_que) {
		ret = rtos_push_to_queue(&s_usb_drv_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			USB_DRIVER_LOGE("usb_driver_send_msg fail ret:%d op:%d\r\n", ret, op);
			return BK_FAIL;
		}

		return ret;
	}
	return BK_OK;
}

bk_err_t bk_usb_drv_send_msg_front(int op, void *param)
{
	bk_err_t ret;
	bk_usb_drv_msg_t msg;

	msg.op = op;
	msg.param = param;
	if (s_usb_drv_msg_que) {
		ret = rtos_push_to_queue_front(&s_usb_drv_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			USB_DRIVER_LOGE("%s fail \r\n", __func__);
			return BK_FAIL;
		}

		return ret;
	}
	return BK_OK;
}

static void usb_control_trans_handle(bk_usb_control_trans_assembly* info)
{
	USB_DRIVER_LOGD("%s! delay_ms:%d\r\n",__func__, info->delay_ms);

	bk_usb_control_transfer(info->setup, info->buf_info);

}

void bk_usb_driver_task_lock_mutex()
{
	if(s_usb_drv_task_mutex)
		rtos_lock_mutex(&s_usb_drv_task_mutex);
}

void bk_usb_driver_task_unlock_mutex()
{
	if(s_usb_drv_task_mutex)
		rtos_unlock_mutex(&s_usb_drv_task_mutex);
}

static void usb_driver_connect_handle(bk_usb_driver_comprehensive_ops *usb_driver)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);
	s_usbh_device_connect_flag = true;

	for(E_USB_DEVICE_T dev = USB_UVC_DEVICE; dev < USB_DEVICE_MAX; dev++)
	{
		if(usb_driver->usbh_connect_cb[dev]) {
			usb_driver->usbh_class_connect_status |= (1 << dev);
			usb_driver->usbh_connect_cb[dev]();
		}
	}

	USB_DRIVER_LOGD("[-]%s\r\n",__func__);
}

static void usb_driver_disconnect_handle(bk_usb_driver_comprehensive_ops *usb_driver)
{
	USB_DRIVER_LOGD("[+]%s\r\n",__func__);
	s_usbh_device_connect_flag = false;
	usb_driver->usbh_class_start_status = 0x0;
	for(E_USB_DEVICE_T dev = USB_UVC_DEVICE; dev < USB_DEVICE_MAX; dev++)
	{
		if(usb_driver->usbh_disconnect_cb[dev]) {
			usb_driver->usbh_class_connect_status &= ~(1 << dev);
			usb_driver->usbh_disconnect_cb[dev]();
		}
	}

	USB_DRIVER_LOGD("[-]%s\r\n",__func__);
}

static void usb_drv_task_main(beken_thread_arg_t param_data)
{
	bk_err_t ret = kNoErr;

	while(1) {
		bk_usb_drv_msg_t msg;
		ret = rtos_pop_from_queue(&s_usb_drv_msg_que, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == ret) {
			switch (msg.op) {
				case USB_DRV_IDLE:
					break;
				case USB_DRV_EXIT:
					USB_DRIVER_LOGI("USB_DRV_EXIT!\r\n");
					rtos_reset_queue(&s_usb_drv_msg_que);
#if CONFIG_USB_MAILBOX_SLAVE
					usb_mailbox_slave_usb_driver_exit();
#endif
					s_usb_open_close_flag = 0;
					s_usbh_device_connect_flag = false;
					s_usb_driver_ops = NULL;
#if CONFIG_SYS_CPU0
					bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_USB_1, 0x1, 100000000);
#endif
					goto usb_driver_exit;
					break;

				case USB_DRV_PRASE_DESCRIPTOR:
					USB_DRIVER_LOGI("USB_DRV_PRASE_DESCRIPTOR!\r\n");
					s_usb_driver_ops = (bk_usb_driver_comprehensive_ops *)msg.param;
					bk_usb_driver_task_lock_mutex();
					bk_usb_parse_all_descriptors((bk_usb_driver_comprehensive_ops *)msg.param);
					bk_usb_driver_task_unlock_mutex();
					break;

				case USB_DRV_INIT_CONTROL_TRANS:
					USB_DRIVER_LOGI("USB_DRV_INIT_CONTROL_TRANS!\r\n");
					s_usb_driver_ops = (bk_usb_driver_comprehensive_ops *)msg.param;
					bk_usb_control_transfer_init();
					break;
				case USB_DRV_DEINIT_CONTROL_TRANS:
					USB_DRIVER_LOGI("USB_DRV_DEINIT_CONTROL_TRANS!\r\n");
					if(bk_usb_get_device_connect_status()) break;
					bk_usb_control_transfer_deinit();
					//reset queue


					break;
				case USB_DRV_CONTROL_TRANSFER:
					USB_DRIVER_LOGI("USB_DRV_CONTROL_TRANSFER!\r\n");
					usb_control_trans_handle((bk_usb_control_trans_assembly *)msg.param);
					break;

				case USB_DRV_CONNECT_HANDLE:
					USB_DRIVER_LOGI("USB_DRV_CONNECT_HANDLE!\r\n");
					rtos_reset_queue(&s_usb_drv_msg_que);
					usb_driver_connect_handle((bk_usb_driver_comprehensive_ops *)msg.param);
					break;
				case USB_DRV_DISCONNECT_HANDLE:
					USB_DRIVER_LOGI("USB_DRV_DISCONNECT_HANDLE!\r\n");
					if(!bk_usb_get_device_connect_status()) break;
					bk_usb_driver_task_lock_mutex();
					usb_driver_disconnect_handle((bk_usb_driver_comprehensive_ops *)msg.param);
					rtos_reset_queue(&s_usb_drv_msg_que);
#if CONFIG_UVC_UAC_DEMO
					bk_usb_uvc_uac_free_enumerate_resources();
#endif
					bk_usb_phy_register_refresh();
					bk_usb_drv_send_msg_front(USB_DRV_DEINIT_CONTROL_TRANS, NULL);
					bk_usb_driver_task_unlock_mutex();
					break;

#if CONFIG_UVC_UAC_DEMO
				case USB_DRV_VIDEO_OPEN_PIPE:
					USB_DRIVER_LOGI("USB_DRV_VIDEO_OPEN_PIPE!\r\n");
					break;
				case USB_DRV_VIDEO_CLOSE_PIPE:
					USB_DRIVER_LOGI("USB_DRV_VIDEO_CLOSE_PIPE!\r\n");
					break;

				case USB_DRV_VIDEO_START:
					USB_DRIVER_LOGI("USB_DRV_VIDEO_START!\r\n");
					if(!bk_usb_get_device_connect_status()) break;
					bk_usbh_video_start_handle((struct usbh_video *)msg.param);
					break;
				case USB_DRV_VIDEO_DUAL_START:
					USB_DRIVER_LOGI("USB_DRV_VIDEO_DUAL_START!\r\n");
					if(!bk_usb_get_device_connect_status()) break;
					bk_usbh_video_start_dual_handle((struct usbh_video *)msg.param);
					break;
				case USB_DRV_VIDEO_DUAL_STOP:
				case USB_DRV_VIDEO_STOP:
					USB_DRIVER_LOGI("USB_DRV_VIDEO_STOP!\r\n");
					if(!bk_usb_get_device_connect_status()) break;
					bk_usbh_video_stop_handle(msg.param);
					break;
				case USB_DRV_VIDEO_RX:
					if(!bk_usb_get_device_connect_status()) break;
					USB_DRIVER_LOGD("USB_DRV_VIDEO_RX!\r\n");
					//malloc_list_buffer
					bk_uvc_trigger_video_stream_rx(msg.param);
					break;
				case USB_DRV_VIDEO_DUAL_RX:
					if(!bk_usb_get_device_connect_status()) break;
					USB_DRIVER_LOGD("USB_DRV_VIDEO_DUAL_RX!\r\n");
					//malloc_list_buffer
					bk_uvc_trigger_video_stream_dual_rx(msg.param);
					break;
				case USB_DRV_VIDEO_RXED:
					if(!bk_usb_get_device_connect_status()) break;
					USB_DRIVER_LOGD("USB_DRV_VIDEO_RXED!\r\n");
					//push_list_buffer
					break;

				case USB_DRV_AUDIO_OPEN_PIPE:
					USB_DRIVER_LOGI("USB_DRV_AUDIO_OPEN_PIPE!\r\n");
					break;
				case USB_DRV_AUDIO_CLOSE_PIPE:
					USB_DRIVER_LOGI("USB_DRV_AUDIO_CLOSE_PIPE!\r\n");
					break;

				case USB_DRV_AUDIO_MIC_START:
					USB_DRIVER_LOGI("USB_DRV_AUDIO_MIC_START!\r\n");
					if(!bk_usb_get_device_connect_status()) break;
					bk_uac_start_mic_handle((struct usbh_audio *)msg.param);
					break;
				case USB_DRV_AUDIO_MIC_STOP:
					USB_DRIVER_LOGI("USB_DRV_AUDIO_MIC_STOP!\r\n");
					if(!bk_usb_get_device_connect_status()) break;
					bk_uac_stop_mic_handle(msg.param);
					break;
				case USB_DRV_AUDIO_SPK_START:
					USB_DRIVER_LOGI("USB_DRV_AUDIO_SPK_START!\r\n");
					if(!bk_usb_get_device_connect_status()) break;
					bk_uac_start_speaker_handle(msg.param);
					break;
				case USB_DRV_AUDIO_SPK_STOP:
					USB_DRIVER_LOGI("USB_DRV_AUDIO_SPK_STOP!\r\n");
					if(!bk_usb_get_device_connect_status()) break;
					bk_uac_stop_speaker_handle(msg.param);
					break;
				case USB_DRV_AUDIO_RX:
					USB_DRIVER_LOGD("USB_DRV_VIDEO_RX!\r\n");
					if(!bk_usb_get_device_connect_status()) break;
					//malloc_list_buffer
					bk_uac_trigger_audio_stream_rx(msg.param);
					break;
				case USB_DRV_AUDIO_RXED:
					USB_DRIVER_LOGD("USB_DRV_VIDEO_RXED!\r\n");
					if(!bk_usb_get_device_connect_status()) break;
					//push_list_buffer
					break;

				case USB_DRV_AUDIO_TX:
					USB_DRIVER_LOGD("USB_DRV_AUDIO_TX!\r\n");
					if(!bk_usb_get_device_connect_status()) break;
					//malloc_list_buffer
					bk_uac_trigger_audio_stream_tx(msg.param);
					break;
				case USB_DRV_AUDIO_TXED:
					if(!bk_usb_get_device_connect_status()) break;
					USB_DRIVER_LOGD("USB_DRV_AUDIO_TXED!\r\n");
					//push_list_buffer
					break;
#endif
				case USB_DRV_USB_OPEN:
					USB_DRIVER_LOGI("USB_DRV_USB_OPEN!\r\n");
					if(*((uint32_t *)msg.param) == USB_HOST_MODE) {
#if CONFIG_USB_HOST	
						bk_analog_layer_usb_sys_related_ops(USB_HOST_MODE, true);
						usbh_initialize();
						USB_DRIVER_LOGI("USB HOST\r\n");
#endif
					} else if(*((uint32_t *)msg.param) == USB_DEVICE_MODE){
#if CONFIG_USB_DEVICE
						bk_analog_layer_usb_sys_related_ops(USB_DEVICE_MODE, true);
						usbd_initialize();
						USB_DRIVER_LOGI("USB DEVICE\r\n");
#endif
					} else
						USB_DRIVER_LOGI("PLEASE check USB mode\r\n");

					s_usb_open_close_flag = 1;
					break;
				case USB_DRV_USB_CLOSE:
					USB_DRIVER_LOGI("USB_DRV_USB_CLOSE!\r\n");
					if(!s_usb_open_close_flag) break;
					bk_usb_driver_task_lock_mutex();
					sys_drv_int_disable(USB_INTERRUPT_CTRL_BIT);
#if CONFIG_UVC_UAC_DEMO
					if(bk_usb_get_device_connect_status()) {
						bk_usb_uvc_uac_free_enumerate_resources();
					}
#endif

#if CONFIG_USB_HOST
					usbh_deinitialize();
					bk_analog_layer_usb_sys_related_ops(USB_HOST_MODE, false);
#endif
#if CONFIG_USB_DEVICE
					usbd_deinitialize();
					bk_analog_layer_usb_sys_related_ops(USB_DEVICE_MODE, false);
#endif
					bk_usb_driver_task_unlock_mutex();

					bk_usb_drv_send_msg_front(USB_DRV_EXIT, NULL);
					break;

				default:
					break;
			}
		}
	}

usb_driver_exit:
	usb_driver_sw_deinit();
}

static bk_err_t usb_driver_sw_init()
{
	uint32_t ret = BK_OK;
	USB_DRIVER_LOGD("[+]%s thread_hdl:%x  msg_que:%x\r\n",__func__, s_usb_drv_thread_hdl, s_usb_drv_msg_que);
    // Modified by TUYA Start
#ifdef CONFIG_TUYA_GPIO_MAP
    uint8_t usb_ldo, active_level;
    extern OPERATE_RET tkl_vi_get_power_info(uint8_t device_type, uint8_t *io, uint8_t *active);
    tkl_vi_get_power_info(UVC_CAMERA, &usb_ldo, &active_level);
	bk_gpio_set_output_high(usb_ldo);
#else
	bk_gpio_set_output_high(CONFIG_USB_VBAT_CONTROL_GPIO_ID);
#endif // CONFIG_TUYA_GPIO_MAP
    // Modified by TUYA End


	if ((!s_usb_drv_thread_hdl) && (!s_usb_drv_msg_que)) {

		if(!s_usb_drv_task_mutex)
			rtos_init_mutex(&s_usb_drv_task_mutex);

		ret = rtos_init_queue(&s_usb_drv_msg_que,
							  "usb_driver_queue",
							  sizeof(bk_usb_drv_msg_t),
							  USB_DRIVER_QITEM_COUNT);
		if (ret != kNoErr) {
			USB_DRIVER_LOGE("ceate usb driver internal message queue fail \r\n");
			return BK_FAIL;
		}
		USB_DRIVER_LOGD("ceate usb driver internal message queue complete \r\n");

		//create usb driver task
		ret = rtos_create_thread(&s_usb_drv_thread_hdl,
							 CONFIG_TASK_USB_PRIO,
							 "usb_driver",
							 (beken_thread_function_t)usb_drv_task_main,
							 CONFIG_USB_DRIVER_TASK_SIZE,
							 NULL);
		if (ret != kNoErr) {
			USB_DRIVER_LOGE("create usb driver task fail \r\n");
			rtos_deinit_queue(&s_usb_drv_msg_que);
			s_usb_drv_msg_que = NULL;
			s_usb_drv_thread_hdl = NULL;
		}
		USB_DRIVER_LOGD("create usb driver task complete \r\n");

	} else {
		return BK_FAIL;
	}
	USB_DRIVER_LOGD("[-]%s thread_hdl:%x  msg_que:%x\r\n",__func__, s_usb_drv_thread_hdl, s_usb_drv_msg_que);

	return BK_OK;
}

static bk_err_t usb_driver_sw_deinit()
{
	USB_DRIVER_LOGD("[+]%s thread_hdl:%x  msg_que:%x\r\n",__func__, s_usb_drv_thread_hdl, s_usb_drv_msg_que);

	beken_mutex_t usb_drv_task_mutex = NULL;
	beken_queue_t usb_drv_msg_que = NULL;

	if(s_usb_drv_task_mutex) {
		usb_drv_task_mutex = s_usb_drv_task_mutex;
		s_usb_drv_task_mutex = NULL;
		rtos_deinit_mutex(&usb_drv_task_mutex);
	}

	if(s_usb_drv_msg_que) {
		usb_drv_msg_que = s_usb_drv_msg_que;
		s_usb_drv_msg_que = NULL;
		rtos_deinit_queue(&usb_drv_msg_que);
	}

	if (s_usb_drv_thread_hdl) {
		s_usb_drv_thread_hdl = NULL;
		rtos_delete_thread(NULL);
	}

	return BK_OK;
}

// Modified by TUYA Start
uint8_t is_usb_opened = 0;
// Modified by TUYA End

bk_err_t bk_usb_open(uint32_t usb_mode)
{
	USB_DRIVER_LOGD("[+]%s\r\n", __func__);
#if !CONFIG_USB_MAILBOX
	bk_pm_module_vote_cpu_freq(PM_DEV_ID_USB_1, PM_CPU_FRQ_480M);
	if((s_usbh_device_connect_flag != false) && (s_usb_driver_ops != NULL)) {
		for(E_USB_DEVICE_T dev = USB_UVC_DEVICE; dev < USB_DEVICE_MAX; dev++)
		{
			if(s_usb_driver_ops->usbh_connect_cb[dev]) {
				s_usb_driver_ops->usbh_class_connect_status |= (1 << dev);
				s_usb_driver_ops->usbh_connect_cb[dev]();
			}
		}
	}
#endif

#if CONFIG_SYS_CPU0
	bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_USB_1, 0x0, 0x0);
#endif
	USB_DRIVER_RETURN_NOT_INIT();;
	//USB_RETURN_NOT_POWERED_ON();
	USB_RETURN_NOT_CLOSED();
#if 0
	gpio_dev_unprotect_unmap(12);
	gpio_dev_unprotect_unmap(13);
	gpio_dev_unprotect_map(12, GPIO_DEV_USB0_DP);
	gpio_dev_unprotect_map(13, GPIO_DEV_USB0_DN);
#endif
	static uint32_t usb_ip_mode = USB_HOST_MODE;
	usb_ip_mode = usb_mode;

	if(usb_driver_sw_init() == BK_OK) {
		USB_DRIVER_LOGD("usb driver sw init OK\r\n");
	} else {
		USB_DRIVER_LOGE("usb driver sw init ERROR\r\n");
		return BK_FAIL;
	}

	bk_usb_drv_send_msg_front(USB_DRV_USB_OPEN, (void *)&usb_ip_mode);

	USB_DRIVER_LOGD("[-]%s\r\n", __func__);

// Modified by TUYA Start
    is_usb_opened = 1;
// Modified by TUYA End
	return BK_OK;
}

bk_err_t bk_usb_close(void)
{
	USB_DRIVER_RETURN_NOT_INIT();
	USB_RETURN_NOT_OPENED();
	if(s_usb_driver_ops)
		USB_RETURN_CLASS_IS_WORKING();

	bk_err_t ret = BK_OK;
	ret = bk_usb_drv_send_msg(USB_DRV_USB_CLOSE, NULL);

#if !CONFIG_USB_MAILBOX
	uint8_t wait_close_finish_count = 0;
	while(s_usb_open_close_flag) {
		if(wait_close_finish_count > 10)
		{
			USB_DRIVER_LOGE("[=]%s close timeout\r\n", __func__);
			ret = BK_FAIL;
			break;
		}
		rtos_delay_milliseconds(10);
		wait_close_finish_count++;
	}
#endif
// Modified by TUYA Start
    is_usb_opened = 0;
// Modified by TUYA End
	return ret;

}

bk_err_t bk_usb_register_connection_status_notification_callback(bk_usb_driver_comprehensive_ops *usb_driver,
																										E_USB_CONNECTION_STATUS status,
																										E_USB_DEVICE_T dev,
																										void *callback)
{
	if(status == USB_DEVICE_CONNECT) {
		usb_driver->usbh_connect_cb[dev] = (FUNCPTR)callback;
	}
	if(status == USB_DEVICE_DISCONNECT) {
		usb_driver->usbh_disconnect_cb[dev] = (FUNCPTR)callback;
	}

	return BK_OK;

}

bk_err_t bk_usb_control_transfer_init(void)
{
	USB_DRIVER_RETURN_NOT_INIT();
	USB_CONTROL_TRANSFER_RETURN_NOT_DEINIT();
	if(!s_usb_driver_ops)
		return BK_ERR_USB_OPERATION_NULL_POINTER;

	s_usbh_control_transfer_init_flag = 1;

	USB_DRIVER_LOGD("[+]%s \r\n",__func__);

	s_usb_driver_ops->control_transfer_irp = (bk_usb_control_irp *)os_malloc(sizeof(bk_usb_control_irp));
	if(!s_usb_driver_ops->control_transfer_irp) {
		USB_DRIVER_LOGE("%s control_transfer_pIrp malloc fail\r\n");
		return BK_FAIL;
	}

	bk_usb_control_irp *irp = s_usb_driver_ops->control_transfer_irp;

	irp->pOutBuffer = (uint8_t *)os_malloc(sizeof(uint8_t) * (64 + sizeof(s_usb_device_request)));
	if(!irp->pOutBuffer) {
		if(irp) {
			os_free(irp);
			irp = NULL;
		}
		USB_DRIVER_LOGE("%s outbuffer malloc fail\r\n",__func__);
		return BK_FAIL;
	}

	//irp->pDevice = s_usb_driver_ops->usb_device;
	USB_DRIVER_LOGD("[-]%s \r\n",__func__);

	return BK_OK;
}

bk_err_t bk_usb_control_transfer_deinit(void)
{
	USB_DRIVER_RETURN_NOT_INIT();
	USB_CONTROL_TRANSFER_RETURN_NOT_INIT();
	s_usbh_control_transfer_init_flag = 0;

	USB_DRIVER_LOGD("[+]%s \r\n",__func__);

	bk_usb_control_irp *irp = s_usb_driver_ops->control_transfer_irp;

	if(irp->pOutBuffer)
	{
		os_free(irp->pOutBuffer);
		irp->pOutBuffer = NULL;
	}

	if(irp)
	{
		os_free(irp);
		irp = NULL;
	}
	USB_DRIVER_LOGD("[-]%s \r\n",__func__);

	return BK_OK;
}

bk_err_t bk_usb_control_transfer(s_usb_device_request *pSetup, s_usb_transfer_buffer_info *buffer_info)
{
	USB_DRIVER_RETURN_NOT_INIT();
	USB_CONTROL_TRANSFER_RETURN_NOT_INIT();

	bk_usb_control_irp *irp = s_usb_driver_ops->control_transfer_irp;

	if(pSetup != NULL)
		os_memcpy((void *)irp->pOutBuffer, pSetup, sizeof(s_usb_device_request));
	else
		return BK_FAIL;

	if(buffer_info->pOutBuffer != NULL)
		os_memcpy((void *)(irp->pOutBuffer + sizeof(s_usb_device_request)), buffer_info->pOutBuffer, buffer_info->dwOutLength);

    irp->dwOutLength = buffer_info->dwOutLength + sizeof(s_usb_device_request);
    irp->pInBuffer = buffer_info->pInBuffer;
    irp->dwInLength = buffer_info->dwInLength;
    irp->dwStatus = 0;
    irp->dwActualOutLength = 0;
    irp->dwActualInLength = 0;
    irp->pfIrpComplete = (bk_usb_control_irp_complete)buffer_info->pfIrpComplete;

	return (uint32_t)irp;
}

bk_err_t bk_usb_check_device_supported (E_USB_DEVICE_T usb_dev)
{
	if(!bk_usb_get_device_connect_status()) return BK_FAIL;

	uint32_t ret = BK_FAIL;
	switch(usb_dev)
	{
#if CONFIG_USB_UAC
		case USB_UVC_DEVICE:
			if(s_usb_driver_ops->usbh_class_connect_status & (0x1 << USB_UVC_DEVICE))
				ret = BK_OK;
			else
				ret = BK_ERR_USB_UVC_NOSUPPORT_ATTRIBUTE;
			break;
#endif

#if CONFIG_USB_UAC
		case USB_UAC_MIC_DEVICE:
			if(s_usb_driver_ops->usbh_class_connect_status & (0x1 << USB_UAC_MIC_DEVICE))
				ret = BK_OK;
			else
				ret = BK_ERR_USB_UAC_NOSUPPORT_ATTRIBUTE;
			break;
		case USB_UAC_SPEAKER_DEVICE:
			if(s_usb_driver_ops->usbh_class_connect_status & (0x1 << USB_UAC_SPEAKER_DEVICE))
				ret = BK_OK;
			else
				ret = BK_ERR_USB_UAC_NOSUPPORT_ATTRIBUTE;
			break;
#endif
		default:
			break;
	}

	return ret;
}

bk_err_t bk_usb_get_device_descriptor(struct usb_device_descriptor_t *dev_descriptor)
{
	USB_DRIVER_LOGD("[+]%s \r\n",__func__);
	if(!s_usb_driver_ops->hport)
		return BK_FAIL;
	struct usb_device_descriptor_t *descriptor = (struct usb_device_descriptor_t *)&s_usb_driver_ops->hport->device_desc;
	memcpy(dev_descriptor, descriptor, descriptor->bLength);
	USB_DRIVER_LOGD("[-]%s \r\n",__func__);
	return BK_OK;
}

bool bk_usb_get_device_connect_status(void)
{
    return s_usbh_device_connect_flag;
}

