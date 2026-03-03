#include <gtest/gtest.h>
#include "../src/ml/FeatureExtractor.h"
#include "../src/indicators/IndicatorEngine.h"
#include <nlohmann/json.hpp>

using namespace crypto;

static IndicatorEngine makeEngine() {
    nlohmann::json cfg;
    cfg["ema_fast"] = 3; cfg["ema_slow"] = 5; cfg["ema_trend"] = 10;
    cfg["rsi_period"] = 5; cfg["atr_period"] = 3;
    cfg["macd_fast"] = 3; cfg["macd_slow"] = 5; cfg["macd_signal"] = 3;
    cfg["bb_period"] = 3; cfg["bb_stddev"] = 2.0;
    cfg["rsi_overbought"] = 70; cfg["rsi_oversold"] = 30;
    return IndicatorEngine(cfg);
}

TEST(FeatureExtractor, FeaturesWhenReady) {
    auto engine = makeEngine();
    FeatureExtractor fe(engine);

    Candle c{};
    for (int i = 1; i <= 30; ++i) {
        c.openTime = static_cast<int64_t>(i) * 60000;
        c.close = 100.0 + i;
        c.high  = c.close + 1.0;
        c.low   = c.close - 1.0;
        c.open  = c.close - 0.3;
        c.volume = 1000.0 + i * 10;
        c.closed = true;
        engine.update(c);
        fe.update(c);
    }

    auto f = fe.extract();
    EXPECT_TRUE(f.valid);
    EXPECT_EQ(static_cast<int>(f.values.size()), FeatureExtractor::FEATURE_DIM);
}

TEST(FeatureExtractor, InsufficientData) {
    auto engine = makeEngine();
    FeatureExtractor fe(engine);

    Candle c{};
    c.close = 100.0; c.high = 101.0; c.low = 99.0; c.open = 100.0;
    fe.update(c);  // only 1 candle

    auto f = fe.extract();
    EXPECT_FALSE(f.valid);
}

TEST(FeatureExtractor, FeatureDimension) {
    EXPECT_EQ(FeatureExtractor::FEATURE_DIM, 60);
}
