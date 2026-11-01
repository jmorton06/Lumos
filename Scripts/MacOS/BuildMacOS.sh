#!/bin/bash
PATH="/usr/local/bin:$PATH"

# The Xcode projects are shared with the iOS build: if the last thing generated
# them was an iOS one (an iOS template / BuildIOS.sh), regenerate for macOS first.
ROOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/../.." && pwd )"
cd "$ROOT_DIR"
if [ ! -f Runtime/Runtime.xcodeproj/project.pbxproj ] || grep -q "IPHONEOS_DEPLOYMENT_TARGET" Runtime/Runtime.xcodeproj/project.pbxproj; then
    echo "Xcode projects missing or set up for iOS - regenerating for macOS"
    ./Scripts/MacOS/GenerateXCode.sh >/dev/null
fi

# -jobs: all cores (was a hardcoded 4 on an 8-core box).
# COMPILER_INDEX_STORE_ENABLE=NO: Xcode's indexer otherwise runs a second
#   -fsyntax-only pass over every TU while the build runs, competing for the
#   same cores. The IDE re-indexes on its own afterwards.
# -quiet: one line per file instead of the full clang invocation, so the log
#   is greppable.
JOBS=$(sysctl -n hw.ncpu)

xcodebuild -project Runtime/Runtime.xcodeproj -parallelizeTargets -jobs "$JOBS" -quiet \
    -configuration Release CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO CODE_SIGNING_ALLOWED=NO \
    COMPILER_INDEX_STORE_ENABLE=NO -sdk macosx -arch x86_64
status=$?

# This used to print "Build Finished!" on a failed build too, which is a good
# way to test a binary that was never rebuilt.
if [ $status -ne 0 ]; then
    echo "BUILD FAILED (exit $status)"
    exit $status
fi

echo "Build Finished!"
