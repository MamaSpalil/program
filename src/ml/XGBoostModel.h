#pragma once
#include "MLModel.h"
#include <vector>
#include <string>

#ifdef USE_XGBOOST
#include <xgboost/c_api.h>
#endif

namespace crypto {

struct XGBoostConfig {
    int maxDepth{6};
    double eta{0.1};
    double subsample{0.8};
    int rounds{500};
};

class XGBoostModel : public MLModel {
public:
    explicit XGBoostModel(const XGBoostConfig& cfg);
    ~XGBoostModel() override;

    std::vector<double> predict(const std::vector<double>& features) override;
    void save(const std::string& path) const override;
    bool load(const std::string& path) override;
    bool ready() const override;

    void train(const std::vector<std::vector<double>>& X,
               const std::vector<int>& y);

    std::vector<double> featureImportance() const;

private:
    XGBoostConfig cfg_;
    bool ready_{false};

#ifdef USE_XGBOOST
    BoosterHandle booster_{nullptr};
#endif
};

} // namespace crypto
