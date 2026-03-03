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

TEST(Exchanges, OKXTestConnectionInvalid) {
    OKXExchange ex("invalid", "invalid", "invalid");
    std::string error;
    bool ok = ex.testConnection(error);
    EXPECT_FALSE(ok);
    EXPECT_FALSE(error.empty());
}

TEST(Exchanges, BitgetTestConnectionInvalid) {
    BitgetExchange ex("invalid", "invalid", "invalid");
    std::string error;
    bool ok = ex.testConnection(error);
    EXPECT_FALSE(ok);
    EXPECT_FALSE(error.empty());
}

TEST(Exchanges, KuCoinTestConnectionInvalid) {
    KuCoinExchange ex("invalid", "invalid", "invalid");
    std::string error;
    bool ok = ex.testConnection(error);
    EXPECT_FALSE(ok);
    EXPECT_FALSE(error.empty());
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
