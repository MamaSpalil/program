#pragma once
#include "TradingDatabase.h"
#include "../ui/DrawingTools.h"
#include <string>
#include <vector>

namespace crypto {

class DrawingRepository {
public:
    explicit DrawingRepository(TradingDatabase& db);

    bool insert(const DrawingObject& d, const std::string& exchange);
    bool remove(const std::string& id);
    bool update(const DrawingObject& d);
    void clear(const std::string& symbol, const std::string& exchange);

    std::vector<DrawingObject> load(const std::string& symbol,
                                     const std::string& exchange) const;

private:
    TradingDatabase& db_;
};

} // namespace crypto
