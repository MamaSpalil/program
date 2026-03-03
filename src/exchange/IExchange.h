#pragma once
#include "../data/CandleData.h"
#include <string>
#include <vector>
#include <functional>
#include <optional>

namespace crypto {

struct OrderRequest {
    enum class Side { BUY, SELL };
    enum class Type { MARKET, LIMIT, STOP_LOSS };
    std::string symbol;
    Side side{Side::BUY};
    Type type{Type::MARKET};
    double qty{0.0};
    double price{0.0};
    double stopPrice{0.0};
};

struct OrderResponse {
    std::string orderId;
    std::string status;
    double executedQty{0.0};
    double executedPrice{0.0};
};

struct AccountBalance {
    double totalUSDT{0.0};
    double availableUSDT{0.0};
    double btcBalance{0.0};
};

class IExchange {
public:
    virtual ~IExchange() = default;

    // Test connection by making a lightweight API call (e.g. getPrice).
    // Returns true on success; on failure sets outError and returns false.
    virtual bool testConnection(std::string& outError) = 0;

    virtual std::vector<Candle> getKlines(const std::string& symbol,
                                           const std::string& interval,
                                           int limit = 500) = 0;

    virtual double getPrice(const std::string& symbol) = 0;

    virtual OrderResponse placeOrder(const OrderRequest& req) = 0;
    virtual AccountBalance getBalance() = 0;

    virtual void subscribeKline(const std::string& symbol,
                                const std::string& interval,
                                std::function<void(const Candle&)> cb) = 0;
    virtual void connect() = 0;
    virtual void disconnect() = 0;
};

} // namespace crypto
