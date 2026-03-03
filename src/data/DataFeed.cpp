#include "DataFeed.h"
#include <optional>

namespace crypto {

DataFeed::DataFeed(size_t maxHistory) : maxHistory_(maxHistory) {}

void DataFeed::onCandle(const Candle& c) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        history_.push_back(c);
        if (history_.size() > maxHistory_) {
            history_.pop_front();
        }
    }
    for (auto& cb : callbacks_) {
        cb(c);
    }
}

void DataFeed::addCallback(CandleCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    callbacks_.push_back(std::move(cb));
}

std::deque<Candle> DataFeed::getHistory() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return history_;
}

std::optional<Candle> DataFeed::latest() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (history_.empty()) return std::nullopt;
    return history_.back();
}

} // namespace crypto
