#include "LSTMModel.h"
#include "../core/Logger.h"
#include <stdexcept>

namespace crypto {

#ifdef USE_LIBTORCH

struct LSTMModel::Net : torch::nn::Module {
    torch::nn::LSTM lstm{nullptr};
    torch::nn::Dropout drop{nullptr};
    torch::nn::Linear fc{nullptr};

    Net(int inputSize, int hiddenSize, int numLayers, float dropout) {
        lstm = register_module("lstm",
            torch::nn::LSTM(torch::nn::LSTMOptions(inputSize, hiddenSize)
                .num_layers(numLayers)
                .dropout(dropout)
                .batch_first(true)));
        drop = register_module("drop", torch::nn::Dropout(dropout));
        fc   = register_module("fc", torch::nn::Linear(hiddenSize, 3));
    }

    torch::Tensor forward(torch::Tensor x) {
        auto [out, hidden] = lstm->forward(x);
        auto last = out.select(1, -1);
        last = drop->forward(last);
        return torch::softmax(fc->forward(last), 1);
    }
};

LSTMModel::LSTMModel(const LSTMConfig& cfg)
    : cfg_(cfg),
      device_(torch::cuda::is_available() ? torch::kCUDA : torch::kCPU) {
    net_ = std::make_shared<Net>(cfg_.inputSize, cfg_.hiddenSize,
                                  cfg_.numLayers, cfg_.dropout);
    net_->to(device_);
}

LSTMModel::~LSTMModel() = default;

void LSTMModel::addSample(const std::vector<double>& features) {
    seqBuffer_.push_back(features);
    if (static_cast<int>(seqBuffer_.size()) > cfg_.sequenceLen) {
        seqBuffer_.pop_front();
    }
}

std::vector<double> LSTMModel::predict(const std::vector<double>& features) {
    if (!ready_) return {0.333, 0.334, 0.333};
    seqBuffer_.push_back(features);
    if (static_cast<int>(seqBuffer_.size()) > cfg_.sequenceLen) seqBuffer_.pop_front();
    if (static_cast<int>(seqBuffer_.size()) < cfg_.sequenceLen) return {0.333, 0.334, 0.333};

    // Build tensor [1, seq_len, input_size]
    auto t = torch::zeros({1, cfg_.sequenceLen, cfg_.inputSize});
    for (int i = 0; i < cfg_.sequenceLen; ++i) {
        for (int j = 0; j < cfg_.inputSize; ++j) {
            t[0][i][j] = static_cast<float>(seqBuffer_[i][j]);
        }
    }
    t = t.to(device_);
    net_->eval();
    torch::NoGradGuard ng;
    auto out = net_->forward(t);
    auto cpu = out.to(torch::kCPU);
    return {cpu[0][0].item<double>(),
            cpu[0][1].item<double>(),
            cpu[0][2].item<double>()};
}

void LSTMModel::train(const std::vector<std::vector<double>>& X,
                      const std::vector<int>& y,
                      int epochs, double lr) {
    if (X.empty() || X.size() != y.size()) return;
    net_->train();
    torch::optim::Adam opt(net_->parameters(),
        torch::optim::AdamOptions(lr).weight_decay(1e-5));

    // Build sequence dataset
    int seqLen = cfg_.sequenceLen;
    int nSamples = static_cast<int>(X.size()) - seqLen;
    if (nSamples <= 0) return;

    double bestLoss = 1e9;
    int patience = 5, noImprove = 0;

    for (int ep = 0; ep < epochs; ++ep) {
        double totalLoss = 0.0;
        for (int i = 0; i < nSamples; ++i) {
            auto inp = torch::zeros({1, seqLen, cfg_.inputSize});
            for (int s = 0; s < seqLen; ++s)
                for (int f = 0; f < cfg_.inputSize; ++f)
                    inp[0][s][f] = static_cast<float>(X[i + s][f]);
            inp = inp.to(device_);
            auto target = torch::tensor({y[i + seqLen]}, torch::kLong).to(device_);
            opt.zero_grad();
            auto out = net_->forward(inp);
            auto logProbs = torch::log_softmax(out, 1);
            auto loss = torch::nll_loss(logProbs, target);
            loss.backward();
            opt.step();
            totalLoss += loss.item<double>();
        }
        double avgLoss = totalLoss / nSamples;
        Logger::get()->debug("LSTM epoch {}/{} loss={:.4f}", ep + 1, epochs, avgLoss);
        if (avgLoss < bestLoss) { bestLoss = avgLoss; noImprove = 0; }
        else if (++noImprove >= patience) { Logger::get()->info("LSTM early stop at epoch {}", ep + 1); break; }
    }
    ready_ = true;
}

void LSTMModel::save(const std::string& path) const {
    if (!ready_) return;
    torch::save(net_, path);
}

bool LSTMModel::load(const std::string& path) {
    try {
        torch::load(net_, path, device_);
        ready_ = true;
        return true;
    } catch (const std::exception& e) {
        Logger::get()->warn("LSTM load failed: {}", e.what());
        return false;
    }
}

bool LSTMModel::ready() const { return ready_; }

#else

// Stub implementation when LibTorch is not available
LSTMModel::LSTMModel(const LSTMConfig& cfg) : cfg_(cfg) {}
LSTMModel::~LSTMModel() = default;
void LSTMModel::addSample(const std::vector<double>&) {}
std::vector<double> LSTMModel::predict(const std::vector<double>&) {
    return {0.333, 0.334, 0.333};
}
void LSTMModel::train(const std::vector<std::vector<double>>&,
                      const std::vector<int>&, int, double) {
    Logger::get()->warn("LSTM: LibTorch not available, using stub");
}
void LSTMModel::save(const std::string&) const {}
bool LSTMModel::load(const std::string&) { return false; }
bool LSTMModel::ready() const { return false; }

#endif

} // namespace crypto
