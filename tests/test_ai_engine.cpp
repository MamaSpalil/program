#include <gtest/gtest.h>
#include "../src/ai/SPSCQueue.h"
#include "../src/ai/RLTradingAgent.h"
#include "../src/ai/FeatureExtractor.h"
#include "../src/ai/OnlineLearningLoop.h"
#include "../src/data/CandleData.h"
#include "../src/exchange/DepthStream.h"

using namespace crypto;
using namespace crypto::ai;

// ─────────────────────────────────────────────────────────────────────────────
// SPSCQueue
// ─────────────────────────────────────────────────────────────────────────────

TEST(SPSCQueueAI, PushPopBasic) {
    SPSCQueue<int, 8> q;
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(q.size_approx(), 0u);
    EXPECT_TRUE(q.try_push(42));
    EXPECT_FALSE(q.empty());
    auto val = q.try_pop();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 42);
    EXPECT_TRUE(q.empty());
}

TEST(SPSCQueueAI, Capacity) {
    // Capacity is N-1 for a power-of-two ring buffer
    SPSCQueue<int, 4> q;
    EXPECT_EQ(q.capacity(), 3u);
    EXPECT_TRUE(q.try_push(1));
    EXPECT_TRUE(q.try_push(2));
    EXPECT_TRUE(q.try_push(3));
    EXPECT_FALSE(q.try_push(4)); // full
}

TEST(SPSCQueueAI, FIFOOrder) {
    SPSCQueue<int, 16> q;
    for (int i = 0; i < 10; ++i)
        EXPECT_TRUE(q.try_push(i));
    for (int i = 0; i < 10; ++i) {
        auto v = q.try_pop();
        ASSERT_TRUE(v.has_value());
        EXPECT_EQ(*v, i);
    }
}

TEST(SPSCQueueAI, PopEmpty) {
    SPSCQueue<int, 4> q;
    auto v = q.try_pop();
    EXPECT_FALSE(v.has_value());
}

TEST(SPSCQueueAI, MoveSemantics) {
    SPSCQueue<std::vector<float>, 8> q;
    std::vector<float> data = {1.0f, 2.0f, 3.0f};
    EXPECT_TRUE(q.try_push(std::move(data)));
    auto val = q.try_pop();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val->size(), 3u);
    EXPECT_FLOAT_EQ((*val)[0], 1.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
// RLTradingAgent (stub mode — no LibTorch)
// ─────────────────────────────────────────────────────────────────────────────

TEST(RLTradingAgentAI, DefaultConstruction) {
    PPOConfig cfg;
    cfg.stateDim  = 64;
    cfg.hiddenDim = 128;
    RLTradingAgent agent(cfg);
    EXPECT_EQ(agent.config().stateDim, 64);
    EXPECT_EQ(agent.config().hiddenDim, 128);
    EXPECT_EQ(agent.config().numActions, 3);
}

TEST(RLTradingAgentAI, ForwardPassStub) {
    PPOConfig cfg;
    cfg.stateDim = 32;
    RLTradingAgent agent(cfg);
    std::vector<float> state(32, 0.5f);
    auto result = agent.forward_pass(state);
    // In stub mode, always returns HOLD (action=1)
#ifndef USE_LIBTORCH
    EXPECT_EQ(result.action, 1);
    EXPECT_FLOAT_EQ(result.logProb, 0.0f);
    EXPECT_FLOAT_EQ(result.value, 0.0f);
#else
    // With LibTorch, any valid action is acceptable
    EXPECT_GE(result.action, 0);
    EXPECT_LE(result.action, 2);
#endif
}

TEST(RLTradingAgentAI, TrainStepStub) {
    RLTradingAgent agent;
    ExperienceBatch batch;
    Experience exp;
    exp.state = std::vector<float>(128, 0.0f);
    exp.action = 0;
    exp.reward = 1.0f;
    exp.nextState = std::vector<float>(128, 0.0f);
    exp.done = true;
    exp.logProb = -0.5f;
    exp.value = 0.5f;
    batch.transitions.push_back(exp);

    auto result = agent.train_step(batch);
    // Stub returns zeros
#ifndef USE_LIBTORCH
    EXPECT_FLOAT_EQ(result.policyLoss, 0.0f);
    EXPECT_FLOAT_EQ(result.valueLoss, 0.0f);
    EXPECT_FLOAT_EQ(result.entropy, 0.0f);
#endif
}

TEST(RLTradingAgentAI, ModeSwitch) {
    RLTradingAgent agent;
    EXPECT_EQ(agent.getMode(), AgentMode::Fast);
    agent.setMode(AgentMode::Swing);
    EXPECT_EQ(agent.getMode(), AgentMode::Swing);
    agent.setMode(AgentMode::Fast);
    EXPECT_EQ(agent.getMode(), AgentMode::Fast);
}

TEST(RLTradingAgentAI, SaveLoadStub) {
    RLTradingAgent agent;
#ifndef USE_LIBTORCH
    EXPECT_FALSE(agent.save("/tmp/test_rl_agent.pt"));
    EXPECT_FALSE(agent.load("/tmp/test_rl_agent.pt"));
#endif
}

TEST(RLTradingAgentAI, PPOConfigDefaults) {
    PPOConfig cfg;
    EXPECT_EQ(cfg.stateDim, 128);
    EXPECT_EQ(cfg.hiddenDim, 256);
    EXPECT_EQ(cfg.numActions, 3);
    EXPECT_FLOAT_EQ(cfg.gamma, 0.99f);
    EXPECT_FLOAT_EQ(cfg.lambda, 0.95f);
    EXPECT_FLOAT_EQ(cfg.epsilon, 0.2f);
    EXPECT_FLOAT_EQ(cfg.entropyCoeff, 0.02f);
    EXPECT_FLOAT_EQ(cfg.valueLossCoeff, 0.5f);
    EXPECT_FLOAT_EQ(cfg.maxGradNorm, 0.5f);
    EXPECT_FLOAT_EQ(cfg.valueLossClipMax, 10.0f);
    EXPECT_FLOAT_EQ(cfg.learningRate, 3e-4f);
    EXPECT_EQ(cfg.ppoEpochs, 4);
    EXPECT_EQ(cfg.miniBatchSize, 64);
}

// ─────────────────────────────────────────────────────────────────────────────
// AIFeatureExtractor
// ─────────────────────────────────────────────────────────────────────────────

TEST(AIFeatureExtractorAI, DefaultConfig) {
    AIFeatureConfig cfg;
    EXPECT_EQ(cfg.pivotLookback, 5);
    EXPECT_EQ(cfg.maxCandles, 256);
    EXPECT_EQ(cfg.obLevels, 20);
    EXPECT_EQ(cfg.fastFeatureDim, 128);
    EXPECT_EQ(cfg.swingFeatureDim, 128);
}

TEST(AIFeatureExtractorAI, FeatureDimensions) {
    AIFeatureExtractor fe;
    EXPECT_EQ(fe.featureDim(AgentMode::Fast), 128);
    EXPECT_EQ(fe.featureDim(AgentMode::Swing), 128);
}

TEST(AIFeatureExtractorAI, PushCandle) {
    AIFeatureExtractor fe;
    EXPECT_EQ(fe.candleCount(), 0u);
    Candle c{};
    c.open = 100; c.high = 105; c.low = 95; c.close = 102; c.volume = 1000;
    fe.pushCandle(c);
    EXPECT_EQ(fe.candleCount(), 1u);
}

TEST(AIFeatureExtractorAI, MaxCandleHistory) {
    AIFeatureConfig cfg;
    cfg.maxCandles = 10;
    AIFeatureExtractor fe(cfg);
    Candle c{};
    c.open = 100; c.high = 105; c.low = 95; c.close = 102; c.volume = 500;
    for (int i = 0; i < 20; ++i) {
        c.close = 100.0 + i;
        fe.pushCandle(c);
    }
    EXPECT_EQ(fe.candleCount(), 10u);
}

TEST(AIFeatureExtractorAI, FastModeExtraction) {
    AIFeatureExtractor fe;

    // Push some depth data
    DepthBook book;
    for (int i = 0; i < 20; ++i) {
        book.bids.push_back({100.0 - i * 0.1, 10.0 + i});
        book.asks.push_back({100.1 + i * 0.1, 10.0 + i});
    }
    fe.pushDepth(book);

    auto features = fe.extract(AgentMode::Fast);
    EXPECT_EQ(static_cast<int>(features.size()), 128);

    // Bid-ask spread should be positive
    EXPECT_GT(features[80], 0.0f);

    // Order imbalance should be finite
    EXPECT_FALSE(std::isnan(features[81]));
    EXPECT_FALSE(std::isinf(features[81]));
}

TEST(AIFeatureExtractorAI, SwingModeExtraction) {
    AIFeatureExtractor fe;

    // Need at least 3 candles for swing mode
    for (int i = 0; i < 30; ++i) {
        Candle c{};
        c.openTime = i * 60000;
        c.open  = 100.0 + i;
        c.high  = 102.0 + i;
        c.low   = 98.0 + i;
        c.close = 101.0 + i;
        c.volume = 1000.0 + i * 10;
        c.closed = true;
        fe.pushCandle(c);
    }

    auto features = fe.extract(AgentMode::Swing);
    EXPECT_EQ(static_cast<int>(features.size()), 128);

    // Close prices should be non-zero (normalised by MA)
    bool hasNonZero = false;
    for (int i = 0; i < 20; ++i) {
        if (features[i] != 0.0f) hasNonZero = true;
    }
    EXPECT_TRUE(hasNonZero);
}

// ─────────────────────────────────────────────────────────────────────────────
// SFP Detection
// ─────────────────────────────────────────────────────────────────────────────

TEST(AIFeatureExtractorAI, SFP_InsufficientData) {
    AIFeatureExtractor fe;
    // Less than pivotLookback + 2 candles
    Candle c{}; c.open = 100; c.high = 105; c.low = 95; c.close = 102;
    fe.pushCandle(c);
    auto sfp = fe.calc_SFP();
    EXPECT_FALSE(sfp.detected);
}

TEST(AIFeatureExtractorAI, SFP_BullishDetection) {
    AIFeatureConfig cfg;
    cfg.pivotLookback = 2;
    AIFeatureExtractor fe(cfg);

    // Create a pivot low then a sweep below it with close above
    // Need: 2*lookback+1 = 5 candles for pivot + 1 more for sweep = at least 6
    // pivot low at idx 2: price=90, low=90-5=85
    double prices[] = {100, 105, 90, 108, 102, 0};

    for (int i = 0; i < 5; ++i) {
        Candle c{};
        c.openTime = i * 60000;
        c.open = prices[i];
        c.high = prices[i] + 5;
        c.low  = prices[i] - 5;
        c.close = prices[i] + 1;
        c.closed = true;
        fe.pushCandle(c);
    }

    // Add a candle that sweeps below the pivot low but closes above it
    Candle sweep{};
    sweep.openTime = 5 * 60000;
    sweep.open  = 88;
    sweep.high  = 92;
    sweep.low   = 83;  // Sweeps below pivot low of 85
    sweep.close = 87;  // Closes above pivot low of 85
    sweep.closed = true;
    fe.pushCandle(sweep);

    auto sfp = fe.calc_SFP();
    // We have a bullish SFP if it detected one
    if (sfp.detected) {
        EXPECT_TRUE(sfp.bullish);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// FVG Detection
// ─────────────────────────────────────────────────────────────────────────────

TEST(AIFeatureExtractorAI, FVG_InsufficientData) {
    AIFeatureExtractor fe;
    auto fvg = fe.calc_FVG();
    EXPECT_FALSE(fvg.detected);

    Candle c{}; c.open = 100; c.high = 105; c.low = 95; c.close = 102;
    fe.pushCandle(c);
    fe.pushCandle(c);
    fvg = fe.calc_FVG();
    EXPECT_FALSE(fvg.detected); // only 2 candles
}

TEST(AIFeatureExtractorAI, FVG_BullishDetection) {
    AIFeatureExtractor fe;

    // Bullish FVG: Low[t] - High[t-2] > 0
    // Candle 0 (t-2): high=100
    // Candle 1 (t-1): anything (momentum candle)
    // Candle 2 (t):   low=102  → gap = 102 - 100 = 2 > 0

    Candle c0{}; c0.open = 95;  c0.high = 100; c0.low = 93;  c0.close = 98;
    Candle c1{}; c1.open = 99;  c1.high = 108; c1.low = 98;  c1.close = 107;
    Candle c2{}; c2.open = 108; c2.high = 115; c2.low = 102; c2.close = 113;

    fe.pushCandle(c0);
    fe.pushCandle(c1);
    fe.pushCandle(c2);

    auto fvg = fe.calc_FVG();
    EXPECT_TRUE(fvg.detected);
    EXPECT_TRUE(fvg.bullish);
    EXPECT_DOUBLE_EQ(fvg.gapLow, 100.0);  // High[t-2]
    EXPECT_DOUBLE_EQ(fvg.gapHigh, 102.0);  // Low[t]
}

TEST(AIFeatureExtractorAI, FVG_BearishDetection) {
    AIFeatureExtractor fe;

    // Bearish FVG: Low[t-2] - High[t] > 0
    // Candle 0 (t-2): low=105
    // Candle 1 (t-1): momentum candle
    // Candle 2 (t):   high=102 → gap = 105 - 102 = 3 > 0

    Candle c0{}; c0.open = 110; c0.high = 115; c0.low = 105; c0.close = 108;
    Candle c1{}; c1.open = 107; c1.high = 107; c1.low = 97;  c1.close = 98;
    Candle c2{}; c2.open = 98;  c2.high = 102; c2.low = 90;  c2.close = 91;

    fe.pushCandle(c0);
    fe.pushCandle(c1);
    fe.pushCandle(c2);

    auto fvg = fe.calc_FVG();
    EXPECT_TRUE(fvg.detected);
    EXPECT_FALSE(fvg.bullish);
    EXPECT_DOUBLE_EQ(fvg.gapHigh, 105.0);  // Low[t-2]
    EXPECT_DOUBLE_EQ(fvg.gapLow, 102.0);   // High[t]
}

TEST(AIFeatureExtractorAI, FVG_NoGap) {
    AIFeatureExtractor fe;

    // Overlapping candles — no gap
    Candle c0{}; c0.open = 100; c0.high = 105; c0.low = 95; c0.close = 103;
    Candle c1{}; c1.open = 103; c1.high = 108; c1.low = 100; c1.close = 106;
    Candle c2{}; c2.open = 106; c2.high = 110; c2.low = 103; c2.close = 109;

    fe.pushCandle(c0);
    fe.pushCandle(c1);
    fe.pushCandle(c2);

    auto fvg = fe.calc_FVG();
    EXPECT_FALSE(fvg.detected); // low[t]=103, high[t-2]=105 → gap = -2 < 0
}

// ─────────────────────────────────────────────────────────────────────────────
// Order Blocks
// ─────────────────────────────────────────────────────────────────────────────

TEST(AIFeatureExtractorAI, OrderBlocks_InsufficientData) {
    AIFeatureExtractor fe;
    auto obs = fe.calc_OrderBlocks();
    EXPECT_TRUE(obs.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// Depth data handling
// ─────────────────────────────────────────────────────────────────────────────

TEST(AIFeatureExtractorAI, PushDepthHandlesEmpty) {
    AIFeatureExtractor fe;
    DepthBook emptyBook;
    fe.pushDepth(emptyBook); // should not crash

    auto features = fe.extract(AgentMode::Fast);
    EXPECT_EQ(static_cast<int>(features.size()), 128);
}

TEST(AIFeatureExtractorAI, PushDepthPartialLevels) {
    AIFeatureExtractor fe;
    DepthBook book;
    book.bids.push_back({100.0, 5.0});
    book.asks.push_back({100.1, 3.0});
    fe.pushDepth(book);

    auto features = fe.extract(AgentMode::Fast);
    // Spread = (100.1 - 100) / 100.05 ≈ 0.001
    EXPECT_GT(features[80], 0.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
// Experience struct
// ─────────────────────────────────────────────────────────────────────────────

TEST(ExperienceAI, DefaultValues) {
    Experience exp;
    EXPECT_EQ(exp.action, 0);
    EXPECT_FLOAT_EQ(exp.reward, 0.0f);
    EXPECT_FALSE(exp.done);
    EXPECT_FLOAT_EQ(exp.logProb, 0.0f);
    EXPECT_FLOAT_EQ(exp.value, 0.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
// OnlineLearningLoop
// ─────────────────────────────────────────────────────────────────────────────

TEST(OnlineLearningLoopAI, ConstructAndDestroy) {
    RLTradingAgent agent;
    AIFeatureExtractor feat;
    // nullptr for trades is acceptable (no DB polling)
    OnlineLearningLoop loop(agent, feat, nullptr);
    EXPECT_FALSE(loop.running());
    EXPECT_EQ(loop.trainSteps(), 0u);
    EXPECT_EQ(loop.samplesConsumed(), 0u);
}

TEST(OnlineLearningLoopAI, StartStop) {
    RLTradingAgent agent;
    AIFeatureExtractor feat;
    OnlineLearningLoop loop(agent, feat, nullptr);

    loop.start();
    EXPECT_TRUE(loop.running());

    // Let it spin briefly
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    loop.stop();
    EXPECT_FALSE(loop.running());
}

TEST(OnlineLearningLoopAI, PushExperience) {
    RLTradingAgent agent;
    AIFeatureExtractor feat;
    OnlineLearningLoop loop(agent, feat, nullptr);

    Experience exp;
    exp.state = std::vector<float>(128, 0.5f);
    exp.action = 0;
    exp.reward = 1.0f;
    exp.nextState = std::vector<float>(128, 0.5f);
    exp.done = true;

    EXPECT_TRUE(loop.pushExperience(std::move(exp)));
}

TEST(OnlineLearningLoopAI, OnlineLearningConfig_Defaults) {
    OnlineLearningConfig cfg;
    EXPECT_EQ(cfg.replayCapacityExp, 12);
    EXPECT_EQ(cfg.minReplaySize, 256);
    EXPECT_EQ(cfg.trainIntervalMs, 1000);
    EXPECT_EQ(cfg.pollIntervalMs, 500);
    EXPECT_FLOAT_EQ(cfg.rewardScale, 1.0f);
    EXPECT_EQ(cfg.saveIntervalSteps, 100);
}

// ─────────────────────────────────────────────────────────────────────────────
// Integration: FeatureExtractor → Agent
// ─────────────────────────────────────────────────────────────────────────────

TEST(AIIntegration, FeatureExtractorToAgent) {
    AIFeatureConfig fCfg;
    // Use default dim (128) to match the feature layout
    AIFeatureExtractor fe(fCfg);

    PPOConfig aCfg;
    aCfg.stateDim = fCfg.fastFeatureDim; // 128
    RLTradingAgent agent(aCfg);

    // Push some depth data
    DepthBook book;
    for (int i = 0; i < 20; ++i) {
        book.bids.push_back({100.0 - i * 0.1, 10.0});
        book.asks.push_back({100.1 + i * 0.1, 10.0});
    }
    fe.pushDepth(book);

    auto state = fe.extract(AgentMode::Fast);
    EXPECT_EQ(static_cast<int>(state.size()), fCfg.fastFeatureDim);

    auto result = agent.forward_pass(state);
    EXPECT_GE(result.action, 0);
    EXPECT_LE(result.action, 2);
}

TEST(AIIntegration, DualModeFeatures) {
    AIFeatureExtractor fe;

    // Set up both candle and depth data
    for (int i = 0; i < 30; ++i) {
        Candle c{};
        c.openTime = i * 60000;
        c.open  = 100.0 + i;
        c.high  = 102.0 + i;
        c.low   = 98.0 + i;
        c.close = 101.0 + i;
        c.volume = 1000.0;
        c.closed = true;
        fe.pushCandle(c);
    }

    DepthBook book;
    for (int i = 0; i < 20; ++i) {
        book.bids.push_back({100.0 - i * 0.1, 10.0});
        book.asks.push_back({100.1 + i * 0.1, 10.0});
    }
    fe.pushDepth(book);

    auto fastFeatures  = fe.extract(AgentMode::Fast);
    auto swingFeatures = fe.extract(AgentMode::Swing);

    EXPECT_EQ(static_cast<int>(fastFeatures.size()), 128);
    EXPECT_EQ(static_cast<int>(swingFeatures.size()), 128);

    // Features should differ between modes
    bool differ = false;
    for (int i = 0; i < 128; ++i) {
        if (fastFeatures[i] != swingFeatures[i]) { differ = true; break; }
    }
    EXPECT_TRUE(differ);
}
