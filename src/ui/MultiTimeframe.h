#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include "../data/CandleData.h"

namespace crypto {

class MultiTimeframeWindow {
public:
    static constexpr int PANE_COUNT = 4;

    MultiTimeframeWindow() = default;

    // Load all panes for a given symbol (4 timeframes: 1m, 5m, 1h, 1d)
    void loadAll(const std::string& symbol);

    // Load a specific pane with a specific interval
    void loadPane(int paneIdx, const std::string& symbol,
                  const std::string& interval);

    bool paneReady(int paneIdx) const;
    int  paneBarCount(int paneIdx) const;
    std::vector<Candle> paneBars(int paneIdx) const;
    std::string paneInterval(int paneIdx) const;

    // For testing: inject bars directly
    void setPaneBars(int paneIdx, const std::vector<Candle>& bars,
                     const std::string& interval);

private:
    struct Pane {
        std::string          interval;
        std::vector<Candle>  bars;
        bool                 ready{false};
    };
    Pane panes_[PANE_COUNT];
    mutable std::mutex mutex_;
};

} // namespace crypto
