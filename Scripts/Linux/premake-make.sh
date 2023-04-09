#!/bin/bash

set -e

BUILD_TYPE=${BUILD_TYPE:-Debug}
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/../

if [ $TRAVIS_OS_NAME == linux ]; then
Tools/linux/premake5 gmake2 -j4
else
Tools/premake5 gmake2 -j4
fi
make $*
./bin/Debug/Tests
