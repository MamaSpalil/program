#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <atomic>

namespace crypto {

struct DepthLevel {
    double price{0.0};
    double qty{0.0};
};

struct DepthBook {
    std::vector<DepthLevel> bids;
    std::vector<DepthLevel> asks;
};

class DepthStreamManager {
public:
    DepthStreamManager() = default;
    ~DepthStreamManager() = default;

    void subscribe(const std::string& symbol, const std::string& marketType = "spot");
    void unsubscribe();
    bool isConnected() const { return connected_; }

    DepthBook getBook() const;

    // For testing: inject book data
    void setBook(const DepthBook& book);
    void setConnected(bool c) { connected_ = c; }

private:
    std::string symbol_;
    std::string marketType_;
    DepthBook   book_;
    mutable std::mutex mutex_;
    std::atomic<bool> connected_{false};
};

} // namespace crypto
