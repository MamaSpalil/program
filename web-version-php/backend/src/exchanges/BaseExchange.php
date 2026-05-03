<?php

namespace CryptoTrader\Exchanges;

/**
 * Base Exchange Class
 * Abstract class for all exchange implementations
 */
abstract class BaseExchange
{
    protected $apiKey;
    protected $apiSecret;
    protected $baseUrl;
    protected $wsUrl;
    protected $testnet;
    protected $config;

    public function __construct($apiKey, $apiSecret, $testnet = true)
    {
        $this->apiKey = $apiKey;
        $this->apiSecret = $apiSecret;
        $this->testnet = $testnet;
        $this->config = require __DIR__ . '/../../config/config.php';
        $this->initialize();
    }

    /**
     * Initialize exchange-specific settings
     */
    abstract protected function initialize();

    /**
     * Get ticker price
     */
    abstract public function getTicker($symbol);

    /**
     * Get candlestick data
     */
    abstract public function getCandles($symbol, $interval, $limit = 100);

    /**
     * Place order
     */
    abstract public function placeOrder($symbol, $side, $type, $quantity, $price = null);

    /**
     * Get open orders
     */
    abstract public function getOpenOrders($symbol = null);

    /**
     * Cancel order
     */
    abstract public function cancelOrder($orderId, $symbol);

    /**
     * Get account balance
     */
    abstract public function getBalance();

    /**
     * Make HTTP request
     */
    protected function httpRequest($method, $endpoint, $params = [], $signed = false)
    {
        $url = $this->baseUrl . $endpoint;

        // Add query parameters
        if (!empty($params) && $method === 'GET') {
            $url .= '?' . http_build_query($params);
        }

        // Initialize cURL
        $ch = curl_init($url);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_FOLLOWLOCATION, true);

        // Set method-specific options
        if ($method === 'POST') {
            curl_setopt($ch, CURLOPT_POST, true);
            if (!empty($params)) {
                curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($params));
            }
        } elseif ($method === 'DELETE') {
            curl_setopt($ch, CURLOPT_CUSTOMREQUEST, 'DELETE');
        }

        // Set headers
        $headers = [
            'Content-Type: application/json',
            'X-MBX-APIKEY: ' . $this->apiKey
        ];
        curl_setopt($ch, CURLOPT_HTTPHEADER, $headers);

        // Execute request
        $response = curl_exec($ch);
        $httpCode = curl_getinfo($ch, CURLINFO_HTTP_CODE);

        if (curl_errno($ch)) {
            throw new \Exception('cURL error: ' . curl_error($ch));
        }

        curl_close($ch);

        $data = json_decode($response, true);

        if ($httpCode >= 400) {
            throw new \Exception('API error: ' . ($data['msg'] ?? 'Unknown error'));
        }

        return $data;
    }

    /**
     * Generate signature for signed requests
     */
    protected function generateSignature($params)
    {
        $queryString = http_build_query($params);
        return hash_hmac('sha256', $queryString, $this->apiSecret);
    }

    /**
     * Encrypt API credentials
     */
    protected function encryptCredentials($data)
    {
        $key = $this->config['security']['encryption_key'];
        $cipher = $this->config['security']['encryption_cipher'];
        $ivLength = openssl_cipher_iv_length($cipher);
        $iv = openssl_random_pseudo_bytes($ivLength);

        $encrypted = openssl_encrypt($data, $cipher, $key, 0, $iv);
        return base64_encode($iv . $encrypted);
    }

    /**
     * Decrypt API credentials
     */
    protected function decryptCredentials($encryptedData)
    {
        $key = $this->config['security']['encryption_key'];
        $cipher = $this->config['security']['encryption_cipher'];
        $data = base64_decode($encryptedData);
        $ivLength = openssl_cipher_iv_length($cipher);

        $iv = substr($data, 0, $ivLength);
        $encrypted = substr($data, $ivLength);

        return openssl_decrypt($encrypted, $cipher, $key, 0, $iv);
    }
}
