@echo off
setlocal
echo ============================================
echo   UTT Plugin Validation
echo ============================================

set PLUGIN_PATH=C:\Program Files\Common Files\VST3\UTT.vst3
set PLUGINVAL=D:\UTT\tools\pluginval\pluginval.exe

if not exist "%PLUGINVAL%" (
    echo ERROR: pluginval not found at %PLUGINVAL%
    exit /b 1
)

if not exist "%PLUGIN_PATH%" (
    echo ERROR: UTT.vst3 not found at %PLUGIN_PATH%
    exit /b 1
)

echo.
echo Validating: %PLUGIN_PATH%
echo.

"%PLUGINVAL%" --strictness-level 5 --skip-gui-tests --timeout-ms 60000 --validate "%PLUGIN_PATH%"

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ============================================
    echo   ALL TESTS PASSED
    echo ============================================
) else (
    echo.
    echo ============================================
    echo   TESTS FAILED (exit code: %ERRORLEVEL%)
    echo ============================================
)

endlocal
exit /b %ERRORLEVEL%
