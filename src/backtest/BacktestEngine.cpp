#include "BacktestEngine.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <chrono>

namespace crypto {

double BacktestEngine::calcEMA(const std::vector<double>& prices, int period, size_t idx) {
    if (idx < static_cast<size_t>(period - 1)) return prices[idx];
    double k = 2.0 / (period + 1);
    double ema = 0;
    for (size_t i = 0; i <= idx && i < static_cast<size_t>(period); ++i)
        ema += prices[idx - (period - 1) + i];
    ema /= period;
    for (size_t i = static_cast<size_t>(period); i <= idx; ++i)
        ema = prices[i] * k + ema * (1.0 - k);
    return ema;
}

int BacktestEngine::emaSignal(const std::vector<Candle>& bars, size_t idx) {
    if (idx < 21) return 0;
    std::vector<double> closes;
    closes.reserve(idx + 1);
    for (size_t i = 0; i <= idx; ++i) closes.push_back(bars[i].close);
    double fast = calcEMA(closes, 9, idx);
    double slow = calcEMA(closes, 21, idx);
    double prevFast = calcEMA(closes, 9, idx - 1);
    double prevSlow = calcEMA(closes, 21, idx - 1);
    if (prevFast <= prevSlow && fast > slow) return 1;   // buy
    if (prevFast >= prevSlow && fast < slow) return -1;  // sell
    return 0;
}

BacktestEngine::Result BacktestEngine::run(const Config& cfg,
                                            const std::vector<Candle>& bars) {
    return run(cfg, bars, emaSignal);
}

BacktestEngine::Result BacktestEngine::run(const Config& cfg,
                                            const std::vector<Candle>& bars,
                                            SignalFunc signalFn) {
    Result result;
    if (bars.empty()) return result;

    double balance = cfg.initialBalance;
    double equity = balance;
    bool inPosition = false;
    std::string positionSide;
    double entryPrice = 0;
    double posQty = 0;
    int64_t entryTime = 0;

    result.equityCurve.push_back(equity);

    for (size_t i = 0; i < bars.size(); ++i) {
        int signal = signalFn(bars, i);
        double price = bars[i].close;

        if (!inPosition && signal != 0) {
            // Open position — use 95% of balance
            double cost = balance * 0.95;
            double commission = cost * cfg.commission;
            posQty = (cost - commission) / price;
            entryPrice = price;
            entryTime = bars[i].openTime;
            positionSide = (signal > 0) ? "BUY" : "SELL";
            inPosition = true;
            balance -= cost;
        } else if (inPosition) {
            // Check for exit signal (opposite direction or last bar)
            bool shouldClose = (i == bars.size() - 1);
            if (signal != 0) {
                if ((positionSide == "BUY" && signal < 0) ||
                    (positionSide == "SELL" && signal > 0))
                    shouldClose = true;
            }
            if (shouldClose) {
                double exitValue = posQty * price;
                double commission = exitValue * cfg.commission;
                double pnl = 0;
                if (positionSide == "BUY")
                    pnl = (price - entryPrice) * posQty - commission;
                else
                    pnl = (entryPrice - price) * posQty - commission;

                BacktestTrade trade;
                trade.side = positionSide;
                trade.entryPrice = entryPrice;
                trade.exitPrice = price;
                trade.qty = posQty;
                trade.pnl = pnl;
                trade.pnlPct = (entryPrice > 0) ? (pnl / (entryPrice * posQty)) * 100.0 : 0.0;
                trade.entryTime = entryTime;
                trade.exitTime = bars[i].openTime;
                result.trades.push_back(trade);

                balance += exitValue - commission;
                inPosition = false;
            }
        }

        // Update equity
        if (inPosition) {
            if (positionSide == "BUY")
                equity = balance + price * posQty;
            else
                equity = balance + posQty * (2.0 * entryPrice - price);
        } else {
            equity = balance;
        }
        result.equityCurve.push_back(equity);
    }

    computeMetrics(result, cfg.initialBalance);
    return result;
}

void BacktestEngine::computeMetrics(Result& result, double initialBalance) {
    // Total PnL
    result.totalPnL = 0;
    double totalWins = 0, totalLosses = 0;
    for (auto& t : result.trades) {
        result.totalPnL += t.pnl;
        if (t.pnl > 0) {
            result.winTrades++;
            totalWins += t.pnl;
        } else {
            result.lossTrades++;
            totalLosses += std::abs(t.pnl);
        }
    }

    result.totalTrades = static_cast<int>(result.trades.size());
    result.totalPnLPct = (initialBalance > 0) ? (result.totalPnL / initialBalance) * 100.0 : 0.0;
    result.winRate = (result.totalTrades > 0) ?
        static_cast<double>(result.winTrades) / result.totalTrades : 0.0;
    result.avgWin = (result.winTrades > 0) ? totalWins / result.winTrades : 0.0;
    result.avgLoss = (result.lossTrades > 0) ? totalLosses / result.lossTrades : 0.0;
    result.profitFactor = (totalLosses > 0) ? totalWins / totalLosses : 0.0;

    // Max Drawdown
    double peak = initialBalance;
    result.maxDrawdown = 0;
    for (double e : result.equityCurve) {
        if (e > peak) peak = e;
        double dd = peak - e;
        if (dd > result.maxDrawdown) result.maxDrawdown = dd;
    }
    result.maxDrawdownPct = (peak > 0) ? (result.maxDrawdown / peak) * 100.0 : 0.0;

    // Sharpe Ratio
    if (result.equityCurve.size() > 1) {
        std::vector<double> returns;
        for (size_t i = 1; i < result.equityCurve.size(); ++i) {
            double r = (result.equityCurve[i] - result.equityCurve[i-1]) / result.equityCurve[i-1];
            returns.push_back(r);
        }
        double avgRet = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
        double sq_sum = 0;
        for (double r : returns) sq_sum += (r - avgRet) * (r - avgRet);
        double stdRet = (returns.size() > 1)
            ? std::sqrt(sq_sum / (returns.size() - 1))
            : 0.0;
        result.sharpeRatio = (stdRet > 0) ? (avgRet / stdRet) * std::sqrt(252.0) : 0.0;
    }

    // Persist results to database if repository is set
    if (btRepo_) {
        BacktestResult dbResult;
        dbResult.id = "bt_" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        dbResult.pnl = result.totalPnL;
        dbResult.pnlPct = result.totalPnLPct;
        dbResult.maxDrawdown = result.maxDrawdownPct;
        dbResult.sharpe = result.sharpeRatio;
        dbResult.winRate = result.winRate;
        dbResult.totalTrades = result.totalTrades;
        dbResult.winningTrades = result.winTrades;
        dbResult.losingTrades = result.lossTrades;
        dbResult.profitFactor = result.profitFactor;
        dbResult.runAt = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        std::vector<BtTradeRecord> dbTrades;
        for (const auto& t : result.trades) {
            BtTradeRecord bt;
            bt.backtestId = dbResult.id;
            bt.side = t.side;
            bt.entryPrice = t.entryPrice;
            bt.exitPrice = t.exitPrice;
            bt.qty = t.qty;
            bt.pnl = t.pnl;
            bt.entryTime = t.entryTime;
            bt.exitTime = t.exitTime;
            dbTrades.push_back(std::move(bt));
        }
        btRepo_->save(dbResult, dbTrades);
    }
}

} // namespace crypto
