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
    std::unique_ptr<ExchangeDB>             tradeDB;
    std::deque<Candle>                      candleHistory;
    OnUpdateCallback                        onUpdateCb;
    int                                     maxCandleHistory{1000};
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
    std::string wsHost     = ex.value("ws_host", "");
    std::string wsPort     = ex.value("ws_port", "");
    bool testnet           = ex.value("testnet", true);

    Logger::get()->debug("[Engine] Creating exchange: name={} baseUrl={} wsHost={} wsPort={} testnet={}",
                         name, baseUrl, wsHost, wsPort, testnet);

    if (name == "bybit") {
        if (baseUrl.empty()) baseUrl = testnet ? "https://api-testnet.bybit.com"      : "https://api.bybit.com";
        if (wsHost.empty())  wsHost  = testnet ? "stream-testnet.bybit.com"            : "stream.bybit.com";
        if (wsPort.empty())  wsPort  = "443";
        impl_->exchange = std::make_unique<BybitExchange>(apiKey, apiSec, baseUrl, wsHost, wsPort);
    } else if (name == "okx") {
        if (baseUrl.empty()) baseUrl = "https://www.okx.com";
        if (wsHost.empty())  wsHost  = testnet ? "wspap.okx.com"                       : "ws.okx.com";
        if (wsPort.empty())  wsPort  = "8443";
        impl_->exchange = std::make_unique<OKXExchange>(apiKey, apiSec, passphrase, baseUrl, wsHost, wsPort);
    } else if (name == "bitget") {
        if (baseUrl.empty()) baseUrl = testnet ? "https://api-simulated.bitget.com"    : "https://api.bitget.com";
        if (wsHost.empty())  wsHost  = "ws.bitget.com";
        if (wsPort.empty())  wsPort  = "443";
        impl_->exchange = std::make_unique<BitgetExchange>(apiKey, apiSec, passphrase, baseUrl, wsHost, wsPort);
    } else if (name == "kucoin") {
        if (baseUrl.empty()) baseUrl = testnet ? "https://api-sandbox.kucoin.com"      : "https://api.kucoin.com";
        if (wsHost.empty())  wsHost  = testnet ? "ws-api-sandbox.kucoin.com"           : "ws-api.kucoin.com";
        if (wsPort.empty())  wsPort  = "443";
        impl_->exchange = std::make_unique<KuCoinExchange>(apiKey, apiSec, passphrase, baseUrl, wsHost, wsPort);
    } else {
        // default: binance
        if (baseUrl.empty()) baseUrl = testnet ? "https://testnet.binance.vision"      : "https://api.binance.com";
        if (wsHost.empty())  wsHost  = testnet ? "testnet.binance.vision"              : "stream.binance.com";
        if (wsPort.empty())  wsPort  = testnet ? "443"                                 : "9443";
        impl_->exchange = std::make_unique<BinanceExchange>(apiKey, apiSec, baseUrl, wsHost, wsPort);
    }

    // Save credentials to a per-exchange ExchangeDB for persistence
    ExchangeDB db("config/db_" + name + ".json");
    db.load();
    ExchangeProfile prof;
    prof.name         = name;
    prof.exchangeType = name;
    prof.apiKey       = apiKey;
    prof.apiSecret    = apiSec;
    prof.passphrase   = passphrase;
    prof.baseUrl      = baseUrl;
    prof.wsHost       = wsHost;
    prof.wsPort       = wsPort;
    prof.testnet      = testnet;
    db.upsertProfile(prof);
    db.setActiveProfile(name);
    db.save();

    impl_->feed     = std::make_unique<DataFeed>(500);
    impl_->strategy = std::make_unique<MLEnhancedStrategy>(
        config_, config_["trading"].value("initial_capital", 1000.0));
    impl_->dashboard = std::make_unique<Dashboard>();

    // Trade statistics database (per-exchange)
    impl_->tradeDB = std::make_unique<ExchangeDB>("config/trades_" + name + ".json");
    impl_->tradeDB->load();

    // Load user Pine Script indicators
    std::string indicatorDir = config_.value("user_indicator_dir", "user_indicator");
    impl_->userIndicators = std::make_unique<UserIndicatorManager>(indicatorDir);
    impl_->userIndicators->scanAndLoad();
    auto loaded = impl_->userIndicators->loadedIndicators();
    if (!loaded.empty()) {
        Logger::get()->info("Loaded {} user indicator(s)", loaded.size());
        for (auto& ind : loaded) Logger::get()->info("  - {}", ind);
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

OrderResponse Engine::placeOrder(const OrderRequest& req) {
    if (!impl_ || !impl_->exchange)
        return OrderResponse{"", "error: exchange not initialized", 0.0, 0.0};
    return impl_->exchange->placeOrder(req);
}

void Engine::run() {
    if (!componentsInitialized_) {
        initComponents();
    }
    running_ = true;

    auto& trading = config_["trading"];
    std::string symbol   = trading.value("symbol", "BTCUSDT");
    std::string interval = trading.value("interval", "15m");

    // Set max candle history based on timeframe for deep analysis
    impl_->maxCandleHistory = maxBarsForTimeframe(interval);

    Logger::get()->info("Starting engine: {} {} mode={} maxBars={}",
        symbol, interval, trading.value("mode", "paper"),
        impl_->maxCandleHistory);

    // Load historical candles — request maximum available for deep analysis
    try {
        int requestLimit = impl_->maxCandleHistory;
        auto hist = impl_->exchange->getKlines(symbol, interval, requestLimit);
        Logger::get()->info("Loaded {} historical candles", hist.size());
        for (auto& c : hist) {
            impl_->feed->onCandle(c);
            impl_->strategy->onCandle(c);
            if (impl_->userIndicators) impl_->userIndicators->updateAll(c);
        }
        // Update dashboard with last historical candle state
        if (!hist.empty()) {
            updateDashboard(hist.back());
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
            updateDashboard(c);
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

void Engine::updateDashboard(const Candle& c) {
    if (!impl_->dashboard || !impl_->strategy) return;

    // Maintain candle history
    impl_->candleHistory.push_back(c);
    if (static_cast<int>(impl_->candleHistory.size()) > impl_->maxCandleHistory)
        impl_->candleHistory.pop_front();

    auto sig = impl_->strategy->getSignal();
    impl_->dashboard->update(c, impl_->strategy->indicators(),
                             impl_->strategy->riskManager(),
                             sig.value_or(Signal{}));

    // Push data to GUI callback
    if (impl_->onUpdateCb) {
        EngineUpdate upd;
        upd.candle = c;
        upd.candleHistory = impl_->candleHistory;

        auto& ind = impl_->strategy->indicators();
        upd.emaFast  = ind.emaFast();
        upd.emaSlow  = ind.emaSlow();
        upd.emaTrend = ind.emaTrend();
        upd.rsi      = ind.rsi();
        upd.atr      = ind.atr();
        upd.macd     = ind.macd();
        upd.bb       = ind.bb();
        upd.signal   = sig.value_or(Signal{});

        auto& rm = impl_->strategy->riskManager();
        upd.equity   = rm.equity();
        upd.drawdown = rm.drawdown();

        if (impl_->userIndicators)
            upd.userIndicatorPlots = impl_->userIndicators->getAllPlots();

        impl_->onUpdateCb(upd);
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

void Engine::setOnUpdateCallback(OnUpdateCallback cb) {
    if (impl_) impl_->onUpdateCb = std::move(cb);
}

std::map<std::string, std::map<std::string, double>> Engine::getUserIndicatorPlots() const {
    if (impl_ && impl_->userIndicators)
        return impl_->userIndicators->getAllPlots();
    return {};
}

std::vector<SymbolInfo> Engine::getSymbols(const std::string& marketType) const {
    if (impl_ && impl_->exchange)
        return impl_->exchange->getSymbols(marketType);
    return {};
}

void Engine::recordTrade(const TradeRecord& tr) {
    if (impl_ && impl_->tradeDB) {
        impl_->tradeDB->addTrade(tr);
        impl_->tradeDB->save();
    }
}

std::vector<TradeRecord> Engine::listTrades() const {
    if (impl_ && impl_->tradeDB)
        return impl_->tradeDB->listTrades();
    return {};
}

double Engine::totalPnl() const {
    if (impl_ && impl_->tradeDB)
        return impl_->tradeDB->totalPnl();
    return 0.0;
}

double Engine::winRate() const {
    if (impl_ && impl_->tradeDB)
        return impl_->tradeDB->winRate();
    return 0.0;
}

} // namespace crypto
