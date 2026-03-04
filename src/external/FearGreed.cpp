#include "FearGreed.h"
#include "../core/Logger.h"
#include <chrono>

namespace crypto {

FearGreed::~FearGreed() {
    stop();
}

void FearGreed::start() {
    if (running_) return;
    running_ = true;
    pollThread_ = std::thread(&FearGreed::loop, this);
}

void FearGreed::stop() {
    running_ = false;
    if (pollThread_.joinable()) pollThread_.join();
}

FearGreedData FearGreed::getData() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return data_;
}

void FearGreed::setData(const FearGreedData& d) {
    std::lock_guard<std::mutex> lk(mutex_);
    data_ = d;
}

void FearGreed::fetch() {
    // Placeholder: In production, GET https://api.alternative.me/fng/?limit=1
    // and parse JSON response
    Logger::get()->debug("[FearGreed] Fetch cycle (placeholder)");
}

void FearGreed::loop() {
    while (running_) {
        fetch();
        // Update every hour
        for (int i = 0; i < 3600 && running_; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

} // namespace crypto
