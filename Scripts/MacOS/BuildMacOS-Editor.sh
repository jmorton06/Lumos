#!/bin/bash
set -euo pipefail

export PATH="/usr/local/bin:$PATH"

PROJECT_PATH="Editor/LumosEditor.xcodeproj"
SCHEME="LumosEditor"
CONFIGURATION="Release"
SDK="macosx"
ARCH="x86_64"

echo "Starting macOS Editor build"

xcodebuild \
  -project "$PROJECT_PATH" \
  -scheme "$SCHEME" \
  -parallelizeTargets \
  -jobs 4 \
  -configuration "$CONFIGURATION" \
  -sdk "$SDK" \
  -arch "$ARCH" \
  CODE_SIGN_IDENTITY="" \
  CODE_SIGNING_REQUIRED=NO \
  CODE_SIGNING_ALLOWED=NO

echo "MacOS Editor Build Finished"
