#pragma once
#include "../data/CandleData.h"
#include <functional>
#include <deque>
#include <mutex>
#include <vector>
#include <optional>

namespace crypto {

using CandleCallback = std::function<void(const Candle&)>;

class DataFeed {
public:
    explicit DataFeed(size_t maxHistory = 500);

    void onCandle(const Candle& c);
    void addCallback(CandleCallback cb);

    std::deque<Candle> getHistory() const;
    std::optional<Candle> latest() const;

private:
    size_t maxHistory_;
    std::deque<Candle> history_;
    std::vector<CandleCallback> callbacks_;
    mutable std::mutex mutex_;
};

} // namespace crypto
