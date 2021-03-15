PATH="/usr/local/bin:$PATH"
cd build
#time make Sandbox -j4 config=release
# -destination 'platform=macOS,arch=x86_64'
xcodebuild -workspace Lumos.xcworkspace -scheme Sandbox -configuration Release -parallelizeTargets -jobs 4 | xcpretty
echo "Build Finished!"