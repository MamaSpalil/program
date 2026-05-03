<?php

/**
 * Analysis API Routes
 */

use CryptoTrader\Exchanges\BinanceExchange;
use CryptoTrader\Indicators\Indicators;
use CryptoTrader\Auth\Auth;

function handleAnalysisRequest($action, $method, $input)
{
    // Verify authentication
    $auth = new Auth();
    $session = $auth->validateSession();

    if (!$session) {
        http_response_code(401);
        echo json_encode(['error' => 'Unauthorized']);
        return;
    }

    $config = require __DIR__ . '/../../config/config.php';
    $exchange = new BinanceExchange('', '', true);

    // Parse action
    $parts = explode('/', $action);
    $endpoint = $parts[0];

    try {
        switch ($endpoint) {
            case 'indicators':
                if ($method !== 'GET') {
                    http_response_code(405);
                    echo json_encode(['error' => 'Method not allowed']);
                    return;
                }

                $symbol = $parts[1] ?? 'BTCUSDT';
                $interval = $_GET['interval'] ?? '15m';
                $limit = 200;

                // Get candle data
                $candles = $exchange->getCandles($symbol, $interval, $limit);

                // Extract close prices
                $closePrices = array_map(function ($candle) {
                    return floatval($candle[4]);
                }, $candles);

                $highPrices = array_map(function ($candle) {
                    return floatval($candle[2]);
                }, $candles);

                $lowPrices = array_map(function ($candle) {
                    return floatval($candle[3]);
                }, $candles);

                // Calculate indicators
                $indicators = [];

                // EMA
                $indicators['ema_9'] = Indicators::calculateEMA($closePrices, 9);
                $indicators['ema_21'] = Indicators::calculateEMA($closePrices, 21);
                $indicators['ema_50'] = Indicators::calculateEMA($closePrices, 50);

                // RSI
                $indicators['rsi'] = Indicators::calculateRSI($closePrices, 14);

                // MACD
                $macdData = Indicators::calculateMACD($closePrices, 12, 26, 9);
                $indicators['macd'] = $macdData['macd'];
                $indicators['macd_signal'] = $macdData['signal'];
                $indicators['macd_histogram'] = $macdData['histogram'];

                // Bollinger Bands
                $bbData = Indicators::calculateBollingerBands($closePrices, 20, 2);
                $indicators['bb_upper'] = $bbData['upper'];
                $indicators['bb_middle'] = $bbData['middle'];
                $indicators['bb_lower'] = $bbData['lower'];

                // ATR
                $indicators['atr'] = Indicators::calculateATR($highPrices, $lowPrices, $closePrices, 14);

                echo json_encode([
                    'success' => true,
                    'data' => $indicators
                ]);
                break;

            case 'signal':
                if ($method !== 'POST') {
                    http_response_code(405);
                    echo json_encode(['error' => 'Method not allowed']);
                    return;
                }

                // Call Java ML service
                $mlServiceUrl = $config['ml_service']['url'] . $config['ml_service']['endpoints']['signal'];

                $requestData = [
                    'symbol' => $input['symbol'] ?? 'BTCUSDT',
                    'interval' => $input['interval'] ?? '15m',
                    'candles' => $input['candles'] ?? [],
                    'indicators' => $input['indicators'] ?? []
                ];

                // Make HTTP request to ML service
                $ch = curl_init($mlServiceUrl);
                curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
                curl_setopt($ch, CURLOPT_POST, true);
                curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($requestData));
                curl_setopt($ch, CURLOPT_HTTPHEADER, [
                    'Content-Type: application/json'
                ]);
                curl_setopt($ch, CURLOPT_TIMEOUT, $config['ml_service']['timeout']);

                $response = curl_exec($ch);
                $httpCode = curl_getinfo($ch, CURLINFO_HTTP_CODE);

                if (curl_errno($ch)) {
                    throw new \Exception('ML service connection error: ' . curl_error($ch));
                }

                curl_close($ch);

                if ($httpCode !== 200) {
                    throw new \Exception('ML service returned error: HTTP ' . $httpCode);
                }

                $mlResponse = json_decode($response, true);

                echo json_encode([
                    'success' => true,
                    'data' => [
                        'signal' => $mlResponse['signal'] ?? 'HOLD',
                        'confidence' => $mlResponse['confidence'] ?? 0.0,
                        'lstm_prediction' => $mlResponse['lstmPrediction'] ?? 0.0,
                        'xgboost_prediction' => $mlResponse['xgboostPrediction'] ?? 0.0,
                        'ensemble_prediction' => $mlResponse['ensemblePrediction'] ?? 0.0,
                        'timestamp' => $mlResponse['timestamp'] ?? time()
                    ]
                ]);
                break;

            case 'backtest':
                if ($method !== 'POST') {
                    http_response_code(405);
                    echo json_encode(['error' => 'Method not allowed']);
                    return;
                }

                // Backtest implementation would go here
                echo json_encode([
                    'success' => true,
                    'message' => 'Backtesting not yet implemented'
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
