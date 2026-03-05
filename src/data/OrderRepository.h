#pragma once
#include "TradingDatabase.h"
#include <string>
#include <vector>

namespace crypto {

struct OrderRecord {
    std::string id;
    std::string exchangeOrderId;
    std::string symbol, exchange;
    std::string side, type;
    double      qty          = 0;
    double      price        = 0;
    double      stopPrice    = 0;
    double      filledQty    = 0;
    double      avgFillPrice = 0;
    std::string status       = "NEW";
    bool        isPaper      = false;
    bool        reduceOnly   = false;
    long long   createdAt    = 0;
    long long   updatedAt    = 0;
};

class OrderRepository {
public:
    explicit OrderRepository(TradingDatabase& db);

    bool insert(const OrderRecord& o);
    bool updateStatus(const std::string& id, const std::string& status,
                      double filledQty, double avgFillPrice);

    std::vector<OrderRecord> getOpen(const std::string& symbol = "") const;
    std::vector<OrderRecord> getAll(const std::string& symbol = "", int limit = 100) const;
    bool deleteOld(int days);

private:
    TradingDatabase& db_;
};

} // namespace crypto
