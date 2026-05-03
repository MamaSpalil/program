<?php

/**
 * Market API Routes
 */

use CryptoTrader\Exchanges\BinanceExchange;
use CryptoTrader\Indicators\Indicators;
use CryptoTrader\Auth\Auth;

function handleMarketRequest($action, $method, $input)
{
    // Verify authentication
    $auth = new Auth();
    $session = $auth->validateSession();

    if (!$session) {
        http_response_code(401);
        echo json_encode(['error' => 'Unauthorized']);
        return;
    }

    // Create exchange instance (simplified - should load user's exchange credentials)
    $exchange = new BinanceExchange('', '', true); // Empty keys for public endpoints

    // Parse action
    $parts = explode('/', $action);
    $endpoint = $parts[0];

    try {
        switch ($endpoint) {
            case 'ticker':
                if ($method !== 'GET') {
                    http_response_code(405);
                    echo json_encode(['error' => 'Method not allowed']);
                    return;
                }

                $symbol = $parts[1] ?? 'BTCUSDT';
                $ticker = $exchange->getTicker($symbol);

                echo json_encode([
                    'success' => true,
                    'data' => [
                        'symbol' => $ticker['symbol'],
                        'price' => $ticker['lastPrice'],
                        'priceChange' => $ticker['priceChange'],
                        'priceChangePercent' => $ticker['priceChangePercent'],
                        'volume' => $ticker['volume'],
                        'high' => $ticker['highPrice'],
                        'low' => $ticker['lowPrice']
                    ]
                ]);
                break;

            case 'candles':
                if ($method !== 'GET') {
                    http_response_code(405);
                    echo json_encode(['error' => 'Method not allowed']);
                    return;
                }

                $symbol = $parts[1] ?? 'BTCUSDT';
                $interval = $_GET['interval'] ?? '15m';
                $limit = intval($_GET['limit'] ?? 100);

                $candles = $exchange->getCandles($symbol, $interval, $limit);

                echo json_encode([
                    'success' => true,
                    'data' => $candles
                ]);
                break;

            case 'orderbook':
                if ($method !== 'GET') {
                    http_response_code(405);
                    echo json_encode(['error' => 'Method not allowed']);
                    return;
                }

                $symbol = $parts[1] ?? 'BTCUSDT';
                $limit = intval($_GET['limit'] ?? 100);

                $orderbook = $exchange->getOrderBook($symbol, $limit);

                echo json_encode([
                    'success' => true,
                    'data' => $orderbook
                ]);
                break;

            case 'trades':
                if ($method !== 'GET') {
                    http_response_code(405);
                    echo json_encode(['error' => 'Method not allowed']);
                    return;
                }

                $symbol = $parts[1] ?? 'BTCUSDT';
                $limit = intval($_GET['limit'] ?? 100);

                $trades = $exchange->getRecentTrades($symbol, $limit);

                echo json_encode([
                    'success' => true,
                    'data' => $trades
                ]);
                break;

            default:
                http_response_code(404);
                echo json_encode(['error' => 'Endpoint not found']);
        }
    } catch (\Exception $e) {
        http_response_code(500);
        echo json_encode(['error' => $e->getMessage()]);
    }
}
