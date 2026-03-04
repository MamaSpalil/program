#include "DepthStream.h"
#include "../core/Logger.h"

namespace crypto {

void DepthStreamManager::subscribe(const std::string& symbol,
                                    const std::string& marketType) {
    std::lock_guard<std::mutex> lk(mutex_);
    symbol_ = symbol;
    marketType_ = marketType;
    // In production: open WebSocket stream for depth data
    Logger::get()->info("[DepthStream] Subscribed to {} ({})", symbol, marketType);
}

void DepthStreamManager::unsubscribe() {
    std::lock_guard<std::mutex> lk(mutex_);
    symbol_.clear();
    connected_ = false;
    book_ = {};
    Logger::get()->info("[DepthStream] Unsubscribed");
}

DepthBook DepthStreamManager::getBook() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return book_;
}

void DepthStreamManager::setBook(const DepthBook& book) {
    std::lock_guard<std::mutex> lk(mutex_);
    book_ = book;
}

} // namespace crypto
