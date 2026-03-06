#pragma once
#include "IExchange.h"
#include "WebSocketClient.h"
#include "EndpointManager.h"
#include <string>
#include <memory>
#include <mutex>
#include <queue>
#include <chrono>
#include <atomic>
#include <nlohmann/json.hpp>

namespace crypto {

class BinanceExchange : public IExchange {
public:
    BinanceExchange(const std::string& apiKey,
                     const std::string& apiSecret,
                     const std::string& baseUrl = "https://testnet.binance.vision",
                     const std::string& wsHost  = "testnet.binance.vision",
                     const std::string& wsPort  = "443",
                     const std::string& futuresBaseUrl = "",
                     const std::string& futuresWsHost  = "",
                     const std::string& futuresWsPort  = "");
    ~BinanceExchange() override;

    std::vector<Candle> getKlines(const std::string& symbol,
                                   const std::string& interval,
                                   int limit = 500) override;

    double getPrice(const std::string& symbol) override;
    OrderResponse placeOrder(const OrderRequest& req) override;
    AccountBalance getBalance() override;

    void subscribeKline(const std::string& symbol,
                        const std::string& interval,
                        std::function<void(const Candle&)> cb) override;
    void connect() override;
    void disconnect() override;
    bool testConnection(std::string& outError) override;
    std::vector<SymbolInfo> getSymbols(const std::string& marketType = "") override;
    OrderBook getOrderBook(const std::string& symbol, int depth = 20) override;

    void setMarketType(const std::string& marketType) override;
    std::vector<OpenOrderInfo> getOpenOrders(const std::string& symbol = "") override;
    std::vector<UserTradeInfo> getMyTrades(const std::string& symbol, int limit = 20) override;
    std::vector<PositionInfo> getPositionRisk(const std::string& symbol = "") override;
    FuturesBalanceInfo getFuturesBalance() override;
    bool cancelOrder(const std::string& symbol, const std::string& orderId) override;
    bool setLeverage(const std::string& symbol, int leverage) override;
    int getLeverage(const std::string& symbol) override;

    // ── Rate limit constants (Binance API, updated March 2026) ─────────────
    // IP rate limits: 6000 weight/min (spot), 2400 weight/min (futures)
    static constexpr int MAX_REQUESTS_PER_MINUTE_SPOT    = 1200;
    static constexpr int MAX_REQUESTS_PER_MINUTE_FUTURES = 400;
    // Order rate limits — Spot: 100/10s, 200 000/24h; Futures: 1 200/min
    static constexpr int MAX_ORDERS_PER_10S_SPOT       = 100;
    static constexpr int MAX_ORDERS_PER_24H_SPOT       = 200000;
    static constexpr int MAX_ORDERS_PER_MINUTE_FUTURES = 1200;
    // WebSocket: 300 connections per 5 minutes per IP
    static constexpr int MAX_WS_CONNECTIONS_PER_5MIN = 300;
    // Iceberg order parts limit (increased to 100 from March 12, 2026)
    static constexpr int MAX_ICEBERG_PARTS = 100;

private:
    std::string apiKey_;
    std::string apiSecret_;
    std::string baseUrl_;
    std::string wsHost_;
    std::string wsPort_;
    std::string marketType_{"spot"};
    std::string futuresBaseUrl_;
    std::string futuresWsHost_;
    std::string futuresWsPort_;

    std::unique_ptr<WebSocketClient> ws_;
    std::function<void(const Candle&)> klineCb_;
    mutable std::mutex rateMutex_;

    // Rate limiting: track request times in a sliding 1-minute window
    mutable std::queue<std::chrono::steady_clock::time_point> requestTimes_;

    // Order rate limiting (account-level)
    mutable std::queue<std::chrono::steady_clock::time_point> orderTimes_;
    mutable std::mutex orderRateMutex_;
    mutable std::atomic<int> dailyOrderCount_{0};
    mutable std::chrono::steady_clock::time_point dailyOrderReset_{std::chrono::steady_clock::now()};

    // WebSocket connection rate limiting
    mutable std::queue<std::chrono::steady_clock::time_point> wsConnectTimes_;
    mutable std::mutex wsRateMutex_;

    // Geo-blocking fallback state
    mutable int spotEndpointIdx_{0};
    mutable int futuresEndpointIdx_{0};
    mutable std::atomic<bool> useAltEndpoint_{false};

    std::string sign(const std::string& payload) const;
    std::string httpGet(const std::string& path, bool signed_ = false) const;
    std::string httpGetUrl(const std::string& fullUrl, bool signed_ = false) const;
    std::string effectiveBaseUrl() const;
    std::string replaceBase(const std::string& url,
                            const std::string& oldBase,
                            const std::string& newBase) const;
    std::string httpPost(const std::string& path,
                          const std::string& body) const;
    void rateLimit() const;
    void orderRateLimit() const;
    void wsConnectionRateLimit() const;
    Candle parseKlineJson(const nlohmann::json& k) const;
    void onWsMessage(const std::string& msg);

public:
    // Whether an alternative endpoint is currently in use (for UI display)
    bool isUsingAltEndpoint() const { return useAltEndpoint_.load(); }
};

} // namespace crypto
