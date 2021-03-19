PATH="/usr/local/bin:$PATH"
cd build
xcodebuild -project Sandbox.xcodeproj -configuration Release -parallelizeTargets -jobs 4 -sdk macosx -arch x86_64 | xcpretty
echo "Build Finished!"