#include <gtest/gtest.h>
#include "../src/exchange/BinanceExchange.h"
#include "../src/exchange/BybitExchange.h"
#include "../src/exchange/OKXExchange.h"
#include "../src/exchange/BitgetExchange.h"
#include "../src/exchange/KuCoinExchange.h"
#include "../src/data/ExchangeDB.h"
#include "../src/ui/AppGui.h"

using namespace crypto;

// Test that all exchange classes can be constructed
TEST(Exchanges, BinanceConstruction) {
    BinanceExchange ex("key", "secret");
    // Should not throw on construction
    SUCCEED();
}

TEST(Exchanges, BybitConstruction) {
    BybitExchange ex("key", "secret");
    SUCCEED();
}

TEST(Exchanges, OKXConstruction) {
    OKXExchange ex("key", "secret", "passphrase");
    SUCCEED();
}

TEST(Exchanges, BitgetConstruction) {
    BitgetExchange ex("key", "secret", "passphrase");
    SUCCEED();
}

TEST(Exchanges, KuCoinConstruction) {
    KuCoinExchange ex("key", "secret", "passphrase");
    SUCCEED();
}

// testConnection with invalid credentials should not crash (returns false)
TEST(Exchanges, BinanceTestConnectionInvalid) {
    BinanceExchange ex("invalid", "invalid");
    std::string error;
    bool ok = ex.testConnection(error);
    // Without valid API key or network, this should fail gracefully
    EXPECT_FALSE(ok);
    EXPECT_FALSE(error.empty());
}

TEST(Exchanges, BybitTestConnectionInvalid) {
    BybitExchange ex("invalid", "invalid");
    std::string error;
    bool ok = ex.testConnection(error);
    EXPECT_FALSE(ok);
    EXPECT_FALSE(error.empty());
}

// OKX market data endpoints are public — testConnection may succeed even
// with invalid credentials.  Verify it does not crash and handles both outcomes.
TEST(Exchanges, OKXTestConnectionInvalid) {
    OKXExchange ex("invalid", "invalid", "invalid");
    std::string error;
    bool ok = ex.testConnection(error);
    // Public endpoint may succeed; just ensure no crash
    if (!ok) {
        EXPECT_FALSE(error.empty());
    } else {
        EXPECT_TRUE(error.empty());
    }
}

// Bitget market data endpoints are public — testConnection may succeed even
// with invalid credentials.  Verify it does not crash and handles both outcomes.
TEST(Exchanges, BitgetTestConnectionInvalid) {
    BitgetExchange ex("invalid", "invalid", "invalid");
    std::string error;
    bool ok = ex.testConnection(error);
    // Public endpoint may succeed; just ensure no crash
    if (!ok) {
        EXPECT_FALSE(error.empty());
    } else {
        EXPECT_TRUE(error.empty());
    }
}

// KuCoin market data endpoints are public — testConnection may succeed even
// with invalid credentials.  Verify it does not crash and handles both outcomes.
TEST(Exchanges, KuCoinTestConnectionInvalid) {
    KuCoinExchange ex("invalid", "invalid", "invalid");
    std::string error;
    bool ok = ex.testConnection(error);
    // Public endpoint may succeed; just ensure no crash
    if (!ok) {
        EXPECT_FALSE(error.empty());
    } else {
        EXPECT_TRUE(error.empty());
    }
}

// ExchangeDB tests
TEST(ExchangeDB, SaveLoadRoundTrip) {
    std::string dbPath = "/tmp/test_exchange_db.json";
    // Clean up from previous runs
    std::remove(dbPath.c_str());

    {
        ExchangeDB db(dbPath);
        db.load();

        ExchangeProfile p;
        p.name = "test-binance";
        p.exchangeType = "binance";
        p.apiKey = "key123";
        p.apiSecret = "secret456";
        p.passphrase = "";
        p.baseUrl = "https://api.binance.com";
        p.testnet = false;
        db.upsertProfile(p);
        db.setActiveProfile("test-binance");
        EXPECT_TRUE(db.save());
    }

    {
        ExchangeDB db(dbPath);
        EXPECT_TRUE(db.load());
        auto profiles = db.listProfiles();
        EXPECT_EQ(profiles.size(), 1u);
        EXPECT_EQ(profiles[0].name, "test-binance");
        EXPECT_EQ(profiles[0].apiKey, "key123");
        EXPECT_EQ(db.activeProfileName(), "test-binance");

        auto active = db.activeProfile();
        ASSERT_TRUE(active.has_value());
        EXPECT_EQ(active->exchangeType, "binance");
    }

    std::remove(dbPath.c_str());
}

TEST(ExchangeDB, UpsertUpdatesExisting) {
    std::string dbPath = "/tmp/test_exchange_db2.json";
    std::remove(dbPath.c_str());

    ExchangeDB db(dbPath);
    db.load();

    ExchangeProfile p;
    p.name = "my-okx";
    p.exchangeType = "okx";
    p.apiKey = "old_key";
    p.apiSecret = "old_secret";
    p.passphrase = "pass";
    db.upsertProfile(p);

    p.apiKey = "new_key";
    db.upsertProfile(p);

    auto profiles = db.listProfiles();
    EXPECT_EQ(profiles.size(), 1u);
    EXPECT_EQ(profiles[0].apiKey, "new_key");

    std::remove(dbPath.c_str());
}

TEST(ExchangeDB, RemoveProfile) {
    std::string dbPath = "/tmp/test_exchange_db3.json";
    std::remove(dbPath.c_str());

    ExchangeDB db(dbPath);
    db.load();

    ExchangeProfile p;
    p.name = "to-remove";
    p.exchangeType = "bybit";
    db.upsertProfile(p);
    db.setActiveProfile("to-remove");

    EXPECT_TRUE(db.removeProfile("to-remove"));
    EXPECT_TRUE(db.listProfiles().empty());
    EXPECT_EQ(db.activeProfileName(), "");
    EXPECT_FALSE(db.removeProfile("nonexistent"));

    std::remove(dbPath.c_str());
}

TEST(ExchangeDB, MultipleProfiles) {
    std::string dbPath = "/tmp/test_exchange_db4.json";
    std::remove(dbPath.c_str());

    ExchangeDB db(dbPath);
    db.load();

    ExchangeProfile p1;
    p1.name = "binance-main";
    p1.exchangeType = "binance";
    p1.apiKey = "bkey";
    db.upsertProfile(p1);

    ExchangeProfile p2;
    p2.name = "okx-test";
    p2.exchangeType = "okx";
    p2.apiKey = "okey";
    p2.passphrase = "pass";
    db.upsertProfile(p2);

    db.setActiveProfile("okx-test");

    auto profiles = db.listProfiles();
    EXPECT_EQ(profiles.size(), 2u);

    auto prof = db.getProfile("okx-test");
    ASSERT_TRUE(prof.has_value());
    EXPECT_EQ(prof->passphrase, "pass");

    auto missing = db.getProfile("nonexistent");
    EXPECT_FALSE(missing.has_value());

    std::remove(dbPath.c_str());
}

// Test GuiConfig defaults include new exchanges and fields
TEST(GuiConfig, DefaultValues) {
    GuiConfig cfg;
    EXPECT_EQ(cfg.exchangeName, "binance");
    EXPECT_EQ(cfg.marketType, "futures");
    EXPECT_EQ(cfg.passphrase, "");
    EXPECT_TRUE(cfg.indEmaEnabled);
    EXPECT_TRUE(cfg.indRsiEnabled);
    EXPECT_TRUE(cfg.indAtrEnabled);
    EXPECT_TRUE(cfg.indMacdEnabled);
    EXPECT_TRUE(cfg.indBbEnabled);
    EXPECT_DOUBLE_EQ(cfg.filterMinVolume, 0.0);
    EXPECT_DOUBLE_EQ(cfg.filterMinPrice, 0.0);
    EXPECT_DOUBLE_EQ(cfg.filterMaxPrice, 0.0);
    EXPECT_DOUBLE_EQ(cfg.filterMinChange, -100.0);
    EXPECT_DOUBLE_EQ(cfg.filterMaxChange, 100.0);
}

// Test GuiState includes order book
TEST(GuiState, OrderBookDefault) {
    GuiState state;
    EXPECT_TRUE(state.orderBook.bids.empty());
    EXPECT_TRUE(state.orderBook.asks.empty());
    EXPECT_EQ(state.orderBook.lastUpdateId, 0);
}

// ── Per-exchange separate database tests ─────────────────────────────────────

TEST(ExchangeDB, SeparateDbPerExchange) {
    // Verify that each exchange has its own isolated database file
    std::string binancePath = "/tmp/test_db_binance.json";
    std::string bybitPath   = "/tmp/test_db_bybit.json";
    std::string okxPath     = "/tmp/test_db_okx.json";
    std::string bitgetPath  = "/tmp/test_db_bitget.json";
    std::string kucoinPath  = "/tmp/test_db_kucoin.json";

    // Clean up
    std::remove(binancePath.c_str());
    std::remove(bybitPath.c_str());
    std::remove(okxPath.c_str());
    std::remove(bitgetPath.c_str());
    std::remove(kucoinPath.c_str());

    // Create separate DB for each exchange
    ExchangeDB dbBinance(binancePath);
    ExchangeDB dbBybit(bybitPath);
    ExchangeDB dbOkx(okxPath);
    ExchangeDB dbBitget(bitgetPath);
    ExchangeDB dbKucoin(kucoinPath);

    dbBinance.load();
    dbBybit.load();
    dbOkx.load();
    dbBitget.load();
    dbKucoin.load();

    // Add profiles to each
    ExchangeProfile p;

    p.name = "binance"; p.exchangeType = "binance"; p.apiKey = "bkey";
    dbBinance.upsertProfile(p);
    dbBinance.setActiveProfile("binance");
    EXPECT_TRUE(dbBinance.save());

    p.name = "bybit"; p.exchangeType = "bybit"; p.apiKey = "bykey";
    dbBybit.upsertProfile(p);
    dbBybit.setActiveProfile("bybit");
    EXPECT_TRUE(dbBybit.save());

    p.name = "okx"; p.exchangeType = "okx"; p.apiKey = "okey"; p.passphrase = "opass";
    dbOkx.upsertProfile(p);
    dbOkx.setActiveProfile("okx");
    EXPECT_TRUE(dbOkx.save());

    p.name = "bitget"; p.exchangeType = "bitget"; p.apiKey = "bgkey"; p.passphrase = "bgpass";
    dbBitget.upsertProfile(p);
    dbBitget.setActiveProfile("bitget");
    EXPECT_TRUE(dbBitget.save());

    p.name = "kucoin"; p.exchangeType = "kucoin"; p.apiKey = "kckey"; p.passphrase = "kcpass";
    dbKucoin.upsertProfile(p);
    dbKucoin.setActiveProfile("kucoin");
    EXPECT_TRUE(dbKucoin.save());

    // Reload and verify isolation: each DB has exactly 1 profile with correct data
    {
        ExchangeDB db(binancePath);
        EXPECT_TRUE(db.load());
        auto profiles = db.listProfiles();
        ASSERT_EQ(profiles.size(), 1u);
        EXPECT_EQ(profiles[0].exchangeType, "binance");
        EXPECT_EQ(profiles[0].apiKey, "bkey");
    }
    {
        ExchangeDB db(bybitPath);
        EXPECT_TRUE(db.load());
        auto profiles = db.listProfiles();
        ASSERT_EQ(profiles.size(), 1u);
        EXPECT_EQ(profiles[0].exchangeType, "bybit");
        EXPECT_EQ(profiles[0].apiKey, "bykey");
    }
    {
        ExchangeDB db(okxPath);
        EXPECT_TRUE(db.load());
        auto profiles = db.listProfiles();
        ASSERT_EQ(profiles.size(), 1u);
        EXPECT_EQ(profiles[0].exchangeType, "okx");
        EXPECT_EQ(profiles[0].apiKey, "okey");
        EXPECT_EQ(profiles[0].passphrase, "opass");
    }
    {
        ExchangeDB db(bitgetPath);
        EXPECT_TRUE(db.load());
        auto profiles = db.listProfiles();
        ASSERT_EQ(profiles.size(), 1u);
        EXPECT_EQ(profiles[0].exchangeType, "bitget");
        EXPECT_EQ(profiles[0].apiKey, "bgkey");
        EXPECT_EQ(profiles[0].passphrase, "bgpass");
    }
    {
        ExchangeDB db(kucoinPath);
        EXPECT_TRUE(db.load());
        auto profiles = db.listProfiles();
        ASSERT_EQ(profiles.size(), 1u);
        EXPECT_EQ(profiles[0].exchangeType, "kucoin");
        EXPECT_EQ(profiles[0].apiKey, "kckey");
        EXPECT_EQ(profiles[0].passphrase, "kcpass");
    }

    // Clean up
    std::remove(binancePath.c_str());
    std::remove(bybitPath.c_str());
    std::remove(okxPath.c_str());
    std::remove(bitgetPath.c_str());
    std::remove(kucoinPath.c_str());
}

// ── Live data pull tests (public market endpoints) ───────────────────────────
// These tests verify that each exchange can pull real market data.
// They use public endpoints that do not require API keys.

TEST(LiveData, OKXGetPrice) {
    OKXExchange ex("", "", "");
    try {
        double price = ex.getPrice("BTC-USDT");
        EXPECT_GT(price, 0.0);
    } catch (const std::exception& e) {
        // Network issues should not crash; log and skip
        GTEST_SKIP() << "OKX API unreachable: " << e.what();
    }
}

TEST(LiveData, OKXGetKlines) {
    OKXExchange ex("", "", "");
    try {
        auto candles = ex.getKlines("BTC-USDT", "15m", 5);
        EXPECT_FALSE(candles.empty());
        // Verify chronological order (oldest first after reverse fix)
        if (candles.size() >= 2) {
            EXPECT_LE(candles.front().openTime, candles.back().openTime);
        }
        // Verify candle data is sane
        for (auto& c : candles) {
            EXPECT_GT(c.open, 0.0);
            EXPECT_GT(c.high, 0.0);
            EXPECT_GT(c.low, 0.0);
            EXPECT_GT(c.close, 0.0);
            EXPECT_GE(c.volume, 0.0);
            EXPECT_LE(c.low, c.high);
        }
    } catch (const std::exception& e) {
        GTEST_SKIP() << "OKX API unreachable: " << e.what();
    }
}

TEST(LiveData, KuCoinGetPrice) {
    KuCoinExchange ex("", "", "");
    try {
        double price = ex.getPrice("BTC-USDT");
        EXPECT_GT(price, 0.0);
    } catch (const std::exception& e) {
        GTEST_SKIP() << "KuCoin API unreachable: " << e.what();
    }
}

TEST(LiveData, KuCoinGetKlines) {
    KuCoinExchange ex("", "", "");
    try {
        auto candles = ex.getKlines("BTC-USDT", "15min", 5);
        EXPECT_FALSE(candles.empty());
        // Verify candle data is sane
        for (auto& c : candles) {
            EXPECT_GT(c.open, 0.0);
            EXPECT_GT(c.high, 0.0);
            EXPECT_GT(c.low, 0.0);
            EXPECT_GT(c.close, 0.0);
            EXPECT_GE(c.volume, 0.0);
            EXPECT_LE(c.low, c.high);
        }
    } catch (const std::exception& e) {
        GTEST_SKIP() << "KuCoin API unreachable: " << e.what();
    }
}

TEST(LiveData, BitgetGetPrice) {
    BitgetExchange ex("", "", "");
    try {
        double price = ex.getPrice("BTCUSDT");
        EXPECT_GT(price, 0.0);
    } catch (const std::exception& e) {
        GTEST_SKIP() << "Bitget API unreachable: " << e.what();
    }
}

TEST(LiveData, BitgetGetKlines) {
    BitgetExchange ex("", "", "");
    try {
        auto candles = ex.getKlines("BTCUSDT", "15m", 5);
        EXPECT_FALSE(candles.empty());
        // Verify chronological order after reverse fix
        if (candles.size() >= 2) {
            EXPECT_LE(candles.front().openTime, candles.back().openTime);
        }
        for (auto& c : candles) {
            EXPECT_GT(c.open, 0.0);
            EXPECT_GT(c.high, 0.0);
            EXPECT_GT(c.low, 0.0);
            EXPECT_GT(c.close, 0.0);
            EXPECT_GE(c.volume, 0.0);
            EXPECT_LE(c.low, c.high);
        }
    } catch (const std::exception& e) {
        GTEST_SKIP() << "Bitget API unreachable: " << e.what();
    }
}

TEST(LiveData, BinanceGetPrice) {
    BinanceExchange ex("", "");
    try {
        double price = ex.getPrice("BTCUSDT");
        EXPECT_GT(price, 0.0);
    } catch (const std::exception& e) {
        // Binance may be geo-restricted in some regions
        GTEST_SKIP() << "Binance API unreachable: " << e.what();
    }
}

TEST(LiveData, BybitGetPrice) {
    BybitExchange ex("", "");
    try {
        double price = ex.getPrice("BTCUSDT");
        EXPECT_GT(price, 0.0);
    } catch (const std::exception& e) {
        // Bybit may be geo-restricted in some regions
        GTEST_SKIP() << "Bybit API unreachable: " << e.what();
    }
}
