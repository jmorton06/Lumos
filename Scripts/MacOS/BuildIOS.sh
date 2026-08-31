#!/bin/bash
set -euo pipefail

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT="$DIR/../.."

CONFIGURATION="Debug"
GAME_PROJECT=""
DESTINATION="generic/platform=iOS"
SHADERC=""

for arg in "$@"; do
  case $arg in
    --project=*)
      GAME_PROJECT="${arg#--project=}"
      ;;
    --release)
      CONFIGURATION="Release"
      ;;
    --simulator)
      DESTINATION="generic/platform=iOS Simulator"
      ;;
    --shaderc)
      SHADERC="1"
      ;;
  esac
done

cd "$ROOT"

# Bake .lmesh caches for any source model (obj/gltf/glb/fbx) in the project
# before packaging for iOS. AssetImporter writes to //Assets/Imported/ on
# first load of a given mesh - fine on desktop, but the iOS app bundle is
# read-only, so any mesh never seen on a writable filesystem before crashes
# on-device ("create_directories: Operation not permitted"). Builds a macOS
# Runtime on demand if one isn't already around, same as PackageGame.sh.
if [ -n "$GAME_PROJECT" ]; then
  RUNTIME_BIN=$(find "$ROOT/bin" -path "*macosx*" -name "Runtime" -type f -perm -u+x 2>/dev/null | head -1)
  if [ -z "$RUNTIME_BIN" ] || [ ! -f "$RUNTIME_BIN" ]; then
    echo "macOS Runtime missing - building it for asset import..."
    ( cd "$ROOT" && Tools/premake5 xcode4 )
    HOST_ARCH=$(uname -m)
    ( cd "$ROOT" && xcodebuild -project Runtime/Runtime.xcodeproj \
        -parallelizeTargets -jobs 4 -configuration Release \
        -sdk macosx -arch "$HOST_ARCH" \
        CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO CODE_SIGNING_ALLOWED=NO \
        2>&1 | tail -20 )
    RUNTIME_BIN=$(find "$ROOT/bin" -path "*macosx*" -name "Runtime" -type f -perm -u+x 2>/dev/null | head -1)
  fi

  if [ -n "$RUNTIME_BIN" ] && [ -f "$RUNTIME_BIN" ]; then
    echo "Importing source models via $RUNTIME_BIN ..."
    "$RUNTIME_BIN" --project="$GAME_PROJECT" --import-assets \
      || { echo "Error: asset import failed"; exit 1; }
  else
    echo "Warning: could not obtain a macOS Runtime binary - skipping asset pre-import (new meshes may crash on-device)"
  fi
else
  echo "Warning: no --project= given, skipping asset pre-import (new meshes may crash on-device)"
fi

# Generate Xcode project
PREMAKE_CMD="Tools/premake5 --os=ios"
if [ -n "$GAME_PROJECT" ]; then
  PREMAKE_CMD="$PREMAKE_CMD --game-project=\"$GAME_PROJECT\""
fi
if [ -n "$SHADERC" ]; then
  PREMAKE_CMD="$PREMAKE_CMD --shaderc"
fi
PREMAKE_CMD="$PREMAKE_CMD xcode4"

echo "Generating iOS Xcode project..."
eval "$PREMAKE_CMD"

echo "Building Runtime ($CONFIGURATION) for '$DESTINATION'..."
xcodebuild \
  -workspace "Lumos.xcworkspace" \
  -scheme "Runtime" \
  -configuration "$CONFIGURATION" \
  -destination "$DESTINATION" \
  -parallelizeTargets \
  DEVELOPMENT_TEAM="C5L4T5BF6L"

echo "iOS build complete."
