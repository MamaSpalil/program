# ============================================================================
# CryptoTrader Web Version - Advanced Installation Script (PowerShell)
# ============================================================================
# This PowerShell script provides advanced installation options including:
# - Prerequisite checking and installation assistance
# - Both Docker and Manual installation methods
# - Automatic configuration generation
# - Service health checking
#
# Usage: Run as Administrator
#        .\install.ps1
# ============================================================================

#Requires -RunAsAdministrator

param(
    [Parameter(Mandatory=$false)]
    [ValidateSet("docker", "manual", "check")]
    [string]$Mode = "docker",

    [Parameter(Mandatory=$false)]
    [switch]$SkipBrowser,

    [Parameter(Mandatory=$false)]
    [switch]$Verbose
)

# Set error action preference
$ErrorActionPreference = "Stop"

# Color functions
function Write-ColorOutput {
    param(
        [string]$Message,
        [string]$Color = "White"
    )
    Write-Host $Message -ForegroundColor $Color
}

function Write-Success { param([string]$Message) Write-ColorOutput "[OK] $Message" "Green" }
function Write-Info { param([string]$Message) Write-ColorOutput "[INFO] $Message" "Cyan" }
function Write-Warning { param([string]$Message) Write-ColorOutput "[WARNING] $Message" "Yellow" }
function Write-Error { param([string]$Message) Write-ColorOutput "[ERROR] $Message" "Red" }
function Write-Step { param([string]$Message) Write-ColorOutput "`n[STEP] $Message" "Magenta" }

# Header
Clear-Host
Write-ColorOutput "`n=======================================================================" "Cyan"
Write-ColorOutput "  CryptoTrader Web Version - Advanced Installation Script" "Cyan"
Write-ColorOutput "=======================================================================" "Cyan"
Write-Info "Mode: $Mode"
Write-Host ""

# Check administrator privileges
if (-not ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Error "This script requires Administrator privileges."
    Write-Info "Please right-click and select 'Run as Administrator'"
    pause
    exit 1
}

Write-Success "Running with Administrator privileges"

# ============================================================================
# Function: Check Prerequisites
# ============================================================================
function Test-Prerequisites {
    Write-Step "Checking Prerequisites..."

    $prerequisites = @{
        "Git" = @{
            Command = "git --version"
            Url = "https://git-scm.com/download/win"
            Required = $true
        }
        "Node.js" = @{
            Command = "node --version"
            Url = "https://nodejs.org/"
            Required = $true
            MinVersion = "20.0.0"
        }
        "npm" = @{
            Command = "npm --version"
            Url = "https://nodejs.org/"
            Required = $true
        }
        "Python" = @{
            Command = "python --version"
            Url = "https://www.python.org/"
            Required = $($Mode -eq "manual")
            MinVersion = "3.11.0"
        }
        "Docker" = @{
            Command = "docker --version"
            Url = "https://www.docker.com/products/docker-desktop/"
            Required = $($Mode -eq "docker")
        }
        "Docker Compose" = @{
            Command = "docker-compose --version"
            Url = "https://www.docker.com/products/docker-desktop/"
            Required = $($Mode -eq "docker")
        }
    }

    $allPassed = $true
    $missing = @()

    foreach ($name in $prerequisites.Keys) {
        $prereq = $prerequisites[$name]

        try {
            $output = Invoke-Expression $prereq.Command 2>$null

            if ($output) {
                # Extract version if possible
                if ($output -match "(\d+\.\d+\.\d+)") {
                    $version = $matches[1]

                    # Check minimum version if specified
                    if ($prereq.MinVersion) {
                        if ([version]$version -lt [version]$prereq.MinVersion) {
                            Write-Warning "$name version $version is below minimum required version $($prereq.MinVersion)"
                            if ($prereq.Required) {
                                $allPassed = $false
                                $missing += $name
                            }
                        } else {
                            Write-Success "$name $version found"
                        }
                    } else {
                        Write-Success "$name $version found"
                    }
                } else {
                    Write-Success "$name found"
                }
            } else {
                throw
            }
        } catch {
            if ($prereq.Required) {
                Write-Error "$name not found or not in PATH"
                Write-Info "Install from: $($prereq.Url)"
                $allPassed = $false
                $missing += $name
            } else {
                Write-Warning "$name not found (optional for current mode)"
            }
        }
    }

    if (-not $allPassed) {
        Write-Host ""
        Write-Error "Missing required prerequisites: $($missing -join ', ')"
        Write-Info "Please install the missing software and try again."
        return $false
    }

    return $true
}

# ============================================================================
# Function: Setup Environment Files
# ============================================================================
function Initialize-EnvironmentFiles {
    Write-Step "Setting up environment files..."

    $envFiles = @(
        @{ Example = "backend\.env.example"; Target = "backend\.env" }
        @{ Example = "frontend\.env.example"; Target = "frontend\.env" }
        @{ Example = "ml-service\.env.example"; Target = "ml-service\.env" }
    )

    foreach ($file in $envFiles) {
        if (Test-Path $file.Example) {
            if (-not (Test-Path $file.Target)) {
                Copy-Item $file.Example $file.Target
                Write-Success "Created $($file.Target)"

                # Generate secure secrets for backend .env
                if ($file.Target -eq "backend\.env") {
                    Write-Info "Generating secure secrets for backend..."
                    $content = Get-Content $file.Target -Raw

                    # Generate JWT secret (64 characters)
                    $jwtSecret = -join ((48..57) + (65..90) + (97..122) | Get-Random -Count 64 | ForEach-Object {[char]$_})
                    $content = $content -replace "your-super-secret-jwt-key-change-in-production", $jwtSecret

                    # Generate encryption key (32 characters)
                    $encryptionKey = -join ((48..57) + (65..90) + (97..122) | Get-Random -Count 32 | ForEach-Object {[char]$_})
                    $content = $content -replace "your-32-character-encryption-key-change-this-in-prod", $encryptionKey

                    Set-Content $file.Target $content -NoNewline
                    Write-Success "Generated secure JWT_SECRET and ENCRYPTION_KEY"
                }
            } else {
                Write-Info "$($file.Target) already exists, skipping"
            }
        } else {
            Write-Warning "$($file.Example) not found, skipping"
        }
    }
}

# ============================================================================
# Function: Docker Installation
# ============================================================================
function Install-WithDocker {
    Write-Step "Installing with Docker Compose..."

    # Check if Docker is running
    try {
        docker ps | Out-Null
        Write-Success "Docker is running"
    } catch {
        Write-Error "Docker is not running"
        Write-Info "Please start Docker Desktop and wait for it to be ready."
        return $false
    }

    # Setup environment files
    Initialize-EnvironmentFiles

    # Start services
    Write-Info "Starting Docker containers (this may take several minutes)..."
    Write-Host ""

    $composeCmd = if (Get-Command "docker-compose" -ErrorAction SilentlyContinue) {
        "docker-compose"
    } else {
        "docker compose"
    }

    & $composeCmd up -d

    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed to start Docker containers"
        return $false
    }

    Write-Host ""
    Write-Success "Docker containers started successfully"

    # Wait for services
    Write-Info "Waiting for services to be ready..."
    Start-Sleep -Seconds 10

    # Show container status
    & $composeCmd ps

    return $true
}

# ============================================================================
# Function: Manual Installation
# ============================================================================
function Install-Manually {
    Write-Step "Installing manually..."

    # Setup environment files
    Initialize-EnvironmentFiles

    # Install backend dependencies
    Write-Info "Installing backend dependencies..."
    Push-Location backend
    npm install
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed to install backend dependencies"
        Pop-Location
        return $false
    }
    Write-Success "Backend dependencies installed"
    Pop-Location

    # Install frontend dependencies
    Write-Info "Installing frontend dependencies..."
    Push-Location frontend
    npm install
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed to install frontend dependencies"
        Pop-Location
        return $false
    }
    Write-Success "Frontend dependencies installed"
    Pop-Location

    # Setup Python virtual environment
    Write-Info "Setting up Python virtual environment..."
    Push-Location ml-service

    if (-not (Test-Path "venv")) {
        python -m venv venv
        if ($LASTEXITCODE -ne 0) {
            Write-Error "Failed to create virtual environment"
            Pop-Location
            return $false
        }
        Write-Success "Virtual environment created"
    }

    # Activate and install Python dependencies
    Write-Info "Installing Python dependencies..."
    & "venv\Scripts\Activate.ps1"
    pip install -r requirements.txt
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed to install Python dependencies"
        Pop-Location
        return $false
    }
    Write-Success "Python dependencies installed"
    deactivate
    Pop-Location

    # Setup database
    Write-Info "Setting up database..."
    Push-Location backend

    npx prisma generate
    if ($LASTEXITCODE -eq 0) {
        Write-Success "Prisma client generated"

        npx prisma migrate dev --name init
        if ($LASTEXITCODE -eq 0) {
            Write-Success "Database migrations completed"
        } else {
            Write-Warning "Failed to run migrations. Check DATABASE_URL in backend/.env"
        }
    } else {
        Write-Warning "Failed to generate Prisma client. Check DATABASE_URL in backend/.env"
    }

    Pop-Location

    return $true
}

# ============================================================================
# Function: Health Check
# ============================================================================
function Test-ServicesHealth {
    param([int]$MaxAttempts = 5)

    Write-Step "Checking service health..."

    $services = @{
        "Frontend" = "http://localhost:3000"
        "Backend" = "http://localhost:4000"
        "ML Service" = "http://localhost:8000"
    }

    $allHealthy = $true

    foreach ($name in $services.Keys) {
        $url = $services[$name]
        $healthy = $false

        for ($i = 1; $i -le $MaxAttempts; $i++) {
            try {
                $response = Invoke-WebRequest -Uri $url -TimeoutSec 2 -UseBasicParsing -ErrorAction Stop
                if ($response.StatusCode -eq 200) {
                    $healthy = $true
                    break
                }
            } catch {
                if ($i -lt $MaxAttempts) {
                    Start-Sleep -Seconds 3
                }
            }
        }

        if ($healthy) {
            Write-Success "$name is responding at $url"
        } else {
            Write-Warning "$name is not responding at $url"
            $allHealthy = $false
        }
    }

    return $allHealthy
}

# ============================================================================
# Main Installation Logic
# ============================================================================

try {
    # Check prerequisites
    if (-not (Test-Prerequisites)) {
        exit 1
    }

    # Perform installation based on mode
    $success = $false

    if ($Mode -eq "check") {
        Write-Info "Prerequisites check completed. No installation performed."
        exit 0
    } elseif ($Mode -eq "docker") {
        $success = Install-WithDocker
    } elseif ($Mode -eq "manual") {
        $success = Install-Manually
    }

    if (-not $success) {
        Write-Error "Installation failed"
        exit 1
    }

    # Health check (only for Docker mode)
    if ($Mode -eq "docker") {
        Start-Sleep -Seconds 5
        Test-ServicesHealth
    }

    # Display success message
    Write-Host ""
    Write-ColorOutput "=======================================================================" "Green"
    Write-ColorOutput "  Installation Complete!" "Green"
    Write-ColorOutput "=======================================================================" "Green"
    Write-Host ""

    if ($Mode -eq "docker") {
        Write-Info "Services are running at:"
        Write-Host "  - Frontend (Web Interface):  http://localhost:3000" -ForegroundColor White
        Write-Host "  - Backend API:               http://localhost:4000" -ForegroundColor White
        Write-Host "  - ML Service:                http://localhost:8000" -ForegroundColor White
        Write-Host "  - PostgreSQL:                localhost:5432" -ForegroundColor White
        Write-Host "  - Redis:                     localhost:6379" -ForegroundColor White
    } else {
        Write-Info "To start all services, run: .\start-all.cmd"
        Write-Info "Or start each service manually in separate terminals:"
        Write-Host "  Terminal 1: cd backend && npm run dev" -ForegroundColor White
        Write-Host "  Terminal 2: cd ml-service && venv\Scripts\activate.ps1 && uvicorn app.main:app --reload --port 8000" -ForegroundColor White
        Write-Host "  Terminal 3: cd frontend && npm run dev" -ForegroundColor White
    }

    Write-Host ""
    Write-ColorOutput "=======================================================================" "Green"
    Write-Host ""

    # Open browser
    if (-not $SkipBrowser -and $Mode -eq "docker") {
        Write-Info "Opening web browser in 5 seconds..."
        Start-Sleep -Seconds 5
        Start-Process "http://localhost:3000"
    }

} catch {
    Write-Error "An error occurred: $_"
    Write-Host $_.ScriptStackTrace
    exit 1
}

Write-Host "Press any key to exit..."
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
