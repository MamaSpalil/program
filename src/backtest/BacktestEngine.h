#pragma once
#include "../data/CandleData.h"
#include "../data/BacktestRepository.h"
#include <string>
#include <vector>
#include <functional>

namespace crypto {

struct BacktestTrade {
    std::string side;       // "BUY" / "SELL"
    double      entryPrice{0.0};
    double      exitPrice{0.0};
    double      qty{0.0};
    double      pnl{0.0};
    double      pnlPct{0.0};
    int64_t     entryTime{0};
    int64_t     exitTime{0};
};

class BacktestEngine {
public:
    struct Config {
        std::string symbol;
        std::string interval;
        double      initialBalance{10000.0};
        double      commission{0.001};   // 0.1%
        double      positionSizePct{0.95}; // fraction of balance to use for position (default 95%)
        double      leverage{1.0};       // leverage multiplier (1x = no leverage)
        double      slippagePct{0.0001}; // slippage percentage (0.01% default)
        std::string startDate;           // "2024-01-01" (informational)
        std::string endDate;             // "2024-12-31" (informational)
        int         emaPeriodFast{9};    // fast EMA period for adaptive optimization
        int         emaPeriodSlow{21};   // slow EMA period for adaptive optimization
    };

    struct Result {
        double totalPnL{0.0};
        double totalPnLPct{0.0};
        double maxDrawdown{0.0};
        double maxDrawdownPct{0.0};
        double sharpeRatio{0.0};
        double winRate{0.0};
        int    totalTrades{0};
        int    winTrades{0};
        int    lossTrades{0};
        double avgWin{0.0};
        double avgLoss{0.0};
        double profitFactor{0.0};
        std::vector<BacktestTrade>  trades;
        std::vector<double>         equityCurve;
        std::string strategyName;   // strategy description with parameters
    };

    // Run backtest on given candles using simple EMA crossover strategy
    Result run(const Config& cfg, const std::vector<Candle>& bars);

    // Run backtest with custom signal function
    using SignalFunc = std::function<int(const std::vector<Candle>&, size_t)>;
    Result run(const Config& cfg, const std::vector<Candle>& bars, SignalFunc signalFn);

    // Run adaptive backtest: optimizes EMA parameters using previous DB results
    // and ML-based scoring. Updates cfg.emaPeriodFast/Slow with best found.
    Result runAdaptive(Config& cfg, const std::vector<Candle>& bars);

    // Optimize EMA parameters by scoring candidate period pairs on given bars.
    // Uses previous backtest results from DB to bias scoring.
    // Returns true if optimization found better parameters than defaults.
    bool optimizeParameters(Config& cfg, const std::vector<Candle>& bars);

    void setRepository(BacktestRepository* repo) { btRepo_ = repo; }

private:
    // Parameterized EMA crossover signal using cfg periods
    static int emaSignalParameterized(const std::vector<Candle>& bars, size_t idx,
                                       int fastPeriod, int slowPeriod);
    // Simple EMA crossover signal: +1 buy, -1 sell, 0 hold (uses default 9/21)
    static int emaSignal(const std::vector<Candle>& bars, size_t idx);
    static double calcEMA(const std::vector<double>& prices, int period, size_t idx);
    void computeMetrics(Result& result, const Config& cfg);

    // Score a backtest result for optimization (higher = better)
    static double scoreResult(const Result& r);

    BacktestRepository* btRepo_{nullptr};
};

} // namespace crypto
