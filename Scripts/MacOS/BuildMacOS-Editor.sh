#!/bin/bash
set -euo pipefail

export PATH="/usr/local/bin:$PATH"

# The Xcode projects are shared with the iOS build: if the last thing generated
# them was an iOS one (an iOS template / BuildIOS.sh), regenerate for macOS first.
ROOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/../.." && pwd )"
cd "$ROOT_DIR"
if [ ! -f Runtime/Runtime.xcodeproj/project.pbxproj ] || grep -q "IPHONEOS_DEPLOYMENT_TARGET" Runtime/Runtime.xcodeproj/project.pbxproj; then
    echo "Xcode projects missing or set up for iOS - regenerating for macOS"
    ./Scripts/MacOS/GenerateXCode.sh >/dev/null
fi

PROJECT_PATH="Editor/LumosEditor.xcodeproj"
SCHEME="LumosEditor"
CONFIGURATION="Release"
SDK="macosx"

USE_XCPRETTY=false

for arg in "$@"; do
  case $arg in
    --pretty|-p)
      USE_XCPRETTY=true
      ;;
  esac
done

echo "Starting macOS Editor build"

BUILD_CMD="xcodebuild \
  -project \"$PROJECT_PATH\" \
  -scheme \"$SCHEME\" \
  -parallelizeTargets \
  -jobs 4 \
  -configuration \"$CONFIGURATION\" \
  -sdk \"$SDK\""

if [ "$USE_XCPRETTY" = true ]; then
  if ! command -v xcpretty &> /dev/null; then
    echo "xcpretty not found. Install with:"
    echo "  gem install xcpretty"
    echo "  gem install xcpretty-actions-formatter"
    read -p "Install now? [y/N] " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
      gem install xcpretty
      gem install xcpretty-actions-formatter
    else
      echo "Running without xcpretty..."
      eval "$BUILD_CMD"
      echo "MacOS Editor Build Finished"
      exit 0
    fi
  fi
  eval "$BUILD_CMD" | xcpretty -f $(xcpretty-actions-formatter)
else
  eval "$BUILD_CMD"
fi

echo "MacOS Editor Build Finished"
