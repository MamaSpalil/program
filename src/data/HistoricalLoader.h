#pragma once
#include "../data/CandleData.h"
#include <string>
#include <vector>

namespace crypto {

class HistoricalLoader {
public:
    HistoricalLoader(const std::string& baseUrl,
                     const std::string& apiKey,
                     const std::string& apiSecret);

    std::vector<Candle> load(const std::string& symbol,
                              const std::string& interval,
                              int limit = 500,
                              int64_t startTime = 0,
                              int64_t endTime = 0) const;

private:
    std::string baseUrl_;
    std::string apiKey_;
    std::string apiSecret_;

    std::string httpGet(const std::string& url) const;
    std::vector<Candle> parseKlines(const std::string& json) const;
};

} // namespace crypto
