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

class OKXExchange : public IExchange {
public:
    OKXExchange(const std::string& apiKey,
                const std::string& apiSecret,
                const std::string& passphrase,
                const std::string& baseUrl = "https://www.okx.com",
                const std::string& wsHost  = "ws.okx.com",
                const std::string& wsPort  = "8443");
    ~OKXExchange() override;

    std::vector<Candle> getKlines(const std::string& symbol,
                                   const std::string& interval,
                                   int limit = 300) override;

    double getPrice(const std::string& symbol) override;
    OrderResponse placeOrder(const OrderRequest& req) override;
    AccountBalance getBalance() override;

    void subscribeKline(const std::string& symbol,
                        const std::string& interval,
                        std::function<void(const Candle&)> cb) override;
    void connect() override;
    void disconnect() override;
    bool testConnection(std::string& outError) override;

private:
    std::string apiKey_;
    std::string apiSecret_;
    std::string passphrase_;
    std::string baseUrl_;
    std::string wsHost_;
    std::string wsPort_;
    std::unique_ptr<WebSocketClient> ws_;
    std::function<void(const Candle&)> klineCb_;
    mutable std::mutex rateMutex_;

    // Rate limiting: track request times
    mutable std::queue<std::chrono::steady_clock::time_point> requestTimes_;
    static constexpr int MAX_REQUESTS_PER_SECOND = 10;

    std::string sign(const std::string& timestamp,
                     const std::string& method,
                     const std::string& path) const;
    std::string httpGet(const std::string& path, bool signed_ = false) const;
    void rateLimit() const;
    void onWsMessage(const std::string& msg);
};

} // namespace crypto
