#!/bin/bash

set -e

BUILD_TYPE=${BUILD_TYPE:-Debug}
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
mkdir -p $DIR/../bin
cd $DIR/../bin
cmake .. -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
make $*
