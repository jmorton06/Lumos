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
