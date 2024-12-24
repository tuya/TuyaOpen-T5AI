set(incs)
set(srcs)

if (CONFIG_CMSIS)
	list(APPEND incs
		CMSIS_5/CMSIS/Core/Include
	)

if (CONFIG_SOC_BK7236XX)
if (CONFIG_SOC_SMP)
	list(APPEND srcs
		CMSIS_5/Device/Beken/bk7236xx/Source/smp/startup_cpu0.c
		CMSIS_5/Device/Beken/bk7236xx/Source/smp/startup_cpu1.c
		CMSIS_5/Device/Beken/bk7236xx/Source/smp/startup_cpu2.c
		CMSIS_5/Device/Beken/bk7236xx/Source/smp/system_cpu0.c
		CMSIS_5/Device/Beken/bk7236xx/Source/smp/system_cpu1.c
		CMSIS_5/Device/Beken/bk7236xx/Source/smp/system_cpu2.c
		CMSIS_5/Device/Beken/bk7236xx/Source/smp/smp.c
	)
else()
	list(APPEND srcs
		CMSIS_5/Device/Beken/bk7236xx/Source/startup_bk7236.c
		CMSIS_5/Device/Beken/bk7236xx/Source/system_bk7236xx.c
	)
endif()

	list(APPEND incs
		CMSIS_5/Device/Beken/bk7236xx/Include
		CMSIS_5/Device/Beken/bk7236xx/Include/Template
		CMSIS_5/Device/Beken/armstar
	)
endif()

if (CONFIG_SOC_BK7239XX)
if (CONFIG_SOC_BK7239_SMP)
	list(APPEND srcs
		CMSIS_5/Device/Beken/bk7239xx/Source/smp/startup_cpu0.c
		CMSIS_5/Device/Beken/bk7239xx/Source/smp/startup_cpu1.c
		CMSIS_5/Device/Beken/bk7239xx/Source/smp/startup_cpu2.c
		CMSIS_5/Device/Beken/bk7239xx/Source/smp/system_cpu0.c
		CMSIS_5/Device/Beken/bk7239xx/Source/smp/system_cpu1.c
		CMSIS_5/Device/Beken/bk7239xx/Source/smp/system_cpu2.c
		CMSIS_5/Device/Beken/bk7239xx/Source/smp/smp.c
	)
else()
	list(APPEND srcs
		CMSIS_5/Device/Beken/bk7239xx/Source/startup_bk7239.c
		CMSIS_5/Device/Beken/bk7239xx/Source/system_bk7239xx.c
	)
endif()

	list(APPEND incs
		CMSIS_5/Device/Beken/armstar
		CMSIS_5/Device/Beken/bk7239xx/Include
		CMSIS_5/Device/Beken/bk7239xx/Include/Template
	)
endif()

if (CONFIG_SOC_BK7286XX)
if (CONFIG_SOC_BK7286_SMP)
	list(APPEND srcs
		CMSIS_5/Device/Beken/bk7286xx/Source/smp/startup_cpu0.c
		CMSIS_5/Device/Beken/bk7286xx/Source/smp/startup_cpu1.c
		CMSIS_5/Device/Beken/bk7286xx/Source/smp/startup_cpu2.c
		CMSIS_5/Device/Beken/bk7286xx/Source/smp/system_cpu0.c
		CMSIS_5/Device/Beken/bk7286xx/Source/smp/system_cpu1.c
		CMSIS_5/Device/Beken/bk7286xx/Source/smp/system_cpu2.c
		CMSIS_5/Device/Beken/bk7286xx/Source/smp/smp.c
	)
else()
	list(APPEND srcs
		CMSIS_5/Device/Beken/bk7286xx/Source/startup_bk7286.c
		CMSIS_5/Device/Beken/bk7286xx/Source/system_bk7286xx.c
	)
endif()

if (CONFIG_SOC_BK7286XX)
	list(APPEND srcs
		CMSIS_5/Device/Beken/bk7286xx/Source/fpga_bk7286.c
	)
endif()

	list(APPEND incs
		CMSIS_5/Device/Beken/armstar
		CMSIS_5/Device/Beken/bk7286xx/Include
		CMSIS_5/Device/Beken/bk7286xx/Include/Template
	)
endif()
endif()

if (CONFIG_SOC_SMP)
armino_component_register(SRCS "${srcs}"
			INCLUDE_DIRS "${incs}"
			PRIV_REQUIRES bk_common coredump bk_rtos)
else()
armino_component_register(SRCS "${srcs}"
			INCLUDE_DIRS "${incs}"
			PRIV_REQUIRES bk_common coredump)
endif()

add_definitions(-D__ARM_ARCH_8M_MAIN__=1)
