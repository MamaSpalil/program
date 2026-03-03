#include <gtest/gtest.h>
#include "../src/indicators/EMA.h"
#include "../src/indicators/RSI.h"
#include "../src/indicators/ATR.h"
#include "../src/indicators/PineScriptIndicator.h"

using namespace crypto;

TEST(EMA, BasicCalculation) {
    EMA ema(3);
    EXPECT_FALSE(ema.ready());

    // Feed 3 candles to initialize
    Candle c{};
    c.close = 10.0; ema.update(c);
    c.close = 20.0; ema.update(c);
    c.close = 30.0; ema.update(c);
    EXPECT_TRUE(ema.ready());
    EXPECT_DOUBLE_EQ(ema.value(), 20.0);  // SMA(3) = (10+20+30)/3

    // Next value: k = 2/4 = 0.5
    c.close = 40.0; ema.update(c);
    double expected = 40.0 * 0.5 + 20.0 * 0.5;
    EXPECT_DOUBLE_EQ(ema.value(), expected);
}

TEST(RSI, Flat) {
    RSI rsi(14);
    Candle c{};
    c.close = 100.0;
    for (int i = 0; i < 20; ++i) rsi.update(c);
    // With flat prices, all gains/losses are 0; avgLoss=0 → RSI=100 by convention
    EXPECT_TRUE(rsi.ready());
    EXPECT_DOUBLE_EQ(rsi.value(), 100.0);
}

TEST(RSI, AllGain) {
    RSI rsi(14);
    Candle c{};
    c.close = 100.0;
    rsi.update(c);
    for (int i = 1; i <= 20; ++i) {
        c.close = 100.0 + i;
        rsi.update(c);
    }
    EXPECT_TRUE(rsi.ready());
    // All gains -> RSI near 100
    EXPECT_GT(rsi.value(), 90.0);
}

TEST(ATR, BasicTrueRange) {
    ATR atr(3);
    Candle c{};
    c.high = 110; c.low = 90; c.close = 100; atr.update(c);
    c.high = 115; c.low = 95; c.close = 105; atr.update(c);
    c.high = 120; c.low = 100; c.close = 110; atr.update(c);
    EXPECT_TRUE(atr.ready());
    EXPECT_GT(atr.value(), 0.0);
}

TEST(PineScript, IndicatorReady) {
    PineScriptIndicator::Config cfg;
    cfg.emaFast = 3; cfg.emaSlow = 5; cfg.emaTrend = 10;
    cfg.rsiPeriod = 5; cfg.atrPeriod = 3;
    cfg.macdFast = 3; cfg.macdSlow = 5; cfg.macdSignal = 3;
    cfg.bbPeriod = 3;
    PineScriptIndicator ind(cfg);

    Candle c{};
    for (int i = 1; i <= 30; ++i) {
        c.close = 100.0 + i * 0.5;
        c.high  = c.close + 1.0;
        c.low   = c.close - 1.0;
        c.open  = c.close - 0.2;
        ind.update(c);
    }
    EXPECT_TRUE(ind.ready());
}

TEST(EMA, UpdateValue) {
    EMA ema(2);
    ema.updateValue(10.0);
    ema.updateValue(20.0);
    EXPECT_TRUE(ema.ready());
    // SMA(2) = (10+20)/2 = 15
    EXPECT_DOUBLE_EQ(ema.value(), 15.0);
}

// ── PineConverter tests ──────────────────────────────────────────────────────

#include "../src/indicators/PineConverter.h"
#include "../src/indicators/UserIndicatorManager.h"

TEST(PineConverter, ParseStrategyAsIndicator) {
    // strategy() should be treated like indicator()
    std::string src = R"(
// @version=6
strategy("My Strategy", overlay=true)
x = ta.ema(close, 9)
plot(x, "EMA9")
)";
    auto script = PineConverter::parseSource(src);
    EXPECT_EQ(script.name, "My Strategy");
    EXPECT_TRUE(script.overlay);
    EXPECT_EQ(script.version, 6);
}

TEST(PineConverter, ParseTypedVariableDeclaration) {
    std::string src = R"(
int MAX_BUFF = 4999
float PENALTY = 0.85
x = MAX_BUFF + PENALTY
plot(x, "result")
)";
    auto script = PineConverter::parseSource(src);
    // Should parse without throwing; typed declarations should work
    bool hasAssign = false;
    for (auto& s : script.statements) {
        if (s.kind == PineStmt::Kind::ASSIGN && s.name == "MAX_BUFF") hasAssign = true;
    }
    EXPECT_TRUE(hasAssign);
}

TEST(PineConverter, SkipFunctionDefinition) {
    // => function definitions should be skipped without error
    std::string src = R"(
f_clamp(int val, int ceil) => math.max(0, math.min(val, ceil))
x = 42
plot(x, "val")
)";
    auto script = PineConverter::parseSource(src);
    // Should have at least the assignment and plot
    bool hasX = false;
    for (auto& s : script.statements) {
        if (s.kind == PineStmt::Kind::ASSIGN && s.name == "x") hasX = true;
    }
    EXPECT_TRUE(hasX);
}

TEST(PineConverter, SkipTypeDeclaration) {
    // type declarations should be skipped
    std::string src = R"(
type MyType
x = 10
plot(x, "val")
)";
    auto script = PineConverter::parseSource(src);
    bool hasX = false;
    for (auto& s : script.statements) {
        if (s.kind == PineStmt::Kind::ASSIGN && s.name == "x") hasX = true;
    }
    EXPECT_TRUE(hasX);
}

TEST(PineConverter, ResilientParsing) {
    // Parser should skip unparseable lines and continue
    std::string src = R"(
// @version=6
indicator("Test")
@@invalid_syntax@@
x = ta.ema(close, 9)
plot(x, "EMA")
)";
    auto script = PineConverter::parseSource(src);
    EXPECT_EQ(script.name, "Test");
    // Should have loaded at least some statements
    EXPECT_GE(script.statements.size(), 1u);
}

TEST(PineRuntime, InputStringAndBool) {
    std::string src = R"(
x = input.int(14, "Period")
y = input.float(0.5, "Factor")
plot(x, "period")
plot(y, "factor")
)";
    auto script = PineConverter::parseSource(src);
    PineRuntime rt;
    rt.load(script);

    Candle c{};
    c.close = 100; c.open = 99; c.high = 101; c.low = 98; c.volume = 1000;
    auto plots = rt.update(c);

    EXPECT_NEAR(plots["period"], 14.0, 0.01);
    EXPECT_NEAR(plots["factor"], 0.5, 0.01);
}

TEST(PineRuntime, BarIndex) {
    std::string src = R"(
plot(bar_index, "bi")
)";
    auto script = PineConverter::parseSource(src);
    PineRuntime rt;
    rt.load(script);

    Candle c{};
    c.close = 100; c.open = 99; c.high = 101; c.low = 98;
    auto p1 = rt.update(c);
    EXPECT_NEAR(p1["bi"], 0.0, 0.01);
    auto p2 = rt.update(c);
    EXPECT_NEAR(p2["bi"], 1.0, 0.01);
}

TEST(PineRuntime, MathFunctions) {
    std::string src = R"(
a = math.round(3.7)
b = math.ceil(3.2)
c = math.floor(3.9)
d = math.pow(2, 3)
plot(a, "round")
plot(b, "ceil")
plot(c, "floor")
plot(d, "pow")
)";
    auto script = PineConverter::parseSource(src);
    PineRuntime rt;
    rt.load(script);

    Candle candle{};
    candle.close = 100; candle.open = 99; candle.high = 101; candle.low = 98;
    auto plots = rt.update(candle);

    EXPECT_NEAR(plots["round"], 4.0, 0.01);
    EXPECT_NEAR(plots["ceil"], 4.0, 0.01);
    EXPECT_NEAR(plots["floor"], 3.0, 0.01);
    EXPECT_NEAR(plots["pow"], 8.0, 0.01);
}

TEST(UserIndicatorManager, ScanAndLoadPineFiles) {
    // Use absolute path to source repo's user_indicator directory
    std::string dir = std::string(CMAKE_SOURCE_DIR) + "/user_indicator";
    UserIndicatorManager mgr(dir);
    mgr.scanAndLoad();
    auto loaded = mgr.loadedIndicators();
    // Should have loaded the two .pine files
    EXPECT_GE(loaded.size(), 1u);
    EXPECT_TRUE(mgr.hasIndicators());
}
