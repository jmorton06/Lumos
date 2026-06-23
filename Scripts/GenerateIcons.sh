#!/bin/bash
# GenerateIcons.sh <project-dir> [--platform=all|macos|ios|windows|android|linux]
# Generates platform-specific app icons from a single source PNG.
# Reads IconPath from .lmproj; falls back to Assets/Textures/icon.png.
set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_DIR="$SCRIPT_DIR/.."

# Parse args
PROJECT_DIR=""
PLATFORM="all"

for arg in "$@"; do
    case "$arg" in
        --platform=*) PLATFORM="${arg#--platform=}" ;;
        *) PROJECT_DIR="$arg" ;;
    esac
done

if [ -z "$PROJECT_DIR" ]; then
    echo "Usage: GenerateIcons.sh <project-dir> [--platform=all|macos|ios|windows|android|linux]"
    exit 1
fi

# Make absolute if relative
if [[ "$PROJECT_DIR" != /* ]]; then
    PROJECT_DIR="$ROOT_DIR/$PROJECT_DIR"
fi

# Find .lmproj
LMPROJ=$(find "$PROJECT_DIR" -maxdepth 1 -name "*.lmproj" -print -quit 2>/dev/null)
if [ -z "$LMPROJ" ]; then
    echo "Error: No .lmproj found in $PROJECT_DIR"
    exit 1
fi

# Extract IconPath from .lmproj
ICON_PATH=$(python3 -c "
import json
with open('$LMPROJ') as f:
    data = json.load(f)
v = data.get('value0', data)
print(v.get('IconPath', ''))
" 2>/dev/null || echo "")

# Resolve VFS path (//Assets/... → project-relative)
if [ -n "$ICON_PATH" ]; then
    ICON_PATH="${ICON_PATH#//}"
    SOURCE_PNG="$PROJECT_DIR/$ICON_PATH"
else
    SOURCE_PNG=""
fi

# Fallback candidates if IconPath not set or file missing
if [ -z "$SOURCE_PNG" ] || [ ! -f "$SOURCE_PNG" ]; then
    CANDIDATES=(
        "$PROJECT_DIR/Assets/Textures/icon.png"
        "$PROJECT_DIR/Assets/Textures/Icon.png"
        "$PROJECT_DIR/Assets/icon.png"
        "$PROJECT_DIR/icon.png"
    )
    SOURCE_PNG=""
    for c in "${CANDIDATES[@]}"; do
        if [ -f "$c" ]; then
            SOURCE_PNG="$c"
            break
        fi
    done
fi

if [ -z "$SOURCE_PNG" ] || [ ! -f "$SOURCE_PNG" ]; then
    echo "Error: No icon source found. Set IconPath in .lmproj or place icon.png in Assets/Textures/"
    exit 1
fi

# Check source dimensions
DIMS=$(python3 -c "
import struct, zlib
with open('$SOURCE_PNG','rb') as f:
    f.read(16)
    w = struct.unpack('>I', f.read(4))[0]
    h = struct.unpack('>I', f.read(4))[0]
    print(f'{w}x{h}')
" 2>/dev/null || echo "unknown")

echo "=== GenerateIcons ==="
echo "Source: $SOURCE_PNG ($DIMS)"
echo "Platform: $PLATFORM"

SRC_W=$(echo "$DIMS" | cut -dx -f1)
if [ "$SRC_W" != "unknown" ] && [ "$SRC_W" -lt 1024 ] 2>/dev/null; then
    echo "Warning: Source image is ${DIMS}, recommend at least 1024x1024 for best quality"
fi

# Output directories
BUILD_DIR="$PROJECT_DIR/Resources/Icons"
mkdir -p "$BUILD_DIR"

# ICO tool location
ICOTOOL="$ROOT_DIR/Tools/LumosIconGen/LumosIconGen"

# Build icotool if not present
if [ ! -f "$ICOTOOL" ]; then
    echo "Building LumosIconGen..."
    make -C "$ROOT_DIR/Tools/LumosIconGen" -s
fi

# Platform dispatch
do_macos() {
    echo "Generating macOS icons..."
    local out_dir="$BUILD_DIR/macos"
    mkdir -p "$out_dir"
    "$SCRIPT_DIR/MacOS/PrepareIcon.sh" "$SOURCE_PNG" "$out_dir"

    # Also update the engine xcassets (used by premake/xcode builds)
    "$SCRIPT_DIR/MacOS/PrepareIcon.sh" "$SOURCE_PNG"
}

do_ios() {
    echo "Generating iOS icons..."
    # iOS uses same xcassets as macOS (single 1024x1024, Xcode handles sizes)
    "$SCRIPT_DIR/MacOS/PrepareIcon.sh" "$SOURCE_PNG"
    echo "iOS icon prepared in xcassets"
}

do_windows() {
    echo "Generating Windows icon..."
    local out_dir="$BUILD_DIR/windows"
    mkdir -p "$out_dir"
    "$ICOTOOL" "$SOURCE_PNG" -o "$out_dir/app.ico"

    # Generate .rc file
    cat > "$out_dir/app.rc" << 'EOF'
IDI_ICON1 ICON "app.ico"
EOF
    echo "Windows icon: $out_dir/app.ico"
}

do_android() {
    echo "Generating Android icons..."
    local sizes=("mdpi:48" "hdpi:72" "xhdpi:96" "xxhdpi:144" "xxxhdpi:192")

    for entry in "${sizes[@]}"; do
        local density="${entry%%:*}"
        local sz="${entry##*:}"
        local out_dir="$BUILD_DIR/android/mipmap-${density}"
        mkdir -p "$out_dir"

        if command -v sips &>/dev/null; then
            sips -z "$sz" "$sz" "$SOURCE_PNG" --out "$out_dir/AppIcon.png" > /dev/null 2>&1
        else
            "$ICOTOOL" "$SOURCE_PNG" --resize "${sz}x${sz}" -o "$out_dir/AppIcon.png"
        fi
    done
    echo "Android icons generated in $BUILD_DIR/android/"
}

do_linux() {
    echo "Generating Linux icons..."
    local sizes=(48 128 256)

    for sz in "${sizes[@]}"; do
        local out_dir="$BUILD_DIR/linux"
        mkdir -p "$out_dir"

        if command -v sips &>/dev/null; then
            sips -z "$sz" "$sz" "$SOURCE_PNG" --out "$out_dir/icon_${sz}.png" > /dev/null 2>&1
        else
            "$ICOTOOL" "$SOURCE_PNG" --resize "${sz}x${sz}" -o "$out_dir/icon_${sz}.png"
        fi
    done
    echo "Linux icons generated in $BUILD_DIR/linux/"
}

case "$PLATFORM" in
    all)
        do_macos
        do_ios
        do_windows
        do_android
        do_linux
        ;;
    macos)  do_macos ;;
    ios)    do_ios ;;
    windows) do_windows ;;
    android) do_android ;;
    linux)  do_linux ;;
    *)
        echo "Error: unknown platform '$PLATFORM'"
        exit 1
        ;;
esac

echo ""
echo "Done. Icons in $BUILD_DIR/"
