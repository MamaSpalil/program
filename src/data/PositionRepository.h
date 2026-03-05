#pragma once
#include "TradingDatabase.h"
#include <string>
#include <vector>
#include <optional>

namespace crypto {

struct PositionRecord {
    std::string id;
    std::string symbol, exchange;
    std::string side;
    double      qty          = 0;
    double      entryPrice   = 0;
    double      currentPrice = 0;
    double      unrealizedPnl = 0;
    double      realizedPnl  = 0;
    double      tpPrice      = 0;
    double      slPrice      = 0;
    double      leverage     = 1;
    bool        isPaper      = false;
    long long   openedAt     = 0;
    long long   updatedAt    = 0;
};

class PositionRepository {
public:
    explicit PositionRepository(TradingDatabase& db);

    bool upsert(const PositionRecord& p);
    bool updatePrice(const std::string& symbol, const std::string& exchange,
                     bool isPaper, double currentPrice, double unrealizedPnl);
    bool updateTPSL(const std::string& symbol, const std::string& exchange,
                    bool isPaper, double tp, double sl);

    std::optional<PositionRecord> get(const std::string& symbol,
                                       const std::string& exchange,
                                       bool isPaper) const;
    std::vector<PositionRecord> getAll(bool isPaper) const;
    bool remove(const std::string& symbol, const std::string& exchange, bool isPaper);

private:
    TradingDatabase& db_;
};

} // namespace crypto
