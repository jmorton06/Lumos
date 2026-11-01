#!/bin/bash
# DeployEditor-iOS.sh [--project=<project dir>] [--starter=<project dir>] [--device=<name or id>] [--debug]
#                     [--no-install] [--install-only] [--archive [--upload]]
# Builds the editor for iOS with a project inside it (ExampleProject by default), installs it on a
# connected device and starts it. A project of your own gets its own app: <bundle id>.editor.
# --starter bundles a project but keeps the Lumos editor's own name / bundle id (the store build).
# --archive = App Store archive + .ipa in bin/Distribution/iOS (starter LearnProject unless given);
# --upload sends it straight to App Store Connect.
set -eo pipefail

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT="$( cd "$DIR/../.." && pwd )"
TEAM="${LUMOS_TEAM_ID:-C5L4T5BF6L}"

PROJECT=""
STARTER=""
ARCHIVE=0
UPLOAD=0
DEVICE=""
CONFIG="Release"
INSTALL=1
BUILD=1
for arg in "$@"; do
  case $arg in
    --project=*) PROJECT="${arg#--project=}" ;;
    --starter=*) STARTER="${arg#--starter=}" ;;
    --archive) ARCHIVE=1 ;;
    --upload) UPLOAD=1 ;;
    --device=*) DEVICE="${arg#--device=}" ;;
    --debug) CONFIG="Debug" ;;
    --no-install) INSTALL=0 ;;
    --install-only) BUILD=0 ;;
  esac
done

cd "$ROOT"

[ "$ARCHIVE" -eq 1 ] && [ -z "$PROJECT" ] && [ -z "$STARTER" ] && STARTER="LearnProject"
# the default ExampleProject goes in as a clean copy too: its export builds (.lumos, Builds) ran to gigabytes
[ -z "$PROJECT" ] && [ -z "$STARTER" ] && STARTER="ExampleProject"
if [ -n "$STARTER" ]; then
  STARTER="${STARTER%/}"
  ls "$STARTER"/*.lmproj > /dev/null 2>&1 || { echo "Error: no .lmproj in $STARTER"; exit 1; }
  # a clean copy: no test output, caches or builds in the app. A store build also leaves out the imported
  # meshes (made again on the device); a development one keeps them so the device skips the import.
  IMPORTED_EXCLUDE="--exclude=Assets/Imported"
  [ "$ARCHIVE" -eq 0 ] && IMPORTED_EXCLUDE="--exclude=*.lods"
  STAGE="$ROOT/bin-int/Starter/$(basename "$STARTER")"
  rm -rf "$STAGE"; mkdir -p "$STAGE"
  rsync -a --exclude Tests --exclude Builds --exclude .lumos --exclude 'Assets/Cache' "$IMPORTED_EXCLUDE" \
    --exclude pipeline_cache.bin --exclude 'EditorDock*.json' --exclude imgui.ini --exclude .gitignore "$STARTER/" "$STAGE/"
  STARTER="$STAGE"
fi

if [ "$BUILD" -eq 1 ]; then
# the launch screen matches the splash of the project that opens
LMPROJ=""
if [ -n "$PROJECT" ]; then
  PROJECT="${PROJECT%/}"
  [ -f "$PROJECT" ] && PROJECT="$(dirname "$PROJECT")"
  LMPROJ=$(ls "$PROJECT"/*.lmproj 2>/dev/null | head -1)
  [ -z "$LMPROJ" ] && { echo "Error: no .lmproj in $PROJECT"; exit 1; }
elif [ -n "$STARTER" ]; then
  LMPROJ=$(ls "$STARTER"/*.lmproj | head -1)
else
  LMPROJ="$ROOT/ExampleProject/Example.lmproj"
fi
"$DIR/GenerateLaunchScreen.sh" "$LMPROJ"

# back to the macOS projects whatever happens: the iOS ones replace them
restore() { ( cd "$ROOT" && Tools/premake5 xcode4 > /dev/null 2>&1 ) || true; }
trap restore EXIT

if [ -n "$PROJECT" ]; then
  Tools/premake5 --os=ios --game-project="$PROJECT" xcode4
elif [ -n "$STARTER" ]; then
  Tools/premake5 --os=ios --starter-project="$STARTER" xcode4
else
  Tools/premake5 --os=ios xcode4
fi

if [ "$ARCHIVE" -eq 1 ]; then
  OUT="$ROOT/bin/Distribution/iOS"
  ARCH="$OUT/LumosEditor.xcarchive"
  rm -rf "$ARCH"; mkdir -p "$OUT"
  echo "Archiving LumosEditor for the App Store..."
  LOG="$ROOT/bin-int/ArchiveEditor-iOS.log"
  xcodebuild archive -project Editor/LumosEditor.xcodeproj -scheme LumosEditor -configuration Release \
    -destination "generic/platform=iOS" -archivePath "$ARCH" -allowProvisioningUpdates \
    DEVELOPMENT_TEAM="$TEAM" CODE_SIGN_STYLE=Automatic DEBUG_INFORMATION_FORMAT=dwarf-with-dsym > "$LOG" 2>&1 \
    || { grep -E "error:" "$LOG" | tail -20; echo "Error: archive failed (log: $LOG)"; exit 1; }
  DEST=export; [ "$UPLOAD" -eq 1 ] && DEST=upload
  cat > "$OUT/ExportOptions.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>method</key><string>app-store-connect</string>
    <key>teamID</key><string>$TEAM</string>
    <key>signingStyle</key><string>automatic</string>
    <key>destination</key><string>$DEST</string>
    <key>uploadSymbols</key><true/>
</dict>
</plist>
PLIST
  xcodebuild -exportArchive -archivePath "$ARCH" -exportOptionsPlist "$OUT/ExportOptions.plist" -exportPath "$OUT" -allowProvisioningUpdates
  ls "$OUT"/*.ipa 2>/dev/null || true
  exit 0
fi

echo "Building LumosEditor ($CONFIG) for iOS..."
LOG="$ROOT/bin-int/DeployEditor-iOS.log"
set +e
xcodebuild -project Editor/LumosEditor.xcodeproj -scheme LumosEditor -configuration "$CONFIG" \
  -destination "generic/platform=iOS" -allowProvisioningUpdates \
  DEVELOPMENT_TEAM="$TEAM" CODE_SIGN_STYLE=Automatic build > "$LOG" 2>&1
CODE=$?
set -e
grep -E "error:|BUILD (SUCCEEDED|FAILED)" "$LOG" | tail -30 || true
[ "$CODE" -eq 0 ] || { echo "Error: build failed (log: $LOG)"; exit 1; }

fi

APP=$(ls -td "$ROOT"/bin/"$CONFIG"-ios-*/*.app 2>/dev/null | grep -i editor | head -1)
[ -z "$APP" ] && { echo "Error: no editor .app in bin/"; exit 1; }
echo "Built $APP"
[ "$INSTALL" -eq 1 ] || exit 0

if [ -z "$DEVICE" ]; then
  JSON=$(mktemp)
  xcrun devicectl list devices --json-output "$JSON" > /dev/null 2>&1 || true
  DEVICE=$(python3 - "$JSON" <<'PY'
import json, sys
try:
    devices = json.load(open(sys.argv[1]))["result"]["devices"]
except Exception:
    devices = []
live = [d for d in devices if d.get("connectionProperties", {}).get("tunnelState") in ("connected", "disconnected")
        and d.get("hardwareProperties", {}).get("platform") == "iOS"
        and d.get("connectionProperties", {}).get("pairingState") == "paired"]
live.sort(key=lambda d: d["connectionProperties"].get("tunnelState") != "connected")
print(live[0]["identifier"] if live else "")
PY
)
  rm -f "$JSON"
fi
[ -z "$DEVICE" ] && { echo "Error: no paired iOS device found (pass --device=)"; exit 1; }

BUNDLE_ID=$(/usr/libexec/PlistBuddy -c "Print CFBundleIdentifier" "$APP/Info.plist")
echo "Installing $BUNDLE_ID on $DEVICE..."
xcrun devicectl device install app --device "$DEVICE" "$APP"
xcrun devicectl device process launch --terminate-existing --device "$DEVICE" "$BUNDLE_ID" \
  || echo "Installed, but could not start it (unlock the device and open it by hand)"
