#pragma once
#include "../exchange/IExchange.h"
#include <atomic>
#include <memory>
#include <string>
#include <map>
#include <nlohmann/json.hpp>

namespace crypto {

class Engine {
public:
    explicit Engine(const std::string& configPath);
    ~Engine();

    void run();
    void stop();

    // Initialize exchange and verify connection. Returns true if API test succeeds.
    // On failure, sets outError with a description. Must be called before run().
    bool initAndTestConnection(std::string& outError);

    // Place a manual order through the exchange
    OrderResponse placeOrder(const OrderRequest& req);

    // Get user indicator plots (thread-safe snapshot)
    std::map<std::string, std::map<std::string, double>> getUserIndicatorPlots() const;

private:
    void loadConfig(const std::string& path);
    void initComponents();
    void mainLoop();

    nlohmann::json config_;
    std::atomic<bool> running_{false};
    bool componentsInitialized_{false};

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace crypto
