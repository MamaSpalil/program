<?php

/**
 * CryptoTrader PHP Backend - Main Entry Point
 */

// Error reporting (disable in production)
error_reporting(E_ALL);
ini_set('display_errors', '1');

// Load environment variables from .env file
if (file_exists(__DIR__ . '/../.env')) {
    $lines = file(__DIR__ . '/../.env', FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);
    foreach ($lines as $line) {
        if (strpos(trim($line), '#') === 0) continue;
        list($name, $value) = explode('=', $line, 2);
        $_ENV[trim($name)] = trim($value);
        putenv(trim($name) . '=' . trim($value));
    }
}

// Set timezone
date_default_timezone_set('UTC');

// Autoload classes (assuming Composer autoload)
if (file_exists(__DIR__ . '/../vendor/autoload.php')) {
    require_once __DIR__ . '/../vendor/autoload.php';
} else {
    // Simple PSR-4 autoloader
    spl_autoload_register(function ($class) {
        $prefix = 'CryptoTrader\\';
        $base_dir = __DIR__ . '/../src/';

        $len = strlen($prefix);
        if (strncmp($prefix, $class, $len) !== 0) {
            return;
        }

        $relative_class = substr($class, $len);
        $file = $base_dir . str_replace('\\', '/', $relative_class) . '.php';

        if (file_exists($file)) {
            require $file;
        }
    });
}

// CORS headers
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type, Authorization, X-Requested-With');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit();
}

// Set JSON response header
header('Content-Type: application/json');

// Get request method and URI
$method = $_SERVER['REQUEST_METHOD'];
$uri = parse_url($_SERVER['REQUEST_URI'], PHP_URL_PATH);
$uri = trim($uri, '/');

// Parse request body for POST/PUT
$input = null;
if (in_array($method, ['POST', 'PUT', 'PATCH'])) {
    $input = json_decode(file_get_contents('php://input'), true);
}

// Simple router
try {
    // Auth routes
    if (preg_match('#^api/auth/(.+)$#', $uri, $matches)) {
        require_once __DIR__ . '/../src/api/auth.php';
        handleAuthRequest($matches[1], $method, $input);
    }
    // Market routes
    elseif (preg_match('#^api/market/(.+)$#', $uri, $matches)) {
        require_once __DIR__ . '/../src/api/market.php';
        handleMarketRequest($matches[1], $method, $input);
    }
    // Trading routes
    elseif (preg_match('#^api/trading/(.+)$#', $uri, $matches)) {
        require_once __DIR__ . '/../src/api/trading.php';
        handleTradingRequest($matches[1], $method, $input);
    }
    // Analysis routes
    elseif (preg_match('#^api/analysis/(.+)$#', $uri, $matches)) {
        require_once __DIR__ . '/../src/api/analysis.php';
        handleAnalysisRequest($matches[1], $method, $input);
    }
    // Health check
    elseif ($uri === 'api/health') {
        echo json_encode([
            'status' => 'ok',
            'timestamp' => time(),
            'version' => '1.0.0'
        ]);
    }
    // Serve frontend
    elseif (empty($uri) || $uri === 'index.php') {
        header('Content-Type: text/html');
        readfile(__DIR__ . '/../../frontend/index.html');
    }
    // 404
    else {
        http_response_code(404);
        echo json_encode(['error' => 'Route not found']);
    }
} catch (\Exception $e) {
    http_response_code(500);
    echo json_encode([
        'error' => $e->getMessage(),
        'trace' => getenv('APP_DEBUG') === 'true' ? $e->getTraceAsString() : null
    ]);
}
