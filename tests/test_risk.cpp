#include <gtest/gtest.h>
#include "../src/strategy/RiskManager.h"
#include "../src/data/CandleData.h"

using namespace crypto;

TEST(RiskManager, PositionSizing) {
    RiskManager::Config cfg;
    cfg.maxRiskPerTrade = 0.02;
    RiskManager rm(cfg, 10000.0);

    double entry = 100.0, stop = 98.0;
    double size = rm.positionSize(entry, stop);
    // risk = 10000 * 0.02 = 200, riskPerUnit = 2.0
    EXPECT_DOUBLE_EQ(size, 100.0);
}

TEST(RiskManager, CanTradeDrawdown) {
    RiskManager::Config cfg;
    cfg.maxRiskPerTrade = 0.02;
    cfg.maxDrawdown = 0.15;
    RiskManager rm(cfg, 10000.0);

    EXPECT_TRUE(rm.canTrade());

    // Simulate large loss
    rm.onTradeOpen(Position{});
    rm.onTradeClose(-2000.0);  // 20% drawdown
    EXPECT_FALSE(rm.canTrade());
}

TEST(RiskManager, EquityUpdate) {
    RiskManager::Config cfg;
    RiskManager rm(cfg, 5000.0);
    EXPECT_DOUBLE_EQ(rm.equity(), 5000.0);

    rm.onTradeOpen(Position{});
    rm.onTradeClose(500.0);
    EXPECT_DOUBLE_EQ(rm.equity(), 5500.0);
}

TEST(RiskManager, KellyCriterion) {
    RiskManager::Config cfg;
    cfg.maxRiskPerTrade = 0.02;
    RiskManager rm(cfg, 10000.0);

    // winRate=0.6, avgWin=100, avgLoss=100 -> kelly = 0.6 - 0.4/1 = 0.2, capped at 0.04
    double size = rm.kellySizing(0.6, 100.0, 100.0);
    EXPECT_GT(size, 0.0);
    // Capped at maxRiskPerTrade * 2 * equity = 400
    EXPECT_LE(size, 10000.0 * cfg.maxRiskPerTrade * 2.0);
}

TEST(RiskManager, ZeroRisk) {
    RiskManager::Config cfg;
    RiskManager rm(cfg, 10000.0);

    // Zero stop distance
    double size = rm.positionSize(100.0, 100.0);
    EXPECT_DOUBLE_EQ(size, 0.0);
}
