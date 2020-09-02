set msBuildDir=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin
cd build
"%msBuildDir%\MSBuild.exe" /p:Platform=x64 /p:Configuration=Debug Lumos.sln