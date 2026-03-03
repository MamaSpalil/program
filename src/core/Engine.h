#pragma once
#include "../exchange/IExchange.h"
#include "../data/ExchangeDB.h"
#include "../indicators/PineScriptIndicator.h"
#include <atomic>
#include <memory>
#include <string>
#include <map>
#include <deque>
#include <functional>
#include <nlohmann/json.hpp>

namespace crypto {

/// Callback payload pushed to the GUI on every candle update
struct EngineUpdate {
    Candle candle;
    std::deque<Candle> candleHistory;
    double emaFast{0}, emaSlow{0}, emaTrend{0};
    double rsi{50}, atr{0};
    MACDResult macd;
    BBResult bb;
    Signal signal;
    double equity{0};
    double drawdown{0};
    std::map<std::string, std::map<std::string, double>> userIndicatorPlots;
    OrderBook orderBook;
};

class Engine {
public:
    explicit Engine(const std::string& configPath);
    ~Engine();

    void run();
    void stop();

    // Initialize exchange and verify connection. Returns true if API test succeeds.
    // On failure, sets outError with a description. Must be called before run().
    bool initAndTestConnection(std::string& outError);

    // Place a manual order through the exchange
    OrderResponse placeOrder(const OrderRequest& req);

    // Get user indicator plots (thread-safe snapshot)
    std::map<std::string, std::map<std::string, double>> getUserIndicatorPlots() const;

    // Get available trading pairs from the exchange
    std::vector<SymbolInfo> getSymbols(const std::string& marketType = "") const;

    // Set market type on the exchange (spot/futures)
    void setMarketType(const std::string& marketType);

    // Reload historical candles for a new symbol/interval pair
    void reloadCandles(const std::string& symbol, const std::string& interval);

    // Get current ticker price
    double getPrice(const std::string& symbol) const;

    // Get open orders (signed)
    std::vector<OpenOrderInfo> getOpenOrders(const std::string& symbol = "") const;

    // Get user's recent trades (signed)
    std::vector<UserTradeInfo> getMyTrades(const std::string& symbol, int limit = 20) const;

    // Get futures position risk (signed)
    std::vector<PositionInfo> getPositionRisk(const std::string& symbol = "") const;

    // Get futures balance (signed)
    FuturesBalanceInfo getFuturesBalance() const;

    // Get account balance details (OKX style)
    std::vector<AccountBalanceDetail> getAccountBalanceDetails() const;

    // Record a trade in the database
    void recordTrade(const TradeRecord& tr);

    // Get trade statistics
    std::vector<TradeRecord> listTrades() const;
    double totalPnl() const;
    double winRate() const;

    // Callback invoked after each candle is processed (historical + live)
    using OnUpdateCallback = std::function<void(const EngineUpdate&)>;
    void setOnUpdateCallback(OnUpdateCallback cb);

private:
    void loadConfig(const std::string& path);
    void initComponents();
    void mainLoop();
    void updateDashboard(const Candle& c);

    nlohmann::json config_;
    std::atomic<bool> running_{false};
    bool componentsInitialized_{false};

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace crypto
