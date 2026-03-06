#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <cmath>

namespace crypto {

struct GridConfig {
    std::string symbol;
    double lowerPrice  = 0.0;
    double upperPrice  = 0.0;
    int    gridCount   = 10;
    double totalInvest = 0.0;
    bool   arithmetic  = true; // false = geometric
};

struct GridLevel {
    double      price       = 0.0;
    double      buyPrice    = 0.0;
    double      quantity    = 0.0;
    std::string buyOrderId;
    std::string sellOrderId;
    bool        hasBuy      = false;
    bool        hasSell     = false;
};

class GridBot {
public:
    GridBot() = default;

    void configure(const GridConfig& cfg);
    void start(const GridConfig& cfg);
    void stop();

    void onOrderFilled(const std::string& orderId);

    double realizedProfit() const;
    int    filledOrders()   const;
    bool   isRunning()      const { return running_; }

    int    levelCount() const;
    double levelAt(int idx) const;
    std::vector<GridLevel> levels() const;

private:
    void initLevels();

    GridConfig              config_;
    std::vector<GridLevel>  levels_;
    std::atomic<bool>       running_{false};
    double                  realizedProfit_ = 0.0;
    int                     filledOrders_   = 0;
    mutable std::mutex      mutex_;
};

} // namespace crypto
