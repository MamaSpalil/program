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
