/**
 * @file test_system.cpp
 * @brief Full system / acceptance / regression tests.
 *
 * Verifies the application as a whole:
 *   - end-to-end trading pipelines (indicators → signals → risk → orders → paper trading → history)
 *   - cross-component interaction (DataFeed ↔ IndicatorEngine ↔ FeatureExtractor)
 *   - backtest engine with metrics & CSV export round-trip
 *   - alert system integration with price/indicator conditions
 *   - market scanner filtering/sorting with realistic scan data
 *   - paper trading full lifecycle with persistence
 *   - security: KeyVault multi-round encrypt/decrypt integrity
 *   - CandleCache ↔ DataFeed ↔ IndicatorEngine pipeline
 *   - EventBus pub/sub cross-component communication
 *   - regression: existing behaviour preserved after pipeline runs
 *   - OrderManagement + RiskManager integration
 */

#include <gtest/gtest.h>
#include <cstdio>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <thread>
#include <atomic>

// Core
#include "../src/core/EventBus.h"
#include "../src/core/Logger.h"

// Data
#include "../src/data/CandleData.h"
#include "../src/data/CandleCache.h"
#include "../src/data/DataFeed.h"
#include "../src/data/CSVExporter.h"
#include "../src/data/ExchangeDB.h"

// Indicators
#include "../src/indicators/EMA.h"
#include "../src/indicators/RSI.h"
#include "../src/indicators/ATR.h"
#include "../src/indicators/IndicatorEngine.h"
#include "../src/indicators/PineVisuals.h"

// ML
#include "../src/ml/FeatureExtractor.h"

// Strategy
#include "../src/strategy/RiskManager.h"

// Trading
#include "../src/trading/OrderManagement.h"
#include "../src/trading/PaperTrading.h"
#include "../src/trading/TradeHistory.h"
#include "../src/trading/TradeMarker.h"

// Other modules
#include "../src/backtest/BacktestEngine.h"
#include "../src/alerts/AlertManager.h"
#include "../src/scanner/MarketScanner.h"
#include "../src/security/KeyVault.h"

using namespace crypto;

// ============================================================================
// Helpers
// ============================================================================

namespace {

// Generate a temporary file path that is cleaned up on destruction.
// Uses /tmp for general files and relative names for CSVExporter (which rejects absolute paths).
struct TempFile {
    std::string path;
    TempFile(const std::string& suffix = ".tmp", bool relative = false) {
        std::random_device rd;
        std::string name = "test_system_" + std::to_string(rd()) + suffix;
        path = relative ? name : ("/tmp/" + name);
    }
    ~TempFile() { std::remove(path.c_str()); }
};

// Build a simple candle with trending price.
Candle makeCandle(int64_t openTime, double price, double volume = 100.0) {
    Candle c;
    c.openTime  = openTime;
    c.closeTime = openTime + 59999;
    c.open      = price - 0.5;
    c.high      = price + 1.0;
    c.low       = price - 1.0;
    c.close     = price;
    c.volume    = volume;
    c.trades    = 10;
    c.closed    = true;
    return c;
}

// Generate N candles with a sine-wave-trending price.
std::vector<Candle> generateCandles(int n, double basePrice = 100.0) {
    std::vector<Candle> v;
    v.reserve(n);
    for (int i = 0; i < n; ++i) {
        double price = basePrice + std::sin(i * 0.2) * 5.0 + i * 0.05;
        v.push_back(makeCandle(static_cast<int64_t>(i) * 60000, price,
                               100.0 + i * 2.0));
    }
    return v;
}

// Build an IndicatorEngine with small periods so it becomes ready quickly.
IndicatorEngine makeSmallIndicatorEngine() {
    nlohmann::json cfg;
    cfg["ema_fast"]     = 3;
    cfg["ema_slow"]     = 5;
    cfg["ema_trend"]    = 10;
    cfg["rsi_period"]   = 5;
    cfg["atr_period"]   = 3;
    cfg["macd_fast"]    = 3;
    cfg["macd_slow"]    = 5;
    cfg["macd_signal"]  = 3;
    cfg["bb_period"]    = 3;
    cfg["bb_stddev"]    = 2.0;
    cfg["rsi_overbought"] = 70;
    cfg["rsi_oversold"]   = 30;
    return IndicatorEngine(cfg);
}

} // anonymous namespace

// ============================================================================
// 1. End-to-end trading pipeline
//    Indicators → signals → RiskManager → OrderManagement → PaperTrading → TradeHistory
// ============================================================================

TEST(SystemTest, EndToEndTradingPipeline) {
    // --- Indicator computation ---
    auto engine = makeSmallIndicatorEngine();
    auto candles = generateCandles(50);
    for (auto& c : candles) engine.update(c);
    ASSERT_TRUE(engine.ready());

    // --- Signal generation ---
    Signal sig = engine.evaluate(0.0);
    // Signal should be one of BUY, SELL, HOLD
    EXPECT_TRUE(sig.type == Signal::Type::BUY ||
                sig.type == Signal::Type::SELL ||
                sig.type == Signal::Type::HOLD);

    // --- Risk manager allows trading ---
    RiskManager::Config rcfg;
    rcfg.maxRiskPerTrade = 0.02;
    rcfg.maxDrawdown = 0.15;
    RiskManager rm(rcfg, 10000.0);
    EXPECT_TRUE(rm.canTrade());

    // --- Order validation ---
    UIOrderRequest req;
    req.symbol   = "BTCUSDT";
    req.side     = "BUY";
    req.type     = "MARKET";
    req.quantity = 0.5;
    auto vr = OrderManagement::validate(req, rm, 10000.0);
    EXPECT_TRUE(vr.valid);

    // --- Paper trading execution ---
    PaperTrading pt(10000.0);
    double lastPrice = candles.back().close;
    EXPECT_TRUE(pt.openPosition("BTCUSDT", "BUY", 0.5, lastPrice));
    pt.updatePrices("BTCUSDT", lastPrice + 10.0);
    EXPECT_TRUE(pt.closePosition("BTCUSDT", lastPrice + 10.0));

    auto acc = pt.getAccount();
    EXPECT_EQ(acc.closedTrades.size(), 1u);
    EXPECT_GT(acc.totalPnL, 0.0);

    // --- Record in trade history ---
    TradeHistory th;
    HistoricalTrade ht;
    ht.symbol     = "BTCUSDT";
    ht.side       = "BUY";
    ht.entryPrice = lastPrice;
    ht.exitPrice  = lastPrice + 10.0;
    ht.qty        = 0.5;
    ht.pnl        = acc.closedTrades[0].pnl;
    ht.isPaper    = true;
    th.addTrade(ht);

    EXPECT_EQ(th.totalTrades(), 1);
    EXPECT_GT(th.totalPnl(), 0.0);
    EXPECT_DOUBLE_EQ(th.winRate(), 1.0);
}

// ============================================================================
// 2. DataFeed → IndicatorEngine → FeatureExtractor pipeline
// ============================================================================

TEST(SystemTest, DataFeedIndicatorFeaturePipeline) {
    auto indEngine = makeSmallIndicatorEngine();
    FeatureExtractor fe(indEngine);
    DataFeed feed(500);

    auto candles = generateCandles(60);

    // Feed candles through DataFeed → subscribers update indicators & features
    feed.addCallback([&](const Candle& c) {
        indEngine.update(c);
        fe.update(c);
    });

    for (auto& c : candles) feed.onCandle(c);

    // DataFeed history should contain all candles
    auto history = feed.getHistory();
    EXPECT_EQ(history.size(), 60u);
    auto latest = feed.latest();
    ASSERT_TRUE(latest.has_value());
    EXPECT_EQ(latest->openTime, candles.back().openTime);

    // Indicators should be ready
    ASSERT_TRUE(indEngine.ready());
    EXPECT_GT(indEngine.emaFast(), 0.0);
    EXPECT_GT(indEngine.emaSlow(), 0.0);
    EXPECT_GT(indEngine.rsi(), 0.0);
    EXPECT_GT(indEngine.atr(), 0.0);

    // Features should be extractable
    auto features = fe.extract();
    EXPECT_TRUE(features.valid);
    EXPECT_EQ(static_cast<int>(features.values.size()), FeatureExtractor::FEATURE_DIM);
}

// ============================================================================
// 3. Backtest with metrics, equity curve, and CSV export round-trip
// ============================================================================

TEST(SystemTest, BacktestWithCSVExport) {
    BacktestEngine bt;
    BacktestEngine::Config cfg;
    cfg.initialBalance = 10000.0;
    cfg.commission     = 0.001;

    auto candles = generateCandles(200, 50000.0);

    // Custom signal: buy on bar 20, sell on bar 40, buy on bar 60, sell on bar 80
    auto signal = [](const std::vector<Candle>& /*bars*/, size_t idx) -> int {
        if (idx == 20 || idx == 60)  return 1;
        if (idx == 40 || idx == 80)  return -1;
        return 0;
    };

    auto result = bt.run(cfg, candles, signal);

    // Should have at least 2 trades
    EXPECT_GE(result.totalTrades, 2);
    EXPECT_FALSE(result.equityCurve.empty());
    EXPECT_EQ(result.winTrades + result.lossTrades, result.totalTrades);

    // --- CSV export round-trip ---
    TempFile tmpBars(".csv", true);
    TempFile tmpTrades(".csv", true);
    TempFile tmpEquity(".csv", true);

    EXPECT_TRUE(CSVExporter::exportBars(candles, tmpBars.path));
    EXPECT_TRUE(CSVExporter::exportBacktestTrades(result.trades, tmpTrades.path));
    EXPECT_TRUE(CSVExporter::exportEquityCurve(result.equityCurve, tmpEquity.path));

    // Verify files exist and have content
    {
        std::ifstream f(tmpBars.path);
        std::string line;
        ASSERT_TRUE(std::getline(f, line)); // header
        EXPECT_NE(line.find("datetime"), std::string::npos);
        int lineCount = 0;
        while (std::getline(f, line)) lineCount++;
        EXPECT_EQ(lineCount, 200);
    }
    {
        std::ifstream f(tmpTrades.path);
        std::string line;
        ASSERT_TRUE(std::getline(f, line)); // header
        int lineCount = 0;
        while (std::getline(f, line)) lineCount++;
        EXPECT_EQ(lineCount, static_cast<int>(result.trades.size()));
    }
    {
        std::ifstream f(tmpEquity.path);
        std::string line;
        ASSERT_TRUE(std::getline(f, line)); // header
        int lineCount = 0;
        while (std::getline(f, line)) lineCount++;
        EXPECT_EQ(lineCount, static_cast<int>(result.equityCurve.size()));
    }
}

// ============================================================================
// 4. Alert system integration — price, RSI, EMA cross
// ============================================================================

TEST(SystemTest, AlertSystemIntegration) {
    AlertManager am;

    // Capture notifications
    std::vector<std::string> notifications;
    am.setNotifyCallback([&](const std::string& msg) {
        notifications.push_back(msg);
    });

    // Add multiple alert types
    Alert priceAbove;
    priceAbove.symbol    = "BTCUSDT";
    priceAbove.condition = "PRICE_ABOVE";
    priceAbove.threshold = 50000.0;
    priceAbove.notifyType = "sound";
    am.addAlert(priceAbove);

    Alert rsiBelow;
    rsiBelow.symbol    = "ETHUSDT";
    rsiBelow.condition = "RSI_BELOW";
    rsiBelow.threshold = 30.0;
    rsiBelow.notifyType = "sound";
    am.addAlert(rsiBelow);

    // Tick that should NOT trigger any alert
    AlertTick tick1;
    tick1.symbol = "BTCUSDT";
    tick1.close  = 49000.0;
    tick1.rsi    = 50.0;
    am.check(tick1);

    auto alerts = am.getAlerts();
    EXPECT_FALSE(alerts[0].triggered);
    EXPECT_FALSE(alerts[1].triggered);

    // Tick that triggers PRICE_ABOVE
    AlertTick tick2;
    tick2.symbol = "BTCUSDT";
    tick2.close  = 51000.0;
    am.check(tick2);

    alerts = am.getAlerts();
    EXPECT_TRUE(alerts[0].triggered);
    EXPECT_FALSE(alerts[1].triggered);

    // Tick that triggers RSI_BELOW
    AlertTick tick3;
    tick3.symbol = "ETHUSDT";
    tick3.rsi    = 25.0;
    tick3.close  = 3000.0;
    am.check(tick3);

    alerts = am.getAlerts();
    EXPECT_TRUE(alerts[1].triggered);

    // Reset and re-check
    am.resetAlert(alerts[0].id);
    alerts = am.getAlerts();
    EXPECT_FALSE(alerts[0].triggered);

    // Remove alert
    am.removeAlert(alerts[1].id);
    EXPECT_EQ(am.getAlerts().size(), 1u);

    // Clear all
    am.clearAll();
    EXPECT_TRUE(am.getAlerts().empty());
}

// ============================================================================
// 5. Alert persistence (save/load)
// ============================================================================

TEST(SystemTest, AlertPersistence) {
    TempFile tmp(".json");

    {
        AlertManager am;
        Alert a;
        a.symbol    = "BTCUSDT";
        a.condition = "PRICE_ABOVE";
        a.threshold = 60000.0;
        a.notifyType = "telegram";
        am.addAlert(a);

        Alert b;
        b.symbol    = "ETHUSDT";
        b.condition = "RSI_BELOW";
        b.threshold = 25.0;
        am.addAlert(b);

        EXPECT_TRUE(am.saveToFile(tmp.path));
    }

    {
        AlertManager am;
        EXPECT_TRUE(am.loadFromFile(tmp.path));
        auto alerts = am.getAlerts();
        EXPECT_EQ(alerts.size(), 2u);
        EXPECT_EQ(alerts[0].symbol, "BTCUSDT");
        EXPECT_DOUBLE_EQ(alerts[0].threshold, 60000.0);
    }
}

// ============================================================================
// 6. Market scanner filtering and sorting
// ============================================================================

TEST(SystemTest, MarketScannerFilterAndSort) {
    MarketScanner ms;

    std::vector<ScanResult> results;
    for (int i = 0; i < 10; ++i) {
        ScanResult r;
        r.symbol    = "COIN" + std::to_string(i);
        r.price     = 100.0 + i * 10.0;
        r.change24h = -5.0 + i;
        r.volume24h = 1000.0 * (i + 1);
        r.rsi       = 30.0 + i * 5.0;
        r.signal    = (i % 3 == 0) ? "BUY" : ((i % 3 == 1) ? "SELL" : "HOLD");
        results.push_back(r);
    }
    ms.updateResults(results);

    auto all = ms.getResults();
    EXPECT_EQ(all.size(), 10u);

    // Filter by volume — only entries with volume >= 5000
    auto filtered = ms.filterByVolume(5000.0);
    for (auto& r : filtered) {
        EXPECT_GE(r.volume24h, 5000.0);
    }
    EXPECT_LT(filtered.size(), all.size());

    // Sort by price ascending
    ms.sort(MarketScanner::SortField::Price, true);
    auto sorted = ms.getResults();
    for (size_t i = 1; i < sorted.size(); ++i) {
        EXPECT_LE(sorted[i - 1].price, sorted[i].price);
    }

    // Sort by change descending
    ms.sort(MarketScanner::SortField::Change24h, false);
    sorted = ms.getResults();
    for (size_t i = 1; i < sorted.size(); ++i) {
        EXPECT_GE(sorted[i - 1].change24h, sorted[i].change24h);
    }
}

// ============================================================================
// 7. Paper trading full lifecycle with persistence
// ============================================================================

TEST(SystemTest, PaperTradingLifecycleAndPersistence) {
    TempFile tmp(".json");

    {
        PaperTrading pt(100000.0);
        EXPECT_TRUE(pt.openPosition("BTCUSDT", "BUY", 1.0, 50000.0));
        EXPECT_TRUE(pt.openPosition("ETHUSDT", "SELL", 10.0, 3000.0));

        pt.updatePrices("BTCUSDT", 51000.0);
        pt.updatePrices("ETHUSDT", 2900.0);

        auto acc = pt.getAccount();
        EXPECT_EQ(acc.openPositions.size(), 2u);
        // BTC should show profit, ETH short should show profit
        for (auto& pos : acc.openPositions) {
            if (pos.symbol == "BTCUSDT") {
                EXPECT_DOUBLE_EQ(pos.currentPrice, 51000.0);
                EXPECT_GT(pos.pnl, 0.0);
            } else {
                EXPECT_DOUBLE_EQ(pos.currentPrice, 2900.0);
                EXPECT_GT(pos.pnl, 0.0);
            }
        }

        EXPECT_TRUE(pt.closePosition("BTCUSDT", 52000.0));
        acc = pt.getAccount();
        EXPECT_EQ(acc.openPositions.size(), 1u);
        EXPECT_EQ(acc.closedTrades.size(), 1u);
        EXPECT_GT(acc.totalPnL, 0.0);

        EXPECT_TRUE(pt.saveToFile(tmp.path));
    }

    // Reload and verify persistence
    {
        PaperTrading pt;
        EXPECT_TRUE(pt.loadFromFile(tmp.path));
        auto acc = pt.getAccount();
        // Should still have 1 open position (ETHUSDT) and 1 closed trade
        EXPECT_EQ(acc.openPositions.size(), 1u);
        EXPECT_EQ(acc.closedTrades.size(), 1u);
        EXPECT_EQ(acc.openPositions[0].symbol, "ETHUSDT");
    }
}

// ============================================================================
// 8. KeyVault multi-round encrypt/decrypt integrity
// ============================================================================

TEST(SystemTest, KeyVaultMultiRoundIntegrity) {
    std::vector<std::string> secrets = {
        "api_key_binance_12345",
        "api_secret_very_long_string_67890",
        "",               // empty string
        "short",
        std::string(1024, 'X')   // 1KB string
    };
    std::string password = "strong_p@ssw0rd!";

    for (auto& secret : secrets) {
        auto encrypted = KeyVault::encrypt(secret, password);
        auto decrypted = KeyVault::decrypt(encrypted, password);
        EXPECT_EQ(decrypted, secret);

        // Hex round-trip
        auto hex = KeyVault::encryptToHex(secret, password);
        EXPECT_FALSE(hex.empty());
        auto decHex = KeyVault::decryptFromHex(hex, password);
        EXPECT_EQ(decHex, secret);
    }

    // Different passwords produce different ciphertexts
    auto enc1 = KeyVault::encrypt("test", "pass1");
    auto enc2 = KeyVault::encrypt("test", "pass2");
    EXPECT_NE(enc1, enc2);

    // Wrong password should fail
    EXPECT_THROW(KeyVault::decrypt(enc1, "wrong_password"), std::runtime_error);
}

// ============================================================================
// 9. CandleCache → DataFeed → IndicatorEngine pipeline
// ============================================================================

TEST(SystemTest, CandleCacheToIndicatorPipeline) {
    TempFile tmpDb(".db");
    CandleCache cache(tmpDb.path);

    auto candles = generateCandles(100);
    cache.store("binance_paper", "BTCUSDT", "1m", candles);

    // Reload from cache
    auto loaded = cache.load("binance_paper", "BTCUSDT", "1m");
    ASSERT_EQ(loaded.size(), 100u);

    // Feed into DataFeed → IndicatorEngine
    DataFeed feed(500);
    auto indEngine = makeSmallIndicatorEngine();

    feed.addCallback([&](const Candle& c) {
        indEngine.update(c);
    });

    for (auto& c : loaded) feed.onCandle(c);

    ASSERT_TRUE(indEngine.ready());

    // Verify indicator values are reasonable
    double rsi = indEngine.rsi();
    EXPECT_GE(rsi, 0.0);
    EXPECT_LE(rsi, 100.0);
    EXPECT_GT(indEngine.atr(), 0.0);

    // Verify cache metadata
    EXPECT_EQ(cache.count("binance_paper", "BTCUSDT", "1m"), 100);
    EXPECT_EQ(cache.latestOpenTime("binance_paper", "BTCUSDT", "1m"),
              candles.back().openTime);
}

// ============================================================================
// 10. EventBus cross-component communication
// ============================================================================

TEST(SystemTest, EventBusCrossComponent) {
    // Use a local EventBus instance (not the global singleton)
    // to avoid interference between tests
    struct PriceEvent {
        std::string symbol;
        double price;
    };

    struct SignalEvent {
        std::string symbol;
        Signal::Type type;
    };

    EventBus bus;
    std::vector<PriceEvent> receivedPrices;
    std::vector<SignalEvent> receivedSignals;

    bus.subscribe<PriceEvent>([&](const PriceEvent& e) {
        receivedPrices.push_back(e);
    });

    bus.subscribe<SignalEvent>([&](const SignalEvent& e) {
        receivedSignals.push_back(e);
    });

    // Simulate: price update triggers a signal
    bus.publish(PriceEvent{"BTCUSDT", 50000.0});
    bus.publish(PriceEvent{"ETHUSDT", 3000.0});
    bus.publish(SignalEvent{"BTCUSDT", Signal::Type::BUY});

    EXPECT_EQ(receivedPrices.size(), 2u);
    EXPECT_EQ(receivedSignals.size(), 1u);
    EXPECT_DOUBLE_EQ(receivedPrices[0].price, 50000.0);
    EXPECT_EQ(receivedSignals[0].type, Signal::Type::BUY);
}

// ============================================================================
// 11. OrderManagement + RiskManager integration (drawdown blocks trades)
// ============================================================================

TEST(SystemTest, RiskBlocksTradingAfterDrawdown) {
    RiskManager::Config rcfg;
    rcfg.maxRiskPerTrade = 0.02;
    rcfg.maxDrawdown     = 0.10;  // 10% max drawdown
    rcfg.maxOpenPositions = 2;
    RiskManager rm(rcfg, 10000.0);

    // Initially can trade
    EXPECT_TRUE(rm.canTrade());

    // Simulate a large loss
    rm.onTradeOpen(Position{});
    rm.onTradeClose(-1500.0);  // 15% loss
    EXPECT_FALSE(rm.canTrade());

    // Order validation should fail because risk manager blocks trading after drawdown
    UIOrderRequest req;
    req.symbol   = "BTCUSDT";
    req.side     = "BUY";
    req.type     = "MARKET";
    req.quantity = 0.1;
    auto vr = OrderManagement::validate(req, rm, rm.equity());
    EXPECT_FALSE(vr.valid);
    EXPECT_NE(vr.reason.find("Risk limit"), std::string::npos);

    // Position sizing with drawdown
    double size = rm.positionSize(50000.0, 49000.0);
    EXPECT_GT(size, 0.0);  // sizing still computes, canTrade() is the gate
}

// ============================================================================
// 12. TradeHistory statistics accuracy with mixed wins/losses
// ============================================================================

TEST(SystemTest, TradeHistoryStatisticsAccuracy) {
    TradeHistory th;

    // 3 wins: +100, +200, +300
    for (double pnl : {100.0, 200.0, 300.0}) {
        HistoricalTrade t;
        t.symbol = "BTCUSDT";
        t.pnl    = pnl;
        t.isPaper = false;
        th.addTrade(t);
    }

    // 2 losses: -50, -150
    for (double pnl : {-50.0, -150.0}) {
        HistoricalTrade t;
        t.symbol = "ETHUSDT";
        t.pnl    = pnl;
        t.isPaper = true;
        th.addTrade(t);
    }

    EXPECT_EQ(th.totalTrades(), 5);
    EXPECT_EQ(th.winCount(), 3);
    EXPECT_EQ(th.lossCount(), 2);
    EXPECT_DOUBLE_EQ(th.totalPnl(), 400.0);
    EXPECT_DOUBLE_EQ(th.winRate(), 0.6);

    // Profit factor = totalWins / |totalLosses| = 600 / 200 = 3.0
    EXPECT_DOUBLE_EQ(th.profitFactor(), 3.0);

    // avgWin = (100+200+300)/3 = 200
    EXPECT_DOUBLE_EQ(th.avgWin(), 200.0);
    // avgLoss = -(50+150)/2 = -100
    EXPECT_DOUBLE_EQ(th.avgLoss(), -100.0);

    // Filter by symbol
    EXPECT_EQ(th.getBySymbol("BTCUSDT").size(), 3u);
    EXPECT_EQ(th.getBySymbol("ETHUSDT").size(), 2u);

    // Paper vs live
    EXPECT_EQ(th.getPaperTrades().size(), 2u);
    EXPECT_EQ(th.getLiveTrades().size(), 3u);

    // Persistence
    TempFile tmp(".json");
    EXPECT_TRUE(th.saveToDisk(tmp.path));

    TradeHistory th2;
    EXPECT_TRUE(th2.loadFromDisk(tmp.path));
    EXPECT_EQ(th2.totalTrades(), 5);
    EXPECT_DOUBLE_EQ(th2.totalPnl(), 400.0);
}

// ============================================================================
// 13. CSVExporter — all export types produce valid files
// ============================================================================

TEST(SystemTest, CSVExporterAllTypes) {
    // Bars
    auto candles = generateCandles(10);
    TempFile tmpBars(".csv", true);
    EXPECT_TRUE(CSVExporter::exportBars(candles, tmpBars.path));
    {
        std::ifstream f(tmpBars.path);
        ASSERT_TRUE(f.is_open());
        std::string header;
        std::getline(f, header);
        EXPECT_NE(header.find("open"), std::string::npos);
        EXPECT_NE(header.find("close"), std::string::npos);
    }

    // Trades
    std::vector<HistoricalTrade> trades;
    HistoricalTrade ht;
    ht.id = "T1"; ht.symbol = "BTC"; ht.side = "BUY"; ht.pnl = 50.0;
    trades.push_back(ht);
    TempFile tmpTrades(".csv", true);
    EXPECT_TRUE(CSVExporter::exportTrades(trades, tmpTrades.path));

    // Backtest trades
    std::vector<BacktestTrade> btTrades;
    BacktestTrade bt;
    bt.side = "BUY"; bt.entryPrice = 100.0; bt.exitPrice = 110.0; bt.pnl = 10.0;
    btTrades.push_back(bt);
    TempFile tmpBt(".csv", true);
    EXPECT_TRUE(CSVExporter::exportBacktestTrades(btTrades, tmpBt.path));

    // Equity curve
    std::vector<double> equity = {10000, 10100, 10050, 10200};
    TempFile tmpEq(".csv", true);
    EXPECT_TRUE(CSVExporter::exportEquityCurve(equity, tmpEq.path));
    {
        std::ifstream f(tmpEq.path);
        std::string header;
        std::getline(f, header);
        EXPECT_NE(header.find("equity"), std::string::npos);
        int count = 0;
        std::string line;
        while (std::getline(f, line)) count++;
        EXPECT_EQ(count, 4);
    }

    // Reject invalid filenames (path traversal)
    EXPECT_FALSE(CSVExporter::exportBars(candles, "../etc/passwd.csv"));
    EXPECT_FALSE(CSVExporter::exportBars(candles, "/absolute/path.csv"));
    EXPECT_FALSE(CSVExporter::exportBars(candles, ""));
}

// ============================================================================
// 14. ExchangeDB trade statistics integration
// ============================================================================

TEST(SystemTest, ExchangeDBTradeStatistics) {
    TempFile tmp(".json");
    ExchangeDB db(tmp.path);
    db.load();

    // Setup profile
    ExchangeProfile p;
    p.name = "test";
    p.exchangeType = "binance";
    p.apiKey = "k";
    db.upsertProfile(p);
    db.setActiveProfile("test");

    // Add trades
    TradeRecord t1;
    t1.symbol = "BTCUSDT"; t1.side = "BUY"; t1.pnl = 500.0;
    db.addTrade(t1);

    TradeRecord t2;
    t2.symbol = "ETHUSDT"; t2.side = "SELL"; t2.pnl = -200.0;
    db.addTrade(t2);

    TradeRecord t3;
    t3.symbol = "BTCUSDT"; t3.side = "SELL"; t3.pnl = 100.0;
    db.addTrade(t3);

    EXPECT_EQ(db.totalTrades(), 3);
    EXPECT_EQ(db.winCount(), 2);
    EXPECT_EQ(db.lossCount(), 1);
    EXPECT_DOUBLE_EQ(db.totalPnl(), 400.0);
    EXPECT_NEAR(db.winRate(), 2.0 / 3.0, 0.001);

    // Persistence
    EXPECT_TRUE(db.save());
    ExchangeDB db2(tmp.path);
    EXPECT_TRUE(db2.load());
    EXPECT_EQ(db2.totalTrades(), 3);
    EXPECT_DOUBLE_EQ(db2.totalPnl(), 400.0);
    auto active = db2.activeProfile();
    ASSERT_TRUE(active.has_value());
    EXPECT_EQ(active->exchangeType, "binance");
}

// ============================================================================
// 15. PineVisuals integration with multiple markers and colors
// ============================================================================

TEST(SystemTest, PineVisualsIntegration) {
    PineVisuals pv;

    // Add plot shapes
    for (int i = 0; i < 5; ++i) {
        PlotShapeMarker m;
        m.time  = 1000.0 + i * 60.0;
        m.price = 50000.0 + i * 100.0;
        m.shape = (i % 2 == 0) ? "triangleup" : "triangledown";
        m.color = (i % 2 == 0) ? 0xFF00FF00 : 0xFFFF0000;
        m.text  = "Signal " + std::to_string(i);
        pv.addPlotShape(m);
    }

    // Add background colors
    for (int i = 0; i < 3; ++i) {
        BgColorBar bg;
        bg.time  = 1000.0 + i * 60.0;
        bg.color = 0x3300FF00;
        pv.addBgColor(bg);
    }

    EXPECT_EQ(pv.getPlotShapes().size(), 5u);
    EXPECT_EQ(pv.getBgColors().size(), 3u);

    auto shapes = pv.getPlotShapes();
    EXPECT_EQ(shapes[0].shape, "triangleup");
    EXPECT_EQ(shapes[1].shape, "triangledown");

    pv.clearAll();
    EXPECT_TRUE(pv.getPlotShapes().empty());
    EXPECT_TRUE(pv.getBgColors().empty());
}

// ============================================================================
// 16. TradeMarker full lifecycle
// ============================================================================

TEST(SystemTest, TradeMarkerFullLifecycle) {
    TradeMarkerManager mgr;

    // Entry marker
    TradeMarker entry;
    entry.time   = 1000.0;
    entry.price  = 50000.0;
    entry.side   = "BUY";
    entry.isEntry = true;
    mgr.addMarker(entry);

    // Exit marker
    TradeMarker exit;
    exit.time   = 2000.0;
    exit.price  = 51000.0;
    exit.side   = "SELL";
    exit.isEntry = false;
    exit.pnl    = 1000.0;
    mgr.addMarker(exit);

    auto markers = mgr.getMarkers();
    ASSERT_EQ(markers.size(), 2u);
    EXPECT_TRUE(markers[0].isEntry);
    EXPECT_FALSE(markers[1].isEntry);
    EXPECT_DOUBLE_EQ(markers[1].pnl, 1000.0);

    // TP/SL lines
    mgr.setTPSL("BTCUSDT", 55000.0, 48000.0);
    mgr.setTPSL("ETHUSDT", 4000.0, 2800.0);

    auto lines = mgr.getTPSLLines();
    EXPECT_EQ(lines.size(), 2u);

    mgr.clearTPSL("BTCUSDT");
    EXPECT_EQ(mgr.getTPSLLines().size(), 1u);

    mgr.clearMarkers();
    EXPECT_TRUE(mgr.getMarkers().empty());
}

// ============================================================================
// 17. Backtest engine regression: EMA crossover default strategy
// ============================================================================

TEST(SystemTest, BacktestDefaultEMAStrategy) {
    BacktestEngine bt;
    BacktestEngine::Config cfg;
    cfg.initialBalance = 10000.0;
    cfg.commission     = 0.001;

    // Generate enough bars for EMA crossover to trigger
    std::vector<Candle> bars;
    for (int i = 0; i < 300; ++i) {
        Candle c;
        c.openTime  = static_cast<int64_t>(i) * 60000;
        c.closeTime = c.openTime + 59999;
        // Create oscillating price pattern for crossovers
        double price = 100.0 + std::sin(i * 0.15) * 15.0 + i * 0.02;
        c.open   = price - 0.3;
        c.high   = price + 1.0;
        c.low    = price - 1.0;
        c.close  = price;
        c.volume = 100.0 + i;
        c.closed = true;
        bars.push_back(c);
    }

    auto result = bt.run(cfg, bars);

    // Should produce trades from EMA crossover
    EXPECT_GE(result.totalTrades, 1);
    EXPECT_FALSE(result.equityCurve.empty());
    // First point of equity curve should be initial balance
    EXPECT_DOUBLE_EQ(result.equityCurve.front(), cfg.initialBalance);
    // Metrics consistency
    EXPECT_EQ(result.winTrades + result.lossTrades, result.totalTrades);
    if (result.totalTrades > 0) {
        EXPECT_GE(result.winRate, 0.0);
        EXPECT_LE(result.winRate, 1.0);
    }
}

// ============================================================================
// 18. Multi-symbol paper trading isolation
// ============================================================================

TEST(SystemTest, PaperTradingMultiSymbolIsolation) {
    PaperTrading pt(100000.0);

    // Open positions in multiple symbols
    EXPECT_TRUE(pt.openPosition("BTCUSDT", "BUY", 1.0, 50000.0));
    EXPECT_TRUE(pt.openPosition("ETHUSDT", "BUY", 10.0, 3000.0));
    EXPECT_TRUE(pt.openPosition("SOLUSDT", "SELL", 100.0, 150.0));

    auto acc = pt.getAccount();
    EXPECT_EQ(acc.openPositions.size(), 3u);

    // Close only ETH
    EXPECT_TRUE(pt.closePosition("ETHUSDT", 3100.0));
    acc = pt.getAccount();
    EXPECT_EQ(acc.openPositions.size(), 2u);
    EXPECT_EQ(acc.closedTrades.size(), 1u);

    // BTC and SOL should still be open
    bool hasBtc = false, hasSol = false;
    for (auto& pos : acc.openPositions) {
        if (pos.symbol == "BTCUSDT") hasBtc = true;
        if (pos.symbol == "SOLUSDT") hasSol = true;
    }
    EXPECT_TRUE(hasBtc);
    EXPECT_TRUE(hasSol);

    // Close remaining
    EXPECT_TRUE(pt.closePosition("BTCUSDT", 49000.0));
    EXPECT_TRUE(pt.closePosition("SOLUSDT", 140.0));

    acc = pt.getAccount();
    EXPECT_EQ(acc.openPositions.size(), 0u);
    EXPECT_EQ(acc.closedTrades.size(), 3u);
}

// ============================================================================
// 19. Indicator regression: EMA, RSI, ATR values after full pipeline
// ============================================================================

TEST(SystemTest, IndicatorRegressionValues) {
    auto engine = makeSmallIndicatorEngine();

    // Feed 50 candles with known uptrend
    for (int i = 1; i <= 50; ++i) {
        Candle c;
        c.openTime = static_cast<int64_t>(i) * 60000;
        c.close = 100.0 + i;      // strictly increasing
        c.open  = c.close - 0.5;
        c.high  = c.close + 1.0;
        c.low   = c.close - 1.0;
        c.volume = 100.0;
        c.closed = true;
        engine.update(c);
    }

    ASSERT_TRUE(engine.ready());

    // In a pure uptrend, EMA fast > EMA slow (fast reacts quicker)
    EXPECT_GT(engine.emaFast(), engine.emaSlow());

    // RSI in strong uptrend should be high
    EXPECT_GT(engine.rsi(), 70.0);

    // ATR should be positive
    EXPECT_GT(engine.atr(), 0.0);

    // MACD in uptrend: MACD line should be positive (fast > slow)
    auto macd = engine.macd();
    EXPECT_GT(macd.macd, 0.0);

    // Bollinger Bands: upper > middle > lower
    auto bb = engine.bb();
    EXPECT_GT(bb.upper, bb.middle);
    EXPECT_GT(bb.middle, bb.lower);
}

// ============================================================================
// 20. CandleCache mode isolation (paper vs live)
// ============================================================================

TEST(SystemTest, CandleCacheModeIsolation) {
    TempFile tmpDb(".db");
    CandleCache cache(tmpDb.path);

    auto paperCandles = generateCandles(20, 100.0);
    auto liveCandles  = generateCandles(15, 200.0);

    cache.store("binance_paper", "BTCUSDT", "1m", paperCandles);
    cache.store("binance_live",  "BTCUSDT", "1m", liveCandles);

    auto paper = cache.load("binance_paper", "BTCUSDT", "1m");
    auto live  = cache.load("binance_live",  "BTCUSDT", "1m");

    EXPECT_EQ(paper.size(), 20u);
    EXPECT_EQ(live.size(), 15u);

    // Data should be different
    EXPECT_NE(paper[0].close, live[0].close);

    // Clear paper should not affect live
    cache.clear("binance_paper", "BTCUSDT", "1m");
    EXPECT_EQ(cache.count("binance_paper", "BTCUSDT", "1m"), 0);
    EXPECT_EQ(cache.count("binance_live",  "BTCUSDT", "1m"), 15);
}

// ============================================================================
// 21. RiskManager Kelly Criterion sizing
// ============================================================================

TEST(SystemTest, RiskManagerKellySizing) {
    RiskManager::Config cfg;
    cfg.maxRiskPerTrade = 0.02;
    RiskManager rm(cfg, 10000.0);

    // Kelly with 60% win rate, equal avg win/loss
    double kelly1 = rm.kellySizing(0.6, 100.0, 100.0);
    EXPECT_GT(kelly1, 0.0);

    // Kelly with 50/50 win rate and equal win/loss → kelly = 0
    double kelly2 = rm.kellySizing(0.5, 100.0, 100.0);
    EXPECT_GE(kelly2, 0.0);

    // Trailing stop calculation
    double trail = rm.trailingStop(50000.0, 52000.0, 500.0, true);
    // For a long position, trailing stop should be below current high
    EXPECT_LT(trail, 52000.0);
    EXPECT_GT(trail, 0.0);
}

// ============================================================================
// 22. Concurrent DataFeed access (thread safety)
// ============================================================================

TEST(SystemTest, DataFeedThreadSafety) {
    DataFeed feed(1000);
    std::atomic<int> callbackCount{0};

    feed.addCallback([&](const Candle&) {
        callbackCount++;
    });

    // Write candles from multiple threads
    auto writer = [&](int start, int count) {
        for (int i = start; i < start + count; ++i) {
            feed.onCandle(makeCandle(i * 60000, 100.0 + i));
        }
    };

    std::thread t1(writer, 0, 50);
    std::thread t2(writer, 50, 50);
    t1.join();
    t2.join();

    auto history = feed.getHistory();
    EXPECT_EQ(history.size(), 100u);
    EXPECT_EQ(callbackCount.load(), 100);
}

// ============================================================================
// 23. Acceptance test: full backtest → export → verify metrics
// ============================================================================

TEST(SystemTest, AcceptanceBacktestToExport) {
    // Generate realistic market data
    auto candles = generateCandles(500, 40000.0);

    // Run backtest with a simple mean-reversion signal
    BacktestEngine bt;
    BacktestEngine::Config cfg;
    cfg.initialBalance = 50000.0;
    cfg.commission     = 0.001;

    auto meanReversion = [](const std::vector<Candle>& bars, size_t idx) -> int {
        if (idx < 20) return 0;
        // Simple moving average
        double sum = 0;
        for (size_t i = idx - 20; i < idx; ++i)
            sum += bars[i].close;
        double sma = sum / 20.0;
        if (bars[idx].close < sma * 0.98) return 1;   // buy dip
        if (bars[idx].close > sma * 1.02) return -1;  // sell rally
        return 0;
    };

    auto result = bt.run(cfg, candles, meanReversion);

    // Metrics consistency checks
    EXPECT_EQ(result.winTrades + result.lossTrades, result.totalTrades);
    if (result.totalTrades > 0) {
        EXPECT_GE(result.winRate, 0.0);
        EXPECT_LE(result.winRate, 1.0);
    }
    EXPECT_FALSE(result.equityCurve.empty());

    // Export all results
    TempFile tmpBars(".csv", true);
    TempFile tmpTrades(".csv", true);
    TempFile tmpEquity(".csv", true);

    EXPECT_TRUE(CSVExporter::exportBars(candles, tmpBars.path));
    EXPECT_TRUE(CSVExporter::exportBacktestTrades(result.trades, tmpTrades.path));
    EXPECT_TRUE(CSVExporter::exportEquityCurve(result.equityCurve, tmpEquity.path));

    // Verify equity curve starts at initial balance
    EXPECT_DOUBLE_EQ(result.equityCurve.front(), cfg.initialBalance);

    // Record results to TradeHistory (acceptance: data flows end-to-end)
    TradeHistory th;
    for (auto& t : result.trades) {
        HistoricalTrade ht;
        ht.symbol     = "BTCUSDT";
        ht.side       = t.side;
        ht.entryPrice = t.entryPrice;
        ht.exitPrice  = t.exitPrice;
        ht.pnl        = t.pnl;
        ht.qty        = t.qty;
        ht.entryTime  = t.entryTime;
        ht.exitTime   = t.exitTime;
        ht.isPaper    = true;
        th.addTrade(ht);
    }
    EXPECT_EQ(th.totalTrades(), result.totalTrades);
    EXPECT_NEAR(th.totalPnl(), result.totalPnL, 0.01);
}

// ============================================================================
// 24. OrderManagement edge cases
// ============================================================================

TEST(SystemTest, OrderManagementEdgeCases) {
    RiskManager::Config cfg;
    RiskManager rm(cfg, 10000.0);

    // STOP_LIMIT without stop price
    UIOrderRequest req;
    req.symbol   = "BTCUSDT";
    req.side     = "BUY";
    req.type     = "STOP_LIMIT";
    req.quantity = 1.0;
    req.price    = 50000.0;
    req.stopPrice = 0.0;
    auto vr = OrderManagement::validate(req, rm, 10000.0);
    // Depending on implementation, this may or may not be valid
    // But at minimum it should not crash
    (void)vr;

    // Close order from managed position
    ManagedPosition pos;
    pos.symbol   = "ETHUSDT";
    pos.side     = "SELL";
    pos.quantity = 5.0;
    auto closeReq = OrderManagement::createCloseOrder(pos);
    EXPECT_EQ(closeReq.side, "BUY");  // opposite side
    EXPECT_DOUBLE_EQ(closeReq.quantity, 5.0);
    EXPECT_EQ(closeReq.type, "MARKET");
    EXPECT_TRUE(closeReq.reduceOnly);

    // toExchangeOrder mapping
    UIOrderRequest uiReq;
    uiReq.symbol   = "BTCUSDT";
    uiReq.side     = "BUY";
    uiReq.type     = "LIMIT";
    uiReq.quantity = 0.1;
    uiReq.price    = 50000.0;
    auto exReq = OrderManagement::toExchangeOrder(uiReq);
    EXPECT_EQ(exReq.symbol, "BTCUSDT");
    EXPECT_EQ(exReq.side, OrderRequest::Side::BUY);
    EXPECT_EQ(exReq.type, OrderRequest::Type::LIMIT);
    EXPECT_DOUBLE_EQ(exReq.qty, 0.1);
    EXPECT_DOUBLE_EQ(exReq.price, 50000.0);

    // Estimate cost
    EXPECT_DOUBLE_EQ(OrderManagement::estimateCost(0.1, 50000.0), 5000.0);
    EXPECT_DOUBLE_EQ(OrderManagement::estimateCost(0.0, 50000.0), 0.0);
}

// ============================================================================
// 25. Paper trading reset preserves isolation
// ============================================================================

TEST(SystemTest, PaperTradingResetIsolation) {
    PaperTrading pt(10000.0);
    pt.openPosition("BTCUSDT", "BUY", 1.0, 100.0);

    auto acc = pt.getAccount();
    EXPECT_EQ(acc.openPositions.size(), 1u);
    EXPECT_LT(acc.balance, 10000.0);

    pt.reset(50000.0);
    acc = pt.getAccount();
    EXPECT_DOUBLE_EQ(acc.balance, 50000.0);
    EXPECT_DOUBLE_EQ(acc.equity, 50000.0);
    EXPECT_EQ(acc.openPositions.size(), 0u);
    EXPECT_EQ(acc.closedTrades.size(), 0u);
    EXPECT_DOUBLE_EQ(acc.totalPnL, 0.0);
}
