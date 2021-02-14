PATH="/usr/local/bin:$PATH"
cd build
#time make Sandbox -j4 config=release
xcodebuild -project Sandbox.xcodeproj -parallelizeTargets -jobs 4 -configuration Release CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO CODE_SIGNING_ALLOWED=NO -sdk macosx -destination generic/platform=macOS 'platform=macOS,arch=x86_64' | xcpretty
echo "Build Finished!"