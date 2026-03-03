#pragma once
#include <atomic>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

namespace crypto {

class Engine {
public:
    explicit Engine(const std::string& configPath);
    ~Engine();

    void run();
    void stop();

private:
    void loadConfig(const std::string& path);
    void initComponents();
    void mainLoop();

    nlohmann::json config_;
    std::atomic<bool> running_{false};

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace crypto
