#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// FeatureExtractor.h — AI Feature Extraction (Smart Money Concepts)
// ─────────────────────────────────────────────────────────────────────────────
// Translates raw market data (CandleCache + DepthStreamManager) into a
// fixed-size floating-point feature vector (at::Tensor) for the RL agent.
//
// Two operational modes:
//   • Fast Mode  — features derived from L2 Orderbook depth stream
//   • Swing Mode — features derived from aggregated candle data (MTF)
//
// Built-in SMC (Smart Money Concepts) detectors:
//   • SFP  — Swing Failure Pattern
//   • FVG  — Fair Value Gap (bullish & bearish)
//   • BOS/CHOCH — Break of Structure / Change of Character
//   • Order Blocks & Breaker Blocks
//
// Zero dynamic allocations on the critical path — all buffers are
// pre-allocated in the constructor.
// ─────────────────────────────────────────────────────────────────────────────

#include "../data/CandleData.h"
#include "../exchange/DepthStream.h"
#include "RLTradingAgent.h"      // AgentMode enum
#include <vector>
#include <deque>
#include <cstddef>
#include <array>

namespace crypto { namespace ai {

// ── Configuration ────────────────────────────────────────────────────────────

struct AIFeatureConfig {
    int    pivotLookback    = 5;     ///< Bars to look back for pivot detection
    int    maxCandles       = 256;   ///< Max candle history retained
    int    obLevels         = 20;    ///< L2 orderbook levels per side
    int    fastFeatureDim   = 128;   ///< Output dimension in Fast mode
    int    swingFeatureDim  = 128;   ///< Output dimension in Swing mode
};

// ── Detected patterns ────────────────────────────────────────────────────────

/// Swing Failure Pattern result.
struct SFPResult {
    bool   detected  = false;
    bool   bullish   = false;  ///< true = bullish SFP (failed breakdown)
    double pivotPrice = 0.0;
    int    barIndex   = -1;    ///< Index in the candle history
};

/// Fair Value Gap result.
struct FVGResult {
    bool   detected  = false;
    bool   bullish   = false;
    double gapHigh   = 0.0;   ///< Upper boundary of the gap
    double gapLow    = 0.0;   ///< Lower boundary of the gap
    int    barIndex   = -1;
};

/// Order Block / Breaker Block.
struct OBResult {
    bool   detected   = false;
    bool   bullish    = false;
    double zoneHigh   = 0.0;
    double zoneLow    = 0.0;
    bool   isBreaker  = false; ///< true if the OB has been invalidated → Breaker
    int    barIndex   = -1;
};

// ── Feature extractor class ─────────────────────────────────────────────────

class AIFeatureExtractor {
public:
    explicit AIFeatureExtractor(const AIFeatureConfig& cfg = AIFeatureConfig{});
    ~AIFeatureExtractor() = default;

    AIFeatureExtractor(const AIFeatureExtractor&) = delete;
    AIFeatureExtractor& operator=(const AIFeatureExtractor&) = delete;

    // ── Data input ───────────────────────────────────────────────────────────

    /// Feed a closed candle (called each time CandleCache produces a new bar).
    void pushCandle(const Candle& c);

    /// Feed a snapshot of the L2 orderbook (from DepthStreamManager).
    void pushDepth(const DepthBook& book);

    // ── Feature extraction ───────────────────────────────────────────────────

    /// Build the full feature vector for the current mode.
    /// The returned vector has exactly `featureDim()` elements.
    std::vector<float> extract(AgentMode mode) const;

    /// Dimension of the feature vector for the given mode.
    int featureDim(AgentMode mode) const;

    // ── SMC pattern detectors ────────────────────────────────────────────────

    /// Detect the most recent Swing Failure Pattern in the candle history.
    /// SFP: price sweeps a local pivot (extreme) then closes back inside the
    /// prior range, trapping breakout traders.
    SFPResult calc_SFP() const;

    /// Detect the most recent Fair Value Gap in the candle history.
    /// Bullish FVG: Low[t] - High[t-2] > 0  (gap between candle t and t-2).
    /// Bearish FVG: Low[t-2] - High[t] > 0.
    FVGResult calc_FVG() const;

    /// Detect recent Order Blocks and Breaker Blocks.
    std::vector<OBResult> calc_OrderBlocks() const;

    /// Number of candles currently buffered.
    std::size_t candleCount() const { return candles_.size(); }

private:
    AIFeatureConfig cfg_;

    // ── Candle history (pre-allocated ring) ──────────────────────────────────
    std::deque<Candle> candles_;

    // ── L2 Orderbook snapshot (pre-allocated arrays) ─────────────────────────
    // Stored flat for zero-copy tensor construction.
    std::vector<float> bidPrices_;
    std::vector<float> bidQtys_;
    std::vector<float> askPrices_;
    std::vector<float> askQtys_;

    // ── Internal helpers ─────────────────────────────────────────────────────

    /// Compute pivot highs / lows over the candle buffer.
    struct PivotPoint {
        double price;
        int    index;
        bool   isHigh; ///< true = pivot high, false = pivot low
    };
    std::vector<PivotPoint> findPivots() const;

    /// Detect Break of Structure / Change of Character.
    /// Returns +1 (bullish BOS), -1 (bearish BOS), 0 (no BOS).
    int detectBOS() const;

    // Feature-building sub-routines (no allocations — write into caller buf)
    void buildFastFeatures(std::vector<float>& out) const;
    void buildSwingFeatures(std::vector<float>& out) const;
};

}} // namespace crypto::ai
