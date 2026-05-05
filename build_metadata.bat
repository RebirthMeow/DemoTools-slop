@echo off
REM build_metadata.bat
REM Builds jkdemometadata.exe from a Windows command prompt by launching the
REM MSYS2 mingw64 environment (so gcc / make / jansson are all on PATH).
REM
REM Usage:
REM   build_metadata.bat           - incremental build
REM   build_metadata.bat clean     - full rebuild
REM   build_metadata.bat install   - build then copy exe to D:\JSON\
REM
REM Requires MSYS2 installed at C:\msys64.  Edit MSYS_HOME below if elsewhere.
REM Inside MSYS2 you must have installed:
REM   pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-make mingw-w64-x86_64-jansson

setlocal
set MSYS_HOME=C:\msys64
set SCRIPT_DIR=%~dp0

if not exist "%MSYS_HOME%\msys2_shell.cmd" (
  echo ERROR: MSYS2 not found at %MSYS_HOME%
  echo Edit MSYS_HOME at the top of this batch file if installed elsewhere.
  exit /b 1
)

REM -mingw64       : initialize the mingw64 subsystem (puts /mingw64/bin on PATH)
REM -defterm       : run in the current console (no new window)
REM -no-start      : don't fork; wait for completion
REM -here          : start in this directory
REM -shell bash    : run bash (default but explicit)
REM -c "..."       : single command to run
"%MSYS_HOME%\msys2_shell.cmd" -mingw64 -defterm -no-start -here -shell bash -c "cd '%SCRIPT_DIR:\=/%' && ./build_metadata.sh %1"
