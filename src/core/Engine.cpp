#include "Engine.h"
#include "Logger.h"
#include "../exchange/BinanceExchange.h"
#include "../exchange/BybitExchange.h"
#include "../exchange/OKXExchange.h"
#include "../exchange/BitgetExchange.h"
#include "../exchange/KuCoinExchange.h"
#include "../data/DataFeed.h"
#include "../data/ExchangeDB.h"
#include "../strategy/MLEnhancedStrategy.h"
#include "../indicators/UserIndicatorManager.h"
#include "../ui/Dashboard.h"
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>

namespace crypto {

struct Engine::Impl {
    std::unique_ptr<IExchange>              exchange;
    std::unique_ptr<DataFeed>               feed;
    std::unique_ptr<MLEnhancedStrategy>     strategy;
    std::unique_ptr<Dashboard>              dashboard;
    std::unique_ptr<UserIndicatorManager>   userIndicators;
};

Engine::Engine(const std::string& configPath)
    : impl_(std::make_unique<Impl>()) {
    loadConfig(configPath);
    Logger::init();

    // Enable debug mode if configured
    bool debug = false;
    if (config_.contains("debug")) debug = config_["debug"].get<bool>();
    if (debug) Logger::setDebugMode(true);
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
    std::string name       = ex.value("name", "binance");
    std::string apiKey     = ex.value("api_key", "");
    std::string apiSec     = ex.value("api_secret", "");
    std::string passphrase = ex.value("passphrase", "");
    std::string baseUrl    = ex.value("base_url", "");

    Logger::get()->debug("[Engine] Creating exchange: name={} baseUrl={}", name, baseUrl);

    if (name == "bybit") {
        if (baseUrl.empty()) baseUrl = "https://api-testnet.bybit.com";
        impl_->exchange = std::make_unique<BybitExchange>(apiKey, apiSec, baseUrl);
    } else if (name == "okx") {
        if (baseUrl.empty()) baseUrl = "https://www.okx.com";
        impl_->exchange = std::make_unique<OKXExchange>(apiKey, apiSec, passphrase, baseUrl);
    } else if (name == "bitget") {
        if (baseUrl.empty()) baseUrl = "https://api.bitget.com";
        impl_->exchange = std::make_unique<BitgetExchange>(apiKey, apiSec, passphrase, baseUrl);
    } else if (name == "kucoin") {
        if (baseUrl.empty()) baseUrl = "https://api.kucoin.com";
        impl_->exchange = std::make_unique<KuCoinExchange>(apiKey, apiSec, passphrase, baseUrl);
    } else {
        // default: binance
        if (baseUrl.empty()) baseUrl = "https://testnet.binance.vision";
        impl_->exchange = std::make_unique<BinanceExchange>(apiKey, apiSec, baseUrl);
    }

    // Verify connection with a test API call
    std::string connError;
    if (impl_->exchange->testConnection(connError)) {
        Logger::get()->info("[Engine] Exchange '{}' connection verified", name);
    } else {
        Logger::get()->warn("[Engine] Exchange '{}' connection test failed: {}", name, connError);
    }

    // Save credentials to ExchangeDB for persistence
    ExchangeDB db;
    db.load();
    ExchangeProfile prof;
    prof.name         = name;
    prof.exchangeType = name;
    prof.apiKey       = apiKey;
    prof.apiSecret    = apiSec;
    prof.passphrase   = passphrase;
    prof.baseUrl      = baseUrl;
    prof.testnet      = ex.value("testnet", false);
    db.upsertProfile(prof);
    db.setActiveProfile(name);
    db.save();

    impl_->feed     = std::make_unique<DataFeed>(500);
    impl_->strategy = std::make_unique<MLEnhancedStrategy>(
        config_, config_["trading"].value("initial_capital", 1000.0));
    impl_->dashboard = std::make_unique<Dashboard>();

    // Load user Pine Script indicators
    std::string indicatorDir = config_.value("user_indicator_dir", "user_indicator");
    impl_->userIndicators = std::make_unique<UserIndicatorManager>(indicatorDir);
    impl_->userIndicators->scanAndLoad();
    auto loaded = impl_->userIndicators->loadedIndicators();
    if (!loaded.empty()) {
        Logger::get()->info("Loaded {} user indicator(s)", loaded.size());
        for (auto& name : loaded) Logger::get()->info("  - {}", name);
    }

    componentsInitialized_ = true;
}

bool Engine::initAndTestConnection(std::string& outError) {
    try {
        initComponents();
        return impl_->exchange->testConnection(outError);
    } catch (const std::exception& e) {
        outError = e.what();
        return false;
    }
}

void Engine::run() {
    if (!componentsInitialized_) {
        initComponents();
    }
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
            if (impl_->userIndicators) impl_->userIndicators->updateAll(c);
        }
    } catch (const std::exception& e) {
        Logger::get()->warn("Historical load failed: {}", e.what());
    }

    // Subscribe to live stream
    try {
        impl_->exchange->subscribeKline(symbol, interval, [this](const Candle& c) {
            impl_->feed->onCandle(c);
            impl_->strategy->onCandle(c);
            if (impl_->userIndicators) impl_->userIndicators->updateAll(c);
        });
        impl_->exchange->connect();
    } catch (const std::exception& e) {
        Logger::get()->warn("WebSocket connection failed: {}", e.what());
    }
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
    if (impl_) {
        if (impl_->exchange) impl_->exchange->disconnect();
        if (impl_->dashboard) impl_->dashboard->stop();
    }
    Logger::get()->info("Engine stopped");
}

} // namespace crypto
