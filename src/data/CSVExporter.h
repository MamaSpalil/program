#pragma once
#include "../data/CandleData.h"
#include "../trading/TradeHistory.h"
#include "../backtest/BacktestEngine.h"
#include <string>
#include <vector>

namespace crypto {

class CSVExporter {
public:
    // Export candle bars to CSV
    static bool exportBars(const std::vector<Candle>& bars,
                           const std::string& filename);

    // Export trade history to CSV
    static bool exportTrades(const std::vector<HistoricalTrade>& trades,
                             const std::string& filename);

    // Export backtest trades to CSV
    static bool exportBacktestTrades(const std::vector<BacktestTrade>& trades,
                                     const std::string& filename);

    // Export equity curve to CSV
    static bool exportEquityCurve(const std::vector<double>& equity,
                                  const std::string& filename);
};

} // namespace crypto
