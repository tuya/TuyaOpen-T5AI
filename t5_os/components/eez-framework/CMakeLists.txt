
set(tuya_build $ENV{TUYA_APP_NAME})
if (NOT ${tuya_build} STREQUAL "")
if (CONFIG_SOC_BK7258) # CONFIG_SOC_BK7258
if (CONFIG_LVGL)
if (CONFIG_SYS_CPU1)
file(GLOB_RECURSE c_srcs src/eez/*.cpp)
file(GLOB_RECURSE cpp_srcs src/eez/*.c)
set(srcs
	${c_srcs}
	${cpp_srcs}
)

if (CONFIG_SYS_CPU1)
#message(STATUS "====================eez-framework cp1 build========================")
#message(STATUS ${srcs})
#message(STATUS "============================================")
else()
#message(STATUS "===================eez-framework cp0 build=========================")
endif()

set(incs 
	./src
	./src/eez/libs/agg
)
set(priv_req bk_common)
endif()
endif()
	
armino_component_register(
	SRCS "${srcs}"
	INCLUDE_DIRS "${incs}" 
	PRIV_REQUIRES lvgl
)

if (CONFIG_SYS_CPU1)
target_compile_options(${COMPONENT_LIB} PRIVATE -DEEZ_FOR_LVGL -DLV_LVGL_H_INCLUDE_SIMPLE )
endif()
endif()
endif()