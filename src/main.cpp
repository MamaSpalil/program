#include "core/Engine.h"
#include "core/Logger.h"
#include <csignal>
#include <atomic>
#include <iostream>
#include <memory>
#include <filesystem>
#include <locale>
#include <clocale>

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

    std::signal(SIGINT,  signalHandler);
    std::signal(SIGTERM, signalHandler);

#ifdef _WIN32
    SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
#endif

    try {
        crypto::Logger::init();
        crypto::Logger::get()->info("CryptoTrader GUI starting...");

        auto gui = std::make_unique<crypto::AppGui>();
        if (!gui->init(configPath)) {
            std::cerr << "Failed to initialize GUI\n";
            return 1;
        }

        std::unique_ptr<crypto::Engine> engine;

        // Wire up GUI callbacks to engine
        gui->setConnectCallback([&](const crypto::GuiConfig& cfg) {
            gui->addLog("[Info] Connecting to " + cfg.exchangeName + "...");
            try {
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

        gui->addLog("[Info] Crypto ML Trader v1.0.0 ready");
        gui->addLog("[Info] Configure exchange settings and press Connect");

        // Run the GUI event loop (blocks until window close)
        gui->run();

        // Cleanup
        if (engine) {
            engine->stop();
            engine.reset();
        }
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

        auto engine = std::make_unique<crypto::Engine>(configPath);
        g_engine = engine.get();
        engine->run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
#endif
