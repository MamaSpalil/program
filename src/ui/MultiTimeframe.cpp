#include "MultiTimeframe.h"
#include <algorithm>

namespace crypto {

static const char* defaultIntervals[4] = {"1m", "5m", "1h", "1d"};

void MultiTimeframeWindow::loadAll(const std::string& symbol) {
    (void)symbol;
    std::lock_guard<std::mutex> lk(mutex_);
    // In production, each pane would fetch klines from the exchange.
    // For now, mark panes as placeholder-ready if they already have bars.
    for (int i = 0; i < PANE_COUNT; ++i) {
        panes_[i].interval = defaultIntervals[i];
        if (!panes_[i].bars.empty()) {
            panes_[i].ready = true;
        }
    }
}

void MultiTimeframeWindow::loadPane(int paneIdx, const std::string& symbol,
                                     const std::string& interval) {
    (void)symbol;
    if (paneIdx < 0 || paneIdx >= PANE_COUNT) return;
    std::lock_guard<std::mutex> lk(mutex_);
    panes_[paneIdx].interval = interval;
    // Placeholder: would fetch from exchange
}

bool MultiTimeframeWindow::paneReady(int paneIdx) const {
    if (paneIdx < 0 || paneIdx >= PANE_COUNT) return false;
    std::lock_guard<std::mutex> lk(mutex_);
    return panes_[paneIdx].ready;
}

int MultiTimeframeWindow::paneBarCount(int paneIdx) const {
    if (paneIdx < 0 || paneIdx >= PANE_COUNT) return 0;
    std::lock_guard<std::mutex> lk(mutex_);
    return static_cast<int>(panes_[paneIdx].bars.size());
}

std::vector<Candle> MultiTimeframeWindow::paneBars(int paneIdx) const {
    if (paneIdx < 0 || paneIdx >= PANE_COUNT) return {};
    std::lock_guard<std::mutex> lk(mutex_);
    return panes_[paneIdx].bars;
}

std::string MultiTimeframeWindow::paneInterval(int paneIdx) const {
    if (paneIdx < 0 || paneIdx >= PANE_COUNT) return "";
    std::lock_guard<std::mutex> lk(mutex_);
    return panes_[paneIdx].interval;
}

void MultiTimeframeWindow::setPaneBars(int paneIdx,
                                        const std::vector<Candle>& bars,
                                        const std::string& interval) {
    if (paneIdx < 0 || paneIdx >= PANE_COUNT) return;
    std::lock_guard<std::mutex> lk(mutex_);
    panes_[paneIdx].bars = bars;
    panes_[paneIdx].interval = interval;
    panes_[paneIdx].ready = !bars.empty();
}

} // namespace crypto
