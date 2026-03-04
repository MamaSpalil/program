#include "GridBot.h"
#include <algorithm>

namespace crypto {

void GridBot::configure(const GridConfig& cfg) {
    std::lock_guard<std::mutex> lk(mutex_);
    config_ = cfg;
    initLevels();
}

void GridBot::start(const GridConfig& cfg) {
    configure(cfg);
    running_ = true;
}

void GridBot::stop() {
    running_ = false;
}

void GridBot::initLevels() {
    levels_.clear();
    if (config_.gridCount <= 0 || config_.upperPrice <= config_.lowerPrice)
        return;

    if (config_.arithmetic) {
        double step = (config_.upperPrice - config_.lowerPrice)
                    / config_.gridCount;
        double price = config_.lowerPrice;
        for (int i = 0; i <= config_.gridCount; ++i) {
            GridLevel lvl;
            lvl.price = price;
            levels_.push_back(lvl);
            price += step;
        }
    } else {
        // Geometric spacing
        double ratio = std::pow(config_.upperPrice / config_.lowerPrice,
                                1.0 / config_.gridCount);
        double price = config_.lowerPrice;
        for (int i = 0; i <= config_.gridCount; ++i) {
            GridLevel lvl;
            lvl.price = price;
            levels_.push_back(lvl);
            price *= ratio;
        }
    }
}

void GridBot::onOrderFilled(const std::string& orderId) {
    std::lock_guard<std::mutex> lk(mutex_);
    for (auto& lvl : levels_) {
        if (lvl.buyOrderId == orderId) {
            lvl.hasBuy = true;
            ++filledOrders_;
            return;
        }
        if (lvl.sellOrderId == orderId) {
            lvl.hasSell = true;
            ++filledOrders_;
            // Realized profit = sell - buy price (simplified)
            realizedProfit_ += lvl.price * 0.001; // placeholder
            return;
        }
    }
}

double GridBot::realizedProfit() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return realizedProfit_;
}

int GridBot::filledOrders() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return filledOrders_;
}

int GridBot::levelCount() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return static_cast<int>(levels_.size());
}

double GridBot::levelAt(int idx) const {
    std::lock_guard<std::mutex> lk(mutex_);
    if (idx < 0 || idx >= static_cast<int>(levels_.size())) return 0.0;
    return levels_[static_cast<size_t>(idx)].price;
}

std::vector<GridLevel> GridBot::levels() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return levels_;
}

} // namespace crypto
