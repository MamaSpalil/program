#include "PaperTrading.h"
#include "../core/Logger.h"
#include <fstream>
#include <chrono>
#include <algorithm>
#include <cmath>

namespace crypto {

PaperTrading::PaperTrading(double initialBalance)
    : initialBalance_(initialBalance)
{
    account_.balance = initialBalance;
    account_.equity = initialBalance;
    account_.peakEquity = initialBalance;
}

bool PaperTrading::openPosition(const std::string& symbol, const std::string& side,
                                 double qty, double price)
{
    std::lock_guard<std::mutex> lk(mutex_);
    if (qty <= 0.0 || price <= 0.0) return false;
    double cost = qty * price;
    double commission = cost * commissionRate_;
    if (cost + commission > account_.balance) return false;

    PaperPosition pos;
    pos.symbol = symbol;
    pos.side = side;
    pos.quantity = qty;
    pos.entryPrice = price;
    pos.currentPrice = price;
    pos.pnl = 0.0;
    account_.openPositions.push_back(pos);
    account_.balance -= (cost + commission);

    // Persist to database
    if (posRepo_) {
        PositionRecord rec;
        rec.id = symbol + "_paper";
        rec.symbol = symbol;
        rec.exchange = "paper";
        rec.side = side;
        rec.qty = qty;
        rec.entryPrice = price;
        rec.currentPrice = price;
        rec.isPaper = true;
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        rec.openedAt = now;
        rec.updatedAt = now;
        posRepo_->upsert(rec);
    }

    recalcEquity();
    return true;
}

bool PaperTrading::closePosition(const std::string& symbol, double currentPrice) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = std::find_if(account_.openPositions.begin(), account_.openPositions.end(),
        [&](const PaperPosition& p) { return p.symbol == symbol; });
    if (it == account_.openPositions.end()) return false;

    double pnl = 0.0;
    if (it->side == "BUY")
        pnl = (currentPrice - it->entryPrice) * it->quantity;
    else
        pnl = (it->entryPrice - currentPrice) * it->quantity;

    double exitValue = it->quantity * currentPrice;
    double commission = exitValue * commissionRate_;
    pnl -= commission; // deduct exit commission

    PaperTrade trade;
    trade.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    trade.symbol = it->symbol;
    trade.side = it->side;
    trade.quantity = it->quantity;
    trade.entryPrice = it->entryPrice;
    trade.exitPrice = currentPrice;
    trade.pnl = pnl;
    account_.closedTrades.push_back(trade);

    account_.balance += (exitValue - commission);
    account_.totalPnL += pnl;
    account_.openPositions.erase(it);

    // Remove from database
    if (posRepo_) {
        posRepo_->remove(symbol, "paper", true);
    }

    recalcEquity();
    return true;
}

void PaperTrading::updatePrices(const std::string& symbol, double price) {
    std::lock_guard<std::mutex> lk(mutex_);
    for (auto& pos : account_.openPositions) {
        if (pos.symbol == symbol) {
            pos.currentPrice = price;
            if (pos.side == "BUY")
                pos.pnl = (price - pos.entryPrice) * pos.quantity;
            else
                pos.pnl = (pos.entryPrice - price) * pos.quantity;
        }
    }
    recalcEquity();
}

PaperAccount PaperTrading::getAccount() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return account_;
}

void PaperTrading::reset(double initialBalance) {
    std::lock_guard<std::mutex> lk(mutex_);
    initialBalance_ = initialBalance;
    account_ = PaperAccount{};
    account_.balance = initialBalance;
    account_.equity = initialBalance;
    account_.peakEquity = initialBalance;
}

void PaperTrading::recalcEquity() {
    double posValue = 0.0;
    for (auto& p : account_.openPositions)
        posValue += p.quantity * p.currentPrice;
    account_.equity = account_.balance + posValue;
    if (account_.equity > account_.peakEquity)
        account_.peakEquity = account_.equity;
    double dd = (account_.peakEquity > 0.0)
        ? (account_.peakEquity - account_.equity) / account_.peakEquity
        : 0.0;
    if (dd > account_.maxDrawdown)
        account_.maxDrawdown = dd;
}

bool PaperTrading::saveToFile(const std::string& path) const {
    std::lock_guard<std::mutex> lk(mutex_);
    try {
        nlohmann::json j;
        j["balance"] = account_.balance;
        j["equity"] = account_.equity;
        j["totalPnL"] = account_.totalPnL;
        j["maxDrawdown"] = account_.maxDrawdown;
        j["peakEquity"] = account_.peakEquity;

        j["openPositions"] = nlohmann::json::array();
        for (auto& p : account_.openPositions) {
            j["openPositions"].push_back({
                {"symbol", p.symbol}, {"side", p.side},
                {"quantity", p.quantity}, {"entryPrice", p.entryPrice},
                {"currentPrice", p.currentPrice}, {"pnl", p.pnl}
            });
        }

        j["closedTrades"] = nlohmann::json::array();
        for (auto& t : account_.closedTrades) {
            j["closedTrades"].push_back({
                {"timestamp", t.timestamp}, {"symbol", t.symbol},
                {"side", t.side}, {"quantity", t.quantity},
                {"entryPrice", t.entryPrice}, {"exitPrice", t.exitPrice},
                {"pnl", t.pnl}
            });
        }

        std::ofstream f(path);
        if (!f.is_open()) return false;
        f << j.dump(2);
        return true;
    } catch (const std::exception& e) {
        Logger::get()->error("PaperTrading::saveToFile failed: {}", e.what());
        return false;
    }
}

bool PaperTrading::loadFromFile(const std::string& path) {
    std::lock_guard<std::mutex> lk(mutex_);
    try {
        std::ifstream f(path);
        if (!f.is_open()) return false;
        auto j = nlohmann::json::parse(f);

        account_.balance = j.value("balance", 10000.0);
        account_.equity = j.value("equity", 10000.0);
        account_.totalPnL = j.value("totalPnL", 0.0);
        account_.maxDrawdown = j.value("maxDrawdown", 0.0);
        account_.peakEquity = j.value("peakEquity", 10000.0);

        account_.openPositions.clear();
        if (j.contains("openPositions")) {
            for (auto& jp : j["openPositions"]) {
                PaperPosition p;
                p.symbol = jp.value("symbol", "");
                p.side = jp.value("side", "BUY");
                p.quantity = jp.value("quantity", 0.0);
                p.entryPrice = jp.value("entryPrice", 0.0);
                p.currentPrice = jp.value("currentPrice", 0.0);
                p.pnl = jp.value("pnl", 0.0);
                account_.openPositions.push_back(p);
            }
        }

        account_.closedTrades.clear();
        if (j.contains("closedTrades")) {
            for (auto& jt : j["closedTrades"]) {
                PaperTrade t;
                t.timestamp = jt.value("timestamp", int64_t(0));
                t.symbol = jt.value("symbol", "");
                t.side = jt.value("side", "BUY");
                t.quantity = jt.value("quantity", 0.0);
                t.entryPrice = jt.value("entryPrice", 0.0);
                t.exitPrice = jt.value("exitPrice", 0.0);
                t.pnl = jt.value("pnl", 0.0);
                account_.closedTrades.push_back(t);
            }
        }
        return true;
    } catch (const std::exception& e) {
        Logger::get()->error("PaperTrading::loadFromFile failed: {}", e.what());
        return false;
    }
}

} // namespace crypto
