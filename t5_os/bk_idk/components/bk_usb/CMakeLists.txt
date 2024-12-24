set(incs)
set(srcs)

if (CONFIG_USB)

if (CONFIG_CHERRY_USB)
	list(APPEND incs
		CherryUSB/
		CherryUSB/common
		CherryUSB/core
		CherryUSB/osal
		CherryUSB/class/cdc
		CherryUSB/class/hid
		CherryUSB/class/msc
		CherryUSB/class/audio
		CherryUSB/class/video
		CherryUSB/class/hub
		CherryUSB/demo
		CherryUSB/driver
	)
	list(APPEND srcs
		CherryUSB/driver/usb_driver.c
		CherryUSB/osal/usb_osal_adaptor.c
	)
	if (CONFIG_USB_DEVICE)
		list(APPEND srcs
			CherryUSB/core/usbd_core.c
		)

		if (CONFIG_MUSB_MHDRC)
			list(APPEND srcs
				CherryUSB/port/beken_musb/usb_dc_beken_musb_mhdrc.c
			)
		else()
			list(APPEND srcs
				CherryUSB/port/beken_musb/usb_dc_beken_musb_fdrc.c
			)
		endif()

		list(APPEND incs
			CherryUSB/demo/beken/usb_device
		)

		if (CONFIG_USB_UVC)
			list(APPEND srcs
				CherryUSB/class/video/usbd_video.c
				CherryUSB/demo/beken/usb_device/video/video_stream_common.c
			)
		endif() # CONFIG_USB_UVC

		if (CONFIG_USB_HID)
			list(APPEND srcs
				#CherryUSB/demo/beken/usb_device/hid/hid_common.c
				CherryUSB/demo/beken/usb_device/hid/hid_mouse_lpm.c
				CherryUSB/class/hid/usbd_hid.c
			)
		endif() # CONFIG_USB_HID
	endif() # CONFIG_USB_DEVICE

	if (CONFIG_USB_HOST)
		list(APPEND srcs
			CherryUSB/core/usbh_core.c
		)

		list(APPEND incs
			CherryUSB/demo/beken/usb_host
		)
		if (CONFIG_MUSB_FDRC OR CONFIG_MUSB_MHDRC)
			list(APPEND srcs
				CherryUSB/port/beken_musb/usb_hc_beken_musb.c
			)
		endif() # CONFIG_MENTOR_USB_IP

		if (CONFIG_USB_HUB)
			list(APPEND srcs
				CherryUSB/class/hub/usbh_hub.c
			)
		endif() # CONFIG_USB_UVC

		if (CONFIG_USB_UVC)
			list(APPEND srcs
				CherryUSB/class/video/usbh_video.c
			)
		endif() # CONFIG_USB_UVC

		if (CONFIG_USB_UAC)
			list(APPEND srcs
				CherryUSB/class/audio/usbh_audio.c
			)
		endif() # CONFIG_USB_UAC

		if (CONFIG_USB_HID)
			list(APPEND srcs

			)
		endif() # CONFIG_USB_HID

		if (CONFIG_USB_MSD)
			list(APPEND srcs
				CherryUSB/class/msc/usbh_msc.c
			)
		endif() # CONFIG_USB_MSD
	endif() # CONFIG_USB_HOST
endif() # CONFIG_CHERRY_USB

if (CONFIG_MENTOR_USB)
	list(APPEND incs
		.
		MentorUSB/include
		MentorUSB/include/class
		MentorUSB/driver
		MentorUSB/driver/class/audio
		MentorUSB/driver/class/video
	)
	list(APPEND srcs
		bk_usb_adaptor.c
		MentorUSB/driver/usb_driver.c
	)

	if (CONFIG_USB_HOST)
		if (CONFIG_USB_UVC)
			list(APPEND srcs
				MentorUSB/driver/class/video/usbh_video.c
			)
		endif() # CONFIG_USB_UVC

		if (CONFIG_USB_UAC)
			list(APPEND srcs
				MentorUSB/driver/class/audio/usbh_audio.c
			)
		endif() # CONFIG_USB_UAC
	endif() # CONFIG_USB_HOST

endif() # CONFIG_MENTOR_USB

if (CONFIG_UVC_OTA_DEMO)
	list(APPEND incs
		example/uvc_ota
		)

	list(APPEND srcs
		example/uvc_ota/uvc_ota_demo.c
		)
endif() # CONFIG_UVC_OTA_DEMO

if (CONFIG_UVC_UAC_DEMO)
	list(APPEND incs
		example/solution_uvc_uac
		)

	if (CONFIG_CHERRY_USB)
		list(APPEND srcs
			example/solution_uvc_uac/bk_cherry_uvc_uac_api.c
		)
	endif() # CONFIG_CHERRY_USB
	
	if (CONFIG_MENTOR_USB)
		list(APPEND srcs
			example/solution_uvc_uac/bk_mentor_uvc_uac_api.c
		)
	endif() # CONFIG_MENTOR_USB

endif() # CONFIG_UVC_UAC_DEMO
endif()

if (CONFIG_USB_MAILBOX)
	list(APPEND incs
		example/usb_mailbox
		)

	if (CONFIG_USB_MAILBOX_MASTER)
		list(APPEND srcs
			example/usb_mailbox/bk_usb_mailbox_master.c
		)
	endif() # CONFIG_USB_MAILBOX_MASTER

	if (CONFIG_USB_MAILBOX_SLAVE)
		list(APPEND srcs
			example/usb_mailbox/bk_usb_mailbox_slave.c
		)
	endif() # CONFIG_USB_MAILBOX_SLAVE

endif() # CONFIG_USB_MAILBOX

if (CONFIG_ATE_TEST)
	list(APPEND incs
		example/ate_test
		)

	list(APPEND srcs
		example/ate_test/ate_test.c
		)
endif() # CONFIG_ATE

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common driver)

if (CONFIG_MUSB_MHDRC)
target_compile_definitions(${COMPONENT_LIB}
	PUBLIC
		CONFIG_USB_HS
)
endif()