#include <gtest/gtest.h>
#include "../src/exchange/BinanceExchange.h"
#include "../src/exchange/BybitExchange.h"
#include "../src/exchange/OKXExchange.h"
#include "../src/exchange/BitgetExchange.h"
#include "../src/exchange/KuCoinExchange.h"
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
