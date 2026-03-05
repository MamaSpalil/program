#pragma once
#include "TradingDatabase.h"
#include <string>
#include <vector>

namespace crypto {

struct AlertRecord {
    std::string id;
    std::string symbol;
    std::string condition;
    double      threshold    = 0;
    bool        enabled      = true;
    bool        triggered    = false;
    int         triggerCount = 0;
    long long   lastTriggered = 0;
    std::string notifyType   = "sound";
    long long   createdAt    = 0;
    long long   updatedAt    = 0;
};

class AlertRepository {
public:
    explicit AlertRepository(TradingDatabase& db);

    bool insert(const AlertRecord& a);
    bool update(const AlertRecord& a);
    bool remove(const std::string& id);
    bool setTriggered(const std::string& id, long long time);
    bool resetTriggered(const std::string& id);

    std::vector<AlertRecord> getAll() const;
    std::vector<AlertRecord> getEnabled() const;

    bool migrateFromJSON(const std::string& jsonPath);

private:
    TradingDatabase& db_;
};

} // namespace crypto
