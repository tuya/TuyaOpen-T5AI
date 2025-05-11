#!/bin/bash
# 参数说明：
# $1 - params path: echo_app_top
# $2 - user cmd: build / clean / ...

set -e
cd `dirname $0`

BUILD_PARAM_DIR=$1
BUILD_PARAM_FILE=$BUILD_PARAM_DIR/build_param.config
. $BUILD_PARAM_FILE

APP_BIN_NAME=$CONFIG_PROJECT_NAME
APP_VERSION=$CONFIG_PROJECT_VERSION
HEADER_DIR=$OPEN_HEADER_DIR
LIBS_DIR=$OPEN_LIBS_DIR
LIBS=$PLATFORM_NEED_LIBS
OUTPUT_DIR=$BIN_OUTPUT_DIR
USER_CMD=$2
BOARD_NAME=$PLATFORM_BOARD

TARGET_PLATFORM=bk7258

# echo APP_BIN_NAME=$APP_BIN_NAME
# echo APP_VERSION=$APP_VERSION
# echo USER_CMD=$USER_CMD
# echo LIBS_DIR=$LIBS_DIR
# echo LIBS=$LIBS
# echo OUTPUT_DIR=$OUTPUT_DIR
# echo HEADER_DIR=$HEADER_DIR
# echo TARGET_PLATFORM=$TARGET_PLATFORM
# echo BOARD_NAME=$BOARD_NAME
# exit 0

USER_SW_VER=`echo $APP_VERSION | cut -d'-' -f1`
TOOLSCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

PYTHON_CMD="python3"
check_python_install() {

    if command -v python3 >/dev/null; then
        PYTHON_CMD=python3
    elif command -v python >/dev/null && python --version | grep -q '^Python 3'; then
        PYTHON_CMD=python
    else
        echo "Python 3 is not installed. Please run: "
        echo ""
        echo "$ sudo apt-get install python3 -y"
        echo ""
        exit 1
    fi

    python_version=$(${PYTHON_CMD} --version 2>&1 | cut -d' ' -f2 | cut -d. -f1-2)
    formatted_version="python${python_version}-venv"
    echo "Python version: ${python_version}"

    major=$(echo "$python_version" | cut -d. -f1)
    minor=$(echo "$python_version" | cut -d. -f2)

    if [ "$major" -lt 3 ] || { [ "$major" -eq 3 ] && [ "$minor" -lt 8 ]; }; then
        echo "Error: Current Python version (${python_version}) is less than 3.8, please upgrade"
        exit 1
    fi

    if apt list --installed | grep -q "^${formatted_version}/"; then
        echo "${formatted_version} is installed, continuing with the script..."
    else
        echo "python3-venv is not installed. Please run:"
        echo ""
        echo "$ sudo apt-get install python3-venv -y"
        echo ""
        exit 1
    fi 
}

MIRROR=0

function get_country_code()
{
    if [ -f ${TOOLSCRIPT_DIR}/.mirror ]; then
        echo "Use existing country code."
        MIRROR=$(cat ${TOOLSCRIPT_DIR}/.mirror)
        echo "MIRROR=${MIRROR}"
    else
        echo "get_country_code ..."
        if command -v python3 &>/dev/null; then
            PYTHON_CMD=python3
        elif command -v python &>/dev/null && python --version | grep -q '^Python 3'; then
            PYTHON_CMD=python
        else
            echo "Python 3 is not installed."
            exit 1
        fi

        MIRROR=$(${PYTHON_CMD} ${TOOLSCRIPT_DIR}/tools/get_conutry.py)
        echo "country code: ${MIRROR}"
        if [ x"$MIRROR" = x"1" ]; then
            echo "enable cn mirror"
        fi

        echo ${MIRROR} > ${TOOLSCRIPT_DIR}/.mirror
    fi
}

enable_python_env() {
    if [ -z $1 ]; then
        echo "Please input virtual environment name."
        exit 1
    fi


    VIRTUAL_NAME=$1
    SCRIPT_DIR=$PWD/${TARGET_PROJECT}
    VIRTUAL_ENV=$SCRIPT_DIR/$VIRTUAL_NAME

    if [ ! -d "${VIRTUAL_ENV}" ]; then
        echo "Virtual environment not found. Creating one..."
        $PYTHON_CMD -m venv "${VIRTUAL_ENV}" || { echo "Failed to create virtual environment."; exit 1; }
        echo "Virtual environment created at ${VIRTUAL_ENV}"
    else
        echo "Virtual environment already exists."
    fi

    ACTIVATE_SCRIPT=${VIRTUAL_ENV}/bin/activate
    PIP_CMD=${VIRTUAL_ENV}/bin/pip3
    
    if [ -f "$ACTIVATE_SCRIPT" ] && [ -f ${PIP_CMD} ]; then
        echo "Activate python virtual environment."
        . ${ACTIVATE_SCRIPT} || { echo "Failed to activate virtual environment."; exit 1; }
        echo "country code: ${MIRROR}"
        if [ x"$MIRROR" = x"1" ]; then
            ${PIP_CMD} install -r "projects/tuya_app/tuya_scripts/requirements.txt" -i https://mirrors.aliyun.com/pypi/simple || { echo "Failed to install required Python packages."; deactivate; exit 1; }
        else
            ${PIP_CMD} install -r "projects/tuya_app/tuya_scripts/requirements.txt" || { echo "Failed to install required Python packages."; deactivate; exit 1; }
        fi
    else
        echo "Activate script not found."
        rm -rf "${VIRTUAL_ENV}"
        $PYTHON_CMD -m venv "${VIRTUAL_ENV}" || { echo "Failed to create virtual environment."; exit 1; }
        . ${ACTIVATE_SCRIPT} || { echo "Failed to activate virtual environment."; exit 1; }
        echo "country code: ${MIRROR}"
        if [ x"$MIRROR" = x"1" ]; then
            ${PIP_CMD} install -r "projects/tuya_app/tuya_scripts/requirements.txt"  -i https://mirrors.aliyun.com/pypi/simple || { echo "Failed to install required Python packages."; deactivate; exit 1; }
        else
            ${PIP_CMD} install -r "projects/tuya_app/tuya_scripts/requirements.txt" || { echo "Failed to install required Python packages."; deactivate; exit 1; }
        fi
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

check_python_install ||  { echo "Failed to check python environment."; exit 1; }

bash toolchain_get.sh $(pwd)/../tools || { echo "Failed to setup toolchain."; exit 1; } 


export TUYA_APP_PATH=$APP_PATH
export TUYA_APP_NAME=$APP_BIN_NAME

export TUYA_PROJECT_DIR=$(pwd)
export TUYA_HEADER_DIR=$HEADER_DIR
export TUYA_LIBS_DIR=$LIBS_DIR
export TUYA_LIBS=$LIBS

APP_PATH=../../$APP_DIR

cd t5_os

TARGET_PROJECT=projects/tuya_app

get_country_code

enable_python_env "tuya_build_env" || { echo "Failed to enable python virtual environment."; exit 1; }

tmp_gen_files_list=bk_idk/tools/build_tools/part_table_tools/config/gen_files_list.txt
if [ -f $tmp_gen_files_list ]; then
    rm $tmp_gen_files_list
fi
touch $tmp_gen_files_list

TOP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [ -f ${TOP_DIR}/.app ]; then
    OLD_APP_BIN_NAME=$(cat ${TOP_DIR}/.app)
    echo OLD_APP_BIN_NAME: ${OLD_APP_BIN_NAME}
fi

echo ${APP_BIN_NAME} > ${TOP_DIR}/.app
if [ "$OLD_APP_BIN_NAME" != "$APP_BIN_NAME" ]; then
	make clean
	make clean -C ./bk_idk/
	echo "AUTO CLEAN SUCCESS"
fi

if [ x$USER_CMD = "xclean" ];then
	make clean
	make clean -C ./bk_idk/
    # git checkout build/*
	echo "*************************************************************************"
	echo "************************CLEAN SUCCESS************************************"
	echo "*************************************************************************"
	exit 0
fi

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

echo "Start Compile"

make ${TARGET_PLATFORM} PROJECT_DIR=../${TARGET_PROJECT} BUILD_DIR=../build APP_NAME=$APP_BIN_NAME APP_VERSION=$USER_SW_VER -j

res=$(echo $?)
rm $tmp_gen_files_list

# delete file whether it exists
rm -f ${TARGET_PROJECT}/.tmp_component_desc
if [ "x${TUYA_LWIP_STACK_USED}" = "xlwip_bk" ]; then
    mv .${TARGET_PLATFORM}_config.save ${TARGET_PROJECT}/config/${TARGET_PLATFORM}/config
else
    rm -rf ../tuyaos/tuya_os_adapter/lwip_intf_v2_1
fi

if [ $res -ne 0 ]; then
    echo "make failed"
    exit -1
fi

echo "Start Combined"

if [ ! -d "$OUTPUT_DIR" ]; then
	mkdir -p $OUTPUT_DIR
fi

# DEBUG_FILE_PATH=${OUTPUT_DIR}/debug
# if [ ! -d "$DEBUG_FILE_PATH" ]; then
# 	mkdir -p $DEBUG_FILE_PATH
# fi

# if [ ! -d "${DEBUG_FILE_PATH}/${TARGET_PLATFORM}" ]; then
#     mkdir -p ${DEBUG_FILE_PATH}/${TARGET_PLATFORM}
# fi

# if [ ! -d "${DEBUG_FILE_PATH}/${TARGET_PLATFORM}_cp1" ]; then
#     mkdir -p ${DEBUG_FILE_PATH}/${TARGET_PLATFORM}_cp1
# fi

#if [ ! -d "${DEBUG_FILE_PATH}/${TARGET_PLATFORM}_cp2" ]; then
#    mkdir -p ${DEBUG_FILE_PATH}/${TARGET_PLATFORM}_cp2
#fi

if [ -e "./build/${TARGET_PLATFORM}/all-app.bin" ]; then
    ofs=$(stat -c %s ./build/${TARGET_PLATFORM}/app.bin)
    app0_max_size=1740800
    # TODO 1920k = 1966080 bytes
    # app0_max_size=1966080
    if [ $ofs -gt $app0_max_size ]; then
        echo "app0 file is too big, limit $app0_max_size, act $ofs"
        exit -1
    fi

    pad_bytes_size=$(expr $app0_max_size - $ofs)
    dd if=/dev/zero bs=1 count=${pad_bytes_size} | tr "\000" "\377" > pad_bin_file

    cat ./build/${TARGET_PLATFORM}/app.bin pad_bin_file ./build/${TARGET_PLATFORM}/app1.bin > ./build/${TARGET_PLATFORM}/ua_file.bin
    total_size=$(stat -c %s ./build/${TARGET_PLATFORM}/ua_file.bin)

    echo "ofs: ${ofs}"
    echo "pad_bytes_size: ${pad_bytes_size}"
    echo "total_size: ${total_size}"

    TUYA_FORMAT_BIN_TOOL=${TARGET_PROJECT}/tuya_scripts/format_up_bin.py
    TUYA_DIFF_OTA_BIN_TOOL=${TARGET_PROJECT}/tuya_scripts/diff2ya

    python3 ${TUYA_FORMAT_BIN_TOOL} ./build/${TARGET_PLATFORM}/ua_file.bin ./build/${TARGET_PLATFORM}/app_ug.bin 500000 1000 0 1000 18D0 $app0_max_size -v
    ./${TUYA_DIFF_OTA_BIN_TOOL} ./build/${TARGET_PLATFORM}/app_ug.bin ./build/${TARGET_PLATFORM}/app_ug.bin ./build/${TARGET_PLATFORM}/app_ota_ug.bin 0

    rm pad_bin_file

    cp ./build/${TARGET_PLATFORM}/all-app.bin       $OUTPUT_DIR/$APP_BIN_NAME"_QIO_"$USER_SW_VER.bin
    cp ./build/${TARGET_PLATFORM}/ua_file.bin       $OUTPUT_DIR/$APP_BIN_NAME"_UA_"$USER_SW_VER.bin
    cp ./build/${TARGET_PLATFORM}/app_ota_ug.bin    $OUTPUT_DIR/$APP_BIN_NAME"_UG_"$USER_SW_VER.bin

	cp ./build/${TARGET_PLATFORM}/app.elf           $OUTPUT_DIR/$APP_BIN_NAME"_"$USER_SW_VER.elf
	cp ./build/${TARGET_PLATFORM}/app.map           $OUTPUT_DIR/$APP_BIN_NAME"_"$USER_SW_VER.map
	cp ./build/${TARGET_PLATFORM}/app.nm            $OUTPUT_DIR/$APP_BIN_NAME"_"$USER_SW_VER.nm
	cp ./build/${TARGET_PLATFORM}/app.txt           $OUTPUT_DIR/$APP_BIN_NAME"_"$USER_SW_VER.txt
	cp ./build/${TARGET_PLATFORM}/size_map*         $OUTPUT_DIR/
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

