PATH="/usr/local/bin:$PATH"
cd build
#time make Sandbox -j4 config=release
xcodebuild -project Sandbox.xcodeproj -configuration Release CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO CODE_SIGNING_ALLOWED=NO
echo "Build Finished!"