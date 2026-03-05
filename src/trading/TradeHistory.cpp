#include "TradeHistory.h"
#include <fstream>
#include <algorithm>
#include <cmath>
#include <numeric>

namespace crypto {

void TradeHistory::addTrade(const HistoricalTrade& t) {
    std::lock_guard<std::mutex> lk(mutex_);
    trades_.push_back(t);

    // Parallel write to database if repository is set
    if (repo_) {
        TradingRecord rec;
        rec.id = t.id;
        rec.symbol = t.symbol;
        rec.exchange = "default";  // HistoricalTrade does not track exchange
        rec.side = t.side;
        rec.type = t.type;
        rec.qty = t.qty;
        rec.entryPrice = t.entryPrice;
        rec.exitPrice = t.exitPrice;
        rec.pnl = t.pnl;
        rec.commission = t.commission;
        rec.entryTime = t.entryTime;
        rec.exitTime = t.exitTime;
        rec.isPaper = t.isPaper;
        rec.status = (t.exitTime > 0) ? "closed" : "open";
        rec.createdAt = t.entryTime;
        rec.updatedAt = t.exitTime > 0 ? t.exitTime : t.entryTime;
        repo_->upsert(rec);
    }
}

std::vector<HistoricalTrade> TradeHistory::getAll() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return trades_;
}

std::vector<HistoricalTrade> TradeHistory::getBySymbol(const std::string& s) const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<HistoricalTrade> result;
    for (auto& t : trades_)
        if (t.symbol == s) result.push_back(t);
    return result;
}

std::vector<HistoricalTrade> TradeHistory::getByDateRange(int64_t from, int64_t to) const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<HistoricalTrade> result;
    for (auto& t : trades_)
        if (t.exitTime >= from && t.exitTime <= to) result.push_back(t);
    return result;
}

std::vector<HistoricalTrade> TradeHistory::getPaperTrades() const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<HistoricalTrade> result;
    for (auto& t : trades_)
        if (t.isPaper) result.push_back(t);
    return result;
}

std::vector<HistoricalTrade> TradeHistory::getLiveTrades() const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<HistoricalTrade> result;
    for (auto& t : trades_)
        if (!t.isPaper) result.push_back(t);
    return result;
}

int TradeHistory::totalTrades() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return static_cast<int>(trades_.size());
}

int TradeHistory::winCount() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return static_cast<int>(std::count_if(trades_.begin(), trades_.end(),
        [](const HistoricalTrade& t) { return t.pnl > 0; }));
}

int TradeHistory::lossCount() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return static_cast<int>(std::count_if(trades_.begin(), trades_.end(),
        [](const HistoricalTrade& t) { return t.pnl <= 0; }));
}

double TradeHistory::totalPnl() const {
    std::lock_guard<std::mutex> lk(mutex_);
    double sum = 0;
    for (auto& t : trades_) sum += t.pnl;
    return sum;
}

double TradeHistory::winRate() const {
    int total = totalTrades();
    if (total == 0) return 0.0;
    return static_cast<double>(winCount()) / total;
}

double TradeHistory::avgWin() const {
    std::lock_guard<std::mutex> lk(mutex_);
    double sum = 0;
    int count = 0;
    for (auto& t : trades_) {
        if (t.pnl > 0) { sum += t.pnl; ++count; }
    }
    return count > 0 ? sum / count : 0.0;
}

double TradeHistory::avgLoss() const {
    std::lock_guard<std::mutex> lk(mutex_);
    double sum = 0;
    int count = 0;
    for (auto& t : trades_) {
        if (t.pnl <= 0) { sum += t.pnl; ++count; }
    }
    return count > 0 ? sum / count : 0.0;
}

double TradeHistory::profitFactor() const {
    std::lock_guard<std::mutex> lk(mutex_);
    double totalWins = 0, totalLosses = 0;
    for (auto& t : trades_) {
        if (t.pnl > 0) totalWins += t.pnl;
        else totalLosses += std::abs(t.pnl);
    }
    return totalLosses > 0 ? totalWins / totalLosses : 0.0;
}

double TradeHistory::maxDrawdown() const {
    std::lock_guard<std::mutex> lk(mutex_);
    if (trades_.empty()) return 0.0;
    double peak = 0, equity = 0, maxDD = 0;
    for (auto& t : trades_) {
        equity += t.pnl;
        if (equity > peak) peak = equity;
        double dd = (peak > 0) ? (peak - equity) / peak : 0.0;
        if (dd > maxDD) maxDD = dd;
    }
    return maxDD;
}

bool TradeHistory::saveToDisk(const std::string& path) const {
    std::lock_guard<std::mutex> lk(mutex_);
    try {
        nlohmann::json arr = nlohmann::json::array();
        for (auto& t : trades_) {
            arr.push_back({
                {"id", t.id}, {"symbol", t.symbol}, {"side", t.side},
                {"type", t.type}, {"qty", t.qty},
                {"entryPrice", t.entryPrice}, {"exitPrice", t.exitPrice},
                {"pnl", t.pnl}, {"commission", t.commission},
                {"entryTime", t.entryTime}, {"exitTime", t.exitTime},
                {"isPaper", t.isPaper}
            });
        }
        std::ofstream f(path);
        if (!f.is_open()) return false;
        f << arr.dump(2);
        return true;
    } catch (...) {
        return false;
    }
}

bool TradeHistory::loadFromDisk(const std::string& path) {
    std::lock_guard<std::mutex> lk(mutex_);
    try {
        std::ifstream f(path);
        if (!f.is_open()) return false;
        auto arr = nlohmann::json::parse(f);
        trades_.clear();
        for (auto& j : arr) {
            HistoricalTrade t;
            t.id = j.value("id", "");
            t.symbol = j.value("symbol", "");
            t.side = j.value("side", "");
            t.type = j.value("type", "");
            t.qty = j.value("qty", 0.0);
            t.entryPrice = j.value("entryPrice", 0.0);
            t.exitPrice = j.value("exitPrice", 0.0);
            t.pnl = j.value("pnl", 0.0);
            t.commission = j.value("commission", 0.0);
            t.entryTime = j.value("entryTime", int64_t(0));
            t.exitTime = j.value("exitTime", int64_t(0));
            t.isPaper = j.value("isPaper", false);
            trades_.push_back(t);
        }
        return true;
    } catch (...) {
        return false;
    }
}

void TradeHistory::clear() {
    std::lock_guard<std::mutex> lk(mutex_);
    trades_.clear();
}

} // namespace crypto
