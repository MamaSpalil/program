#include "AppGui.h"
#include "../core/Logger.h"

#include "../../third_party/imgui/imgui.h"
#include "../../third_party/imgui/imgui_impl_glfw.h"
#include "../../third_party/imgui/imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>
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
    if (!glfwInit()) return false;

    // GL 3.0 + GLSL 130  (works on most systems)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window_ = glfwCreateWindow(1440, 900, "Crypto ML Trader", nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // vsync

    // ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Apply dark-metal theme
    setupTheme();

    // Platform + renderer init
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 130");

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
        volumeDeltaHistory_.push_back(delta);
        if ((int)volumeDeltaHistory_.size() > kMaxHistory) volumeDeltaHistory_.pop_front();
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
        {"ws_port",    config_.wsPort}
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
    return j;
}

void AppGui::saveConfigToFile(const std::string& path) {
    std::ofstream f(path);
    if (!f.is_open()) return;
    f << configToJson().dump(2);
}

// ---------------------------------------------------------------------------
// renderFrame
// ---------------------------------------------------------------------------
void AppGui::renderFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Full-screen dockable window
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoResize   |
                             ImGuiWindowFlags_NoMove     |
                             ImGuiWindowFlags_NoBringToFrontOnFocus |
                             ImGuiWindowFlags_MenuBar;
    ImGui::Begin("##MainWin", nullptr, flags);

    drawMenuBar();
    drawToolbar();
    drawFilterPanel();

    // Layout: three columns — PairList | Chart | UserPanel
    float totalW = ImGui::GetContentRegionAvail().x;
    float h = ImGui::GetContentRegionAvail().y - 26; // room for status
    float pairListW = 200.0f;
    float userPanelW = 280.0f;
    float centerW = totalW - pairListW - userPanelW - 8.0f; // spacing

    // Left column: PairList
    ImGui::BeginChild("##PairList", ImVec2(pairListW, h), ImGuiChildFlags_Border);
    drawPairListPanel();
    ImGui::EndChild();

    ImGui::SameLine();

    // Center column: indicators + other panels
    ImGui::BeginChild("##CenterCol", ImVec2(centerW, h), ImGuiChildFlags_None);
    drawVolumeDeltaPanel();
    if (showOrderBook_) drawOrderBookPanel();
    drawIndicatorsPanel();
    drawLogPanel();
    ImGui::EndChild();

    ImGui::SameLine();

    // Right column: User Panel (always visible, integrated)
    ImGui::BeginChild("##UserCol", ImVec2(userPanelW, h), ImGuiChildFlags_Border);
    drawUserPanel();
    ImGui::EndChild();

    drawStatusBar();

    ImGui::End();

    // Market Data window (separate, with chart)
    drawMarketDataWindow();

    // Settings window (modal-like)
    if (showSettings_) drawSettingsPanel();

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
            ImGui::MenuItem("Settings Panel", nullptr, &showSettings_);
            ImGui::MenuItem("User Panel",     nullptr, &showUserPanel_);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                addLog("[Info] Crypto ML Trader v1.0.0 — Algorithmic Trading System");
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
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
        }
        ImGui::PopStyleColor(3);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.50f, 0.18f, 0.18f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.60f, 0.22f, 0.22f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.40f, 0.15f, 0.15f, 1.0f));
        if (ImGui::Button("  Disconnect  ")) {
            if (onDisconnect_) onDisconnect_();
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
        }
        ImGui::PopStyleColor(3);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.55f, 0.35f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.65f, 0.42f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.45f, 0.28f, 0.12f, 1.0f));
        if (ImGui::Button("  Stop Trading  ")) {
            if (onStop_) onStop_();
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
    if (ImGui::Button(u8"  Режим Стакан  ")) {
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

    // ── Window size / position (applied only on first launch) ──
    ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(220, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(400, 300), ImVec2(FLT_MAX, FLT_MAX));

    ImGui::Begin("Market Data", nullptr,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

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

        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Price: %.4f",
                           snap.lastCandle.close);
        ImGui::SameLine(0, 20);
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "RSI: %.1f", rsiVal);
        ImGui::SameLine(0, 20);
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "EMA9: %.4f", ema9Val);
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
            "O: %.4f  H: %.4f  L: %.4f  V: %.1f",
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

    // ── Candlestick chart ──
    if (snap.candleHistory.empty() || (int)snap.candleHistory.size() < 2) {
        ImGui::End();
        return;
    }

    {
        static constexpr double kPricePadding = 0.05; // 5% Y-axis padding
        float priceScaleW = 70.0f;
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float totalH = std::max(200.0f, avail.y - 4.0f);
        // Layout: candles 65%, volume 20%, RSI 15%
        float priceH = totalH * 0.65f;
        float volH   = totalH * 0.20f;
        float rsiH   = totalH * 0.15f;
        ImVec2 canvasSize = ImVec2(avail.x, totalH);

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
            snprintf(label, sizeof(label), "%.2f", priceVal);
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
                snprintf(priceLabel, sizeof(priceLabel), "%.2f", lastPrice);
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
                        snprintf(overlayLabel, sizeof(overlayLabel), "%s: %.4f", pName.c_str(), pVal);
                        draw->AddText(ImVec2(p.x + 4, labelY),
                                      IM_COL32(180, 140, 240, 200), overlayLabel);
                        labelY += 14;
                    }
                }
            }
        }

        // ── 5.3: Crosshair with price label ──
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
                    snprintf(crossLabel, sizeof(crossLabel), "%.4f", crossPrice);
                    draw->AddRectFilled(ImVec2(p.x + chartW + 1, my - 8),
                                        ImVec2(p.x + w, my + 8),
                                        IM_COL32(80, 80, 90, 200));
                    draw->AddText(ImVec2(p.x + chartW + 4, my - 6),
                                  IM_COL32(240, 240, 245, 255), crossLabel);
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
//  Indicators Panel
// ---------------------------------------------------------------------------
void AppGui::drawIndicatorsPanel() {
    GuiState snap;
    {
        std::lock_guard<std::mutex> lk(stateMutex_);
        snap = state_;
    }

    if (ImGui::CollapsingHeader("Indicators", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BeginChild("##IndChild", ImVec2(0, 200), ImGuiChildFlags_Border);

        // EMA
        if (config_.indEmaEnabled) {
            ImGui::Columns(3, "##indcols", false);
            ImGui::TextColored(ImVec4(0.50f, 0.75f, 0.50f, 1.0f), "EMA Fast (%d)", config_.emaFast);
            ImGui::Text("%.4f", snap.emaFast);
            ImGui::NextColumn();
            ImGui::TextColored(ImVec4(0.75f, 0.50f, 0.50f, 1.0f), "EMA Slow (%d)", config_.emaSlow);
            ImGui::Text("%.4f", snap.emaSlow);
            ImGui::NextColumn();
            ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.75f, 1.0f), "EMA Trend (%d)", config_.emaTrend);
            ImGui::Text("%.4f", snap.emaTrend);
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
                    ImGui::Text("%.4f", snap.atr);
                    ImGui::NextColumn();
                }
                if (config_.indMacdEnabled) {
                    ImGui::Text("MACD");
                    ImGui::TextColored(
                        snap.macd.histogram >= 0
                            ? ImVec4(0.30f, 0.80f, 0.30f, 1.0f)
                            : ImVec4(0.80f, 0.30f, 0.30f, 1.0f),
                        "%.4f (S: %.4f H: %.4f)",
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
                "Upper: %.4f  Mid: %.4f  Lower: %.4f",
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
                    std::snprintf(buf, sizeof(buf), "%s: %.4f", pName.c_str(), pVal);
                    plotStr += buf;
                }
                ImGui::TextColored(ImVec4(0.70f, 0.70f, 0.90f, 1.0f), "%s", plotStr.c_str());
            }
        }

        ImGui::EndChild();
    }
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
                        ImGui::TextColored(valColor, "%.4f", pVal);
                    } else if (pVal < 0) {
                        valColor = ImVec4(0.90f, 0.30f, 0.30f, 1.0f);
                        ImGui::TextColored(valColor, "%.4f", pVal);
                    } else {
                        valColor = ImVec4(0.80f, 0.80f, 0.82f, 1.0f);
                        ImGui::TextColored(valColor, "%.4f", pVal);
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
        ImGui::Text("Stop Loss:   %.4f", snap.lastSignal.stopLoss);
        ImGui::Text("Take Profit: %.4f", snap.lastSignal.takeProfit);
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
                ImGui::TextColored(cl, "  %s @ %.4f (%.1f%%)",
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
        ImGui::InputDouble("Qty", &orderQty_, 0.001, 0.01, "%.4f");
        if (orderQty_ < 0.0) orderQty_ = 0.0;

        // Limit price (only for Limit orders)
        if (orderTypeIdx_ == 1) {
            ImGui::SetNextItemWidth(120);
            ImGui::InputDouble("Price", &orderPrice_, 1.0, 10.0, "%.2f");
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
        ImGui::TextColored(eqColor, "$%.2f", snap.equity);
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
            static bool bufInit = false;
            if (!bufInit) {
                strncpy(apiKeyBuf, config_.apiKey.c_str(), sizeof(apiKeyBuf) - 1);
                strncpy(apiSecBuf, config_.apiSecret.c_str(), sizeof(apiSecBuf) - 1);
                strncpy(passBuf, config_.passphrase.c_str(), sizeof(passBuf) - 1);
                strncpy(baseUrlBuf, config_.baseUrl.c_str(), sizeof(baseUrlBuf) - 1);
                strncpy(wsHostBuf, config_.wsHost.c_str(), sizeof(wsHostBuf) - 1);
                strncpy(wsPortBuf, config_.wsPort.c_str(), sizeof(wsPortBuf) - 1);
                bufInit = true;
            }
            ImGui::InputText("API Key", apiKeyBuf, sizeof(apiKeyBuf));
            ImGui::InputText("API Secret", apiSecBuf, sizeof(apiSecBuf),
                             ImGuiInputTextFlags_Password);
            ImGui::InputText("Passphrase", passBuf, sizeof(passBuf),
                             ImGuiInputTextFlags_Password);
            ImGui::InputText("Base URL", baseUrlBuf, sizeof(baseUrlBuf));
            ImGui::InputText("WS Host", wsHostBuf, sizeof(wsHostBuf));
            ImGui::InputText("WS Port", wsPortBuf, sizeof(wsPortBuf));
            ImGui::Checkbox("Testnet", &config_.testnet);

            if (ImGui::Button("Apply Exchange Settings")) {
                config_.apiKey = apiKeyBuf;
                config_.apiSecret = apiSecBuf;
                config_.passphrase = passBuf;
                config_.baseUrl = baseUrlBuf;
                config_.wsHost = wsHostBuf;
                config_.wsPort = wsPortBuf;
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

        ImGui::EndTabBar();
    }

    ImGui::Separator();
    if (ImGui::Button("Save All Settings to File")) {
        saveConfigToFile(configPath_);
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
    ImGui::InputDouble("##MinPrice", &config_.filterMinPrice, 0, 0, "P>%.2f");
    ImGui::SameLine();

    ImGui::SetNextItemWidth(80);
    ImGui::InputDouble("##MaxPrice", &config_.filterMaxPrice, 0, 0, "P<%.2f");
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
//  Volume Delta Bars
// ---------------------------------------------------------------------------
void AppGui::drawVolumeDeltaPanel() {
    std::lock_guard<std::mutex> lk(stateMutex_);
    if (volumeDeltaHistory_.empty()) return;

    if (ImGui::CollapsingHeader("Volume Delta", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImVec2 canvasSize = ImVec2(ImGui::GetContentRegionAvail().x, 80);
        ImGui::BeginChild("##VolDelta", canvasSize, ImGuiChildFlags_Border);

        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        float w = canvasSize.x - 4;
        float h = canvasSize.y - 4;

        int count = (int)volumeDeltaHistory_.size();
        if (count < 1) { ImGui::EndChild(); return; }

        double maxAbs = 1.0;
        for (auto& v : volumeDeltaHistory_) {
            if (std::abs(v) > maxAbs) maxAbs = std::abs(v);
        }

        float barW = std::max(2.0f, w / (float)count - 1.0f);
        float midY = p.y + h * 0.5f;

        // Zero line
        draw->AddLine(ImVec2(p.x, midY), ImVec2(p.x + w, midY),
                      IM_COL32(80, 80, 85, 150), 1.0f);

        for (int i = 0; i < count; ++i) {
            float x = p.x + 2.0f + (float)i * (barW + 1.0f);
            double val = volumeDeltaHistory_[i];
            float barH = (float)(std::abs(val) / maxAbs) * (h * 0.48f);

            ImU32 col = (val >= 0)
                ? IM_COL32(40, 180, 80, 200)
                : IM_COL32(200, 60, 60, 200);

            if (val >= 0) {
                draw->AddRectFilled(ImVec2(x, midY - barH), ImVec2(x + barW, midY), col);
            } else {
                draw->AddRectFilled(ImVec2(x, midY), ImVec2(x + barW, midY + barH), col);
            }
        }

        ImGui::Dummy(canvasSize);
        ImGui::EndChild();
    }
}

// ---------------------------------------------------------------------------
//  Order Book Panel (Стакан / Биржевой стакан)
// ---------------------------------------------------------------------------
void AppGui::drawOrderBookPanel() {
    GuiState snap;
    {
        std::lock_guard<std::mutex> lk(stateMutex_);
        snap = state_;
    }

    if (ImGui::CollapsingHeader(u8"Order Book (Стакан)", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BeginChild("##OrderBook", ImVec2(0, 260), ImGuiChildFlags_Border);

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
                "%.4f", level.price);
            ImGui::SameLine(colW * 0.4f);
            ImGui::Text("%.6f", level.qty);
            ImGui::SameLine(colW * 0.75f);
            ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.62f, 1.0f),
                "%.2f", level.price * level.qty);
        }

        // Spread
        ImGui::Separator();
        double spread = 0.0;
        if (!snap.orderBook.asks.empty() && !snap.orderBook.bids.empty()) {
            spread = snap.orderBook.asks[0].price - snap.orderBook.bids[0].price;
        }
        ImGui::TextColored(ImVec4(0.70f, 0.70f, 0.72f, 1.0f),
            "  Spread: %.4f", spread);
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
                "%.4f", level.price);
            ImGui::SameLine(colW * 0.4f);
            ImGui::Text("%.6f", level.qty);
            ImGui::SameLine(colW * 0.75f);
            ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.62f, 1.0f),
                "%.2f", level.price * level.qty);
        }

        if (snap.orderBook.bids.empty() && snap.orderBook.asks.empty()) {
            ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.52f, 1.0f),
                "No order book data available. Connect to an exchange to see the order book.");
        }

        ImGui::EndChild();
    }
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
                       "Crypto ML Trader v1.0.0");
}

// ---------------------------------------------------------------------------
//  Pair List Panel — left sidebar with SPOT/FUTURES toggle, search, pair list
// ---------------------------------------------------------------------------
void AppGui::drawPairListPanel() {
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
}

// ---------------------------------------------------------------------------
//  User Panel — embedded right column with balance, orders, trades, price, P&L
// ---------------------------------------------------------------------------
void AppGui::drawUserPanel() {
    GuiState snap;
    {
        std::lock_guard<std::mutex> lk(stateMutex_);
        snap = state_;
    }

    // ── Current Price ──
    ImGui::TextColored(ImVec4(0.60f, 0.70f, 0.85f, 1.0f), "%s", config_.symbol.c_str());
    if (snap.currentPrice > 0) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.30f, 0.85f, 0.35f, 1.0f), "$%.2f", snap.currentPrice);
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
    ImGui::Separator();

    // ── 3.1 Account Balance ──
    if (ImGui::CollapsingHeader("Balance", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (config_.marketType == "futures") {
            // Futures balance
            ImGui::Text("Wallet:     $%.2f", snap.futuresBalance.totalWalletBalance);
            ImVec4 uplCol = snap.futuresBalance.totalUnrealizedProfit >= 0
                ? ImVec4(0.30f, 0.85f, 0.35f, 1.0f)
                : ImVec4(0.90f, 0.30f, 0.30f, 1.0f);
            ImGui::TextColored(uplCol, "Unreal P&L: $%.2f", snap.futuresBalance.totalUnrealizedProfit);
            ImGui::Text("Margin:     $%.2f", snap.futuresBalance.totalMarginBalance);
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
                        ImGui::Text("%.4f", d.availBal);
                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("$%.2f", d.usdValue);
                    }
                    ImGui::EndTable();
                }
            } else {
                // Simple spot balance
                double pnl = snap.equity - snap.initialCapital;
                ImVec4 eqCol = pnl >= 0
                    ? ImVec4(0.30f, 0.85f, 0.35f, 1.0f)
                    : ImVec4(0.90f, 0.30f, 0.30f, 1.0f);
                ImGui::TextColored(eqCol, "Equity: $%.2f", snap.equity);
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
                    ImGui::Text("%.4f", o.price);
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%.4f", o.qty);
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
                    ImGui::Text("%.4f", t.price);
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%.4f", t.qty);
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
                    ImGui::Text("  Amt: %.4f  Entry: %.2f", p.positionAmt, p.entryPrice);
                    ImVec4 uplCol = p.unrealizedProfit >= 0
                        ? ImVec4(0.30f, 0.85f, 0.35f, 1.0f)
                        : ImVec4(0.90f, 0.30f, 0.30f, 1.0f);
                    ImGui::TextColored(uplCol, "  UPL: $%.4f", p.unrealizedProfit);
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
}

} // namespace crypto
