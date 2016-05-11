echo off
rem Add a new TAP virtual ethernet adapter
SET mypath=%~dp0
cd %mypath%
tapinstall.exe install OemWin2k.inf tap0901
tapinstall.exe install OemWin2k.inf tap0901
pause
