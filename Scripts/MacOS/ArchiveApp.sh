#!/bin/bash
# Superseded by the export system: Scripts/lumos export|deploy (or Project > Export... in the editor),
# which uses prebuilt templates instead of regenerating the Runtime project per game. See Docs/Packaging.md.
# ArchiveApp.sh <project-dir> [--ios] [--export]
# App Store build of a Lumos game: packs the assets, generates the Xcode project, archives it
# (Production, signed with the team's automatic signing) and, with --export, exports an
# App Store Connect package (needs an Apple Distribution certificate - Xcode can create one).
# Output: bin/Archives/<Title>-<platform>.xcarchive (+ bin/Archives/<Title>-<platform>/ on export)
set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_DIR="$( cd "$SCRIPT_DIR/../.." && pwd )"
TEAM="${DEVELOPMENT_TEAM:-C5L4T5BF6L}"

PROJECT_DIR=""
IOS=false
EXPORT=false
for arg in "$@"; do
    case "$arg" in
        --ios) IOS=true ;;
        --export) EXPORT=true ;;
        *) PROJECT_DIR="$arg" ;;
    esac
done
if [ -z "$PROJECT_DIR" ]; then
    echo "Usage: ArchiveApp.sh <project-dir> [--ios] [--export]"
    exit 1
fi
[[ "$PROJECT_DIR" != /* ]] && PROJECT_DIR="$ROOT_DIR/$PROJECT_DIR"
LMPROJ=$(find "$PROJECT_DIR" -maxdepth 1 -name "*.lmproj" -print -quit)
TITLE=$(python3 -c "import json;v=json.load(open('$LMPROJ'));v=v.get('value0',v);print(v.get('Title') or '')")
[ -z "$TITLE" ] && TITLE=$(basename "$PROJECT_DIR")

if [ "$IOS" = true ]; then
    "$SCRIPT_DIR/PackageGame.sh" "$PROJECT_DIR" --ios
    PLATFORM=iOS
    DEST="generic/platform=iOS"
else
    "$SCRIPT_DIR/PackageGame.sh" "$PROJECT_DIR" --no-build
    PLATFORM=macOS
    DEST="generic/platform=macOS"
fi

ARCHIVES="$ROOT_DIR/bin/Archives"
ARCHIVE="$ARCHIVES/$TITLE-$PLATFORM.xcarchive"
mkdir -p "$ARCHIVES"
rm -rf "$ARCHIVE"

echo "Archiving $TITLE for $PLATFORM..."
xcodebuild -project "$ROOT_DIR/Runtime/Runtime.xcodeproj" -scheme Runtime -configuration Production \
    -destination "$DEST" -archivePath "$ARCHIVE" -allowProvisioningUpdates \
    DEVELOPMENT_TEAM="$TEAM" CODE_SIGN_STYLE=Automatic CODE_SIGN_IDENTITY="Apple Development" \
    archive | tail -5
[ -d "$ARCHIVE" ] || { echo "Archive failed"; exit 1; }
echo "Archive: $ARCHIVE"

if [ "$EXPORT" = true ]; then
    OPTS="$ARCHIVES/ExportOptions-$PLATFORM.plist"
    cat > "$OPTS" << PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>method</key><string>app-store-connect</string>
    <key>teamID</key><string>$TEAM</string>
    <key>signingStyle</key><string>automatic</string>
    <key>destination</key><string>export</string>
    <key>uploadSymbols</key><true/>
</dict>
</plist>
PLIST
    rm -rf "$ARCHIVES/$TITLE-$PLATFORM"
    xcodebuild -exportArchive -archivePath "$ARCHIVE" -exportOptionsPlist "$OPTS" \
        -exportPath "$ARCHIVES/$TITLE-$PLATFORM" -allowProvisioningUpdates | tail -5
    echo "Exported: $ARCHIVES/$TITLE-$PLATFORM"
    echo "Upload with Xcode's Organizer, Transporter, or: xcrun altool --upload-app ..."
fi
