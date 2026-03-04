#include "TradeMarker.h"
#include <algorithm>

namespace crypto {

void TradeMarkerManager::addMarker(const TradeMarker& marker) {
    std::lock_guard<std::mutex> lk(mutex_);
    markers_.push_back(marker);
}

void TradeMarkerManager::clearMarkers() {
    std::lock_guard<std::mutex> lk(mutex_);
    markers_.clear();
}

std::vector<TradeMarker> TradeMarkerManager::getMarkers() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return markers_;
}

void TradeMarkerManager::setTPSL(const std::string& symbol, double tp, double sl) {
    std::lock_guard<std::mutex> lk(mutex_);
    for (auto& [sym, line] : tpslLines_) {
        if (sym == symbol) {
            line.tp = tp;
            line.sl = sl;
            return;
        }
    }
    tpslLines_.push_back({symbol, {tp, sl}});
}

void TradeMarkerManager::clearTPSL(const std::string& symbol) {
    std::lock_guard<std::mutex> lk(mutex_);
    tpslLines_.erase(
        std::remove_if(tpslLines_.begin(), tpslLines_.end(),
            [&](const auto& p) { return p.first == symbol; }),
        tpslLines_.end());
}

std::vector<TPSLLine> TradeMarkerManager::getTPSLLines() const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<TPSLLine> result;
    result.reserve(tpslLines_.size());
    for (auto& [sym, line] : tpslLines_)
        result.push_back(line);
    return result;
}

} // namespace crypto
