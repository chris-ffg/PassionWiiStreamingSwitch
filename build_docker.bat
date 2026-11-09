@echo off
echo ==============================================
echo Building DangoPlayer Switch using Docker...
echo ==============================================

docker build -t dangoplayer-switch .
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Docker build failed.
    exit /b %ERRORLEVEL%
)

docker run --rm -v "%cd%":/project dangoplayer-switch make
if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] Build finished! Output: DangoPlayerSwitch.nro
) else (
    echo [ERROR] Compilation failed.
)
