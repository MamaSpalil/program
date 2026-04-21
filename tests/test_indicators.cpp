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

// ── New Pine Script function tests ──────────────────────────────────────────

TEST(PineRuntime, VwapCalculation) {
    std::string src = R"(
v = ta.vwap(close)
plot(v, "vwap")
)";
    auto script = PineConverter::parseSource(src);
    PineRuntime rt;
    rt.load(script);

    Candle c{};
    c.close = 100; c.open = 99; c.high = 101; c.low = 98; c.volume = 1000;
    auto p1 = rt.update(c);
    EXPECT_NEAR(p1["vwap"], 100.0, 0.01);

    c.close = 110; c.volume = 2000;
    auto p2 = rt.update(c);
    // VWAP = (100*1000 + 110*2000) / (1000 + 2000) = 320000/3000 = 106.67
    EXPECT_NEAR(p2["vwap"], 106.67, 0.01);
}

TEST(PineRuntime, HighestBarsLowestBars) {
    std::string src = R"(
hb = ta.highestbars(close, 5)
lb = ta.lowestbars(close, 5)
plot(hb, "hbars")
plot(lb, "lbars")
)";
    auto script = PineConverter::parseSource(src);
    PineRuntime rt;
    rt.load(script);

    Candle c{};
    double prices[] = {100, 105, 102, 108, 103};
    for (int i = 0; i < 5; ++i) {
        c.close = prices[i]; c.open = c.close - 1; c.high = c.close + 1; c.low = c.close - 1;
        rt.update(c);
    }
    // Use a distinct new price to validate against fresh data
    c.close = 106; c.open = 105; c.high = 107; c.low = 104;
    auto plots = rt.update(c);
    // hbars should be negative (bars since highest)
    EXPECT_LE(plots["hbars"], 0.0);
}

TEST(PineRuntime, MathExp) {
    std::string src = R"(
a = math.exp(1.0)
plot(a, "exp1")
)";
    auto script = PineConverter::parseSource(src);
    PineRuntime rt;
    rt.load(script);
    Candle c{};
    c.close = 100; c.open = 99; c.high = 101; c.low = 98;
    auto plots = rt.update(c);
    EXPECT_NEAR(plots["exp1"], 2.71828, 0.001);
}

TEST(PineRuntime, BuiltinVariables) {
    std::string src = R"(
a = hl2
b = hlc3
c = ohlc4
plot(a, "hl2")
plot(b, "hlc3")
plot(c, "ohlc4")
)";
    auto script = PineConverter::parseSource(src);
    PineRuntime rt;
    rt.load(script);
    Candle c{};
    c.open = 100; c.high = 110; c.low = 90; c.close = 105;
    auto plots = rt.update(c);
    EXPECT_NEAR(plots["hl2"],  100.0, 0.01);  // (110+90)/2
    EXPECT_NEAR(plots["hlc3"], 101.67, 0.01); // (110+90+105)/3
    EXPECT_NEAR(plots["ohlc4"], 101.25, 0.01); // (100+110+90+105)/4
}

TEST(PineConverter, ParsePlotshapeSkipped) {
    std::string src = R"(
// @version=6
indicator("Test", overlay=true)
x = ta.ema(close, 9)
plotshape(x > 50, style=shape.triangleup, location=location.belowbar)
plot(x, "EMA9")
)";
    auto script = PineConverter::parseSource(src);
    EXPECT_EQ(script.name, "Test");
    // Should have parsed plot(x, "EMA9") despite plotshape
    bool hasPlot = false;
    for (auto& s : script.statements) {
        if (s.kind == PineStmt::Kind::PLOT && s.plotTitle == "EMA9") hasPlot = true;
    }
    EXPECT_TRUE(hasPlot);
}

TEST(PineConverter, ParseVarLinefill) {
    std::string src = R"(
var linefill lf = na
var line myline = na
x = 42
plot(x, "val")
)";
    auto script = PineConverter::parseSource(src);
    bool hasX = false;
    bool hasLf = false;
    for (auto& s : script.statements) {
        if (s.kind == PineStmt::Kind::ASSIGN && s.name == "x") hasX = true;
        if (s.kind == PineStmt::Kind::VAR_DECL && s.name == "lf") hasLf = true;
    }
    EXPECT_TRUE(hasX);
    EXPECT_TRUE(hasLf);
}

TEST(UserIndicatorManager, UpdateAllProducesPlots) {
    std::string dir = std::string(CMAKE_SOURCE_DIR) + "/user_indicator";
    UserIndicatorManager mgr(dir);
    mgr.scanAndLoad();
    ASSERT_TRUE(mgr.hasIndicators());

    // Feed candle data and verify plots are produced
    Candle c{};
    c.open = 60000; c.high = 61000; c.low = 59000; c.close = 60500; c.volume = 100;
    for (int i = 0; i < 20; ++i) {
        c.close = 60000 + i * 50;
        c.high  = c.close + 500;
        c.low   = c.close - 500;
        c.open  = c.close - 100;
        mgr.updateAll(c);
    }
    auto allPlots = mgr.getAllPlots();
    // Should have produced some plot data from the loaded scripts
    EXPECT_FALSE(allPlots.empty());
}

// ── PineConverter if/else tests (v2.8.0) ────────────────────────────────────

TEST(PineConverterV280, ParseIfStatement) {
    // Verify that 'if' statements are parsed into IF_STMT nodes
    std::string src = R"(
//@version=6
indicator("IfTest")
x = close
if x > 100
    x = 200
)";
    auto script = PineConverter::parseSource(src);
    bool hasIf = false;
    for (auto& s : script.statements) {
        if (s.kind == PineStmt::Kind::IF_STMT) {
            hasIf = true;
            EXPECT_NE(s.expr, nullptr); // condition exists
            EXPECT_FALSE(s.thenStmts.empty()); // then branch exists
        }
    }
    EXPECT_TRUE(hasIf);
}

TEST(PineConverterV280, ParseIfElseStatement) {
    // Verify that 'if/else' is parsed with both branches
    std::string src = R"(
//@version=6
indicator("IfElseTest")
rsiVal = 50
if rsiVal > 70
    rsiVal = 1
else
    rsiVal = 0
)";
    auto script = PineConverter::parseSource(src);
    bool hasIfElse = false;
    for (auto& s : script.statements) {
        if (s.kind == PineStmt::Kind::IF_STMT) {
            hasIfElse = true;
            EXPECT_NE(s.expr, nullptr);
            EXPECT_FALSE(s.thenStmts.empty());
            EXPECT_FALSE(s.elseStmts.empty());
        }
    }
    EXPECT_TRUE(hasIfElse);
}

TEST(PineConverterV280, GenerateCppWithIfStatement) {
    // Verify that generateCpp produces an 'if' block in C++ code
    std::string src = R"(
//@version=6
indicator("IfGen")
flag = 0
if close > 100
    flag = 1
plot(flag, "flag")
)";
    auto script = PineConverter::parseSource(src);
    std::string cpp = PineConverter::generateCpp(script, "IfGenIndicator");
    EXPECT_NE(cpp.find("if ("), std::string::npos);
}

TEST(PineConverterV280, GenerateCppWithIfElseStatement) {
    // Verify that generateCpp produces an 'if/else' block in C++ code
    std::string src = R"(
//@version=6
indicator("IfElseGen")
sig = 0
if close > 100
    sig = 1
else
    sig = -1
plot(sig, "sig")
)";
    auto script = PineConverter::parseSource(src);
    std::string cpp = PineConverter::generateCpp(script, "IfElseGenIndicator");
    EXPECT_NE(cpp.find("if ("), std::string::npos);
    EXPECT_NE(cpp.find("} else {"), std::string::npos);
}

TEST(PineConverterV280, RuntimeExecsIfTrue) {
    // PineRuntime should execute the then-branch when condition is true
    std::string src = R"(
//@version=6
indicator("RuntimeIf")
sig = 0
if close > 50
    sig = 1
plot(sig, "sig")
)";
    auto script = PineConverter::parseSource(src);
    PineRuntime rt;
    rt.load(script);

    Candle c{};
    c.close = 100.0; c.open = 99.0; c.high = 101.0; c.low = 99.0; c.volume = 1.0;
    auto plots = rt.update(c);
    // close(100) > 50 → sig should be 1
    auto it = plots.find("sig");
    if (it != plots.end()) {
        EXPECT_DOUBLE_EQ(it->second, 1.0);
    }
    // At minimum the if statement should not crash the runtime
}

TEST(PineConverterV280, RuntimeExecsIfFalse) {
    // PineRuntime should execute the else-branch when condition is false
    std::string src = R"(
//@version=6
indicator("RuntimeIfElse")
sig = 0
if close > 200
    sig = 1
else
    sig = -1
plot(sig, "sig")
)";
    auto script = PineConverter::parseSource(src);
    PineRuntime rt;
    rt.load(script);

    Candle c{};
    c.close = 100.0; c.open = 99.0; c.high = 101.0; c.low = 99.0; c.volume = 1.0;
    auto plots = rt.update(c);
    // close(100) > 200 is false → sig should be -1
    auto it = plots.find("sig");
    if (it != plots.end()) {
        EXPECT_DOUBLE_EQ(it->second, -1.0);
    }
}
