#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <nlohmann/json.hpp>
#include "../data/TradeRepository.h"

namespace crypto {

struct HistoricalTrade {
    std::string id;
    std::string symbol;
    std::string side;       // "BUY" / "SELL"
    std::string type;       // "MARKET" / "LIMIT"
    double      qty{0.0};
    double      entryPrice{0.0};
    double      exitPrice{0.0};
    double      pnl{0.0};
    double      commission{0.0};
    int64_t     entryTime{0};
    int64_t     exitTime{0};
    bool        isPaper{false};
};

class TradeHistory {
public:
    void addTrade(const HistoricalTrade& t);

    std::vector<HistoricalTrade> getAll() const;
    std::vector<HistoricalTrade> getBySymbol(const std::string& s) const;
    std::vector<HistoricalTrade> getByDateRange(int64_t from, int64_t to) const;
    std::vector<HistoricalTrade> getPaperTrades() const;
    std::vector<HistoricalTrade> getLiveTrades() const;

    // Statistics
    int    totalTrades() const;
    int    winCount() const;
    int    lossCount() const;
    double totalPnl() const;
    double winRate() const;
    double avgWin() const;
    double avgLoss() const;
    double profitFactor() const;
    double maxDrawdown() const;

    // Persistence
    bool saveToDisk(const std::string& path) const;
    bool loadFromDisk(const std::string& path);
    void clear();

    // Optional: set database repository for parallel persistence
    void setRepository(TradeRepository* repo) { repo_ = repo; }

private:
    mutable std::mutex mutex_;
    std::vector<HistoricalTrade> trades_;
    TradeRepository* repo_{nullptr};
};

} // namespace crypto
