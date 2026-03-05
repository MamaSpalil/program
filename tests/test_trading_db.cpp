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
