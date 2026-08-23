@echo off
echo === UTT Plugin Validation ===
"D:\UTT\tools\pluginval\pluginval.exe" --validate --strictness-level 5 --output-dir "D:\UTT\tests\results" "D:\UTT\tests\pluginval_config.xml"
exit /b %ERRORLEVEL%
