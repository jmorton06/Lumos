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

# Extract title and splash color from .lmproj
read -r TITLE BG_R BG_G BG_B <<< $(python3 -c "
import json, sys, os
with open('$LMPROJ') as f:
    data = json.load(f)
v = data.get('value0', data)
title = v.get('Title', '')
if not title:
    title = os.path.basename(os.path.dirname('$LMPROJ'))
r = v.get('SplashBGR', 0.0)
g = v.get('SplashBGG', 0.023)
b = v.get('SplashBGB', 0.169)
print(f'{title}\t{r}\t{g}\t{b}')
" 2>/dev/null | tr '\t' ' ')

# Fallbacks
TITLE="${TITLE:-Game}"
BG_R="${BG_R:-0.0}"
BG_G="${BG_G:-0.023}"
BG_B="${BG_B:-0.169}"

mkdir -p "$OUTPUT_DIR"

# Copy Info.plist from original iOS client dir
cp "$ROOT_DIR/Lumos/Source/Lumos/Platform/iOS/Client/Info.plist" "$OUTPUT_DIR/Info.plist"

cat > "$OUTPUT_DIR/Launch Screen.storyboard" << STORYBOARD_EOF
<?xml version="1.0" encoding="UTF-8"?>
<document type="com.apple.InterfaceBuilder3.CocoaTouch.Storyboard.XIB" version="3.0" toolsVersion="17506" targetRuntime="iOS.CocoaTouch" propertyAccessControl="none" useAutolayout="YES" launchScreen="YES" useTraitCollections="YES" useSafeAreas="YES" colorMatched="YES" initialViewController="01J-lp-oVM">
    <device id="retina6_1" orientation="portrait" appearance="light"/>
    <dependencies>
        <deployment identifier="iOS"/>
        <plugIn identifier="com.apple.InterfaceBuilder.IBCocoaTouchPlugin" version="17505"/>
        <capability name="Safe area layout guides" minToolsVersion="9.0"/>
        <capability name="System colors in document resources" minToolsVersion="11.0"/>
        <capability name="documents saved in the Xcode 8 format" minToolsVersion="8.0"/>
    </dependencies>
    <scenes>
        <!--View Controller-->
        <scene sceneID="EHf-IW-A2E">
            <objects>
                <viewController id="01J-lp-oVM" sceneMemberID="viewController">
                    <view key="view" contentMode="scaleToFill" id="Ze5-6b-2t3">
                        <rect key="frame" x="0.0" y="0.0" width="414" height="896"/>
                        <autoresizingMask key="autoresizingMask" widthSizable="YES" heightSizable="YES"/>
                        <subviews>
                            <label opaque="NO" clipsSubviews="YES" userInteractionEnabled="NO" contentMode="left" horizontalHuggingPriority="251" verticalHuggingPriority="251" text="${TITLE}" textAlignment="center" lineBreakMode="middleTruncation" baselineAdjustment="alignBaselines" minimumFontSize="18" translatesAutoresizingMaskIntoConstraints="NO" id="GJd-Yh-RWb">
                                <rect key="frame" x="0.0" y="147" width="414" height="131"/>
                                <fontDescription key="fontDescription" type="boldSystem" pointSize="48"/>
                                <color key="textColor" white="1" alpha="1" colorSpace="custom" customColorSpace="genericGamma22GrayColorSpace"/>
                                <color key="highlightedColor" systemColor="systemBackgroundColor"/>
                            </label>
                            <imageView clipsSubviews="YES" userInteractionEnabled="NO" contentMode="scaleAspectFit" horizontalHuggingPriority="251" verticalHuggingPriority="251" image="icon.png" translatesAutoresizingMaskIntoConstraints="NO" id="QVp-xv-xe6">
                                <rect key="frame" x="79" y="320" width="256" height="256"/>
                                <constraints>
                                    <constraint firstAttribute="width" secondItem="QVp-xv-xe6" secondAttribute="height" multiplier="1:1" id="WtK-RP-Ypo"/>
                                </constraints>
                            </imageView>
                        </subviews>
                        <viewLayoutGuide key="safeArea" id="Bcu-3y-fUS"/>
                        <color key="backgroundColor" red="${BG_R}" green="${BG_G}" blue="${BG_B}" alpha="1" colorSpace="custom" customColorSpace="sRGB"/>
                        <constraints>
                            <constraint firstItem="GJd-Yh-RWb" firstAttribute="leading" secondItem="Ze5-6b-2t3" secondAttribute="leading" id="2GV-f7-yNg"/>
                            <constraint firstAttribute="trailing" secondItem="GJd-Yh-RWb" secondAttribute="trailing" id="3eh-lJ-j5q"/>
                            <constraint firstItem="Bcu-3y-fUS" firstAttribute="bottom" secondItem="QVp-xv-xe6" secondAttribute="bottom" constant="286" id="791-uY-me5"/>
                            <constraint firstItem="QVp-xv-xe6" firstAttribute="top" secondItem="GJd-Yh-RWb" secondAttribute="bottom" constant="42" id="J4w-lH-7yK"/>
                            <constraint firstItem="GJd-Yh-RWb" firstAttribute="top" secondItem="Bcu-3y-fUS" secondAttribute="top" constant="103" id="Tsu-wg-jwm"/>
                            <constraint firstItem="GJd-Yh-RWb" firstAttribute="centerX" secondItem="QVp-xv-xe6" secondAttribute="centerX" id="YFx-X1-D0D"/>
                            <constraint firstItem="QVp-xv-xe6" firstAttribute="centerY" secondItem="Ze5-6b-2t3" secondAttribute="centerY" id="mvz-A1-bp4"/>
                        </constraints>
                    </view>
                </viewController>
                <placeholder placeholderIdentifier="IBFirstResponder" id="iYj-Kq-Ea1" userLabel="First Responder" sceneMemberID="firstResponder"/>
            </objects>
            <point key="canvasLocation" x="52.173913043478265" y="375"/>
        </scene>
    </scenes>
    <resources>
        <image name="icon.png" width="1024" height="1024"/>
        <systemColor name="systemBackgroundColor">
            <color white="1" alpha="1" colorSpace="custom" customColorSpace="genericGamma22GrayColorSpace"/>
        </systemColor>
    </resources>
</document>
STORYBOARD_EOF

echo "Generated launch screen: $OUTPUT_DIR/Launch Screen.storyboard"
echo "  Title: $TITLE"
echo "  Background: rgb($BG_R, $BG_G, $BG_B)"
