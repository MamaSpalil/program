#include <gtest/gtest.h>
#include "../src/trading/OrderManagement.h"
#include "../src/trading/PaperTrading.h"
#include "../src/trading/TradeMarker.h"
#include "../src/trading/TradeHistory.h"
#include "../src/backtest/BacktestEngine.h"
#include "../src/alerts/AlertManager.h"
#include "../src/scanner/MarketScanner.h"
#include "../src/data/CSVExporter.h"
#include "../src/indicators/PineVisuals.h"
#include "../src/security/KeyVault.h"

using namespace crypto;

// ── H1: OrderManagement ────────────────────────────────────────────────────

TEST(OrderManagement, ValidateEmptySymbol) {
    UIOrderRequest req;
    req.symbol = "";
    req.quantity = 1.0;
    RiskManager::Config cfg;
    RiskManager rm(cfg, 10000.0);
    auto result = OrderManagement::validate(req, rm, 10000.0);
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.reason, "Symbol is required");
}

TEST(OrderManagement, ValidateZeroQuantity) {
    UIOrderRequest req;
    req.symbol = "BTCUSDT";
    req.quantity = 0.0;
    RiskManager::Config cfg;
    RiskManager rm(cfg, 10000.0);
    auto result = OrderManagement::validate(req, rm, 10000.0);
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.reason, "Quantity must be positive");
}

TEST(OrderManagement, ValidateLimitNoPrice) {
    UIOrderRequest req;
    req.symbol = "BTCUSDT";
    req.quantity = 1.0;
    req.type = "LIMIT";
    req.price = 0.0;
    RiskManager::Config cfg;
    RiskManager rm(cfg, 10000.0);
    auto result = OrderManagement::validate(req, rm, 10000.0);
    EXPECT_FALSE(result.valid);
}

TEST(OrderManagement, ValidateSuccess) {
    UIOrderRequest req;
    req.symbol = "BTCUSDT";
    req.side = "BUY";
    req.type = "MARKET";
    req.quantity = 0.001;
    RiskManager::Config cfg;
    RiskManager rm(cfg, 10000.0);
    auto result = OrderManagement::validate(req, rm, 10000.0);
    EXPECT_TRUE(result.valid);
}

TEST(OrderManagement, EstimateCost) {
    // estimateCost includes 0.1% taker commission
    EXPECT_NEAR(OrderManagement::estimateCost(0.5, 100.0), 50.05, 0.01);
    EXPECT_DOUBLE_EQ(OrderManagement::estimateCost(1.0, 0.0), 0.0);
}

TEST(OrderManagement, CreateCloseOrder) {
    ManagedPosition pos;
    pos.symbol = "BTCUSDT";
    pos.side = "BUY";
    pos.quantity = 0.5;
    auto close = OrderManagement::createCloseOrder(pos);
    EXPECT_EQ(close.side, "SELL");
    EXPECT_EQ(close.type, "MARKET");
    EXPECT_DOUBLE_EQ(close.quantity, 0.5);
    EXPECT_TRUE(close.reduceOnly);
}

TEST(OrderManagement, ToExchangeOrder) {
    UIOrderRequest req;
    req.symbol = "ETHUSDT";
    req.side = "SELL";
    req.type = "LIMIT";
    req.quantity = 2.0;
    req.price = 3000.0;
    auto er = OrderManagement::toExchangeOrder(req);
    EXPECT_EQ(er.symbol, "ETHUSDT");
    EXPECT_EQ(er.side, OrderRequest::Side::SELL);
    EXPECT_EQ(er.type, OrderRequest::Type::LIMIT);
    EXPECT_DOUBLE_EQ(er.qty, 2.0);
    EXPECT_DOUBLE_EQ(er.price, 3000.0);
}

// ── H2: PaperTrading ───────────────────────────────────────────────────────

TEST(PaperTrading, InitialBalance) {
    PaperTrading pt(10000.0);
    auto acc = pt.getAccount();
    EXPECT_DOUBLE_EQ(acc.balance, 10000.0);
    EXPECT_DOUBLE_EQ(acc.equity, 10000.0);
}

TEST(PaperTrading, OpenPosition) {
    PaperTrading pt(10000.0);
    bool ok = pt.openPosition("BTCUSDT", "BUY", 1.0, 100.0);
    EXPECT_TRUE(ok);
    auto acc = pt.getAccount();
    EXPECT_EQ(acc.openPositions.size(), 1u);
    EXPECT_DOUBLE_EQ(acc.balance, 9900.0);
}

TEST(PaperTrading, OpenPositionInsufficientBalance) {
    PaperTrading pt(100.0);
    bool ok = pt.openPosition("BTCUSDT", "BUY", 1.0, 200.0);
    EXPECT_FALSE(ok);
}

TEST(PaperTrading, ClosePosition) {
    PaperTrading pt(10000.0);
    pt.openPosition("BTCUSDT", "BUY", 1.0, 100.0);
    bool ok = pt.closePosition("BTCUSDT", 110.0);
    EXPECT_TRUE(ok);
    auto acc = pt.getAccount();
    EXPECT_EQ(acc.openPositions.size(), 0u);
    EXPECT_EQ(acc.closedTrades.size(), 1u);
    EXPECT_GT(acc.totalPnL, 0.0);
}

TEST(PaperTrading, Reset) {
    PaperTrading pt(10000.0);
    pt.openPosition("BTCUSDT", "BUY", 1.0, 100.0);
    pt.reset(5000.0);
    auto acc = pt.getAccount();
    EXPECT_DOUBLE_EQ(acc.balance, 5000.0);
    EXPECT_EQ(acc.openPositions.size(), 0u);
}

TEST(PaperTrading, UpdatePrices) {
    PaperTrading pt(10000.0);
    pt.openPosition("BTCUSDT", "BUY", 1.0, 100.0);
    pt.updatePrices("BTCUSDT", 120.0);
    auto acc = pt.getAccount();
    EXPECT_EQ(acc.openPositions.size(), 1u);
    EXPECT_DOUBLE_EQ(acc.openPositions[0].currentPrice, 120.0);
    EXPECT_GT(acc.openPositions[0].pnl, 0.0);
}

// ── H3: TradeMarker ────────────────────────────────────────────────────────

TEST(TradeMarker, AddAndGet) {
    TradeMarkerManager mgr;
    TradeMarker m;
    m.time = 1000.0;
    m.price = 50000.0;
    m.side = "BUY";
    m.isEntry = true;
    mgr.addMarker(m);
    auto markers = mgr.getMarkers();
    EXPECT_EQ(markers.size(), 1u);
    EXPECT_EQ(markers[0].side, "BUY");
}

TEST(TradeMarker, Clear) {
    TradeMarkerManager mgr;
    TradeMarker m;
    m.time = 1000.0;
    mgr.addMarker(m);
    mgr.clearMarkers();
    EXPECT_TRUE(mgr.getMarkers().empty());
}

TEST(TradeMarker, TPSL) {
    TradeMarkerManager mgr;
    mgr.setTPSL("BTCUSDT", 55000.0, 48000.0);
    auto lines = mgr.getTPSLLines();
    EXPECT_EQ(lines.size(), 1u);
    EXPECT_DOUBLE_EQ(lines[0].tp, 55000.0);
    EXPECT_DOUBLE_EQ(lines[0].sl, 48000.0);
    mgr.clearTPSL("BTCUSDT");
    EXPECT_TRUE(mgr.getTPSLLines().empty());
}

// ── H4: KeyVault ───────────────────────────────────────────────────────────

TEST(KeyVault, EncryptDecrypt) {
    std::string plaintext = "my_secret_api_key_12345";
    std::string password = "strongpassword";
    auto encrypted = KeyVault::encrypt(plaintext, password);
    EXPECT_GT(encrypted.size(), plaintext.size());
    auto decrypted = KeyVault::decrypt(encrypted, password);
    EXPECT_EQ(decrypted, plaintext);
}

TEST(KeyVault, EncryptDecryptHex) {
    std::string plaintext = "another_secret";
    std::string password = "pass123";
    auto hex = KeyVault::encryptToHex(plaintext, password);
    EXPECT_FALSE(hex.empty());
    auto decrypted = KeyVault::decryptFromHex(hex, password);
    EXPECT_EQ(decrypted, plaintext);
}

TEST(KeyVault, WrongPasswordFails) {
    std::string plaintext = "secret";
    auto encrypted = KeyVault::encrypt(plaintext, "correct_pass");
    EXPECT_THROW(KeyVault::decrypt(encrypted, "wrong_pass"), std::runtime_error);
}

TEST(KeyVault, HexRoundTrip) {
    std::vector<uint8_t> data = {0xDE, 0xAD, 0xBE, 0xEF};
    auto hex = KeyVault::toHex(data);
    EXPECT_EQ(hex, "deadbeef");
    auto back = KeyVault::fromHex(hex);
    EXPECT_EQ(back, data);
}

TEST(KeyVault, GenerateSalt) {
    auto salt1 = KeyVault::generateSalt(16);
    auto salt2 = KeyVault::generateSalt(16);
    EXPECT_EQ(salt1.size(), 16u);
    EXPECT_NE(salt1, salt2);
}

// ── TradeHistory ───────────────────────────────────────────────────────────

TEST(TradeHistory, AddAndStats) {
    TradeHistory th;
    HistoricalTrade t1;
    t1.symbol = "BTCUSDT";
    t1.side = "BUY";
    t1.pnl = 100.0;
    t1.isPaper = false;
    th.addTrade(t1);

    HistoricalTrade t2;
    t2.symbol = "ETHUSDT";
    t2.side = "SELL";
    t2.pnl = -50.0;
    t2.isPaper = true;
    th.addTrade(t2);

    EXPECT_EQ(th.totalTrades(), 2);
    EXPECT_EQ(th.winCount(), 1);
    EXPECT_EQ(th.lossCount(), 1);
    EXPECT_DOUBLE_EQ(th.totalPnl(), 50.0);
    EXPECT_DOUBLE_EQ(th.winRate(), 0.5);
}

TEST(TradeHistory, FilterBySymbol) {
    TradeHistory th;
    HistoricalTrade t1;
    t1.symbol = "BTCUSDT";
    th.addTrade(t1);
    HistoricalTrade t2;
    t2.symbol = "ETHUSDT";
    th.addTrade(t2);

    auto btc = th.getBySymbol("BTCUSDT");
    EXPECT_EQ(btc.size(), 1u);
    auto eth = th.getBySymbol("ETHUSDT");
    EXPECT_EQ(eth.size(), 1u);
}

TEST(TradeHistory, PaperVsLive) {
    TradeHistory th;
    HistoricalTrade t1;
    t1.isPaper = true;
    th.addTrade(t1);
    HistoricalTrade t2;
    t2.isPaper = false;
    th.addTrade(t2);

    EXPECT_EQ(th.getPaperTrades().size(), 1u);
    EXPECT_EQ(th.getLiveTrades().size(), 1u);
}

TEST(TradeHistory, ProfitFactor) {
    TradeHistory th;
    HistoricalTrade t1;
    t1.pnl = 200.0;
    th.addTrade(t1);
    HistoricalTrade t2;
    t2.pnl = -100.0;
    th.addTrade(t2);

    EXPECT_DOUBLE_EQ(th.profitFactor(), 2.0);
}

TEST(TradeHistory, Clear) {
    TradeHistory th;
    HistoricalTrade t;
    th.addTrade(t);
    th.clear();
    EXPECT_EQ(th.totalTrades(), 0);
}

// ── M1: BacktestEngine ─────────────────────────────────────────────────────

TEST(BacktestEngine, EmptyBars) {
    BacktestEngine bt;
    BacktestEngine::Config cfg;
    cfg.initialBalance = 10000.0;
    auto result = bt.run(cfg, {});
    EXPECT_EQ(result.totalTrades, 0);
    EXPECT_DOUBLE_EQ(result.totalPnL, 0.0);
}

TEST(BacktestEngine, RunOnBars) {
    BacktestEngine bt;
    BacktestEngine::Config cfg;
    cfg.initialBalance = 10000.0;
    cfg.commission = 0.001;

    // Generate 100 simple bars with a trending price
    std::vector<Candle> bars;
    for (int i = 0; i < 100; ++i) {
        Candle c;
        c.openTime = i * 60000;
        c.closeTime = (i + 1) * 60000 - 1;
        double price = 100.0 + std::sin(i * 0.3) * 10.0 + i * 0.1;
        c.open = price;
        c.high = price + 1.0;
        c.low = price - 1.0;
        c.close = price + 0.5;
        c.volume = 100.0;
        bars.push_back(c);
    }

    auto result = bt.run(cfg, bars);
    EXPECT_FALSE(result.equityCurve.empty());
    EXPECT_GE(result.equityCurve.size(), 1u);
}

TEST(BacktestEngine, CustomSignal) {
    BacktestEngine bt;
    BacktestEngine::Config cfg;
    cfg.initialBalance = 10000.0;
    cfg.commission = 0.0;

    std::vector<Candle> bars;
    for (int i = 0; i < 10; ++i) {
        Candle c;
        c.openTime = i * 60000;
        c.closeTime = (i + 1) * 60000 - 1;
        c.open = 100.0 + i;
        c.high = 101.0 + i;
        c.low = 99.0 + i;
        c.close = 100.5 + i;
        c.volume = 100.0;
        bars.push_back(c);
    }

    // Buy on bar 2, sell on bar 5
    auto signal = [](const std::vector<Candle>& /*bars*/, size_t idx) -> int {
        if (idx == 2) return 1;   // buy
        if (idx == 5) return -1;  // sell
        return 0;
    };

    auto result = bt.run(cfg, bars, signal);
    // Should have at least 1 trade (entry at bar 2, exit at bar 5)
    EXPECT_GE(result.totalTrades, 1);
}

// ── M3: AlertManager ───────────────────────────────────────────────────────

TEST(AlertManager, AddAndGet) {
    AlertManager am;
    Alert a;
    a.symbol = "BTCUSDT";
    a.condition = "RSI_ABOVE";
    a.threshold = 70.0;
    a.notifyType = "sound";
    am.addAlert(a);
    auto alerts = am.getAlerts();
    EXPECT_EQ(alerts.size(), 1u);
    EXPECT_EQ(alerts[0].symbol, "BTCUSDT");
}

TEST(AlertManager, CheckTrigger) {
    AlertManager am;
    Alert a;
    a.symbol = "BTCUSDT";
    a.condition = "RSI_ABOVE";
    a.threshold = 70.0;
    a.notifyType = "sound";
    am.addAlert(a);

    AlertTick tick;
    tick.symbol = "BTCUSDT";
    tick.rsi = 75.0;
    tick.close = 50000.0;
    am.check(tick);

    auto alerts = am.getAlerts();
    EXPECT_TRUE(alerts[0].triggered);
}

TEST(AlertManager, NoTriggerBelowThreshold) {
    AlertManager am;
    Alert a;
    a.symbol = "BTCUSDT";
    a.condition = "RSI_ABOVE";
    a.threshold = 70.0;
    am.addAlert(a);

    AlertTick tick;
    tick.symbol = "BTCUSDT";
    tick.rsi = 60.0;
    am.check(tick);

    auto alerts = am.getAlerts();
    EXPECT_FALSE(alerts[0].triggered);
}

TEST(AlertManager, RemoveAlert) {
    AlertManager am;
    Alert a;
    a.symbol = "BTCUSDT";
    a.condition = "PRICE_ABOVE";
    a.threshold = 50000.0;
    am.addAlert(a);
    auto alerts = am.getAlerts();
    EXPECT_EQ(alerts.size(), 1u);
    am.removeAlert(alerts[0].id);
    EXPECT_TRUE(am.getAlerts().empty());
}

TEST(AlertManager, ResetTriggered) {
    AlertManager am;
    Alert a;
    a.symbol = "BTCUSDT";
    a.condition = "PRICE_BELOW";
    a.threshold = 40000.0;
    am.addAlert(a);

    AlertTick tick;
    tick.symbol = "BTCUSDT";
    tick.close = 35000.0;
    am.check(tick);

    auto alerts = am.getAlerts();
    EXPECT_TRUE(alerts[0].triggered);
    am.resetAlert(alerts[0].id);
    alerts = am.getAlerts();
    EXPECT_FALSE(alerts[0].triggered);
}

// ── L1: MarketScanner ──────────────────────────────────────────────────────

TEST(MarketScanner, UpdateAndGet) {
    MarketScanner ms;
    std::vector<ScanResult> results;
    ScanResult r1;
    r1.symbol = "BTCUSDT";
    r1.price = 50000.0;
    r1.change24h = 2.5;
    results.push_back(r1);
    ms.updateResults(results);

    auto got = ms.getResults();
    EXPECT_EQ(got.size(), 1u);
    EXPECT_EQ(got[0].symbol, "BTCUSDT");
}

TEST(MarketScanner, FilterByVolume) {
    MarketScanner ms;
    std::vector<ScanResult> results;
    ScanResult r1;
    r1.symbol = "BTC";
    r1.volume24h = 1000.0;
    results.push_back(r1);
    ScanResult r2;
    r2.symbol = "DOGE";
    r2.volume24h = 10.0;
    results.push_back(r2);
    ms.updateResults(results);

    auto filtered = ms.filterByVolume(100.0);
    EXPECT_EQ(filtered.size(), 1u);
    EXPECT_EQ(filtered[0].symbol, "BTC");
}

// ── PineVisuals ────────────────────────────────────────────────────────────

TEST(PineVisuals, AddShapeAndGet) {
    PineVisuals pv;
    PlotShapeMarker m;
    m.time = 1000.0;
    m.price = 50000.0;
    m.shape = "triangleup";
    pv.addPlotShape(m);
    EXPECT_EQ(pv.getPlotShapes().size(), 1u);
}

TEST(PineVisuals, ClearAll) {
    PineVisuals pv;
    PlotShapeMarker m;
    pv.addPlotShape(m);
    BgColorBar bg;
    pv.addBgColor(bg);
    pv.clearAll();
    EXPECT_TRUE(pv.getPlotShapes().empty());
    EXPECT_TRUE(pv.getBgColors().empty());
}
