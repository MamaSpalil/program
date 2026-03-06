#include <gtest/gtest.h>
#include "../src/exchange/EndpointManager.h"
#include "../src/exchange/BinanceExchange.h"
#include "../src/ui/LayoutManager.h"

using namespace crypto;

// ══════════════════════════════════════════════════════════════════════════
// GROUP 1 — Geo-blocking: EndpointManager integration
// ══════════════════════════════════════════════════════════════════════════

TEST(EndpointManager, IsGeoBlocked403) {
    EXPECT_TRUE(ExchangeEndpointManager::isGeoBlocked(403));
}

TEST(EndpointManager, IsGeoBlocked451) {
    EXPECT_TRUE(ExchangeEndpointManager::isGeoBlocked(451));
}

TEST(EndpointManager, NotGeoBlocked200) {
    EXPECT_FALSE(ExchangeEndpointManager::isGeoBlocked(200));
}

TEST(EndpointManager, NotGeoBlocked429) {
    EXPECT_FALSE(ExchangeEndpointManager::isGeoBlocked(429));
}

TEST(EndpointManager, RoundRobinSpotEndpoints) {
    int idx = -1;
    const auto& eps = ExchangeEndpointManager::binanceEndpoints();
    std::string first = ExchangeEndpointManager::getNextEndpoint(eps, idx);
    EXPECT_EQ(idx, 0);
    EXPECT_EQ(first, "https://api.binance.com");

    std::string second = ExchangeEndpointManager::getNextEndpoint(eps, idx);
    EXPECT_EQ(idx, 1);
    EXPECT_EQ(second, "https://api1.binance.com");
}

TEST(EndpointManager, RoundRobinWraps) {
    int idx = 3;
    const auto& eps = ExchangeEndpointManager::binanceEndpoints();
    // idx 3 -> next is 4
    std::string e4 = ExchangeEndpointManager::getNextEndpoint(eps, idx);
    EXPECT_EQ(idx, 4);
    EXPECT_EQ(e4, "https://api4.binance.com");

    // idx 4 -> wraps to 0
    std::string e0 = ExchangeEndpointManager::getNextEndpoint(eps, idx);
    EXPECT_EQ(idx, 0);
    EXPECT_EQ(e0, "https://api.binance.com");
}

TEST(EndpointManager, FuturesEndpointsExist) {
    const auto& eps = ExchangeEndpointManager::binanceFuturesEndpoints();
    EXPECT_GE(eps.size(), 2u);
    EXPECT_EQ(eps[0], "https://fapi.binance.com");
}

TEST(BinanceExchange, ReplaceBase) {
    BinanceExchange ex("key", "secret", "https://api.binance.com");
    // replaceBase is private but we test the isUsingAltEndpoint accessor
    // which is set via httpGet geo-blocking logic.
    // At construction, alt endpoint should be false.
    EXPECT_FALSE(ex.isUsingAltEndpoint());
}

// ══════════════════════════════════════════════════════════════════════════
// GROUP 2 — LayoutManager
// ══════════════════════════════════════════════════════════════════════════

TEST(LayoutManager, LockedFlagsNonZero) {
    int flags = LayoutManager::lockedFlags();
    EXPECT_NE(flags, 0);
}

TEST(LayoutManager, LockedScrollFlagsNonZero) {
    int flags = LayoutManager::lockedScrollFlags();
    EXPECT_NE(flags, 0);
}

TEST(LayoutManager, ScrollFlagsIncludeLockedFlags) {
    int locked = LayoutManager::lockedFlags();
    int scroll = LayoutManager::lockedScrollFlags();
    // Scroll flags should include all locked flags plus more
    EXPECT_EQ(locked & scroll, locked);
    EXPECT_NE(locked, scroll);
}

TEST(LayoutManager, RecalculateCreatesLayouts) {
    LayoutManager mgr;
    mgr.recalculate(1920, 1080);

    EXPECT_TRUE(mgr.hasLayout("Main Toolbar"));
    EXPECT_TRUE(mgr.hasLayout("Filters Bar"));
    EXPECT_TRUE(mgr.hasLayout("Pair List"));
    EXPECT_TRUE(mgr.hasLayout("Logs"));
    EXPECT_TRUE(mgr.hasLayout("Volume Delta"));
    EXPECT_TRUE(mgr.hasLayout("Market Data"));
    EXPECT_TRUE(mgr.hasLayout("Indicators"));
    EXPECT_TRUE(mgr.hasLayout("User Panel"));
}

TEST(LayoutManager, GetDefaultsForUnknown) {
    LayoutManager mgr;
    mgr.recalculate(1920, 1080);

    auto layout = mgr.get("NonExistentWindow");
    EXPECT_EQ(layout.name, "NonExistentWindow");
    // Should return default position/size
    EXPECT_GT(layout.pos.x, 0);
    EXPECT_GT(layout.size.x, 0);
    EXPECT_TRUE(layout.visible);
}

TEST(LayoutManager, WindowsDoNotOverlap) {
    LayoutManager mgr;
    mgr.recalculate(1920, 1080);

    auto pl  = mgr.get("Pair List");
    auto vd  = mgr.get("Volume Delta");
    auto md  = mgr.get("Market Data");
    auto ind = mgr.get("Indicators");

    // Left column: Pair List above Volume Delta
    EXPECT_LT(pl.pos.y + pl.size.y, vd.pos.y + 1);

    // Center column: Market Data above Indicators
    EXPECT_LT(md.pos.y + md.size.y, ind.pos.y + 1);

    // Logs is at bottom, below the center column
    auto log = mgr.get("Logs");
    EXPECT_GT(log.pos.y, ind.pos.y);

    // Pair List / VD is left of center column
    EXPECT_LE(pl.pos.x + pl.size.x, md.pos.x + 1);
    EXPECT_LE(vd.pos.x + vd.size.x, md.pos.x + 1);

    // User Panel is right of center column
    auto up = mgr.get("User Panel");
    EXPECT_GE(up.pos.x, md.pos.x + md.size.x - 1);
}

TEST(LayoutManager, DifferentScreenSizesProduceDifferentLayouts) {
    LayoutManager mgr1, mgr2;
    mgr1.recalculate(1920, 1080);
    mgr2.recalculate(1280, 720);

    auto md1 = mgr1.get("Market Data");
    auto md2 = mgr2.get("Market Data");

    // Different screen sizes should produce different widths
    EXPECT_NE(md1.size.x, md2.size.x);
}

TEST(LayoutManager, HasLayoutFalseBeforeRecalculate) {
    LayoutManager mgr;
    EXPECT_FALSE(mgr.hasLayout("Logs"));
    EXPECT_FALSE(mgr.hasLayout("Volume Delta"));
    EXPECT_FALSE(mgr.hasLayout("Market Data"));
}

TEST(LayoutManager, MarketDataIsTallest) {
    LayoutManager mgr;
    mgr.recalculate(1920, 1080);

    auto log = mgr.get("Logs");
    auto vd  = mgr.get("Volume Delta");
    auto md  = mgr.get("Market Data");
    auto ind = mgr.get("Indicators");

    EXPECT_GT(md.size.y, log.size.y);
    EXPECT_GT(md.size.y, vd.size.y);
    EXPECT_GT(md.size.y, ind.size.y);
}

TEST(LayoutManager, WindowsAreFullWidth) {
    LayoutManager mgr;
    mgr.recalculate(1920, 1080);

    auto log = mgr.get("Logs");
    auto vd  = mgr.get("Volume Delta");
    auto pl  = mgr.get("Pair List");
    auto md  = mgr.get("Market Data");
    auto ind = mgr.get("Indicators");
    auto tb  = mgr.get("Main Toolbar");

    // Center column windows should have the same width
    EXPECT_FLOAT_EQ(md.size.x, ind.size.x);

    // Volume Delta is in left column (same width as Pair List)
    EXPECT_FLOAT_EQ(vd.size.x, pl.size.x);
    EXPECT_FLOAT_EQ(vd.size.x, 200.0f);

    // Logs and Main Toolbar are full screen width
    EXPECT_GT(log.size.x, 1900.0f);
    EXPECT_GT(tb.size.x, 1900.0f);

    // Center column width = screenW - pairListW(200) - userPanelW(290) - 2px gaps
    EXPECT_FLOAT_EQ(md.size.x, 1920.0f - 200.0f - 290.0f - 2.0f);
}

TEST(LayoutManager, CustomProportions) {
    LayoutManager mgr;
    mgr.setLogPct(0.15f);
    mgr.setVdPct(0.10f);
    mgr.setIndPct(0.20f);
    mgr.recalculate(1920, 1080);

    EXPECT_FLOAT_EQ(mgr.logPct(), 0.15f);
    EXPECT_FLOAT_EQ(mgr.vdPct(),  0.10f);
    EXPECT_FLOAT_EQ(mgr.indPct(), 0.20f);

    auto log = mgr.get("Logs");
    auto md  = mgr.get("Market Data");
    // Market Data should still be tallest (gets remaining space)
    EXPECT_GT(md.size.y, log.size.y);
}

TEST(LayoutManager, LogsWindowExists) {
    LayoutManager mgr;
    mgr.recalculate(1920, 1080);

    auto log = mgr.get("Logs");
    EXPECT_EQ(log.name, "Logs");
    EXPECT_GT(log.size.x, 0);
    EXPECT_GT(log.size.y, 0);
}
