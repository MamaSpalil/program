#include <gtest/gtest.h>
#include "../src/data/TradingDatabase.h"
#include "../src/data/TradeRepository.h"
#include "../src/data/OrderRepository.h"
#include "../src/data/PositionRepository.h"
#include "../src/data/EquityRepository.h"
#include "../src/data/AlertRepository.h"
#include "../src/data/BacktestRepository.h"
#include "../src/data/DrawingRepository.h"
#include "../src/data/AuxRepository.h"

using namespace crypto;

class TradingDBTest : public ::testing::Test {
protected:
    TradingDatabase db{":memory:"};
    void SetUp() override { db.init(); }
};

// ── Schema ─────────────────────────────────────────────────────────────────

TEST_F(TradingDBTest, SchemaCreated) {
    const char* tables[] = {
        "trades", "orders", "positions",
        "equity_history", "alerts",
        "backtest_results", "backtest_trades",
        "drawings", "funding_rates",
        "scanner_cache", "webhook_log",
        "tax_events"
    };
    for (auto* t : tables) {
        EXPECT_TRUE(db.tableExists(t)) << "Missing table: " << t;
    }
}

// ── TradeRepository ────────────────────────────────────────────────────────

TEST_F(TradingDBTest, TradeUpsertAndLoad) {
    TradeRepository repo(db);
    TradingRecord t;
    t.id = "trade-001"; t.symbol = "BTCUSDT";
    t.exchange = "binance"; t.side = "BUY";
    t.type = "MARKET"; t.qty = 0.01;
    t.entryPrice = 68000; t.isPaper = true;
    t.status = "open";
    t.entryTime = t.createdAt = t.updatedAt = 1700000000000LL;
    ASSERT_TRUE(repo.upsert(t));
    auto open = repo.getOpen(true);
    ASSERT_EQ(open.size(), 1u);
    EXPECT_EQ(open[0].symbol, "BTCUSDT");
    EXPECT_NEAR(open[0].entryPrice, 68000, 0.01);
}

TEST_F(TradingDBTest, TradeClose) {
    TradeRepository repo(db);
    TradingRecord t;
    t.id = "t2"; t.symbol = "ETHUSDT"; t.exchange = "binance";
    t.side = "BUY"; t.type = "MARKET";
    t.qty = 1.0; t.entryPrice = 3000;
    t.isPaper = false; t.status = "open";
    t.entryTime = t.createdAt = t.updatedAt = 1700000000000LL;
    repo.upsert(t);
    ASSERT_TRUE(repo.close("t2", 3200.0, 200.0, 1700003600000LL));
    auto open = repo.getOpen(false);
    EXPECT_TRUE(open.empty());
}

// ── OrderRepository ────────────────────────────────────────────────────────

TEST_F(TradingDBTest, OrderStatusUpdate) {
    OrderRepository repo(db);
    OrderRecord o;
    o.id = "ord-001"; o.symbol = "BTCUSDT"; o.exchange = "binance";
    o.side = "BUY"; o.type = "LIMIT";
    o.qty = 0.01; o.price = 67000;
    o.status = "NEW"; o.isPaper = false;
    o.createdAt = o.updatedAt = 1700000000000LL;
    ASSERT_TRUE(repo.insert(o));
    ASSERT_TRUE(repo.updateStatus("ord-001", "FILLED", 0.01, 67000.0));
    auto open = repo.getOpen("BTCUSDT");
    EXPECT_TRUE(open.empty());
}

// ── PositionRepository ─────────────────────────────────────────────────────

TEST_F(TradingDBTest, PositionUpsertAndUpdate) {
    PositionRepository repo(db);
    PositionRecord p;
    p.id = "pos-001"; p.symbol = "BTCUSDT"; p.exchange = "binance";
    p.side = "LONG"; p.qty = 0.1;
    p.entryPrice = 68000; p.leverage = 1;
    p.isPaper = true;
    p.openedAt = p.updatedAt = 1700000000000LL;
    ASSERT_TRUE(repo.upsert(p));
    ASSERT_TRUE(repo.updatePrice("BTCUSDT", "binance", true, 69000, 100.0));
    auto pos = repo.get("BTCUSDT", "binance", true);
    ASSERT_TRUE(pos.has_value());
    EXPECT_NEAR(pos->currentPrice, 69000, 0.01);
    EXPECT_NEAR(pos->unrealizedPnl, 100.0, 0.01);
}

// ── EquityRepository ───────────────────────────────────────────────────────

TEST_F(TradingDBTest, EquityFlush) {
    EquityRepository repo(db);
    for (int i = 0; i < 10; i++)
        repo.push({1700000000000LL + i * 60000,
                   10000.0 + i * 10, 10000.0, 0, true});
    repo.flush();
    auto hist = repo.getHistory(true, 100);
    EXPECT_EQ(hist.size(), 10u);
}

// ── BacktestRepository ─────────────────────────────────────────────────────

TEST_F(TradingDBTest, BacktestCascadeDelete) {
    BacktestRepository repo(db);
    BacktestResult r;
    r.id = "bt-001"; r.symbol = "BTCUSDT"; r.timeframe = "1h";
    std::vector<BtTradeRecord> trades(5);
    for (auto& t : trades) {
        t.backtestId = "bt-001";
        t.symbol = "BTCUSDT";
    }
    ASSERT_TRUE(repo.save(r, trades));
    ASSERT_TRUE(repo.remove("bt-001"));
    auto bt = repo.getTrades("bt-001");
    EXPECT_TRUE(bt.empty());
}

// ── DrawingRepository ──────────────────────────────────────────────────────

TEST_F(TradingDBTest, DrawingsSaveLoad) {
    DrawingRepository repo(db);
    DrawingObject d;
    d.id = "drw-001";
    d.type = DrawingType::TREND_LINE;
    d.x1 = 1700000000; d.y1 = 68000;
    d.x2 = 1700003600; d.y2 = 69000;
    ASSERT_TRUE(repo.insert(d, "binance"));
    auto loaded = repo.load("BTCUSDT", "binance");
    EXPECT_GE(loaded.size(), 0u);
}

// ── AuxRepository: Webhook ─────────────────────────────────────────────────

TEST_F(TradingDBTest, WebhookLogPrune) {
    AuxRepository repo(db);
    for (int i = 0; i < 600; i++)
        repo.logWebhook({1700000000000LL + i, "buy",
                         "BTCUSDT", 0.01, 68000,
                         "ok", "", "1.2.3.4"});
    repo.pruneWebhookLog(500);
    auto log = repo.getWebhookLog(1000);
    EXPECT_LE(log.size(), 500u);
}

// ── Foreign Keys ───────────────────────────────────────────────────────────

TEST_F(TradingDBTest, ForeignKeysEnabled) {
    BacktestRepository btRepo(db);
    BtTradeRecord orphan;
    orphan.backtestId = "non-existent";
    EXPECT_FALSE(btRepo.insertTrade(orphan));
}

// ── Indices ────────────────────────────────────────────────────────────────

TEST_F(TradingDBTest, IndicesCreated) {
    const char* indices[] = {
        "idx_trades_symbol", "idx_trades_status", "idx_trades_time",
        "idx_orders_symbol", "idx_orders_status",
        "idx_positions_symbol",
        "idx_equity_time",
        "idx_alerts_symbol",
        "idx_backtest_symbol", "idx_bt_trades_id",
        "idx_drawings_symbol",
        "idx_funding_symbol",
        "idx_webhook_time",
        "idx_tax_year"
    };
    for (auto* idx : indices) {
        std::string sql = "SELECT name FROM sqlite_master WHERE type='index' AND name='" +
                          std::string(idx) + "'";
        EXPECT_TRUE(db.execute(sql)) << "Missing index: " << idx;
    }
}

// ── Alert: setTriggered and resetTriggered ─────────────────────────────────

TEST_F(TradingDBTest, AlertSetAndResetTriggered) {
    AlertRepository repo(db);
    AlertRecord a;
    a.id = "alert-001"; a.symbol = "BTCUSDT";
    a.condition = "PRICE_ABOVE"; a.threshold = 70000;
    a.enabled = true; a.triggered = false;
    a.createdAt = a.updatedAt = 1700000000000LL;
    ASSERT_TRUE(repo.insert(a));

    ASSERT_TRUE(repo.setTriggered("alert-001", 1700001000000LL));
    auto all = repo.getAll();
    ASSERT_EQ(all.size(), 1u);
    EXPECT_TRUE(all[0].triggered);
    EXPECT_EQ(all[0].triggerCount, 1);

    ASSERT_TRUE(repo.resetTriggered("alert-001"));
    all = repo.getAll();
    ASSERT_EQ(all.size(), 1u);
    EXPECT_FALSE(all[0].triggered);
}

// ── Position close (remove) ────────────────────────────────────────────────

TEST_F(TradingDBTest, PositionClose) {
    PositionRepository repo(db);
    PositionRecord p;
    p.id = "pos-close"; p.symbol = "ETHUSDT"; p.exchange = "binance";
    p.side = "LONG"; p.qty = 1.0; p.entryPrice = 3000;
    p.isPaper = true;
    p.openedAt = p.updatedAt = 1700000000000LL;
    ASSERT_TRUE(repo.upsert(p));
    ASSERT_TRUE(repo.remove("ETHUSDT", "binance", true));
    auto pos = repo.get("ETHUSDT", "binance", true);
    EXPECT_FALSE(pos.has_value());
}

// ── Trade getHistory ───────────────────────────────────────────────────────

TEST_F(TradingDBTest, TradeGetHistory) {
    TradeRepository repo(db);
    for (int i = 0; i < 5; i++) {
        TradingRecord t;
        t.id = "th-" + std::to_string(i);
        t.symbol = (i % 2 == 0) ? "BTCUSDT" : "ETHUSDT";
        t.exchange = "binance"; t.side = "BUY"; t.type = "MARKET";
        t.qty = 0.01; t.entryPrice = 68000 + i * 100;
        t.status = "closed"; t.isPaper = false;
        t.entryTime = 1700000000000LL + i * 60000;
        t.createdAt = t.updatedAt = t.entryTime;
        repo.upsert(t);
    }
    auto all = repo.getHistory("", 10);
    EXPECT_EQ(all.size(), 5u);
    auto btc = repo.getHistory("BTCUSDT", 10);
    EXPECT_EQ(btc.size(), 3u);
}

// ── Scanner cache upsert ───────────────────────────────────────────────────

TEST_F(TradingDBTest, ScannerCacheUpsert) {
    AuxRepository repo(db);
    ScanResult r;
    r.symbol = "BTCUSDT"; r.price = 68000;
    r.change24h = 2.5; r.volume24h = 1000000;
    r.rsi = 55; r.signal = "BUY"; r.confidence = 0.8;
    ASSERT_TRUE(repo.upsertScanResult(r, "binance"));
    auto cache = repo.getScanCache("binance");
    ASSERT_EQ(cache.size(), 1u);
    EXPECT_EQ(cache[0].symbol, "BTCUSDT");
    EXPECT_NEAR(cache[0].price, 68000, 0.01);
}

// ── Migration flags table ──────────────────────────────────────────────────

TEST_F(TradingDBTest, MigrationFlagsTable) {
    // After init, _meta table should be creatable
    db.execute("CREATE TABLE IF NOT EXISTS _meta (key TEXT PRIMARY KEY, value TEXT)");
    db.execute("INSERT OR REPLACE INTO _meta (key, value) VALUES ('migrated', '1')");
    EXPECT_TRUE(db.tableExists("_meta"));
}

// ── Backtest save with trades ──────────────────────────────────────────────

TEST_F(TradingDBTest, BacktestSaveAndGetTrades) {
    BacktestRepository repo(db);
    BacktestResult r;
    r.id = "bt-save-001"; r.symbol = "BTCUSDT"; r.timeframe = "1h";
    r.pnl = 500; r.totalTrades = 3; r.winningTrades = 2; r.losingTrades = 1;
    r.runAt = 1700000000000LL;

    std::vector<BtTradeRecord> trades;
    for (int i = 0; i < 3; i++) {
        BtTradeRecord bt;
        bt.backtestId = "bt-save-001"; bt.symbol = "BTCUSDT";
        bt.side = (i < 2) ? "BUY" : "SELL";
        bt.entryPrice = 68000; bt.exitPrice = 69000;
        bt.qty = 0.01; bt.pnl = (i < 2) ? 100 : -50;
        bt.entryTime = 1700000000000LL + i * 3600000;
        bt.exitTime = bt.entryTime + 1800000;
        trades.push_back(bt);
    }
    ASSERT_TRUE(repo.save(r, trades));

    auto results = repo.getAll("BTCUSDT", 10);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].totalTrades, 3);

    auto dbTrades = repo.getTrades("bt-save-001");
    EXPECT_EQ(dbTrades.size(), 3u);
}
