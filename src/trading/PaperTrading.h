#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <nlohmann/json.hpp>

namespace crypto {

struct PaperPosition {
    std::string symbol;
    std::string side;       // "BUY" / "SELL"
    double      quantity{0.0};
    double      entryPrice{0.0};
    double      currentPrice{0.0};
    double      pnl{0.0};
};

struct PaperTrade {
    int64_t     timestamp{0};
    std::string symbol;
    std::string side;
    double      quantity{0.0};
    double      entryPrice{0.0};
    double      exitPrice{0.0};
    double      pnl{0.0};
};

struct PaperAccount {
    double                     balance{10000.0};
    double                     equity{10000.0};
    std::vector<PaperPosition> openPositions;
    std::vector<PaperTrade>    closedTrades;
    double                     totalPnL{0.0};
    double                     maxDrawdown{0.0};
    double                     peakEquity{10000.0};
};

class PaperTrading {
public:
    explicit PaperTrading(double initialBalance = 10000.0);

    // Open a paper position
    bool openPosition(const std::string& symbol, const std::string& side,
                      double qty, double price);

    // Close a paper position by symbol
    bool closePosition(const std::string& symbol, double currentPrice);

    // Update mark prices for open positions
    void updatePrices(const std::string& symbol, double price);

    // Get account snapshot
    PaperAccount getAccount() const;

    // Reset to initial state
    void reset(double initialBalance = 10000.0);

    // Persistence
    bool saveToFile(const std::string& path) const;
    bool loadFromFile(const std::string& path);

    bool isPaperMode() const { return true; }

private:
    void recalcEquity();

    mutable std::mutex mutex_;
    PaperAccount account_;
    double initialBalance_;
};

} // namespace crypto
