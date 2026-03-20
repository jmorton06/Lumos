#!/bin/bash
# PackageGame.sh <project-dir> [--ios]
# Builds a standalone .app (macOS) or generates iOS Xcode project from a Lumos game project
set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_DIR="$SCRIPT_DIR/../.."

# Parse args
PROJECT_DIR=""
IOS_MODE=false

for arg in "$@"; do
    case "$arg" in
        --ios) IOS_MODE=true ;;
        *) PROJECT_DIR="$arg" ;;
    esac
done

if [ -z "$PROJECT_DIR" ]; then
    echo "Usage: PackageGame.sh <project-dir> [--ios]"
    echo ""
    echo "  <project-dir>  Path to game project directory (contains .lmproj)"
    echo "  --ios          Generate iOS Xcode project instead of building macOS .app"
    exit 1
fi

# Make absolute if relative
if [[ "$PROJECT_DIR" != /* ]]; then
    PROJECT_DIR="$ROOT_DIR/$PROJECT_DIR"
fi

# Verify project exists
LMPROJ=$(find "$PROJECT_DIR" -maxdepth 1 -name "*.lmproj" -print -quit 2>/dev/null)
if [ -z "$LMPROJ" ]; then
    echo "Error: No .lmproj found in $PROJECT_DIR"
    exit 1
fi

# Extract project info from .lmproj
TITLE=$(python3 -c "
import json, sys
with open('$LMPROJ') as f:
    data = json.load(f)
v = data.get('value0', data)
print(v.get('Title', ''))
" 2>/dev/null || echo "")

BUNDLE_ID=$(python3 -c "
import json, sys
with open('$LMPROJ') as f:
    data = json.load(f)
v = data.get('value0', data)
print(v.get('BundleIdentifier', ''))
" 2>/dev/null || echo "")

if [ -z "$TITLE" ]; then
    TITLE=$(basename "$PROJECT_DIR")
fi

# Safe project name (no spaces)
PROJECT_NAME=$(echo "$TITLE" | sed 's/[^a-zA-Z0-9_]//g')
if [ -z "$PROJECT_NAME" ]; then
    PROJECT_NAME=$(basename "$PROJECT_DIR" | sed 's/[^a-zA-Z0-9_]//g')
fi

echo "=== Lumos PackageGame ==="
echo "Project:   $TITLE"
echo "Directory: $PROJECT_DIR"
echo "Bundle ID: ${BUNDLE_ID:-auto}"
echo "Mode:      $([ "$IOS_MODE" = true ] && echo "iOS" || echo "macOS")"
echo ""

# Prepare icon if available
ICON_CANDIDATES=(
    "$PROJECT_DIR/Assets/Textures/icon.png"
    "$PROJECT_DIR/Assets/Textures/Icon.png"
    "$PROJECT_DIR/Assets/icon.png"
    "$PROJECT_DIR/icon.png"
)

for icon in "${ICON_CANDIDATES[@]}"; do
    if [ -f "$icon" ]; then
        echo "Preparing icon from $icon..."
        "$SCRIPT_DIR/PrepareIcon.sh" "$icon"
        break
    fi
done

# Generate customized iOS launch screen
"$SCRIPT_DIR/GenerateLaunchScreen.sh" "$LMPROJ"

cd "$ROOT_DIR"

# Pack assets into .lpak
RUNTIME_BIN="$ROOT_DIR/bin/Release-macosx-x86_64/Runtime.app/Contents/MacOS/Runtime"
PACK_OUTPUT="$PROJECT_DIR/Assets.lpak"
if [ -f "$RUNTIME_BIN" ]; then
    echo "Packing assets..."
    "$RUNTIME_BIN" --project="$PROJECT_DIR" --pack-assets="$PACK_OUTPUT" || echo "Warning: asset packing failed, bundling raw assets"
else
    echo "Warning: Runtime binary not found, skipping asset packing"
fi

# Run premake
if [ "$IOS_MODE" = true ]; then
    echo "Generating iOS Xcode project..."
    Tools/premake5 xcode4 --game-project="$PROJECT_DIR" --os=ios
else
    echo "Generating macOS Xcode project..."
    Tools/premake5 xcode4 --game-project="$PROJECT_DIR"
fi

# Post-process pbxproj (SKIP_INSTALL fix)
python3 -c "
import re, glob

for pbx in glob.glob('**/project.pbxproj', recursive=True):
    with open(pbx, 'r') as f:
        content = f.read()

    original = content

    if 'Runtime' in pbx or 'LumosEditor' in pbx:
        # App target: SKIP_INSTALL = NO
        content = re.sub(r'SKIP_INSTALL = \(\s*YES,?\s*\);', 'SKIP_INSTALL = NO;', content)
        content = re.sub(r'SKIP_INSTALL = YES;', 'SKIP_INSTALL = NO;', content)
        content = re.sub(r'INSTALL_PATH = .*?;', 'INSTALL_PATH = \"/Applications\";', content)
    else:
        content = re.sub(r'SKIP_INSTALL = \(\s*YES,?\s*\);', 'SKIP_INSTALL = YES;', content)

    if content != original:
        with open(pbx, 'w') as f:
            f.write(content)
        print(f'  Patched {pbx}')
"

if [ "$IOS_MODE" = true ]; then
    echo ""
    echo "=== iOS project generated ==="
    echo "Open in Xcode: $ROOT_DIR/Runtime/Runtime.xcodeproj"
else
    echo ""
    echo "Building macOS app (Production config)..."

    xcodebuild -project "$ROOT_DIR/Runtime/Runtime.xcodeproj" \
        -parallelizeTargets -jobs 4 \
        -configuration Production \
        -sdk macosx \
        CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO CODE_SIGNING_ALLOWED=NO \
        2>&1 | tail -20

    # Find built .app (PRODUCT_NAME sets .app name to project title on macOS)
    APP_PATH=$(find "$ROOT_DIR/bin" -name "*.app" -path "*Production*" -type d 2>/dev/null | head -1)
    if [ -z "$APP_PATH" ]; then
        APP_PATH=$(find "$ROOT_DIR/bin" -name "Runtime.app" -type d 2>/dev/null | head -1)
    fi

    if [ -n "$APP_PATH" ]; then
        echo ""
        echo "=== Build complete ==="
        echo "App: $APP_PATH"
        echo ""
        echo "To codesign for distribution:"
        echo "  codesign --deep --force --sign \"Developer ID Application: <Your Name>\" \"$APP_PATH\""
        echo "  xcrun notarytool submit \"$APP_PATH\" --apple-id <email> --team-id <team> --password <app-pw>"
    else
        echo ""
        echo "Build may have succeeded — check bin/ for .app output"
    fi
fi
