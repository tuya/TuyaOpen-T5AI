set(incs
	camera
	spidma
	video_transfer
	capture
	)
set(srcs)

if (NOT CONFIG_SOC_BK7258) #NOT CONFIG_SOC_BK7258
if (CONFIG_DVP_CAMERA)
	list(APPEND srcs
		"camera/dvp_camera_driver.c"
		)
endif()

if (CONFIG_USB_UVC)
	list(APPEND srcs
		#"camera/uvc_camera_driver.c"
		#"uvc_display/uvc_display.c"
		)
endif()

if (CONFIG_SPIDMA)
	list(APPEND srcs
		"spidma/spidma_driver.c"
		)
endif()

if (CONFIG_DVP_CAMERA OR CONFIG_USB_UVC OR CONFIG_SPIDMA)
	list(APPEND srcs
		"video_transfer/video_transfer.c"
		"video_transfer/video_buffer.c"
		"dvp_display/dvp_display.c"
		)

	if (CONFIG_DVP_CAMERA_TEST)
			list(APPEND srcs
			"capture/capture.c"
			)
	endif()
endif()
endif() #NOT CONFIG_SOC_BK7258

if (CONFIG_SOC_BK7258) #CONFIG_SOC_BK7258

if (CONFIG_SYS_CPU1)
	if (CONFIG_DVP_CAMERA)
		list(APPEND srcs
			"camera/dvp_camera_major_driver.c"
			)
	endif()
endif()

if (CONFIG_SYS_CPU0)
	list(APPEND srcs
		"camera/dvp_camera_app_driver.c"
		)
endif()

if (CONFIG_USB_UVC)
	list(APPEND srcs
		#"camera/uvc_camera_driver.c"
		#"uvc_display/uvc_display.c"
		)
endif()

if (CONFIG_SPIDMA)
	list(APPEND srcs
		"spidma/spidma_driver.c"
		)
endif()

# if (CONFIG_SYS_CPU0)
# 	list(APPEND srcs
# 		"video_transfer/video_transfer.c"
# 		"video_transfer/video_buffer.c"
# 		"dvp_display/dvp_display.c"
# 		)
# 
# 	if (CONFIG_DVP_CAMERA_TEST)
# 			list(APPEND srcs
# 			"capture/capture.c"
# 			)
# 	endif()
# endif()

endif() #CONFIG_SOC_BK7258

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common bk_log media multimedia)
