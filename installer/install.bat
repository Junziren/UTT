@echo off
title UTT v0.2.0 - Installing...
echo ========================================
echo   UTT - Utility Toolbox by UnpureBloom
echo   Version 0.2.0
echo ========================================
echo.

set "VST3DIR=C:\Program Files\Common Files\VST3"

echo [1/3] Copying VST3 plugin...
if exist "%VST3DIR%\UTT.vst3" rmdir /s /q "%VST3DIR%\UTT.vst3"
xcopy /e /i /y "UTT.vst3" "%VST3DIR%\UTT.vst3" >nul
if %errorlevel% neq 0 (
    echo   ERROR: Failed to copy. Try running as Administrator.
    pause
    exit /b 1
)

echo [2/3] Copying documentation...
copy /y "README.md" "%VST3DIR%\UTT_README.md" >nul
copy /y "LICENSE" "%VST3DIR%\UTT_LICENSE.txt" >nul

echo [3/3] Opening README...
start "" "%VST3DIR%\UTT_README.md"

echo.
echo Installation complete! Restart your DAW to load UTT.
pause
