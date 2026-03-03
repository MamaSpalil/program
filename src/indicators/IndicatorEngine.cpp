#include "IndicatorEngine.h"

namespace crypto {

IndicatorEngine::IndicatorEngine(const nlohmann::json& cfg) :
    ind_([&]{
        PineScriptIndicator::Config c;
        c.emaFast       = cfg.value("ema_fast",       9);
        c.emaSlow       = cfg.value("ema_slow",       21);
        c.emaTrend      = cfg.value("ema_trend",      200);
        c.rsiPeriod     = cfg.value("rsi_period",     14);
        c.rsiOverbought = cfg.value("rsi_overbought", 70.0);
        c.rsiOversold   = cfg.value("rsi_oversold",   30.0);
        c.atrPeriod     = cfg.value("atr_period",     14);
        c.macdFast      = cfg.value("macd_fast",      12);
        c.macdSlow      = cfg.value("macd_slow",      26);
        c.macdSignal    = cfg.value("macd_signal",    9);
        c.bbPeriod      = cfg.value("bb_period",      20);
        c.bbStddev      = cfg.value("bb_stddev",      2.0);
        return c;
    }())
{}

void IndicatorEngine::update(const Candle& c) { ind_.update(c); }
Signal IndicatorEngine::evaluate(double mlConf) const { return ind_.evaluate(mlConf, mlThreshold_); }
bool   IndicatorEngine::ready()   const { return ind_.ready(); }
double IndicatorEngine::emaFast() const { return ind_.emaFastVal(); }
double IndicatorEngine::emaSlow() const { return ind_.emaSlowVal(); }
double IndicatorEngine::emaTrend()const { return ind_.emaTrendVal(); }
double IndicatorEngine::rsi()     const { return ind_.rsiVal(); }
double IndicatorEngine::atr()     const { return ind_.atrVal(); }
MACDResult IndicatorEngine::macd()const { return ind_.macdVal(); }
BBResult   IndicatorEngine::bb()  const { return ind_.bbVal(); }

} // namespace crypto
