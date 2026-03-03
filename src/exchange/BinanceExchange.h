#pragma once
#include "IExchange.h"
#include "WebSocketClient.h"
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
                     const std::string& wsPort  = "443");
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

private:
    std::string apiKey_;
    std::string apiSecret_;
    std::string baseUrl_;
    std::string wsHost_;
    std::string wsPort_;
    std::string marketType_{"spot"};
    std::string futuresBaseUrl_;
    std::string futuresWsHost_;

    std::unique_ptr<WebSocketClient> ws_;
    std::function<void(const Candle&)> klineCb_;
    mutable std::mutex rateMutex_;

    // Rate limiting: track request times
    mutable std::queue<std::chrono::steady_clock::time_point> requestTimes_;
    static constexpr int MAX_REQUESTS_PER_SECOND = 10;

    std::string sign(const std::string& payload) const;
    std::string httpGet(const std::string& path, bool signed_ = false) const;
    std::string httpGetUrl(const std::string& fullUrl, bool signed_ = false) const;
    std::string effectiveBaseUrl() const;
    std::string httpPost(const std::string& path,
                          const std::string& body) const;
    void rateLimit() const;
    Candle parseKlineJson(const nlohmann::json& k) const;
    void onWsMessage(const std::string& msg);
};

} // namespace crypto
