#!/bin/bash
set -euo pipefail

export PATH="/usr/local/bin:$PATH"

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
