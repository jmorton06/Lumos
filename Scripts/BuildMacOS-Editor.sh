PATH="/usr/local/bin:$PATH"
cd build
xcodebuild -project LumosEditor.xcodeproj -parallelizeTargets -jobs 4 -configuration Release CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO CODE_SIGNING_ALLOWED=NO -sdk macosx -arch x86_64 | xcpretty
echo "Build Finished!"