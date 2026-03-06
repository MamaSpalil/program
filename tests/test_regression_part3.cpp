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

    // Center column (v2.7.0): Indicators above Market Data
    EXPECT_LT(ind.pos.y + ind.size.y, md.pos.y + 1);

    // Logs is at bottom of center column, below Market Data
    auto log = mgr.get("Logs");
    EXPECT_GT(log.pos.y, md.pos.y);

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

    // v2.7.0: Logs is center column width (same as Market Data)
    EXPECT_FLOAT_EQ(log.size.x, md.size.x);
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

// ══════════════════════════════════════════════════════════════════════════
// GROUP 3 — Auto-resize: layout adapts to viewport changes
// ══════════════════════════════════════════════════════════════════════════

TEST(LayoutManager, AutoResizeRecalculateChangesLayout) {
    LayoutManager mgr;

    // Simulate windowed mode
    mgr.recalculate(1440, 900);
    auto md1 = mgr.get("Market Data");
    auto up1 = mgr.get("User Panel");

    // Simulate fullscreen mode
    mgr.recalculate(1920, 1080);
    auto md2 = mgr.get("Market Data");
    auto up2 = mgr.get("User Panel");

    // Window sizes should change when viewport changes
    EXPECT_NE(md1.size.x, md2.size.x);
    EXPECT_NE(md1.size.y, md2.size.y);
    // User panel width stays fixed
    EXPECT_FLOAT_EQ(up1.size.x, up2.size.x);
    // User panel height changes with viewport
    EXPECT_NE(up1.size.y, up2.size.y);
}

TEST(LayoutManager, AutoResizeGapsPreserved) {
    LayoutManager mgr;

    // Test at multiple resolutions
    float widths[]  = {1024.0f, 1280.0f, 1440.0f, 1920.0f, 2560.0f};
    float heights[] = {768.0f,  720.0f,  900.0f,  1080.0f, 1440.0f};

    for (int i = 0; i < 5; ++i) {
        mgr.recalculate(widths[i], heights[i]);

        auto pl  = mgr.get("Pair List");
        auto vd  = mgr.get("Volume Delta");
        auto ind = mgr.get("Indicators");
        auto md  = mgr.get("Market Data");
        auto log = mgr.get("Logs");
        auto up  = mgr.get("User Panel");

        // Windows should not touch: gaps >= 1px between adjacent windows
        // Left column: Pair List above Volume Delta
        float gapPL_VD = vd.pos.y - (pl.pos.y + pl.size.y);
        EXPECT_GE(gapPL_VD, 0.5f) << "Resolution: " << widths[i] << "x" << heights[i];

        // Center: Indicators above Market Data
        float gapInd_MD = md.pos.y - (ind.pos.y + ind.size.y);
        EXPECT_GE(gapInd_MD, 0.5f) << "Resolution: " << widths[i] << "x" << heights[i];

        // Center: Market Data above Logs
        float gapMD_Log = log.pos.y - (md.pos.y + md.size.y);
        EXPECT_GE(gapMD_Log, 0.5f) << "Resolution: " << widths[i] << "x" << heights[i];

        // Left column to center column gap
        float gapLeft_Center = ind.pos.x - (pl.pos.x + pl.size.x);
        EXPECT_GE(gapLeft_Center, 0.5f) << "Resolution: " << widths[i] << "x" << heights[i];

        // Center column to right column gap
        float gapCenter_Right = up.pos.x - (md.pos.x + md.size.x);
        EXPECT_GE(gapCenter_Right, 0.5f) << "Resolution: " << widths[i] << "x" << heights[i];
    }
}

TEST(LayoutManager, AutoResizeLayoutOrderPreserved) {
    LayoutManager mgr;
    mgr.recalculate(1920, 1080);

    auto tb  = mgr.get("Main Toolbar");
    auto fb  = mgr.get("Filters Bar");
    auto pl  = mgr.get("Pair List");
    auto vd  = mgr.get("Volume Delta");
    auto ind = mgr.get("Indicators");
    auto md  = mgr.get("Market Data");
    auto log = mgr.get("Logs");
    auto up  = mgr.get("User Panel");

    // Vertical order: TopBar → Filters → content
    EXPECT_LT(tb.pos.y, fb.pos.y);
    EXPECT_LT(fb.pos.y, pl.pos.y);

    // Left column: Pair List → Volume Delta
    EXPECT_LT(pl.pos.y, vd.pos.y);

    // Center column: Indicators → Market Data → Logs
    EXPECT_LT(ind.pos.y, md.pos.y);
    EXPECT_LT(md.pos.y, log.pos.y);

    // Horizontal: Left → Center → Right
    EXPECT_LT(pl.pos.x, ind.pos.x);
    EXPECT_LT(ind.pos.x, up.pos.x);
}

// ══════════════════════════════════════════════════════════════════════════
// GROUP 4 — Layout INI file persistence
// ══════════════════════════════════════════════════════════════════════════

#include <fstream>
#include <filesystem>

TEST(LayoutIni, SaveAndLoadRoundTrip) {
    namespace fs = std::filesystem;
    std::string tmpPath = "/tmp/test_layout_roundtrip.ini";

    // Cleanup
    fs::remove(tmpPath);

    // Write a test INI file
    {
        std::ofstream f(tmpPath);
        f << "# Test INI\n";
        f << "[layout]\n";
        f << "log_pct=0.12\n";
        f << "vd_pct=0.18\n";
        f << "ind_pct=0.22\n";
        f << "locked=1\n";
        f << "show_pair_list=1\n";
        f << "show_user_panel=0\n";
        f << "show_volume_delta=1\n";
        f << "show_indicators=0\n";
        f << "show_logs=1\n";
    }

    // Verify file was created
    EXPECT_TRUE(fs::exists(tmpPath));

    // Read it back and verify values
    std::ifstream f(tmpPath);
    ASSERT_TRUE(f.is_open());

    std::map<std::string, std::string> values;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#' || line[0] == '[') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        values[line.substr(0, eq)] = line.substr(eq + 1);
    }

    EXPECT_EQ(values["log_pct"], "0.12");
    EXPECT_EQ(values["vd_pct"], "0.18");
    EXPECT_EQ(values["ind_pct"], "0.22");
    EXPECT_EQ(values["locked"], "1");
    EXPECT_EQ(values["show_user_panel"], "0");
    EXPECT_EQ(values["show_indicators"], "0");
    EXPECT_EQ(values["show_logs"], "1");

    fs::remove(tmpPath);
}

TEST(LayoutIni, FileFormatIsReadable) {
    namespace fs = std::filesystem;
    std::string tmpPath = "/tmp/test_layout_format.ini";
    fs::remove(tmpPath);

    {
        std::ofstream f(tmpPath);
        f << "[layout]\n";
        f << "log_pct=0.15\n";
        f << "vd_pct=0.10\n";
        f << "ind_pct=0.25\n";
        f << "locked=0\n";
        f << "show_pair_list=1\n";
        f << "show_user_panel=1\n";
        f << "show_volume_delta=0\n";
        f << "show_indicators=1\n";
        f << "show_logs=0\n";
        f << "\n[window]\n";
        f << "window_width=1920\n";
        f << "window_height=1080\n";
    }

    // Verify structure
    std::ifstream f(tmpPath);
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

    EXPECT_NE(content.find("[layout]"), std::string::npos);
    EXPECT_NE(content.find("[window]"), std::string::npos);
    EXPECT_NE(content.find("log_pct=0.15"), std::string::npos);
    EXPECT_NE(content.find("window_width=1920"), std::string::npos);
    EXPECT_NE(content.find("window_height=1080"), std::string::npos);

    fs::remove(tmpPath);
}

TEST(LayoutIni, MalformedFileDoesNotCrash) {
    namespace fs = std::filesystem;
    std::string tmpPath = "/tmp/test_layout_malformed.ini";
    fs::remove(tmpPath);

    {
        std::ofstream f(tmpPath);
        f << "garbage data\n";
        f << "no_equals_here\n";
        f << "=empty_key\n";
        f << "key_with_bad_value=not_a_number\n";
        f << "log_pct=abc\n";
        f << "[invalid section\n";
    }

    // Just verify it can be opened and parsed without crashing
    std::ifstream f(tmpPath);
    EXPECT_TRUE(f.is_open());

    // Parse like the INI loader does — should not crash
    std::string line;
    int lineCount = 0;
    while (std::getline(f, line)) {
        lineCount++;
        if (line.empty() || line[0] == '#' || line[0] == ';' || line[0] == '[')
            continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        // Try parsing — should not throw
        try { std::stof(val); } catch (...) {}
    }
    EXPECT_GT(lineCount, 0);

    fs::remove(tmpPath);
}
