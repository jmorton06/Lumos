#!/bin/bash

set -e

BUILD_TYPE=${BUILD_TYPE:-Release}
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
mkdir -p $DIR/../bin-ios
cd $DIR/../bin-ios
cmake .. -DCMAKE_TOOLCHAIN_FILE=$DIR/../CMake/Modules/ios.toolchain.cmake  -DCMAKE_OSX_ARCHITECTURES=x86_64 -DIOS_PLATFORM=SIMULATOR64 -DIOS_DEPLOYMENT_TARGET=10.3 -DIOS=ON -DASSIMP_BUILD_STATIC_LIB=ON
#-G Xcode
#cd $DIR/../
make $*
#make ios-build

STATE=$(adb get-state || exit 0)
if [ "$STATE" == "device" ]; then
make ios-install ios-start
fi
