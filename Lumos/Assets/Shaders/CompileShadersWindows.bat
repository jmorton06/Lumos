
@echo off
setLocal enableExtensions enableDelayedExpansion

dir=$(cd -P -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)

set COMPILER=C:/VulkanSDK/1.3.204.1/Bin/glslc.exe
set DSTDIR=CompiledSPV

echo Compiling Shaders to spv

if not exist "%DSTDIR%" (
  mkdir "%DSTDIR%"
)

for %%f in (*.vert *.frag *.comp) do (

  set SRC=%%f

  set DST=%DSTDIR%\!SRC:.vert=.vert.spv!
  set DST=!DST:.frag=.frag.spv!
  set DST=!DST:.comp=.comp.spv!

  call :getModifiedDate "!SRC!"
  set SRC_DATE=!DATE_MODIFIED!

  call :getModifiedDate "!DST!"
  set DST_DATE=!DATE_MODIFIED!

  if "!SRC_DATE!" gtr "!DST_DATE!" (
    echo Compiling
    %COMPILER% "!SRC!" -o "!DST!"

  )
)
pause
endlocal
goto :EOF

:getModifiedDate
  set FILE=%~f1
  set FILE=%FILE:\=\\%
  if NOT EXIST %~1 (
	set DATE_MODIFIED=0
	goto:done
  )

  for /F "skip=1 delims=. usebackq" %%D in (`wmic datafile where name^="%FILE%" get LastModified`) do (
    set DATE_MODIFIED=%%D
    goto done
  )
  :done
exit /B
pause