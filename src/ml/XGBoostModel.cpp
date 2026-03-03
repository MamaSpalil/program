#include "XGBoostModel.h"
#include "../core/Logger.h"
#include <stdexcept>

namespace crypto {

#ifdef USE_XGBOOST

XGBoostModel::XGBoostModel(const XGBoostConfig& cfg) : cfg_(cfg) {
    XGBoosterCreate(nullptr, 0, &booster_);
}

XGBoostModel::~XGBoostModel() {
    if (booster_) XGBoosterFree(booster_);
}

void XGBoostModel::train(const std::vector<std::vector<double>>& X,
                          const std::vector<int>& y) {
    if (X.empty()) return;
    int nRows = static_cast<int>(X.size());
    int nCols = static_cast<int>(X[0].size());

    std::vector<float> flat;
    flat.reserve(nRows * nCols);
    std::vector<float> labels;
    labels.reserve(nRows);
    for (int i = 0; i < nRows; ++i) {
        for (double v : X[i]) flat.push_back(static_cast<float>(v));
        labels.push_back(static_cast<float>(y[i]));
    }

    DMatrixHandle dmat;
    XGDMatrixCreateFromMat(flat.data(), nRows, nCols, NAN, &dmat);
    XGDMatrixSetFloatInfo(dmat, "label", labels.data(), nRows);

    XGBoosterCreate(&dmat, 1, &booster_);
    XGBoosterSetParam(booster_, "max_depth", std::to_string(cfg_.maxDepth).c_str());
    XGBoosterSetParam(booster_, "eta", std::to_string(cfg_.eta).c_str());
    XGBoosterSetParam(booster_, "subsample", std::to_string(cfg_.subsample).c_str());
    XGBoosterSetParam(booster_, "objective", "multi:softprob");
    XGBoosterSetParam(booster_, "num_class", "3");
    XGBoosterSetParam(booster_, "eval_metric", "mlogloss");

    for (int i = 0; i < cfg_.rounds; ++i) {
        XGBoosterUpdateOneIter(booster_, i, dmat);
    }
    XGDMatrixFree(dmat);
    ready_ = true;
    Logger::get()->info("XGBoost training complete ({} rounds)", cfg_.rounds);
}

std::vector<double> XGBoostModel::predict(const std::vector<double>& features) {
    if (!ready_) return {0.333, 0.334, 0.333};
    int nCols = static_cast<int>(features.size());
    std::vector<float> flat;
    flat.reserve(features.size());
    for (double d : features) flat.push_back(static_cast<float>(d));

    DMatrixHandle dmat;
    XGDMatrixCreateFromMat(flat.data(), 1, nCols, NAN, &dmat);

    bst_ulong outLen;
    const float* outResult;
    XGBoosterPredict(booster_, dmat, 0, 0, 0, &outLen, &outResult);
    XGDMatrixFree(dmat);

    if (outLen < 3) return {0.333, 0.334, 0.333};
    return {outResult[0], outResult[1], outResult[2]};
}

void XGBoostModel::save(const std::string& path) const {
    if (!ready_) return;
    XGBoosterSaveModel(booster_, path.c_str());
}

bool XGBoostModel::load(const std::string& path) {
    try {
        XGBoosterLoadModel(booster_, path.c_str());
        ready_ = true;
        return true;
    } catch (...) {
        return false;
    }
}

bool XGBoostModel::ready() const { return ready_; }

std::vector<double> XGBoostModel::featureImportance() const {
    return {};  // Simplified
}

#else

XGBoostModel::XGBoostModel(const XGBoostConfig& cfg) : cfg_(cfg) {}
XGBoostModel::~XGBoostModel() = default;
void XGBoostModel::train(const std::vector<std::vector<double>>&,
                          const std::vector<int>&) {
    Logger::get()->warn("XGBoost: library not available, using stub");
}
std::vector<double> XGBoostModel::predict(const std::vector<double>&) {
    return {0.333, 0.334, 0.333};
}
void XGBoostModel::save(const std::string&) const {}
bool XGBoostModel::load(const std::string&) { return false; }
bool XGBoostModel::ready() const { return false; }
std::vector<double> XGBoostModel::featureImportance() const { return {}; }

#endif

} // namespace crypto
