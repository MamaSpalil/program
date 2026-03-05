// ─────────────────────────────────────────────────────────────────────────────
// FeatureExtractor.cpp — AI Feature Extraction (Smart Money Concepts)
// ─────────────────────────────────────────────────────────────────────────────

#include "FeatureExtractor.h"
#include "../core/Logger.h"

#include <cmath>
#include <algorithm>
#include <numeric>
#include <limits>
#include <cassert>

namespace crypto { namespace ai {

// ── Constructor ──────────────────────────────────────────────────────────────

AIFeatureExtractor::AIFeatureExtractor(const AIFeatureConfig& cfg)
    : cfg_(cfg) {
    // Pre-allocate depth arrays to avoid allocations on the hot path
    bidPrices_.resize(cfg_.obLevels, 0.0f);
    bidQtys_.resize(cfg_.obLevels, 0.0f);
    askPrices_.resize(cfg_.obLevels, 0.0f);
    askQtys_.resize(cfg_.obLevels, 0.0f);
}

// ── Data input ───────────────────────────────────────────────────────────────

void AIFeatureExtractor::pushCandle(const Candle& c) {
    candles_.push_back(c);
    if (static_cast<int>(candles_.size()) > cfg_.maxCandles)
        candles_.pop_front();
}

void AIFeatureExtractor::pushDepth(const DepthBook& book) {
    // Flatten the top `obLevels` of the order book into pre-allocated arrays.
    const int nBids = std::min(static_cast<int>(book.bids.size()), cfg_.obLevels);
    const int nAsks = std::min(static_cast<int>(book.asks.size()), cfg_.obLevels);

    // Zero out first, then fill — avoids stale data in trailing slots
    std::fill(bidPrices_.begin(), bidPrices_.end(), 0.0f);
    std::fill(bidQtys_.begin(),   bidQtys_.end(),   0.0f);
    std::fill(askPrices_.begin(), askPrices_.end(), 0.0f);
    std::fill(askQtys_.begin(),   askQtys_.end(),   0.0f);

    for (int i = 0; i < nBids; ++i) {
        bidPrices_[i] = static_cast<float>(book.bids[i].price);
        bidQtys_[i]   = static_cast<float>(book.bids[i].qty);
    }
    for (int i = 0; i < nAsks; ++i) {
        askPrices_[i] = static_cast<float>(book.asks[i].price);
        askQtys_[i]   = static_cast<float>(book.asks[i].qty);
    }
}

// ── Feature dimension ────────────────────────────────────────────────────────

int AIFeatureExtractor::featureDim(AgentMode mode) const {
    return (mode == AgentMode::Fast) ? cfg_.fastFeatureDim
                                     : cfg_.swingFeatureDim;
}

// ── Feature extraction (top level) ───────────────────────────────────────────

std::vector<float> AIFeatureExtractor::extract(AgentMode mode) const {
    const int dim = featureDim(mode);
    std::vector<float> out(dim, 0.0f);

    if (mode == AgentMode::Fast) {
        buildFastFeatures(out);
    } else {
        buildSwingFeatures(out);
    }
    return out;
}

// ═════════════════════════════════════════════════════════════════════════════
// Fast Mode features (L2 Orderbook)
// ═════════════════════════════════════════════════════════════════════════════
//
// Feature layout (128-dim):
//   [0..19]    bid prices (normalised by mid-price)
//   [20..39]   bid quantities (log1p normalised)
//   [40..59]   ask prices (normalised by mid-price)
//   [60..79]   ask quantities (log1p normalised)
//   [80]       bid-ask spread (normalised)
//   [81]       order imbalance = (Σ bid_qty - Σ ask_qty) / (Σ bid_qty + Σ ask_qty)
//   [82]       weighted mid-price offset
//   [83..87]   cumulative depth pressure at 5 thresholds
//   [88]       SFP signal  (-1, 0, +1)
//   [89]       FVG signal  (-1, 0, +1)
//   [90]       BOS signal  (-1, 0, +1)
//   [91..127]  reserved (zero-padded for future features)
// ─────────────────────────────────────────────────────────────────────────────

void AIFeatureExtractor::buildFastFeatures(std::vector<float>& out) const {
    const int L = cfg_.obLevels;
    const int dim = static_cast<int>(out.size());
    if (dim < cfg_.fastFeatureDim) return;

    // Safe index setter — prevents out-of-bounds writes
    auto set = [&](int idx, float val) {
        if (idx >= 0 && idx < dim) out[idx] = val;
    };

    // Mid-price for normalisation (avoid division by zero)
    float bestBid = bidPrices_[0];
    float bestAsk = askPrices_[0];
    float mid     = (bestBid + bestAsk) * 0.5f;
    if (mid < 1e-12f) mid = 1.0f; // degenerate case

    // Bid/ask prices normalised as (price - mid) / mid
    for (int i = 0; i < L && i < 20; ++i) {
        set(i,      (bidPrices_[i] - mid) / mid);
        set(40 + i, (askPrices_[i] - mid) / mid);
    }

    // Bid/ask quantities (log1p normalisation for heavy-tailed distributions)
    for (int i = 0; i < L && i < 20; ++i) {
        set(20 + i, std::log1p(bidQtys_[i]));
        set(60 + i, std::log1p(askQtys_[i]));
    }

    // Bid-ask spread normalised by mid
    set(80, (bestAsk - bestBid) / mid);

    // Order imbalance ratio
    float sumBid = 0.0f, sumAsk = 0.0f;
    for (int i = 0; i < L; ++i) {
        sumBid += bidQtys_[i];
        sumAsk += askQtys_[i];
    }
    float totalQty = sumBid + sumAsk;
    set(81, (totalQty > 1e-12f) ? (sumBid - sumAsk) / totalQty : 0.0f);

    // Weighted mid-price = (bestBid * askQty + bestAsk * bidQty) / (bidQty + askQty)
    float wmid = (totalQty > 1e-12f)
        ? (bestBid * askQtys_[0] + bestAsk * bidQtys_[0]) / (bidQtys_[0] + askQtys_[0] + 1e-12f)
        : mid;
    set(82, (wmid - mid) / mid);

    // Cumulative depth pressure at 5 buckets (20%, 40%, 60%, 80%, 100% of levels)
    for (int k = 0; k < 5; ++k) {
        int cutoff = std::max(1, (L * (k + 1)) / 5);
        float bSum = 0.0f, aSum = 0.0f;
        for (int i = 0; i < cutoff; ++i) {
            bSum += bidQtys_[i];
            aSum += askQtys_[i];
        }
        float t = bSum + aSum;
        set(83 + k, (t > 1e-12f) ? (bSum - aSum) / t : 0.0f);
    }

    // SMC signals from candle history
    auto sfp = calc_SFP();
    auto fvg = calc_FVG();
    int  bos = detectBOS();

    set(88, sfp.detected ? (sfp.bullish ? 1.0f : -1.0f) : 0.0f);
    set(89, fvg.detected ? (fvg.bullish ? 1.0f : -1.0f) : 0.0f);
    set(90, static_cast<float>(bos));
}

// ═════════════════════════════════════════════════════════════════════════════
// Swing Mode features (Candle-based MTF analysis)
// ═════════════════════════════════════════════════════════════════════════════
//
// Feature layout (128-dim):
//   [0..19]    last 20 close prices (normalised by moving average)
//   [20..39]   last 20 volumes (log1p normalised)
//   [40..59]   last 20 log-returns
//   [60..69]   rolling volatility at 5, 10, 20, 30, 50, 60, 90, 120, 150, 200 bars
//   [70..79]   rolling mean log-return at same lookbacks
//   [80]       SFP signal
//   [81]       FVG signal
//   [82]       BOS signal
//   [83..92]   FVG gap size (normalised) for last 10 detected FVGs
//   [93..102]  Order Block zone mid-points (normalised) for last 10 OBs
//   [103..127] reserved (zero-padded)
// ─────────────────────────────────────────────────────────────────────────────

void AIFeatureExtractor::buildSwingFeatures(std::vector<float>& out) const {
    const int N = static_cast<int>(candles_.size());
    const int dim = static_cast<int>(out.size());
    if (N < 3 || dim < cfg_.swingFeatureDim) return;

    // Safe index setter — prevents out-of-bounds writes
    auto set = [&](int idx, float val) {
        if (idx >= 0 && idx < dim) out[idx] = val;
    };

    // ── Moving-average for normalisation ─────────────────────────────────────
    double sumClose = 0.0;
    for (auto& c : candles_) sumClose += c.close;
    double ma = sumClose / N;
    if (std::fabs(ma) < 1e-12) ma = 1.0;

    // ── Last 20 closes / volumes / log-returns ───────────────────────────────
    int start = std::max(0, N - 20);
    for (int i = start, k = 0; i < N && k < 20; ++i, ++k) {
        set(k,      static_cast<float>((candles_[i].close - ma) / ma));
        set(20 + k, static_cast<float>(std::log1p(candles_[i].volume)));
    }

    // Log-returns for last 20 bars
    for (int i = start + 1, k = 0; i < N && k < 20; ++i, ++k) {
        double prev = candles_[i - 1].close;
        double cur  = candles_[i].close;
        set(40 + k, (prev > 1e-12) ? static_cast<float>(std::log(cur / prev)) : 0.0f);
    }

    // ── Rolling volatility & mean at various lookbacks ───────────────────────
    constexpr int lookbacks[] = {5, 10, 20, 30, 50, 60, 90, 120, 150, 200};
    for (int idx = 0; idx < 10; ++idx) {
        int lb = std::min(lookbacks[idx], N - 1);
        if (lb < 2) { set(60 + idx, 0.0f); set(70 + idx, 0.0f); continue; }

        double sumR = 0.0, sumR2 = 0.0;
        int cnt = 0;
        for (int j = N - lb; j < N; ++j) {
            double prev = candles_[j - 1].close;
            if (prev < 1e-12) continue;
            double r = std::log(candles_[j].close / prev);
            sumR  += r;
            sumR2 += r * r;
            ++cnt;
        }
        if (cnt > 1) {
            double meanR = sumR / cnt;
            double var   = (sumR2 / cnt) - (meanR * meanR);
            set(60 + idx, static_cast<float>(std::sqrt(std::max(var, 0.0))));
            set(70 + idx, static_cast<float>(meanR));
        }
    }

    // ── SMC signals ──────────────────────────────────────────────────────────
    auto sfp = calc_SFP();
    auto fvg = calc_FVG();
    int  bos = detectBOS();

    set(80, sfp.detected ? (sfp.bullish ? 1.0f : -1.0f) : 0.0f);
    set(81, fvg.detected ? (fvg.bullish ? 1.0f : -1.0f) : 0.0f);
    set(82, static_cast<float>(bos));

    // ── FVG gap sizes (scan backwards for up to 10 FVGs) ─────────────────────
    int fvgCount = 0;
    for (int i = N - 1; i >= 2 && fvgCount < 10; --i) {
        // Bullish FVG: candles_[i].low > candles_[i-2].high
        double bullGap = candles_[i].low - candles_[i - 2].high;
        // Bearish FVG: candles_[i-2].low > candles_[i].high
        double bearGap = candles_[i - 2].low - candles_[i].high;

        if (bullGap > 0.0) {
            set(83 + fvgCount, static_cast<float>(bullGap / ma));
            ++fvgCount;
        } else if (bearGap > 0.0) {
            set(83 + fvgCount, static_cast<float>(-bearGap / ma));
            ++fvgCount;
        }
    }

    // ── Order Block mid-points ───────────────────────────────────────────────
    auto obs = calc_OrderBlocks();
    for (int i = 0; i < static_cast<int>(obs.size()) && i < 10; ++i) {
        double mid = (obs[i].zoneHigh + obs[i].zoneLow) * 0.5;
        set(93 + i, static_cast<float>((mid - ma) / ma));
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// Swing Failure Pattern (SFP) Detection
// ═════════════════════════════════════════════════════════════════════════════
//
// An SFP occurs when price sweeps a local pivot (breaking above a swing high
// or below a swing low) but then closes back inside the prior range, trapping
// breakout traders.
//
// Algorithm:
//   1. Find pivot highs/lows using N-bar lookback.
//   2. Check if the most recent bar's wick swept beyond the pivot but
//      the close reverted inside the previous range.
// ─────────────────────────────────────────────────────────────────────────────

SFPResult AIFeatureExtractor::calc_SFP() const {
    SFPResult result;
    const int N = static_cast<int>(candles_.size());
    const int lb = cfg_.pivotLookback;
    if (N < lb + 2) return result;

    auto pivots = findPivots();
    if (pivots.empty()) return result;

    // Check the latest candle against the most recent pivots
    const auto& last = candles_.back();

    // Scan pivots from most recent to oldest
    for (auto it = pivots.rbegin(); it != pivots.rend(); ++it) {
        if (it->isHigh) {
            // Bearish SFP: wick above pivot high, close below it
            if (last.high > it->price && last.close < it->price) {
                result.detected   = true;
                result.bullish    = false;
                result.pivotPrice = it->price;
                result.barIndex   = N - 1;
                return result;
            }
        } else {
            // Bullish SFP: wick below pivot low, close above it
            if (last.low < it->price && last.close > it->price) {
                result.detected   = true;
                result.bullish    = true;
                result.pivotPrice = it->price;
                result.barIndex   = N - 1;
                return result;
            }
        }
    }
    return result;
}

// ═════════════════════════════════════════════════════════════════════════════
// Fair Value Gap (FVG) Detection
// ═════════════════════════════════════════════════════════════════════════════
//
// A Fair Value Gap is a price imbalance between the wicks of three
// consecutive candles where no trading occurred, indicating aggressive
// momentum.
//
// Bullish FVG:   Low[t]   - High[t-2]  > 0   (gap between 1st and 3rd candle)
// Bearish FVG:   Low[t-2] - High[t]    > 0
//
// The gap boundaries are:
//   Bullish: gapLow  = High[t-2],  gapHigh = Low[t]
//   Bearish: gapLow  = High[t],    gapHigh = Low[t-2]
// ─────────────────────────────────────────────────────────────────────────────

FVGResult AIFeatureExtractor::calc_FVG() const {
    FVGResult result;
    const int N = static_cast<int>(candles_.size());
    if (N < 3) return result;

    // Check the three most recent candles (t = N-1, t-1 = N-2, t-2 = N-3)
    const auto& c0 = candles_[N - 3]; // oldest of the three (t-2)
    const auto& c2 = candles_[N - 1]; // newest (t)

    // Bullish FVG: gap above c0.high up to c2.low
    double bullGap = c2.low - c0.high;
    if (bullGap > 0.0) {
        result.detected = true;
        result.bullish  = true;
        result.gapLow   = c0.high;
        result.gapHigh  = c2.low;
        result.barIndex = N - 1;
        return result;
    }

    // Bearish FVG: gap below c0.low down to c2.high
    double bearGap = c0.low - c2.high;
    if (bearGap > 0.0) {
        result.detected = true;
        result.bullish  = false;
        result.gapLow   = c2.high;
        result.gapHigh  = c0.low;
        result.barIndex = N - 1;
        return result;
    }

    return result;
}

// ═════════════════════════════════════════════════════════════════════════════
// Order Blocks & Breaker Blocks Detection
// ═════════════════════════════════════════════════════════════════════════════
//
// An Order Block is the last opposite-direction candle before a strong
// impulsive move that breaks market structure.
//
// A Breaker Block is a former Order Block that has been invalidated (price
// returned and closed through it), converting support→resistance or vice versa.
//
// Detection algorithm:
//   1. Identify pivot highs and lows.
//   2. For each break of structure (BOS), find the last opposing candle
//      before the impulse move → that is the Order Block.
//   3. If a later candle closes through the OB zone, mark it as Breaker.
// ─────────────────────────────────────────────────────────────────────────────

std::vector<OBResult> AIFeatureExtractor::calc_OrderBlocks() const {
    std::vector<OBResult> results;
    const int N = static_cast<int>(candles_.size());
    if (N < 5) return results;

    auto pivots = findPivots();
    if (pivots.size() < 2) return results;

    // Walk through pairs of consecutive pivots to detect structure breaks
    for (size_t p = 1; p < pivots.size(); ++p) {
        const auto& prev = pivots[p - 1];
        const auto& curr = pivots[p];

        // Bullish BOS: a pivot low followed by a higher pivot high that
        // exceeds the previous pivot high
        if (!prev.isHigh && curr.isHigh && curr.price > prev.price) {
            // The Order Block is the last bearish candle before the impulse
            int obIdx = -1;
            for (int j = curr.index - 1; j >= prev.index; --j) {
                if (j < N && candles_[j].close < candles_[j].open) {
                    obIdx = j;
                    break;
                }
            }
            if (obIdx >= 0 && obIdx < N) {
                OBResult ob;
                ob.detected  = true;
                ob.bullish   = true;
                ob.zoneHigh  = candles_[obIdx].high;
                ob.zoneLow   = candles_[obIdx].low;
                ob.barIndex  = obIdx;
                ob.isBreaker = false;

                // Check if a later candle closed below the OB → Breaker
                for (int j = obIdx + 1; j < N; ++j) {
                    if (candles_[j].close < ob.zoneLow) {
                        ob.isBreaker = true;
                        break;
                    }
                }
                results.push_back(ob);
            }
        }

        // Bearish BOS: a pivot high followed by a lower pivot low
        if (prev.isHigh && !curr.isHigh && curr.price < prev.price) {
            // The Order Block is the last bullish candle before the impulse
            int obIdx = -1;
            for (int j = curr.index - 1; j >= prev.index; --j) {
                if (j < N && candles_[j].close > candles_[j].open) {
                    obIdx = j;
                    break;
                }
            }
            if (obIdx >= 0 && obIdx < N) {
                OBResult ob;
                ob.detected  = true;
                ob.bullish   = false;
                ob.zoneHigh  = candles_[obIdx].high;
                ob.zoneLow   = candles_[obIdx].low;
                ob.barIndex  = obIdx;
                ob.isBreaker = false;

                // Check if a later candle closed above the OB → Breaker
                for (int j = obIdx + 1; j < N; ++j) {
                    if (candles_[j].close > ob.zoneHigh) {
                        ob.isBreaker = true;
                        break;
                    }
                }
                results.push_back(ob);
            }
        }
    }

    return results;
}

// ── Pivot detection ──────────────────────────────────────────────────────────
//
// A pivot high at index i requires:
//   High[i] > High[j]  for all j in [i-lookback, i+lookback], j ≠ i
//
// A pivot low at index i requires:
//   Low[i]  < Low[j]   for all j in [i-lookback, i+lookback], j ≠ i
// ─────────────────────────────────────────────────────────────────────────────

std::vector<AIFeatureExtractor::PivotPoint>
AIFeatureExtractor::findPivots() const {
    std::vector<PivotPoint> pivots;
    const int N  = static_cast<int>(candles_.size());
    const int lb = cfg_.pivotLookback;
    if (N < 2 * lb + 1) return pivots;

    for (int i = lb; i < N - lb; ++i) {
        bool isHigh = true, isLow = true;
        for (int j = -lb; j <= lb; ++j) {
            if (j == 0) continue;
            if (candles_[i].high <= candles_[i + j].high) isHigh = false;
            if (candles_[i].low  >= candles_[i + j].low)  isLow  = false;
        }
        if (isHigh) pivots.push_back({candles_[i].high, i, true});
        if (isLow)  pivots.push_back({candles_[i].low,  i, false});
    }
    return pivots;
}

// ── Break of Structure (BOS) / Change of Character (CHOCH) ──────────────────
//
// BOS: Price closes beyond the most recent same-direction pivot.
//   +1 = bullish BOS (close > last pivot high)
//   -1 = bearish BOS (close < last pivot low)
//    0 = no break detected
// ─────────────────────────────────────────────────────────────────────────────

int AIFeatureExtractor::detectBOS() const {
    const int N = static_cast<int>(candles_.size());
    if (N < 3) return 0;

    auto pivots = findPivots();
    if (pivots.empty()) return 0;

    const double lastClose = candles_.back().close;

    // Find the most recent pivot high and pivot low
    double lastPivotHigh = -std::numeric_limits<double>::max();
    double lastPivotLow  =  std::numeric_limits<double>::max();

    for (auto it = pivots.rbegin(); it != pivots.rend(); ++it) {
        if (it->isHigh && it->price > lastPivotHigh) {
            lastPivotHigh = it->price;
        }
        if (!it->isHigh && it->price < lastPivotLow) {
            lastPivotLow = it->price;
        }
        // Only need the first of each type (most recent)
        if (lastPivotHigh > -std::numeric_limits<double>::max() &&
            lastPivotLow  <  std::numeric_limits<double>::max())
            break;
    }

    if (lastClose > lastPivotHigh) return +1;  // Bullish BOS
    if (lastClose < lastPivotLow)  return -1;  // Bearish BOS
    return 0;
}

}} // namespace crypto::ai
