#pragma once
#include "TradingDatabase.h"
#include <string>
#include <vector>

namespace crypto {

struct BacktestResult {
    std::string id;
    std::string symbol;
    std::string timeframe;
    long long   dateFrom       = 0;
    long long   dateTo         = 0;
    double      initialBalance = 0;
    double      finalBalance   = 0;
    double      pnl            = 0;
    double      pnlPct         = 0;
    double      winRate        = 0;
    double      sharpe         = 0;
    double      maxDrawdown    = 0;
    double      profitFactor   = 0;
    int         totalTrades    = 0;
    int         winningTrades  = 0;
    int         losingTrades   = 0;
    std::string strategy;
    double      commission     = 0;
    long long   runAt          = 0;
};

struct BtTradeRecord {
    int         id           = 0;
    std::string backtestId;
    std::string symbol;
    std::string side;
    double      entryPrice   = 0;
    double      exitPrice    = 0;
    double      qty          = 0;
    double      pnl          = 0;
    double      commission   = 0;
    long long   entryTime    = 0;
    long long   exitTime     = 0;
};

class BacktestRepository {
public:
    explicit BacktestRepository(TradingDatabase& db);

    bool save(const BacktestResult& result, const std::vector<BtTradeRecord>& trades);

    std::vector<BacktestResult> getAll(const std::string& symbol = "", int limit = 50) const;
    std::vector<BtTradeRecord> getTrades(const std::string& backtestId) const;

    bool remove(const std::string& id);
    bool insertTrade(const BtTradeRecord& t);

private:
    TradingDatabase& db_;
};

} // namespace crypto
