#include "core/Engine.h"
#include "core/Logger.h"
#include <csignal>
#include <atomic>
#include <iostream>
#include <memory>
#include <filesystem>
#include <locale>
#include <clocale>
#include <chrono>
#include <nlohmann/json.hpp>

#include "data/TradingDatabase.h"
#include "data/TradeRepository.h"
#include "data/OrderRepository.h"
#include "data/PositionRepository.h"
#include "data/EquityRepository.h"
#include "data/AlertRepository.h"
#include "data/BacktestRepository.h"
#include "data/DrawingRepository.h"
#include "data/AuxRepository.h"
#include "data/DatabaseMigrator.h"
#include "data/DatabaseMaintenance.h"
#include "integration/TelegramBot.h"
#include "exchange/SymbolFormatter.h"
#include <future>

#ifdef USE_IMGUI
#include "ui/AppGui.h"
#include <thread>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

namespace {
std::atomic<bool> g_shutdown{false};
crypto::Engine*   g_engine{nullptr};

// Resolve config path: try as-is, then relative to executable directory
std::string resolveConfigPath(const std::string& path, const char* argv0) {
    namespace fs = std::filesystem;
    if (fs::exists(path)) return path;

    fs::path exeDir = fs::path(argv0).parent_path();
    if (!exeDir.empty()) {
        // Try relative to executable, then 1-2 levels up (build/<preset>/ layouts)
        fs::path base = exeDir;
        for (int i = 0; i <= 2; ++i) {
            fs::path altPath = base / path;
            if (fs::exists(altPath)) return fs::weakly_canonical(altPath).string();
            base = base / "..";
        }
    }

    return path; // return original; Engine will report the error
}

void signalHandler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        g_shutdown = true;
        if (g_engine) g_engine->stop();
    }
}

#ifdef _WIN32
BOOL WINAPI consoleCtrlHandler(DWORD ctrlType) {
    if (ctrlType == CTRL_C_EVENT || ctrlType == CTRL_CLOSE_EVENT) {
        g_shutdown = true;
        if (g_engine) g_engine->stop();
        return TRUE;
    }
    return FALSE;
}
#endif
} // namespace

#ifdef USE_IMGUI
// ── GUI mode ─────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    std::setlocale(LC_ALL, "en_US.UTF-8");

    std::string configPath = "config/settings.json";
    if (argc > 1) configPath = argv[1];
    configPath = resolveConfigPath(configPath, argv[0]);

    // Compute executable directory for Config.ini placement
    std::string exeDir;
    {
        namespace fs = std::filesystem;
#ifdef _WIN32
        // On Windows, use GetModuleFileName for a reliable absolute path
        wchar_t exeBuf[4096]{};
        DWORD len = GetModuleFileNameW(NULL, exeBuf, 4096);
        if (len > 0 && len < 4096) {
            exeDir = fs::path(exeBuf).parent_path().string();
        }
#else
        // On Linux, resolve /proc/self/exe symlink
        std::error_code ec;
        auto selfExe = fs::read_symlink("/proc/self/exe", ec);
        if (!ec) {
            exeDir = selfExe.parent_path().string();
        }
#endif
        // Fallback: derive from argv[0]
        if (exeDir.empty()) {
            fs::path p = fs::path(argv[0]).parent_path();
            if (!p.empty()) {
                std::error_code ec2;
                auto canon = fs::weakly_canonical(p, ec2);
                exeDir = ec2 ? p.string() : canon.string();
            }
        }
    }

    std::signal(SIGINT,  signalHandler);
    std::signal(SIGTERM, signalHandler);

#ifdef _WIN32
    SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
#endif

    try {
        crypto::Logger::init();
        crypto::Logger::get()->info("CryptoTrader GUI starting...");

        // ── Initialize Trading Database ──────────────────────────────
        // Ensure config directory exists for first-launch DB creation
        auto dbDir = std::filesystem::path(configPath).parent_path();
        if (!dbDir.empty() && !std::filesystem::exists(dbDir)) {
            std::filesystem::create_directories(dbDir);
            crypto::Logger::get()->info("Created config directory: {}", dbDir.string());
        }

        std::string dbPath = (dbDir / "trading.db").string();
        auto tradingDb = std::make_shared<crypto::TradingDatabase>(dbPath);
        if (!tradingDb->init()) {
            crypto::Logger::get()->error("Failed to initialize trading database");
        } else {
            crypto::Logger::get()->info("Trading database initialized: {}", dbPath);
        }

        // Create repositories
        auto tradeRepo    = std::make_shared<crypto::TradeRepository>(*tradingDb);
        auto orderRepo    = std::make_shared<crypto::OrderRepository>(*tradingDb);
        auto positionRepo = std::make_shared<crypto::PositionRepository>(*tradingDb);
        auto equityRepo   = std::make_shared<crypto::EquityRepository>(*tradingDb);
        auto alertRepo    = std::make_shared<crypto::AlertRepository>(*tradingDb);
        auto backtestRepo = std::make_shared<crypto::BacktestRepository>(*tradingDb);
        auto drawingRepo  = std::make_shared<crypto::DrawingRepository>(*tradingDb);
        auto auxRepo      = std::make_shared<crypto::AuxRepository>(*tradingDb);

        // Run migration asynchronously (non-blocking)
        auto migrator = std::make_shared<crypto::DatabaseMigrator>(*tradingDb, *tradeRepo, *alertRepo, *drawingRepo);
        auto migrationFuture = std::async(std::launch::async, [migrator]() {
            migrator->runIfNeeded();
        });

        // Start database maintenance (background cleanup thread)
        auto maintenance = std::make_shared<crypto::DatabaseMaintenance>(*tradingDb, *equityRepo, *orderRepo, *auxRepo);
        maintenance->start();

        auto gui = std::make_unique<crypto::AppGui>();
        if (!gui->init(configPath, exeDir)) {
            std::cerr << "Failed to initialize GUI\n";
            return 1;
        }

        // Wire repositories to GUI-owned modules
        gui->setDatabaseRepositories(
            tradeRepo.get(), alertRepo.get(), backtestRepo.get(),
            positionRepo.get(), drawingRepo.get(), auxRepo.get(),
            equityRepo.get());

        std::unique_ptr<crypto::Engine> engine;
        auto telegramBot = std::make_shared<crypto::TelegramBot>();

        // Wire up GUI callbacks to engine
        gui->setConnectCallback([&](const crypto::GuiConfig& cfg) {
            gui->addLog("[Info] Connecting to " + cfg.exchangeName + "...");
            try {
                // Save current GUI settings so Engine reads user-entered values
                gui->saveConfigToFile(configPath);

                engine = std::make_unique<crypto::Engine>(configPath);
                g_engine = engine.get();

                // Verify API connection before reporting success
                std::string connError;
                bool connected = engine->initAndTestConnection(connError);

                if (!connected) {
                    gui->addLog("[Error] API connection test failed: " + connError);
                    crypto::GuiState st;
                    st.connected = false;
                    st.trading = false;
                    st.statusMessage = "Connection failed: " + connError;
                    gui->updateState(st);
                    engine.reset();
                    g_engine = nullptr;
                    return;
                }

                gui->addLog("[OK] API connection verified");

                // Fetch available trading pairs in background
                try {
                    auto symbols = engine->getSymbols(cfg.marketType);
                    if (!symbols.empty()) {
                        crypto::GuiState st;
                        {
                            // Merge symbols into existing state
                            st = crypto::GuiState{};
                            st.availableSymbols = std::move(symbols);
                            st.connected = true;
                            st.trading = false;
                            st.statusMessage = "Loading symbols...";
                        }
                        gui->updateState(st);
                        gui->addLog("[OK] Loaded " + std::to_string(st.availableSymbols.size()) + " trading pairs");
                    }
                } catch (...) {
                    gui->addLog("[Warn] Could not fetch trading pairs");
                }

                // Set market type on exchange
                engine->setMarketType(cfg.marketType);

                // ── Wire TelegramBot commands to Engine ──────────────
                {
                    // Read telegram config from settings
                    std::ifstream tgf(configPath);
                    if (tgf.is_open()) {
                        try {
                            nlohmann::json tgCfg;
                            tgf >> tgCfg;
                            if (tgCfg.contains("telegram")) {
                                auto& tg = tgCfg["telegram"];
                                std::string token = tg.value("token", "");
                                std::string chatId = tg.value("chat_id", "");
                                if (!token.empty()) {
                                    telegramBot->setToken(token);
                                    if (!chatId.empty()) telegramBot->setAuthorizedChat(chatId);

                                    // Register real command callbacks
                                    // Capture cfg fields by value to avoid dangling reference
                                    // (cfg is a parameter of the connect callback lambda)
                                    std::string tgExchangeName = cfg.exchangeName;
                                    std::string tgSymbol = cfg.symbol;
                                    std::string tgInterval = cfg.interval;
                                    std::string tgMode = cfg.mode;
                                    std::string tgMarketType = cfg.marketType;

                                    telegramBot->setCommandCallback("/balance", [&engine]() -> std::string {
                                        if (!engine) return "Engine not running";
                                        try {
                                            auto bal = engine->getFuturesBalance();
                                            return "Balance: " + std::to_string(bal.totalWalletBalance) +
                                                   " USDT\nMargin: " + std::to_string(bal.totalMarginBalance) +
                                                   "\nUnrealized PnL: " + std::to_string(bal.totalUnrealizedProfit);
                                        } catch (...) {
                                            return "Balance fetch failed";
                                        }
                                    });

                                    telegramBot->setCommandCallback("/status", [&engine, tgExchangeName, tgSymbol, tgInterval, tgMode, tgMarketType]() -> std::string {
                                        if (!engine) return "Engine not running";
                                        return "Connected to " + tgExchangeName +
                                               "\nSymbol: " + tgSymbol +
                                               "\nInterval: " + tgInterval +
                                               "\nMode: " + tgMode +
                                               "\nMarket: " + tgMarketType;
                                    });

                                    telegramBot->setCommandCallback("/positions", [&engine, tgSymbol]() -> std::string {
                                        if (!engine) return "Engine not running";
                                        try {
                                            auto positions = engine->getPositionRisk(tgSymbol);
                                            if (positions.empty()) return "No open positions";
                                            std::string result;
                                            for (auto& p : positions) {
                                                result += p.symbol +
                                                          " qty=" + std::to_string(p.positionAmt) +
                                                          " entry=" + std::to_string(p.entryPrice) +
                                                          " pnl=" + std::to_string(p.unrealizedProfit) + "\n";
                                            }
                                            return result;
                                        } catch (...) {
                                            return "Positions fetch failed";
                                        }
                                    });

                                    telegramBot->setCommandCallback("/pnl", [&engine]() -> std::string {
                                        if (!engine) return "Engine not running";
                                        double pnl = engine->totalPnl();
                                        double wr = engine->winRate();
                                        return "Total P&L: " + std::to_string(pnl) +
                                               " USDT\nWin Rate: " + std::to_string(wr * 100.0) + "%";
                                    });

                                    telegramBot->startPolling();
                                    gui->addLog("[OK] TelegramBot started (chat: " + chatId + ")");
                                }
                            }
                        } catch (...) {
                            // Telegram config parsing failed — non-critical
                        }
                    }
                }

                // Show User Panel automatically after successful connection
                // (the panel visibility flag is set from within the GUI)

                // Wire engine updates to the GUI so chart and indicators are displayed
                static constexpr int kPricePollIntervalMs = 100;
                static constexpr int kUserDataPollIntervalSec = 5;

                // Capture cfg fields by value to avoid dangling reference:
                // cfg is a callback parameter that goes out of scope when the
                // connect callback returns, but the onUpdate callback is invoked
                // repeatedly from the engine thread long after that.
                std::string cfgExchangeName  = cfg.exchangeName;
                std::string cfgSymbol        = cfg.symbol;
                std::string cfgInterval      = cfg.interval;
                std::string cfgMarketType    = cfg.marketType;
                double      cfgInitialCapital = cfg.initialCapital;

                engine->setOnUpdateCallback([&gui, cfgExchangeName, cfgSymbol, cfgInterval,
                                             cfgMarketType, cfgInitialCapital, &engine](const crypto::EngineUpdate& upd) {
                    static auto lastPriceTime = std::chrono::steady_clock::now();
                    static auto lastUserDataTime = std::chrono::steady_clock::time_point{};
                    auto now = std::chrono::steady_clock::now();

                    crypto::GuiState st;
                    st.connected = true;
                    st.trading   = true;
                    st.statusMessage = "Connected to " + cfgExchangeName +
                                       " (" + cfgSymbol + " " + cfgInterval + ")";
                    st.lastCandle    = upd.candle;
                    st.candleHistory = upd.candleHistory;
                    st.emaFast  = upd.emaFast;
                    st.emaSlow  = upd.emaSlow;
                    st.emaTrend = upd.emaTrend;
                    st.rsi      = upd.rsi;
                    st.atr      = upd.atr;
                    st.macd     = upd.macd;
                    st.bb       = upd.bb;
                    st.lastSignal = upd.signal;
                    st.equity    = upd.equity;
                    st.initialCapital = cfgInitialCapital;
                    st.drawdown  = upd.drawdown;
                    st.userIndicatorPlots = upd.userIndicatorPlots;
                    st.orderBook = upd.orderBook;

                    // Use last candle close as current price immediately (tick update)
                    if (upd.candle.close > 0) {
                        st.currentPrice = upd.candle.close;
                    }

                    // Fetch trading pairs once (kept across updates)
                    if (engine) {
                        st.trades   = engine->listTrades();
                        st.totalPnl = engine->totalPnl();
                        st.winRate  = engine->winRate();

                        // Fetch precise price periodically (fast tick update)
                        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPriceTime).count() >= kPricePollIntervalMs) {
                            try {
                                double p = engine->getPrice(cfgSymbol);
                                if (p > 0) st.currentPrice = p;
                            } catch (const std::exception& e) {
                                // Use candle close as fallback, no error display
                                crypto::Logger::get()->debug("Price fetch: {}", e.what());
                            } catch (...) {
                                // Use candle close as fallback
                            }
                            lastPriceTime = now;
                        }

                        // Fetch user data periodically
                        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastUserDataTime).count() >= kUserDataPollIntervalSec) {
                            try {
                                st.openOrders = engine->getOpenOrders(cfgSymbol);
                                st.userTrades = engine->getMyTrades(cfgSymbol, 20);
                                if (cfgMarketType == "futures") {
                                    st.futuresBalance = engine->getFuturesBalance();
                                    st.futuresPositions = engine->getPositionRisk(cfgSymbol);
                                } else {
                                    st.accountBalanceDetails = engine->getAccountBalanceDetails();
                                }
                            } catch (...) {
                                // Silently skip user data errors
                            }
                            lastUserDataTime = now;
                        }
                    }

                    gui->updateState(st);
                });

                // Start the engine on a background thread
                std::thread([&]() {
                    try {
                        engine->run();
                    } catch (const std::exception& e) {
                        gui->addLog(std::string("[Error] Engine: ") + e.what());
                    }
                }).detach();

                crypto::GuiState st;
                st.connected = true;
                st.trading = true;
                st.statusMessage = "Connected to " + cfg.exchangeName +
                                   " (" + cfg.symbol + " " + cfg.interval + ")";
                st.equity = cfg.initialCapital;
                st.initialCapital = cfg.initialCapital;
                gui->updateState(st);
                gui->addLog("[OK] Connected successfully");
            } catch (const std::exception& e) {
                gui->addLog(std::string("[Error] Connection failed: ") + e.what());
            }
        });

        gui->setDisconnectCallback([&]() {
            gui->addLog("[Info] Disconnecting...");
            telegramBot->stopPolling();
            if (engine) {
                engine->stop();
                engine.reset();
                g_engine = nullptr;
            }
            crypto::GuiState st;
            st.connected = false;
            st.trading = false;
            st.statusMessage = "Disconnected";
            gui->updateState(st);
            gui->addLog("[OK] Disconnected");
        });

        gui->setStartCallback([&]() {
            gui->addLog("[Info] Trading started");
        });

        gui->setStopCallback([&]() {
            gui->addLog("[Info] Trading stopped");
        });

        gui->setOrderCallback([&](const std::string& symbol,
                                   const std::string& side,
                                   const std::string& type,
                                   double qty,
                                   double price) {
            if (!engine) {
                gui->addLog("[Error] Engine not running");
                return;
            }
            try {
                crypto::OrderRequest req;
                req.symbol = symbol;
                req.side   = (side == "BUY") ? crypto::OrderRequest::Side::BUY
                                             : crypto::OrderRequest::Side::SELL;
                req.type   = (type == "LIMIT") ? crypto::OrderRequest::Type::LIMIT
                                               : crypto::OrderRequest::Type::MARKET;
                req.qty    = qty;
                req.price  = price;
                auto resp = engine->placeOrder(req);
                if (!resp.orderId.empty()) {
                    gui->addLog("[OK] Order " + resp.orderId + " " + resp.status +
                                " qty=" + std::to_string(resp.executedQty));
                    // Record trade in database
                    crypto::TradeRecord tr;
                    tr.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                    tr.symbol    = symbol;
                    tr.side      = side;
                    tr.orderType = type;
                    tr.qty       = resp.executedQty;
                    tr.price     = resp.executedPrice;
                    engine->recordTrade(tr);

                    // Also persist order to trading database
                    crypto::OrderRecord dbOrder;
                    dbOrder.id = resp.orderId;
                    dbOrder.symbol = symbol;
                    dbOrder.exchange = gui->getConfig().exchangeName;
                    dbOrder.side = side;
                    dbOrder.type = type;
                    dbOrder.qty = qty;
                    dbOrder.price = price;
                    dbOrder.filledQty = resp.executedQty;
                    dbOrder.avgFillPrice = resp.executedPrice;
                    dbOrder.status = resp.status;
                    dbOrder.isPaper = (gui->getConfig().mode == "paper");
                    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                    dbOrder.createdAt = now;
                    dbOrder.updatedAt = now;
                    orderRepo->insert(dbOrder);

                    // Persist trade to trading database
                    crypto::TradingRecord dbTrade;
                    dbTrade.id = resp.orderId;
                    dbTrade.symbol = symbol;
                    dbTrade.exchange = gui->getConfig().exchangeName;
                    dbTrade.side = side;
                    dbTrade.type = type;
                    dbTrade.qty = resp.executedQty;
                    dbTrade.entryPrice = resp.executedPrice;
                    dbTrade.isPaper = (gui->getConfig().mode == "paper");
                    dbTrade.status = "open";
                    dbTrade.entryTime = tr.timestamp;
                    dbTrade.createdAt = tr.timestamp;
                    dbTrade.updatedAt = tr.timestamp;
                    tradeRepo->upsert(dbTrade);
                } else {
                    gui->addLog("[Error] Order failed: " + resp.status);
                }
            } catch (const std::exception& e) {
                gui->addLog(std::string("[Error] Order: ") + e.what());
            }
        });

        // Load pairs callback — reload pair list when market type changes
        gui->setLoadPairsCallback([&](const std::string& marketType) {
            if (!engine) return;
            std::thread([&gui, &engine, marketType]() {
                try {
                    engine->setMarketType(marketType);
                    auto symbols = engine->getSymbols(marketType);
                    crypto::GuiState st;
                    st.availableSymbols = std::move(symbols);
                    st.connected = true;
                    st.trading = true;
                    gui->updateState(st);
                    gui->addLog("[OK] Reloaded " + std::to_string(st.availableSymbols.size()) + " pairs for " + marketType);
                } catch (const std::exception& e) {
                    gui->addLog(std::string("[Error] Load pairs: ") + e.what());
                }
            }).detach();
        });

        // Refresh data callback — reload candles when pair or timeframe changes
        gui->setRefreshDataCallback([&]() {
            if (!engine) return;
            auto cfg = gui->getConfig();
            engine->setMarketType(cfg.marketType);
            std::thread([&gui, &engine, symbol = cfg.symbol, interval = cfg.interval]() {
                try {
                    engine->reloadCandles(symbol, interval);
                    gui->addLog("[OK] Chart reloaded for " + symbol + " " + interval);
                } catch (const std::exception& e) {
                    gui->addLog(std::string("[Error] Reload: ") + e.what());
                }
            }).detach();
        });

        gui->addLog("[Info] Crypto ML Trader v2.6.0 ready");
        gui->addLog("[Info] Configure exchange settings and press Connect");

        // Run the GUI event loop (blocks until window close)
        gui->run();

        // Cleanup
        telegramBot->stopPolling();
        if (engine) {
            engine->stop();
            engine.reset();
        }
        // Stop database maintenance
        maintenance->stop();
        
        // Flush equity buffer
        equityRepo->flush();

        gui->shutdown();

    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
#else
// ── Console mode (fallback when ImGui not available) ─────────────────────────
int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    std::setlocale(LC_ALL, "en_US.UTF-8");

    std::string configPath = "config/settings.json";
    if (argc > 1) configPath = argv[1];
    configPath = resolveConfigPath(configPath, argv[0]);

    std::signal(SIGINT,  signalHandler);
    std::signal(SIGTERM, signalHandler);

#ifdef _WIN32
    SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
#endif

    try {
        crypto::Logger::init();
        crypto::Logger::get()->info("CryptoTrader starting (console mode)...");

        // ── Initialize Trading Database ──────────────────────────────
        // Ensure config directory exists for first-launch DB creation
        auto dbDir = std::filesystem::path(configPath).parent_path();
        if (!dbDir.empty() && !std::filesystem::exists(dbDir)) {
            std::filesystem::create_directories(dbDir);
            crypto::Logger::get()->info("Created config directory: {}", dbDir.string());
        }

        std::string dbPath = (dbDir / "trading.db").string();
        auto tradingDb = std::make_shared<crypto::TradingDatabase>(dbPath);
        if (!tradingDb->init()) {
            crypto::Logger::get()->error("Failed to initialize trading database");
        } else {
            crypto::Logger::get()->info("Trading database initialized: {}", dbPath);
        }

        auto tradeRepo    = std::make_shared<crypto::TradeRepository>(*tradingDb);
        auto orderRepo    = std::make_shared<crypto::OrderRepository>(*tradingDb);
        auto positionRepo = std::make_shared<crypto::PositionRepository>(*tradingDb);
        auto equityRepo   = std::make_shared<crypto::EquityRepository>(*tradingDb);
        auto alertRepo    = std::make_shared<crypto::AlertRepository>(*tradingDb);
        auto backtestRepo = std::make_shared<crypto::BacktestRepository>(*tradingDb);
        auto drawingRepo  = std::make_shared<crypto::DrawingRepository>(*tradingDb);
        auto auxRepo      = std::make_shared<crypto::AuxRepository>(*tradingDb);

        // Run migration
        crypto::DatabaseMigrator migrator(*tradingDb, *tradeRepo, *alertRepo, *drawingRepo);
        migrator.runIfNeeded();

        // Start maintenance
        auto maintenance = std::make_shared<crypto::DatabaseMaintenance>(*tradingDb, *equityRepo, *orderRepo, *auxRepo);
        maintenance->start();

        auto engine = std::make_unique<crypto::Engine>(configPath);
        g_engine = engine.get();
        engine->run();

        maintenance->stop();
        equityRepo->flush();
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
#endif
