#include <gtest/gtest.h>
#include "../src/data/CandleCache.h"
#include <cstdio>
#include <string>
#include <random>

using namespace crypto;

namespace {

// Helper: create a temporary DB path and clean up after test
struct TempDB {
    std::string path;
    TempDB() {
        std::random_device rd;
        path = "/tmp/test_candle_cache_" + std::to_string(rd()) + ".db";
    }
    ~TempDB() { std::remove(path.c_str()); }
};

Candle makeCandle(int64_t openTime, double price, double volume = 100.0) {
    Candle c;
    c.openTime  = openTime;
    c.closeTime = openTime + 59999;
    c.open      = price;
    c.high      = price + 1.0;
    c.low       = price - 1.0;
    c.close     = price + 0.5;
    c.volume    = volume;
    c.trades    = 10;
    c.closed    = true;
    return c;
}

} // namespace

// ── Basic store and load ────────────────────────────────────────────────────

TEST(CandleCache, StoreAndLoad) {
    TempDB tmp;
    CandleCache cache(tmp.path);

    std::vector<Candle> candles;
    candles.push_back(makeCandle(1000, 100.0));
    candles.push_back(makeCandle(2000, 101.0));
    candles.push_back(makeCandle(3000, 102.0));

    cache.store("binance", "BTCUSDT", "1m", candles);
    auto loaded = cache.load("binance", "BTCUSDT", "1m");

    ASSERT_EQ(loaded.size(), 3u);
    EXPECT_EQ(loaded[0].openTime, 1000);
    EXPECT_EQ(loaded[1].openTime, 2000);
    EXPECT_EQ(loaded[2].openTime, 3000);
    EXPECT_DOUBLE_EQ(loaded[0].open, 100.0);
    EXPECT_DOUBLE_EQ(loaded[0].high, 101.0);
    EXPECT_DOUBLE_EQ(loaded[0].low, 99.0);
    EXPECT_DOUBLE_EQ(loaded[0].close, 100.5);
    EXPECT_DOUBLE_EQ(loaded[0].volume, 100.0);
    EXPECT_EQ(loaded[0].trades, 10);
    EXPECT_TRUE(loaded[0].closed);
}

// ── Load returns sorted by openTime ─────────────────────────────────────────

TEST(CandleCache, LoadReturnsSorted) {
    TempDB tmp;
    CandleCache cache(tmp.path);

    // Store out of order
    std::vector<Candle> candles;
    candles.push_back(makeCandle(3000, 102.0));
    candles.push_back(makeCandle(1000, 100.0));
    candles.push_back(makeCandle(2000, 101.0));

    cache.store("binance", "BTCUSDT", "1m", candles);
    auto loaded = cache.load("binance", "BTCUSDT", "1m");

    ASSERT_EQ(loaded.size(), 3u);
    EXPECT_EQ(loaded[0].openTime, 1000);
    EXPECT_EQ(loaded[1].openTime, 2000);
    EXPECT_EQ(loaded[2].openTime, 3000);
}

// ── Upsert replaces existing candles ────────────────────────────────────────

TEST(CandleCache, UpsertReplacesExisting) {
    TempDB tmp;
    CandleCache cache(tmp.path);

    std::vector<Candle> v1 = { makeCandle(1000, 100.0) };
    cache.store("binance", "BTCUSDT", "1m", v1);

    // Store again with different price — should replace
    std::vector<Candle> v2 = { makeCandle(1000, 200.0) };
    cache.store("binance", "BTCUSDT", "1m", v2);

    auto loaded = cache.load("binance", "BTCUSDT", "1m");
    ASSERT_EQ(loaded.size(), 1u);
    EXPECT_DOUBLE_EQ(loaded[0].open, 200.0);
}

// ── Isolation between different keys ────────────────────────────────────────

TEST(CandleCache, IsolationByKey) {
    TempDB tmp;
    CandleCache cache(tmp.path);

    cache.store("binance", "BTCUSDT", "1m", { makeCandle(1000, 100.0) });
    cache.store("binance", "ETHUSDT", "1m", { makeCandle(2000, 200.0) });
    cache.store("bybit",   "BTCUSDT", "1m", { makeCandle(3000, 300.0) });
    cache.store("binance", "BTCUSDT", "5m", { makeCandle(4000, 400.0) });

    EXPECT_EQ(cache.load("binance", "BTCUSDT", "1m").size(), 1u);
    EXPECT_EQ(cache.load("binance", "ETHUSDT", "1m").size(), 1u);
    EXPECT_EQ(cache.load("bybit",   "BTCUSDT", "1m").size(), 1u);
    EXPECT_EQ(cache.load("binance", "BTCUSDT", "5m").size(), 1u);
    EXPECT_EQ(cache.load("okx",     "BTCUSDT", "1m").size(), 0u);
}

// ── Isolation between LIVE and Paper modes ──────────────────────────────────

TEST(CandleCache, IsolationByMode) {
    TempDB tmp;
    CandleCache cache(tmp.path);

    // Simulate mode-qualified exchange names as used by Engine
    cache.store("binance_paper", "BTCUSDT", "1m", { makeCandle(1000, 100.0) });
    cache.store("binance_live",  "BTCUSDT", "1m", { makeCandle(2000, 200.0) });

    auto paper = cache.load("binance_paper", "BTCUSDT", "1m");
    auto live  = cache.load("binance_live",  "BTCUSDT", "1m");

    ASSERT_EQ(paper.size(), 1u);
    ASSERT_EQ(live.size(), 1u);
    EXPECT_EQ(paper[0].openTime, 1000);
    EXPECT_DOUBLE_EQ(paper[0].open, 100.0);
    EXPECT_EQ(live[0].openTime, 2000);
    EXPECT_DOUBLE_EQ(live[0].open, 200.0);
}

// ── latestOpenTime ──────────────────────────────────────────────────────────

TEST(CandleCache, LatestOpenTime) {
    TempDB tmp;
    CandleCache cache(tmp.path);

    EXPECT_EQ(cache.latestOpenTime("binance", "BTCUSDT", "1m"), 0);

    cache.store("binance", "BTCUSDT", "1m", {
        makeCandle(1000, 100.0),
        makeCandle(5000, 105.0),
        makeCandle(3000, 103.0)
    });

    EXPECT_EQ(cache.latestOpenTime("binance", "BTCUSDT", "1m"), 5000);
}

// ── count ───────────────────────────────────────────────────────────────────

TEST(CandleCache, Count) {
    TempDB tmp;
    CandleCache cache(tmp.path);

    EXPECT_EQ(cache.count("binance", "BTCUSDT", "1m"), 0);

    cache.store("binance", "BTCUSDT", "1m", {
        makeCandle(1000, 100.0),
        makeCandle(2000, 101.0),
        makeCandle(3000, 102.0)
    });

    EXPECT_EQ(cache.count("binance", "BTCUSDT", "1m"), 3);
}

// ── clear removes specific key data ────────────────────────────────────────

TEST(CandleCache, ClearSpecificKey) {
    TempDB tmp;
    CandleCache cache(tmp.path);

    cache.store("binance", "BTCUSDT", "1m", { makeCandle(1000, 100.0) });
    cache.store("binance", "ETHUSDT", "1m", { makeCandle(2000, 200.0) });

    cache.clear("binance", "BTCUSDT", "1m");

    EXPECT_EQ(cache.count("binance", "BTCUSDT", "1m"), 0);
    EXPECT_EQ(cache.count("binance", "ETHUSDT", "1m"), 1);
}

// ── clearAll removes everything ─────────────────────────────────────────────

TEST(CandleCache, ClearAll) {
    TempDB tmp;
    CandleCache cache(tmp.path);

    cache.store("binance", "BTCUSDT", "1m", { makeCandle(1000, 100.0) });
    cache.store("bybit",   "ETHUSDT", "5m", { makeCandle(2000, 200.0) });

    cache.clearAll();

    EXPECT_EQ(cache.count("binance", "BTCUSDT", "1m"), 0);
    EXPECT_EQ(cache.count("bybit",   "ETHUSDT", "5m"), 0);
}

// ── Empty store is a no-op ──────────────────────────────────────────────────

TEST(CandleCache, EmptyStoreIsNoop) {
    TempDB tmp;
    CandleCache cache(tmp.path);

    cache.store("binance", "BTCUSDT", "1m", {});
    EXPECT_EQ(cache.count("binance", "BTCUSDT", "1m"), 0);
}

// ── Persistence across instances ────────────────────────────────────────────

TEST(CandleCache, PersistenceAcrossInstances) {
    TempDB tmp;

    {
        CandleCache cache(tmp.path);
        cache.store("binance", "BTCUSDT", "1m", {
            makeCandle(1000, 100.0),
            makeCandle(2000, 101.0)
        });
    }

    // Reopen same DB file
    {
        CandleCache cache(tmp.path);
        auto loaded = cache.load("binance", "BTCUSDT", "1m");
        ASSERT_EQ(loaded.size(), 2u);
        EXPECT_EQ(loaded[0].openTime, 1000);
        EXPECT_EQ(loaded[1].openTime, 2000);
    }
}
