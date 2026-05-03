<?php
/**
 * Application Configuration
 */

return [
    // Application settings
    'app' => [
        'name' => 'CryptoTrader',
        'version' => '1.0.0',
        'debug' => getenv('APP_DEBUG') === 'true',
        'timezone' => 'UTC',
        'locale' => 'en',
        'url' => getenv('APP_URL') ?: 'http://localhost'
    ],

    // Session configuration
    'session' => [
        'lifetime' => 7200, // 2 hours
        'name' => 'CRYPTOTRADER_SESSION',
        'secure' => getenv('APP_ENV') === 'production',
        'httponly' => true,
        'samesite' => 'Strict'
    ],

    // Security settings
    'security' => [
        'encryption_key' => getenv('ENCRYPTION_KEY') ?: 'change-this-key-in-production',
        'encryption_cipher' => 'AES-256-CBC',
        'hash_algo' => PASSWORD_BCRYPT,
        'hash_cost' => 12,
        'csrf_enabled' => true,
        'rate_limit' => [
            'enabled' => true,
            'max_requests' => 100,
            'window' => 60 // seconds
        ]
    ],

    // ML Service configuration
    'ml_service' => [
        'url' => getenv('ML_SERVICE_URL') ?: 'http://localhost:8080',
        'timeout' => 30,
        'retry_attempts' => 3,
        'endpoints' => [
            'signal' => '/api/ml/signal',
            'features' => '/api/ml/features',
            'backtest' => '/api/ml/backtest'
        ]
    ],

    // Exchange default settings
    'exchanges' => [
        'binance' => [
            'name' => 'Binance',
            'testnet_url' => 'https://testnet.binance.vision',
            'mainnet_url' => 'https://api.binance.com',
            'ws_testnet_url' => 'wss://testnet.binance.vision/ws',
            'ws_mainnet_url' => 'wss://stream.binance.com:9443/ws'
        ],
        'bybit' => [
            'name' => 'Bybit',
            'testnet_url' => 'https://api-testnet.bybit.com',
            'mainnet_url' => 'https://api.bybit.com',
            'ws_testnet_url' => 'wss://stream-testnet.bybit.com/realtime',
            'ws_mainnet_url' => 'wss://stream.bybit.com/realtime'
        ],
        'okx' => [
            'name' => 'OKX',
            'testnet_url' => 'https://www.okx.com',
            'mainnet_url' => 'https://www.okx.com',
            'ws_url' => 'wss://ws.okx.com:8443/ws/v5/public'
        ],
        'bitget' => [
            'name' => 'Bitget',
            'mainnet_url' => 'https://api.bitget.com',
            'ws_url' => 'wss://ws.bitget.com/mix/v1/stream'
        ],
        'kucoin' => [
            'name' => 'KuCoin',
            'mainnet_url' => 'https://api.kucoin.com',
            'ws_url' => 'wss://ws-api.kucoin.com/endpoint'
        ]
    ],

    // Trading default settings
    'trading' => [
        'default_mode' => 'paper', // 'paper' or 'live'
        'default_symbol' => 'BTCUSDT',
        'default_interval' => '15m',
        'default_leverage' => 1,
        'max_leverage' => 10,
        'supported_intervals' => ['1m', '3m', '5m', '15m', '30m', '1h', '2h', '4h', '6h', '12h', '1d', '1w'],
        'min_order_size' => [
            'BTC' => 0.0001,
            'ETH' => 0.001,
            'default' => 0.001
        ]
    ],

    // Risk management settings
    'risk' => [
        'max_risk_per_trade' => 2.0, // percentage
        'max_daily_loss' => 5.0, // percentage
        'max_positions' => 5,
        'default_stop_loss' => 2.0, // percentage
        'default_take_profit' => 4.0, // percentage
        'trailing_stop' => [
            'enabled' => true,
            'activation_percent' => 1.0,
            'trailing_percent' => 0.5
        ]
    ],

    // Indicator default parameters
    'indicators' => [
        'ema' => [
            'periods' => [9, 21, 50, 200]
        ],
        'rsi' => [
            'period' => 14,
            'overbought' => 70,
            'oversold' => 30
        ],
        'macd' => [
            'fast_period' => 12,
            'slow_period' => 26,
            'signal_period' => 9
        ],
        'bollinger_bands' => [
            'period' => 20,
            'std_dev' => 2
        ],
        'atr' => [
            'period' => 14
        ]
    ],

    // Logging configuration
    'logging' => [
        'level' => getenv('LOG_LEVEL') ?: 'info', // debug, info, warning, error
        'path' => __DIR__ . '/../logs',
        'max_files' => 30,
        'format' => '[%datetime%] %level%: %message% %context%'
    ],

    // WebSocket configuration
    'websocket' => [
        'enabled' => true,
        'host' => '0.0.0.0',
        'port' => 8081,
        'heartbeat_interval' => 30
    ],

    // Cache configuration
    'cache' => [
        'enabled' => true,
        'driver' => 'file', // 'file' or 'redis'
        'ttl' => 300, // seconds
        'path' => __DIR__ . '/../cache'
    ],

    // API configuration
    'api' => [
        'prefix' => '/api',
        'version' => 'v1',
        'rate_limit' => [
            'public' => 60, // requests per minute
            'authenticated' => 120
        ],
        'cors' => [
            'enabled' => true,
            'origins' => ['*'], // Restrict in production
            'methods' => ['GET', 'POST', 'PUT', 'DELETE', 'OPTIONS'],
            'headers' => ['Content-Type', 'Authorization', 'X-Requested-With']
        ]
    ]
];
