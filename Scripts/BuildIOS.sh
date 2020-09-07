Tools/premake5 xcode4 --os=ios
xcodebuild -project build/Sandbox.xcodeproj CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO CODE_SIGNING_ALLOWED=NO 
echo "Build Finished!"