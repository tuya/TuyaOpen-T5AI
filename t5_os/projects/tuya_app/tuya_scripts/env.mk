#
# Makefile
# cc, 2023-06-29 20:38
#

# include ${TUYA_PROJECT_DIR}/apps/${APP_BIN_NAME}/components.mk
# 
VENDOR_DIR := $(dir $(lastword $(MAKEFILE_LIST)))../../../..
# 
# TUYA_COMPONENTS_DESC=${VENDOR_DIR}/t5_os/projects/tuya_app/.tmp_component_desc
# tuya_components:
# 	@ echo "Automatically generated file. DO NOT EDIT." > ${TUYA_COMPONENTS_DESC};
# 	@ echo "COMPONENT apps/$${APP}" >> ${TUYA_COMPONENTS_DESC};
# 	@ for c in ${COMPONENTS}; do \
# 		echo "COMPONENT ${COMPONENTS_PATH}/$${c}" >> ${TUYA_COMPONENTS_DESC}; \
# 		done
# 	@ for c in ${COMPONENTS_LIB}; do \
# 		echo "COMPONENT_LIB ${COMPONENTS_PATH}/$${c}" >> ${TUYA_COMPONENTS_DESC}; \
# 		done
# 	@ cat ${TUYA_COMPONENTS_DESC}

# lwip_dir = ${VENDOR_DIR}/tuyaos/tuyaos_adapter/lwip_intf_v2_1
lwip_dir = ${VENDOR_DIR}/t5_os/bk_idk/components/lwip_intf_v2_1
lwip_cmake_file = CMakeLists.txt
tuya_lwip:
	# @ if [ ! -d ${lwip_dir} ]; then mkdir -p ${lwip_dir}; echo "mkdir ${lwip_dir}"; fi
	@ mv ${lwip_dir}/CMakeLists.txt ${lwip_dir}/CMakeLists.txt.backup
	@ echo "" > ${lwip_dir}/${lwip_cmake_file}
	@ echo "set(incs)" >> ${lwip_dir}/${lwip_cmake_file}
	@ echo "set(srcs)" >> ${lwip_dir}/${lwip_cmake_file}
	@ echo "" >> ${lwip_dir}/${lwip_cmake_file}
	@ echo "list(APPEND incs" 																	>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "    \$$ENV{TUYA_PROJECT_DIR}/include/base/include" 									>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "    \$$ENV{TUYA_PROJECT_DIR}/include/components/tal_lwip/include" 					>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "    \$$ENV{TUYA_PROJECT_DIR}/include/components/tal_lwip/include/lwip" 				>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "    \$$ENV{TUYA_PROJECT_DIR}/include/components/tal_lwip/include/netif" 			>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "    \$$ENV{TUYA_PROJECT_DIR}/include/components/tal_lwip/include/compat" 			>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "    \$$ENV{TUYA_PROJECT_DIR}/vendor/T5/tuyaos/tuyaos_adapter/include/lwip" 			>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "    \$$ENV{TUYA_PROJECT_DIR}/vendor/T5/tuyaos/tuyaos_adapter/bk_extension/include" 	>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "    dhcpd" 																			>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "    lwip-2.1.2/port" 																>> ${lwip_dir}/${lwip_cmake_file}
	@ echo ")" 																					>> ${lwip_dir}/${lwip_cmake_file}
	@ echo ""  																					>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "if (CONFIG_LWIP_V2_1)" 																>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "list(APPEND srcs" 																	>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "    lwip-2.1.2/port/tuya_net.c" 													>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "    lwip-2.1.2/port/wlanif.c" 														>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "    lwip-2.1.2/port/ethernetif.c" 													>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "    )" 																				>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "" 																					>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "if (CONFIG_SYS_CPU0)" 																>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "list(APPEND srcs" 																	>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "    lwip-2.1.2/src/core/ipv4/ip4_addr.c" 											>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "    lwip-2.1.2/src/core/def.c" 														>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "    lwip-2.1.2/src/core/pbuf.c" 													>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "    lwip-2.1.2/port/tuya_sys_arch.c" 												>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "    lwip-2.1.2/src/core/mem.c" 														>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "    lwip-2.1.2/src/core/memp.c" 													>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "    )" 																				>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "endif() # CONFIG_SYS_CPU0" 															>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "" 																					>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "else()" 																			>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "" 																					>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "if (CONFIG_SYS_CPU1)" 																>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "list(APPEND srcs" 																	>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "    dhcpd/dhcp-server.c" 															>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "    dhcpd/dhcp-server-main.c" 													>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "    lwip-2.1.2/src/core/disable_wifi.c" 											>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "    )" 																				>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "endif() # CONFIG_SYS_CPU1" 															>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "" 																					>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "endif() # CONFIG_LWIP_V2_1" 														>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "" 																					>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "armino_component_register(" 														>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "    SRCS \"\$${srcs}\"" 															>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "    INCLUDE_DIRS \"\$${incs}\"" 													>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "    REQUIRES driver" 																>> ${lwip_dir}/${lwip_cmake_file}
	@ echo "    PRIV_REQUIRES bk_common bk_wifi bk_rtos os_source tuyaos_adapter" 				>> ${lwip_dir}/${lwip_cmake_file}
	@ echo ")" 																					>> ${lwip_dir}/${lwip_cmake_file}
	@ cd -

# vim:ft=make
#
