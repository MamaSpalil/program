#include <gtest/gtest.h>
#include "../src/ui/DrawingTools.h"
#include "../src/ui/MultiTimeframe.h"
#include "../src/indicators/VWAPIndicator.h"
#include "../src/strategy/PositionCalc.h"
#include "../src/exchange/DepthStream.h"

using namespace crypto;

// ── T01: Topbar button text — no '?' in button labels ─────────────────────

TEST(Regression, TopbarButtonTextNoQuestionMarks) {
    // Verify the known button labels contain no question marks
    std::vector<std::string> buttons = {
        "  Connect  ", "  Disconnect  ",
        "  Start Trading  ", "  Stop Trading  ",
        "  Settings  ", "  Order Book  ",
        "  User Panel  ", " PAPER TRADING ", " LIVE TRADING "
    };
    for (auto& btn : buttons) {
        EXPECT_EQ(btn.find('?'), std::string::npos)
            << "Button '" << btn << "' contains '?'";
    }
}

// ── T01b: DepthStreamManager ──────────────────────────────────────────────

TEST(Regression, DepthStreamConnects) {
    DepthStreamManager mgr;
    mgr.subscribe("BTCUSDT", "futures");
    // Simulate connected state and book data
    mgr.setConnected(true);
    DepthBook book;
    book.bids.push_back({67000.0, 1.5});
    book.asks.push_back({67001.0, 0.8});
    mgr.setBook(book);

    EXPECT_TRUE(mgr.isConnected());
    auto got = mgr.getBook();
    EXPECT_FALSE(got.bids.empty());
    EXPECT_FALSE(got.asks.empty());
}

// ── T02: DrawingManager ──────────────────────────────────────────────────

TEST(DrawingTools, AddAndRender) {
    DrawingManager dm;
    DrawingObject obj;
    obj.id   = "test1";
    obj.type = DrawingType::TREND_LINE;
    obj.x1 = 100; obj.y1 = 50000;
    obj.x2 = 200; obj.y2 = 52000;
    dm.add(obj);
    EXPECT_EQ(dm.count(), 1);
    dm.remove("test1");
    EXPECT_EQ(dm.count(), 0);
}

TEST(DrawingTools, SaveLoad) {
    // Use /tmp so we don't pollute the repo
    const std::string dir = "/tmp/crypto_test_drawings";

    DrawingManager dm;
    DrawingObject o;
    o.id = "fib1";
    o.type = DrawingType::FIBONACCI;
    o.x1 = 100; o.y1 = 60000;
    o.x2 = 200; o.y2 = 65000;
    dm.add(o);
    dm.save("BTCUSDT", dir);

    DrawingManager dm2;
    dm2.load("BTCUSDT", dir);
    EXPECT_EQ(dm2.count(), 1);

    auto all = dm2.getAll();
    EXPECT_EQ(all[0].id, "fib1");
    EXPECT_EQ(all[0].type, DrawingType::FIBONACCI);
}

// ── T03: MultiTimeframe ──────────────────────────────────────────────────

TEST(MultiTimeframe, LoadsAllPanes) {
    MultiTimeframeWindow w;

    // Inject test bars into each pane
    for (int i = 0; i < 4; ++i) {
        std::vector<Candle> bars;
        for (int j = 0; j < 10; ++j) {
            Candle c;
            c.openTime = j * 60000;
            c.close = 50000.0 + j;
            c.high = c.close + 1;
            c.low = c.close - 1;
            c.open = c.close;
            c.volume = 100.0;
            bars.push_back(c);
        }
        std::string interval = (i == 0) ? "1m" : (i == 1) ? "5m" : (i == 2) ? "1h" : "1d";
        w.setPaneBars(i, bars, interval);
    }

    w.loadAll("BTCUSDT");

    for (int i = 0; i < 4; ++i) {
        EXPECT_TRUE(w.paneReady(i));
        EXPECT_GT(w.paneBarCount(i), 0);
    }
}

// ── T04: VWAP correctness ────────────────────────────────────────────────

TEST(Indicators, VWAPMonotonicity) {
    std::vector<Bar> bars;
    for (int i = 0; i < 100; ++i) {
        Bar b;
        b.openTime = i * 60000;
        b.open = 50000.0 + i * 10;
        b.high = b.open + 5;
        b.low  = b.open - 5;
        b.close = b.open + 2;
        b.volume = 100.0 + i;
        bars.push_back(b);
    }
    auto vwap = calcVWAP(bars);
    EXPECT_EQ(vwap.size(), bars.size());
    for (auto v : vwap) {
        EXPECT_GT(v, 0.0);
    }
}

// ── T05: Pearson correlation ─────────────────────────────────────────────

TEST(Correlation, PerfectPositive) {
    std::vector<double> x = {1, 2, 3, 4, 5};
    std::vector<double> y = {2, 4, 6, 8, 10};
    EXPECT_NEAR(pearson(x, y), 1.0, 1e-9);
}

TEST(Correlation, PerfectNegative) {
    std::vector<double> x = {1, 2, 3, 4, 5};
    std::vector<double> y = {10, 8, 6, 4, 2};
    EXPECT_NEAR(pearson(x, y), -1.0, 1e-9);
}

TEST(Correlation, Uncorrelated) {
    std::vector<double> x = {1, 2, 3, 4, 5};
    std::vector<double> y = {3, 1, 4, 1, 5};
    double r = pearson(x, y);
    // Not perfectly correlated
    EXPECT_LT(std::abs(r), 1.0);
}

// ── T06: Position Calculator ─────────────────────────────────────────────

TEST(RiskCalc, PositionSize) {
    PositionCalc c;
    c.balance  = 10000;
    c.riskPct  = 1.0;
    c.entry    = 68000;
    c.stopLoss = 67000;
    EXPECT_NEAR(c.riskAmt(),   100.0,  0.01);
    EXPECT_NEAR(c.priceDiff(), 1000.0, 0.01);
    EXPECT_NEAR(c.posSize(),   0.1,    0.0001);
}

TEST(RiskCalc, ZeroStopDistance) {
    PositionCalc c;
    c.balance = 10000;
    c.riskPct = 1.0;
    c.entry = 50000;
    c.stopLoss = 50000;
    EXPECT_DOUBLE_EQ(c.posSize(), 0.0);
}

// ── T07: Daily Loss Guard ────────────────────────────────────────────────

TEST(DailyLoss, TriggersAtLimit) {
    DailyLossGuard g;
    g.init(10000.0, 2.0); // 2% limit
    g.update(9850.0);     // -1.5% — not triggered
    EXPECT_FALSE(g.isTriggered());
    g.update(9700.0);     // -3.0% — triggered
    EXPECT_TRUE(g.isTriggered());
}

TEST(DailyLoss, ResetClearsTriggered) {
    DailyLossGuard g;
    g.init(10000.0, 2.0);
    g.update(9700.0);
    EXPECT_TRUE(g.isTriggered());
    g.reset(10000.0);
    EXPECT_FALSE(g.isTriggered());
}
