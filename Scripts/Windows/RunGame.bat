@echo off
REM RunGame.bat - launch the Runtime against a game project of your choosing.
REM
REM Usage:
REM   RunGame.bat                      Pick a project interactively
REM   RunGame.bat ExampleProject       Run that project directly
REM   RunGame.bat --config=Release     Pin the build config
REM
REM With no project argument it lists every .lmproj in the tree and prompts.
REM The last choice is remembered in .zed\.last-game-project.
setlocal enabledelayedexpansion

set "ROOT=%~dp0..\.."
pushd "%ROOT%"
set "ROOT=%CD%"
set "LAST_FILE=%ROOT%\.zed\.last-game-project"

set "CONFIG="
set "PROJECT_ARG="

:parse_args
if "%~1"=="" goto args_done
set "arg=%~1"
if "!arg:~0,9!"=="--config=" (
    set "CONFIG=!arg:~9!"
) else (
    set "PROJECT_ARG=!arg!"
)
shift
goto parse_args
:args_done

REM ------------------------------------------------------------- pick project

if not "%PROJECT_ARG%"=="" (
    call :resolve_project "%PROJECT_ARG%"
    if errorlevel 1 (
        echo Error: no .lmproj found at '%PROJECT_ARG%'
        popd & exit /b 1
    )
    goto have_project
)

set "DEFAULT="
if exist "%LAST_FILE%" set /p DEFAULT=<"%LAST_FILE%"
if not "!DEFAULT!"=="" if not exist "!DEFAULT!" set "DEFAULT="

echo Lumos projects found:
echo.

set "COUNT=0"
for /f "delims=" %%f in ('dir /s /b "%ROOT%\*.lmproj" 2^>nul ^| findstr /v /i "\\bin\\ \\bin-int\\ \\.git\\"') do (
    set /a COUNT+=1
    set "PROJ[!COUNT!]=%%~dpf"
    set "MARKER= "
    if /i "%%~dpf"=="!DEFAULT!\" set "MARKER=*"
    echo   !MARKER!!COUNT!^) %%~nf
)

if %COUNT%==0 (
    echo Error: no .lmproj files found under %ROOT%
    popd & exit /b 1
)
echo.

:prompt_loop
if not "!DEFAULT!"=="" (
    set /p "choice=Select a project [1-%COUNT%] or type a path (Enter for default): "
) else (
    set /p "choice=Select a project [1-%COUNT%] or type a path: "
)

if "!choice!"=="" if not "!DEFAULT!"=="" (
    set "PROJECT_DIR=!DEFAULT!"
    goto save_choice
)

REM Numeric selection from the list above.
echo !choice!| findstr /r "^[1-9][0-9]*$" >nul
if not errorlevel 1 (
    if !choice! leq %COUNT% (
        for %%i in (!choice!) do set "PROJECT_DIR=!PROJ[%%i]!"
        REM Strip the trailing backslash left by %%~dpf
        if "!PROJECT_DIR:~-1!"=="\" set "PROJECT_DIR=!PROJECT_DIR:~0,-1!"
        goto save_choice
    )
)

call :resolve_project "!choice!"
if not errorlevel 1 goto save_choice

echo   Not a project directory - pick a number or a path containing a .lmproj.
goto prompt_loop

:save_choice
if not exist "%ROOT%\.zed" mkdir "%ROOT%\.zed"
>"%LAST_FILE%" echo !PROJECT_DIR!

:have_project

REM ------------------------------------------------------------- find Runtime

set "RUNTIME="
if not "%CONFIG%"=="" (
    if exist "%ROOT%\bin\%CONFIG%-windows-x86_64\Runtime.exe" set "RUNTIME=%ROOT%\bin\%CONFIG%-windows-x86_64\Runtime.exe"
) else (
    REM Newest build wins - dir /o-d sorts by modified time, newest first.
    for /f "delims=" %%f in ('dir /s /b /o-d "%ROOT%\bin\*-windows-*\Runtime.exe" 2^>nul') do (
        if "!RUNTIME!"=="" set "RUNTIME=%%f"
    )
)

if "!RUNTIME!"=="" (
    echo Error: no Runtime.exe found under %ROOT%\bin
    if not "%CONFIG%"=="" echo   for config '%CONFIG%'
    echo Build one first, e.g.  Scripts\Windows\BuildWindows.bat
    popd & exit /b 1
)

REM ---------------------------------------------------------------- launch

echo.
echo Project : !PROJECT_DIR!
echo Runtime : !RUNTIME!
echo.

cd /d "%%~dp0"
for %%r in ("!RUNTIME!") do cd /d "%%~dpr"
"!RUNTIME!" --project="!PROJECT_DIR!"
set "EXITCODE=!errorlevel!"
popd
exit /b %EXITCODE%

REM ---------------------------------------------------------------- helpers

:resolve_project
REM Accepts a .lmproj file or a directory containing one. Sets PROJECT_DIR.
set "input=%~1"
set "candidate=%~f1"
if not exist "!candidate!" set "candidate=%ROOT%\!input!"
if not exist "!candidate!" exit /b 1

if /i "%~x1"==".lmproj" (
    for %%p in ("!candidate!") do set "candidate=%%~dpp"
    if "!candidate:~-1!"=="\" set "candidate=!candidate:~0,-1!"
)

if not exist "!candidate!\*.lmproj" exit /b 1
for %%p in ("!candidate!") do set "PROJECT_DIR=%%~fp"
exit /b 0
