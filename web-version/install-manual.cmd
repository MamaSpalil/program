@echo off
REM ============================================================================
REM CryptoTrader Web Version - Manual Installation Setup Script
REM ============================================================================
REM This script helps set up the development environment for manual installation
REM
REM Prerequisites:
REM - Node.js 20+ installed
REM - Python 3.11+ installed
REM - PostgreSQL 15+ installed and running
REM - Redis 7+ installed and running
REM - Git installed
REM
REM Usage: Run this script from the web-version directory
REM ============================================================================

setlocal EnableDelayedExpansion

echo.
echo ========================================================================
echo  CryptoTrader Web Version - Manual Installation Setup
echo ========================================================================
echo.

set SCRIPT_DIR=%~dp0
cd /d "%SCRIPT_DIR%"

echo [STEP 1/7] Checking prerequisites...
echo.

REM Check Node.js
echo [CHECK] Node.js...
node --version >nul 2>&1
if %errorLevel% neq 0 (
    echo [ERROR] Node.js is not installed or not in PATH.
    echo Please install Node.js 20 LTS from: https://nodejs.org/
    pause
    exit /b 1
)
for /f "tokens=1" %%i in ('node --version') do set NODE_VERSION=%%i
echo [OK] Node.js %NODE_VERSION% found.

REM Check npm
echo [CHECK] npm...
npm --version >nul 2>&1
if %errorLevel% neq 0 (
    echo [ERROR] npm is not installed or not in PATH.
    pause
    exit /b 1
)
for /f "tokens=1" %%i in ('npm --version') do set NPM_VERSION=%%i
echo [OK] npm %NPM_VERSION% found.

REM Check Python
echo [CHECK] Python...
python --version >nul 2>&1
if %errorLevel% neq 0 (
    echo [ERROR] Python is not installed or not in PATH.
    echo Please install Python 3.11+ from: https://www.python.org/
    pause
    exit /b 1
)
for /f "tokens=2" %%i in ('python --version') do set PYTHON_VERSION=%%i
echo [OK] Python %PYTHON_VERSION% found.

REM Check pip
echo [CHECK] pip...
pip --version >nul 2>&1
if %errorLevel% neq 0 (
    echo [ERROR] pip is not installed or not in PATH.
    pause
    exit /b 1
)
echo [OK] pip found.

REM Check PostgreSQL (attempt connection)
echo [CHECK] PostgreSQL...
where psql >nul 2>&1
if %errorLevel% neq 0 (
    echo [WARNING] PostgreSQL psql not found in PATH.
    echo Please ensure PostgreSQL is installed and running on port 5432.
) else (
    echo [OK] PostgreSQL tools found.
)

REM Check Redis (attempt connection)
echo [CHECK] Redis...
where redis-cli >nul 2>&1
if %errorLevel% neq 0 (
    echo [WARNING] redis-cli not found in PATH.
    echo Please ensure Redis is installed and running on port 6379.
) else (
    echo [OK] Redis tools found.
)

echo.
echo [STEP 2/7] Setting up environment files...
echo.

REM Setup backend .env
if not exist "backend\.env" (
    if exist "backend\.env.example" (
        echo [INFO] Creating backend/.env...
        copy "backend\.env.example" "backend\.env" >nul
        echo [OK] backend/.env created.
        echo [IMPORTANT] Please edit backend/.env and configure:
        echo            - DATABASE_URL
        echo            - REDIS_HOST
        echo            - JWT_SECRET
        echo            - ENCRYPTION_KEY
    )
) else (
    echo [INFO] backend/.env already exists.
)

REM Setup frontend .env
if not exist "frontend\.env" (
    if exist "frontend\.env.example" (
        echo [INFO] Creating frontend/.env...
        copy "frontend\.env.example" "frontend\.env" >nul
        echo [OK] frontend/.env created.
    )
) else (
    echo [INFO] frontend/.env already exists.
)

REM Setup ml-service .env
if not exist "ml-service\.env" (
    if exist "ml-service\.env.example" (
        echo [INFO] Creating ml-service/.env...
        copy "ml-service\.env.example" "ml-service\.env" >nul
        echo [OK] ml-service/.env created.
    )
) else (
    echo [INFO] ml-service/.env already exists.
)

echo.
echo [STEP 3/7] Installing Backend dependencies...
echo.
cd backend
call npm install
if %errorLevel% neq 0 (
    echo [ERROR] Failed to install backend dependencies.
    pause
    exit /b 1
)
echo [OK] Backend dependencies installed.
cd ..

echo.
echo [STEP 4/7] Installing Frontend dependencies...
echo.
cd frontend
call npm install
if %errorLevel% neq 0 (
    echo [ERROR] Failed to install frontend dependencies.
    pause
    exit /b 1
)
echo [OK] Frontend dependencies installed.
cd ..

echo.
echo [STEP 5/7] Setting up Python virtual environment...
echo.
cd ml-service
if not exist "venv" (
    echo [INFO] Creating virtual environment...
    python -m venv venv
    if %errorLevel% neq 0 (
        echo [ERROR] Failed to create virtual environment.
        pause
        exit /b 1
    )
    echo [OK] Virtual environment created.
)

echo [INFO] Activating virtual environment and installing dependencies...
call venv\Scripts\activate.bat
pip install -r requirements.txt
if %errorLevel% neq 0 (
    echo [ERROR] Failed to install Python dependencies.
    pause
    exit /b 1
)
echo [OK] Python dependencies installed.
call venv\Scripts\deactivate.bat
cd ..

echo.
echo [STEP 6/7] Setting up database...
echo.
cd backend
echo [INFO] Generating Prisma client...
call npx prisma generate
if %errorLevel% neq 0 (
    echo [WARNING] Failed to generate Prisma client.
    echo You may need to configure DATABASE_URL in backend/.env first.
) else (
    echo [OK] Prisma client generated.

    echo [INFO] Running database migrations...
    call npx prisma migrate dev --name init
    if %errorLevel% neq 0 (
        echo [WARNING] Failed to run migrations.
        echo Please ensure PostgreSQL is running and DATABASE_URL is correct.
    ) else (
        echo [OK] Database migrations completed.
    )
)
cd ..

echo.
echo [STEP 7/7] Setup complete!
echo.
echo ========================================================================
echo  Manual Installation Setup Complete!
echo ========================================================================
echo.
echo Next steps:
echo.
echo 1. Configure your .env files (especially backend/.env):
echo    - Set DATABASE_URL for your PostgreSQL connection
echo    - Set REDIS_HOST and REDIS_PORT
echo    - Generate secure JWT_SECRET and ENCRYPTION_KEY
echo.
echo 2. Start the services (in separate terminals):
echo.
echo    Terminal 1 - Backend:
echo       cd backend
echo       npm run dev
echo.
echo    Terminal 2 - ML Service:
echo       cd ml-service
echo       venv\Scripts\activate.bat
echo       uvicorn app.main:app --reload --port 8000
echo.
echo    Terminal 3 - Frontend:
echo       cd frontend
echo       npm run dev
echo.
echo 3. Or use the provided start-all.cmd script to launch all services
echo.
echo ========================================================================
echo.

pause
endlocal
