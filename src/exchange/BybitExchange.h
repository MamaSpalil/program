#pragma once
#include "IExchange.h"
#include "WebSocketClient.h"
#include <memory>
#include <string>
#include <functional>
#include <mutex>
#include <queue>
#include <chrono>

namespace crypto {

class BybitExchange : public IExchange {
public:
    BybitExchange(const std::string& apiKey,
                   const std::string& apiSecret,
                   const std::string& baseUrl = "https://api-testnet.bybit.com",
                   const std::string& wsHost  = "stream-testnet.bybit.com",
                   const std::string& wsPort  = "443");
    ~BybitExchange() override;

    std::vector<Candle> getKlines(const std::string& symbol,
                                   const std::string& interval,
                                   int limit = 200) override;

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
    std::vector<PositionInfo> getPositionRisk(const std::string& symbol = "") override;
    bool cancelOrder(const std::string& symbol, const std::string& orderId) override;
    bool setLeverage(const std::string& symbol, int leverage) override;
    int getLeverage(const std::string& symbol) override;

private:
    std::string apiKey_;
    std::string apiSecret_;
    std::string baseUrl_;
    std::string wsHost_;
    std::string wsPort_;
    std::unique_ptr<WebSocketClient> ws_;
    std::function<void(const Candle&)> klineCb_;
    mutable std::mutex rateMutex_;

    // Rate limiting: sliding window
    mutable std::queue<std::chrono::steady_clock::time_point> requestTimes_;
    static constexpr int MAX_REQUESTS_PER_SECOND = 50; // public endpoints

    std::string sign(const std::string& payload) const;
    std::string httpGet(const std::string& path) const;
    std::string httpPost(const std::string& path, const std::string& body) const;
    void rateLimit() const;
    void onWsMessage(const std::string& msg);
};

} // namespace crypto
