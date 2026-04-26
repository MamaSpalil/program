@echo off
REM ============================================================================
REM CryptoTrader Web Version - Main Installation Launcher
REM ============================================================================
REM This script helps you choose the installation method
REM ============================================================================

setlocal EnableDelayedExpansion

:MENU
cls
echo.
echo ========================================================================
echo  CryptoTrader Web Version - Installation Launcher
echo ========================================================================
echo.
echo Please select installation method:
echo.
echo  1. Docker Installation (Recommended - Easy and Fast)
echo     Requires: Docker Desktop
echo.
echo  2. Manual Installation (Advanced - For Developers)
echo     Requires: Node.js, Python, PostgreSQL, Redis
echo.
echo  3. PowerShell Advanced Installation (Most Features)
echo     Requires: PowerShell 5.0+
echo.
echo  4. Start Services (if already installed)
echo.
echo  5. Stop Services
echo.
echo  6. View Documentation
echo.
echo  0. Exit
echo.
echo ========================================================================
echo.

set /p CHOICE="Enter your choice (0-6): "

if "%CHOICE%"=="1" goto DOCKER_INSTALL
if "%CHOICE%"=="2" goto MANUAL_INSTALL
if "%CHOICE%"=="3" goto POWERSHELL_INSTALL
if "%CHOICE%"=="4" goto START_SERVICES
if "%CHOICE%"=="5" goto STOP_SERVICES
if "%CHOICE%"=="6" goto DOCUMENTATION
if "%CHOICE%"=="0" goto EXIT

echo.
echo Invalid choice. Please try again.
timeout /t 2 >nul
goto MENU

:DOCKER_INSTALL
cls
echo.
echo ========================================================================
echo  Docker Installation
echo ========================================================================
echo.
echo This will install and start all services using Docker Compose.
echo.
echo Prerequisites:
echo  - Docker Desktop must be installed and running
echo.
set /p CONFIRM="Continue? (Y/N): "
if /i not "%CONFIRM%"=="Y" goto MENU

call install-docker.cmd
pause
goto MENU

:MANUAL_INSTALL
cls
echo.
echo ========================================================================
echo  Manual Installation
echo ========================================================================
echo.
echo This will set up the development environment manually.
echo.
echo Prerequisites (must be installed):
echo  - Node.js 20+
echo  - Python 3.11+
echo  - PostgreSQL 15+
echo  - Redis 7+
echo.
set /p CONFIRM="Continue? (Y/N): "
if /i not "%CONFIRM%"=="Y" goto MENU

call install-manual.cmd
pause
goto MENU

:POWERSHELL_INSTALL
cls
echo.
echo ========================================================================
echo  PowerShell Advanced Installation
echo ========================================================================
echo.
echo This will run the advanced PowerShell installation script.
echo.
echo Options:
echo  1. Docker mode (default)
echo  2. Manual mode
echo  3. Prerequisites check only
echo.
set /p PS_CHOICE="Enter choice (1-3): "

if "%PS_CHOICE%"=="1" (
    powershell -ExecutionPolicy Bypass -File install.ps1
) else if "%PS_CHOICE%"=="2" (
    powershell -ExecutionPolicy Bypass -File install.ps1 -Mode manual
) else if "%PS_CHOICE%"=="3" (
    powershell -ExecutionPolicy Bypass -File install.ps1 -Mode check
) else (
    echo Invalid choice.
    timeout /t 2 >nul
)

pause
goto MENU

:START_SERVICES
cls
echo.
echo ========================================================================
echo  Start Services
echo ========================================================================
echo.
call start-all.cmd
goto MENU

:STOP_SERVICES
cls
echo.
echo ========================================================================
echo  Stop Services
echo ========================================================================
echo.
call stop-all.cmd
goto MENU

:DOCUMENTATION
cls
echo.
echo ========================================================================
echo  Documentation
echo ========================================================================
echo.
echo Available documentation:
echo.
echo  1. Installation Scripts Guide (Russian)
echo  2. Full Windows Installation Guide (Russian)
echo  3. Project README
echo  4. Getting Started Guide
echo.
echo  0. Back to main menu
echo.
set /p DOC_CHOICE="Enter choice (0-4): "

if "%DOC_CHOICE%"=="1" (
    if exist "INSTALLATION_SCRIPTS_RU.md" (
        start INSTALLATION_SCRIPTS_RU.md
    ) else (
        echo File not found: INSTALLATION_SCRIPTS_RU.md
    )
) else if "%DOC_CHOICE%"=="2" (
    if exist "WINDOWS_INSTALLATION_RU.md" (
        start WINDOWS_INSTALLATION_RU.md
    ) else (
        echo File not found: WINDOWS_INSTALLATION_RU.md
    )
) else if "%DOC_CHOICE%"=="3" (
    if exist "README.md" (
        start README.md
    ) else (
        echo File not found: README.md
    )
) else if "%DOC_CHOICE%"=="4" (
    if exist "GETTING_STARTED.md" (
        start GETTING_STARTED.md
    ) else (
        echo File not found: GETTING_STARTED.md
    )
) else if "%DOC_CHOICE%"=="0" (
    goto MENU
)

timeout /t 2 >nul
goto DOCUMENTATION

:EXIT
cls
echo.
echo Thank you for using CryptoTrader Web Version!
echo.
echo For support, visit: https://github.com/MamaSpalil/program/issues
echo.
timeout /t 3 >nul
exit /b 0
