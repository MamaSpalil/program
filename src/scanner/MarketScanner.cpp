#include "MarketScanner.h"
#include <algorithm>
#include <cmath>

namespace crypto {

void MarketScanner::updateResults(std::vector<ScanResult> results) {
    std::lock_guard<std::mutex> lk(mutex_);
    results_ = std::move(results);
}

std::vector<ScanResult> MarketScanner::getResults() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return results_;
}

void MarketScanner::sort(SortField field, bool ascending) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto cmp = [&](const ScanResult& a, const ScanResult& b) -> bool {
        double va = 0, vb = 0;
        switch (field) {
            case SortField::Symbol:
                return ascending ? a.symbol < b.symbol : a.symbol > b.symbol;
            case SortField::Price: va = a.price; vb = b.price; break;
            case SortField::Change24h: va = a.change24h; vb = b.change24h; break;
            case SortField::Volume24h: va = a.volume24h; vb = b.volume24h; break;
            case SortField::RSI: va = a.rsi; vb = b.rsi; break;
            case SortField::Signal: va = a.confidence; vb = b.confidence; break;
        }
        return ascending ? va < vb : va > vb;
    };
    std::sort(results_.begin(), results_.end(), cmp);
}

std::vector<ScanResult> MarketScanner::filterByVolume(double minVolume) const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<ScanResult> filtered;
    for (auto& r : results_)
        if (r.volume24h >= minVolume) filtered.push_back(r);
    return filtered;
}

} // namespace crypto
