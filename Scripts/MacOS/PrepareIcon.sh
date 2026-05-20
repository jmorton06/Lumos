#!/bin/bash
# PrepareIcon.sh <source-png>
# Resizes source image to 1024x1024 and installs into xcassets for app icon
set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_DIR="$SCRIPT_DIR/../.."

SOURCE_PNG="$1"
if [ -z "$SOURCE_PNG" ]; then
    echo "Usage: PrepareIcon.sh <source-png> [output-dir]"
    exit 1
fi

# Optional output directory (default: engine xcassets)
ICON_DIR="${2:-$ROOT_DIR/Resources/AppIcons/Assets.xcassets/AppIcon.appiconset}"

if [ ! -f "$SOURCE_PNG" ]; then
    echo "Error: $SOURCE_PNG not found"
    exit 1
fi

mkdir -p "$ICON_DIR"

# Resize to 1024x1024 using sips
sips -z 1024 1024 "$SOURCE_PNG" --out "$ICON_DIR/AppIcon.png" > /dev/null 2>&1

# Write Contents.json with universal iOS + mac idioms
cat > "$ICON_DIR/Contents.json" << 'EOF'
{
  "images" : [
    {
      "filename" : "AppIcon.png",
      "idiom" : "universal",
      "platform" : "ios",
      "size" : "1024x1024"
    },
    {
      "filename" : "AppIcon.png",
      "idiom" : "mac",
      "scale" : "1x",
      "size" : "512x512"
    },
    {
      "filename" : "AppIcon.png",
      "idiom" : "mac",
      "scale" : "2x",
      "size" : "512x512"
    }
  ],
  "info" : {
    "author" : "Lumos",
    "version" : 1
  }
}
EOF

echo "Icon prepared: $ICON_DIR/AppIcon.png"
