if (BL2_DOWNLOAD)
	add_library(download
		bl_bk_hci_protocol.c
		bl_extflash.c
		bl_spi.c
		bl_tra_hcit.c
		bl_uart.c
		bootloader.c
		crc32.c
		system.c
	)

	string(REGEX REPLACE "beken/" "" SOC "${TFM_PLATFORM}")
	set(download_inc
		.
		./inc
		./inc/pal
		./hal/inc
		${TFM_BEKEN}/${TFM_SOC}/armino/driver/hal
		../${TFM_SOC}/cmsis_core
		${TFM_BEKEN}/retarget
		${TFM_BEKEN}/partition
		../armino/include
		../${TFM_SOC}/armino/config
		./../../../cmsis/
		${ARMINO_MIDDLEWARE_REPO_PATH}/boards/${SOC}/partitions
		${PARTITION_HDR_DIR}

	)

	target_compile_definitions(download
		PUBLIC
			CONFIG_BL2_DOWNLOAD
	)

	if(CONFIG_BL2_UPDATE_WITH_PC)
		target_compile_definitions(download
			PUBLIC
				CONFIG_BL2_UPDATE_WITH_PC
		)
	endif()

	if (CONFIG_RANDOM_AES_UPGRADE_BL2 STREQUAL "y")
		target_compile_definitions(download
			PUBLIC
				CONFIG_RANDOM_AES_UPGRADE_BL2
		)
	endif()

	target_compile_options(download PUBLIC ${BL2_COMPILER_CP_FLAG})
	target_include_directories(download PRIVATE ${download_inc})
	target_link_libraries(platform_bl2 PUBLIC download)
	link_directories(${CMAKE_CURRENT_BINARY_DIR})
endif()


