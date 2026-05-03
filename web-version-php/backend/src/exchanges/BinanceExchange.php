<?php

namespace CryptoTrader\Exchanges;

/**
 * Binance Exchange Implementation
 */
class BinanceExchange extends BaseExchange
{
    protected function initialize()
    {
        $exchangeConfig = $this->config['exchanges']['binance'];

        $this->baseUrl = $this->testnet
            ? $exchangeConfig['testnet_url']
            : $exchangeConfig['mainnet_url'];

        $this->wsUrl = $this->testnet
            ? $exchangeConfig['ws_testnet_url']
            : $exchangeConfig['ws_mainnet_url'];
    }

    /**
     * Get ticker price
     */
    public function getTicker($symbol)
    {
        $endpoint = '/api/v3/ticker/24hr';
        $params = ['symbol' => $symbol];

        return $this->httpRequest('GET', $endpoint, $params);
    }

    /**
     * Get candlestick data
     */
    public function getCandles($symbol, $interval, $limit = 100)
    {
        $endpoint = '/api/v3/klines';
        $params = [
            'symbol' => $symbol,
            'interval' => $interval,
            'limit' => min($limit, 1000) // Binance max is 1000
        ];

        return $this->httpRequest('GET', $endpoint, $params);
    }

    /**
     * Place order
     */
    public function placeOrder($symbol, $side, $type, $quantity, $price = null)
    {
        $endpoint = '/api/v3/order';

        $params = [
            'symbol' => $symbol,
            'side' => strtoupper($side),
            'type' => strtoupper($type),
            'quantity' => $quantity,
            'timestamp' => round(microtime(true) * 1000)
        ];

        // Add price for limit orders
        if ($type === 'LIMIT') {
            if ($price === null) {
                throw new \Exception('Price is required for limit orders');
            }
            $params['price'] = $price;
            $params['timeInForce'] = 'GTC';
        }

        // Add signature
        $params['signature'] = $this->generateSignature($params);

        return $this->httpRequest('POST', $endpoint, $params, true);
    }

    /**
     * Get open orders
     */
    public function getOpenOrders($symbol = null)
    {
        $endpoint = '/api/v3/openOrders';

        $params = [
            'timestamp' => round(microtime(true) * 1000)
        ];

        if ($symbol !== null) {
            $params['symbol'] = $symbol;
        }

        // Add signature
        $params['signature'] = $this->generateSignature($params);

        return $this->httpRequest('GET', $endpoint, $params, true);
    }

    /**
     * Cancel order
     */
    public function cancelOrder($orderId, $symbol)
    {
        $endpoint = '/api/v3/order';

        $params = [
            'symbol' => $symbol,
            'orderId' => $orderId,
            'timestamp' => round(microtime(true) * 1000)
        ];

        // Add signature
        $params['signature'] = $this->generateSignature($params);

        return $this->httpRequest('DELETE', $endpoint, $params, true);
    }

    /**
     * Get account balance
     */
    public function getBalance()
    {
        $endpoint = '/api/v3/account';

        $params = [
            'timestamp' => round(microtime(true) * 1000)
        ];

        // Add signature
        $params['signature'] = $this->generateSignature($params);

        $response = $this->httpRequest('GET', $endpoint, $params, true);

        // Format balance data
        $balances = [];
        foreach ($response['balances'] as $balance) {
            if (floatval($balance['free']) > 0 || floatval($balance['locked']) > 0) {
                $balances[] = [
                    'asset' => $balance['asset'],
                    'free' => floatval($balance['free']),
                    'locked' => floatval($balance['locked']),
                    'total' => floatval($balance['free']) + floatval($balance['locked'])
                ];
            }
        }

        return $balances;
    }

    /**
     * Get order book
     */
    public function getOrderBook($symbol, $limit = 100)
    {
        $endpoint = '/api/v3/depth';
        $params = [
            'symbol' => $symbol,
            'limit' => min($limit, 5000)
        ];

        return $this->httpRequest('GET', $endpoint, $params);
    }

    /**
     * Get recent trades
     */
    public function getRecentTrades($symbol, $limit = 100)
    {
        $endpoint = '/api/v3/trades';
        $params = [
            'symbol' => $symbol,
            'limit' => min($limit, 1000)
        ];

        return $this->httpRequest('GET', $endpoint, $params);
    }

    /**
     * Get exchange info
     */
    public function getExchangeInfo()
    {
        $endpoint = '/api/v3/exchangeInfo';
        return $this->httpRequest('GET', $endpoint);
    }

    /**
     * Test connectivity
     */
    public function ping()
    {
        $endpoint = '/api/v3/ping';
        return $this->httpRequest('GET', $endpoint);
    }

    /**
     * Get server time
     */
    public function getServerTime()
    {
        $endpoint = '/api/v3/time';
        return $this->httpRequest('GET', $endpoint);
    }
}
