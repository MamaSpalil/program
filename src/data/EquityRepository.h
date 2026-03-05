#pragma once
#include "TradingDatabase.h"
#include <string>
#include <vector>

namespace crypto {

struct EquitySnapshot {
    long long timestamp     = 0;
    double    equity        = 0;
    double    balance       = 0;
    double    unrealizedPnl = 0;
    bool      isPaper       = false;
};

class EquityRepository {
public:
    explicit EquityRepository(TradingDatabase& db);

    void push(const EquitySnapshot& snap);
    void flush();

    std::vector<EquitySnapshot> getHistory(bool isPaper, int limit = 1000) const;
    void pruneOld(int keepDays);

private:
    TradingDatabase& db_;
    std::vector<EquitySnapshot> buffer_;
    std::mutex bufferMutex_;
};

} // namespace crypto
