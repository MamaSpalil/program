#include "AppGui.h"
#include "../core/Logger.h"
#include "../indicators/PineConverter.h"
#include "../trading/OrderManagement.h"

#include "../../third_party/imgui/imgui.h"
#include "../../third_party/imgui/imgui_impl_glfw.h"
#include "../../third_party/imgui/imgui_impl_opengl3.h"

#ifdef _WIN32
#  include <windows.h>
#endif
#include <GLFW/glfw3.h>
#ifdef _WIN32
#  undef APIENTRY                // prevent C4005 if subsequent headers redefine the macro
#  define GLFW_EXPOSE_NATIVE_WIN32
#  include <GLFW/glfw3native.h>
#endif
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <ctime>

namespace crypto {

// ---------------------------------------------------------------------------
// Helper: dark-metal / dark-grey theme
// ---------------------------------------------------------------------------
static void ApplyDarkMetalTheme() {
    ImGuiStyle& s = ImGui::GetStyle();

    // Rounding & spacing — clean industrial look
    s.WindowRounding    = 2.0f;
    s.FrameRounding     = 2.0f;
    s.GrabRounding      = 1.0f;
    s.ScrollbarRounding = 2.0f;
    s.TabRounding       = 2.0f;
    s.ChildRounding     = 2.0f;
    s.PopupRounding     = 2.0f;
    s.WindowBorderSize  = 1.0f;
    s.FrameBorderSize   = 0.0f;
    s.FramePadding      = ImVec2(6, 4);
    s.ItemSpacing       = ImVec2(8, 5);
    s.ScrollbarSize     = 14.0f;
    s.GrabMinSize       = 10.0f;

    ImVec4* c = s.Colors;

    // Base dark-grey / metal palette
    ImVec4 bg        = ImVec4(0.11f, 0.11f, 0.12f, 1.00f); // #1C1C1F
    ImVec4 bgChild   = ImVec4(0.13f, 0.13f, 0.14f, 1.00f);
    ImVec4 panel     = ImVec4(0.16f, 0.16f, 0.17f, 1.00f); // #292929
    ImVec4 border    = ImVec4(0.28f, 0.28f, 0.30f, 0.60f);
    ImVec4 text      = ImVec4(0.82f, 0.83f, 0.84f, 1.00f);
    ImVec4 textDim   = ImVec4(0.50f, 0.50f, 0.52f, 1.00f);
    ImVec4 accent    = ImVec4(0.35f, 0.55f, 0.75f, 1.00f); // steel-blue
    ImVec4 accentH   = ImVec4(0.42f, 0.62f, 0.82f, 1.00f);
    ImVec4 header    = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    ImVec4 headerH   = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);
    ImVec4 headerA   = ImVec4(0.24f, 0.24f, 0.26f, 1.00f);
    ImVec4 btnCol    = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
    ImVec4 btnHov    = ImVec4(0.30f, 0.30f, 0.32f, 1.00f);
    ImVec4 btnAct    = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    ImVec4 tab       = ImVec4(0.15f, 0.15f, 0.16f, 1.00f);
    ImVec4 tabH      = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);
    ImVec4 tabA      = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    ImVec4 scrollbar = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    ImVec4 scrollG   = ImVec4(0.32f, 0.32f, 0.34f, 1.00f);
    ImVec4 scrollGH  = ImVec4(0.40f, 0.40f, 0.42f, 1.00f);
    ImVec4 scrollGA  = ImVec4(0.45f, 0.45f, 0.47f, 1.00f);

    // Window
    c[ImGuiCol_WindowBg]             = bg;
    c[ImGuiCol_ChildBg]              = bgChild;
    c[ImGuiCol_PopupBg]              = ImVec4(0.14f, 0.14f, 0.15f, 0.96f);
    c[ImGuiCol_Border]               = border;
    c[ImGuiCol_BorderShadow]         = ImVec4(0, 0, 0, 0);

    // Text
    c[ImGuiCol_Text]                 = text;
    c[ImGuiCol_TextDisabled]         = textDim;

    // Frame (input boxes, etc.)
    c[ImGuiCol_FrameBg]              = panel;
    c[ImGuiCol_FrameBgHovered]       = btnHov;
    c[ImGuiCol_FrameBgActive]        = btnAct;

    // Title bar
    c[ImGuiCol_TitleBg]              = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);
    c[ImGuiCol_TitleBgActive]        = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
    c[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.08f, 0.08f, 0.09f, 0.80f);

    // Menu bar
    c[ImGuiCol_MenuBarBg]            = ImVec4(0.14f, 0.14f, 0.15f, 1.00f);

    // Scrollbar
    c[ImGuiCol_ScrollbarBg]          = scrollbar;
    c[ImGuiCol_ScrollbarGrab]        = scrollG;
    c[ImGuiCol_ScrollbarGrabHovered] = scrollGH;
    c[ImGuiCol_ScrollbarGrabActive]  = scrollGA;

    // Check mark / slider
    c[ImGuiCol_CheckMark]            = accent;
    c[ImGuiCol_SliderGrab]           = accent;
    c[ImGuiCol_SliderGrabActive]     = accentH;

    // Buttons
    c[ImGuiCol_Button]               = btnCol;
    c[ImGuiCol_ButtonHovered]        = btnHov;
    c[ImGuiCol_ButtonActive]         = btnAct;

    // Headers (collapsing, selectable, tree node)
    c[ImGuiCol_Header]               = header;
    c[ImGuiCol_HeaderHovered]        = headerH;
    c[ImGuiCol_HeaderActive]         = headerA;

    // Separator
    c[ImGuiCol_Separator]            = border;
    c[ImGuiCol_SeparatorHovered]     = accentH;
    c[ImGuiCol_SeparatorActive]      = accent;

    // Resize grip
    c[ImGuiCol_ResizeGrip]           = ImVec4(0.25f, 0.25f, 0.27f, 0.40f);
    c[ImGuiCol_ResizeGripHovered]    = accentH;
    c[ImGuiCol_ResizeGripActive]     = accent;

    // Tabs
    c[ImGuiCol_Tab]                  = tab;
    c[ImGuiCol_TabHovered]           = tabH;
    c[ImGuiCol_TabActive]            = tabA;
    c[ImGuiCol_TabUnfocused]         = tab;
    c[ImGuiCol_TabUnfocusedActive]   = tabA;

    // Table
    c[ImGuiCol_TableHeaderBg]        = ImVec4(0.18f, 0.18f, 0.19f, 1.00f);
    c[ImGuiCol_TableBorderStrong]    = border;
    c[ImGuiCol_TableBorderLight]     = ImVec4(0.22f, 0.22f, 0.24f, 0.40f);
    c[ImGuiCol_TableRowBg]           = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_TableRowBgAlt]        = ImVec4(1.0f, 1.0f, 1.0f, 0.02f);

    // Misc
    c[ImGuiCol_TextSelectedBg]       = ImVec4(accent.x, accent.y, accent.z, 0.35f);
    c[ImGuiCol_DragDropTarget]       = accentH;
    c[ImGuiCol_NavHighlight]         = accent;
    c[ImGuiCol_NavWindowingHighlight]= ImVec4(1, 1, 1, 0.10f);
    c[ImGuiCol_NavWindowingDimBg]    = ImVec4(0, 0, 0, 0.60f);
    c[ImGuiCol_ModalWindowDimBg]     = ImVec4(0, 0, 0, 0.55f);
}

// ---------------------------------------------------------------------------
// GLFW error callback
// ---------------------------------------------------------------------------
static void glfwErrorCb(int /*err*/, const char* desc) {
    fprintf(stderr, "GLFW error: %s\n", desc);
}

// ---------------------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------------------
AppGui::AppGui()  = default;
AppGui::~AppGui() { shutdown(); }

// ---------------------------------------------------------------------------
// init
// ---------------------------------------------------------------------------
bool AppGui::init(const std::string& configPath) {
    configPath_ = configPath;

    // Load settings from JSON
    try {
        loadConfig(configPath);
    } catch (...) {
        // Use defaults if config not found
    }

    glfwSetErrorCallback(glfwErrorCb);
    if (!glfwInit()) {
        fprintf(stderr, "AppGui::init — glfwInit() failed\n");
        return false;
    }

    // GL 3.0 + GLSL 130  (works on most systems)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

    window_ = glfwCreateWindow(1440, 900, "VT \xe2\x80\x94 Virtual Trade System", nullptr, nullptr);
    if (!window_) {
        fprintf(stderr, "AppGui::init — glfwCreateWindow() failed\n");
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // vsync

    // Load and set window icon (Win32: LoadImage → glfwSetWindowIcon)
#ifdef _WIN32
    {
        HICON hIcon = (HICON)LoadImageA(
            GetModuleHandleA(nullptr),
            "IDI_ICON1",
            IMAGE_ICON, 32, 32,
            LR_DEFAULTCOLOR);
        if (!hIcon) {
            // Try loading from file as fallback
            hIcon = (HICON)LoadImageA(
                nullptr,
                "resources/app_icon.ico",
                IMAGE_ICON, 32, 32,
                LR_LOADFROMFILE);
        }
        if (hIcon) {
            // Send WM_SETICON to the native window
            HWND hwnd = glfwGetWin32Window(window_);
            if (hwnd) {
                SendMessageA(hwnd, WM_SETICON, ICON_BIG,   (LPARAM)hIcon);
                SendMessageA(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            }
        }
    }
#endif

    // ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Set imgui.ini path in config directory for persistent window layout
    {
        namespace fs = std::filesystem;
        static std::string iniPath;
        fs::path cfgDir = fs::path(configPath_).parent_path();
        if (cfgDir.empty()) cfgDir = ".";
        iniPath = (cfgDir / "imgui.ini").string();
        io.IniFilename = iniPath.c_str();
    }

    // Apply dark-metal theme
    setupTheme();

    // Platform + renderer init
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Layout .ini persistence: load if exists, create with defaults if not
    {
        namespace fs = std::filesystem;
        fs::path cfgDir = fs::path(configPath_).parent_path();
        if (cfgDir.empty()) cfgDir = ".";
        layoutIniPath_ = (cfgDir / "layout.ini").string();

        if (fs::exists(layoutIniPath_)) {
            loadLayoutIni(layoutIniPath_);
        } else {
            // First launch — create layout.ini with default settings
            saveLayoutIni(layoutIniPath_);
        }
        layoutNeedsReset_ = true;
    }

    return true;
}

// ---------------------------------------------------------------------------
// run  (blocks until window close)
// ---------------------------------------------------------------------------
void AppGui::run() {
    while (!glfwWindowShouldClose(window_) && !shouldClose_) {
        glfwPollEvents();
        renderFrame();
    }
    shouldClose_ = true;
}

// ---------------------------------------------------------------------------
// shutdown
// ---------------------------------------------------------------------------
void AppGui::shutdown() {
    // Save layout.ini before destroying the window
    if (!layoutIniPath_.empty() && window_) {
        try { saveLayoutIni(layoutIniPath_); } catch (...) {}
    }

    if (!window_) return;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window_);
    glfwTerminate();
    window_ = nullptr;
}

// ---------------------------------------------------------------------------
// Theme
// ---------------------------------------------------------------------------
void AppGui::setupTheme() {
    ApplyDarkMetalTheme();

    // Slightly larger default font size
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = 1.0f;
}

// ---------------------------------------------------------------------------
// updateState (thread-safe, called from engine thread)
// ---------------------------------------------------------------------------
void AppGui::updateState(const GuiState& s) {
    std::lock_guard<std::mutex> lk(stateMutex_);
    state_ = s;

    // Track history
    if (priceHistory_.empty() || priceHistory_.back() != s.lastCandle.close) {
        priceHistory_.push_back(s.lastCandle.close);
        if ((int)priceHistory_.size() > kMaxHistory) priceHistory_.pop_front();
    }
    if (equityHistory_.empty() || equityHistory_.back() != s.equity) {
        equityHistory_.push_back(s.equity);
        if ((int)equityHistory_.size() > kMaxHistory) equityHistory_.pop_front();
    }
    if (rsiHistory_.empty() || rsiHistory_.back() != s.rsi) {
        rsiHistory_.push_back(s.rsi);
        if ((int)rsiHistory_.size() > kMaxHistory) rsiHistory_.pop_front();
    }
    // Volume delta: approximate buy vs sell using candle direction
    // Note: this is an approximation; real delta requires tick-level order flow data
    {
        double delta = (s.lastCandle.close >= s.lastCandle.open)
            ?  s.lastCandle.volume
            : -s.lastCandle.volume;
        if (volumeDeltaHistory_.empty() || volumeDeltaHistory_.back() != delta) {
            volumeDeltaHistory_.push_back(delta);
            if ((int)volumeDeltaHistory_.size() > kMaxHistory) volumeDeltaHistory_.pop_front();
        }
    }
    if (s.lastSignal.type != Signal::Type::HOLD) {
        signalHistory_.push_back(s.lastSignal);
        if ((int)signalHistory_.size() > 50) signalHistory_.pop_front();
    }
}

GuiConfig AppGui::getConfig() const {
    std::lock_guard<std::mutex> lk(stateMutex_);
    return config_;
}

void AppGui::addLog(const std::string& line) {
    std::lock_guard<std::mutex> lk(stateMutex_);
    state_.logLines.push_back(line);
    if ((int)state_.logLines.size() > kMaxLogLines)
        state_.logLines.pop_front();
}

void AppGui::setDatabaseRepositories(TradeRepository* tradeRepo,
                                      AlertRepository* alertRepo,
                                      BacktestRepository* backtestRepo,
                                      PositionRepository* positionRepo,
                                      DrawingRepository* drawingRepo,
                                      AuxRepository* auxRepo,
                                      EquityRepository* /*equityRepo — used by Engine, not GUI*/) {
    tradeHistory_.setRepository(tradeRepo);
    alertManager_.setRepository(alertRepo);
    paperTrader_.setPositionRepository(positionRepo);
    // BacktestEngine is created per-run; store repo pointer for later use
    btRepo_ = backtestRepo;
    drawRepo_ = drawingRepo;
    // Wire scanner to AuxRepository for cache persistence
    scanner_.setAuxRepository(auxRepo);
}

// ---------------------------------------------------------------------------
// Config load / save
// ---------------------------------------------------------------------------
void AppGui::loadConfig(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return;

    nlohmann::json j;
    f >> j;

    // Try to load active exchange from profiles.json (if it exists)
    {
        namespace fs = std::filesystem;
        fs::path profilesPath = fs::path(path).parent_path() / "profiles.json";
        std::ifstream pf(profilesPath);
        if (pf.is_open()) {
            try {
                nlohmann::json pj;
                pf >> pj;
                std::string activeExchange = pj.value("active", "");
                if (!activeExchange.empty() && pj.contains("exchanges") &&
                    pj["exchanges"].contains(activeExchange)) {
                    auto& exchangeProfile = pj["exchanges"][activeExchange];
                    // Override exchange config from profiles.json
                    if (j.contains("exchange")) {
                        auto& ex = j["exchange"];
                        ex["name"]       = exchangeProfile.value("name", activeExchange);
                        ex["api_key"]    = exchangeProfile.value("api_key", ex.value("api_key", ""));
                        ex["api_secret"] = exchangeProfile.value("api_secret", ex.value("api_secret", ""));
                        ex["passphrase"] = exchangeProfile.value("passphrase", ex.value("passphrase", ""));
                        ex["testnet"]    = exchangeProfile.value("testnet", ex.value("testnet", true));
                        ex["base_url"]   = exchangeProfile.value("base_url", ex.value("base_url", ""));
                        ex["ws_host"]    = exchangeProfile.value("ws_host", ex.value("ws_host", ""));
                        ex["ws_port"]    = exchangeProfile.value("ws_port", ex.value("ws_port", ""));
                        ex["futures_base_url"] = exchangeProfile.value("futures_base_url", ex.value("futures_base_url", ""));
                        ex["futures_ws_host"]  = exchangeProfile.value("futures_ws_host", ex.value("futures_ws_host", ""));
                        ex["futures_ws_port"]  = exchangeProfile.value("futures_ws_port", ex.value("futures_ws_port", ""));
                    }
                }
            } catch (...) {
                // Ignore profiles.json parse errors
            }
        }
    }

    auto& ex = j["exchange"];
    config_.exchangeName = ex.value("name", "binance");
    config_.apiKey       = ex.value("api_key", "");
    config_.apiSecret    = ex.value("api_secret", "");
    config_.passphrase   = ex.value("passphrase", "");
    config_.testnet      = ex.value("testnet", true);
    config_.baseUrl      = ex.value("base_url", "https://testnet.binance.vision");
    config_.wsHost       = ex.value("ws_host", "testnet.binance.vision");
    config_.wsPort       = ex.value("ws_port", "443");
    config_.futuresBaseUrl = ex.value("futures_base_url", "https://testnet.binancefuture.com");
    config_.futuresWsHost  = ex.value("futures_ws_host", "fstream.binancefuture.com");
    config_.futuresWsPort  = ex.value("futures_ws_port", "443");

    auto& tr = j["trading"];
    config_.symbol         = tr.value("symbol", "BTCUSDT");
    config_.interval       = tr.value("interval", "15m");
    config_.mode           = tr.value("mode", "paper");
    config_.marketType     = tr.value("market_type", "futures");
    config_.initialCapital = tr.value("initial_capital", 1000.0);

    auto& ind = j["indicator"];
    config_.emaFast       = ind.value("ema_fast", 9);
    config_.emaSlow       = ind.value("ema_slow", 21);
    config_.emaTrend      = ind.value("ema_trend", 200);
    config_.rsiPeriod     = ind.value("rsi_period", 14);
    config_.rsiOverbought = ind.value("rsi_overbought", 70.0);
    config_.rsiOversold   = ind.value("rsi_oversold", 30.0);
    config_.atrPeriod     = ind.value("atr_period", 14);
    config_.macdFast      = ind.value("macd_fast", 12);
    config_.macdSlow      = ind.value("macd_slow", 26);
    config_.macdSignal    = ind.value("macd_signal", 9);
    config_.bbPeriod      = ind.value("bb_period", 20);
    config_.bbStddev      = ind.value("bb_stddev", 2.0);

    auto& ml = j["ml"];
    config_.lstmHidden   = ml["lstm"].value("hidden_size", 128);
    config_.lstmSeqLen   = ml["lstm"].value("seq_len", 60);
    config_.lstmDropout  = ml["lstm"].value("dropout", 0.2);
    config_.xgbMaxDepth  = ml["xgboost"].value("max_depth", 6);
    config_.xgbRounds    = ml["xgboost"].value("rounds", 500);
    config_.xgbEta       = ml["xgboost"].value("eta", 0.1);
    config_.ensembleMinConfidence = ml["ensemble"].value("min_confidence", 0.65);
    config_.retrainIntervalHours  = ml.value("retrain_interval_hours", 24);
    config_.lookbackDays          = ml.value("lookback_days", 90);

    auto& rk = j["risk"];
    config_.maxRiskPerTrade  = rk.value("max_risk_per_trade", 0.02);
    config_.maxDrawdown      = rk.value("max_drawdown", 0.15);
    config_.atrStopMultiplier= rk.value("atr_stop_multiplier", 1.5);
    config_.rrRatio          = rk.value("rr_ratio", 2.0);

    config_.userIndicatorDir = j.value("user_indicator_dir", "user_indicator");

    if (j.contains("layout")) {
        auto& ly = j["layout"];
        config_.layoutLogPct = ly.value("log_pct",  0.10f);
        config_.layoutVdPct  = ly.value("vd_pct",   0.15f);
        config_.layoutIndPct = ly.value("ind_pct",  0.20f);
        config_.layoutLocked = ly.value("locked",   true);
        showPairList_    = ly.value("show_pair_list",    true);
        showUserPanel_   = ly.value("show_user_panel",   true);
        showVolumeDelta_ = ly.value("show_volume_delta", true);
        showIndicators_  = ly.value("show_indicators",   true);
        showLogs_        = ly.value("show_logs",         true);
    }

    if (j.contains("filters")) {
        auto& f = j["filters"];
        config_.filterMinVolume = f.value("min_volume", 0.0);
        config_.filterMinPrice  = f.value("min_price",  0.0);
        config_.filterMaxPrice  = f.value("max_price",  0.0);
        config_.filterMinChange = f.value("min_change", -100.0);
        config_.filterMaxChange = f.value("max_change",  100.0);
    }

    if (j.contains("indicators_enabled")) {
        auto& ie = j["indicators_enabled"];
        config_.indEmaEnabled  = ie.value("ema",  true);
        config_.indRsiEnabled  = ie.value("rsi",  true);
        config_.indAtrEnabled  = ie.value("atr",  true);
        config_.indMacdEnabled = ie.value("macd", true);
        config_.indBbEnabled   = ie.value("bb",   true);
    }

    if (j.contains("chart")) {
        auto& ch = j["chart"];
        chartBarWidth_     = ch.value("bar_width",     8.0f);
        chartScrollOffset_ = ch.value("scroll_offset", 0);
    }

    state_.equity = config_.initialCapital;
    state_.initialCapital = config_.initialCapital;
}

nlohmann::json AppGui::configToJson() const {
    nlohmann::json j;
    j["exchange"] = {
        {"name",       config_.exchangeName},
        {"api_key",    config_.apiKey},
        {"api_secret", config_.apiSecret},
        {"passphrase", config_.passphrase},
        {"testnet",    config_.testnet},
        {"base_url",   config_.baseUrl},
        {"ws_host",    config_.wsHost},
        {"ws_port",    config_.wsPort},
        {"futures_base_url", config_.futuresBaseUrl},
        {"futures_ws_host",  config_.futuresWsHost},
        {"futures_ws_port",  config_.futuresWsPort}
    };
    j["trading"] = {
        {"symbol",          config_.symbol},
        {"interval",        config_.interval},
        {"mode",            config_.mode},
        {"market_type",     config_.marketType},
        {"initial_capital", config_.initialCapital}
    };
    j["indicator"] = {
        {"ema_fast",       config_.emaFast},
        {"ema_slow",       config_.emaSlow},
        {"ema_trend",      config_.emaTrend},
        {"rsi_period",     config_.rsiPeriod},
        {"rsi_overbought", config_.rsiOverbought},
        {"rsi_oversold",   config_.rsiOversold},
        {"atr_period",     config_.atrPeriod},
        {"macd_fast",      config_.macdFast},
        {"macd_slow",      config_.macdSlow},
        {"macd_signal",    config_.macdSignal},
        {"bb_period",      config_.bbPeriod},
        {"bb_stddev",      config_.bbStddev}
    };
    j["ml"] = {
        {"lstm",     {{"hidden_size", config_.lstmHidden},
                      {"seq_len",     config_.lstmSeqLen},
                      {"dropout",     config_.lstmDropout}}},
        {"xgboost",  {{"max_depth", config_.xgbMaxDepth},
                      {"eta",       config_.xgbEta},
                      {"rounds",    config_.xgbRounds}}},
        {"ensemble", {{"min_confidence", config_.ensembleMinConfidence}}},
        {"retrain_interval_hours", config_.retrainIntervalHours},
        {"lookback_days",          config_.lookbackDays}
    };
    j["risk"] = {
        {"max_risk_per_trade",  config_.maxRiskPerTrade},
        {"max_drawdown",        config_.maxDrawdown},
        {"atr_stop_multiplier", config_.atrStopMultiplier},
        {"rr_ratio",            config_.rrRatio}
    };
    j["user_indicator_dir"] = config_.userIndicatorDir;
    j["layout"] = {
        {"log_pct",  config_.layoutLogPct},
        {"vd_pct",   config_.layoutVdPct},
        {"ind_pct",  config_.layoutIndPct},
        {"locked",   config_.layoutLocked},
        {"show_pair_list",    showPairList_},
        {"show_user_panel",   showUserPanel_},
        {"show_volume_delta", showVolumeDelta_},
        {"show_indicators",   showIndicators_},
        {"show_logs",         showLogs_}
    };
    j["filters"] = {
        {"min_volume", config_.filterMinVolume},
        {"min_price",  config_.filterMinPrice},
        {"max_price",  config_.filterMaxPrice},
        {"min_change", config_.filterMinChange},
        {"max_change", config_.filterMaxChange}
    };
    j["indicators_enabled"] = {
        {"ema",  config_.indEmaEnabled},
        {"rsi",  config_.indRsiEnabled},
        {"atr",  config_.indAtrEnabled},
        {"macd", config_.indMacdEnabled},
        {"bb",   config_.indBbEnabled}
    };
    j["chart"] = {
        {"bar_width",     chartBarWidth_},
        {"scroll_offset", chartScrollOffset_}
    };
    return j;
}

void AppGui::saveConfigToFile(const std::string& path) {
    std::ofstream f(path);
    if (!f.is_open()) return;
    f << configToJson().dump(2);

    // Also update profiles.json so the active exchange config persists across restarts
    namespace fs = std::filesystem;
    fs::path profilesPath = fs::path(path).parent_path() / "profiles.json";
    try {
        nlohmann::json pj;
        {
            std::ifstream pf(profilesPath);
            if (pf.is_open()) {
                pf >> pj;
            }
        }
        pj["active"] = config_.exchangeName;
        if (!pj.contains("exchanges")) pj["exchanges"] = nlohmann::json::object();
        pj["exchanges"][config_.exchangeName] = {
            {"name",       config_.exchangeName},
            {"api_key",    config_.apiKey},
            {"api_secret", config_.apiSecret},
            {"passphrase", config_.passphrase},
            {"testnet",    config_.testnet},
            {"base_url",   config_.baseUrl},
            {"ws_host",    config_.wsHost},
            {"ws_port",    config_.wsPort},
            {"futures_base_url", config_.futuresBaseUrl},
            {"futures_ws_host",  config_.futuresWsHost},
            {"futures_ws_port",  config_.futuresWsPort}
        };
        std::ofstream pout(profilesPath);
        if (pout.is_open()) {
            pout << pj.dump(2);
        }
    } catch (const std::exception& e) {
        Logger::get()->warn("[Config] Failed to save profiles.json: {}", e.what());
    } catch (...) {
        Logger::get()->warn("[Config] Failed to save profiles.json");
    }
}

// ---------------------------------------------------------------------------
// Layout INI persistence — simple key=value format for window sizes
// ---------------------------------------------------------------------------
void AppGui::loadLayoutIni(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return;

    std::string line;
    while (std::getline(f, line)) {
        // Skip comments and section headers
        if (line.empty() || line[0] == '#' || line[0] == ';' || line[0] == '[')
            continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        // trim whitespace
        {
            auto kEnd = key.find_last_not_of(" \t");
            if (kEnd != std::string::npos) key = key.substr(0, kEnd + 1);
            auto vStart = val.find_first_not_of(" \t");
            if (vStart != std::string::npos) val = val.substr(vStart);
            else val.clear();
        }

        try {
            if      (key == "log_pct")           config_.layoutLogPct  = std::stof(val);
            else if (key == "vd_pct")            config_.layoutVdPct   = std::stof(val);
            else if (key == "ind_pct")           config_.layoutIndPct  = std::stof(val);
            else if (key == "locked")            config_.layoutLocked  = (std::stoi(val) != 0);
            else if (key == "show_pair_list")    showPairList_         = (std::stoi(val) != 0);
            else if (key == "show_user_panel")   showUserPanel_        = (std::stoi(val) != 0);
            else if (key == "show_volume_delta") showVolumeDelta_      = (std::stoi(val) != 0);
            else if (key == "show_indicators")   showIndicators_       = (std::stoi(val) != 0);
            else if (key == "show_logs")         showLogs_             = (std::stoi(val) != 0);
            else if (key == "window_width") {
                int w = std::stoi(val);
                if (window_ && w > 100) {
                    int cw, ch;
                    glfwGetWindowSize(window_, &cw, &ch);
                    glfwSetWindowSize(window_, w, ch);
                }
            }
            else if (key == "window_height") {
                int h = std::stoi(val);
                if (window_ && h > 100) {
                    int cw, ch;
                    glfwGetWindowSize(window_, &cw, &ch);
                    glfwSetWindowSize(window_, cw, h);
                }
            }
        } catch (...) {
            // Skip malformed values
        }
    }
}

void AppGui::saveLayoutIni(const std::string& path) const {
    std::ofstream f(path);
    if (!f.is_open()) return;

    f << "# VT Layout Configuration\n";
    f << "# Auto-generated — saved window layout settings\n\n";

    f << "[layout]\n";
    f << "log_pct=" << config_.layoutLogPct << "\n";
    f << "vd_pct=" << config_.layoutVdPct << "\n";
    f << "ind_pct=" << config_.layoutIndPct << "\n";
    f << "locked=" << (config_.layoutLocked ? 1 : 0) << "\n";
    f << "show_pair_list=" << (showPairList_ ? 1 : 0) << "\n";
    f << "show_user_panel=" << (showUserPanel_ ? 1 : 0) << "\n";
    f << "show_volume_delta=" << (showVolumeDelta_ ? 1 : 0) << "\n";
    f << "show_indicators=" << (showIndicators_ ? 1 : 0) << "\n";
    f << "show_logs=" << (showLogs_ ? 1 : 0) << "\n";

    if (window_) {
        int w, h;
        glfwGetWindowSize(window_, &w, &h);
        f << "\n[window]\n";
        f << "window_width=" << w << "\n";
        f << "window_height=" << h << "\n";
    }
}

// ---------------------------------------------------------------------------
// renderFrame
// ---------------------------------------------------------------------------
void AppGui::renderFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Viewport dimensions for layout calculation
    ImGuiViewport* vp = ImGui::GetMainViewport();

    // Auto-resize: detect viewport size change (fullscreen ↔ windowed toggle)
    {
        const float curW = vp->WorkSize.x;
        const float curH = vp->WorkSize.y;
        if (prevViewportW_ > 0.0f && prevViewportH_ > 0.0f) {
            const float dw = std::fabs(curW - prevViewportW_);
            const float dh = std::fabs(curH - prevViewportH_);
            if (dw > 1.0f || dh > 1.0f) {
                // Viewport size changed — force layout recalculation
                layoutNeedsReset_ = true;
            }
        }
        prevViewportW_ = curW;
        prevViewportH_ = curH;
    }

    // Sync layout proportions from config
    layoutMgr_.setLogPct(config_.layoutLogPct);
    layoutMgr_.setVdPct(config_.layoutVdPct);
    layoutMgr_.setIndPct(config_.layoutIndPct);

    // Recalculate layout based on current screen size and show flags
    layoutMgr_.recalculate(vp->WorkSize.x, vp->WorkSize.y,
                           vp->WorkPos.x,  vp->WorkPos.y,
                           showPairList_, showUserPanel_,
                           showVolumeDelta_, showIndicators_, showLogs_);

    // ── All 8 mandatory windows ──

    // Main Toolbar — top bar with connect/start/settings buttons
    drawMainToolbarWindow();

    // Filters Bar — below toolbar
    drawFiltersBarWindow();

    // Pair List — left column, top
    if (showPairList_) drawPairListPanel();

    // Volume Delta — left column, below Pair List
    if (showVolumeDelta_) drawVolumeDeltaPanel();

    // Center column: Indicators (top) → Market Data (middle) → Logs (bottom)
    if (showIndicators_) drawIndicatorsPanel();
    drawMarketDataWindow();
    if (showLogs_) drawLogWindow();

    // User Panel — right sidebar
    if (showUserPanel_) drawUserPanel();

    // Reset flag consumed after one frame
    if (layoutNeedsReset_) layoutNeedsReset_ = false;

    // Settings window (modal-like)
    if (showSettings_) drawSettingsPanel();

    // Order book as optional floating window
    if (showOrderBook_) drawOrderBookPanel();

    // New windows
    if (showOrderManagement_) drawOrderManagementWindow();
    if (showPaperTrading_) drawPaperTradingWindow();
    if (showBacktest_) drawBacktestWindow();
    if (showAlerts_) drawAlertsWindow();
    if (showScanner_) drawMarketScannerWindow();
    if (showPineEditor_) drawPineEditorWindow();
    if (showTradeHistory_) drawTradeHistoryWindow();

    // Render
    ImGui::Render();
    int dw, dh;
    glfwGetFramebufferSize(window_, &dw, &dh);
    glViewport(0, 0, dw, dh);
    glClearColor(0.08f, 0.08f, 0.09f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window_);
}

// ---------------------------------------------------------------------------
//  Menu bar
// ---------------------------------------------------------------------------
void AppGui::drawMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Settings", "Ctrl+S")) showSettings_ = true;
            ImGui::Separator();
            if (ImGui::MenuItem("Save Config")) {
                saveConfigToFile(configPath_);
                addLog("[Config] Saved to " + configPath_);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) shouldClose_ = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Settings Panel",    nullptr, &showSettings_);
            ImGui::MenuItem("Pair List",         nullptr, &showPairList_);
            ImGui::MenuItem("User Panel",        nullptr, &showUserPanel_);
            ImGui::MenuItem("Volume Delta",      nullptr, &showVolumeDelta_);
            ImGui::MenuItem("Indicators",        nullptr, &showIndicators_);
            ImGui::MenuItem("Logs",              nullptr, &showLogs_);
            ImGui::Separator();
            ImGui::MenuItem("Order Management",  nullptr, &showOrderManagement_);
            ImGui::MenuItem("Paper Trading",     nullptr, &showPaperTrading_);
            ImGui::MenuItem("Backtesting",       nullptr, &showBacktest_);
            ImGui::MenuItem("Alerts",            nullptr, &showAlerts_);
            ImGui::MenuItem("Market Scanner",    nullptr, &showScanner_);
            ImGui::MenuItem("Pine Editor",       nullptr, &showPineEditor_);
            ImGui::MenuItem("Trade History",     nullptr, &showTradeHistory_);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                addLog("[Info] Crypto ML Trader v2.6.0 — Algorithmic Trading System");
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

// ---------------------------------------------------------------------------
//  Main Toolbar Window — standalone window at top with menu bar + toolbar
// ---------------------------------------------------------------------------
void AppGui::drawMainToolbarWindow() {
    auto layout = layoutMgr_.get("Main Toolbar");
    LayoutManager::lockWindow("Main Toolbar", layout.pos, layout.size);

    int wflags = ImGuiWindowFlags_NoTitleBar
               | ImGuiWindowFlags_NoCollapse
               | ImGuiWindowFlags_NoBringToFrontOnFocus
               | ImGuiWindowFlags_NoMove
               | ImGuiWindowFlags_NoResize
               | ImGuiWindowFlags_MenuBar;

    ImGui::Begin("Main Toolbar", nullptr, wflags);
    drawMenuBar();
    drawToolbar();
    ImGui::End();
}

// ---------------------------------------------------------------------------
//  Filters Bar Window — standalone window below toolbar
// ---------------------------------------------------------------------------
void AppGui::drawFiltersBarWindow() {
    auto layout = layoutMgr_.get("Filters Bar");
    LayoutManager::lockWindow("Filters Bar", layout.pos, layout.size);

    int wflags = ImGuiWindowFlags_NoTitleBar
               | ImGuiWindowFlags_NoCollapse
               | ImGuiWindowFlags_NoBringToFrontOnFocus
               | ImGuiWindowFlags_NoMove
               | ImGuiWindowFlags_NoResize;

    ImGui::Begin("Filters Bar", nullptr, wflags);
    drawFilterPanel();
    ImGui::End();
}

// ---------------------------------------------------------------------------
//  Toolbar
// ---------------------------------------------------------------------------
void AppGui::drawToolbar() {
    GuiState snap;
    {
        std::lock_guard<std::mutex> lk(stateMutex_);
        snap = state_;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 6));

    // Connect / Disconnect
    if (!snap.connected) {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.18f, 0.40f, 0.22f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.22f, 0.50f, 0.28f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.15f, 0.35f, 0.18f, 1.0f));
        if (ImGui::Button("  Connect  ")) {
            if (onConnect_) onConnect_(config_);
#ifdef USE_LIBTORCH
            addLog("[AI] Status: Enabled — awaiting connection");
#else
            addLog("[AI] Status: Disabled — LibTorch not available");
#endif
        }
        ImGui::PopStyleColor(3);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.50f, 0.18f, 0.18f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.60f, 0.22f, 0.22f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.40f, 0.15f, 0.15f, 1.0f));
        if (ImGui::Button("  Disconnect  ")) {
            if (onDisconnect_) onDisconnect_();
            addLog("[AI] Status: Idle — disconnected");
        }
        ImGui::PopStyleColor(3);
    }
    ImGui::SameLine();

    // Start / Stop trading
    if (!snap.trading) {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.20f, 0.35f, 0.55f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.25f, 0.42f, 0.65f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.15f, 0.28f, 0.45f, 1.0f));
        if (ImGui::Button("  Start Trading  ")) {
            if (onStart_) onStart_();
            addLog("[AI] Status: Working — trading active");
        }
        ImGui::PopStyleColor(3);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.55f, 0.35f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.65f, 0.42f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.45f, 0.28f, 0.12f, 1.0f));
        if (ImGui::Button("  Stop Trading  ")) {
            if (onStop_) onStop_();
            addLog("[AI] Status: Idle — trading stopped");
        }
        ImGui::PopStyleColor(3);
    }
    ImGui::SameLine();

    // Settings button
    if (ImGui::Button("  Settings  ")) {
        showSettings_ = !showSettings_;
    }
    ImGui::SameLine();

    // Order Book (Стакан) toggle
    if (showOrderBook_) {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.45f, 0.25f, 0.55f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.55f, 0.32f, 0.65f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.38f, 0.20f, 0.48f, 1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.22f, 0.22f, 0.24f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.30f, 0.30f, 0.32f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.18f, 0.18f, 0.20f, 1.0f));
    }
    if (ImGui::Button("  Order Book  ")) {
        showOrderBook_ = !showOrderBook_;
    }
    ImGui::PopStyleColor(3);
    ImGui::SameLine();

    // User Panel toggle (shown after connection)
    if (snap.connected) {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.25f, 0.45f, 0.65f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.32f, 0.55f, 0.75f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.20f, 0.38f, 0.55f, 1.0f));
        if (ImGui::Button("  User Panel  ")) {
            showUserPanel_ = !showUserPanel_;
        }
        ImGui::PopStyleColor(3);
        ImGui::SameLine();
    }

    // Paper/Live mode indicator (H2)
    {
        bool isPaper = (config_.mode == "paper");
        if (isPaper) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.6f, 0.0f, 1.0f));
            ImGui::Button(" PAPER TRADING ");
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
            ImGui::Button(" LIVE TRADING ");
            ImGui::PopStyleColor();
        }
    }
    ImGui::SameLine();

    // Pair selector dropdown
    drawPairSelector();
    ImGui::SameLine();

    // Market type toggle (futures / spot)
    {
        const char* marketTypes[] = { "futures", "spot" };
        int mtIdx = (config_.marketType == "spot") ? 1 : 0;
        ImGui::SetNextItemWidth(100);
        if (ImGui::Combo("##MarketType", &mtIdx, marketTypes, 2)) {
            config_.marketType = marketTypes[mtIdx];
        }
    }
    ImGui::SameLine();

    // Display current pair & mode
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
    ImGui::TextColored(ImVec4(0.60f, 0.70f, 0.85f, 1.0f),
                       "%s  |  %s  |  %s  |  %s",
                       config_.symbol.c_str(),
                       config_.interval.c_str(),
                       config_.marketType.c_str(),
                       config_.mode.c_str());

    // AI status indicator
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);
#ifdef USE_LIBTORCH
    if (snap.connected && snap.trading) {
        ImGui::TextColored(ImVec4(0.70f, 0.50f, 0.90f, 1.0f), "[AI: Working]");
    } else if (snap.connected) {
        ImGui::TextColored(ImVec4(0.55f, 0.45f, 0.75f, 1.0f), "[AI: Enabled]");
    } else {
        ImGui::TextColored(ImVec4(0.45f, 0.45f, 0.50f, 1.0f), "[AI: Idle]");
    }
#else
    ImGui::TextColored(ImVec4(0.45f, 0.45f, 0.50f, 1.0f), "[AI: Disabled]");
#endif

    ImGui::PopStyleVar();
    ImGui::Separator();
}

// ---------------------------------------------------------------------------
//  Market Data Panel
// ---------------------------------------------------------------------------
static void PlotLineFromDeque(const char* label, const std::deque<double>& data,
                               float height, ImVec4 color) {
    if (data.empty()) {
        ImGui::Text("%s: no data", label);
        return;
    }
    // Convert deque to vector for PlotLines
    std::vector<float> v;
    v.reserve(data.size());
    for (double d : data) v.push_back(static_cast<float>(d));
    ImGui::PushStyleColor(ImGuiCol_PlotLines, color);
    ImGui::PlotLines(label, v.data(), (int)v.size(), 0, nullptr,
                     FLT_MAX, FLT_MAX, ImVec2(-1, height));
    ImGui::PopStyleColor();
}

// ── Local helpers for EMA / RSI computation from candle history ──
static std::vector<double> calcEMAFromCandles(const std::deque<Candle>& bars, int period) {
    std::vector<double> ema(bars.size(), 0.0);
    if (bars.empty()) return ema;
    double k = 2.0 / (period + 1);
    ema[0] = bars[0].close;
    for (size_t i = 1; i < bars.size(); ++i)
        ema[i] = bars[i].close * k + ema[i - 1] * (1.0 - k);
    return ema;
}

static std::vector<double> calcRSIFromCandles(const std::deque<Candle>& bars, int period = 14) {
    std::vector<double> rsi(bars.size(), 50.0);
    if ((int)bars.size() <= period) return rsi;
    double avgGain = 0.0, avgLoss = 0.0;
    for (int i = 1; i <= period; ++i) {
        double d = bars[i].close - bars[i - 1].close;
        if (d > 0) avgGain += d; else avgLoss -= d;
    }
    avgGain /= period;
    avgLoss /= period;
    for (int i = period; i < (int)bars.size(); ++i) {
        double d = bars[i].close - bars[i - 1].close;
        double gain = d > 0 ? d : 0;
        double loss = d < 0 ? -d : 0;
        avgGain = (avgGain * (period - 1) + gain) / period;
        avgLoss = (avgLoss * (period - 1) + loss) / period;
        rsi[i] = (avgLoss == 0.0) ? 100.0 : 100.0 - 100.0 / (1.0 + avgGain / avgLoss);
    }
    return rsi;
}

void AppGui::drawMarketDataWindow() {
    GuiState snap;
    {
        std::lock_guard<std::mutex> lk(stateMutex_);
        snap = state_;
    }

    // ── Window: fixed position/size via LayoutManager ──
    auto layout = layoutMgr_.get("Market Data");
    if (config_.layoutLocked || layoutNeedsReset_)
        LayoutManager::lockWindow("Market Data", layout.pos, layout.size);
    else
        LayoutManager::lockWindowOnce("Market Data", layout.pos, layout.size);

    int mdFlags = config_.layoutLocked ? LayoutManager::lockedFlags() : (ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);
    mdFlags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::Begin("Market Data", nullptr, mdFlags);

    // ── 5.2: Signal panel — price, indicators, signal, confidence ──
    {
        // Compute latest EMA9 & RSI from history for display
        double ema9Val = snap.emaFast;  // from engine
        double rsiVal  = snap.rsi;
        if (!snap.candleHistory.empty()) {
            auto ema9vec = calcEMAFromCandles(snap.candleHistory, 9);
            ema9Val = ema9vec.back();
            auto rsivec = calcRSIFromCandles(snap.candleHistory, 14);
            rsiVal = rsivec.back();
        }

        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Price: %.8f",
                           snap.lastCandle.close);
        ImGui::SameLine(0, 20);
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "RSI: %.1f", rsiVal);
        ImGui::SameLine(0, 20);
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "EMA9: %.8f", ema9Val);
        ImGui::SameLine(0, 20);

        ImVec4 sigColor;
        const char* sigText;
        switch (snap.lastSignal.type) {
            case Signal::Type::BUY:  sigColor = ImVec4(0.0f, 0.9f, 0.0f, 1.0f); sigText = "BUY";  break;
            case Signal::Type::SELL: sigColor = ImVec4(0.9f, 0.1f, 0.1f, 1.0f); sigText = "SELL"; break;
            default:                 sigColor = ImVec4(0.7f, 0.7f, 0.7f, 1.0f); sigText = "HOLD"; break;
        }
        ImGui::TextColored(sigColor, "Signal: %s", sigText);
        ImGui::SameLine(0, 10);
        ImGui::TextColored(sigColor, "(%.1f%%)", snap.lastSignal.confidence * 100.0);
    }

    // ── Price info bar (OHLCV) ──
    {
        double priceChange = 0.0;
        double pctChange = 0.0;
        if (snap.lastCandle.open > 0) {
            priceChange = snap.lastCandle.close - snap.lastCandle.open;
            pctChange = (priceChange / snap.lastCandle.open) * 100.0;
        }
        ImVec4 chgColor = (priceChange >= 0)
            ? ImVec4(0.30f, 0.85f, 0.35f, 1.0f)
            : ImVec4(0.90f, 0.30f, 0.30f, 1.0f);

        ImGui::TextColored(chgColor, "(%+.2f%%)", pctChange);
        ImGui::SameLine(0, 20);
        ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.52f, 1.0f),
            "O: %.8f  H: %.8f  L: %.8f  V: %.1f",
            snap.lastCandle.open, snap.lastCandle.high,
            snap.lastCandle.low, snap.lastCandle.volume);
    }
    ImGui::Separator();

    // ── Timeframe quick-switch buttons ──
    {
        const char* tfButtons[] = {"1m","3m","5m","15m","1h","4h","1d"};
        for (int i = 0; i < (int)(sizeof(tfButtons)/sizeof(tfButtons[0])); ++i) {
            if (i > 0) ImGui::SameLine();
            bool active = (config_.interval == tfButtons[i]);
            if (active) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.55f, 0.75f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.42f, 0.62f, 0.82f, 1.0f));
            }
            char btnLabel[16];
            snprintf(btnLabel, sizeof(btnLabel), " %s ", tfButtons[i]);
            if (ImGui::SmallButton(btnLabel)) {
                config_.interval = tfButtons[i];
                needsChartReset_ = true;
                chartScrollOffset_ = 0;
                addLog(std::string("[Chart] Timeframe: ") + tfButtons[i]);
                if (onRefreshData_) onRefreshData_();
            }
            if (active) ImGui::PopStyleColor(2);
        }
    }

    // ── Reset View / Fit All buttons ──
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.52f, 1.0f), " |");
    ImGui::SameLine();
    if (ImGui::SmallButton("Reset View")) {
        needsChartReset_ = true;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Fit All")) {
        int totalCount = (int)snap.candleHistory.size();
        if (totalCount > 0) {
            float chartW = ImGui::GetContentRegionAvail().x - 70.0f;
            chartBarWidth_ = std::clamp(chartW / (float)totalCount - 1.0f,
                                        chartMinBarWidth_, chartMaxBarWidth_);
            chartScrollOffset_ = 0;
        }
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Export Bars")) {
        std::vector<Candle> bars(snap.candleHistory.begin(), snap.candleHistory.end());
        std::string fname = config_.symbol + "_" + config_.interval + "_bars.csv";
        if (CSVExporter::exportBars(bars, fname))
            addLog("[Export] Saved " + fname);
    }

    // ── Connection/WS status indicator (right side of toolbar) ──
    {
        ImGui::SameLine();
        float statusPosX = ImGui::GetWindowWidth() - 110.0f;
        if (ImGui::GetCursorPosX() < statusPosX)
            ImGui::SetCursorPosX(statusPosX);
        if (snap.connected && snap.trading) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s LIVE", u8"\u25CF");
        } else if (snap.connected) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "%s IDLE", u8"\u25CF");
        } else {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s OFFLINE", u8"\u25CF");
        }
    }

    // ── Candlestick chart ──

    // КРИТИЧНО: получить реальный доступный размер ПОСЛЕ отрисовки тулбара
    ImVec2 availSize = ImGui::GetContentRegionAvail();

    // Защита от нулевого/отрицательного размера
    if (availSize.x < 100.0f || availSize.y < 100.0f) {
        ImGui::TextColored(ImVec4(1, 0.5f, 0, 1),
            "Window too small to render chart (%.0fx%.0f)",
            availSize.x, availSize.y);
        ImGui::End();
        return;
    }

    // Защита от пустого вектора баров
    if (snap.candleHistory.empty() || (int)snap.candleHistory.size() < 2) {
        ImGui::TextColored(ImVec4(1, 1, 0, 1),
            "No bar data loaded. Waiting...");
        ImGui::End();
        return;
    }

    {
        static constexpr double kPricePadding = 0.05; // 5% Y-axis padding
        float priceScaleW = 70.0f;
        float totalH = std::max(200.0f, availSize.y - 4.0f);
        // Layout: candles 65%, volume 20%, RSI 15%
        float priceH = totalH * 0.65f;
        float volH   = totalH * 0.20f;
        float rsiH   = totalH * 0.15f;
        ImVec2 canvasSize = ImVec2(availSize.x, totalH);

        // Throttled diagnostics — log once every ~5 seconds (300 frames at 60fps)
        {
            static int diagCounter = 0;
            if (++diagCounter >= 300) {
                diagCounter = 0;
                Logger::get()->debug("[MarketData] bars={} plotSize={}x{} first.time={} last.time={} first.open={} first.close={}",
                    snap.candleHistory.size(), canvasSize.x, canvasSize.y,
                    snap.candleHistory.front().openTime, snap.candleHistory.back().openTime,
                    snap.candleHistory.front().open, snap.candleHistory.front().close);
            }
        }

        int totalCount = (int)snap.candleHistory.size();

        // ── Apply reset: fit last 50 bars ──
        if (needsChartReset_ && totalCount >= 2) {
            float chartW = canvasSize.x - priceScaleW;
            int visibleCount = std::min(50, totalCount);
            chartBarWidth_ = std::clamp(chartW / (float)visibleCount - 1.0f,
                                        chartMinBarWidth_, chartMaxBarWidth_);
            chartScrollOffset_ = 0;
            needsChartReset_ = false;
        }

        ImGui::BeginChild("##CandleChart", canvasSize, ImGuiChildFlags_Border);

        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        float w = canvasSize.x - 4;
        float chartW = w - priceScaleW;

        // ── Mouse interaction: wheel = zoom, drag = pan ──
        bool hovered = ImGui::IsWindowHovered();
        if (hovered) {
            float wheel = ImGui::GetIO().MouseWheel;
            if (wheel != 0.0f) {
                float zoomFactor = 1.0f + wheel * 0.15f;
                chartBarWidth_ = std::clamp(chartBarWidth_ * zoomFactor,
                                            chartMinBarWidth_, chartMaxBarWidth_);
            }
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                float dx = ImGui::GetIO().MouseDelta.x;
                float barStepD = chartBarWidth_ + 1.0f;
                if (std::abs(dx) > 0.5f) {
                    int scrollDelta = (int)(dx / barStepD);
                    if (scrollDelta == 0) scrollDelta = (dx > 0) ? 1 : -1;
                    chartScrollOffset_ += scrollDelta;
                }
            }
        }

        // ── Compute visible bar range ──
        float barStep = chartBarWidth_ + 1.0f;
        int visibleBars = std::max(2, (int)(chartW / barStep));

        int maxScroll = std::max(0, totalCount - visibleBars);
        chartScrollOffset_ = std::clamp(chartScrollOffset_, 0, maxScroll);

        int startIdx = totalCount - visibleBars - chartScrollOffset_;
        if (startIdx < 0) startIdx = 0;
        int endIdx = std::min(totalCount, startIdx + visibleBars);

        // ── Find price/volume range for visible bars only ──
        double pMin = 1e18, pMax = -1e18;
        double vMax = 1.0;
        for (int i = startIdx; i < endIdx; ++i) {
            auto& c = snap.candleHistory[i];
            if (c.low  < pMin) pMin = c.low;
            if (c.high > pMax) pMax = c.high;
            if (c.volume > vMax) vMax = c.volume;
        }
        double range = pMax - pMin;
        if (range < 1e-9) range = 1.0;
        double padding = range * kPricePadding;
        pMin -= padding;
        pMax += padding;
        range = pMax - pMin;

        // Проверить что значения корректны
        if (pMin >= pMax || !std::isfinite(pMin) || !std::isfinite(pMax)) {
            Logger::get()->warn("[MarketData] Invalid axis range: y=[{},{}]", pMin, pMax);
            ImGui::Dummy(canvasSize);
            ImGui::EndChild();
            ImGui::End();
            return;
        }

        float halfBar = chartBarWidth_ * 0.5f;

        // ── Compute EMA9 & RSI for the full history ──
        auto ema9 = calcEMAFromCandles(snap.candleHistory, 9);
        auto rsiVec = calcRSIFromCandles(snap.candleHistory, 14);

        // ── Background grid lines (price section) ──
        for (int i = 0; i <= 6; ++i) {
            float frac = (float)i / 6.0f;
            float yLine = p.y + priceH - frac * priceH;
            draw->AddLine(ImVec2(p.x, yLine), ImVec2(p.x + chartW, yLine),
                          IM_COL32(40, 40, 45, 80), 1.0f);
        }

        // ── Price candles (top 65%) ──
        for (int i = startIdx; i < endIdx; ++i) {
            auto& c = snap.candleHistory[i];
            int vi = i - startIdx;
            float x = p.x + 2.0f + (float)vi * barStep;
            if (x > p.x + chartW) break;

            float yHigh  = p.y + priceH - (float)((c.high  - pMin) / range) * priceH;
            float yLow   = p.y + priceH - (float)((c.low   - pMin) / range) * priceH;
            float yOpen  = p.y + priceH - (float)((c.open  - pMin) / range) * priceH;
            float yClose = p.y + priceH - (float)((c.close - pMin) / range) * priceH;

            bool bullish = c.close >= c.open;
            ImU32 color = bullish
                ? IM_COL32(40, 200, 80, 255)
                : IM_COL32(220, 60, 60, 255);

            float wickX = x + halfBar;
            draw->AddLine(ImVec2(wickX, yHigh), ImVec2(wickX, yLow), color, 1.0f);

            float bodyTop = std::min(yOpen, yClose);
            float bodyBot = std::max(yOpen, yClose);
            if (bodyBot - bodyTop < 1.0f) bodyBot = bodyTop + 1.0f;
            draw->AddRectFilled(ImVec2(x, bodyTop), ImVec2(x + chartBarWidth_, bodyBot), color);
        }

        // ── 5.1: EMA9 overlay line (yellow) ──
        {
            ImU32 emaColor = IM_COL32(255, 204, 0, 220);
            float prevX = 0, prevY = 0;
            bool first = true;
            for (int i = startIdx; i < endIdx; ++i) {
                double val = ema9[i];
                if (val < pMin || val > pMax) { first = true; continue; }
                int vi = i - startIdx;
                float x = p.x + 2.0f + (float)vi * barStep + halfBar;
                float y = p.y + priceH - (float)((val - pMin) / range) * priceH;
                if (!first) {
                    draw->AddLine(ImVec2(prevX, prevY), ImVec2(x, y), emaColor, 1.5f);
                }
                prevX = x; prevY = y;
                first = false;
            }
            // Label
            draw->AddText(ImVec2(p.x + 4, p.y + 4), emaColor, "EMA9");
        }

        // ── Volume bars (middle 20%) ──
        float volTop = p.y + priceH + 2;
        draw->AddLine(ImVec2(p.x, volTop - 1), ImVec2(p.x + chartW, volTop - 1),
                      IM_COL32(60, 60, 65, 180), 1.0f);

        for (int i = startIdx; i < endIdx; ++i) {
            auto& c = snap.candleHistory[i];
            int vi = i - startIdx;
            float x = p.x + 2.0f + (float)vi * barStep;
            if (x > p.x + chartW) break;
            float barH = (float)(c.volume / vMax) * (volH - 6);

            bool bullish = c.close >= c.open;
            ImU32 vColor = bullish
                ? IM_COL32(40, 180, 80, 140)
                : IM_COL32(200, 60, 60, 140);

            float volBot = volTop + volH - 4;
            draw->AddRectFilled(ImVec2(x, volBot - barH), ImVec2(x + chartBarWidth_, volBot), vColor);
        }

        // ── 5.1: RSI subplot (bottom 15%) ──
        float rsiTop = volTop + volH + 2;
        draw->AddLine(ImVec2(p.x, rsiTop - 1), ImVec2(p.x + chartW, rsiTop - 1),
                      IM_COL32(60, 60, 65, 180), 1.0f);
        // RSI background levels: 30 (green), 50 (gray), 70 (red)
        {
            float rsiDrawH = rsiH - 6;
            float rsiBot = rsiTop + rsiDrawH;
            auto rsiY = [&](double val) -> float {
                return rsiBot - (float)(val / 100.0) * rsiDrawH;
            };
            // Level lines
            draw->AddLine(ImVec2(p.x, rsiY(70)), ImVec2(p.x + chartW, rsiY(70)),
                          IM_COL32(220, 60, 60, 80), 1.0f);
            draw->AddLine(ImVec2(p.x, rsiY(50)), ImVec2(p.x + chartW, rsiY(50)),
                          IM_COL32(180, 180, 180, 40), 1.0f);
            draw->AddLine(ImVec2(p.x, rsiY(30)), ImVec2(p.x + chartW, rsiY(30)),
                          IM_COL32(60, 200, 60, 80), 1.0f);
            // Level labels
            draw->AddText(ImVec2(p.x + chartW + 4, rsiY(70) - 6), IM_COL32(220, 60, 60, 180), "70");
            draw->AddText(ImVec2(p.x + chartW + 4, rsiY(30) - 6), IM_COL32(60, 200, 60, 180), "30");

            // RSI line
            ImU32 rsiColor = IM_COL32(128, 128, 255, 220);
            float prevX = 0, prevRsiY = 0;
            bool first = true;
            for (int i = startIdx; i < endIdx; ++i) {
                double val = std::clamp(rsiVec[i], 0.0, 100.0);
                int vi = i - startIdx;
                float x = p.x + 2.0f + (float)vi * barStep + halfBar;
                float y = rsiY(val);
                if (!first) {
                    draw->AddLine(ImVec2(prevX, prevRsiY), ImVec2(x, y), rsiColor, 1.5f);
                }
                prevX = x; prevRsiY = y;
                first = false;
            }
            // Label
            draw->AddText(ImVec2(p.x + 4, rsiTop + 2), rsiColor, "RSI(14)");
        }

        // ── Price scale on the right ──
        for (int i = 0; i <= 6; ++i) {
            float frac = (float)i / 6.0f;
            float yLine = p.y + priceH - frac * priceH;
            double priceVal = pMin + frac * range;
            char label[32];
            OrderManagement::formatPrice(label, sizeof(label), priceVal);
            draw->AddText(ImVec2(p.x + chartW + 4, yLine - 6),
                          IM_COL32(160, 160, 165, 220), label);
        }

        // ── Current price line (last close) ──
        if (endIdx > 0) {
            double lastPrice = snap.candleHistory[endIdx - 1].close;
            if (lastPrice >= pMin && lastPrice <= pMax) {
                float yLast = p.y + priceH - (float)((lastPrice - pMin) / range) * priceH;
                draw->AddLine(ImVec2(p.x, yLast), ImVec2(p.x + chartW, yLast),
                              IM_COL32(80, 140, 220, 150), 1.0f);
                char priceLabel[32];
                OrderManagement::formatPrice(priceLabel, sizeof(priceLabel), lastPrice);
                draw->AddRectFilled(ImVec2(p.x + chartW + 1, yLast - 8),
                                    ImVec2(p.x + w, yLast + 8),
                                    IM_COL32(60, 120, 200, 200));
                draw->AddText(ImVec2(p.x + chartW + 4, yLast - 6),
                              IM_COL32(240, 240, 245, 255), priceLabel);
            }
        }

        // ── User indicator overlay ──
        if (!snap.userIndicatorPlots.empty()) {
            float labelY = p.y + 18;
            for (const auto& [indName, plots] : snap.userIndicatorPlots) {
                for (const auto& [pName, pVal] : plots) {
                    if (pVal > pMin && pVal < pMax) {
                        float yVal = p.y + priceH - (float)((pVal - pMin) / range) * priceH;
                        draw->AddLine(ImVec2(p.x, yVal), ImVec2(p.x + chartW, yVal),
                                      IM_COL32(140, 100, 200, 120), 1.0f);
                        char overlayLabel[64];
                        snprintf(overlayLabel, sizeof(overlayLabel), "%s: %.8f", pName.c_str(), pVal);
                        draw->AddText(ImVec2(p.x + 4, labelY),
                                      IM_COL32(180, 140, 240, 200), overlayLabel);
                        labelY += 14;
                    }
                }
            }
        }

        // ── 5.3: Crosshair with price label, time label, and OHLCV tooltip ──
        if (hovered) {
            ImVec2 mouse = ImGui::GetMousePos();
            float mx = mouse.x, my = mouse.y;
            // Only draw crosshair inside the chart drawing area
            if (mx >= p.x && mx <= p.x + chartW && my >= p.y && my <= p.y + totalH) {
                // Vertical line
                draw->AddLine(ImVec2(mx, p.y), ImVec2(mx, p.y + totalH),
                              IM_COL32(255, 255, 255, 60), 1.0f);
                // Horizontal line (only in price section)
                if (my <= p.y + priceH) {
                    draw->AddLine(ImVec2(p.x, my), ImVec2(p.x + chartW, my),
                                  IM_COL32(255, 255, 255, 60), 1.0f);
                    // Price label at crosshair Y
                    double crossPrice = pMax - ((double)(my - p.y) / priceH) * range;
                    char crossLabel[32];
                    OrderManagement::formatPrice(crossLabel, sizeof(crossLabel), crossPrice);
                    draw->AddRectFilled(ImVec2(p.x + chartW + 1, my - 8),
                                        ImVec2(p.x + w, my + 8),
                                        IM_COL32(80, 80, 90, 200));
                    draw->AddText(ImVec2(p.x + chartW + 4, my - 6),
                                  IM_COL32(240, 240, 245, 255), crossLabel);
                }

                // Find nearest visible bar to cursor X
                int nearestIdx = -1;
                float nearestDist = FLT_MAX;
                for (int i = startIdx; i < endIdx; ++i) {
                    int vi = i - startIdx;
                    float bx = p.x + 2.0f + (float)vi * barStep + halfBar;
                    float dist = std::abs(mx - bx);
                    if (dist < nearestDist) {
                        nearestDist = dist;
                        nearestIdx = i;
                    }
                }

                // Time label at bottom of chart (under crosshair X position)
                if (nearestIdx >= 0 && nearestIdx < (int)snap.candleHistory.size()) {
                    auto& nearBar = snap.candleHistory[nearestIdx];
                    // Time label at crosshair X position
                    time_t barTimeSec = (time_t)(nearBar.openTime / 1000);
                    struct tm tmBuf{};
#ifdef _WIN32
                    bool tmOk = (localtime_s(&tmBuf, &barTimeSec) == 0);
#else
                    bool tmOk = (localtime_r(&barTimeSec, &tmBuf) != nullptr);
#endif
                    char timeBuf[32];
                    if (tmOk) {
                        strftime(timeBuf, sizeof(timeBuf), " %H:%M ", &tmBuf);
                    } else {
                        snprintf(timeBuf, sizeof(timeBuf), " --:-- ");
                    }
                    float timeLabelW = 50.0f;
                    float tlx = mx - timeLabelW * 0.5f;
                    float tly = p.y + totalH - 16.0f;
                    draw->AddRectFilled(ImVec2(tlx, tly),
                                        ImVec2(tlx + timeLabelW, tly + 16.0f),
                                        IM_COL32(40, 40, 50, 220));
                    draw->AddText(ImVec2(tlx + 3.0f, tly + 2.0f),
                                  IM_COL32(240, 240, 245, 255), timeBuf);

                    // OHLCV tooltip
                    ImGui::BeginTooltip();
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f),
                        "O: %.8f  H: %.8f", nearBar.open, nearBar.high);
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f),
                        "L: %.8f  C: %.8f", nearBar.low, nearBar.close);
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.7f, 1.0f),
                        "Vol: %.2f", nearBar.volume);
                    char dateBuf[64];
                    if (tmOk) {
                        strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d %H:%M", &tmBuf);
                    } else {
                        snprintf(dateBuf, sizeof(dateBuf), "--");
                    }
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 1.0f), "%s", dateBuf);
                    ImGui::EndTooltip();
                }
            }

            // ── 5.4: Right-click context menu for placing orders at price level ──
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && mx >= p.x && mx <= p.x + chartW && my >= p.y && my <= p.y + priceH) {
                chartRightClickPrice_ = pMax - ((double)(my - p.y) / priceH) * range;
                chartRightClickOpen_ = true;
                ImGui::OpenPopup("ChartOrderMenu");
            }
        }

        // ── H3: Trade markers ──
        {
            auto markers = tradeMarkerMgr_.getMarkers();
            for (auto& m : markers) {
                // Find which bar this trade maps to
                for (int bi = startIdx; bi < endIdx; ++bi) {
                    auto& bar = snap.candleHistory[bi];
                    if (bar.openTime <= (int64_t)m.time && (int64_t)m.time <= bar.closeTime) {
                        float barX = p.x + (float)(bi - startIdx) * barStep + barStep * 0.5f;
                        float priceY = (float)(p.y + priceH - ((m.price - pMin) / range) * priceH);
                        if (m.isEntry) {
                            if (m.side == "BUY") {
                                draw->AddTriangleFilled(
                                    ImVec2(barX, priceY + 4),
                                    ImVec2(barX - 6, priceY + 14),
                                    ImVec2(barX + 6, priceY + 14),
                                    IM_COL32(0, 220, 0, 255));
                                draw->AddText(ImVec2(barX + 8, priceY),
                                    IM_COL32(0, 220, 0, 255), "B");
                            } else {
                                draw->AddTriangleFilled(
                                    ImVec2(barX, priceY - 4),
                                    ImVec2(barX - 6, priceY - 14),
                                    ImVec2(barX + 6, priceY - 14),
                                    IM_COL32(220, 0, 0, 255));
                                draw->AddText(ImVec2(barX + 8, priceY - 14),
                                    IM_COL32(220, 0, 0, 255), "S");
                            }
                        } else {
                            // Exit marker — white X + PnL tag
                            draw->AddText(ImVec2(barX - 4, priceY - 7),
                                IM_COL32(255, 255, 255, 200), "X");
                            char pnlBuf[32];
                            snprintf(pnlBuf, sizeof(pnlBuf), "%.8f", m.pnl);
                            draw->AddRectFilled(
                                ImVec2(barX + 6, priceY - 8),
                                ImVec2(barX + 55, priceY + 8),
                                m.pnl >= 0 ? IM_COL32(0, 80, 0, 180)
                                           : IM_COL32(80, 0, 0, 180));
                            draw->AddText(ImVec2(barX + 8, priceY - 6),
                                m.pnl >= 0 ? IM_COL32(0, 255, 0, 255)
                                           : IM_COL32(255, 0, 0, 255), pnlBuf);
                        }
                        break;
                    }
                }
            }

            // TP/SL horizontal lines for open positions
            auto tpslLines = tradeMarkerMgr_.getTPSLLines();
            for (auto& line : tpslLines) {
                if (line.tp > 0 && line.tp >= pMin && line.tp <= pMax) {
                    float tpY = (float)(p.y + priceH - ((line.tp - pMin) / range) * priceH);
                    draw->AddLine(ImVec2(p.x, tpY), ImVec2(p.x + chartW, tpY),
                                  IM_COL32(0, 200, 0, 150), 1.0f);
                    draw->AddText(ImVec2(p.x + chartW + 4, tpY - 6),
                                  IM_COL32(0, 200, 0, 200), "TP");
                }
                if (line.sl > 0 && line.sl >= pMin && line.sl <= pMax) {
                    float slY = (float)(p.y + priceH - ((line.sl - pMin) / range) * priceH);
                    draw->AddLine(ImVec2(p.x, slY), ImVec2(p.x + chartW, slY),
                                  IM_COL32(200, 0, 0, 150), 1.0f);
                    draw->AddText(ImVec2(p.x + chartW + 4, slY - 6),
                                  IM_COL32(200, 0, 0, 200), "SL");
                }
            }
        }

        // ── 5.5: Right-click context menu popup for chart order placement ──
        if (ImGui::BeginPopup("ChartOrderMenu")) {
            char priceLabel[64];
            OrderManagement::formatPrice(priceLabel, sizeof(priceLabel), chartRightClickPrice_);
            char priceLabelFull[80];
            snprintf(priceLabelFull, sizeof(priceLabelFull), "Price: %s", priceLabel);
            ImGui::TextColored(ImVec4(0.7f, 0.8f, 0.9f, 1.0f), "%s", priceLabelFull);
            ImGui::Separator();
            if (ImGui::MenuItem("Buy Limit at this price")) {
                omSideIdx_ = 0;
                omTypeIdx_ = 1;  // LIMIT
                omPrice_ = chartRightClickPrice_;
                showOrderManagement_ = true;
                addLog("[Chart] Buy Limit set at " + std::to_string(chartRightClickPrice_));
            }
            if (ImGui::MenuItem("Sell Limit at this price")) {
                omSideIdx_ = 1;
                omTypeIdx_ = 1;  // LIMIT
                omPrice_ = chartRightClickPrice_;
                showOrderManagement_ = true;
                addLog("[Chart] Sell Limit set at " + std::to_string(chartRightClickPrice_));
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Buy Stop-Limit at this price")) {
                omSideIdx_ = 0;
                omTypeIdx_ = 2;  // STOP_LIMIT
                omStopPrice_ = chartRightClickPrice_;
                showOrderManagement_ = true;
            }
            if (ImGui::MenuItem("Sell Stop-Limit at this price")) {
                omSideIdx_ = 1;
                omTypeIdx_ = 2;  // STOP_LIMIT
                omStopPrice_ = chartRightClickPrice_;
                showOrderManagement_ = true;
            }
            ImGui::EndPopup();
        }

        // ── 5.6: Draw open order price lines on chart ──
        if (!snap.openOrders.empty()) {
            for (auto& ord : snap.openOrders) {
                double oPrice = ord.price;
                if (oPrice >= pMin && oPrice <= pMax) {
                    float ordY = (float)(p.y + priceH - ((oPrice - pMin) / range) * priceH);
                    ImU32 ordCol = (ord.side == "BUY")
                        ? IM_COL32(0, 180, 80, 160)
                        : IM_COL32(200, 60, 60, 160);
                    draw->AddLine(ImVec2(p.x, ordY), ImVec2(p.x + chartW, ordY),
                                  ordCol, 1.0f);
                    char ordLabel[64];
                    snprintf(ordLabel, sizeof(ordLabel), "%s %.4f @ %.8f",
                             ord.side.c_str(), ord.qty, oPrice);
                    draw->AddRectFilled(ImVec2(p.x + chartW + 1, ordY - 8),
                                        ImVec2(p.x + w, ordY + 8), ordCol);
                    draw->AddText(ImVec2(p.x + chartW + 4, ordY - 6),
                                  IM_COL32(255, 255, 255, 230), ordLabel);
                }
            }
        }

        // ── Info bar ──
        {
            char info[128];
            snprintf(info, sizeof(info), "Bars: %d/%d  Zoom: %.0f%%  Offset: %d",
                     endIdx - startIdx, totalCount,
                     (chartBarWidth_ / 8.0f) * 100.0f, chartScrollOffset_);
            draw->AddText(ImVec2(p.x + 4, p.y + totalH - 16),
                          IM_COL32(100, 100, 105, 180), info);
        }

        ImGui::Dummy(canvasSize);
        ImGui::EndChild();
    }

    ImGui::End();
}

// ---------------------------------------------------------------------------
//  Indicators Panel — standalone resizable window
// ---------------------------------------------------------------------------
void AppGui::drawIndicatorsPanel() {
    GuiState snap;
    {
        std::lock_guard<std::mutex> lk(stateMutex_);
        snap = state_;
    }

    auto layout = layoutMgr_.get("Indicators");
    if (config_.layoutLocked || layoutNeedsReset_)
        LayoutManager::lockWindow("Indicators", layout.pos, layout.size);
    else
        LayoutManager::lockWindowOnce("Indicators", layout.pos, layout.size);

    int wflags = config_.layoutLocked ? LayoutManager::lockedScrollFlags()
                 : (ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_HorizontalScrollbar);

    if (!ImGui::Begin("Indicators", nullptr, wflags)) {
        ImGui::End();
        return;
    }

    // Zoom / Pan interaction for indicator content
    if (ImGui::IsWindowHovered()) {
        ImGuiIO& io = ImGui::GetIO();
        float wheel = io.MouseWheel;
        if (wheel != 0.0f) {
            if (io.KeyShift) {
                indZoomY_ *= (wheel > 0) ? 1.1f : 0.9f;
                indZoomY_ = std::clamp(indZoomY_, 0.3f, 5.0f);
            } else {
                indZoomX_ *= (wheel > 0) ? 1.1f : 0.9f;
                indZoomX_ = std::clamp(indZoomX_, 0.3f, 5.0f);
            }
        }
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            ImVec2 delta = io.MouseDelta;
            indPanX_ += delta.x;
            indPanY_ += delta.y;
        }
    }

    // Scrollable child area with zoom-scaled virtual size
    float childW = ImGui::GetContentRegionAvail().x * indZoomX_;
    float childH = ImGui::GetContentRegionAvail().y * indZoomY_;
    if (childH < 80.0f) childH = 80.0f;

    ImGui::BeginChild("##IndContent", ImVec2(childW, childH), ImGuiChildFlags_None,
                       ImGuiWindowFlags_HorizontalScrollbar);

    // EMA
    if (config_.indEmaEnabled) {
        ImGui::Columns(3, "##indcols", false);
        ImGui::TextColored(ImVec4(0.50f, 0.75f, 0.50f, 1.0f), "EMA Fast (%d)", config_.emaFast);
        ImGui::Text("%.8f", snap.emaFast);
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.75f, 0.50f, 0.50f, 1.0f), "EMA Slow (%d)", config_.emaSlow);
        ImGui::Text("%.8f", snap.emaSlow);
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.75f, 1.0f), "EMA Trend (%d)", config_.emaTrend);
        ImGui::Text("%.8f", snap.emaTrend);
        ImGui::Columns(1);
        ImGui::Separator();
    }

    // RSI & ATR & MACD
    {
        int cols = 0;
        if (config_.indRsiEnabled) ++cols;
        if (config_.indAtrEnabled) ++cols;
        if (config_.indMacdEnabled) ++cols;
        if (cols > 0) {
            ImGui::Columns(std::max(cols, 1), "##indcols2", false);

            if (config_.indRsiEnabled) {
                ImGui::Text("RSI (%d)", config_.rsiPeriod);
                float rsiVal = (float)snap.rsi;
                ImVec4 rsiColor = (rsiVal > 70) ? ImVec4(0.90f, 0.30f, 0.30f, 1.0f) :
                                  (rsiVal < 30) ? ImVec4(0.30f, 0.90f, 0.30f, 1.0f) :
                                                  ImVec4(0.80f, 0.80f, 0.30f, 1.0f);
                ImGui::TextColored(rsiColor, "%.2f", snap.rsi);
                ImGui::NextColumn();
            }
            if (config_.indAtrEnabled) {
                ImGui::Text("ATR (%d)", config_.atrPeriod);
                ImGui::Text("%.8f", snap.atr);
                ImGui::NextColumn();
            }
            if (config_.indMacdEnabled) {
                ImGui::Text("MACD");
                ImGui::TextColored(
                    snap.macd.histogram >= 0
                        ? ImVec4(0.30f, 0.80f, 0.30f, 1.0f)
                        : ImVec4(0.80f, 0.30f, 0.30f, 1.0f),
                    "%.8f (S: %.8f H: %.8f)",
                    snap.macd.macd, snap.macd.signal, snap.macd.histogram);
                ImGui::NextColumn();
            }
            ImGui::Columns(1);
        }
    }

    // Bollinger Bands
    if (config_.indBbEnabled) {
        ImGui::Separator();
        ImGui::Text("Bollinger Bands (%d, %.1f)", config_.bbPeriod, config_.bbStddev);
        ImGui::SameLine(220);
        ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.80f, 1.0f),
            "Upper: %.8f  Mid: %.8f  Lower: %.8f",
            snap.bb.upper, snap.bb.middle, snap.bb.lower);
    }

    // RSI mini chart
    if (config_.indRsiEnabled) {
        std::lock_guard<std::mutex> lk(stateMutex_);
        if (!rsiHistory_.empty()) {
            std::vector<float> v;
            v.reserve(rsiHistory_.size());
            for (double d : rsiHistory_) v.push_back(static_cast<float>(d));
            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.85f, 0.70f, 0.30f, 1.0f));
            ImGui::PlotLines("RSI##chart", v.data(), (int)v.size(), 0,
                             nullptr, 0.0f, 100.0f, ImVec2(-1, 40));
            ImGui::PopStyleColor();
        }
    }

    // User Pine Script indicators
    if (!snap.userIndicatorPlots.empty()) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.70f, 0.50f, 0.90f, 1.0f), "User Indicators (Pine Script)");
        for (const auto& [indName, plots] : snap.userIndicatorPlots) {
            if (plots.empty()) continue;
            ImGui::Text("  %s:", indName.c_str());
            ImGui::SameLine(180);
            std::string plotStr;
            for (const auto& [pName, pVal] : plots) {
                if (!plotStr.empty()) plotStr += "  ";
                char buf[64];
                std::snprintf(buf, sizeof(buf), "%s: %.8f", pName.c_str(), pVal);
                plotStr += buf;
            }
            ImGui::TextColored(ImVec4(0.70f, 0.70f, 0.90f, 1.0f), "%s", plotStr.c_str());
        }
    }

    ImGui::EndChild();  // ##IndContent

    // Zoom info bar
    {
        char info[64];
        snprintf(info, sizeof(info), "Zoom X:%.0f%% Y:%.0f%%",
                 indZoomX_ * 100.0f, indZoomY_ * 100.0f);
        ImGui::TextColored(ImVec4(0.40f, 0.40f, 0.42f, 1.0f), "%s", info);
    }

    ImGui::End();
}

// ---------------------------------------------------------------------------
//  User Indicator Dashboard (Pine Script)
// ---------------------------------------------------------------------------
void AppGui::drawUserIndicatorDashboard() {
    GuiState snap;
    {
        std::lock_guard<std::mutex> lk(stateMutex_);
        snap = state_;
    }

    if (snap.userIndicatorPlots.empty()) return;

    if (ImGui::CollapsingHeader("User Indicator Dashboard", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BeginChild("##UserIndDash", ImVec2(0, 280), ImGuiChildFlags_Border);

        for (const auto& [indName, plots] : snap.userIndicatorPlots) {
            if (plots.empty()) continue;

            ImGui::TextColored(ImVec4(0.70f, 0.50f, 0.90f, 1.0f), "%s", indName.c_str());
            ImGui::Separator();

            // Display plots in a two-column table layout
            if (ImGui::BeginTable(("##uind_" + indName).c_str(), 2,
                                  ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV)) {
                ImGui::TableSetupColumn("Indicator", ImGuiTableColumnFlags_WidthStretch, 0.55f);
                ImGui::TableSetupColumn("Value",     ImGuiTableColumnFlags_WidthStretch, 0.45f);
                ImGui::TableHeadersRow();

                for (const auto& [pName, pVal] : plots) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextColored(ImVec4(0.70f, 0.70f, 0.90f, 1.0f), "%s", pName.c_str());
                    ImGui::TableSetColumnIndex(1);

                    // Color-code values: positive green, negative red, zero/NaN grey
                    ImVec4 valColor;
                    if (std::isnan(pVal)) {
                        valColor = ImVec4(0.50f, 0.50f, 0.52f, 1.0f);
                        ImGui::TextColored(valColor, "N/A");
                    } else if (pVal > 0) {
                        valColor = ImVec4(0.30f, 0.85f, 0.35f, 1.0f);
                        ImGui::TextColored(valColor, "%.8f", pVal);
                    } else if (pVal < 0) {
                        valColor = ImVec4(0.90f, 0.30f, 0.30f, 1.0f);
                        ImGui::TextColored(valColor, "%.8f", pVal);
                    } else {
                        valColor = ImVec4(0.80f, 0.80f, 0.82f, 1.0f);
                        ImGui::TextColored(valColor, "%.8f", pVal);
                    }
                }
                ImGui::EndTable();
            }
            ImGui::Spacing();
        }

        ImGui::EndChild();
    }
}

// ---------------------------------------------------------------------------
//  Signal Panel
// ---------------------------------------------------------------------------
void AppGui::drawSignalPanel() {
    GuiState snap;
    {
        std::lock_guard<std::mutex> lk(stateMutex_);
        snap = state_;
    }

    if (ImGui::CollapsingHeader("Trading Signal", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BeginChild("##SigChild", ImVec2(0, 220), ImGuiChildFlags_Border);

        // Current signal
        const char* sigStr = "HOLD";
        ImVec4 sigColor(0.60f, 0.60f, 0.60f, 1.0f);
        if (snap.lastSignal.type == Signal::Type::BUY) {
            sigStr = "BUY";
            sigColor = ImVec4(0.20f, 0.85f, 0.30f, 1.0f);
        } else if (snap.lastSignal.type == Signal::Type::SELL) {
            sigStr = "SELL";
            sigColor = ImVec4(0.90f, 0.25f, 0.25f, 1.0f);
        }

        ImGui::TextColored(ImVec4(0.70f, 0.70f, 0.72f, 1.0f), "Current Signal:");
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, sigColor);
        ImGui::SetWindowFontScale(1.5f);
        ImGui::Text("%s", sigStr);
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();

        // Confidence bar
        ImGui::Text("Confidence:");
        ImGui::SameLine();
        float conf = (float)snap.lastSignal.confidence;
        ImVec4 barColor = (conf > 0.75f) ? ImVec4(0.20f, 0.70f, 0.30f, 1.0f) :
                          (conf > 0.50f) ? ImVec4(0.70f, 0.70f, 0.20f, 1.0f) :
                                           ImVec4(0.70f, 0.30f, 0.20f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f%%", conf * 100.0f);
        ImGui::ProgressBar(conf, ImVec2(-1, 20), buf);
        ImGui::PopStyleColor();

        ImGui::Separator();
        ImGui::Text("Stop Loss:   %.8f", snap.lastSignal.stopLoss);
        ImGui::Text("Take Profit: %.8f", snap.lastSignal.takeProfit);
        if (!snap.lastSignal.reason.empty())
            ImGui::TextWrapped("Reason: %s", snap.lastSignal.reason.c_str());

        // Signal history
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.62f, 1.0f), "Recent Signals");
        {
            std::lock_guard<std::mutex> lk(stateMutex_);
            int n = std::min((int)signalHistory_.size(), 8);
            for (int i = (int)signalHistory_.size() - 1;
                 i >= (int)signalHistory_.size() - n && i >= 0; --i) {
                auto& sig = signalHistory_[i];
                const char* t = (sig.type == Signal::Type::BUY) ? "BUY" : "SELL";
                ImVec4 cl = (sig.type == Signal::Type::BUY)
                    ? ImVec4(0.20f, 0.80f, 0.30f, 1.0f)
                    : ImVec4(0.85f, 0.25f, 0.25f, 1.0f);
                ImGui::TextColored(cl, "  %s @ %.8f (%.1f%%)",
                                   t, sig.price, sig.confidence * 100.0);
            }
        }

        ImGui::EndChild();
    }
}

// ---------------------------------------------------------------------------
//  Trading Panel (Long / Short manual orders)
// ---------------------------------------------------------------------------
void AppGui::drawTradingPanel() {
    bool connected;
    {
        std::lock_guard<std::mutex> lk(stateMutex_);
        connected = state_.connected;
    }

    if (ImGui::CollapsingHeader(u8"Manual Trading", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BeginChild("##TradeChild", ImVec2(0, 180), ImGuiChildFlags_Border);

        if (!connected) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                               u8"Connect to exchange to trade");
            ImGui::EndChild();
            return;
        }

        // Order type
        const char* orderTypes[] = {"Market", "Limit"};
        ImGui::SetNextItemWidth(120);
        ImGui::Combo("Type", &orderTypeIdx_, orderTypes, 2);

        // Quantity
        ImGui::SetNextItemWidth(120);
        ImGui::InputDouble("Qty", &orderQty_, 0.001, 0.01, "%.8f");
        if (orderQty_ < 0.0) orderQty_ = 0.0;

        // Limit price (only for Limit orders)
        if (orderTypeIdx_ == 1) {
            ImGui::SetNextItemWidth(120);
            ImGui::InputDouble("Price", &orderPrice_, 1.0, 10.0, "%.8f");
        }

        ImGui::Separator();
        float btnW = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
        std::string typeStr = (orderTypeIdx_ == 0) ? "MARKET" : "LIMIT";

        // Long button (green)
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.13f, 0.55f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.18f, 0.68f, 0.32f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.10f, 0.45f, 0.20f, 1.0f));
        if (ImGui::Button("Long", ImVec2(btnW, 36))) {
            if (onOrder_ && orderQty_ > 0.0) {
                onOrder_(config_.symbol, "BUY", typeStr, orderQty_, orderPrice_);
                addLog("[BUY] Long " + std::to_string(orderQty_) + " " + config_.symbol);
            }
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();

        // Short button (red)
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.60f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.75f, 0.20f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.50f, 0.12f, 0.12f, 1.0f));
        if (ImGui::Button("Short", ImVec2(btnW, 36))) {
            if (onOrder_ && orderQty_ > 0.0) {
                onOrder_(config_.symbol, "SELL", typeStr, orderQty_, orderPrice_);
                addLog("[SELL] Short " + std::to_string(orderQty_) + " " + config_.symbol);
            }
        }
        ImGui::PopStyleColor(3);

        ImGui::EndChild();
    }
}

// ---------------------------------------------------------------------------
//  Portfolio Panel
// ---------------------------------------------------------------------------
void AppGui::drawPortfolioPanel() {
    GuiState snap;
    {
        std::lock_guard<std::mutex> lk(stateMutex_);
        snap = state_;
    }

    if (ImGui::CollapsingHeader("Portfolio & Risk", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BeginChild("##PortChild", ImVec2(0, 0), ImGuiChildFlags_Border);

        // Equity
        ImGui::TextColored(ImVec4(0.70f, 0.70f, 0.72f, 1.0f), "Equity");
        double pnl = snap.equity - snap.initialCapital;
        double pnlPct = snap.initialCapital > 0 ? (pnl / snap.initialCapital) * 100.0 : 0.0;
        ImVec4 eqColor = (pnl >= 0)
            ? ImVec4(0.30f, 0.85f, 0.35f, 1.0f)
            : ImVec4(0.90f, 0.30f, 0.30f, 1.0f);
        ImGui::TextColored(eqColor, "$%.8f", snap.equity);
        ImGui::SameLine();
        ImGui::TextColored(eqColor, " (%+.2f%%)", pnlPct);

        ImGui::Separator();

        // Drawdown
        ImGui::Text("Drawdown:");
        ImGui::SameLine();
        float dd = (float)snap.drawdown;
        ImVec4 ddColor = (dd > 0.10f) ? ImVec4(0.90f, 0.25f, 0.25f, 1.0f) :
                         (dd > 0.05f) ? ImVec4(0.80f, 0.60f, 0.20f, 1.0f) :
                                        ImVec4(0.40f, 0.70f, 0.40f, 1.0f);
        ImGui::TextColored(ddColor, "%.2f%%", dd * 100.0f);

        ImGui::Text("Open Positions: %d", snap.openPositions);
        ImGui::Text("Max Risk/Trade: %.1f%%", config_.maxRiskPerTrade * 100.0);
        ImGui::Text("Max Drawdown:   %.1f%%", config_.maxDrawdown * 100.0);
        ImGui::Text("R:R Ratio:      %.1f", config_.rrRatio);

        // Equity chart
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.62f, 1.0f), "Equity Curve");
        {
            std::lock_guard<std::mutex> lk(stateMutex_);
            PlotLineFromDeque("##equity", equityHistory_, 100,
                              ImVec4(0.35f, 0.75f, 0.35f, 1.0f));
        }

        ImGui::EndChild();
    }
}

// ---------------------------------------------------------------------------
//  Settings Panel (separate window)
// ---------------------------------------------------------------------------
void AppGui::drawSettingsPanel() {
    ImGui::SetNextWindowSize(ImVec2(600, 700), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Settings", &showSettings_)) {
        ImGui::End();
        return;
    }

    if (ImGui::BeginTabBar("##SettingsTabs")) {
        // ── Exchange ────────────────────────────────────
        if (ImGui::BeginTabItem("Exchange")) {
            ImGui::Text("Exchange Configuration");
            ImGui::Separator();

            const char* exchanges[] = { "binance", "bybit", "okx", "bitget", "kucoin" };
            int exIdx = 0;
            for (int i = 0; i < 5; ++i) {
                if (config_.exchangeName == exchanges[i]) { exIdx = i; break; }
            }
            if (ImGui::Combo("Exchange", &exIdx, exchanges, 5)) {
                config_.exchangeName = exchanges[exIdx];
            }

            static char apiKeyBuf[256] = {};
            static char apiSecBuf[256] = {};
            static char passBuf[256] = {};
            static char baseUrlBuf[256] = {};
            static char wsHostBuf[256] = {};
            static char wsPortBuf[16] = {};
            static char futuresBaseUrlBuf[256] = {};
            static char futuresWsHostBuf[256] = {};
            static char futuresWsPortBuf[16] = {};
            static bool bufInit = false;
            if (!bufInit) {
                strncpy(apiKeyBuf, config_.apiKey.c_str(), sizeof(apiKeyBuf) - 1);
                strncpy(apiSecBuf, config_.apiSecret.c_str(), sizeof(apiSecBuf) - 1);
                strncpy(passBuf, config_.passphrase.c_str(), sizeof(passBuf) - 1);
                strncpy(baseUrlBuf, config_.baseUrl.c_str(), sizeof(baseUrlBuf) - 1);
                strncpy(wsHostBuf, config_.wsHost.c_str(), sizeof(wsHostBuf) - 1);
                strncpy(wsPortBuf, config_.wsPort.c_str(), sizeof(wsPortBuf) - 1);
                strncpy(futuresBaseUrlBuf, config_.futuresBaseUrl.c_str(), sizeof(futuresBaseUrlBuf) - 1);
                strncpy(futuresWsHostBuf, config_.futuresWsHost.c_str(), sizeof(futuresWsHostBuf) - 1);
                strncpy(futuresWsPortBuf, config_.futuresWsPort.c_str(), sizeof(futuresWsPortBuf) - 1);
                bufInit = true;
            }
            ImGui::InputText("API Key", apiKeyBuf, sizeof(apiKeyBuf));
            ImGui::InputText("API Secret", apiSecBuf, sizeof(apiSecBuf),
                             ImGuiInputTextFlags_Password);
            ImGui::InputText("Passphrase", passBuf, sizeof(passBuf),
                             ImGuiInputTextFlags_Password);
            ImGui::InputText("Base URL (Spot)", baseUrlBuf, sizeof(baseUrlBuf));
            ImGui::InputText("WS Host (Futures)", futuresWsHostBuf, sizeof(futuresWsHostBuf));
            ImGui::InputText("WS Port (Futures, default 9443)", futuresWsPortBuf, sizeof(futuresWsPortBuf));
            ImGui::Separator();
            ImGui::Text("Advanced");
            ImGui::InputText("Spot WS Host", wsHostBuf, sizeof(wsHostBuf));
            ImGui::InputText("Spot WS Port", wsPortBuf, sizeof(wsPortBuf));
            ImGui::InputText("Futures Base URL", futuresBaseUrlBuf, sizeof(futuresBaseUrlBuf));
            ImGui::Separator();
            bool prevTestnet = config_.testnet;
            ImGui::Checkbox("Testnet", &config_.testnet);
            // Auto-populate defaults when testnet toggle changes (Binance)
            if (config_.testnet != prevTestnet && config_.exchangeName == "binance") {
                if (config_.testnet) {
                    strncpy(baseUrlBuf, "https://testnet.binance.vision", sizeof(baseUrlBuf) - 1);
                    baseUrlBuf[sizeof(baseUrlBuf) - 1] = '\0';
                    strncpy(wsHostBuf, "testnet.binance.vision", sizeof(wsHostBuf) - 1);
                    wsHostBuf[sizeof(wsHostBuf) - 1] = '\0';
                    strncpy(wsPortBuf, "443", sizeof(wsPortBuf) - 1);
                    wsPortBuf[sizeof(wsPortBuf) - 1] = '\0';
                    strncpy(futuresBaseUrlBuf, "https://testnet.binancefuture.com", sizeof(futuresBaseUrlBuf) - 1);
                    futuresBaseUrlBuf[sizeof(futuresBaseUrlBuf) - 1] = '\0';
                    strncpy(futuresWsHostBuf, "fstream.binancefuture.com", sizeof(futuresWsHostBuf) - 1);
                    futuresWsHostBuf[sizeof(futuresWsHostBuf) - 1] = '\0';
                    strncpy(futuresWsPortBuf, "443", sizeof(futuresWsPortBuf) - 1);
                    futuresWsPortBuf[sizeof(futuresWsPortBuf) - 1] = '\0';
                } else {
                    strncpy(baseUrlBuf, "https://api.binance.com", sizeof(baseUrlBuf) - 1);
                    baseUrlBuf[sizeof(baseUrlBuf) - 1] = '\0';
                    strncpy(wsHostBuf, "stream.binance.com", sizeof(wsHostBuf) - 1);
                    wsHostBuf[sizeof(wsHostBuf) - 1] = '\0';
                    strncpy(wsPortBuf, "9443", sizeof(wsPortBuf) - 1);
                    wsPortBuf[sizeof(wsPortBuf) - 1] = '\0';
                    strncpy(futuresBaseUrlBuf, "https://fapi.binance.com", sizeof(futuresBaseUrlBuf) - 1);
                    futuresBaseUrlBuf[sizeof(futuresBaseUrlBuf) - 1] = '\0';
                    strncpy(futuresWsHostBuf, "fstream.binance.com", sizeof(futuresWsHostBuf) - 1);
                    futuresWsHostBuf[sizeof(futuresWsHostBuf) - 1] = '\0';
                    strncpy(futuresWsPortBuf, "9443", sizeof(futuresWsPortBuf) - 1);
                    futuresWsPortBuf[sizeof(futuresWsPortBuf) - 1] = '\0';
                }
            }

            if (ImGui::Button("Apply Exchange Settings")) {
                config_.apiKey = apiKeyBuf;
                config_.apiSecret = apiSecBuf;
                config_.passphrase = passBuf;
                config_.baseUrl = baseUrlBuf;
                config_.wsHost = wsHostBuf;
                config_.wsPort = wsPortBuf;
                config_.futuresBaseUrl = futuresBaseUrlBuf;
                config_.futuresWsHost = futuresWsHostBuf;
                config_.futuresWsPort = futuresWsPortBuf;
                addLog("[Config] Exchange settings updated");
            }
            ImGui::EndTabItem();
        }

        // ── Trading ─────────────────────────────────────
        if (ImGui::BeginTabItem("Trading")) {
            ImGui::Text("Trading Parameters");
            ImGui::Separator();

            static char symBuf[32] = {};
            static bool symInit = false;
            if (!symInit) {
                strncpy(symBuf, config_.symbol.c_str(), sizeof(symBuf) - 1);
                symInit = true;
            }
            ImGui::InputText("Symbol", symBuf, sizeof(symBuf));

            const char* intervals[] = {
                "1m","3m","5m","10m","15m","20m","25m","30m","40m","45m","50m","55m",
                "1h","2h","3h","4h","5h","6h","7h","8h","9h","10h","11h","12h","14h","16h","18h","20h","22h",
                "1d","2d","3d","4d","5d","6d",
                "1w","2w","3w",
                "1M","2M","3M"
            };
            static constexpr int numIntervals = 39;
            int intIdx = 4; // 15m default
            for (int i = 0; i < numIntervals; ++i) {
                if (config_.interval == intervals[i]) { intIdx = i; break; }
            }
            if (ImGui::Combo("Interval", &intIdx, intervals, numIntervals)) {
                config_.interval = intervals[intIdx];
            }

            const char* modes[] = {"paper", "live"};
            int modeIdx = (config_.mode == "live") ? 1 : 0;
            if (ImGui::Combo("Mode", &modeIdx, modes, 2)) {
                config_.mode = modes[modeIdx];
            }

            const char* marketTypes[] = {"futures", "spot"};
            int mtIdx = (config_.marketType == "spot") ? 1 : 0;
            if (ImGui::Combo("Market Type", &mtIdx, marketTypes, 2)) {
                config_.marketType = marketTypes[mtIdx];
            }

            ImGui::InputDouble("Initial Capital ($)", &config_.initialCapital, 100.0, 1000.0, "%.2f");

            if (ImGui::Button("Apply Trading Settings")) {
                config_.symbol = symBuf;
                addLog("[Config] Trading settings updated");
            }
            ImGui::EndTabItem();
        }

        // ── Indicators ──────────────────────────────────
        if (ImGui::BeginTabItem("Indicators")) {
            ImGui::Text("Technical Indicator Parameters");
            ImGui::Separator();

            // Indicator selection: up to 5 simultaneously
            ImGui::Text("Select indicators (up to 5):");
            int activeCount = (config_.indEmaEnabled ? 1 : 0) +
                              (config_.indRsiEnabled ? 1 : 0) +
                              (config_.indAtrEnabled ? 1 : 0) +
                              (config_.indMacdEnabled ? 1 : 0) +
                              (config_.indBbEnabled ? 1 : 0);
            ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.62f, 1.0f),
                               "Active: %d / 5", activeCount);
            ImGui::Separator();

            // EMA checkbox & config
            if (!config_.indEmaEnabled && activeCount >= 5)
                ImGui::BeginDisabled();
            ImGui::Checkbox("EMA (Exponential Moving Average)", &config_.indEmaEnabled);
            if (!config_.indEmaEnabled && activeCount >= 5)
                ImGui::EndDisabled();
            if (config_.indEmaEnabled) {
                ImGui::Indent();
                ImGui::SliderInt("EMA Fast", &config_.emaFast, 2, 50);
                ImGui::SliderInt("EMA Slow", &config_.emaSlow, 5, 100);
                ImGui::SliderInt("EMA Trend", &config_.emaTrend, 50, 500);
                ImGui::Unindent();
            }

            // RSI checkbox & config
            if (!config_.indRsiEnabled && activeCount >= 5)
                ImGui::BeginDisabled();
            ImGui::Checkbox("RSI (Relative Strength Index)", &config_.indRsiEnabled);
            if (!config_.indRsiEnabled && activeCount >= 5)
                ImGui::EndDisabled();
            if (config_.indRsiEnabled) {
                ImGui::Indent();
                ImGui::SliderInt("RSI Period", &config_.rsiPeriod, 2, 50);
                {
                    float rsiOB = (float)config_.rsiOverbought;
                    if (ImGui::SliderFloat("RSI Overbought", &rsiOB, 50.0f, 90.0f, "%.0f"))
                        config_.rsiOverbought = rsiOB;
                    float rsiOS = (float)config_.rsiOversold;
                    if (ImGui::SliderFloat("RSI Oversold", &rsiOS, 10.0f, 50.0f, "%.0f"))
                        config_.rsiOversold = rsiOS;
                }
                ImGui::Unindent();
            }

            // ATR checkbox & config
            if (!config_.indAtrEnabled && activeCount >= 5)
                ImGui::BeginDisabled();
            ImGui::Checkbox("ATR (Average True Range)", &config_.indAtrEnabled);
            if (!config_.indAtrEnabled && activeCount >= 5)
                ImGui::EndDisabled();
            if (config_.indAtrEnabled) {
                ImGui::Indent();
                ImGui::SliderInt("ATR Period", &config_.atrPeriod, 2, 50);
                ImGui::Unindent();
            }

            // MACD checkbox & config
            if (!config_.indMacdEnabled && activeCount >= 5)
                ImGui::BeginDisabled();
            ImGui::Checkbox("MACD", &config_.indMacdEnabled);
            if (!config_.indMacdEnabled && activeCount >= 5)
                ImGui::EndDisabled();
            if (config_.indMacdEnabled) {
                ImGui::Indent();
                ImGui::SliderInt("MACD Fast", &config_.macdFast, 2, 50);
                ImGui::SliderInt("MACD Slow", &config_.macdSlow, 5, 100);
                ImGui::SliderInt("MACD Signal", &config_.macdSignal, 2, 50);
                ImGui::Unindent();
            }

            // BB checkbox & config
            if (!config_.indBbEnabled && activeCount >= 5)
                ImGui::BeginDisabled();
            ImGui::Checkbox("Bollinger Bands", &config_.indBbEnabled);
            if (!config_.indBbEnabled && activeCount >= 5)
                ImGui::EndDisabled();
            if (config_.indBbEnabled) {
                ImGui::Indent();
                ImGui::SliderInt("BB Period", &config_.bbPeriod, 5, 50);
                ImGui::InputDouble("BB Std Dev", &config_.bbStddev, 0.1, 0.5, "%.1f");
                ImGui::Unindent();
            }

            if (ImGui::Button("Apply Indicator Settings")) {
                addLog("[Config] Indicator settings updated");
            }
            ImGui::EndTabItem();
        }

        // ── ML ──────────────────────────────────────────
        if (ImGui::BeginTabItem("ML Models")) {
            ImGui::Text("Machine Learning Configuration");
            ImGui::Separator();

            ImGui::Text("LSTM Model");
            ImGui::SliderInt("Hidden Size", &config_.lstmHidden, 32, 512);
            ImGui::SliderInt("Sequence Length", &config_.lstmSeqLen, 10, 200);
            {
                float dropout = (float)config_.lstmDropout;
                if (ImGui::SliderFloat("Dropout", &dropout, 0.0f, 0.5f, "%.2f"))
                    config_.lstmDropout = dropout;
            }

            ImGui::Separator();
            ImGui::Text("XGBoost Model");
            ImGui::SliderInt("Max Depth", &config_.xgbMaxDepth, 1, 15);
            ImGui::SliderInt("Rounds", &config_.xgbRounds, 50, 2000);
            ImGui::InputDouble("Learning Rate (eta)", &config_.xgbEta, 0.01, 0.05, "%.3f");

            ImGui::Separator();
            ImGui::Text("Ensemble");
            {
                float minConf = (float)config_.ensembleMinConfidence;
                if (ImGui::SliderFloat("Min Confidence", &minConf, 0.5f, 0.9f, "%.2f"))
                    config_.ensembleMinConfidence = minConf;
            }
            ImGui::SliderInt("Retrain Interval (h)", &config_.retrainIntervalHours, 1, 168);
            ImGui::SliderInt("Lookback (days)", &config_.lookbackDays, 7, 365);

            if (ImGui::Button("Apply ML Settings")) {
                addLog("[Config] ML settings updated");
            }
            ImGui::EndTabItem();
        }

        // ── Risk ────────────────────────────────────────
        if (ImGui::BeginTabItem("Risk")) {
            ImGui::Text("Risk Management");
            ImGui::Separator();

            float riskPct = (float)(config_.maxRiskPerTrade * 100.0);
            if (ImGui::SliderFloat("Max Risk/Trade (%)", &riskPct, 0.5f, 10.0f, "%.1f")) {
                config_.maxRiskPerTrade = riskPct / 100.0;
            }

            float ddPct = (float)(config_.maxDrawdown * 100.0);
            if (ImGui::SliderFloat("Max Drawdown (%)", &ddPct, 1.0f, 50.0f, "%.1f")) {
                config_.maxDrawdown = ddPct / 100.0;
            }

            ImGui::InputDouble("ATR Stop Multiplier", &config_.atrStopMultiplier,
                               0.1, 0.5, "%.1f");
            ImGui::InputDouble("Reward/Risk Ratio", &config_.rrRatio, 0.1, 0.5, "%.1f");

            if (ImGui::Button("Apply Risk Settings")) {
                addLog("[Config] Risk settings updated");
            }
            ImGui::EndTabItem();
        }

        // ── Layout ──────────────────────────────────────
        if (ImGui::BeginTabItem("Layout")) {
            ImGui::Text("Window Layout Configuration");
            ImGui::Separator();

            ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.62f, 1.0f),
                "Adjust height proportions for each window.");
            ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.62f, 1.0f),
                "Market Data gets the remaining space (tallest).");
            ImGui::Spacing();

            // Height proportions
            float logPct = config_.layoutLogPct * 100.0f;
            if (ImGui::SliderFloat("Logs Height (%%)", &logPct, 5.0f, 30.0f, "%.0f%%"))
                config_.layoutLogPct = logPct / 100.0f;

            float vdPct = config_.layoutVdPct * 100.0f;
            if (ImGui::SliderFloat("Volume Delta Height (%%)", &vdPct, 5.0f, 30.0f, "%.0f%%"))
                config_.layoutVdPct = vdPct / 100.0f;

            float indPct = config_.layoutIndPct * 100.0f;
            if (ImGui::SliderFloat("Indicators Height (%%)", &indPct, 10.0f, 40.0f, "%.0f%%"))
                config_.layoutIndPct = indPct / 100.0f;

            // Market Data gets center column remainder (not affected by VD — VD is in left column)
            float mdCenterPct = (1.0f - config_.layoutIndPct) * (1.0f - config_.layoutLogPct);
            ImGui::TextColored(ImVec4(0.50f, 0.80f, 0.50f, 1.0f),
                "Market Data Height: ~%.0f%%", mdCenterPct * 100.0f);
            ImGui::TextColored(ImVec4(0.45f, 0.45f, 0.50f, 1.0f),
                "(Logs: %.0f%% of viewport, VD: %.0f%% of left, Ind: %.0f%% of center)",
                config_.layoutLogPct * 100.0f, config_.layoutVdPct * 100.0f,
                config_.layoutIndPct * 100.0f);

            if (mdCenterPct < 0.30f) {
                ImGui::TextColored(ImVec4(0.90f, 0.30f, 0.30f, 1.0f),
                    "Warning: Market Data window too small! Reduce other proportions.");
            }

            ImGui::Separator();
            ImGui::Checkbox("Lock Windows (prevent move/resize)", &config_.layoutLocked);

            ImGui::Spacing();
            if (ImGui::Button("Apply Layout")) {
                layoutNeedsReset_ = true;
                // Save config (imgui.ini will auto-save after positions are applied)
                saveConfigToFile(configPath_);
                if (!layoutIniPath_.empty()) saveLayoutIni(layoutIniPath_);
                addLog("[Config] Layout applied and saved");
                if (onSaveConfig_) onSaveConfig_(config_);
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset Layout to Defaults")) {
                config_.layoutLogPct = 0.10f;
                config_.layoutVdPct  = 0.15f;
                config_.layoutIndPct = 0.20f;
                config_.layoutLocked = true;
                layoutNeedsReset_ = true;
                // Reset visibility to defaults
                showPairList_    = true;
                showUserPanel_   = true;
                showVolumeDelta_ = true;
                showIndicators_  = true;
                showLogs_        = true;
                saveConfigToFile(configPath_);
                if (!layoutIniPath_.empty()) saveLayoutIni(layoutIniPath_);
                addLog("[Config] Layout reset to defaults and saved");
                if (onSaveConfig_) onSaveConfig_(config_);
            }

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.55f, 1.0f),
                "Tip: Window positions are saved in layout.ini.");
            ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.55f, 1.0f),
                "Unlock windows to drag and resize freely.");

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::Separator();
    if (ImGui::Button("Save All Settings to File")) {
        saveConfigToFile(configPath_);
        if (!layoutIniPath_.empty()) saveLayoutIni(layoutIniPath_);
        addLog("[Config] Settings saved to " + configPath_);
        if (onSaveConfig_) onSaveConfig_(config_);
    }

    ImGui::End();
}

// ---------------------------------------------------------------------------
//  Log Panel
// ---------------------------------------------------------------------------
void AppGui::drawLogPanel() {
    if (ImGui::CollapsingHeader("Log", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BeginChild("##LogChild", ImVec2(0, 160), ImGuiChildFlags_Border);
        {
            std::lock_guard<std::mutex> lk(stateMutex_);
            for (auto& line : state_.logLines) {
                // Color coding based on prefix
                if (line.find("[Error]") != std::string::npos ||
                    line.find("[SELL]")  != std::string::npos) {
                    ImGui::TextColored(ImVec4(0.90f, 0.30f, 0.30f, 1.0f),
                                       "%s", line.c_str());
                } else if (line.find("[BUY]") != std::string::npos ||
                           line.find("[OK]")  != std::string::npos) {
                    ImGui::TextColored(ImVec4(0.30f, 0.85f, 0.35f, 1.0f),
                                       "%s", line.c_str());
                } else if (line.find("[Config]") != std::string::npos ||
                           line.find("[Info]")   != std::string::npos) {
                    ImGui::TextColored(ImVec4(0.50f, 0.70f, 0.90f, 1.0f),
                                       "%s", line.c_str());
                } else if (line.find("[Warn]") != std::string::npos) {
                    ImGui::TextColored(ImVec4(0.85f, 0.70f, 0.25f, 1.0f),
                                       "%s", line.c_str());
                } else {
                    ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.67f, 1.0f),
                                       "%s", line.c_str());
                }
            }
        }
        // Auto-scroll to bottom
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 20)
            ImGui::SetScrollHereY(1.0f);
        ImGui::EndChild();
    }
}

// ---------------------------------------------------------------------------
//  Logs Window — standalone fixed window with scroll
// ---------------------------------------------------------------------------
void AppGui::drawLogWindow() {
    auto layout = layoutMgr_.get("Logs");
    if (config_.layoutLocked || layoutNeedsReset_)
        LayoutManager::lockWindow("Logs", layout.pos, layout.size);
    else
        LayoutManager::lockWindowOnce("Logs", layout.pos, layout.size);

    int wflags = config_.layoutLocked ? LayoutManager::lockedScrollFlags()
                 : (ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_HorizontalScrollbar);

    if (!ImGui::Begin("Logs", nullptr, wflags)) {
        ImGui::End();
        return;
    }

    {
        std::lock_guard<std::mutex> lk(stateMutex_);
        for (auto& line : state_.logLines) {
            // Color coding based on prefix
            if (line.find("[Error]") != std::string::npos ||
                line.find("[SELL]")  != std::string::npos) {
                ImGui::TextColored(ImVec4(0.90f, 0.30f, 0.30f, 1.0f),
                                   "%s", line.c_str());
            } else if (line.find("[BUY]") != std::string::npos ||
                       line.find("[OK]")  != std::string::npos) {
                ImGui::TextColored(ImVec4(0.30f, 0.85f, 0.35f, 1.0f),
                                   "%s", line.c_str());
            } else if (line.find("[Config]") != std::string::npos ||
                       line.find("[Info]")   != std::string::npos) {
                ImGui::TextColored(ImVec4(0.50f, 0.70f, 0.90f, 1.0f),
                                   "%s", line.c_str());
            } else if (line.find("[AI]") != std::string::npos) {
                ImGui::TextColored(ImVec4(0.70f, 0.50f, 0.90f, 1.0f),
                                   "%s", line.c_str());
            } else if (line.find("[Warn]") != std::string::npos) {
                ImGui::TextColored(ImVec4(0.85f, 0.70f, 0.25f, 1.0f),
                                   "%s", line.c_str());
            } else {
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.67f, 1.0f),
                                   "%s", line.c_str());
            }
        }
    }
    // Auto-scroll to bottom
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 20)
        ImGui::SetScrollHereY(1.0f);

    ImGui::End();
}

// ---------------------------------------------------------------------------
//  Filter Panel (volume, price, percent change filters)
// ---------------------------------------------------------------------------
void AppGui::drawFilterPanel() {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
    ImGui::BeginChild("##FilterBar", ImVec2(0, 30), ImGuiChildFlags_Border);

    ImGui::Text("Filters:");
    ImGui::SameLine();

    ImGui::SetNextItemWidth(80);
    ImGui::InputDouble("##MinVol", &config_.filterMinVolume, 0, 0, "Vol>%.0f");
    ImGui::SameLine();

    ImGui::SetNextItemWidth(80);
    ImGui::InputDouble("##MinPrice", &config_.filterMinPrice, 0, 0, "P>%.8f");
    ImGui::SameLine();

    ImGui::SetNextItemWidth(80);
    ImGui::InputDouble("##MaxPrice", &config_.filterMaxPrice, 0, 0, "P<%.8f");
    ImGui::SameLine();

    ImGui::SetNextItemWidth(80);
    float fMinChg = (float)config_.filterMinChange;
    if (ImGui::InputFloat("##MinChg", &fMinChg, 0, 0, "%%>%.1f"))
        config_.filterMinChange = fMinChg;
    ImGui::SameLine();

    ImGui::SetNextItemWidth(80);
    float fMaxChg = (float)config_.filterMaxChange;
    if (ImGui::InputFloat("##MaxChg", &fMaxChg, 0, 0, "%%<%.1f"))
        config_.filterMaxChange = fMaxChg;

    ImGui::EndChild();
    ImGui::PopStyleVar();
}

// ---------------------------------------------------------------------------
//  Volume Delta Bars — standalone resizable window
// ---------------------------------------------------------------------------
void AppGui::drawVolumeDeltaPanel() {
    auto layout = layoutMgr_.get("Volume Delta");
    if (config_.layoutLocked || layoutNeedsReset_)
        LayoutManager::lockWindow("Volume Delta", layout.pos, layout.size);
    else
        LayoutManager::lockWindowOnce("Volume Delta", layout.pos, layout.size);

    int wflags = config_.layoutLocked ? LayoutManager::lockedFlags()
                 : (ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);
    wflags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    if (!ImGui::Begin("Volume Delta", nullptr, wflags)) {
        ImGui::End();
        return;
    }

    ImVec2 canvasPos  = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    if (canvasSize.x < 10 || canvasSize.y < 10) {
        ImGui::End();
        return;
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Background
    dl->AddRectFilled(canvasPos,
        ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
        IM_COL32(20, 20, 25, 255));

    // Zoom / Pan interaction
    if (ImGui::IsWindowHovered()) {
        ImGuiIO& io = ImGui::GetIO();
        float wheel = io.MouseWheel;
        if (wheel != 0.0f) {
            if (io.KeyShift) {
                // Shift+Wheel: zoom Y
                vdZoomY_ *= (wheel > 0) ? 1.1f : 0.9f;
                vdZoomY_ = std::clamp(vdZoomY_, 0.2f, 5.0f);
            } else {
                // Wheel: zoom X
                vdZoomX_ *= (wheel > 0) ? 1.1f : 0.9f;
                vdZoomX_ = std::clamp(vdZoomX_, 0.2f, 5.0f);
            }
        }
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            ImVec2 delta = io.MouseDelta;
            vdPanX_ += delta.x;
            vdPanY_ += delta.y;
        }
    }

    std::lock_guard<std::mutex> lk(stateMutex_);
    int count = (int)volumeDeltaHistory_.size();
    if (count < 1) { ImGui::End(); return; }

    // Use log scale to normalize large volume differences
    double maxAbs = 1.0;
    std::vector<double> normalizedDelta(count);
    for (int i = 0; i < count; ++i) {
        double v = volumeDeltaHistory_[i];
        double sign = (v >= 0) ? 1.0 : -1.0;
        normalizedDelta[i] = sign * std::log1p(std::abs(v));
        if (std::abs(normalizedDelta[i]) > maxAbs) maxAbs = std::abs(normalizedDelta[i]);
    }

    float w = canvasSize.x;
    float h = canvasSize.y;
    float gap   = 1.0f;
    float barW  = std::max(2.0f, (w / vdZoomX_ - gap * (float)(count - 1)) / (float)count) * vdZoomX_;
    float step  = barW + gap;
    // If bars still overflow, clamp to fit
    if (step * (float)count > w * vdZoomX_) {
        step = (w * vdZoomX_) / (float)count;
        barW = std::max(1.0f, step - gap);
    }
    float midY = canvasPos.y + h * 0.5f + vdPanY_;

    // Clip rendering to canvas
    dl->PushClipRect(canvasPos,
        ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), true);

    // Zero baseline (follows vertical pan)
    dl->AddLine(ImVec2(canvasPos.x, midY),
                ImVec2(canvasPos.x + w, midY),
                IM_COL32(80, 80, 85, 150), 1.0f);

    for (int i = 0; i < count; ++i) {
        float x = canvasPos.x + (float)i * step + vdPanX_;
        double val = normalizedDelta[i];
        float barH = (float)(std::abs(val) / maxAbs) * (h * 0.48f) * vdZoomY_;

        ImU32 col = (val >= 0)
            ? IM_COL32(40, 180, 80, 200)   // green — buy pressure
            : IM_COL32(200, 60, 60, 200);  // red   — sell pressure

        if (val >= 0) {
            dl->AddRectFilled(ImVec2(x, midY - barH),
                              ImVec2(x + barW, midY), col);
        } else {
            dl->AddRectFilled(ImVec2(x, midY),
                              ImVec2(x + barW, midY + barH), col);
        }
    }

    dl->PopClipRect();

    // Zoom info
    {
        char info[64];
        snprintf(info, sizeof(info), "Zoom X:%.0f%% Y:%.0f%%",
                 vdZoomX_ * 100.0f, vdZoomY_ * 100.0f);
        dl->AddText(ImVec2(canvasPos.x + 4, canvasPos.y + h - 14),
                    IM_COL32(100, 100, 105, 180), info);
    }

    ImGui::Dummy(canvasSize);
    ImGui::End();
}

// ---------------------------------------------------------------------------
//  Order Book Panel (Стакан / Биржевой стакан)
// ---------------------------------------------------------------------------
void AppGui::drawOrderBookPanel() {
    ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Order Book", &showOrderBook_)) {
        ImGui::End();
        return;
    }

    GuiState snap;
    {
        std::lock_guard<std::mutex> lk(stateMutex_);
        snap = state_;
    }

    {
        ImGui::BeginChild("##OrderBook", ImVec2(0, 0), ImGuiChildFlags_Border);

        // Find max quantity for bar scaling
        double maxQty = 1.0;
        for (auto& a : snap.orderBook.asks) {
            if (a.qty > maxQty) maxQty = a.qty;
        }
        for (auto& b : snap.orderBook.bids) {
            if (b.qty > maxQty) maxQty = b.qty;
        }

        float colW = ImGui::GetContentRegionAvail().x;

        // Asks (sell orders) — top, red, reversed (lowest ask at bottom)
        ImGui::TextColored(ImVec4(0.90f, 0.30f, 0.30f, 1.0f), "ASKS (Sell)");
        ImGui::Separator();

        int askCount = std::min((int)snap.orderBook.asks.size(), 15);
        for (int i = askCount - 1; i >= 0; --i) {
            auto& level = snap.orderBook.asks[i];
            float frac = (float)(level.qty / maxQty);

            // Background bar
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddRectFilled(
                cursorPos,
                ImVec2(cursorPos.x + colW * frac, cursorPos.y + ImGui::GetTextLineHeight()),
                IM_COL32(180, 40, 40, 60));

            ImGui::TextColored(ImVec4(0.90f, 0.35f, 0.35f, 1.0f),
                "%.8f", level.price);
            ImGui::SameLine(colW * 0.4f);
            ImGui::Text("%.6f", level.qty);
            ImGui::SameLine(colW * 0.75f);
            ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.62f, 1.0f),
                "%.8f", level.price * level.qty);
        }

        // Spread
        ImGui::Separator();
        double spread = 0.0;
        if (!snap.orderBook.asks.empty() && !snap.orderBook.bids.empty()) {
            spread = snap.orderBook.asks[0].price - snap.orderBook.bids[0].price;
        }
        ImGui::TextColored(ImVec4(0.70f, 0.70f, 0.72f, 1.0f),
            "  Spread: %.8f", spread);
        ImGui::Separator();

        // Bids (buy orders) — bottom, green
        ImGui::TextColored(ImVec4(0.30f, 0.85f, 0.35f, 1.0f), "BIDS (Buy)");
        ImGui::Separator();

        int bidCount = std::min((int)snap.orderBook.bids.size(), 15);
        for (int i = 0; i < bidCount; ++i) {
            auto& level = snap.orderBook.bids[i];
            float frac = (float)(level.qty / maxQty);

            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddRectFilled(
                cursorPos,
                ImVec2(cursorPos.x + colW * frac, cursorPos.y + ImGui::GetTextLineHeight()),
                IM_COL32(40, 160, 60, 60));

            ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.40f, 1.0f),
                "%.8f", level.price);
            ImGui::SameLine(colW * 0.4f);
            ImGui::Text("%.6f", level.qty);
            ImGui::SameLine(colW * 0.75f);
            ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.62f, 1.0f),
                "%.8f", level.price * level.qty);
        }

        if (snap.orderBook.bids.empty() && snap.orderBook.asks.empty()) {
            ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.52f, 1.0f),
                "No order book data available. Connect to an exchange to see the order book.");
        }

        ImGui::EndChild();
    }

    ImGui::End();
}

// ---------------------------------------------------------------------------
//  Status Bar
// ---------------------------------------------------------------------------
void AppGui::drawStatusBar() {
    ImGui::Separator();
    GuiState snap;
    {
        std::lock_guard<std::mutex> lk(stateMutex_);
        snap = state_;
    }

    ImVec4 statusColor = snap.connected
        ? ImVec4(0.30f, 0.80f, 0.35f, 1.0f)
        : ImVec4(0.60f, 0.60f, 0.60f, 1.0f);
    ImGui::TextColored(statusColor, "%s", snap.statusMessage.c_str());
    ImGui::SameLine(ImGui::GetWindowWidth() - 200);
    ImGui::TextColored(ImVec4(0.45f, 0.45f, 0.47f, 1.0f),
                       "Crypto ML Trader v2.6.0");
}

// ---------------------------------------------------------------------------
//  Pair List Panel — left sidebar with SPOT/FUTURES toggle, search, pair list
// ---------------------------------------------------------------------------
void AppGui::drawPairListPanel() {
    auto layout = layoutMgr_.get("Pair List");
    if (config_.layoutLocked || layoutNeedsReset_)
        LayoutManager::lockWindow("Pair List", layout.pos, layout.size);
    else
        LayoutManager::lockWindowOnce("Pair List", layout.pos, layout.size);

    int wflags = config_.layoutLocked ? LayoutManager::lockedFlags()
                 : (ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

    if (!ImGui::Begin("Pair List", nullptr, wflags)) {
        ImGui::End();
        return;
    }

    // ── SPOT / FUTURES toggle ──
    {
        bool isSpot = (config_.marketType == "spot");
        float btnW = 90.0f;

        if (isSpot) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.55f, 0.75f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.42f, 0.62f, 0.82f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.22f, 0.24f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.30f, 0.32f, 1.0f));
        }
        if (ImGui::Button("SPOT", ImVec2(btnW, 0))) {
            if (config_.marketType != "spot") {
                config_.marketType = "spot";
                selectedPairIdx_ = -1;
                cachedPairs_.clear();
                if (onLoadPairs_) onLoadPairs_("spot");
            }
        }
        ImGui::PopStyleColor(2);

        ImGui::SameLine();

        if (!isSpot) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.35f, 0.15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.65f, 0.42f, 0.20f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.22f, 0.24f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.30f, 0.32f, 1.0f));
        }
        if (ImGui::Button("FUTURES", ImVec2(btnW, 0))) {
            if (config_.marketType != "futures") {
                config_.marketType = "futures";
                selectedPairIdx_ = -1;
                cachedPairs_.clear();
                if (onLoadPairs_) onLoadPairs_("futures");
            }
        }
        ImGui::PopStyleColor(2);
    }

    ImGui::Separator();

    // ── Search filter ──
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##PairSearch", pairSearchBuf_, sizeof(pairSearchBuf_));
    ImGui::Separator();

    // ── Get pair list from state ──
    std::vector<SymbolInfo> syms;
    {
        std::lock_guard<std::mutex> lk(stateMutex_);
        syms = state_.availableSymbols;
    }

    // Update cache
    if (!syms.empty()) {
        cachedPairs_ = syms;
    }

    // Filter by market type and search text
    std::string searchFilter(pairSearchBuf_);
    for (auto& ch : searchFilter) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));

    std::vector<const SymbolInfo*> filtered;
    for (auto& s : cachedPairs_) {
        if (!config_.marketType.empty() && s.marketType != config_.marketType)
            continue;
        if (!searchFilter.empty()) {
            std::string name = s.symbol;
            for (auto& ch : name) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            if (name.find(searchFilter) == std::string::npos) continue;
        }
        filtered.push_back(&s);
    }

    // ── Loading spinner ──
    if (pairListLoading_) {
        float time = (float)ImGui::GetTime();
        const char* spinner = "|/-\\";
        char spinChar = spinner[(int)(time * 8.0f) % 4];
        ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.62f, 1.0f), "Loading %c", spinChar);
        return;
    }

    // ── Error display ──
    if (!pairListError_.empty()) {
        ImGui::TextColored(ImVec4(0.90f, 0.30f, 0.30f, 1.0f), "%s", pairListError_.c_str());
    }

    if (filtered.empty() && cachedPairs_.empty()) {
        ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.52f, 1.0f), "Connect to load pairs");
        return;
    }

    if (filtered.empty()) {
        ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.52f, 1.0f), "No pairs found");
        return;
    }

    // Initialize active pair
    if (activePair_.empty()) {
        activePair_ = config_.symbol;
    }

    // ── Pair list ──
    ImGui::BeginChild("##PairScroll", ImVec2(0, 0), ImGuiChildFlags_None);
    for (int i = 0; i < (int)filtered.size(); ++i) {
        bool isActive = (filtered[i]->symbol == activePair_);

        if (isActive) {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.45f, 0.65f, 0.80f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.30f, 0.50f, 0.70f, 0.90f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        }

        if (ImGui::Selectable(filtered[i]->displayName.c_str(), isActive)) {
            activePair_ = filtered[i]->symbol;
            config_.symbol = filtered[i]->symbol;
            config_.marketType = filtered[i]->marketType;
            selectedPairIdx_ = i;
            chartScrollOffset_ = 0;
            needsChartReset_ = true;
            addLog("[Config] Pair changed to " + filtered[i]->displayName);
            if (onRefreshData_) onRefreshData_();
        }

        if (isActive) {
            ImGui::PopStyleColor(3);
        }
    }
    ImGui::EndChild();
}

// ---------------------------------------------------------------------------
//  Pair Selector — dropdown for spot & futures pairs
// ---------------------------------------------------------------------------
void AppGui::drawPairSelector() {
    std::vector<SymbolInfo> syms;
    {
        std::lock_guard<std::mutex> lk(stateMutex_);
        syms = state_.availableSymbols;
    }

    if (syms.empty()) {
        // Show static symbol input when no pairs are loaded
        ImGui::SetNextItemWidth(130);
        static char pairBuf[32] = {};
        static bool pairInit = false;
        if (!pairInit) {
            strncpy(pairBuf, config_.symbol.c_str(), sizeof(pairBuf) - 1);
            pairInit = true;
        }
        if (ImGui::InputText("##PairInput", pairBuf, sizeof(pairBuf),
                             ImGuiInputTextFlags_EnterReturnsTrue)) {
            config_.symbol = pairBuf;
        }
        return;
    }

    // Filter by current market type
    std::vector<const SymbolInfo*> filtered;
    for (auto& s : syms) {
        if (config_.marketType.empty() || s.marketType == config_.marketType)
            filtered.push_back(&s);
    }

    // Find current selection
    if (selectedPairIdx_ < 0) {
        for (int i = 0; i < (int)filtered.size(); ++i) {
            if (filtered[i]->symbol == config_.symbol) {
                selectedPairIdx_ = i;
                break;
            }
        }
    }

    const char* preview = (selectedPairIdx_ >= 0 && selectedPairIdx_ < (int)filtered.size())
        ? filtered[selectedPairIdx_]->displayName.c_str()
        : "Select PAIR";

    ImGui::SetNextItemWidth(180);
    if (ImGui::BeginCombo("##PairSelect", preview)) {
        // Search filter
        static char searchBuf[32] = {};
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##PairSearch", searchBuf, sizeof(searchBuf));
        ImGui::Separator();

        std::string filter(searchBuf);
        for (auto& ch : filter) ch = static_cast<char>(std::toupper(ch));

        for (int i = 0; i < (int)filtered.size(); ++i) {
            // Apply text filter
            if (!filter.empty()) {
                std::string name = filtered[i]->displayName;
                for (auto& ch : name) ch = static_cast<char>(std::toupper(ch));
                if (name.find(filter) == std::string::npos) continue;
            }

            bool selected = (i == selectedPairIdx_);
            // Color-code: spot=cyan, futures=orange
            ImVec4 col = (filtered[i]->marketType == "futures")
                ? ImVec4(0.90f, 0.65f, 0.20f, 1.0f)
                : ImVec4(0.40f, 0.80f, 0.90f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, col);
            if (ImGui::Selectable(filtered[i]->displayName.c_str(), selected)) {
                selectedPairIdx_ = i;
                config_.symbol = filtered[i]->symbol;
                config_.marketType = filtered[i]->marketType;
                needsChartReset_ = true;
                addLog("[Config] Pair changed to " + filtered[i]->displayName);
            }
            ImGui::PopStyleColor();
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::End();
}

// ---------------------------------------------------------------------------
//  User Panel — embedded right column with balance, orders, trades, price, P&L
// ---------------------------------------------------------------------------
void AppGui::drawUserPanel() {
    auto layout = layoutMgr_.get("User Panel");
    if (config_.layoutLocked || layoutNeedsReset_)
        LayoutManager::lockWindow("User Panel", layout.pos, layout.size);
    else
        LayoutManager::lockWindowOnce("User Panel", layout.pos, layout.size);

    int wflags = config_.layoutLocked ? LayoutManager::lockedFlags()
                 : (ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

    if (!ImGui::Begin("User Panel", &showUserPanel_, wflags)) {
        ImGui::End();
        return;
    }

    GuiState snap;
    {
        std::lock_guard<std::mutex> lk(stateMutex_);
        snap = state_;
    }

    // ── Current Price ──
    ImGui::TextColored(ImVec4(0.60f, 0.70f, 0.85f, 1.0f), "%s", config_.symbol.c_str());
    if (snap.currentPrice > 0) {
        ImGui::SameLine();
        char priceBuf[32];
        OrderManagement::formatPrice(priceBuf, sizeof(priceBuf), snap.currentPrice);
        ImGui::TextColored(ImVec4(0.30f, 0.85f, 0.35f, 1.0f), "$%s", priceBuf);
    }
    if (!snap.currentPriceError.empty()) {
        ImGui::TextColored(ImVec4(0.90f, 0.30f, 0.30f, 1.0f), "%s", snap.currentPriceError.c_str());
    }
    ImGui::Separator();

    // ── Connection Status ──
    if (snap.connected) {
        ImGui::TextColored(ImVec4(0.30f, 0.85f, 0.35f, 1.0f), "Connected");
    } else {
        ImGui::TextColored(ImVec4(0.85f, 0.35f, 0.35f, 1.0f), "Disconnected");
    }
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.52f, 1.0f), "%s", config_.exchangeName.c_str());
    if (snap.usingAltEndpoint) {
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "!! Alt endpoint");
    }
    ImGui::Separator();

    // ── 3.1 Account Balance ──
    if (ImGui::CollapsingHeader("Balance", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (config_.marketType == "futures") {
            // Futures balance
            ImGui::Text("Wallet:     $%.8f", snap.futuresBalance.totalWalletBalance);
            ImVec4 uplCol = snap.futuresBalance.totalUnrealizedProfit >= 0
                ? ImVec4(0.30f, 0.85f, 0.35f, 1.0f)
                : ImVec4(0.90f, 0.30f, 0.30f, 1.0f);
            ImGui::TextColored(uplCol, "Unreal P&L: $%.8f", snap.futuresBalance.totalUnrealizedProfit);
            ImGui::Text("Margin:     $%.8f", snap.futuresBalance.totalMarginBalance);
        } else {
            // Spot balance / OKX account details
            if (!snap.accountBalanceDetails.empty()) {
                if (ImGui::BeginTable("##BalTbl", 3,
                    ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV,
                    ImVec2(0, 100))) {
                    ImGui::TableSetupColumn("Coin");
                    ImGui::TableSetupColumn("Avail");
                    ImGui::TableSetupColumn("USD");
                    ImGui::TableHeadersRow();
                    for (auto& d : snap.accountBalanceDetails) {
                        if (d.availBal <= 0 && d.frozenBal <= 0) continue;
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%s", d.currency.c_str());
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%.8f", d.availBal);
                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("$%.8f", d.usdValue);
                    }
                    ImGui::EndTable();
                }
            } else {
                // Simple spot balance
                double pnl = snap.equity - snap.initialCapital;
                ImVec4 eqCol = pnl >= 0
                    ? ImVec4(0.30f, 0.85f, 0.35f, 1.0f)
                    : ImVec4(0.90f, 0.30f, 0.30f, 1.0f);
                ImGui::TextColored(eqCol, "Equity: $%.8f", snap.equity);
                ImGui::Text("Drawdown: %.2f%%", snap.drawdown * 100.0);
            }
        }
    }

    // ── 3.2 Open Orders ──
    if (ImGui::CollapsingHeader("Open Orders", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (snap.openOrders.empty()) {
            ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.52f, 1.0f), "No open orders");
        } else {
            if (ImGui::BeginTable("##OrdersTbl", 4,
                ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV |
                ImGuiTableFlags_ScrollY, ImVec2(0, 120))) {
                ImGui::TableSetupColumn("Pair");
                ImGui::TableSetupColumn("Side");
                ImGui::TableSetupColumn("Price");
                ImGui::TableSetupColumn("Qty");
                ImGui::TableHeadersRow();
                for (auto& o : snap.openOrders) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", o.symbol.c_str());
                    ImGui::TableSetColumnIndex(1);
                    ImVec4 sCol = (o.side == "BUY")
                        ? ImVec4(0.30f, 0.85f, 0.35f, 1.0f)
                        : ImVec4(0.90f, 0.30f, 0.30f, 1.0f);
                    ImGui::TextColored(sCol, "%s", o.side.c_str());
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%.8f", o.price);
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%.8f", o.qty);
                }
                ImGui::EndTable();
            }
        }
    }

    // ── 3.4 Recent Trades (My Trades) ──
    if (ImGui::CollapsingHeader("My Trades", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (snap.userTrades.empty()) {
            ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.52f, 1.0f), "No trades");
        } else {
            if (ImGui::BeginTable("##MyTradesTbl", 4,
                ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV |
                ImGuiTableFlags_ScrollY, ImVec2(0, 140))) {
                ImGui::TableSetupColumn("Side");
                ImGui::TableSetupColumn("Price");
                ImGui::TableSetupColumn("Qty");
                ImGui::TableSetupColumn("Fee");
                ImGui::TableHeadersRow();
                for (auto& t : snap.userTrades) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImVec4 sCol = (t.side == "BUY")
                        ? ImVec4(0.30f, 0.85f, 0.35f, 1.0f)
                        : ImVec4(0.90f, 0.30f, 0.30f, 1.0f);
                    ImGui::TextColored(sCol, "%s", t.side.c_str());
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.8f", t.price);
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%.8f", t.qty);
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%.6f", t.commission);
                }
                ImGui::EndTable();
            }
        }
    }

    // ── 3.5 P&L (Futures) ──
    if (config_.marketType == "futures") {
        if (ImGui::CollapsingHeader("Positions / P&L", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (snap.futuresPositions.empty()) {
                ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.52f, 1.0f), "No positions");
            } else {
                for (auto& p : snap.futuresPositions) {
                    if (p.positionAmt == 0) continue;
                    ImGui::TextColored(ImVec4(0.60f, 0.70f, 0.85f, 1.0f), "%s", p.symbol.c_str());
                    ImGui::Text("  Amt: %.8f  Entry: %.8f", p.positionAmt, p.entryPrice);
                    ImVec4 uplCol = p.unrealizedProfit >= 0
                        ? ImVec4(0.30f, 0.85f, 0.35f, 1.0f)
                        : ImVec4(0.90f, 0.30f, 0.30f, 1.0f);
                    ImGui::TextColored(uplCol, "  UPL: $%.8f", p.unrealizedProfit);
                    ImGui::Text("  Leverage: %.0fx  %s", p.leverage, p.marginType.c_str());
                    ImGui::Separator();
                }
            }
        }
    }

    // ── Signal ──
    if (ImGui::CollapsingHeader("Signal")) {
        const char* sigStr = "HOLD";
        ImVec4 sigCol(0.60f, 0.60f, 0.60f, 1.0f);
        if (snap.lastSignal.type == Signal::Type::BUY) {
            sigStr = "BUY"; sigCol = ImVec4(0.20f, 0.85f, 0.30f, 1.0f);
        } else if (snap.lastSignal.type == Signal::Type::SELL) {
            sigStr = "SELL"; sigCol = ImVec4(0.90f, 0.25f, 0.25f, 1.0f);
        }
        ImGui::TextColored(sigCol, "%s (%.0f%%)", sigStr, snap.lastSignal.confidence * 100.0);
        if (!snap.lastSignal.reason.empty())
            ImGui::TextWrapped("%s", snap.lastSignal.reason.c_str());
    }

    ImGui::End();
}

// ---------------------------------------------------------------------------
//  H1 — Order Management Window
// ---------------------------------------------------------------------------
void AppGui::drawOrderManagementWindow() {
    ImGui::SetNextWindowSize(ImVec2(420, 520), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Order Management", &showOrderManagement_)) {
        ImGui::End();
        return;
    }

    GuiState snap;
    {
        std::lock_guard<std::mutex> lk(stateMutex_);
        snap = state_;
    }

    if (!snap.connected) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Connect to exchange first");
        ImGui::End();
        return;
    }

    // Side buttons
    float btnW = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
    ImGui::PushStyleColor(ImGuiCol_Button, (omSideIdx_ == 0)
        ? ImVec4(0.13f, 0.55f, 0.25f, 1.0f) : ImVec4(0.22f, 0.22f, 0.24f, 1.0f));
    if (ImGui::Button("BUY", ImVec2(btnW, 30))) omSideIdx_ = 0;
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, (omSideIdx_ == 1)
        ? ImVec4(0.60f, 0.15f, 0.15f, 1.0f) : ImVec4(0.22f, 0.22f, 0.24f, 1.0f));
    if (ImGui::Button("SELL", ImVec2(btnW, 30))) omSideIdx_ = 1;
    ImGui::PopStyleColor();

    // Type combo
    const char* orderTypes[] = {"MARKET", "LIMIT", "STOP_LIMIT"};
    ImGui::SetNextItemWidth(-1);
    ImGui::Combo("Type##OM", &omTypeIdx_, orderTypes, 3);

    // Quantity
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("Quantity##OM", &omQuantity_, 0.001, 0.01, "%.6f");
    if (omQuantity_ < 0) omQuantity_ = 0;

    // Price (LIMIT / STOP_LIMIT)
    if (omTypeIdx_ >= 1) {
        ImGui::SetNextItemWidth(-1);
        ImGui::InputDouble("Price##OM", &omPrice_, 1.0, 10.0, "%.8f");
    }

    // Stop Price (STOP_LIMIT only)
    if (omTypeIdx_ == 2) {
        ImGui::SetNextItemWidth(-1);
        ImGui::InputDouble("Stop Price##OM", &omStopPrice_, 1.0, 10.0, "%.8f");
    }

    // Reduce Only (futures)
    if (config_.marketType == "futures") {
        ImGui::Checkbox("Reduce Only", &omReduceOnly_);

        // Leverage input for futures trading
        ImGui::SetNextItemWidth(-1);
        ImGui::SliderInt("Leverage##OM", &omLeverage_, 1, 125, "%dx");
        if (omLeverage_ < 1) omLeverage_ = 1;
        if (omLeverage_ > 125) omLeverage_ = 125;

        // Apply leverage to exchange
        if (ImGui::Button("Apply Leverage##OM", ImVec2(-1, 0))) {
            if (onSetLeverage_) {
                onSetLeverage_(config_.symbol, omLeverage_);
                addLog("[Leverage] Set " + std::to_string(omLeverage_) + "x for " + config_.symbol);
            }
        }
    }

    ImGui::Separator();

    // Estimated cost
    double estPrice = (omTypeIdx_ == 0) ? snap.currentPrice : omPrice_;
    double estCost = omQuantity_ * estPrice;
    ImGui::Text("Est. Cost:  $%.2f", estCost);
    ImGui::Text("Balance:    $%.2f", snap.futuresBalance.totalWalletBalance);

    ImGui::Separator();

    // Validation error
    if (!omValidationError_.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", omValidationError_.c_str());
    }

    // Place Order button
    ImGui::PushStyleColor(ImGuiCol_Button, (omSideIdx_ == 0)
        ? ImVec4(0.13f, 0.55f, 0.25f, 1.0f) : ImVec4(0.60f, 0.15f, 0.15f, 1.0f));
    if (ImGui::Button("PLACE ORDER", ImVec2(-1, 36))) {
        UIOrderRequest req;
        req.symbol = config_.symbol;
        req.side = (omSideIdx_ == 0) ? "BUY" : "SELL";
        req.type = orderTypes[omTypeIdx_];
        req.quantity = omQuantity_;
        req.price = omPrice_;
        req.stopPrice = omStopPrice_;
        req.reduceOnly = omReduceOnly_;

        // Validate
        RiskManager::Config rcfg;
        rcfg.maxRiskPerTrade = config_.maxRiskPerTrade;
        rcfg.maxDrawdown = config_.maxDrawdown;
        RiskManager risk(rcfg, config_.initialCapital);
        auto v = OrderManagement::validate(req, risk, snap.futuresBalance.totalWalletBalance);
        if (v.valid) {
            omValidationError_.clear();
            omShowConfirm_ = true;
            ImGui::OpenPopup("Confirm Order");
        } else {
            omValidationError_ = v.reason;
        }
    }
    ImGui::PopStyleColor();

    // Confirmation popup
    if (ImGui::BeginPopupModal("Confirm Order", &omShowConfirm_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Side: %s", (omSideIdx_ == 0) ? "BUY" : "SELL");
        ImGui::Text("Type: %s", orderTypes[omTypeIdx_]);
        ImGui::Text("Quantity: %.6f", omQuantity_);
        if (omTypeIdx_ >= 1) {
            char pb[32]; OrderManagement::formatPrice(pb, sizeof(pb), omPrice_);
            ImGui::Text("Price: %s", pb);
        }
        if (omTypeIdx_ == 2) {
            char sp[32]; OrderManagement::formatPrice(sp, sizeof(sp), omStopPrice_);
            ImGui::Text("Stop Price: %s", sp);
        }
        ImGui::Text("Est. Cost: $%.2f", estCost);
        ImGui::Separator();

        if (ImGui::Button("Confirm", ImVec2(120, 0))) {
            std::string side = (omSideIdx_ == 0) ? "BUY" : "SELL";
            std::string type = orderTypes[omTypeIdx_];
            double price = (omTypeIdx_ == 0) ? 0.0 : omPrice_;
            if (onOrder_) {
                onOrder_(config_.symbol, side, type, omQuantity_, price);
                addLog("[Order] " + side + " " + type + " " + std::to_string(omQuantity_) + " " + config_.symbol);
            }
            ImGui::CloseCurrentPopup();
            omShowConfirm_ = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
            omShowConfirm_ = false;
        }
        ImGui::EndPopup();
    }

    // ── Open Positions Table ──
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.6f, 0.7f, 0.85f, 1.0f), "Open Positions");

    if (ImGui::BeginTable("##positions", 8, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("Pair");
        ImGui::TableSetupColumn("Side");
        ImGui::TableSetupColumn("Qty");
        ImGui::TableSetupColumn("Entry");
        ImGui::TableSetupColumn("PnL");
        ImGui::TableSetupColumn("TP");
        ImGui::TableSetupColumn("SL");
        ImGui::TableSetupColumn("Action");
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < snap.futuresPositions.size(); ++i) {
            auto& p = snap.futuresPositions[i];
            if (p.positionAmt == 0) continue;
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%s", p.symbol.c_str());
            ImGui::TableNextColumn();
            ImVec4 sideCol = (p.positionAmt > 0)
                ? ImVec4(0.3f, 0.85f, 0.35f, 1.0f) : ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
            ImGui::TextColored(sideCol, "%s", p.positionAmt > 0 ? "LONG" : "SHORT");
            ImGui::TableNextColumn(); ImGui::Text("%.8f", std::abs(p.positionAmt));
            ImGui::TableNextColumn(); ImGui::Text("%.8f", p.entryPrice);
            ImGui::TableNextColumn();
            ImVec4 pnlCol = (p.unrealizedProfit >= 0)
                ? ImVec4(0.3f, 0.85f, 0.35f, 1.0f) : ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
            ImGui::TextColored(pnlCol, "%.8f", p.unrealizedProfit);
            ImGui::TableNextColumn(); ImGui::Text("-");
            ImGui::TableNextColumn(); ImGui::Text("-");
            ImGui::TableNextColumn();
            ImGui::PushID(static_cast<int>(i));
            if (ImGui::SmallButton("Close")) {
                std::string side = (p.positionAmt > 0) ? "SELL" : "BUY";
                if (onOrder_) {
                    onOrder_(p.symbol, side, "MARKET", std::abs(p.positionAmt), 0.0);
                    addLog("[Close] " + side + " " + std::to_string(std::abs(p.positionAmt)) + " " + p.symbol);
                }
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    ImGui::End();
}

// ---------------------------------------------------------------------------
//  H2 — Paper Trading Window
// ---------------------------------------------------------------------------
void AppGui::drawPaperTradingWindow() {
    ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Paper Trading", &showPaperTrading_)) {
        ImGui::End();
        return;
    }

    auto acc = paperTrader_.getAccount();

    ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.3f, 1.0f), "Virtual Balance: $%.8f", acc.balance);
    ImGui::Text("Equity:          $%.8f", acc.equity);

    double pnlPct = (acc.equity > 0 && acc.balance > 0) ? ((acc.totalPnL / 10000.0) * 100.0) : 0.0;
    ImVec4 pnlCol = acc.totalPnL >= 0 ? ImVec4(0.3f, 0.85f, 0.35f, 1.0f) : ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
    ImGui::TextColored(pnlCol, "Total PnL:  %+.2f (%+.1f%%)", acc.totalPnL, pnlPct);
    ImGui::Text("Max Drawdown:    -%.2f%%", acc.maxDrawdown * 100.0);

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.6f, 0.7f, 0.85f, 1.0f), "Open Positions");

    if (acc.openPositions.empty()) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No open positions");
    } else {
        if (ImGui::BeginTable("##paper_pos", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
            ImGui::TableSetupColumn("Pair");
            ImGui::TableSetupColumn("Qty");
            ImGui::TableSetupColumn("Entry");
            ImGui::TableSetupColumn("Current");
            ImGui::TableSetupColumn("PnL");
            ImGui::TableHeadersRow();
            for (auto& p : acc.openPositions) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn(); ImGui::Text("%s", p.symbol.c_str());
                ImGui::TableNextColumn(); ImGui::Text("%.8f", p.quantity);
                ImGui::TableNextColumn(); ImGui::Text("%.8f", p.entryPrice);
                ImGui::TableNextColumn(); ImGui::Text("%.8f", p.currentPrice);
                ImGui::TableNextColumn();
                ImVec4 c = p.pnl >= 0 ? ImVec4(0.3f, 0.85f, 0.35f, 1.0f) : ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
                ImGui::TextColored(c, "%.8f", p.pnl);
            }
            ImGui::EndTable();
        }
    }

    ImGui::Separator();
    if (ImGui::Button("Reset Paper Account")) {
        paperResetConfirm_ = true;
        ImGui::OpenPopup("Reset Paper?");
    }

    if (ImGui::BeginPopupModal("Reset Paper?", &paperResetConfirm_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Reset paper account to initial balance?");
        if (ImGui::Button("Confirm", ImVec2(120, 0))) {
            paperTrader_.reset(config_.initialCapital);
            addLog("[Paper] Account reset to $" + std::to_string(config_.initialCapital));
            ImGui::CloseCurrentPopup();
            paperResetConfirm_ = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
            paperResetConfirm_ = false;
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

// ---------------------------------------------------------------------------
//  M1 — Backtesting Window
// ---------------------------------------------------------------------------
void AppGui::drawBacktestWindow() {
    ImGui::SetNextWindowSize(ImVec2(700, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Backtesting", &showBacktest_)) {
        ImGui::End();
        return;
    }

    const char* btIntervals[] = {"1m","5m","15m","30m","1h","4h","1d"};
    int btIntervalsCount = 7;

    ImGui::SetNextItemWidth(120);
    // Pair selection: combo from cachedPairs_ or manual text input
    if (!cachedPairs_.empty()) {
        if (ImGui::BeginCombo("Symbol##BT", btSymbol_)) {
            for (auto& p : cachedPairs_) {
                bool selected = (p.symbol == btSymbol_);
                if (ImGui::Selectable(p.symbol.c_str(), selected)) {
                    std::strncpy(btSymbol_, p.symbol.c_str(), sizeof(btSymbol_) - 1);
                    btSymbol_[sizeof(btSymbol_) - 1] = '\0';
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    } else {
        ImGui::InputText("Symbol##BT", btSymbol_, sizeof(btSymbol_));
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    ImGui::Combo("TF##BT", &btIntervalIdx_, btIntervals, btIntervalsCount);

    ImGui::SetNextItemWidth(120);
    ImGui::InputText("Start##BT", btStartDate_, sizeof(btStartDate_));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    ImGui::InputText("End##BT", btEndDate_, sizeof(btEndDate_));

    ImGui::SetNextItemWidth(120);
    ImGui::InputDouble("Balance##BT", &btBalance_, 100, 1000, "%.0f");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    ImGui::InputDouble("Comm%##BT", &btCommission_, 0.01, 0.1, "%.2f");

    ImGui::Separator();

    if (ImGui::Button("Run Backtest", ImVec2(140, 30)) && !btRunning_) {
        btRunning_ = true;
        GuiState snap;
        {
            std::lock_guard<std::mutex> lk(stateMutex_);
            snap = state_;
        }
        // Run backtest on current candle history
        BacktestEngine bt;
        if (btRepo_) bt.setRepository(btRepo_);
        BacktestEngine::Config cfg;
        cfg.symbol = btSymbol_;
        cfg.interval = btIntervals[btIntervalIdx_];
        cfg.initialBalance = btBalance_;
        cfg.commission = btCommission_ / 100.0;
        cfg.startDate = btStartDate_;
        cfg.endDate = btEndDate_;

        std::vector<Candle> bars(snap.candleHistory.begin(), snap.candleHistory.end());
        btResult_ = bt.run(cfg, bars);
        btRunning_ = false;
        addLog("[Backtest] Completed: " + std::to_string(btResult_.totalTrades) + " trades, PnL: $" +
               std::to_string(btResult_.totalPnL));
    }
    ImGui::SameLine();
    if (ImGui::Button("Export CSV##BT", ImVec2(120, 30))) {
        std::string fname = std::string(btSymbol_) + "_backtest_trades.csv";
        if (CSVExporter::exportBacktestTrades(btResult_.trades, fname))
            addLog("[Export] Saved " + fname);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset##BT", ImVec2(80, 30))) {
        btResult_ = BacktestEngine::Result{};
        btRunning_ = false;
        addLog("[Backtest] Reset");
    }

    ImGui::Separator();

    // Results
    if (btResult_.totalTrades > 0) {
        ImVec4 pnlCol = btResult_.totalPnL >= 0
            ? ImVec4(0.3f, 0.85f, 0.35f, 1.0f) : ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
        ImGui::TextColored(pnlCol, "PnL: $%.8f (%.1f%%)", btResult_.totalPnL, btResult_.totalPnLPct);
        ImGui::Text("Trades: %d  Win: %d (%.1f%%)", btResult_.totalTrades,
                     btResult_.winTrades, btResult_.winRate * 100.0);
        ImGui::Text("Max DD: -$%.8f (-%.1f%%)", btResult_.maxDrawdown, btResult_.maxDrawdownPct);
        ImGui::Text("Sharpe: %.2f  PF: %.2f", btResult_.sharpeRatio, btResult_.profitFactor);
        ImGui::Text("Avg Win: $%.8f  Avg Loss: $%.8f", btResult_.avgWin, btResult_.avgLoss);

        // Equity curve mini chart
        if (!btResult_.equityCurve.empty()) {
            ImGui::Separator();
            std::vector<float> eq;
            eq.reserve(btResult_.equityCurve.size());
            for (double e : btResult_.equityCurve) eq.push_back(static_cast<float>(e));
            ImGui::PlotLines("Equity", eq.data(), static_cast<int>(eq.size()),
                             0, nullptr, FLT_MAX, FLT_MAX, ImVec2(-1, 120));
        }

        // Trades table
        ImGui::Separator();
        if (ImGui::BeginTable("##bt_trades", 6, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
                              ImGuiTableFlags_ScrollY, ImVec2(0, 200))) {
            ImGui::TableSetupColumn("#");
            ImGui::TableSetupColumn("Side");
            ImGui::TableSetupColumn("Entry");
            ImGui::TableSetupColumn("Exit");
            ImGui::TableSetupColumn("PnL");
            ImGui::TableSetupColumn("%");
            ImGui::TableHeadersRow();
            for (size_t i = 0; i < btResult_.trades.size(); ++i) {
                auto& t = btResult_.trades[i];
                ImGui::TableNextRow();
                ImGui::TableNextColumn(); ImGui::Text("%d", static_cast<int>(i + 1));
                ImGui::TableNextColumn();
                ImVec4 sc = (t.side == "BUY") ? ImVec4(0.3f, 0.85f, 0.35f, 1.0f) : ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
                ImGui::TextColored(sc, "%s", t.side.c_str());
                ImGui::TableNextColumn(); ImGui::Text("%.8f", t.entryPrice);
                ImGui::TableNextColumn(); ImGui::Text("%.8f", t.exitPrice);
                ImGui::TableNextColumn();
                ImVec4 pc = t.pnl >= 0 ? ImVec4(0.3f, 0.85f, 0.35f, 1.0f) : ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
                ImGui::TextColored(pc, "%.8f", t.pnl);
                ImGui::TableNextColumn(); ImGui::TextColored(pc, "%.1f%%", t.pnlPct);
            }
            ImGui::EndTable();
        }
    }

    ImGui::End();
}

// ---------------------------------------------------------------------------
//  M3 — Alerts Window
// ---------------------------------------------------------------------------
void AppGui::drawAlertsWindow() {
    ImGui::SetNextWindowSize(ImVec2(450, 350), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Alerts", &showAlerts_)) {
        ImGui::End();
        return;
    }

    // New alert form
    const char* conditions[] = {"RSI_ABOVE","RSI_BELOW","PRICE_ABOVE","PRICE_BELOW","EMA_CROSS_UP","EMA_CROSS_DOWN"};
    const char* notifyTypes[] = {"sound","telegram","both"};

    ImGui::SetNextItemWidth(100);
    ImGui::InputText("Symbol##AL", alertSymbol_, sizeof(alertSymbol_));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    ImGui::Combo("Cond##AL", &alertCondIdx_, conditions, 6);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    ImGui::InputDouble("##ALThresh", &alertThreshold_, 0, 0, "%.8f");

    ImGui::SetNextItemWidth(100);
    ImGui::Combo("Notify##AL", &alertNotifyIdx_, notifyTypes, 3);
    ImGui::SameLine();
    if (ImGui::Button("+ Add Alert")) {
        Alert a;
        a.symbol = alertSymbol_;
        a.condition = conditions[alertCondIdx_];
        a.threshold = alertThreshold_;
        a.notifyType = notifyTypes[alertNotifyIdx_];
        alertManager_.addAlert(a);
        addLog("[Alert] Added: " + a.symbol + " " + a.condition + " " + std::to_string(a.threshold));
    }

    ImGui::Separator();

    // Alert list
    auto alerts = alertManager_.getAlerts();
    for (size_t i = 0; i < alerts.size(); ++i) {
        auto& a = alerts[i];
        ImGui::PushID(static_cast<int>(i));
        ImGui::TextColored(a.triggered ? ImVec4(1.0f, 0.6f, 0.0f, 1.0f) : ImVec4(0.5f, 0.8f, 0.5f, 1.0f),
                           "%s", a.triggered ? "!" : "o");
        ImGui::SameLine();
        ImGui::Text("%s | %s %.0f | %s", a.symbol.c_str(), a.condition.c_str(),
                    a.threshold, a.notifyType.c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton("Reset")) alertManager_.resetAlert(a.id);
        ImGui::SameLine();
        if (ImGui::SmallButton("Del")) alertManager_.removeAlert(a.id);
        ImGui::PopID();
    }

    ImGui::End();
}

// ---------------------------------------------------------------------------
//  L1 — Market Scanner Window
// ---------------------------------------------------------------------------
void AppGui::drawMarketScannerWindow() {
    ImGui::SetNextWindowSize(ImVec2(800, 500), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Market Scanner", &showScanner_)) {
        ImGui::End();
        return;
    }

    auto results = scanner_.getResults();

    if (results.empty()) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
            "No scan results. Connect and data will populate automatically.");
    } else {
        if (ImGui::BeginTable("##scanner", 7,
            ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable |
            ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY)) {
            ImGui::TableSetupColumn("Pair");
            ImGui::TableSetupColumn("Price");
            ImGui::TableSetupColumn("24h%");
            ImGui::TableSetupColumn("Volume");
            ImGui::TableSetupColumn("RSI");
            ImGui::TableSetupColumn("Signal");
            ImGui::TableSetupColumn("Conf%");
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < results.size(); ++i) {
                auto& r = results[i];
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (ImGui::Selectable(r.symbol.c_str(), false, ImGuiSelectableFlags_SpanAllColumns)) {
                    // Switch to this pair
                    std::strncpy(pairSearchBuf_, r.symbol.c_str(), sizeof(pairSearchBuf_) - 1);
                    config_.symbol = r.symbol;
                    if (onRefreshData_) onRefreshData_();
                }
                ImGui::TableNextColumn(); ImGui::Text("%.8f", r.price);
                ImGui::TableNextColumn();
                ImVec4 chgCol = r.change24h >= 0
                    ? ImVec4(0.3f, 0.85f, 0.35f, 1.0f) : ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
                ImGui::TextColored(chgCol, "%.2f%%", r.change24h);
                ImGui::TableNextColumn(); ImGui::Text("%.0f", r.volume24h);
                ImGui::TableNextColumn();
                ImVec4 rsiCol = (r.rsi > 70) ? ImVec4(0.9f, 0.3f, 0.3f, 1.0f)
                    : (r.rsi < 30) ? ImVec4(0.3f, 0.85f, 0.35f, 1.0f)
                    : ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
                ImGui::TextColored(rsiCol, "%.1f", r.rsi);
                ImGui::TableNextColumn();
                ImVec4 sigCol = (r.signal == "BUY") ? ImVec4(0.3f, 0.85f, 0.35f, 1.0f)
                    : (r.signal == "SELL") ? ImVec4(0.9f, 0.3f, 0.3f, 1.0f)
                    : ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
                ImGui::TextColored(sigCol, "%s", r.signal.c_str());
                ImGui::TableNextColumn(); ImGui::Text("%.0f%%", r.confidence * 100.0);
            }
            ImGui::EndTable();
        }
    }

    ImGui::End();
}

// ---------------------------------------------------------------------------
//  L3 — Pine Script Editor Window
// ---------------------------------------------------------------------------
void AppGui::drawPineEditorWindow() {
    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Pine Script Editor", &showPineEditor_)) {
        ImGui::End();
        return;
    }

    ImGui::Text("Max %d characters", (int)(sizeof(pineCode_) - 1));
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(%d used)", (int)std::strlen(pineCode_));

    float btnAreaH = 60.0f;
    float availH = ImGui::GetContentRegionAvail().y - btnAreaH;
    ImGui::InputTextMultiline("##pine", pineCode_, sizeof(pineCode_),
        ImVec2(-1, std::max(availH, 100.0f)),
        ImGuiInputTextFlags_AllowTabInput);

    if (ImGui::Button("Run")) {
        pineEditorError_.clear();
        // Save the script to file first
        namespace fs = std::filesystem;
        std::string dir = config_.userIndicatorDir;
        std::error_code ec;
        fs::create_directories(dir, ec);
        std::string fname = dir + "/custom_script.pine";
        {
            std::ofstream f(fname);
            if (f.is_open()) {
                f << pineCode_;
                f.close();
                addLog("[Pine] Script saved to " + fname);
            } else {
                pineEditorError_ = "Failed to save script file";
                ImGui::End();
                return;
            }
        }
        // Validate and parse the script
        try {
            std::string src(pineCode_);
            auto script = PineConverter::parseSource(src);
            addLog("[Pine] Script compiled: " + script.name + " (" +
                   std::to_string(script.statements.size()) + " statements)");
            addLog("[Pine] Script applied to chart. Restart or reconnect to see results.");
        } catch (const std::exception& e) {
            pineEditorError_ = e.what();
            addLog("[Pine] Compilation error: " + pineEditorError_);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Save As...")) {
        namespace fs = std::filesystem;
        std::string dir = config_.userIndicatorDir;
        std::error_code ec;
        fs::create_directories(dir, ec);
        std::string fname = dir + "/custom_script.pine";
        std::ofstream f(fname);
        if (f.is_open()) {
            f << pineCode_;
            addLog("[Pine] Saved to " + fname);
        } else {
            pineEditorError_ = "Failed to save file: " + fname;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        std::string fname = config_.userIndicatorDir + "/custom_script.pine";
        std::ifstream f(fname);
        if (f.is_open()) {
            std::string content((std::istreambuf_iterator<char>(f)),
                                 std::istreambuf_iterator<char>());
            if (content.size() >= sizeof(pineCode_)) {
                content.resize(sizeof(pineCode_) - 1);
                addLog("[Pine] Script truncated to max length");
            }
            std::strncpy(pineCode_, content.c_str(), sizeof(pineCode_) - 1);
            pineCode_[sizeof(pineCode_) - 1] = '\0';
            addLog("[Pine] Loaded from " + fname);
            pineEditorError_.clear();
        } else {
            pineEditorError_ = "File not found: " + fname;
        }
    }

    if (!pineEditorError_.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Error: %s", pineEditorError_.c_str());
    }

    ImGui::End();
}

// ---------------------------------------------------------------------------
//  Trade History & Analytics Window
// ---------------------------------------------------------------------------
void AppGui::drawTradeHistoryWindow() {
    ImGui::SetNextWindowSize(ImVec2(750, 550), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Trade History & Analytics", &showTradeHistory_)) {
        ImGui::End();
        return;
    }

    auto allTrades = tradeHistory_.getAll();
    int total = static_cast<int>(allTrades.size());
    int wins = tradeHistory_.winCount();
    int losses = tradeHistory_.lossCount();
    double pnl = tradeHistory_.totalPnl();
    double wr = tradeHistory_.winRate();

    ImGui::Text("Trades: %d | Win: %d (%.1f%%) | Loss: %d", total, wins, wr * 100.0, losses);
    ImGui::Text("PnL: $%.8f | Avg Win: $%.8f | Avg Loss: $%.8f",
                pnl, tradeHistory_.avgWin(), tradeHistory_.avgLoss());
    ImGui::Text("Sharpe: -- | Max DD: %.1f%% | PF: %.2f",
                tradeHistory_.maxDrawdown() * 100.0, tradeHistory_.profitFactor());

    // Export button
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 100);
    if (ImGui::Button("Export CSV")) {
        auto trades = tradeHistory_.getAll();
        if (CSVExporter::exportTrades(trades, "trade_history.csv"))
            addLog("[Export] Saved trade_history.csv");
    }

    ImGui::Separator();

    // Equity curve mini chart
    if (total > 0) {
        std::vector<float> eq;
        double equity = 0;
        for (auto& t : allTrades) {
            equity += t.pnl;
            eq.push_back(static_cast<float>(equity));
        }
        ImGui::PlotLines("Equity##TH", eq.data(), static_cast<int>(eq.size()),
                         0, nullptr, FLT_MAX, FLT_MAX, ImVec2(-1, 100));
    }

    ImGui::Separator();

    // Trades table
    if (ImGui::BeginTable("##th_trades", 8, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
                          ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable, ImVec2(0, 250))) {
        ImGui::TableSetupColumn("#");
        ImGui::TableSetupColumn("Date");
        ImGui::TableSetupColumn("Pair");
        ImGui::TableSetupColumn("Side");
        ImGui::TableSetupColumn("Entry");
        ImGui::TableSetupColumn("Exit");
        ImGui::TableSetupColumn("PnL");
        ImGui::TableSetupColumn("Paper");
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < allTrades.size(); ++i) {
            auto& t = allTrades[i];
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%d", static_cast<int>(i + 1));
            ImGui::TableNextColumn();
            if (t.exitTime > 0) {
                time_t ts = static_cast<time_t>(t.exitTime / 1000);
                char buf[32];
#ifdef _WIN32
                struct tm tmBuf;
                gmtime_s(&tmBuf, &ts);
                strftime(buf, sizeof(buf), "%Y-%m-%d", &tmBuf);
#else
                struct tm tmBuf;
                gmtime_r(&ts, &tmBuf);
                strftime(buf, sizeof(buf), "%Y-%m-%d", &tmBuf);
#endif
                ImGui::Text("%s", buf);
            } else {
                ImGui::Text("-");
            }
            ImGui::TableNextColumn(); ImGui::Text("%s", t.symbol.c_str());
            ImGui::TableNextColumn();
            ImVec4 sc = (t.side == "BUY") ? ImVec4(0.3f, 0.85f, 0.35f, 1.0f) : ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
            ImGui::TextColored(sc, "%s", t.side.c_str());
            ImGui::TableNextColumn(); ImGui::Text("%.8f", t.entryPrice);
            ImGui::TableNextColumn(); ImGui::Text("%.8f", t.exitPrice);
            ImGui::TableNextColumn();
            ImVec4 pc = t.pnl >= 0 ? ImVec4(0.3f, 0.85f, 0.35f, 1.0f) : ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
            ImGui::TextColored(pc, "%.8f", t.pnl);
            ImGui::TableNextColumn(); ImGui::Text("%s", t.isPaper ? "Y" : "N");
        }
        ImGui::EndTable();
    }

    ImGui::End();
}

} // namespace crypto
