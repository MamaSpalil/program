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

// Open order from exchange
struct OpenOrderInfo {
    std::string symbol;
    std::string orderId;
    std::string side;       // "BUY" or "SELL"
    std::string type;       // "LIMIT", "MARKET", etc.
    double price{0.0};
    double qty{0.0};
    double executedQty{0.0};
    int64_t time{0};
};

// User trade (fill)
struct UserTradeInfo {
    int64_t time{0};
    std::string symbol;
    std::string side;       // "BUY" or "SELL"
    double price{0.0};
    double qty{0.0};
    double commission{0.0};
    std::string commissionAsset;
};

// Futures position info
struct PositionInfo {
    std::string symbol;
    double positionAmt{0.0};
    double entryPrice{0.0};
    double unrealizedProfit{0.0};
    double realizedProfit{0.0};
    std::string marginType;  // "cross" or "isolated"
    double leverage{1.0};
};

// Futures account balance
struct FuturesBalanceInfo {
    double totalWalletBalance{0.0};
    double totalUnrealizedProfit{0.0};
    double totalMarginBalance{0.0};
    std::vector<PositionInfo> positions;
};

// OKX account balance detail
struct AccountBalanceDetail {
    std::string currency;
    double availBal{0.0};
    double frozenBal{0.0};
    double usdValue{0.0};
};

// Compute maximum bar count for deep analysis based on timeframe.
// Shorter timeframes get more bars to cover similar wall-clock spans.
// Minimum 5000 bars for intraday timeframes for full chart visualization.
inline int maxBarsForTimeframe(const std::string& interval) {
    if (interval == "1m")  return 5000;
    if (interval == "3m")  return 5000;
    if (interval == "5m")  return 5000;
    if (interval == "15m") return 5000;
    if (interval == "30m") return 5000;
    if (interval == "1h")  return 5000;
    if (interval == "2h")  return 5000;
    if (interval == "4h")  return 5000;
    if (interval == "6h")  return 3000;
    if (interval == "8h")  return 3000;
    if (interval == "12h") return 2000;
    if (interval == "1d")  return 1500;
    if (interval == "1w")  return 500;
    if (interval == "1M")  return 300;
    return 5000; // default
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

    // Fetch order book (depth) snapshot for a symbol.
    // Default implementation returns empty — exchanges override as supported.
    virtual OrderBook getOrderBook(const std::string& symbol, int depth = 20) {
        (void)symbol; (void)depth;
        return {};
    }

    // Set the current market type ("spot" or "futures") for endpoint selection
    virtual void setMarketType(const std::string& marketType) { (void)marketType; }

    // Get open orders for a symbol (signed)
    virtual std::vector<OpenOrderInfo> getOpenOrders(const std::string& symbol = "") {
        (void)symbol;
        return {};
    }

    // Get recent user trades (signed)
    virtual std::vector<UserTradeInfo> getMyTrades(const std::string& symbol, int limit = 20) {
        (void)symbol; (void)limit;
        return {};
    }

    // Get futures position risk / positions (signed)
    virtual std::vector<PositionInfo> getPositionRisk(const std::string& symbol = "") {
        (void)symbol;
        return {};
    }

    // Get futures account balance (signed)
    virtual FuturesBalanceInfo getFuturesBalance() {
        return {};
    }

    // Get account balance details (for OKX style balances)
    virtual std::vector<AccountBalanceDetail> getAccountBalanceDetails() {
        return {};
    }
};

} // namespace crypto
