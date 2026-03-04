#pragma once
#include <string>
#include <mutex>
#include <thread>
#include <atomic>

namespace crypto {

struct FearGreedData {
    int         value{50};           // 0-100
    std::string classification{"Neutral"};
    long long   timestamp{0};
};

class FearGreed {
public:
    FearGreed() = default;
    ~FearGreed();

    void start();
    void stop();
    bool isRunning() const { return running_; }

    FearGreedData getData() const;
    void setData(const FearGreedData& d); // For testing / manual injection

private:
    void loop();
    void fetch();

    FearGreedData       data_;
    mutable std::mutex  mutex_;
    std::thread         pollThread_;
    std::atomic<bool>   running_{false};
};

} // namespace crypto
