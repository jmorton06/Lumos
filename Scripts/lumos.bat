@echo off
rem lumos <command> [options] - runs LumosEditor.exe in CLI mode. LUMOS_EDITOR overrides the lookup.
setlocal
set "ROOT=%~dp0.."
set "EDITOR=%LUMOS_EDITOR%"
if "%EDITOR%"=="" (
    for /f "delims=" %%f in ('dir /b /s /o-d "%ROOT%\bin\LumosEditor.exe" 2^>nul') do if not defined EDITOR set "EDITOR=%%f"
)
if "%EDITOR%"=="" (
    echo lumos: no LumosEditor.exe build found ^(build the editor, or set LUMOS_EDITOR^) 1>&2
    exit /b 1
)
if "%~1"=="" ( "%EDITOR%" help ) else ( "%EDITOR%" %* )
