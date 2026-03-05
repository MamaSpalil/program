#pragma once
#include "TradingDatabase.h"
#include "TradeRepository.h"
#include "AlertRepository.h"
#include "DrawingRepository.h"
#include <string>

namespace crypto {

class DatabaseMigrator {
public:
    DatabaseMigrator(TradingDatabase& db,
                     TradeRepository& tradeRepo,
                     AlertRepository& alertRepo,
                     DrawingRepository& drawingRepo);

    void runIfNeeded();

private:
    bool isMigrated();
    void markMigrated();

    TradingDatabase&   db_;
    TradeRepository&   tradeRepo_;
    AlertRepository&   alertRepo_;
    DrawingRepository& drawingRepo_;
};

} // namespace crypto
