#include "Engine.h"
#include "Logger.h"
#include "../exchange/BinanceExchange.h"
#include "../exchange/BybitExchange.h"
#include "../data/DataFeed.h"
#include "../strategy/MLEnhancedStrategy.h"
#include "../ui/Dashboard.h"
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>

namespace crypto {

struct Engine::Impl {
    std::unique_ptr<IExchange>          exchange;
    std::unique_ptr<DataFeed>           feed;
    std::unique_ptr<MLEnhancedStrategy> strategy;
    std::unique_ptr<Dashboard>          dashboard;
};

Engine::Engine(const std::string& configPath)
    : impl_(std::make_unique<Impl>()) {
    loadConfig(configPath);
    Logger::init();
}

Engine::~Engine() {
    stop();
}

void Engine::loadConfig(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open())
        throw std::runtime_error("Cannot open config: " + path);
    f >> config_;
}

void Engine::initComponents() {
    auto& ex = config_["exchange"];
    std::string name    = ex.value("name", "binance");
    std::string apiKey  = ex.value("api_key", "");
    std::string apiSec  = ex.value("api_secret", "");
    std::string baseUrl = ex.value("base_url", "https://testnet.binance.vision");

    if (name == "bybit") {
        impl_->exchange = std::make_unique<BybitExchange>(apiKey, apiSec, baseUrl);
    } else {
        impl_->exchange = std::make_unique<BinanceExchange>(apiKey, apiSec, baseUrl);
    }

    impl_->feed     = std::make_unique<DataFeed>(500);
    impl_->strategy = std::make_unique<MLEnhancedStrategy>(
        config_, config_["trading"].value("initial_capital", 1000.0));
    impl_->dashboard = std::make_unique<Dashboard>();
}

void Engine::run() {
    initComponents();
    running_ = true;

    auto& trading = config_["trading"];
    std::string symbol   = trading.value("symbol", "BTCUSDT");
    std::string interval = trading.value("interval", "15m");

    Logger::get()->info("Starting engine: {} {} mode={}",
        symbol, interval, trading.value("mode", "paper"));

    // Load historical candles first
    try {
        auto hist = impl_->exchange->getKlines(symbol, interval, 500);
        Logger::get()->info("Loaded {} historical candles", hist.size());
        for (auto& c : hist) {
            impl_->feed->onCandle(c);
            impl_->strategy->onCandle(c);
        }
    } catch (const std::exception& e) {
        Logger::get()->warn("Historical load failed: {}", e.what());
    }

    // Subscribe to live stream
    impl_->exchange->subscribeKline(symbol, interval, [this](const Candle& c) {
        impl_->feed->onCandle(c);
        impl_->strategy->onCandle(c);
    });
    impl_->exchange->connect();
    impl_->dashboard->start();

    mainLoop();
}

void Engine::mainLoop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void Engine::stop() {
    running_ = false;
    if (impl_->exchange) impl_->exchange->disconnect();
    if (impl_->dashboard) impl_->dashboard->stop();
    Logger::get()->info("Engine stopped");
}

} // namespace crypto
