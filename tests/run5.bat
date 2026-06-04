@echo off
setlocal enabledelayedexpansion
set PLUGINVAL=D:\UTT\tools\pluginval\pluginval.exe
set PLUGIN=D:\UTT\build\UTT.vst3

copy /Y "C:\Program Files\Common Files\VST3\UTT.vst3" "%PLUGIN%" >nul 2>&1

for /l %%i in (1,1,5) do (
    echo Run %%i:
    "%PLUGINVAL%" --strictness-level 5 --timeout-ms 30000 --validate "%PLUGIN%" 2>&1 | findstr /c:"FAILED" >nul
    if errorlevel 1 (
        echo   PASS
    ) else (
        echo   FAIL
    )
)
endlocal
