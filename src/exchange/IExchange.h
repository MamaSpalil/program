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

// A trading pair descriptor returned by getSymbols()
struct SymbolInfo {
    std::string symbol;       // e.g. "BTCUSDT"
    std::string baseAsset;    // e.g. "BTC"
    std::string quoteAsset;   // e.g. "USDT"
    std::string displayName;  // e.g. "BTC/USDT" or "BTC/USDT Swap"
    std::string marketType;   // "spot" or "futures"
};

// Compute maximum bar count for deep analysis based on timeframe.
// Shorter timeframes get more bars to cover similar wall-clock spans.
inline int maxBarsForTimeframe(const std::string& interval) {
    if (interval == "1m")  return 1500;
    if (interval == "3m")  return 1500;
    if (interval == "5m")  return 1500;
    if (interval == "15m") return 1000;
    if (interval == "30m") return 1000;
    if (interval == "1h")  return 1000;
    if (interval == "2h")  return 750;
    if (interval == "4h")  return 750;
    if (interval == "6h")  return 500;
    if (interval == "8h")  return 500;
    if (interval == "12h") return 500;
    if (interval == "1d")  return 365;
    if (interval == "1w")  return 200;
    if (interval == "1M")  return 120;
    return 1000; // default
}

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

    // Fetch available trading pairs from the exchange.
    // Default implementation returns empty — exchanges override as supported.
    virtual std::vector<SymbolInfo> getSymbols(const std::string& marketType = "") {
        (void)marketType;
        return {};
    }
};

} // namespace crypto
