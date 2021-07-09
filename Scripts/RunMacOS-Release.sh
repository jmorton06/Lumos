cd build
make $* -j4 config=release

cd ../bin/Release
./Runtime.app
