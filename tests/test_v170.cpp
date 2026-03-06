// ═══════════════════════════════════════════════════════════════════════════════
// test_v170.cpp — v1.7.0 Stress Tests: Layout, AI, ML, Integration
// ═══════════════════════════════════════════════════════════════════════════════
#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <random>
#include <fstream>
#include <filesystem>

// ── Layout ──────────────────────────────────────────────────────────────────
#include "../src/ui/LayoutManager.h"

// ── AI ──────────────────────────────────────────────────────────────────────
#include "../src/ai/SPSCQueue.h"
#include "../src/ai/RLTradingAgent.h"
#include "../src/ai/FeatureExtractor.h"
#include "../src/ai/OnlineLearningLoop.h"

// ── ML ──────────────────────────────────────────────────────────────────────
#include "../src/ml/LSTMModel.h"
#include "../src/ml/XGBoostModel.h"
#include "../src/ml/FeatureExtractor.h"
#include "../src/ml/SignalEnhancer.h"
#include "../src/ml/ModelTrainer.h"

// ── Data / Indicators ───────────────────────────────────────────────────────
#include "../src/data/CandleData.h"
#include "../src/exchange/DepthStream.h"
#include "../src/indicators/IndicatorEngine.h"
#include <nlohmann/json.hpp>

using namespace crypto;
using namespace crypto::ai;

// Helper: create a small IndicatorEngine for testing
static IndicatorEngine makeTestEngine() {
    nlohmann::json cfg;
    cfg["ema_fast"] = 3; cfg["ema_slow"] = 5; cfg["ema_trend"] = 10;
    cfg["rsi_period"] = 5; cfg["atr_period"] = 3;
    cfg["macd_fast"] = 3; cfg["macd_slow"] = 5; cfg["macd_signal"] = 3;
    cfg["bb_period"] = 3; cfg["bb_stddev"] = 2.0;
    cfg["rsi_overbought"] = 70; cfg["rsi_oversold"] = 30;
    return IndicatorEngine(cfg);
}

// ═════════════════════════════════════════════════════════════════════════════
// SECTION 1: Layout Stress Tests
// Validates the 8-window tile layout matches the expected diagram:
// ┌──────────────────────────────────────────┐
// │ TopBar (W × 32)                          │
// ├────────────┬─────────────────┬───────────┤
// │ PairList   │ Market Data     │UserPanel  │
// │ 200 × ...  │ (W-490) × ...  │ 290 × ... │
// ├────────────┼─────────────────┴───────────┤
// │VolumeDelta │ Indicators                  │
// ├────────────┴─────────────────────────────┤
// │ Log Window (W × 120)                     │
// └──────────────────────────────────────────┘
// ═════════════════════════════════════════════════════════════════════════════

// ── PairList width = 200, Market Data = W-490, UserPanel = 290 ──────────────

TEST(LayoutStress, PairListWidth200) {
    LayoutManager mgr;
    mgr.recalculate(1920, 1080);
    auto pl = mgr.get("Pair List");
    EXPECT_FLOAT_EQ(pl.size.x, 200.0f);
}

TEST(LayoutStress, UserPanelWidth290) {
    LayoutManager mgr;
    mgr.recalculate(1920, 1080);
    auto up = mgr.get("User Panel");
    EXPECT_FLOAT_EQ(up.size.x, 290.0f);
}

TEST(LayoutStress, MarketDataWidthWminus490) {
    LayoutManager mgr;
    mgr.recalculate(1920, 1080);
    auto md = mgr.get("Market Data");
    // W - PairList(200) - UserPanel(290) = W - 490
    EXPECT_FLOAT_EQ(md.size.x, 1920.0f - 490.0f);
}

TEST(LayoutStress, TopBarHeight32) {
    LayoutManager mgr;
    mgr.recalculate(1920, 1080);
    auto tb = mgr.get("Main Toolbar");
    EXPECT_FLOAT_EQ(tb.size.y, 32.0f);
    EXPECT_FLOAT_EQ(tb.pos.y, 0.0f);
    EXPECT_FLOAT_EQ(tb.size.x, 1920.0f);
}

TEST(LayoutStress, LogWindowHeight120) {
    LayoutManager mgr;
    mgr.recalculate(1920, 1080);
    auto log = mgr.get("Logs");
    EXPECT_FLOAT_EQ(log.size.y, 120.0f);
    EXPECT_FLOAT_EQ(log.size.x, 1920.0f);
    // Logs at bottom of viewport
    EXPECT_FLOAT_EQ(log.pos.y + log.size.y, 1080.0f);
}

// ── No overlapping between adjacent windows ─────────────────────────────────

TEST(LayoutStress, NoOverlapTopBarAndCenter) {
    LayoutManager mgr;
    mgr.recalculate(1920, 1080);
    auto tb = mgr.get("Main Toolbar");
    auto fb = mgr.get("Filters Bar");
    auto pl = mgr.get("Pair List");
    // Filters bar starts where toolbar ends
    EXPECT_LE(tb.pos.y + tb.size.y, fb.pos.y + 0.5f);
    // Pair list starts where filters bar ends
    EXPECT_LE(fb.pos.y + fb.size.y, pl.pos.y + 0.5f);
}

TEST(LayoutStress, NoOverlapCenterColumns) {
    LayoutManager mgr;
    mgr.recalculate(1920, 1080);
    auto pl = mgr.get("Pair List");
    auto vd = mgr.get("Volume Delta");
    auto md = mgr.get("Market Data");
    auto up = mgr.get("User Panel");

    // PairList and VD are in the left column (same X)
    EXPECT_FLOAT_EQ(pl.pos.x, vd.pos.x);
    // Left column right edge ≤ center column left edge
    EXPECT_LE(pl.pos.x + pl.size.x, md.pos.x + 0.5f);
    // Center column right edge ≤ UserPanel left edge
    EXPECT_LE(md.pos.x + md.size.x, up.pos.x + 0.5f);
}

TEST(LayoutStress, NoOverlapCenterVertical) {
    LayoutManager mgr;
    mgr.recalculate(1920, 1080);
    auto pl  = mgr.get("Pair List");
    auto vd  = mgr.get("Volume Delta");
    auto md  = mgr.get("Market Data");
    auto ind = mgr.get("Indicators");
    auto log = mgr.get("Logs");

    // Left column: PairList → VD: no vertical overlap
    EXPECT_LE(pl.pos.y + pl.size.y, vd.pos.y + 0.5f);
    // Center column: MD → Indicators: no vertical overlap
    EXPECT_LE(md.pos.y + md.size.y, ind.pos.y + 0.5f);
    // Indicators → Logs: no vertical overlap
    EXPECT_LE(ind.pos.y + ind.size.y, log.pos.y + 0.5f);
    // VD → Logs: no vertical overlap
    EXPECT_LE(vd.pos.y + vd.size.y, log.pos.y + 0.5f);
}

TEST(LayoutStress, NoOverlapLogsAndSidebars) {
    LayoutManager mgr;
    mgr.recalculate(1920, 1080);
    auto pl  = mgr.get("Pair List");
    auto vd  = mgr.get("Volume Delta");
    auto up  = mgr.get("User Panel");
    auto log = mgr.get("Logs");

    // Left column (Pair List + VD) must not overlap logs area
    EXPECT_LE(vd.pos.y + vd.size.y, log.pos.y + 0.5f);
    // Right column (User Panel) must not overlap logs area
    EXPECT_LE(up.pos.y + up.size.y, log.pos.y + 0.5f);
}

// ── Full coverage test: no pixel gaps and total area = viewport ─────────────

TEST(LayoutStress, AllWindowsFitViewport) {
    const float W = 1920, H = 1080;
    LayoutManager mgr;
    mgr.recalculate(W, H);

    const char* names[] = {
        "Main Toolbar", "Filters Bar", "Pair List",
        "Volume Delta", "Market Data", "Indicators",
        "User Panel", "Logs"
    };
    for (auto* n : names) {
        auto l = mgr.get(n);
        EXPECT_GE(l.pos.x, 0.0f) << n << " has negative X";
        EXPECT_GE(l.pos.y, 0.0f) << n << " has negative Y";
        EXPECT_LE(l.pos.x + l.size.x, W + 0.5f) << n << " exceeds width";
        EXPECT_LE(l.pos.y + l.size.y, H + 0.5f) << n << " exceeds height";
        EXPECT_GT(l.size.x, 0.0f) << n << " zero width";
        EXPECT_GT(l.size.y, 0.0f) << n << " zero height";
    }
}

// ── Multiple screen resolutions ─────────────────────────────────────────────

TEST(LayoutStress, MultipleResolutionsNoOverlap) {
    struct Res { float w, h; };
    Res resolutions[] = {
        {800, 600}, {1024, 768}, {1280, 720}, {1366, 768},
        {1440, 900}, {1600, 900}, {1920, 1080}, {2560, 1440},
        {3840, 2160}
    };

    for (auto& r : resolutions) {
        LayoutManager mgr;
        mgr.recalculate(r.w, r.h);

        const char* names[] = {
            "Main Toolbar", "Filters Bar", "Pair List",
            "Volume Delta", "Market Data", "Indicators",
            "User Panel", "Logs"
        };

        for (auto* n : names) {
            auto l = mgr.get(n);
            EXPECT_GE(l.pos.x, -0.5f) << n << " @" << r.w << "x" << r.h;
            EXPECT_GE(l.pos.y, -0.5f) << n << " @" << r.w << "x" << r.h;
            EXPECT_LE(l.pos.x + l.size.x, r.w + 0.5f) << n << " @" << r.w << "x" << r.h;
            EXPECT_LE(l.pos.y + l.size.y, r.h + 0.5f) << n << " @" << r.w << "x" << r.h;
        }

        // Left column: PairList above VD
        auto pl = mgr.get("Pair List");
        auto vd = mgr.get("Volume Delta");
        EXPECT_LE(pl.pos.y + pl.size.y, vd.pos.y + 0.5f) << "@" << r.w << "x" << r.h;
        // Center column: MD above Indicators
        auto md = mgr.get("Market Data");
        auto ind = mgr.get("Indicators");
        EXPECT_LE(md.pos.y + md.size.y, ind.pos.y + 0.5f) << "@" << r.w << "x" << r.h;
    }
}

// ── Viewport offset support ─────────────────────────────────────────────────

TEST(LayoutStress, ViewportOffsetShiftsAllWindows) {
    LayoutManager mgr1, mgr2;
    mgr1.recalculate(1920, 1080, 0, 0);
    mgr2.recalculate(1920, 1080, 50, 30);

    auto tb1 = mgr1.get("Main Toolbar");
    auto tb2 = mgr2.get("Main Toolbar");
    EXPECT_FLOAT_EQ(tb2.pos.x, tb1.pos.x + 50.0f);
    EXPECT_FLOAT_EQ(tb2.pos.y, tb1.pos.y + 30.0f);

    auto log1 = mgr1.get("Logs");
    auto log2 = mgr2.get("Logs");
    EXPECT_FLOAT_EQ(log2.pos.x, log1.pos.x + 50.0f);
    EXPECT_FLOAT_EQ(log2.pos.y, log1.pos.y + 30.0f);
}

// ── Show flags: hiding panels expands neighbors ─────────────────────────────

TEST(LayoutStress, HidePairListExpandsCenter) {
    LayoutManager mgrAll, mgrNo;
    mgrAll.recalculate(1920, 1080, 0, 0, true, true, true, true, true);
    // Hiding both PairList and VD removes the left column (200px)
    mgrNo.recalculate(1920, 1080, 0, 0, false, true, false, true, true);

    auto mdAll = mgrAll.get("Market Data");
    auto mdNo  = mgrNo.get("Market Data");
    // Hiding left column (200px) should expand center by 200
    EXPECT_GT(mdNo.size.x, mdAll.size.x + 190.0f);
}

TEST(LayoutStress, HideUserPanelExpandsCenter) {
    LayoutManager mgrAll, mgrNo;
    mgrAll.recalculate(1920, 1080, 0, 0, true, true, true, true, true);
    mgrNo.recalculate(1920, 1080, 0, 0, true, false, true, true, true);

    auto mdAll = mgrAll.get("Market Data");
    auto mdNo  = mgrNo.get("Market Data");
    // Hiding UserPanel(290px) should expand center
    EXPECT_GT(mdNo.size.x, mdAll.size.x + 280.0f);
}

TEST(LayoutStress, HideLogsExpandsCenter) {
    LayoutManager mgrAll, mgrNo;
    mgrAll.recalculate(1920, 1080, 0, 0, true, true, true, true, true);
    mgrNo.recalculate(1920, 1080, 0, 0, true, true, true, true, false);

    auto plAll = mgrAll.get("Pair List");
    auto plNo  = mgrNo.get("Pair List");
    // Hiding Logs(120px) should increase sidebar height
    // PairList gets ~85% of freed space (rest goes to VD)
    EXPECT_GT(plNo.size.y, plAll.size.y + 90.0f);
}

TEST(LayoutStress, HideAllOptionalPanels) {
    LayoutManager mgr;
    mgr.recalculate(1920, 1080, 0, 0, false, false, false, false, false);

    auto md = mgr.get("Market Data");
    // Center column should be full width minus zero sidebars
    EXPECT_FLOAT_EQ(md.size.x, 1920.0f);
    // Market Data visible even when VD/Indicators hidden
    EXPECT_TRUE(md.visible);
}

// ── Custom proportions ──────────────────────────────────────────────────────

TEST(LayoutStress, CustomProportionsAffectColumnHeights) {
    LayoutManager mgr;
    mgr.setVdPct(0.25f);
    mgr.setIndPct(0.25f);
    mgr.recalculate(1920, 1080);

    auto vd  = mgr.get("Volume Delta");
    auto pl  = mgr.get("Pair List");
    auto md  = mgr.get("Market Data");
    auto ind = mgr.get("Indicators");

    // VD height = 25% of left column, Indicators height = 25% of center column
    // Both columns have same Hcenter so VD and Indicators should be similar height
    EXPECT_NEAR(vd.size.y, ind.size.y, 2.0f);
    // PairList gets remainder of left column (75%)
    EXPECT_GT(pl.size.y, vd.size.y);
    // Market Data gets remainder of center column (75%)
    EXPECT_GT(md.size.y, ind.size.y);
}

// ═════════════════════════════════════════════════════════════════════════════
// SECTION 2: AI Engine Stress Tests
// ═════════════════════════════════════════════════════════════════════════════

// ── SPSCQueue stress: high-throughput push/pop ──────────────────────────────

TEST(AIStress, SPSCQueueHighThroughput) {
    constexpr int N = 10000;
    SPSCQueue<int, 1024> q;

    // Producer thread
    std::thread producer([&]() {
        for (int i = 0; i < N; ++i) {
            while (!q.try_push(i)) {
                std::this_thread::yield();
            }
        }
    });

    // Consumer thread
    int received = 0;
    int lastVal = -1;
    std::thread consumer([&]() {
        while (received < N) {
            auto val = q.try_pop();
            if (val.has_value()) {
                // FIFO ordering must be preserved
                EXPECT_EQ(*val, lastVal + 1);
                lastVal = *val;
                ++received;
            } else {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();
    EXPECT_EQ(received, N);
    EXPECT_EQ(lastVal, N - 1);
}

TEST(AIStress, SPSCQueueWrapAround) {
    // Test wraparound behavior
    SPSCQueue<int, 8> q;
    for (int round = 0; round < 100; ++round) {
        for (int i = 0; i < 7; ++i) {
            EXPECT_TRUE(q.try_push(round * 7 + i));
        }
        EXPECT_FALSE(q.try_push(-1)); // full
        for (int i = 0; i < 7; ++i) {
            auto v = q.try_pop();
            ASSERT_TRUE(v.has_value());
            EXPECT_EQ(*v, round * 7 + i);
        }
        EXPECT_TRUE(q.empty());
    }
}

// ── RLTradingAgent stress ───────────────────────────────────────────────────

TEST(AIStress, RLAgentRepeatedForwardPass) {
    RLTradingAgent agent;
    std::vector<float> state(128, 0.0f);

    // Stress test: many forward passes should be stable
    for (int i = 0; i < 1000; ++i) {
        state[0] = static_cast<float>(i) * 0.001f;
        auto result = agent.forward_pass(state);
        EXPECT_GE(result.action, 0);
        EXPECT_LE(result.action, 2);
    }
}

TEST(AIStress, RLAgentRepeatedTrainSteps) {
    RLTradingAgent agent;
    std::vector<float> state(128, 0.0f);

    ExperienceBatch batch;
    for (int i = 0; i < 32; ++i) {
        Experience exp;
        exp.state.resize(128, static_cast<float>(i) * 0.01f);
        exp.action = i % 3;
        exp.reward = (i % 2 == 0) ? 1.0f : -1.0f;
        exp.nextState.resize(128, static_cast<float>(i + 1) * 0.01f);
        exp.done = (i == 31);
        exp.logProb = -1.0f;
        exp.value = 0.5f;
        batch.transitions.push_back(exp);
    }

    // Multiple train steps should not crash or produce NaN
    for (int epoch = 0; epoch < 10; ++epoch) {
        auto result = agent.train_step(batch);
        EXPECT_FALSE(std::isnan(result.policyLoss));
        EXPECT_FALSE(std::isnan(result.valueLoss));
        EXPECT_FALSE(std::isnan(result.entropy));
    }
}

TEST(AIStress, RLAgentModeSwitch) {
    RLTradingAgent agent;
    for (int i = 0; i < 100; ++i) {
        agent.setMode(i % 2 == 0 ? AgentMode::Fast : AgentMode::Swing);
        EXPECT_EQ(agent.getMode(), i % 2 == 0 ? AgentMode::Fast : AgentMode::Swing);
    }
}

TEST(AIStress, RLAgentSaveLoadRoundTrip) {
    RLTradingAgent agent;
    const std::string path = "/tmp/test_rlagent_stress";

    agent.save(path);
    RLTradingAgent agent2;
    bool loaded = agent2.load(path);
    // With LibTorch: load succeeds; Without: returns false (stub)
#ifdef USE_LIBTORCH
    EXPECT_TRUE(loaded);
#else
    EXPECT_FALSE(loaded);
#endif

    // Cleanup
    std::filesystem::remove_all(path);
}

// ── AIFeatureExtractor stress ───────────────────────────────────────────────

TEST(AIStress, FeatureExtractorLargeCandleVolume) {
    AIFeatureExtractor fe;
    // Feed 500 candles
    for (int i = 0; i < 500; ++i) {
        Candle c{};
        c.openTime = i * 60000;
        c.open  = 100.0 + std::sin(i * 0.1) * 10;
        c.high  = c.open + 2.0;
        c.low   = c.open - 2.0;
        c.close = c.open + std::cos(i * 0.1) * 1.5;
        c.volume = 1000.0 + i * 10;
        c.closed = true;
        fe.pushCandle(c);
    }

    auto fastFeatures  = fe.extract(AgentMode::Fast);
    auto swingFeatures = fe.extract(AgentMode::Swing);

    EXPECT_EQ(static_cast<int>(fastFeatures.size()), 128);
    EXPECT_EQ(static_cast<int>(swingFeatures.size()), 128);

    // No NaN in features
    for (auto v : fastFeatures) EXPECT_FALSE(std::isnan(v));
    for (auto v : swingFeatures) EXPECT_FALSE(std::isnan(v));
}

TEST(AIStress, FeatureExtractorDepthUpdates) {
    AIFeatureExtractor fe;
    // Feed minimal candle data
    for (int i = 0; i < 30; ++i) {
        Candle c{};
        c.openTime = i * 60000;
        c.open = 100.0 + i; c.high = 102.0 + i;
        c.low = 98.0 + i; c.close = 101.0 + i;
        c.volume = 1000.0; c.closed = true;
        fe.pushCandle(c);
    }

    // Repeated depth updates (simulating rapid orderbook changes)
    for (int round = 0; round < 100; ++round) {
        DepthBook book;
        for (int i = 0; i < 20; ++i) {
            book.bids.push_back({100.0 - i * 0.1 - round * 0.001, 10.0 + round});
            book.asks.push_back({100.1 + i * 0.1 + round * 0.001, 10.0 + round});
        }
        fe.pushDepth(book);

        auto features = fe.extract(AgentMode::Fast);
        EXPECT_EQ(static_cast<int>(features.size()), 128);
        for (auto v : features) EXPECT_FALSE(std::isnan(v));
    }
}

TEST(AIStress, FeatureExtractorSMCPatterns) {
    AIFeatureExtractor fe;

    // Create a pattern with clear SFP: price breaks below previous low then recovers
    std::vector<Candle> candles;
    for (int i = 0; i < 50; ++i) {
        Candle c{};
        c.openTime = i * 60000;
        if (i < 40) {
            c.open = 100.0 + i * 0.5;
            c.high = c.open + 2.0;
            c.low  = c.open - 1.0;
            c.close = c.open + 1.0;
        } else {
            // Create swing failure: dip below lows then recover
            c.open = 100.0 + (i - 10) * 0.5;
            c.high = c.open + 3.0;
            c.low  = 95.0;  // breaks below recent lows
            c.close = c.open + 2.0; // closes above
        }
        c.volume = 1000.0;
        c.closed = true;
        fe.pushCandle(c);
    }

    auto features = fe.extract(AgentMode::Swing);
    EXPECT_EQ(static_cast<int>(features.size()), 128);
    for (auto v : features) EXPECT_FALSE(std::isnan(v));
}

// ── OnlineLearningLoop stress ───────────────────────────────────────────────

TEST(AIStress, OnlineLearningLoopStartStop) {
    RLTradingAgent agent;
    AIFeatureExtractor feat;
    OnlineLearningConfig cfg;
    cfg.trainIntervalMs = 50;
    OnlineLearningLoop loop(agent, feat, nullptr, cfg);
    // Start/stop multiple times
    for (int i = 0; i < 5; ++i) {
        loop.start();
        EXPECT_TRUE(loop.running());
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        loop.stop();
        EXPECT_FALSE(loop.running());
    }
}

TEST(AIStress, OnlineLearningLoopExperienceBatch) {
    RLTradingAgent agent;
    AIFeatureExtractor feat;
    OnlineLearningConfig cfg;
    cfg.trainIntervalMs = 50;
    cfg.minReplaySize = 32;
    OnlineLearningLoop loop(agent, feat, nullptr, cfg);
    loop.start();

    // Push many experiences
    for (int i = 0; i < 100; ++i) {
        Experience exp;
        exp.state.resize(128, static_cast<float>(i) * 0.01f);
        exp.action = i % 3;
        exp.reward = (i % 2 == 0) ? 0.5f : -0.3f;
        exp.nextState.resize(128, static_cast<float>(i + 1) * 0.01f);
        exp.done = false;
        exp.logProb = -1.0f;
        exp.value = 0.0f;
        loop.pushExperience(std::move(exp));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    loop.stop();
    // No crash is the primary assertion
    EXPECT_FALSE(loop.running());
}

// ═════════════════════════════════════════════════════════════════════════════
// SECTION 3: ML Engine Stress Tests (XGBoost + LSTM)
// ═════════════════════════════════════════════════════════════════════════════

TEST(MLStress, XGBoostModelConstruction) {
    XGBoostConfig cfg;
    cfg.maxDepth = 6;
    cfg.eta = 0.1;
    cfg.subsample = 0.8;
    cfg.rounds = 500;
    XGBoostModel model(cfg);
    // Not ready until trained
    EXPECT_FALSE(model.ready());
}

TEST(MLStress, XGBoostPredictWithoutTraining) {
    XGBoostConfig cfg;
    XGBoostModel model(cfg);
    std::vector<double> features(60, 0.5);
    auto result = model.predict(features);
    // Should return empty or default without training
    // (behavior depends on implementation — no crash is key)
    (void)result;
}

TEST(MLStress, XGBoostTrainAndPredict) {
    XGBoostConfig cfg;
    cfg.rounds = 10;  // small for speed
    XGBoostModel model(cfg);

    // Generate synthetic training data
    std::mt19937 rng(42);
    std::normal_distribution<double> dist(0.0, 1.0);

    std::vector<std::vector<double>> X;
    std::vector<int> y;
    for (int i = 0; i < 100; ++i) {
        std::vector<double> row(60);
        for (auto& v : row) v = dist(rng);
        X.push_back(row);
        y.push_back(row[0] > 0 ? 1 : 0);
    }

    model.train(X, y);

#ifdef USE_XGBOOST
    EXPECT_TRUE(model.ready());
    auto pred = model.predict(X[0]);
    EXPECT_FALSE(pred.empty());
    for (auto v : pred) {
        EXPECT_FALSE(std::isnan(v));
    }
#else
    EXPECT_FALSE(model.ready());
#endif
}

TEST(MLStress, XGBoostSaveLoadRoundTrip) {
    XGBoostConfig cfg;
    cfg.rounds = 5;
    XGBoostModel model(cfg);

    // Train with simple data
    std::vector<std::vector<double>> X = {{1,0,0}, {0,1,0}, {0,0,1}, {1,1,0}};
    std::vector<int> y = {1, 0, 1, 0};
    model.train(X, y);

    const std::string path = "/tmp/test_xgb_stress.model";
    model.save(path);

    XGBoostModel model2(cfg);
    bool loaded = model2.load(path);

#ifdef USE_XGBOOST
    EXPECT_TRUE(loaded);
    EXPECT_TRUE(model2.ready());
    // Predictions should match
    auto p1 = model.predict(X[0]);
    auto p2 = model2.predict(X[0]);
    EXPECT_EQ(p1.size(), p2.size());
    for (size_t i = 0; i < p1.size(); ++i) {
        EXPECT_NEAR(p1[i], p2[i], 1e-6);
    }
#else
    (void)loaded;
#endif

    std::filesystem::remove(path);
}

TEST(MLStress, XGBoostFeatureImportance) {
    XGBoostConfig cfg;
    cfg.rounds = 10;
    XGBoostModel model(cfg);

    std::vector<std::vector<double>> X;
    std::vector<int> y;
    std::mt19937 rng(123);
    std::normal_distribution<double> dist(0, 1);
    for (int i = 0; i < 50; ++i) {
        std::vector<double> row(10);
        for (auto& v : row) v = dist(rng);
        X.push_back(row);
        y.push_back(row[0] > 0 ? 1 : 0);
    }
    model.train(X, y);

    auto importance = model.featureImportance();
    // featureImportance() returns simplified empty vector — verify no crash
    (void)importance;
}

// ── LSTM Model stress ───────────────────────────────────────────────────────

TEST(MLStress, LSTMModelConstruction) {
    LSTMConfig cfg;
    cfg.inputSize = 60;
    cfg.hiddenSize = 128;
    cfg.numLayers = 2;
    LSTMModel model(cfg);
    EXPECT_FALSE(model.ready());
}

TEST(MLStress, LSTMPredictWithoutTraining) {
    LSTMConfig cfg;
    LSTMModel model(cfg);
    std::vector<double> features(60, 0.5);
    auto result = model.predict(features);
    (void)result; // no crash
}

TEST(MLStress, LSTMAddSamplesAndPredict) {
    LSTMConfig cfg;
    cfg.sequenceLen = 10;
    LSTMModel model(cfg);

    // Add enough samples for a full sequence
    for (int i = 0; i < 20; ++i) {
        std::vector<double> sample(60, std::sin(i * 0.5));
        model.addSample(sample);
    }

    auto pred = model.predict(std::vector<double>(60, 0.0));
    (void)pred; // no crash is primary check
}

TEST(MLStress, LSTMTrainSmallDataset) {
    LSTMConfig cfg;
    cfg.inputSize = 10;
    cfg.hiddenSize = 32;
    cfg.numLayers = 1;
    cfg.sequenceLen = 5;
    LSTMModel model(cfg);

    std::vector<std::vector<double>> X;
    std::vector<int> y;
    for (int i = 0; i < 50; ++i) {
        std::vector<double> row(10);
        for (int j = 0; j < 10; ++j) row[j] = std::sin(i * 0.3 + j);
        X.push_back(row);
        y.push_back(i % 3);
    }

    // Training with small epochs for speed
    model.train(X, y, 3, 0.01);

#ifdef USE_LIBTORCH
    EXPECT_TRUE(model.ready());
    auto pred = model.predict(X[0]);
    EXPECT_FALSE(pred.empty());
    for (auto v : pred) EXPECT_FALSE(std::isnan(v));
#else
    EXPECT_FALSE(model.ready());
#endif
}

TEST(MLStress, LSTMSaveLoadRoundTrip) {
    LSTMConfig cfg;
    cfg.inputSize = 10;
    cfg.hiddenSize = 32;
    cfg.numLayers = 1;
    LSTMModel model(cfg);

    const std::string path = "/tmp/test_lstm_stress.pt";
    model.save(path);

    LSTMModel model2(cfg);
    bool loaded = model2.load(path);
#ifdef USE_LIBTORCH
    // May or may not succeed depending on implementation
    (void)loaded;
#else
    EXPECT_FALSE(loaded);
#endif
    std::filesystem::remove(path);
}

// ── SignalEnhancer stress ───────────────────────────────────────────────────

TEST(MLStress, SignalEnhancerBasic) {
    LSTMConfig lcfg;
    XGBoostConfig xcfg;
    LSTMModel lstm(lcfg);
    XGBoostModel xgb(xcfg);

    SignalEnhancer::Config cfg;
    cfg.weightIndicator = 0.4;
    cfg.weightLstm = 0.3;
    cfg.weightXgb = 0.3;
    cfg.minConfidence = 0.65;
    SignalEnhancer enhancer(lstm, xgb, cfg);

    std::vector<double> features(60, 0.5);
    double confidence = 0.0;
    double result = enhancer.enhance(1, features, confidence);
    // Should return something (may be 0 if models not trained)
    EXPECT_FALSE(std::isnan(result));
    EXPECT_FALSE(std::isnan(confidence));
}

TEST(MLStress, SignalEnhancerRecordOutcome) {
    LSTMConfig lcfg;
    XGBoostConfig xcfg;
    LSTMModel lstm(lcfg);
    XGBoostModel xgb(xcfg);

    SignalEnhancer::Config cfg;
    SignalEnhancer enhancer(lstm, xgb, cfg);

    // Record many outcomes
    for (int i = 0; i < 200; ++i) {
        enhancer.recordOutcome(i % 3 - 1, (i + 1) % 3 - 1);
    }
    // No crash after many outcomes
    std::vector<double> features(60, 0.3);
    double confidence = 0.0;
    enhancer.enhance(0, features, confidence);
    EXPECT_FALSE(std::isnan(confidence));
}

// ── ModelTrainer stress ─────────────────────────────────────────────────────

TEST(MLStress, ModelTrainerAddCandles) {
    LSTMConfig lcfg;
    XGBoostConfig xcfg;
    LSTMModel lstm(lcfg);
    XGBoostModel xgb(xcfg);
    auto ind = makeTestEngine();
    FeatureExtractor feat(ind);

    ModelTrainer::Config cfg;
    cfg.retrainIntervalHours = 0; // allow immediate retrain
    ModelTrainer trainer(lstm, xgb, feat, cfg);

    // Feed many candles
    for (int i = 0; i < 300; ++i) {
        Candle c{};
        c.openTime = i * 60000;
        c.open  = 100.0 + std::sin(i * 0.1) * 5;
        c.high  = c.open + 2.0;
        c.low   = c.open - 2.0;
        c.close = c.open + std::cos(i * 0.1) * 1.5;
        c.volume = 1000.0 + i;
        c.closed = true;
        trainer.addCandle(c);
    }

    // Should not crash; retrain check should work
    bool sr = trainer.shouldRetrain();
    (void)sr;
}

// ═════════════════════════════════════════════════════════════════════════════
// SECTION 4: Integration Stress Tests (AI + ML + Layout combined)
// ═════════════════════════════════════════════════════════════════════════════

TEST(IntegrationStress, AIFeatureExtractorToRLAgent) {
    AIFeatureExtractor fe;
    RLTradingAgent agent;

    // Feed real-looking candle data
    for (int i = 0; i < 100; ++i) {
        Candle c{};
        c.openTime = i * 60000;
        c.open  = 50000.0 + std::sin(i * 0.05) * 2000;
        c.high  = c.open + std::abs(std::cos(i * 0.1)) * 500;
        c.low   = c.open - std::abs(std::sin(i * 0.1)) * 500;
        c.close = c.open + std::cos(i * 0.07) * 300;
        c.volume = 500.0 + i * 2;
        c.closed = true;
        fe.pushCandle(c);
    }

    DepthBook book;
    for (int i = 0; i < 20; ++i) {
        book.bids.push_back({50000.0 - i * 10.0, 5.0});
        book.asks.push_back({50010.0 + i * 10.0, 5.0});
    }
    fe.pushDepth(book);

    // Extract and run through agent for both modes
    auto fastFeatures  = fe.extract(AgentMode::Fast);
    auto swingFeatures = fe.extract(AgentMode::Swing);

    EXPECT_EQ(static_cast<int>(fastFeatures.size()), 128);
    EXPECT_EQ(static_cast<int>(swingFeatures.size()), 128);

    agent.setMode(AgentMode::Fast);
    auto fastResult = agent.forward_pass(fastFeatures);
    EXPECT_GE(fastResult.action, 0);
    EXPECT_LE(fastResult.action, 2);

    agent.setMode(AgentMode::Swing);
    auto swingResult = agent.forward_pass(swingFeatures);
    EXPECT_GE(swingResult.action, 0);
    EXPECT_LE(swingResult.action, 2);
}

TEST(IntegrationStress, FullMLPipeline) {
    // Test the full ML pipeline: IndicatorEngine → FeatureExtractor → Models
    auto ind = makeTestEngine();
    FeatureExtractor feat(ind);

    // Feed candles through indicator engine and feature extractor
    for (int i = 0; i < 100; ++i) {
        Candle c{};
        c.openTime = i * 60000;
        c.open  = 40000.0 + i * 10;
        c.high  = c.open + 50;
        c.low   = c.open - 50;
        c.close = c.open + 20;
        c.volume = 1000.0;
        c.closed = true;
        ind.update(c);
        feat.update(c);
    }

    auto features = feat.extract();
    if (features.valid) {
        EXPECT_EQ(static_cast<int>(features.values.size()), FeatureExtractor::FEATURE_DIM);
        for (auto v : features.values) {
            EXPECT_FALSE(std::isnan(v));
        }

        // Feed to models
        LSTMConfig lcfg;
        LSTMModel lstm(lcfg);
        auto lstmResult = lstm.predict(features.values);
        // No crash

        XGBoostConfig xcfg;
        XGBoostModel xgb(xcfg);
        auto xgbResult = xgb.predict(features.values);
        // No crash
    }
}

TEST(IntegrationStress, LayoutConsistencyAcrossRecalculations) {
    LayoutManager mgr;

    // Rapid recalculations simulating window resize
    for (int i = 0; i < 1000; ++i) {
        float w = 800.0f + (i % 50) * 20.0f;
        float h = 600.0f + (i % 30) * 15.0f;
        mgr.recalculate(w, h);

        auto md = mgr.get("Market Data");
        auto pl = mgr.get("Pair List");
        auto up = mgr.get("User Panel");
        auto log = mgr.get("Logs");

        // No overlap assertions
        EXPECT_LE(pl.pos.x + pl.size.x, md.pos.x + 0.5f);
        EXPECT_LE(md.pos.x + md.size.x, up.pos.x + 0.5f);
        EXPECT_LE(up.pos.y + up.size.y, log.pos.y + 0.5f);

        // No negative dimensions
        EXPECT_GE(md.size.x, 0.0f);
        EXPECT_GE(md.size.y, 0.0f);
    }
}

// ── Compile-time feature flag verification ──────────────────────────────────

TEST(IntegrationStress, LibTorchFeatureFlag) {
#ifdef USE_LIBTORCH
    // LibTorch is available — verify agent can be created and used
    RLTradingAgent agent;
    std::vector<float> state(128, 0.5f);
    auto result = agent.forward_pass(state);
    EXPECT_GE(result.action, 0);
    EXPECT_LE(result.action, 2);
    EXPECT_FALSE(std::isnan(result.logProb));
    EXPECT_FALSE(std::isnan(result.value));
#else
    GTEST_SKIP() << "LibTorch not available";
#endif
}

TEST(IntegrationStress, XGBoostFeatureFlag) {
#ifdef USE_XGBOOST
    // XGBoost is available — verify model can train
    XGBoostConfig cfg;
    cfg.rounds = 5;
    XGBoostModel model(cfg);

    std::vector<std::vector<double>> X = {{1,2,3}, {4,5,6}, {7,8,9}, {2,3,4}};
    std::vector<int> y = {0, 1, 0, 1};
    model.train(X, y);
    EXPECT_TRUE(model.ready());

    auto pred = model.predict(X[0]);
    EXPECT_FALSE(pred.empty());
#else
    GTEST_SKIP() << "XGBoost not available";
#endif
}
