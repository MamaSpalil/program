#include "MLEnhancedStrategy.h"
#include "../core/Logger.h"
#include <cmath>

namespace crypto {

MLEnhancedStrategy::MLEnhancedStrategy(const nlohmann::json& config,
                                        double initialCapital)
    : indEngine_(config.value("indicator", nlohmann::json::object())),
      lstmCfg_([&]{
          LSTMConfig c;
          auto& lj = config["ml"]["lstm"];
          c.hiddenSize  = lj.value("hidden_size", 128);
          c.sequenceLen = lj.value("seq_len", 60);
          c.dropout     = lj.value("dropout", 0.2f);
          c.inputSize   = FeatureExtractor::FEATURE_DIM;
          return c;
      }()),
      lstmModel_(lstmCfg_),
      xgbCfg_([&]{
          XGBoostConfig c;
          auto& xj = config["ml"]["xgboost"];
          c.maxDepth = xj.value("max_depth", 6);
          c.eta      = xj.value("eta", 0.1);
          c.rounds   = xj.value("rounds", 500);
          return c;
      }()),
      xgbModel_(xgbCfg_),
      riskMgr_([&]{
          RiskManager::Config c;
          auto& rj = config["risk"];
          c.maxRiskPerTrade    = rj.value("max_risk_per_trade", 0.02);
          c.maxDrawdown        = rj.value("max_drawdown", 0.15);
          c.atrStopMultiplier  = rj.value("atr_stop_multiplier", 1.5);
          c.rrRatio            = rj.value("rr_ratio", 2.0);
          return c;
      }(), initialCapital),
      featExtract_(indEngine_),
      enhancer_(lstmModel_, xgbModel_, [&]{
          SignalEnhancer::Config c;
          c.minConfidence = config["ml"]["ensemble"].value("min_confidence", 0.65);
          return c;
      }()),
      trainer_(lstmModel_, xgbModel_, featExtract_, [&]{
          ModelTrainer::Config c;
          c.retrainIntervalHours = config["ml"].value("retrain_interval_hours", 24);
          c.lookbackDays         = config["ml"].value("lookback_days", 90);
          return c;
      }()),
      minConfidence_(config["ml"]["ensemble"].value("min_confidence", 0.65))
{}

void MLEnhancedStrategy::onCandle(const Candle& c) {
    if (!c.closed) return;  // Only process closed candles

    indEngine_.update(c);
    trainer_.addCandle(c);

    if (trainer_.shouldRetrain()) {
        trainer_.retrain();
    }

    checkEarlyExit(c);

    if (!indEngine_.ready() || !riskMgr_.canTrade()) {
        lastSignal_ = std::nullopt;
        return;
    }

    auto features = featExtract_.extract();
    if (!features.valid) {
        lastSignal_ = std::nullopt;
        return;
    }

    // Get raw indicator signal
    Signal indSig = indEngine_.evaluate(0.0);
    int indDir = 0;
    if (indSig.type == Signal::Type::BUY)  indDir = 1;
    if (indSig.type == Signal::Type::SELL) indDir = -1;

    double confidence = 0.0;
    enhancer_.enhance(indDir, features.values, confidence);

    if (confidence < minConfidence_ || indDir == 0) {
        lastSignal_ = std::nullopt;
        return;
    }

    Signal sig = indEngine_.evaluate(confidence);
    double atr = indEngine_.atr();

    if (sig.type == Signal::Type::BUY) {
        sig.stopLoss   = sig.price - 1.5 * atr;
        sig.takeProfit = sig.price + 3.0 * atr;
    } else if (sig.type == Signal::Type::SELL) {
        sig.stopLoss   = sig.price + 1.5 * atr;
        sig.takeProfit = sig.price - 3.0 * atr;
    }
    sig.confidence = confidence;
    lastSignal_ = sig;
}

void MLEnhancedStrategy::checkEarlyExit(const Candle& c) {
    if (!openPos_.has_value()) return;
    auto& pos = openPos_.value();

    bool hitStop = false, hitTP = false;
    if (pos.side == Position::Side::LONG) {
        hitStop = c.low  <= pos.stopLoss;
        hitTP   = c.high >= pos.takeProfit;
    } else if (pos.side == Position::Side::SHORT) {
        hitStop = c.high >= pos.stopLoss;
        hitTP   = c.low  <= pos.takeProfit;
    }

    if (hitStop || hitTP) {
        double exitPrice = hitTP ? pos.takeProfit : pos.stopLoss;
        double pnl = (pos.side == Position::Side::LONG)
            ? (exitPrice - pos.entryPrice) * pos.qty
            : (pos.entryPrice - exitPrice) * pos.qty;
        riskMgr_.onTradeClose(pnl);
        openPos_ = std::nullopt;
        Logger::get()->info("Position closed at {:.2f} PnL={:.2f}", exitPrice, pnl);
    }
}

std::optional<Signal> MLEnhancedStrategy::getSignal() {
    return lastSignal_;
}

void MLEnhancedStrategy::onFill(const Position& pos) {
    openPos_ = pos;
    riskMgr_.onTradeOpen(pos);
    Logger::get()->info("Position filled: side={} entry={:.2f} qty={:.4f}",
        static_cast<int>(pos.side), pos.entryPrice, pos.qty);
}

} // namespace crypto
