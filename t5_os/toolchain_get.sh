#!/usr/bin/env bash

set -e

if [ -z $1 ]; then
    echo "Please input toolchain path"
    exit 1
fi

TOOLCHAIN_PATH=$1

TOP_DIR=$(pwd)
echo $TOP_DIR

echo "Start board prepare ..."

if [ ! -d "$TOOLCHAIN_PATH" ]; then
    mkdir -p $TOOLCHAIN_PATH
    echo "Toolchain $TOOLCHAIN_PATH not exist"
fi

TOOLCHAIN_NAME=$TOOLCHAIN_PATH/gcc-arm-none-eabi-10.3-2021.10

if [ -d "$TOOLCHAIN_NAME" ]; then
    if [ -n "$(ls -A $TOOLCHAIN_NAME)" ]; then
        echo "Toolchain $TOOLCHAIN_NAME check Successful"
        echo "Run board prepare success ..."
        exit 0
    fi
fi

echo "Start download toolchain"
# restult=$(curl -m 15 -s http://www.ip-api.com/json)
# country=$(echo $restult | sed 's/.*"country":"\([^"]*\)".*/\1/')
# echo "country: $country"


cd $TOOLCHAIN_PATH
HOST_MACHINE=$(uname -m)

case "$(uname -s)" in
Linux*)
	SYSTEM_NAME="Linux"
    if [ $HOST_MACHINE = "x86_64" ]; then
        TOOLCHAIN_URL=https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2
        TOOLCHAIN_FILE=gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2
        TOOLCHAIN_SIZE=157089706
        TOOLCHAIN_SHA256=97dbb4f019ad1650b732faffcc881689cedc14e2b7ee863d390e0a41ef16c9a3
    elif [ $HOST_MACHINE = "aarch64" ]; then
        TOOLCHAIN_URL=https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-aarch64-linux.tar.bz2
        TOOLCHAIN_FILE=gcc-arm-none-eabi-10.3-2021.10-aarch64-linux.tar.bz2
        TOOLCHAIN_SIZE=168772350
        TOOLCHAIN_SHA256=f605b5f23ca898e9b8b665be208510a54a6e9fdd0fa5bfc9592002f6e7431208
    else
        echo "Toolchain not support, Please download toolchain from https://developer.arm.com/downloads/-/gnu-rm"
        exit 1
    fi
	;;
Darwin*)
	SYSTEM_NAME="Apple"
    if [ $HOST_MACHINE = "x86_64" ]; then
        TOOLCHAIN_URL=https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-mac.tar.bz2
        TOOLCHAIN_FILE=gcc-arm-none-eabi-10.3-2021.10-mac.tar.bz2
        TOOLCHAIN_SIZE=158961466
        TOOLCHAIN_SHA256=fb613dacb25149f140f73fe9ff6c380bb43328e6bf813473986e9127e2bc283b
    else
        echo "Toolchain not support, Please download toolchain from https://developer.arm.com/downloads/-/gnu-rm"
        exit 1
    fi
	;;
MINGW* | CYGWIN* | MSYS*)
	SYSTEM_NAME="Windows"
    TOOLCHAIN_URL=https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-win32.zip
    TOOLCHAIN_FILE=gcc-arm-none-eabi-10.3-2021.10-win32.zip
    TOOLCHAIN_SIZE=200578763
    TOOLCHAIN_SHA256=d287439b3090843f3f4e29c7c41f81d958a5323aecefcf705c203bfd8ae3f2e7
	;;
*)
	SYSTEM_NAME="Unknown"
    exit 1
	;;
esac

echo "Running on [$SYSTEM_NAME]"

if [ ! -f $TOOLCHAIN_FILE ]; then
    wget $TOOLCHAIN_URL -O $TOOLCHAIN_FILE

    if [ $? -ne 0 ]; then
        echo "Download toolchain failed"
        rm -rf $TOOLCHAIN_FILE
        exit 1
    fi
fi

if [ $TOOLCHAIN_SHA256 != $(sha256sum $TOOLCHAIN_FILE | awk '{print $1}') ]; then
    echo "Toolchain file checksum error"
    rm -rf $TOOLCHAIN_FILE
    exit 1
fi

if [ $TOOLCHAIN_SIZE != $(stat -c %s $TOOLCHAIN_FILE) ]; then
    echo "Toolchain file size error"
    rm -rf $TOOLCHAIN_FILE
    exit 1
fi

FILE_EXTENSION=${TOOLCHAIN_FILE##*.}

echo "start decompression"

echo "FILE_EXTENSION: ${FILE_EXTENSION}"

if [ $FILE_EXTENSION = "bz2" ]; then
    tar -xvf $TOOLCHAIN_FILE -C $TOOLCHAIN_PATH
elif [ $FILE_EXTENSION = "zip" ]; then
    unzip $TOOLCHAIN_FILE
else
    echo "File not support"
    exit 1
fi

echo "Run board prepare success ..."