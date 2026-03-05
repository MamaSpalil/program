#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <functional>

namespace crypto {

class AuxRepository;

struct ScanResult {
    std::string symbol;
    double      price{0.0};
    double      change24h{0.0};
    double      volume24h{0.0};
    double      rsi{0.0};
    std::string signal;       // "BUY" / "SELL" / "HOLD"
    double      confidence{0.0};
};

class MarketScanner {
public:
    // Set scan results (called from background thread)
    void updateResults(std::vector<ScanResult> results);

    // Get current scan results (thread-safe)
    std::vector<ScanResult> getResults() const;

    // Sort results by field
    enum class SortField { Symbol, Price, Change24h, Volume24h, RSI, Signal };
    void sort(SortField field, bool ascending = true);

    // Filter by minimum volume
    std::vector<ScanResult> filterByVolume(double minVolume) const;

    // Optional: set database repository for cache persistence
    void setAuxRepository(AuxRepository* repo, const std::string& exchange = "binance") {
        auxRepo_ = repo; exchange_ = exchange;
    }

private:
    mutable std::mutex mutex_;
    std::vector<ScanResult> results_;
    AuxRepository* auxRepo_{nullptr};
    std::string exchange_;
};

} // namespace crypto
