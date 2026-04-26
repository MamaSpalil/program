@echo off
REM ============================================================================
REM CryptoTrader Web Version - Stop All Services
REM ============================================================================
REM This script stops all running services
REM ============================================================================

setlocal EnableDelayedExpansion

echo.
echo ========================================================================
echo  CryptoTrader Web Version - Stopping All Services
echo ========================================================================
echo.

set SCRIPT_DIR=%~dp0
cd /d "%SCRIPT_DIR%"

REM Check if using Docker
docker-compose ps >nul 2>&1
if %errorLevel% equ 0 (
    echo [INFO] Stopping Docker Compose services...
    docker-compose stop

    if %errorLevel% equ 0 (
        echo [OK] Docker services stopped successfully!
    ) else (
        echo [ERROR] Failed to stop Docker services.
    )

    echo.
    set /p REMOVE="Do you want to remove containers? (y/n): "
    if /i "!REMOVE!"=="y" (
        docker-compose down
        echo [OK] Containers removed.
    )

    pause
    exit /b
)

echo [INFO] Stopping manual installation services...
echo.

REM Kill Node.js processes (Backend and Frontend)
echo [1/3] Stopping Node.js processes...
taskkill /F /IM node.exe >nul 2>&1
if %errorLevel% equ 0 (
    echo [OK] Node.js processes stopped.
) else (
    echo [INFO] No Node.js processes found.
)

REM Kill Python processes (ML Service)
echo [2/3] Stopping Python processes...
taskkill /F /IM python.exe >nul 2>&1
if %errorLevel% equ 0 (
    echo [OK] Python processes stopped.
) else (
    echo [INFO] No Python processes found.
)

REM Close terminal windows by title
echo [3/3] Closing terminal windows...
taskkill /FI "WINDOWTITLE eq CryptoTrader*" >nul 2>&1

echo.
echo [OK] All services stopped.
echo.

pause
endlocal
