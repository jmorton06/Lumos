cd build
make $* -j4 config=release

cd ../bin/Release
./Sandbox.app
