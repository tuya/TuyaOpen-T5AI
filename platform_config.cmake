list_subdirectories(PLATFORM_PUBINC_1 ${PLATFORM_PATH}/tuyaos/tuyaos_adapter)

# [交付仓 T5AI 接入] 类型头优先：本平台 tuyaos_adapter/include/utilities/include 的
# tuya_cloud_types.h **缺 TAL 系别名**（INT_T / VOID_T / CHAR_T —— OPERATE_RET/BOOL_T 有），
# core 与各组件都依赖它们；由 tuyaopen/include/common 副本补齐并置于最前，
# 使其优先命中（与 ESP32 平台同口径）。
list(INSERT PLATFORM_PUBINC_1 0 "${CMAKE_CURRENT_LIST_DIR}/tuyaos/tuyaos_adapter/tuyaopen/include/common")

set(PLATFORM_PUBINC_2 
    ${PLATFORM_PATH}/t5_os/ap/include
    ${PLATFORM_PATH}/t5_os/build/bk7258/tuya_app/config
    ${PLATFORM_PATH}/t5_os/ap/components/lwip_intf_v2_1/lwip-2.1.2
)

# [tuyaopen-core integration] 契约层（C2，与 LINUX/ESP32 同构）：core 与平台共用的
# ops 类型放在 tuyaos/tuyaos_adapter/tuyaopen/interface/；core 的 tal_*.h 反向 include
# 它们（经 PLATFORM_PUBINC → tuyaopen-headers 注入 core 编译单元）。
# ops 卡注册见 tuyaopen/tuyaopen-core_ops.c（薄卡，平台实现文件零改动）。
list(APPEND PLATFORM_PUBINC_1 "${PLATFORM_PATH}/tuyaos/tuyaos_adapter/tuyaopen/interface")

set(PLATFORM_PUBINC 
    ${PLATFORM_PUBINC_1}
    ${PLATFORM_PUBINC_2})