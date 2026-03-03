#pragma once
#include "../data/CandleData.h"
#include "../indicators/IndicatorEngine.h"
#include "../strategy/RiskManager.h"
#include <atomic>
#include <thread>
#include <mutex>
#include <string>

namespace crypto {

class Dashboard {
public:
    Dashboard();
    ~Dashboard();

    void start();
    void stop();

    void update(const Candle& c,
                const IndicatorEngine& ind,
                const RiskManager& risk,
                const Signal& sig);

private:
    std::atomic<bool> running_{false};
    std::thread renderThread_;
    mutable std::mutex dataMutex_;

    Candle lastCandle_;
    double ema_fast_{0}, ema_slow_{0}, rsi_{50}, atr_{0};
    double equity_{0};
    Signal lastSignal_;
    std::string mode_{"paper"};

    void renderLoop();
    void initCurses();
    void cleanupCurses();
    void draw();
};

} // namespace crypto
