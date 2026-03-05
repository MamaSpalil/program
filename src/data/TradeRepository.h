#pragma once
#include "TradingDatabase.h"
#include <string>
#include <vector>

namespace crypto {

struct TradingRecord {
    std::string id;
    std::string symbol, exchange;
    std::string side, type;
    double      qty          = 0;
    double      entryPrice   = 0;
    double      exitPrice    = 0;
    double      pnl          = 0;
    double      commission   = 0;
    long long   entryTime    = 0;
    long long   exitTime     = 0;
    bool        isPaper      = false;
    std::string strategy;
    std::string status       = "open";
    long long   createdAt    = 0;
    long long   updatedAt    = 0;
};

class TradeRepository {
public:
    explicit TradeRepository(TradingDatabase& db);

    bool upsert(const TradingRecord& t);
    bool close(const std::string& id, double exitPrice, double pnl, long long exitTime);

    std::vector<TradingRecord> getOpen(bool isPaper) const;
    std::vector<TradingRecord> getHistory(const std::string& symbol = "", int limit = 100) const;

    bool migrateFromJSON(const std::string& jsonPath);

private:
    TradingDatabase& db_;
};

} // namespace crypto
