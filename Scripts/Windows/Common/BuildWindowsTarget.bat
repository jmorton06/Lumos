@echo off
setlocal enabledelayedexpansion

if "%1"=="" (
    echo Please provide a target to build using the /target: switch.
    exit /b 1
)

set "target=%1"

for /f "usebackq tokens=*" %%i in (`vswhere -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do (
    pushd ..\..

    set "buildParameters=/p:Platform=x64 /p:Configuration=Release /p:DisableFastUpToDateCheck=1"

    "%%i" !buildParameters! "/target:%target%" -m
    set "errorlevel=!errorlevel!"

    popd

    exit /b !errorlevel!
)
