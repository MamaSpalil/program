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
    config_.testnet      = ex.value("testnet", true);
    config_.baseUrl      = ex.value("base_url", "https://testnet.binance.vision");

    auto& tr = j["trading"];
    config_.symbol         = tr.value("symbol", "BTCUSDT");
    config_.interval       = tr.value("interval", "15m");
    config_.mode           = tr.value("mode", "paper");
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
        {"testnet",    config_.testnet},
        {"base_url",   config_.baseUrl}
    };
    j["trading"] = {
        {"symbol",          config_.symbol},
        {"interval",        config_.interval},
        {"mode",            config_.mode},
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

    // Layout: two columns
    float w = ImGui::GetContentRegionAvail().x;
    float h = ImGui::GetContentRegionAvail().y - 26; // room for status

    ImGui::BeginChild("##LeftCol", ImVec2(w * 0.65f, h), ImGuiChildFlags_None);
    drawMarketPanel();
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

    // Display current pair & mode
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
    ImGui::TextColored(ImVec4(0.60f, 0.70f, 0.85f, 1.0f),
                       "%s  |  %s  |  %s",
                       config_.symbol.c_str(),
                       config_.interval.c_str(),
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
    std::vector<float> v(data.begin(), data.end());
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

        // Price display
        ImGui::TextColored(ImVec4(0.90f, 0.90f, 0.92f, 1.0f), "Price");
        ImGui::SameLine(100);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.0f));
        ImGui::Text("%.4f", snap.lastCandle.close);
        ImGui::PopStyleColor();

        ImGui::SameLine(250);
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
        ImGui::BeginChild("##IndChild", ImVec2(0, 180), ImGuiChildFlags_Border);

        // EMA
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

        // RSI
        ImGui::Columns(3, "##indcols2", false);
        ImGui::Text("RSI (%d)", config_.rsiPeriod);
        {
            float rsiVal = (float)snap.rsi;
            ImVec4 rsiColor = (rsiVal > 70) ? ImVec4(0.90f, 0.30f, 0.30f, 1.0f) :
                              (rsiVal < 30) ? ImVec4(0.30f, 0.90f, 0.30f, 1.0f) :
                                              ImVec4(0.80f, 0.80f, 0.30f, 1.0f);
            ImGui::TextColored(rsiColor, "%.2f", snap.rsi);
        }

        ImGui::NextColumn();
        ImGui::Text("ATR (%d)", config_.atrPeriod);
        ImGui::Text("%.4f", snap.atr);

        ImGui::NextColumn();
        ImGui::Text("MACD");
        ImGui::TextColored(
            snap.macd.histogram >= 0
                ? ImVec4(0.30f, 0.80f, 0.30f, 1.0f)
                : ImVec4(0.80f, 0.30f, 0.30f, 1.0f),
            "%.4f (S: %.4f H: %.4f)",
            snap.macd.macd, snap.macd.signal, snap.macd.histogram);
        ImGui::Columns(1);

        // Bollinger Bands
        ImGui::Separator();
        ImGui::Text("Bollinger Bands (%d, %.1f)", config_.bbPeriod, config_.bbStddev);
        ImGui::SameLine(220);
        ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.80f, 1.0f),
            "Upper: %.4f  Mid: %.4f  Lower: %.4f",
            snap.bb.upper, snap.bb.middle, snap.bb.lower);

        // RSI mini chart
        {
            std::lock_guard<std::mutex> lk(stateMutex_);
            if (!rsiHistory_.empty()) {
                std::vector<float> v(rsiHistory_.begin(), rsiHistory_.end());
                ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.85f, 0.70f, 0.30f, 1.0f));
                ImGui::PlotLines("RSI##chart", v.data(), (int)v.size(), 0,
                                 nullptr, 0.0f, 100.0f, ImVec2(-1, 40));
                ImGui::PopStyleColor();
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

            const char* exchanges[] = { "binance", "bybit" };
            int exIdx = (config_.exchangeName == "bybit") ? 1 : 0;
            if (ImGui::Combo("Exchange", &exIdx, exchanges, 2)) {
                config_.exchangeName = exchanges[exIdx];
            }

            static char apiKeyBuf[256] = {};
            static char apiSecBuf[256] = {};
            static char baseUrlBuf[256] = {};
            static bool bufInit = false;
            if (!bufInit) {
                strncpy(apiKeyBuf, config_.apiKey.c_str(), sizeof(apiKeyBuf) - 1);
                strncpy(apiSecBuf, config_.apiSecret.c_str(), sizeof(apiSecBuf) - 1);
                strncpy(baseUrlBuf, config_.baseUrl.c_str(), sizeof(baseUrlBuf) - 1);
                bufInit = true;
            }
            ImGui::InputText("API Key", apiKeyBuf, sizeof(apiKeyBuf));
            ImGui::InputText("API Secret", apiSecBuf, sizeof(apiSecBuf),
                             ImGuiInputTextFlags_Password);
            ImGui::InputText("Base URL", baseUrlBuf, sizeof(baseUrlBuf));
            ImGui::Checkbox("Testnet", &config_.testnet);

            if (ImGui::Button("Apply Exchange Settings")) {
                config_.apiKey = apiKeyBuf;
                config_.apiSecret = apiSecBuf;
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

            const char* intervals[] = {"1m","3m","5m","15m","30m","1h","2h","4h","1d"};
            int intIdx = 3; // 15m default
            for (int i = 0; i < 9; ++i) {
                if (config_.interval == intervals[i]) { intIdx = i; break; }
            }
            if (ImGui::Combo("Interval", &intIdx, intervals, 9)) {
                config_.interval = intervals[intIdx];
            }

            const char* modes[] = {"paper", "live"};
            int modeIdx = (config_.mode == "live") ? 1 : 0;
            if (ImGui::Combo("Mode", &modeIdx, modes, 2)) {
                config_.mode = modes[modeIdx];
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

            ImGui::Text("Exponential Moving Averages");
            ImGui::SliderInt("EMA Fast", &config_.emaFast, 2, 50);
            ImGui::SliderInt("EMA Slow", &config_.emaSlow, 5, 100);
            ImGui::SliderInt("EMA Trend", &config_.emaTrend, 50, 500);

            ImGui::Separator();
            ImGui::Text("Relative Strength Index");
            ImGui::SliderInt("RSI Period", &config_.rsiPeriod, 2, 50);
            {
                float rsiOB = (float)config_.rsiOverbought;
                if (ImGui::SliderFloat("RSI Overbought", &rsiOB, 50.0f, 90.0f, "%.0f"))
                    config_.rsiOverbought = rsiOB;
                float rsiOS = (float)config_.rsiOversold;
                if (ImGui::SliderFloat("RSI Oversold", &rsiOS, 10.0f, 50.0f, "%.0f"))
                    config_.rsiOversold = rsiOS;
            }

            ImGui::Separator();
            ImGui::Text("ATR");
            ImGui::SliderInt("ATR Period", &config_.atrPeriod, 2, 50);

            ImGui::Separator();
            ImGui::Text("MACD");
            ImGui::SliderInt("MACD Fast", &config_.macdFast, 2, 50);
            ImGui::SliderInt("MACD Slow", &config_.macdSlow, 5, 100);
            ImGui::SliderInt("MACD Signal", &config_.macdSignal, 2, 50);

            ImGui::Separator();
            ImGui::Text("Bollinger Bands");
            ImGui::SliderInt("BB Period", &config_.bbPeriod, 5, 50);
            ImGui::InputDouble("BB Std Dev", &config_.bbStddev, 0.1, 0.5, "%.1f");

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
