#!/bin/bash

set -e

BUILD_TYPE=${BUILD_TYPE:-Release}
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
mkdir -p $DIR/../bin-android
cd $DIR/../bin-android
cmake .. -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DANDROID_NDK=$ANDROID_NDK -DANDROID_NATIVE_API_LEVEL=22 -DANDROID_PLATFORM=android-22 -DANDROID_TOOLCHAIN_NAME=arm-linux-androideabi-4.9 -DCMAKE_TOOLCHAIN_FILE=$DIR/../CMake/Modules/android.toolchain.cmake -DANDROID=ON -DASSIMP_BUILD_STATIC_LIB=ON
make $*
#make android-build

STATE=$(adb get-state || exit 0)
if [ "$STATE" == "device" ]; then
	make android-install android-start
fi
