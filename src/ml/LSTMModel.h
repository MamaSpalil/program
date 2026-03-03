#pragma once
#include "MLModel.h"
#include <string>
#include <vector>
#include <deque>

// LibTorch is optional; if not available we fall back to a stub
#ifdef USE_LIBTORCH
#include <torch/torch.h>
#endif

namespace crypto {

struct LSTMConfig {
    int inputSize{60};
    int hiddenSize{128};
    int numLayers{2};
    int sequenceLen{60};
    float dropout{0.2f};
};

class LSTMModel : public MLModel {
public:
    explicit LSTMModel(const LSTMConfig& cfg);
    ~LSTMModel() override;

    std::vector<double> predict(const std::vector<double>& features) override;
    void save(const std::string& path) const override;
    bool load(const std::string& path) override;
    bool ready() const override;

    void train(const std::vector<std::vector<double>>& X,
               const std::vector<int>& y,
               int epochs = 50,
               double lr = 0.001);

    void addSample(const std::vector<double>& features);

private:
    LSTMConfig cfg_;
    bool ready_{false};

    std::deque<std::vector<double>> seqBuffer_;

#ifdef USE_LIBTORCH
    struct Net;
    std::shared_ptr<Net> net_;
    torch::Device device_;
#endif
};

} // namespace crypto
