#pragma once
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <cmath>
#include <algorithm>
#include "../trading/TradeHistory.h"

namespace crypto {

class TaxReporter {
public:
    enum class Method { FIFO, LIFO };

    struct TaxLot {
        double    qty{0.0};
        double    costBasis{0.0};
        long long acquireTime{0};
    };

    struct TaxEvent {
        std::string symbol;
        double      qty{0.0};
        double      proceeds{0.0};
        double      costBasis{0.0};
        double      gainLoss{0.0};
        long long   openTime{0};
        long long   closeTime{0};
        bool        isLongTerm{false};
    };

    std::vector<TaxEvent> calculate(
        const std::vector<HistoricalTrade>& trades,
        Method method,
        int taxYear) const;

    static bool exportCSV(
        const std::vector<TaxEvent>& events,
        const std::string& filename);
};

} // namespace crypto
