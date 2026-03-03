#pragma once
#include "../data/CandleData.h"
#include <vector>
#include <string>

namespace crypto {

// Base interface for all ML models
class MLModel {
public:
    virtual ~MLModel() = default;

    // Returns {p_buy, p_hold, p_sell}
    virtual std::vector<double> predict(const std::vector<double>& features) = 0;

    virtual void save(const std::string& path) const = 0;
    virtual bool load(const std::string& path) = 0;
    virtual bool ready() const = 0;
};

} // namespace crypto
