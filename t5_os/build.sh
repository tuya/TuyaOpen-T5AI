#!/bin/sh

# Modified by TUYA Start
APP_BIN_NAME=$1
APP_VERSION=$2
TARGET_PLATFORM=$3
APP_PATH=../../../$4
USER_CMD=$5

TARGET_PLATFORM=bk7258

echo APP_BIN_NAME=$APP_BIN_NAME
echo APP_VERSION=$APP_VERSION
echo TARGET_PLATFORM=$TARGET_PLATFORM
echo APP_PATH=$APP_PATH
echo USER_CMD=$USER_CMD

export TUYA_APP_PATH=$APP_PATH
export TUYA_APP_NAME=$APP_BIN_NAME

p=$(pwd);p1=${p%%/vendor*};echo $p1
export TUYA_PROJECT_DIR=$p1

USER_SW_VER=`echo $APP_VERSION | cut -d'-' -f1`

APP_DIR_temp=$(echo $APP_PROJ_PATH)
if [ "x$APP_DIR_temp" != "x" ];then
    last_character=$(echo -n $APP_PROJ_PATH | tail -c 1)
    if [ "x$last_character" = 'x/' ];then
        APP_DIR_temp=${APP_PROJ_PATH%?}
    else
        APP_DIR_temp=$APP_PROJ_PATH
    fi
    APP_DIR=${APP_DIR_temp%/*}
else
APP_DIR=apps
fi

APP_PATH=../../$APP_DIR

# Remove TUYA APP OBJs first
if [ -e "${APP_OBJ_PATH}/$APP_BIN_NAME/src" ]; then
for i in `find ${APP_OBJ_PATH}/$APP_BIN_NAME/src -type d`; do
    echo "Deleting $i"
    rm -rf $i/*.o
done
fi

tmp_gen_files_list=bk_idk/tools/build_tools/part_table_tools/config/gen_files_list.txt
if [ -f $tmp_gen_files_list ]; then
    rm $tmp_gen_files_list
fi
touch $tmp_gen_files_list

if [ -z $CI_PACKAGE_PATH ]; then
    echo "not is ci build"
else
    make clean
	make clean -C ./bk_idk/
fi

if [ x$USER_CMD = "xclean" ];then
    # save sdkconfig.h
    mkdir -p .tmp_build/bk7258_cp1/armino_as_lib/bk7258_cp1/config/
    mkdir -p .tmp_build/bk7258_cp1/config
    mkdir -p .tmp_build/bk7258/armino_as_lib/bk7258/config
    mkdir -p .tmp_build/bk7258/config

    cp ./build/bk7258_cp1/armino_as_lib/bk7258_cp1/config/sdkconfig.h .tmp_build/bk7258_cp1/armino_as_lib/bk7258_cp1/config/
    cp ./build/bk7258_cp1/config/sdkconfig.h .tmp_build/bk7258_cp1/config
    cp ./build/bk7258/armino_as_lib/bk7258/config/sdkconfig.h .tmp_build/bk7258/armino_as_lib/bk7258/config
    cp ./build/bk7258/config/sdkconfig.h .tmp_build/bk7258/config

	make clean
	make clean -C ./bk_idk/
    cp -a .tmp_build build
    rm -rf .tmp_build

	echo "*************************************************************************"
	echo "************************CLEAN SUCCESS************************************"
	echo "*************************************************************************"
	exit 0
fi

# lwip check
sdk_config_file=${TUYA_PROJECT_DIR}/include/base/include/tuya_iot_config.h
if [ ! -f ${sdk_config_file} ]; then
    echo "${sdk_config_file} is not exist"
    exit -1;
fi

echo "------ enable uac ------"
TUYA_APP_DEMO_PATH=projects/tuya_app

ty_lwip=$(grep -wcE "#define *ENABLE_LWIP" ${sdk_config_file})
if [ "x$ty_lwip" = "x1" ]; then
    echo "------ use tuya lwip ------"
    export TUYA_LWIP_STACK_USED="lwip_tuya"
    make tuya_lwip -f ${TUYA_APP_DEMO_PATH}/tuya_scripts/env.mk
else
    echo "------ use bk lwip ------"
    export TUYA_LWIP_STACK_USED="lwip_bk"
    cp ${TUYA_APP_DEMO_PATH}/config/${TARGET_PLATFORM}/config .${TARGET_PLATFORM}_config.save
    sed -i "s/CONFIG_LWIP=n/CONFIG_LWIP=y/g" ${TUYA_APP_DEMO_PATH}/config/${TARGET_PLATFORM}/config
    sed -i "s/CONFIG_LWIP_V2_1=n/CONFIG_LWIP_V2_1=y/g" ${TUYA_APP_DEMO_PATH}/config/${TARGET_PLATFORM}/config
fi

echo "CHECK COMPONENTS"
is_componenst_file_exist=0
if [ -f "${TUYA_PROJECT_DIR}/apps/$APP_BIN_NAME/components.mk" ]; then
    is_componenst_file_exist=1
    comp_dir=""
    if [ -d "${TUYA_PROJECT_DIR}/application_components" ]; then
        comp_dir=application_components
    elif [ -d "${TUYA_PROJECT_DIR}/components" ]; then
        comp_dir=components
    fi # comp_dir
    echo "components path: $comp_dir"

    make tuya_components -f ${TUYA_APP_DEMO_PATH}/tuya_scripts/env.mk COMPONENTS_PATH=$comp_dir APP=$APP_BIN_NAME

fi # components.mk

# export PYTHONPATH=${TUYA_PROJECT_DIR}/vendor/T5/toolchain/site-packages:${PYTHONPATH}

echo "APP_DIR:"$APP_DIR

echo "check bootloader.bin"
boot_file=bk_idk/components/bk_libs/bk7258/bootloader/normal_bootloader/bootloader.bin
check_value=$(md5sum ${boot_file} | awk '{print $1}')
ori_value=f8f45b0779a8269fa089ac84ebd9c149
if [ "x${check_value}" != "x${ori_value}" ]; then
    echo -e "\033[1;31m bootloader.bin check failed, the file had been changed, please update md5 value in build.sh \033[0m"
    exit
else
    echo "bootloader check ok"
fi

# python 虚拟环境
PYTHON_CMD="python3"
PIP_CMD="pip3"
enable_python_env() {
    if [ -z $1 ]; then
        echo "Please input virtual environment name."
        exit 1
    fi

    VIRTUAL_NAME=$1
    SCRIPT_DIR=$PWD/${TUYA_APP_DEMO_PATH}
    VIRTUAL_ENV=$SCRIPT_DIR/$VIRTUAL_NAME

    echo "SCRIPT_DIR $VIRTUAL_ENV"
    if command -v python3 &>/dev/null; then
        PYTHON_CMD=python3
    elif command -v python &>/dev/null && python --version | grep -q '^Python 3'; then
        PYTHON_CMD=python
    else
        echo "Python 3 is not installed."
        exit 1
    fi

    if [ ! -d "${VIRTUAL_ENV}" ]; then
        echo "Virtual environment not found. Creating one..."
        $PYTHON_CMD -m venv "${VIRTUAL_ENV}" || { echo "Failed to create virtual environment."; exit 1; }
        echo "Virtual environment created at ${VIRTUAL_ENV}"
    else
        echo "Virtual environment already exists."
    fi

    ACTIVATE_SCRIPT=${VIRTUAL_ENV}/bin/activate
    PYTHON_CMD="${VENV_DIR}/bin/python3"
    if [ -f "$ACTIVATE_SCRIPT" ]; then
        echo "Activate python virtual environment."
        . ${ACTIVATE_SCRIPT} || { echo "Failed to activate virtual environment."; exit 1; }
        ${PIP_CMD} install -r "projects/tuya_app/tuya_scripts/requirements.txt" || { echo "Failed to install required Python packages."; deactivate; exit 1; }
    else
        echo "Activate script not found."
        exit 1
    fi
}

disable_python_env() {
    if [ -z $1 ]; then
        echo "Please input virtual environment name."
        exit 1
    fi

    VIRTUAL_NAME=$1
    SCRIPT_DIR=$PWD
    VIRTUAL_ENV=$SCRIPT_DIR/$VIRTUAL_NAME

    echo "SCRIPT_DIR $VIRTUAL_ENV"
    if [ -n "$VIRTUAL_ENV" ]; then
        echo "Deactivate python virtual environment."
        deactivate
    else
        echo "No virtual environment is active."
    fi
}

if [ -z $CI_PACKAGE_PATH ]; then
    enable_python_env "tuya_build_env" || { echo "Failed to enable python virtual environment."; exit 1; }
else
    export PYTHONPATH=${TUYA_PROJECT_DIR}/vendor/T5/toolchain/site-packages:${PYTHONPATH}
fi


# 业务需求：提前初始化gpio
app_config_file=${TUYA_PROJECT_DIR}/apps/$APP_BIN_NAME/default_gpio_config.json
out_file_path=${TUYA_PROJECT_DIR}/vendor/T5/t5_os/projects/tuya_app/config
vendor_config_file=projects/tuya_app/tuya_scripts/tuya_gpio_config.json
vendor_convert_script=projects/tuya_app/tuya_scripts/default_gpio_config.py
echo "python3 ${vendor_convert_script} ${out_file_path} ${vendor_config_file} ${app_config_file}"
python3 ${vendor_convert_script} ${out_file_path} ${vendor_config_file} ${app_config_file}
if [ -f ${out_file_path}/usr_gpio_cfg0.h ]; then
    mv ${out_file_path}/usr_gpio_cfg0.h ${out_file_path}/bk7258/usr_gpio_cfg.h
fi
if [ -f ${out_file_path}/usr_gpio_cfg1.h ]; then
    mv ${out_file_path}/usr_gpio_cfg1.h ${out_file_path}/bk7258_cp1/usr_gpio_cfg.h
fi
if [ -f ${out_file_path}/usr_gpio_cfg2.h ]; then
    mv ${out_file_path}/usr_gpio_cfg2.h ${out_file_path}/bk7258_cp2/usr_gpio_cfg.h
fi

echo "Start Compile"

echo "make ${TARGET_PLATFORM} PROJECT_DIR=../${TUYA_APP_DEMO_PATH} BUILD_DIR=../build APP_NAME=$APP_BIN_NAME APP_VERSION=$USER_SW_VER -j"
make ${TARGET_PLATFORM} PROJECT_DIR=../${TUYA_APP_DEMO_PATH} BUILD_DIR=../build APP_NAME=$APP_BIN_NAME APP_VERSION=$USER_SW_VER -j
res=$(echo $?)
rm $tmp_gen_files_list

# delete file whether it exists
rm -f ${TUYA_APP_DEMO_PATH}/.tmp_component_desc
if [ "x${TUYA_LWIP_STACK_USED}" = "xlwip_bk" ]; then
    mv .${TARGET_PLATFORM}_config.save ${TUYA_APP_DEMO_PATH}/config/${TARGET_PLATFORM}/config
else
    rm -rf ../tuyaos/tuya_os_adapter/lwip_intf_v2_1
fi

if [ $res -ne 0 ]; then
    echo "make failed"
    exit -1
fi

echo "Start Combined"

OUTPUT_PATH=""
if [ "x${CI_PACKAGE_PATH}" != "x" ]; then
    echo "ci build"
    OUTPUT_PATH=${CI_PACKAGE_PATH}
else
    echo "local build"
    OUTPUT_PATH=../../../apps/$APP_BIN_NAME/output/$USER_SW_VER
fi

if [ ! -d "$OUTPUT_PATH" ]; then
	mkdir -p $OUTPUT_PATH
fi

DEBUG_FILE_PATH=${OUTPUT_PATH}/debug
if [ ! -d "$DEBUG_FILE_PATH" ]; then
	mkdir -p $DEBUG_FILE_PATH
fi

if [ ! -d "${DEBUG_FILE_PATH}/${TARGET_PLATFORM}" ]; then
    mkdir -p ${DEBUG_FILE_PATH}/${TARGET_PLATFORM}
fi

if [ ! -d "${DEBUG_FILE_PATH}/${TARGET_PLATFORM}_cp1" ]; then
    mkdir -p ${DEBUG_FILE_PATH}/${TARGET_PLATFORM}_cp1
fi

#if [ ! -d "${DEBUG_FILE_PATH}/${TARGET_PLATFORM}_cp2" ]; then
#    mkdir -p ${DEBUG_FILE_PATH}/${TARGET_PLATFORM}_cp2
#fi


if [ -e "./build/${TARGET_PLATFORM}/all-app.bin" ]; then
    set -x
    set -e

    app1_ofs=$(stat -c %s ./build/${TARGET_PLATFORM}/app.bin)
    app0_max_size=1740800
    # TODO 1920k = 1966080 bytes
#    app0_max_size=1966080
#    if [ $app1_ofs -gt $app0_max_size ]; then
#        echo "app0 file is too big, limit $app0_max_size, act $app1_ofs"
#        exit -1
#    fi

#    pad_bytes_size=$(expr $app0_max_size - $app1_ofs)
#    dd if=/dev/zero bs=1 count=${pad_bytes_size} | tr "\000" "\377" > pad_bin_file
#
#    cat ./build/${TARGET_PLATFORM}/app.bin pad_bin_file ./build/${TARGET_PLATFORM}/app1.bin > ./build/${TARGET_PLATFORM}/ua_file.bin
#    total_size=$(stat -c %s ./build/${TARGET_PLATFORM}/ua_file.bin)
#
#    echo "app1_ofs: ${app1_ofs}"
#    echo "pad_bytes_size: ${pad_bytes_size}"
#    echo "total_size: ${total_size}"

    TUYA_CREATE_UA_FILE_TOOL=${TUYA_APP_DEMO_PATH}/tuya_scripts/create_ua_file.py
    TUYA_FORMAT_BIN_TOOL=${TUYA_APP_DEMO_PATH}/tuya_scripts/format_up_bin.py
    TUYA_DIFF_OTA_BIN_TOOL=${TUYA_APP_DEMO_PATH}/tuya_scripts/diff2ya

    partiton_file=${TUYA_APP_DEMO_PATH}/config/bk7258/bk7258_partitions.csv
    cpu0_bin_file=build/bk7258/app.bin
    cpu1_bin_file=build/bk7258/app1.bin
    cpu2_bin_file=build/bk7258/app2.bin
    ua_bin_file=build/${TARGET_PLATFORM}/ua_file.bin

    # python3 ${TUYA_CREATE_UA_FILE_TOOL} ${partiton_file} ${cpu0_bin_file} ${cpu1_bin_file} --cpu2_bin=${cpu2_bin_file} --ua_file=${ua_bin_file}
    python3 ${TUYA_CREATE_UA_FILE_TOOL} ${partiton_file} ${cpu0_bin_file} ${cpu1_bin_file} --ua_file=${ua_bin_file}

    python3 ${TUYA_FORMAT_BIN_TOOL} ./build/${TARGET_PLATFORM}/ua_file.bin ./build/${TARGET_PLATFORM}/app_ug.bin 5ED000 1000 0 1000 10D0 $app0_max_size -v
    ./${TUYA_DIFF_OTA_BIN_TOOL} ./build/${TARGET_PLATFORM}/app_ug.bin ./build/${TARGET_PLATFORM}/app_ug.bin ./build/${TARGET_PLATFORM}/app_ota_ug.bin 0

    # rm pad_bin_file

    set -x
    # 在固件尾部追加固件校验信息
    script_file=${TUYA_PROJECT_DIR}/scripts/write_verid_to_bin.py
    if [ "x" != "x$TUYAOS_VERSION_ID" ] && [ -f ${script_file} ]; then
        echo "add info to file tail"
        cp ./build/${TARGET_PLATFORM}/all-app.bin      $DEBUG_FILE_PATH/${TARGET_PLATFORM}/ori-all-app.bin
        cp ./build/${TARGET_PLATFORM}/app_ota_ug.bin   $DEBUG_FILE_PATH/${TARGET_PLATFORM}/ori-app_ota_ug.bin

        cd ./build/${TARGET_PLATFORM}
        python3 ${script_file} all-app.bin
        python3 ${script_file} app_ota_ug.bin
        echo "add info end"
        cd -
    fi

    cp ./build/${TARGET_PLATFORM}/all-app.bin       $OUTPUT_PATH/$APP_BIN_NAME"_QIO_"$USER_SW_VER.bin
    cp ./build/${TARGET_PLATFORM}/ua_file.bin       $OUTPUT_PATH/$APP_BIN_NAME"_UA_"$USER_SW_VER.bin
    cp ./build/${TARGET_PLATFORM}/app_ota_ug.bin    $OUTPUT_PATH/$APP_BIN_NAME"_UG_"$USER_SW_VER.bin

    cp ./build/${TARGET_PLATFORM}/app*              $DEBUG_FILE_PATH/${TARGET_PLATFORM}
    cp ./build/${TARGET_PLATFORM}/size_map*         $DEBUG_FILE_PATH/${TARGET_PLATFORM}
    cp ./build/${TARGET_PLATFORM}/sdkconfig         $DEBUG_FILE_PATH/${TARGET_PLATFORM}
    cp ./build/${TARGET_PLATFORM}/bootloader.bin    $DEBUG_FILE_PATH/${TARGET_PLATFORM}

    cp ./build/${TARGET_PLATFORM}_cp1/app*          $DEBUG_FILE_PATH/${TARGET_PLATFORM}_cp1
    cp ./build/${TARGET_PLATFORM}_cp1/size_map*     $DEBUG_FILE_PATH/${TARGET_PLATFORM}_cp1
    cp ./build/${TARGET_PLATFORM}_cp1/sdkconfig     $DEBUG_FILE_PATH/${TARGET_PLATFORM}_cp1

#    cp ./build/${TARGET_PLATFORM}_cp2/app*          $DEBUG_FILE_PATH/${TARGET_PLATFORM}_cp2
#    cp ./build/${TARGET_PLATFORM}_cp2/size_map*     $DEBUG_FILE_PATH/${TARGET_PLATFORM}_cp2
#    cp ./build/${TARGET_PLATFORM}_cp2/sdkconfig     $DEBUG_FILE_PATH/${TARGET_PLATFORM}_cp2
fi

echo "*************************************************************************"
echo "*************************************************************************"
echo "*************************************************************************"
echo "*********************${APP_BIN_NAME}_$APP_VERSION.bin********************"
echo "*************************************************************************"
echo "**********************COMPILE SUCCESS************************************"
echo "*************************************************************************"

disable_python_env "tuya_build_env"
# Modified by TUYA End

