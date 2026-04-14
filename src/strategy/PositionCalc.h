#pragma once
#include <cmath>

namespace crypto {

struct PositionCalc {
    double balance  = 0.0;
    double riskPct  = 0.0; // e.g. 1.0 for 1%
    double entry    = 0.0;
    double stopLoss = 0.0;

    double riskAmt() const {
        return balance * (riskPct / 100.0);
    }

    double priceDiff() const {
        return std::abs(entry - stopLoss);
    }

    double posSize() const {
        double diff = priceDiff();
        if (diff < 1e-12) return 0.0;
        return riskAmt() / diff;
    }
};

class DailyLossGuard {
public:
    DailyLossGuard() = default;

    void init(double startingBalance, double limitPct) {
        startingBalance_ = startingBalance;
        limitPct_ = limitPct;
        triggered_ = false;
        currentBalance_ = startingBalance;
    }

    void update(double currentBalance) {
        currentBalance_ = currentBalance;
        if (startingBalance_ <= 0.0) return;  // guard against division by zero
        double lossPct = ((startingBalance_ - currentBalance) / startingBalance_) * 100.0;
        if (lossPct >= limitPct_) {
            triggered_ = true;
        }
    }

    bool isTriggered() const { return triggered_; }

    double currentDrawdownPct() const {
        if (startingBalance_ <= 0.0) return 0.0;
        return ((startingBalance_ - currentBalance_) / startingBalance_) * 100.0;
    }

    void reset(double newBalance) {
        startingBalance_ = newBalance;
        currentBalance_ = newBalance;
        triggered_ = false;
    }

private:
    double startingBalance_ = 0.0;
    double limitPct_ = 0.0;
    double currentBalance_ = 0.0;
    bool   triggered_ = false;
};

} // namespace crypto
