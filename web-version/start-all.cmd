@echo off
REM ============================================================================
REM CryptoTrader Web Version - Quick Start Script
REM ============================================================================
REM This script starts all services for manual installation
REM ============================================================================

setlocal EnableDelayedExpansion

echo.
echo ========================================================================
echo  CryptoTrader Web Version - Starting All Services
echo ========================================================================
echo.

set SCRIPT_DIR=%~dp0
cd /d "%SCRIPT_DIR%"

REM Check if using Docker or Manual
if exist "docker-compose.yml" (
    docker-compose ps >nul 2>&1
    if %errorLevel% equ 0 (
        echo [INFO] Docker Compose detected. Use Docker method instead.
        echo.
        echo Starting services with Docker Compose...
        docker-compose up -d

        if %errorLevel% equ 0 (
            echo.
            echo [OK] Services started successfully!
            echo.
            echo Services are available at:
            echo   - Frontend: http://localhost:3000
            echo   - Backend:  http://localhost:4000
            echo   - ML Service: http://localhost:8000
            echo.
            timeout /t 3 /nobreak >nul
            start http://localhost:3000
        ) else (
            echo [ERROR] Failed to start Docker services.
        )
        pause
        exit /b
    )
)

echo [INFO] Starting manual installation services...
echo This will open 3 terminal windows for Backend, ML Service, and Frontend.
echo.

REM Start Backend
echo [1/3] Starting Backend...
start "CryptoTrader Backend" cmd /k "cd /d %SCRIPT_DIR%backend && npm run dev"
timeout /t 2 /nobreak >nul

REM Start ML Service
echo [2/3] Starting ML Service...
start "CryptoTrader ML Service" cmd /k "cd /d %SCRIPT_DIR%ml-service && call venv\Scripts\activate.bat && uvicorn app.main:app --reload --port 8000"
timeout /t 2 /nobreak >nul

REM Start Frontend
echo [3/3] Starting Frontend...
start "CryptoTrader Frontend" cmd /k "cd /d %SCRIPT_DIR%frontend && npm run dev"
timeout /t 5 /nobreak >nul

echo.
echo [OK] All services are starting in separate windows.
echo.
echo Services will be available at:
echo   - Frontend (Web Interface): http://localhost:3000
echo   - Backend API:              http://localhost:4000
echo   - ML Service:               http://localhost:8000
echo.
echo Opening web browser...
timeout /t 10 /nobreak >nul
start http://localhost:3000

echo.
echo [INFO] To stop all services, close the terminal windows.
echo.

pause
endlocal
