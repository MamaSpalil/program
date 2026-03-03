#pragma once
#include "../data/CandleData.h"
#include "EMA.h"
#include "RSI.h"
#include "ATR.h"
#include <deque>
#include <cmath>

namespace crypto {

// Pine Script built-in equivalents
namespace ta {

template<typename Container>
inline double highest(const Container& src, int len) {
    double h = -std::numeric_limits<double>::infinity();
    int n = static_cast<int>(src.size());
    int start = std::max(0, n - len);
    for (int i = start; i < n; ++i) h = std::max(h, src[i]);
    return h;
}

template<typename Container>
inline double lowest(const Container& src, int len) {
    double l = std::numeric_limits<double>::infinity();
    int n = static_cast<int>(src.size());
    int start = std::max(0, n - len);
    for (int i = start; i < n; ++i) l = std::min(l, src[i]);
    return l;
}

inline bool crossover(double cur, double prev, double sigCur, double sigPrev) {
    return prev <= sigPrev && cur > sigCur;
}

inline bool crossunder(double cur, double prev, double sigCur, double sigPrev) {
    return prev >= sigPrev && cur < sigCur;
}

inline double nz(double val, double replace = 0.0) {
    return std::isnan(val) ? replace : val;
}

} // namespace ta

struct MACDResult {
    double macd{0.0};
    double signal{0.0};
    double histogram{0.0};
};

struct BBResult {
    double upper{0.0};
    double middle{0.0};
    double lower{0.0};
};

class PineScriptIndicator {
public:
    struct Config {
        int emaFast{9};
        int emaSlow{21};
        int emaTrend{200};
        int rsiPeriod{14};
        double rsiOverbought{70.0};
        double rsiOversold{30.0};
        int atrPeriod{14};
        int macdFast{12};
        int macdSlow{26};
        int macdSignal{9};
        int bbPeriod{20};
        double bbStddev{2.0};
    };

    explicit PineScriptIndicator(const Config& cfg);

    void update(const Candle& c);
    Signal evaluate(double mlConfidence = 0.0, double mlThreshold = 0.65) const;

    double emaFastVal() const  { return emaFast_.value(); }
    double emaSlowVal() const  { return emaSlow_.value(); }
    double emaTrendVal() const { return emaTrend_.value(); }
    double rsiVal() const      { return rsi_.value(); }
    double atrVal() const      { return atr_.value(); }
    MACDResult macdVal() const { return macd_; }
    BBResult bbVal() const     { return bb_; }
    bool ready() const;

private:
    Config cfg_;
    EMA emaFast_, emaSlow_, emaTrend_;
    EMA macdFastEma_, macdSlowEma_, macdSignalEma_;
    RSI rsi_;
    ATR atr_;

    double prevEmaFast_{0.0}, prevEmaSlow_{0.0};
    MACDResult macd_;
    BBResult bb_;

    std::deque<double> closes_;

    void updateMACD();
    void updateBB();
};

} // namespace crypto
