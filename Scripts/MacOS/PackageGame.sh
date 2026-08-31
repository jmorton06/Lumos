#!/bin/bash
# PackageGame.sh <project-dir> [--ios]
# Builds a standalone .app (macOS) or generates iOS Xcode project from a Lumos game project
set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_DIR="$SCRIPT_DIR/../.."

# Parse args
PROJECT_DIR=""
IOS_MODE=false
SHADERC=""

for arg in "$@"; do
    case "$arg" in
        --ios) IOS_MODE=true ;;
        --shaderc) SHADERC="1" ;;
        *) PROJECT_DIR="$arg" ;;
    esac
done

if [ -z "$PROJECT_DIR" ]; then
    echo "Usage: PackageGame.sh <project-dir> [--ios] [--shaderc]"
    echo ""
    echo "  <project-dir>  Path to game project directory (contains .lmproj)"
    echo "  --ios          Generate iOS Xcode project instead of building macOS .app"
    echo "  --shaderc      Enable runtime shader compilation"
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

# Generate icons (reads IconPath from .lmproj, falls back to common locations)
ICON_PLATFORM="macos"
if [ "$IOS_MODE" = true ]; then
    ICON_PLATFORM="ios"
fi
echo "Generating icons..."
"$SCRIPT_DIR/../GenerateIcons.sh" "$PROJECT_DIR" --platform="$ICON_PLATFORM"

# Generate customized iOS launch screen
"$SCRIPT_DIR/GenerateLaunchScreen.sh" "$LMPROJ"

cd "$ROOT_DIR"

# Pack assets into .lpak — needs a macOS Runtime binary. Builds one on demand if missing,
# since iOS-only build pipelines wipe the macOS bin/ tree.
PACK_OUTPUT="$PROJECT_DIR/Assets.lpak"
RUNTIME_BIN=$(find "$ROOT_DIR/bin" -path "*macosx*" -name "Runtime" -type f -perm -u+x 2>/dev/null | head -1)

if [ -z "$RUNTIME_BIN" ] || [ ! -f "$RUNTIME_BIN" ]; then
    echo "macOS Runtime missing — building it for asset packing..."

    # Regenerate Xcode projects for macOS. iOS pass later in this script overwrites them again.
    echo "  Generating macOS Xcode project..."
    ( cd "$ROOT_DIR" && Tools/premake5 xcode4 )

    HOST_ARCH=$(uname -m)
    ( cd "$ROOT_DIR" && xcodebuild -project Runtime/Runtime.xcodeproj \
        -parallelizeTargets -jobs 4 -configuration Release \
        -sdk macosx -arch "$HOST_ARCH" \
        CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO CODE_SIGNING_ALLOWED=NO \
        2>&1 | tail -20 )

    RUNTIME_BIN=$(find "$ROOT_DIR/bin" -path "*macosx*" -name "Runtime" -type f -perm -u+x 2>/dev/null | head -1)
fi

if [ -n "$RUNTIME_BIN" ] && [ -f "$RUNTIME_BIN" ]; then
    # Bake .lmesh caches for any source model (obj/gltf/glb/fbx) before packing.
    # AssetImporter writes to //Assets/Imported/ on first load of a given mesh -
    # fine on desktop, but the iOS app bundle is read-only, so any mesh never
    # seen on a writable filesystem before crashes on-device. Runs headless,
    # no scene/window needed.
    echo "Importing source models via $RUNTIME_BIN ..."
    "$RUNTIME_BIN" --project="$PROJECT_DIR" --import-assets \
        || { echo "Error: asset import failed"; exit 1; }

    echo "Packing assets via $RUNTIME_BIN ..."
    "$RUNTIME_BIN" --project="$PROJECT_DIR" --pack-assets="$PACK_OUTPUT" --embed-engine-shaders \
        || { echo "Error: asset packing failed"; exit 1; }
else
    echo "Error: failed to obtain a macOS Runtime binary for packing"
    exit 1
fi

# Run premake
PREMAKE_ARGS="--game-project=\"$PROJECT_DIR\""
if [ -n "$SHADERC" ]; then
    PREMAKE_ARGS="$PREMAKE_ARGS --shaderc"
fi

if [ "$IOS_MODE" = true ]; then
    echo "Generating iOS Xcode project..."
    eval "Tools/premake5 xcode4 $PREMAKE_ARGS --os=ios"
else
    echo "Generating macOS Xcode project..."
    eval "Tools/premake5 xcode4 $PREMAKE_ARGS"
fi

# Post-process pbxproj (SKIP_INSTALL fix + orientation array split)
python3 -c "
import re, glob

# Premake serialises a Lua string with spaces as one quoted element in an array.
# Split any UISupportedInterfaceOrientations entry that still holds a joined
# string into proper per-orientation array elements so iOS honours the lock.
ORIENT_KEY = re.compile(r'(INFOPLIST_KEY_UISupportedInterfaceOrientations(?:~ipad)?)\s*=\s*\(\s*((?:\"[^\"]*\",?\s*)+)\)\s*;')

def fix_orient(m):
    key = m.group(1)
    body = m.group(2)
    tokens = []
    for s in re.findall(r'\"([^\"]*)\"', body):
        for t in s.split():
            t = t.strip()
            if t and t.startswith('UIInterfaceOrientation'):
                tokens.append(t)
    # Dedup preserving order
    seen = set()
    out = []
    for t in tokens:
        if t not in seen:
            seen.add(t)
            out.append(t)
    if not out:
        return m.group(0)
    inner = ',\n\t\t\t\t\t'.join('\"%s\"' % t for t in out)
    return '%s = (\n\t\t\t\t\t%s,\n\t\t\t\t);' % (key, inner)

for pbx in glob.glob('**/project.pbxproj', recursive=True):
    with open(pbx, 'r') as f:
        content = f.read()

    original = content

    if 'Runtime' in pbx or 'LumosEditor' in pbx:
        # App target: SKIP_INSTALL = NO
        content = re.sub(r'SKIP_INSTALL = \(\s*YES,?\s*\);', 'SKIP_INSTALL = NO;', content)
        content = re.sub(r'SKIP_INSTALL = YES;', 'SKIP_INSTALL = NO;', content)
        content = re.sub(r'INSTALL_PATH = .*?;', 'INSTALL_PATH = \"/Applications\";', content)
        content = ORIENT_KEY.sub(fix_orient, content)
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
