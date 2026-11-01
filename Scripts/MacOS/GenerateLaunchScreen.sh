#!/bin/bash
# GenerateLaunchScreen.sh <path-to-.lmproj>
# Generates a customized iOS Launch Screen.storyboard from project settings
set -e

LMPROJ="$1"
if [ -z "$LMPROJ" ]; then
    echo "Usage: GenerateLaunchScreen.sh <path-to-.lmproj>"
    exit 1
fi

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_DIR="$SCRIPT_DIR/../.."
OUTPUT_DIR="$ROOT_DIR/bin-int/LaunchScreen"

# Splash colour and image from the .lmproj - the launch screen matches the engine splash that follows it
read -r BG_R BG_G BG_B SPLASH <<< $(python3 -c "
import json, os
p = '$LMPROJ'
v = json.load(open(p))
v = v.get('value0', v)
splash = v.get('SplashPath', '') or ''
if splash:
    splash = os.path.join(os.path.dirname(p), splash.replace('//Assets/', 'Assets/'))
print(v.get('SplashBGR', 0.0), v.get('SplashBGG', 0.023), v.get('SplashBGB', 0.169), splash or '-')
" 2>/dev/null)

BG_R="${BG_R:-0.0}"
BG_G="${BG_G:-0.023}"
BG_B="${BG_B:-0.169}"
if [ -z "$SPLASH" ] || [ "$SPLASH" = "-" ] || [ ! -f "$SPLASH" ]; then
    SPLASH="$ROOT_DIR/ExampleProject/Assets/Textures/splash.png"
fi

rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"
cp "$SPLASH" "$OUTPUT_DIR/LaunchSplash.png"

# Centred, half the width in portrait (the engine splash uses the same size)
cat > "$OUTPUT_DIR/Launch Screen.storyboard" << STORYBOARD_EOF
<?xml version="1.0" encoding="UTF-8"?>
<document type="com.apple.InterfaceBuilder3.CocoaTouch.Storyboard.XIB" version="3.0" toolsVersion="17506" targetRuntime="iOS.CocoaTouch" propertyAccessControl="none" useAutolayout="YES" launchScreen="YES" useTraitCollections="YES" useSafeAreas="YES" colorMatched="YES" initialViewController="01J-lp-oVM">
    <device id="retina6_1" orientation="portrait" appearance="light"/>
    <dependencies>
        <deployment identifier="iOS"/>
        <plugIn identifier="com.apple.InterfaceBuilder.IBCocoaTouchPlugin" version="17505"/>
        <capability name="Safe area layout guides" minToolsVersion="9.0"/>
        <capability name="documents saved in the Xcode 8 format" minToolsVersion="8.0"/>
    </dependencies>
    <scenes>
        <scene sceneID="EHf-IW-A2E">
            <objects>
                <viewController id="01J-lp-oVM" sceneMemberID="viewController">
                    <view key="view" contentMode="scaleToFill" id="Ze5-6b-2t3">
                        <rect key="frame" x="0.0" y="0.0" width="414" height="896"/>
                        <autoresizingMask key="autoresizingMask" widthSizable="YES" heightSizable="YES"/>
                        <subviews>
                            <imageView clipsSubviews="YES" userInteractionEnabled="NO" contentMode="scaleAspectFit" image="LaunchSplash.png" translatesAutoresizingMaskIntoConstraints="NO" id="QVp-xv-xe6">
                                <rect key="frame" x="103.5" y="344.5" width="207" height="207"/>
                                <constraints>
                                    <constraint firstAttribute="width" secondItem="QVp-xv-xe6" secondAttribute="height" multiplier="1:1" id="WtK-RP-Ypo"/>
                                </constraints>
                            </imageView>
                        </subviews>
                        <viewLayoutGuide key="safeArea" id="Bcu-3y-fUS"/>
                        <color key="backgroundColor" red="${BG_R}" green="${BG_G}" blue="${BG_B}" alpha="1" colorSpace="custom" customColorSpace="sRGB"/>
                        <constraints>
                            <constraint firstItem="QVp-xv-xe6" firstAttribute="centerX" secondItem="Ze5-6b-2t3" secondAttribute="centerX" id="cx1-aa-001"/>
                            <constraint firstItem="QVp-xv-xe6" firstAttribute="centerY" secondItem="Ze5-6b-2t3" secondAttribute="centerY" id="cy1-aa-002"/>
                            <constraint firstItem="QVp-xv-xe6" firstAttribute="width" secondItem="Ze5-6b-2t3" secondAttribute="width" multiplier="0.5" priority="750" id="w1a-aa-003"/>
                            <constraint firstItem="QVp-xv-xe6" firstAttribute="width" relation="lessThanOrEqual" secondItem="Ze5-6b-2t3" secondAttribute="height" multiplier="0.5" id="w2a-aa-004"/>
                        </constraints>
                    </view>
                </viewController>
                <placeholder placeholderIdentifier="IBFirstResponder" id="iYj-Kq-Ea1" userLabel="First Responder" sceneMemberID="firstResponder"/>
            </objects>
            <point key="canvasLocation" x="52" y="375"/>
        </scene>
    </scenes>
    <resources>
        <image name="LaunchSplash.png" width="512" height="512"/>
    </resources>
</document>
STORYBOARD_EOF

echo "Generated launch screen: $OUTPUT_DIR/Launch Screen.storyboard"
echo "  Background: rgb($BG_R, $BG_G, $BG_B)  Splash: $SPLASH"
