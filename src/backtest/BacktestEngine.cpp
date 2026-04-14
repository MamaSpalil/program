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
    return emaSignalParameterized(bars, idx, 9, 21);
}

int BacktestEngine::emaSignalParameterized(const std::vector<Candle>& bars, size_t idx,
                                            int fastPeriod, int slowPeriod) {
    if (idx < static_cast<size_t>(slowPeriod) || idx < 1) return 0;
    std::vector<double> closes;
    closes.reserve(idx + 1);
    for (size_t i = 0; i <= idx; ++i) closes.push_back(bars[i].close);
    double fast = calcEMA(closes, fastPeriod, idx);
    double slow = calcEMA(closes, slowPeriod, idx);
    double prevFast = calcEMA(closes, fastPeriod, idx - 1);
    double prevSlow = calcEMA(closes, slowPeriod, idx - 1);
    if (prevFast <= prevSlow && fast > slow) return 1;   // buy
    if (prevFast >= prevSlow && fast < slow) return -1;  // sell
    return 0;
}

// Compute a composite score from backtest result metrics (higher = better).
// Uses profit factor, win rate, PnL, and drawdown to evaluate R:R quality.
static double computeCompositeScore(double profitFactor, double winRate,
                                     double pnlPct, double maxDrawdownPct) {
    double pf = std::max(profitFactor, 0.0);
    double wr = winRate;
    double ddPenalty = maxDrawdownPct / 100.0;  // normalize to 0..1
    // Use log1p(100 + pnlPct) for consistent scaling of both positive and negative returns.
    // This produces ~4.6 at 0% and grows logarithmically for gains, shrinks for losses.
    double pnlScore = std::log1p(100.0 + pnlPct) - std::log1p(100.0);
    return pf * 0.3 + wr * 0.3 + pnlScore * 0.3 - ddPenalty * 0.1;
}

double BacktestEngine::scoreResult(const Result& r) {
    // Penalize strategies with no trades
    if (r.totalTrades == 0) return -1000.0;
    return computeCompositeScore(r.profitFactor, r.winRate,
                                  r.totalPnLPct, r.maxDrawdownPct);
}

bool BacktestEngine::optimizeParameters(Config& cfg, const std::vector<Candle>& bars) {
    if (bars.size() < 50) return false;  // Not enough data to optimize

    // Candidate EMA period pairs: fast in [3..20], slow in [fast+5..60]
    struct Candidate { int fast; int slow; double score; };
    std::vector<Candidate> candidates;

    // Load historical bias from DB if available
    double bestHistScore = -1e9;
    int histFast = cfg.emaPeriodFast;
    int histSlow = cfg.emaPeriodSlow;

    if (btRepo_) {
        auto pastResults = btRepo_->getAll(cfg.symbol, 20);
        for (const auto& pr : pastResults) {
            double s = computeCompositeScore(pr.profitFactor, pr.winRate,
                                              pr.pnlPct, pr.maxDrawdown);
            if (s > bestHistScore) {
                bestHistScore = s;
                // Try to extract periods from strategy name
                auto pos = pr.strategy.find('(');
                if (pos != std::string::npos) {
                    auto end = pr.strategy.find(')', pos);
                    if (end != std::string::npos) {
                        auto params = pr.strategy.substr(pos + 1, end - pos - 1);
                        auto sep = params.find('/');
                        if (sep != std::string::npos) {
                            try {
                                histFast = std::stoi(params.substr(0, sep));
                                histSlow = std::stoi(params.substr(sep + 1));
                            } catch (...) {}
                        }
                    }
                }
            }
        }
    }

    // Generate candidate period pairs around the historical best and also explore.
    // Bounds: fast in [3..30] (3 = min meaningful EMA, 30 = max to avoid lag),
    // slow must exceed fast by >2 for meaningful crossovers, max 80 to prevent overfitting.
    auto addCandidate = [&](int f, int s) {
        if (f >= 3 && f <= 30 && s > f + 2 && s <= 80
            && static_cast<size_t>(s) < bars.size()) {
            candidates.push_back({f, s, 0.0});
        }
    };

    // Explore neighborhood of historical best (fine-grained local search)
    for (int df = -3; df <= 3; ++df) {
        for (int ds = -5; ds <= 5; ++ds) {
            addCandidate(histFast + df, histSlow + ds);
        }
    }
    // Broader grid with larger steps (coarse global search for diversity).
    // Steps of 4/8 balance coverage vs computational cost (~50 candidates total).
    for (int f = 3; f <= 25; f += 4) {
        for (int s = f + 5; s <= 60; s += 8) {
            addCandidate(f, s);
        }
    }

    // Remove duplicates
    std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        return (a.fast < b.fast) || (a.fast == b.fast && a.slow < b.slow);
    });
    candidates.erase(std::unique(candidates.begin(), candidates.end(),
        [](const Candidate& a, const Candidate& b) {
            return a.fast == b.fast && a.slow == b.slow;
        }), candidates.end());

    // Score each candidate by running a lightweight backtest (no DB save)
    BacktestEngine scorer;  // no repository — don't persist scoring runs
    Config scoreCfg = cfg;
    double bestScore = -1e9;
    int bestFast = cfg.emaPeriodFast;
    int bestSlow = cfg.emaPeriodSlow;

    for (auto& c : candidates) {
        auto sigFn = [f = c.fast, s = c.slow](const std::vector<Candle>& b, size_t idx) -> int {
            return emaSignalParameterized(b, idx, f, s);
        };
        auto r = scorer.run(scoreCfg, bars, sigFn);
        c.score = scoreResult(r);
        if (c.score > bestScore) {
            bestScore = c.score;
            bestFast = c.fast;
            bestSlow = c.slow;
        }
    }

    bool improved = (bestFast != cfg.emaPeriodFast || bestSlow != cfg.emaPeriodSlow);
    cfg.emaPeriodFast = bestFast;
    cfg.emaPeriodSlow = bestSlow;
    return improved;
}

BacktestEngine::Result BacktestEngine::runAdaptive(Config& cfg, const std::vector<Candle>& bars) {
    // Step 1: Optimize EMA parameters
    optimizeParameters(cfg, bars);

    // Step 2: Run backtest with optimized parameters
    int fast = cfg.emaPeriodFast;
    int slow = cfg.emaPeriodSlow;
    auto sigFn = [fast, slow](const std::vector<Candle>& b, size_t idx) -> int {
        return emaSignalParameterized(b, idx, fast, slow);
    };
    auto result = run(cfg, bars, sigFn);
    result.strategyName = "EMA_Crossover(" + std::to_string(fast) + "/" + std::to_string(slow) + ")";
    return result;
}

BacktestEngine::Result BacktestEngine::run(const Config& cfg,
                                            const std::vector<Candle>& bars) {
    int fast = cfg.emaPeriodFast;
    int slow = cfg.emaPeriodSlow;
    auto sigFn = [fast, slow](const std::vector<Candle>& b, size_t idx) -> int {
        return emaSignalParameterized(b, idx, fast, slow);
    };
    return run(cfg, bars, sigFn);
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
    double margin = 0;       // margin locked for current position
    int64_t entryTime = 0;

    result.equityCurve.push_back(equity);

    for (size_t i = 0; i < bars.size(); ++i) {
        int signal = signalFn(bars, i);
        double price = bars[i].close;

        if (!inPosition && signal != 0) {
            // Open position — use configurable fraction of balance, apply slippage
            margin = balance * cfg.positionSizePct;
            double entryCommission = margin * cfg.commission;
            double slipAdj = (signal > 0) ? (1.0 + cfg.slippagePct) : (1.0 - cfg.slippagePct);
            double fillPrice = price * slipAdj;
            posQty = (margin - entryCommission) / fillPrice * cfg.leverage;
            entryPrice = fillPrice;
            entryTime = bars[i].openTime;
            positionSide = (signal > 0) ? "BUY" : "SELL";
            inPosition = true;
            balance -= margin;
        } else if (inPosition) {
            // Check for exit signal (opposite direction or last bar)
            bool shouldClose = (i == bars.size() - 1);
            if (signal != 0) {
                if ((positionSide == "BUY" && signal < 0) ||
                    (positionSide == "SELL" && signal > 0))
                    shouldClose = true;
            }
            // Simplified liquidation check for leveraged positions:
            // Triggers when price moves more than 1/leverage from entry.
            // Real exchanges use maintenance margin and liquidation fees;
            // this is a conservative approximation for backtesting.
            if (cfg.leverage > 1.0) {
                double liqDist = 1.0 / cfg.leverage;
                if (positionSide == "BUY" && price <= entryPrice * (1.0 - liqDist))
                    shouldClose = true;
                else if (positionSide == "SELL" && price >= entryPrice * (1.0 + liqDist))
                    shouldClose = true;
            }
            if (shouldClose) {
                double slipAdj = (positionSide == "BUY") ? (1.0 - cfg.slippagePct) : (1.0 + cfg.slippagePct);
                double exitFillPrice = price * slipAdj;
                double exitValue = posQty * exitFillPrice / cfg.leverage;
                double exitCommission = exitValue * cfg.commission;
                double pnl = 0;
                if (positionSide == "BUY")
                    pnl = (exitFillPrice - entryPrice) * posQty / cfg.leverage - exitCommission;
                else
                    pnl = (entryPrice - exitFillPrice) * posQty / cfg.leverage - exitCommission;

                BacktestTrade trade;
                trade.side = positionSide;
                trade.entryPrice = entryPrice;
                trade.exitPrice = exitFillPrice;
                trade.qty = posQty;
                trade.pnl = pnl;
                trade.pnlPct = (margin > 0) ? (pnl / margin) * 100.0 : 0.0; // ROI on margin (leveraged return)
                trade.entryTime = entryTime;
                trade.exitTime = bars[i].openTime;
                result.trades.push_back(trade);

                balance += margin + pnl;
                margin = 0;
                inPosition = false;
            }
        }

        // Update equity
        if (inPosition) {
            double unrealizedPnl = 0;
            if (positionSide == "BUY")
                unrealizedPnl = (price - entryPrice) * posQty / cfg.leverage;
            else
                unrealizedPnl = (entryPrice - price) * posQty / cfg.leverage;
            equity = balance + margin + unrealizedPnl;
        } else {
            equity = balance;
        }
        result.equityCurve.push_back(equity);
    }

    computeMetrics(result, cfg);
    return result;
}

void BacktestEngine::computeMetrics(Result& result, const Config& cfg) {
    double initialBalance = cfg.initialBalance;
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
            double prev = result.equityCurve[i-1];
            if (prev <= 0.0) continue; // skip if equity was zero/negative
            double r = (result.equityCurve[i] - prev) / prev;
            returns.push_back(r);
        }
        if (!returns.empty()) {
            double avgRet = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
            double sq_sum = 0;
            for (double r : returns) sq_sum += (r - avgRet) * (r - avgRet);
            double stdRet = (returns.size() > 1)
                ? std::sqrt(sq_sum / (returns.size() - 1))
                : 0.0;
            result.sharpeRatio = (stdRet > 0) ? (avgRet / stdRet) * std::sqrt(252.0) : 0.0;
        }
    }

    // Persist results to database if repository is set
    if (btRepo_) {
        BacktestResult dbResult;
        dbResult.id = "bt_" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        dbResult.symbol = cfg.symbol;
        dbResult.timeframe = cfg.interval;
        dbResult.initialBalance = cfg.initialBalance;
        dbResult.finalBalance = result.equityCurve.empty() ? cfg.initialBalance : result.equityCurve.back();
        dbResult.pnl = result.totalPnL;
        dbResult.pnlPct = result.totalPnLPct;
        dbResult.maxDrawdown = result.maxDrawdownPct;
        dbResult.sharpe = result.sharpeRatio;
        dbResult.winRate = result.winRate;
        dbResult.totalTrades = result.totalTrades;
        dbResult.winningTrades = result.winTrades;
        dbResult.losingTrades = result.lossTrades;
        dbResult.profitFactor = result.profitFactor;
        dbResult.strategy = result.strategyName.empty()
            ? "EMA_Crossover(" + std::to_string(cfg.emaPeriodFast) + "/" + std::to_string(cfg.emaPeriodSlow) + ")"
            : result.strategyName;
        dbResult.commission = cfg.commission;
        dbResult.dateFrom = (!result.trades.empty()) ? result.trades.front().entryTime : 0;
        dbResult.dateTo = (!result.trades.empty()) ? result.trades.back().exitTime : 0;
        dbResult.runAt = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        std::vector<BtTradeRecord> dbTrades;
        for (const auto& t : result.trades) {
            BtTradeRecord bt;
            bt.backtestId = dbResult.id;
            bt.symbol = cfg.symbol;
            bt.side = t.side;
            bt.entryPrice = t.entryPrice;
            bt.exitPrice = t.exitPrice;
            bt.qty = t.qty;
            bt.pnl = t.pnl;
            bt.commission = (t.entryPrice * t.qty + t.exitPrice * t.qty) * cfg.commission;
            bt.entryTime = t.entryTime;
            bt.exitTime = t.exitTime;
            dbTrades.push_back(std::move(bt));
        }
        btRepo_->save(dbResult, dbTrades);
    }
}

} // namespace crypto
