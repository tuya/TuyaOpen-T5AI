list_subdirectories(PLATFORM_PUBINC_1 ${PLATFORM_PATH}/tuyaos/tuyaos_adapter)

set(PLATFORM_PUBINC_2 
    ${PLATFORM_PATH}/t5_os/bk_idk/include
    ${PLATFORM_PATH}/t5_os/build/bk7258_cp1/config
)

set(PLATFORM_PUBINC 
    ${PLATFORM_PUBINC_1}
    ${PLATFORM_PUBINC_2})