@echo off
REM ============================================================================
REM CryptoTrader Web Version - Automated Docker Installation Script
REM ============================================================================
REM This script automates the installation and launch of CryptoTrader Web
REM using Docker Compose on Windows 10 Pro x64
REM
REM Prerequisites:
REM - Windows 10 Pro x64
REM - Docker Desktop installed and running
REM - Git installed
REM
REM Usage: Run as Administrator
REM ============================================================================

setlocal EnableDelayedExpansion

echo.
echo ========================================================================
echo  CryptoTrader Web Version - Automated Docker Installation
echo ========================================================================
echo.

REM Check if running as administrator
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [ERROR] This script requires Administrator privileges.
    echo Please right-click and select "Run as Administrator"
    pause
    exit /b 1
)

echo [INFO] Administrator privileges confirmed.
echo.

REM Check if Docker is installed
echo [STEP 1/6] Checking Docker installation...
docker --version >nul 2>&1
if %errorLevel% neq 0 (
    echo [ERROR] Docker is not installed or not in PATH.
    echo Please install Docker Desktop from: https://www.docker.com/products/docker-desktop/
    pause
    exit /b 1
)
echo [OK] Docker is installed.

REM Check if Docker is running
echo [STEP 2/6] Checking Docker service status...
docker ps >nul 2>&1
if %errorLevel% neq 0 (
    echo [ERROR] Docker is not running.
    echo Please start Docker Desktop and wait for it to be ready.
    echo Look for the Docker icon in the system tray (it should be green).
    pause
    exit /b 1
)
echo [OK] Docker is running.

REM Check if docker-compose is available
echo [STEP 3/6] Checking Docker Compose...
docker-compose --version >nul 2>&1
if %errorLevel% neq 0 (
    docker compose version >nul 2>&1
    if !errorLevel! neq 0 (
        echo [ERROR] Docker Compose is not available.
        echo Please ensure Docker Desktop is properly installed.
        pause
        exit /b 1
    )
    set DOCKER_COMPOSE_CMD=docker compose
) else (
    set DOCKER_COMPOSE_CMD=docker-compose
)
echo [OK] Docker Compose is available.

REM Get current directory
set SCRIPT_DIR=%~dp0
cd /d "%SCRIPT_DIR%"

echo.
echo [STEP 4/6] Setting up environment files...

REM Create .env files if they don't exist
if not exist "backend\.env" (
    if exist "backend\.env.example" (
        echo [INFO] Creating backend/.env from .env.example...
        copy "backend\.env.example" "backend\.env" >nul
        echo [OK] backend/.env created.
    ) else (
        echo [WARNING] backend/.env.example not found, skipping.
    )
) else (
    echo [INFO] backend/.env already exists, skipping.
)

if not exist "frontend\.env" (
    if exist "frontend\.env.example" (
        echo [INFO] Creating frontend/.env from .env.example...
        copy "frontend\.env.example" "frontend\.env" >nul
        echo [OK] frontend/.env created.
    ) else (
        echo [WARNING] frontend/.env.example not found, skipping.
    )
) else (
    echo [INFO] frontend/.env already exists, skipping.
)

if not exist "ml-service\.env" (
    if exist "ml-service\.env.example" (
        echo [INFO] Creating ml-service/.env from .env.example...
        copy "ml-service\.env.example" "ml-service\.env" >nul
        echo [OK] ml-service/.env created.
    ) else (
        echo [INFO] ml-service/.env.example not found, skipping.
    )
) else (
    echo [INFO] ml-service/.env already exists, skipping.
)

echo.
echo [STEP 5/6] Starting Docker containers...
echo This may take several minutes on first run...
echo.

%DOCKER_COMPOSE_CMD% up -d

if %errorLevel% neq 0 (
    echo.
    echo [ERROR] Failed to start Docker containers.
    echo Please check Docker Desktop and try again.
    pause
    exit /b 1
)

echo.
echo [STEP 6/6] Waiting for services to be ready...
timeout /t 10 /nobreak >nul

REM Check if containers are running
%DOCKER_COMPOSE_CMD% ps

echo.
echo ========================================================================
echo  Installation Complete!
echo ========================================================================
echo.
echo Services are now running at:
echo.
echo   - Frontend (Web Interface):  http://localhost:3000
echo   - Backend API:               http://localhost:4000
echo   - ML Service:                http://localhost:8000
echo   - PostgreSQL Database:       localhost:5432
echo   - Redis Cache:               localhost:6379
echo.
echo ========================================================================
echo.
echo Opening web browser in 5 seconds...
timeout /t 5 /nobreak >nul

REM Open default browser
start http://localhost:3000

echo.
echo [INFO] To view logs, run:
echo        %DOCKER_COMPOSE_CMD% logs -f
echo.
echo [INFO] To stop all services, run:
echo        %DOCKER_COMPOSE_CMD% stop
echo.
echo [INFO] To stop and remove all containers, run:
echo        %DOCKER_COMPOSE_CMD% down
echo.

pause
endlocal
