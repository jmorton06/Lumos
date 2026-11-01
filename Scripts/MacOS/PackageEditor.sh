#!/bin/bash
# PackageEditor.sh [--skip-build] [--sign="Developer ID Application: Name (TEAM)"] [--notarize=<profile>]
#                  [--templates=all|none] [--build-templates] [--project=<dir>|none]
# A distributable Lumos editor: LumosEditor.app with the engine assets, the export templates and
# the docs inside, so people can make and export projects without the engine source. Output:
# bin/Distribution/Lumos Editor.app + Lumos-<version>-macOS.dmg
#
#   --skip-build       use the editor already in bin/ (default: a universal Release build)
#   --build-templates  build any missing macOS / iOS templates first (lumos templates build)
#   --templates=none   leave the templates out (much smaller; users install them themselves)
#   --sign             default "-" (ad hoc: runs on this Mac only)
#   --notarize         notarytool keychain profile (needs a Developer ID --sign)
#   --project          project opened on first run, copied into Contents/Resources (default LearnProject)
set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT="$( cd "$SCRIPT_DIR/../.." && pwd )"
SKIP_BUILD=false
BUILD_TEMPLATES=false
TEMPLATES=all
SIGN="-"
NOTARY=""
PROJECT=LearnProject
for arg in "$@"; do
    case "$arg" in
        --skip-build) SKIP_BUILD=true ;;
        --build-templates) BUILD_TEMPLATES=true ;;
        --templates=*) TEMPLATES="${arg#--templates=}" ;;
        --sign=*) SIGN="${arg#--sign=}" ;;
        --notarize=*) NOTARY="${arg#--notarize=}" ;;
        --project=*) PROJECT="${arg#--project=}" ;;
        *) echo "Unknown option $arg"; exit 1 ;;
    esac
done
cd "$ROOT"

VERSION=$(sed -n 's/.*int major = \([0-9]*\);.*/\1/p;s/.*int minor = \([0-9]*\);.*/\1/p;s/.*int patch = \([0-9]*\);.*/\1/p' Lumos/Source/Lumos/Core/Version.h | paste -sd. -)
echo "== Lumos editor $VERSION"

if [ "$SKIP_BUILD" = false ]; then
    echo "== Building the editor (Release, Apple Silicon + Intel)"
    Tools/premake5 xcode4 --arch=universal > /dev/null
    xcodebuild -project Editor/LumosEditor.xcodeproj -scheme LumosEditor -configuration Release \
        -destination "generic/platform=macOS" -parallelizeTargets -jobs "$(sysctl -n hw.ncpu)" -quiet \
        CODE_SIGN_IDENTITY= CODE_SIGNING_REQUIRED=NO CODE_SIGNING_ALLOWED=NO COMPILER_INDEX_STORE_ENABLE=NO
fi
# the most recently built editor binary (bin/ can hold more than one architecture's)
EDITOR=$(ls -t bin/Release-macosx-*/LumosEditor.app/Contents/MacOS/LumosEditor 2>/dev/null | head -1)
EDITOR="${EDITOR%/Contents/MacOS/LumosEditor}"
[ -d "$EDITOR" ] || { echo "No Release editor build in bin/"; exit 1; }
echo "   using $EDITOR ($(lipo -archs "$EDITOR/Contents/MacOS/LumosEditor"))"

if [ "$BUILD_TEMPLATES" = true ]; then
    for p in macos ios; do
        for c in release debug; do
            Scripts/lumos templates list | grep -qE "^  $p +$c +installed" || Scripts/lumos templates build --platform=$p --config=$c
        done
    done
fi

OUT="$ROOT/bin/Distribution"
APP="$OUT/Lumos Editor.app"
rm -rf "$APP"
mkdir -p "$OUT"
echo "== Staging $APP"
ditto "$EDITOR" "$APP"
RES="$APP/Contents/Resources"
rm -f "$RES/imgui.ini" "$RES/pipeline_cache.bin"

# engine assets (shaders etc.) where Application::SetEngineAssetPath looks for them in a bundle
mkdir -p "$RES/EngineAssets"
rsync -a --exclude ExportTemplates Lumos/Assets/ "$RES/EngineAssets/"

# the starter project: the editor copies it out of the bundle on first run (FindBundledProject)
if [ "$PROJECT" != none ]; then
    PROJECT="${PROJECT%/}"
    ls "$PROJECT"/*.lmproj > /dev/null 2>&1 || { echo "No .lmproj in $PROJECT"; exit 1; }
    NAME=$(basename "$PROJECT")
    rsync -a --exclude Tests --exclude Builds --exclude .lumos --exclude 'Assets/Cache' --exclude 'Assets/Imported' \
        --exclude pipeline_cache.bin --exclude 'EditorDock*.json' --exclude imgui.ini --exclude .gitignore \
        "$PROJECT/" "$RES/$NAME/"
    echo "   project: $NAME ($(du -sh "$RES/$NAME" | cut -f1))"
fi

# docs for Help > Packaging Guide
mkdir -p "$RES/Docs"
cp Docs/*.md "$RES/Docs/"

# export templates: Export::TemplatesRoot() finds Contents/Resources/Templates
if [ "$TEMPLATES" != none ]; then
    if [ ! -d "Templates/$VERSION" ]; then
        echo "   no templates for $VERSION (build them, or use --build-templates / --templates=none)"
        exit 1
    fi
    mkdir -p "$RES/Templates"
    ditto "Templates/$VERSION" "$RES/Templates/$VERSION"
    # debug info in the static libs (iOS, Mac App Store) is most of their size; exports don't need it
    for lib in "$RES"/Templates/"$VERSION"/ios/*/lib/*.a "$RES"/Templates/"$VERSION"/macos/*/store/lib/*.a; do
        [ -f "$lib" ] && xcrun strip -S "$lib" 2> /dev/null || true
    done
    echo "   templates: $(cd "$RES/Templates/$VERSION" && ls -d */* | tr '\n' ' ')"
fi

echo "== Signing ($SIGN)"
if [ "$SIGN" = "-" ]; then
    codesign --force --deep --sign - "$APP"
else
    # hardened runtime for notarising; the editor isn't sandboxed (it runs xcodebuild / codesign for exports)
    codesign --force --deep --options runtime --timestamp \
        --entitlements Editor/LumosEditor-Distribution.entitlements --sign "$SIGN" "$APP"
fi
codesign --verify --deep "$APP"

DMG="$OUT/Lumos-$VERSION-macOS.dmg"
rm -f "$DMG"
echo "== $DMG"
STAGE=$(mktemp -d)
ditto "$APP" "$STAGE/Lumos Editor.app"
ln -s /Applications "$STAGE/Applications"
hdiutil create -quiet -volname "Lumos $VERSION" -srcfolder "$STAGE" -ov -format UDZO "$DMG"
rm -rf "$STAGE"
if [ "$SIGN" != "-" ]; then
    codesign --force --sign "$SIGN" "$DMG"
fi

if [ -n "$NOTARY" ]; then
    [ "$SIGN" = "-" ] && { echo "Notarising needs a Developer ID --sign"; exit 1; }
    echo "== Notarising (a few minutes)"
    xcrun notarytool submit "$DMG" --keychain-profile "$NOTARY" --wait
    xcrun stapler staple "$DMG"
fi

du -sh "$APP" "$DMG"
echo "Done: $DMG"
