#include "AppGui.h"
#include "../core/Logger.h"

#include "../../third_party/imgui/imgui.h"
#include "../../third_party/imgui/imgui_impl_glfw.h"
#include "../../third_party/imgui/imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>
#include <fstream>
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

    auto& ex = j["exchange"];
    config_.exchangeName = ex.value("name", "binance");
    config_.apiKey       = ex.value("api_key", "");
    config_.apiSecret    = ex.value("api_secret", "");
    config_.passphrase   = ex.value("passphrase", "");
    config_.testnet      = ex.value("testnet", true);
    config_.baseUrl      = ex.value("base_url", "https://testnet.binance.vision");

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
        {"base_url",   config_.baseUrl}
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

    // Layout: two columns
    float w = ImGui::GetContentRegionAvail().x;
    float h = ImGui::GetContentRegionAvail().y - 26; // room for status

    ImGui::BeginChild("##LeftCol", ImVec2(w * 0.65f, h), ImGuiChildFlags_None);
    drawMarketPanel();
    drawCandlestickChart();
    drawVolumeDeltaPanel();
    if (showOrderBook_) drawOrderBookPanel();
    drawIndicatorsPanel();
    drawLogPanel();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##RightCol", ImVec2(0, h), ImGuiChildFlags_None);
    drawSignalPanel();
    drawPortfolioPanel();
    ImGui::EndChild();

    drawStatusBar();

    ImGui::End();

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

void AppGui::drawMarketPanel() {
    GuiState snap;
    {
        std::lock_guard<std::mutex> lk(stateMutex_);
        snap = state_;
    }

    if (ImGui::CollapsingHeader("Market Data", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BeginChild("##MarketChild", ImVec2(0, 190), ImGuiChildFlags_Border);

        // Price display with change percentage
        double priceChange = 0.0;
        double pctChange = 0.0;
        if (snap.lastCandle.open > 0) {
            priceChange = snap.lastCandle.close - snap.lastCandle.open;
            pctChange = (priceChange / snap.lastCandle.open) * 100.0;
        }
        ImVec4 chgColor = (priceChange >= 0)
            ? ImVec4(0.30f, 0.85f, 0.35f, 1.0f)
            : ImVec4(0.90f, 0.30f, 0.30f, 1.0f);

        ImGui::TextColored(ImVec4(0.90f, 0.90f, 0.92f, 1.0f), "Price");
        ImGui::SameLine(100);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.0f));
        ImGui::Text("%.4f", snap.lastCandle.close);
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::TextColored(chgColor, "(%+.2f%%)", pctChange);

        ImGui::SameLine(350);
        ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.52f, 1.0f),
            "O: %.4f  H: %.4f  L: %.4f  V: %.1f",
            snap.lastCandle.open, snap.lastCandle.high,
            snap.lastCandle.low, snap.lastCandle.volume);

        // Mini price chart
        {
            std::lock_guard<std::mutex> lk(stateMutex_);
            PlotLineFromDeque("##price", priceHistory_, 120,
                              ImVec4(0.40f, 0.65f, 0.90f, 1.0f));
        }

        ImGui::EndChild();
    }
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
            static bool bufInit = false;
            if (!bufInit) {
                strncpy(apiKeyBuf, config_.apiKey.c_str(), sizeof(apiKeyBuf) - 1);
                strncpy(apiSecBuf, config_.apiSecret.c_str(), sizeof(apiSecBuf) - 1);
                strncpy(passBuf, config_.passphrase.c_str(), sizeof(passBuf) - 1);
                strncpy(baseUrlBuf, config_.baseUrl.c_str(), sizeof(baseUrlBuf) - 1);
                bufInit = true;
            }
            ImGui::InputText("API Key", apiKeyBuf, sizeof(apiKeyBuf));
            ImGui::InputText("API Secret", apiSecBuf, sizeof(apiSecBuf),
                             ImGuiInputTextFlags_Password);
            ImGui::InputText("Passphrase", passBuf, sizeof(passBuf),
                             ImGuiInputTextFlags_Password);
            ImGui::InputText("Base URL", baseUrlBuf, sizeof(baseUrlBuf));
            ImGui::Checkbox("Testnet", &config_.testnet);

            if (ImGui::Button("Apply Exchange Settings")) {
                config_.apiKey = apiKeyBuf;
                config_.apiSecret = apiSecBuf;
                config_.passphrase = passBuf;
                config_.baseUrl = baseUrlBuf;
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
//  Candlestick Chart — improved bar visualization
// ---------------------------------------------------------------------------
void AppGui::drawCandlestickChart() {
    GuiState snap;
    {
        std::lock_guard<std::mutex> lk(stateMutex_);
        snap = state_;
    }

    if (snap.candleHistory.empty()) return;

    if (ImGui::CollapsingHeader("Candlestick Chart", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImVec2 canvasSize = ImVec2(ImGui::GetContentRegionAvail().x, 220);
        ImGui::BeginChild("##CandleChart", canvasSize, ImGuiChildFlags_Border);

        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        float w = canvasSize.x - 4;
        float h = canvasSize.y - 4;

        int count = (int)snap.candleHistory.size();
        if (count < 2) { ImGui::EndChild(); return; }

        // Find price range
        double pMin = 1e18, pMax = -1e18;
        for (auto& c : snap.candleHistory) {
            if (c.low  < pMin) pMin = c.low;
            if (c.high > pMax) pMax = c.high;
        }
        double range = pMax - pMin;
        if (range < 1e-9) range = 1.0;

        float barWidth = std::max(2.0f, w / (float)count - 1.0f);
        float halfBar  = barWidth * 0.5f;

        for (int i = 0; i < count; ++i) {
            auto& c = snap.candleHistory[i];
            float x = p.x + 2.0f + (float)i * (barWidth + 1.0f);

            // Map prices to pixel Y (top = high price, bottom = low price)
            float yHigh  = p.y + h - (float)((c.high  - pMin) / range) * h;
            float yLow   = p.y + h - (float)((c.low   - pMin) / range) * h;
            float yOpen  = p.y + h - (float)((c.open  - pMin) / range) * h;
            float yClose = p.y + h - (float)((c.close - pMin) / range) * h;

            bool bullish = c.close >= c.open;
            ImU32 color = bullish
                ? IM_COL32(40, 200, 80, 255)    // green
                : IM_COL32(220, 60, 60, 255);   // red

            // Wick (high-low line)
            float wickX = x + halfBar;
            draw->AddLine(ImVec2(wickX, yHigh), ImVec2(wickX, yLow), color, 1.0f);

            // Body (open-close rect)
            float bodyTop = std::min(yOpen, yClose);
            float bodyBot = std::max(yOpen, yClose);
            if (bodyBot - bodyTop < 1.0f) bodyBot = bodyTop + 1.0f;

            if (bullish) {
                draw->AddRectFilled(ImVec2(x, bodyTop), ImVec2(x + barWidth, bodyBot), color);
            } else {
                draw->AddRectFilled(ImVec2(x, bodyTop), ImVec2(x + barWidth, bodyBot), color);
            }
        }

        // Price scale on the right
        for (int i = 0; i <= 4; ++i) {
            float frac = (float)i / 4.0f;
            float yLine = p.y + h - frac * h;
            double priceVal = pMin + frac * range;
            char label[32];
            snprintf(label, sizeof(label), "%.2f", priceVal);
            draw->AddText(ImVec2(p.x + w - 60, yLine - 6), IM_COL32(130, 130, 135, 200), label);
            draw->AddLine(ImVec2(p.x, yLine), ImVec2(p.x + w - 65, yLine),
                          IM_COL32(50, 50, 55, 100), 1.0f);
        }

        // Draw user indicator overlay values as colored label on the chart
        if (!snap.userIndicatorPlots.empty()) {
            float labelY = p.y + 4;
            for (const auto& [indName, plots] : snap.userIndicatorPlots) {
                for (const auto& [pName, pVal] : plots) {
                    if (pVal > pMin && pVal < pMax) {
                        float yVal = p.y + h - (float)((pVal - pMin) / range) * h;
                        draw->AddLine(ImVec2(p.x, yVal), ImVec2(p.x + w - 65, yVal),
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

        ImGui::Dummy(canvasSize);
        ImGui::EndChild();
    }
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

} // namespace crypto
