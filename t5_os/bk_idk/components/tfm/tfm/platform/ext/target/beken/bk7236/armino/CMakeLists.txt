string(REGEX REPLACE "beken/" "" SOC "${TFM_PLATFORM}")

set(incs
        config/
        driver/
	${TFM_BEKEN}/armino/driver
        driver/include/bk_private
        driver/common
        driver/include
        ${TFM_BEKEN}/armino/driver/uart
        ${TFM_BEKEN}/armino/driver/gpio
        ${TFM_BEKEN}/armino/driver/sys_ctrl
        ${TFM_BEKEN}/armino/driver/flash
        ${TFM_BEKEN}/armino/driver/spinlock
        ${TFM_BEKEN}/armino/driver/timer
        ${TFM_BEKEN}/armino/driver/efuse
        driver/hal
        ${TFM_BEKEN}/armino/include/
        ../../../../../../secure_fw/spm/include
        ../../../../../../secure_fw/include
        ../../../../../../secure_fw/spm/include/interface
)

set(srcs
    ${TFM_BEKEN}/armino/driver/port/os_port.c
    ${TFM_BEKEN}/armino/driver/port/mem_port.c
    ${TFM_BEKEN}/armino/driver/sys_ctrl/sys_driver.c
    ${TFM_BEKEN}/armino/driver/sys_ctrl/sys_driver_common.c
    ${TFM_BEKEN}/armino/driver/sys_ctrl/sys_clock_driver.c
    ${TFM_BEKEN}/armino/driver/sys_ctrl/sys_int_driver.c
    ${TFM_BEKEN}/armino/driver/uart/uart_driver.c
    ${TFM_BEKEN}/armino/driver/uart/uart_statis.c
    ${TFM_BEKEN}/armino/driver/gpio/gpio_driver_base.c
    ${TFM_BEKEN}/armino/driver/gpio/gpio_driver.c
    ${TFM_BEKEN}/armino/driver/spinlock/amp_res_lock.c
    ${TFM_BEKEN}/armino/driver/flash/flash_driver.c
    ${TFM_BEKEN}/armino/driver/timer/timer_driver.c
    ${TFM_BEKEN}/armino/driver/ppc/prro_driver.c
    ${TFM_BEKEN}/armino/driver/efuse/efuse_driver.c
    ${TFM_BEKEN}/armino/driver/bk_efuse.c
    ./driver/hal/prro_hal_v5.c
    ./driver/hal/uart_hal.c
    ./driver/hal/timer_hal.c
    ./driver/hal/sys_hal.c
    ./driver/hal/aon_pmu_hal.c
    ./driver/hal/gpio_hal.c
    ./driver/hal/uart_hal_debug.c
    ./driver/hal/gpio_hal_debug.c
    ./driver/hal/flash_hal_debug.c
    ./driver/hal/flash_hal.c
    ./driver/hal/uart_ll.c
    ./driver/hal/timer_ll.c
    ./driver/hal/gpio_ll.c
    ./driver/hal/flash_ll.c
    ./driver/hal/efuse_hal.c
    ./driver/hal/wdt_hal.c
)

if(CONFIG_BUILD_TFM)
set(s_srcs
        ${TFM_BEKEN}/armino/driver/bk_tfm_mpc.c
        ${TFM_BEKEN}/armino/driver/bk_tfm_log.c
        ${TFM_BEKEN}/armino/driver/mpc/mpc_driver.c
        driver/hal/mpc_hal.c
        driver/hal/mpc_hal_debug.c
)

set(s_incs
        driver
        ${TFM_BEKEN}/armino/driver/mpc
        ${TFM_BEKEN}/armino/driver/ppc
)

if(TFM_AON_PMU_LL_NSC)
    list(APPEND s_srcs "./driver/hal/aon_pmu_ll_nsc.c")
endif()

if(TFM_SYS_LL_NSC)
    list(APPEND s_srcs "./driver/hal/sys_ll_nsc.c")
endif()

if(CONFIG_REG_ACCESS_NSC)
    list(APPEND s_srcs "./driver/hal/reg_nsc.c")

    target_compile_definitions(platform_s
        PUBLIC
        CONFIG_REG_ACCESS_NSC
    )
endif()

if(CONFIG_DUBHE_KEY_LADDER_NSC)
    list(APPEND s_srcs "./driver/hal/dubhe_key_ladder_nsc.c")

    target_compile_definitions(platform_s
        PUBLIC
        CONFIG_DUBHE_KEY_LADDER_NSC
    )
endif()

if(CONFIG_MPC_NSC)
    list(APPEND s_srcs "./driver/hal/mpc_nsc.c")

    target_compile_definitions(platform_s
        PUBLIC
        CONFIG_MPC_NSC
    )
endif()

if(CONFIG_PM_NSC)
    list(APPEND s_srcs "./driver/hal/pm_nsc.c")

    target_compile_definitions(platform_s
        PUBLIC
        CONFIG_PM_NSC
    )
endif()

if(CONFIG_FLASH_NSC)
    list(APPEND s_srcs "./driver/hal/flash_nsc.c")

    target_compile_definitions(platform_s
        PUBLIC
        CONFIG_FLASH_NSC
    )
endif()

if(CONFIG_TFM_READ_FLASH_NSC)
    list(APPEND s_srcs "./driver/hal/flash_nsc.c")

    target_compile_definitions(platform_s
        PUBLIC
        CONFIG_TFM_READ_FLASH_NSC
    )
endif()

if(CONFIG_OTP_NSC)
    list(APPEND s_srcs "./driver/hal/otp_nsc.c")

    target_compile_definitions(platform_s
        PUBLIC
        CONFIG_OTP_NSC
    )
endif()

if(CONFIG_INT_TARGET_NSC)
    list(APPEND s_srcs "./driver/hal/target_nsc.c")

    target_compile_definitions(platform_s
        PUBLIC
        CONFIG_INT_TARGET_NSC
    )
endif()
endif()

if(CONFIG_AES_GCM_NSC)
    list(APPEND s_srcs "./driver/hal/aes_gcm_nsc.c")

    target_compile_definitions(platform_s
        PUBLIC
        CONFIG_AES_GCM_NSC
    )
endif()

if(CONFIG_TFM_PANIC_DEAD_LOOP)
    target_compile_definitions(platform_s PUBLIC CONFIG_PANIC_DEAD_LOOP)
endif()

target_include_directories(platform_s PUBLIC ${incs} ${s_incs})
target_sources(platform_s PRIVATE ${srcs} ${s_srcs})
target_compile_options(platform_s
    PUBLIC
        ${COMPILER_CP_FLAG}
)

target_include_directories(platform_ns PUBLIC ${incs})
target_sources(platform_ns PRIVATE ${srcs})
target_compile_options(platform_ns
    PUBLIC
        ${COMPILER_CP_FLAG}
)

if(CONFIG_TFM_BK7236_V5)
    target_compile_definitions(platform_s PUBLIC CONFIG_TFM_BK7236_V5)
    target_compile_definitions(platform_ns PUBLIC CONFIG_TFM_BK7236_V5)
    target_compile_definitions(platform_bl2 PUBLIC CONFIG_TFM_BK7236_V5)
endif()

if(CONFIG_TFM_MPU)
    target_compile_definitions(platform_s PUBLIC CONFIG_TFM_MPU)
    target_compile_definitions(platform_ns PUBLIC CONFIG_TFM_MPU)
endif()

if(TEST_S)
    target_compile_definitions(platform_s
        PUBLIC
        CONFIG_ENABLE_TEST_S
    )
endif()

if(TEST_NS)
    target_compile_definitions(platform_ns
        PUBLIC
        CONFIG_ENABLE_TEST_NS
    )
endif()

if(BL2)
    set(bl2_incs driver)
    target_include_directories(platform_bl2 PUBLIC ${incs} ${bl2_incs})
    target_sources(platform_bl2 PRIVATE ${srcs} ${bl2_srcs})
    target_compile_options(platform_bl2 PUBLIC ${BL2_COMPILER_CP_FLAG})
endif()

if(NS)
    #TODO wangzhilei
    target_compile_definitions(platform_ns
        PUBLIC
        CONFIG_TFM_NS
        DOMAIN_NS
    )
endif()

if(CONFIG_XIP_NO_VERSION)
    target_compile_definitions(platform_s
        PUBLIC
            CONFIG_XIP_NO_VERSION
    )
endif()
