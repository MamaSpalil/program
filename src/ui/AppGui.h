#pragma once

#include "../data/CandleData.h"
#include "../exchange/IExchange.h"
#include "../data/ExchangeDB.h"
#include "../indicators/IndicatorEngine.h"
#include "../strategy/RiskManager.h"
#include "../trading/OrderManagement.h"
#include "../trading/PaperTrading.h"
#include "../trading/TradeMarker.h"
#include "../trading/TradeHistory.h"
#include "../backtest/BacktestEngine.h"
#include "../alerts/AlertManager.h"
#include "../scanner/MarketScanner.h"
#include "../data/CSVExporter.h"
#include "../indicators/PineVisuals.h"
#include "../security/KeyVault.h"
#include "../data/TradeRepository.h"
#include "../data/AlertRepository.h"
#include "../data/BacktestRepository.h"
#include "../data/PositionRepository.h"
#include "../data/DrawingRepository.h"
#include "../data/AuxRepository.h"
#include "../data/EquityRepository.h"
#include "LayoutManager.h"
#include <nlohmann/json.hpp>
#include <atomic>
#include <mutex>
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <chrono>
#include <functional>
#include <thread>

struct GLFWwindow;

namespace crypto {

// All data the GUI needs to display, updated from the engine thread
struct GuiState {
    // Market data
    Candle lastCandle;
    std::deque<Candle> candleHistory;  // last N candles for chart

    // Order book snapshot
    OrderBook orderBook;

    // Indicators
    double emaFast{0}, emaSlow{0}, emaTrend{0};
    double rsi{50};
    double atr{0};
    MACDResult macd;
    BBResult bb;

    // Signal
    Signal lastSignal;

    // Risk / Portfolio
    double equity{0};
    double initialCapital{1000};
    double drawdown{0};
    int openPositions{0};

    // Status
    bool connected{false};
    bool trading{false};
    std::string statusMessage{"Disconnected"};

    // User indicator results from Pine Script files
    std::map<std::string, std::map<std::string, double>> userIndicatorPlots;

    // Available trading pairs (populated after connection)
    std::vector<SymbolInfo> availableSymbols;

    // Trade statistics
    std::vector<TradeRecord> trades;
    double totalPnl{0.0};
    double winRate{0.0};

    // Log lines
    std::deque<std::string> logLines;

    // Current price of active pair (updated via polling)
    double currentPrice{0.0};
    std::string currentPriceError;

    // Open orders from exchange
    std::vector<OpenOrderInfo> openOrders;

    // User trades from exchange
    std::vector<UserTradeInfo> userTrades;

    // Futures positions
    std::vector<PositionInfo> futuresPositions;

    // Futures balance
    FuturesBalanceInfo futuresBalance;

    // Account balance details (OKX style)
    std::vector<AccountBalanceDetail> accountBalanceDetails;

    // Trade markers for chart visualization (H3)
    std::vector<TradeMarker> tradeMarkers;

    // TP/SL lines for chart
    std::vector<TPSLLine> tpslLines;

    // Paper trading account snapshot (H2)
    PaperAccount paperAccount;

    // Scanner results (L1)
    std::vector<ScanResult> scanResults;

    // Alert notifications
    std::vector<std::string> alertNotifications;

    // Backtest result (M1)
    BacktestEngine::Result backtestResult;
    bool backtestDone{false};

    // Equity curve points (M2)
    std::vector<double> equityCurveData;
    std::vector<double> equityCurveTimes;

    // Geo-blocking: alternative endpoint in use
    bool usingAltEndpoint{false};
};

// Configuration that can be edited through the GUI
struct GuiConfig {
    // Exchange
    std::string exchangeName{"binance"};
    std::string apiKey;
    std::string apiSecret;
    std::string passphrase;   // for OKX, Bitget, KuCoin
    bool testnet{true};
    std::string baseUrl{"https://testnet.binance.vision"};
    std::string wsHost{"testnet.binance.vision"};
    std::string wsPort{"443"};
    std::string futuresBaseUrl{"https://testnet.binancefuture.com"};
    std::string futuresWsHost{"fstream.binancefuture.com"};
    std::string futuresWsPort{"443"};

    // Trading
    std::string symbol{"BTCUSDT"};
    std::string interval{"15m"};
    std::string mode{"paper"};
    std::string marketType{"futures"}; // "futures" or "spot"
    double initialCapital{1000.0};

    // Filters
    double filterMinVolume{0.0};
    double filterMinPrice{0.0};
    double filterMaxPrice{0.0};
    double filterMinChange{-100.0};
    double filterMaxChange{100.0};

    // Active indicators (up to 5 selected from list)
    bool indEmaEnabled{true};
    bool indRsiEnabled{true};
    bool indAtrEnabled{true};
    bool indMacdEnabled{true};
    bool indBbEnabled{true};

    // Indicators
    int emaFast{9}, emaSlow{21}, emaTrend{200};
    int rsiPeriod{14};
    double rsiOverbought{70}, rsiOversold{30};
    int atrPeriod{14};
    int macdFast{12}, macdSlow{26}, macdSignal{9};
    int bbPeriod{20};
    double bbStddev{2.0};

    // ML
    int lstmHidden{128}, lstmSeqLen{60};
    double lstmDropout{0.2};
    int xgbMaxDepth{6}, xgbRounds{500};
    double xgbEta{0.1};
    double ensembleMinConfidence{0.65};
    int retrainIntervalHours{24};
    int lookbackDays{90};

    // Risk
    double maxRiskPerTrade{0.02};
    double maxDrawdown{0.15};
    double atrStopMultiplier{1.5};
    double rrRatio{2.0};

    // User indicator dir
    std::string userIndicatorDir{"user_indicator"};

    // Layout proportions (height fraction of available space)
    float layoutLogPct{0.10f};
    float layoutVdPct{0.15f};
    float layoutIndPct{0.20f};
    bool  layoutLocked{true};   // when true, windows cannot be moved/resized (default ON)
};

class AppGui {
public:
    AppGui();
    ~AppGui();

    // Initialize window and ImGui context
    bool init(const std::string& configPath);

    // Main render loop (blocking)
    void run();

    // Shutdown
    void shutdown();

    // Thread-safe state update (called from engine thread)
    void updateState(const GuiState& state);

    // Get current config (thread-safe)
    GuiConfig getConfig() const;

    // Callbacks for engine control
    using ConnectCallback    = std::function<void(const GuiConfig&)>;
    using DisconnectCallback = std::function<void()>;
    using StartCallback      = std::function<void()>;
    using StopCallback       = std::function<void()>;
    using SaveConfigCallback = std::function<void(const GuiConfig&)>;
    using OrderCallback      = std::function<void(const std::string& symbol,
                                                   const std::string& side,
                                                   const std::string& type,
                                                   double qty,
                                                   double price)>;
    using LoadPairsCallback  = std::function<void(const std::string& marketType)>;
    using RefreshDataCallback = std::function<void()>;
    using SetLeverageCallback = std::function<void(const std::string& symbol, int leverage)>;

    void setConnectCallback(ConnectCallback cb)       { onConnect_ = std::move(cb); }
    void setDisconnectCallback(DisconnectCallback cb)  { onDisconnect_ = std::move(cb); }
    void setStartCallback(StartCallback cb)            { onStart_ = std::move(cb); }
    void setStopCallback(StopCallback cb)              { onStop_ = std::move(cb); }
    void setSaveConfigCallback(SaveConfigCallback cb)  { onSaveConfig_ = std::move(cb); }
    void setOrderCallback(OrderCallback cb)            { onOrder_ = std::move(cb); }
    void setLoadPairsCallback(LoadPairsCallback cb)    { onLoadPairs_ = std::move(cb); }
    void setRefreshDataCallback(RefreshDataCallback cb) { onRefreshData_ = std::move(cb); }
    void setSetLeverageCallback(SetLeverageCallback cb) { onSetLeverage_ = std::move(cb); }

    // Add a log line (thread-safe)
    void addLog(const std::string& line);

    // Save current config to JSON file
    void saveConfigToFile(const std::string& path);

    bool shouldClose() const { return shouldClose_; }

    /// Wire database repositories to owned modules (TradeHistory, AlertManager, etc.)
    void setDatabaseRepositories(TradeRepository* tradeRepo,
                                  AlertRepository* alertRepo,
                                  BacktestRepository* backtestRepo,
                                  PositionRepository* positionRepo,
                                  DrawingRepository* drawingRepo,
                                  AuxRepository* auxRepo,
                                  EquityRepository* equityRepo);

private:
    void setupTheme();
    void renderFrame();

    // UI Panels
    void drawMenuBar();
    void drawToolbar();
    void drawMainToolbarWindow();  // standalone Main Toolbar window
    void drawFiltersBarWindow();   // standalone Filters Bar window
    void drawMarketDataWindow();  // combined Market Data window with chart
    void drawVolumeDeltaPanel();
    void drawOrderBookPanel();
    void drawIndicatorsPanel();
    void drawSignalPanel();
    void drawTradingPanel();
    void drawPortfolioPanel();
    void drawSettingsPanel();
    void drawLogPanel();
    void drawLogWindow();  // standalone log window (fixed layout)
    void drawStatusBar();
    void drawFilterPanel();
    void drawUserIndicatorDashboard();
    void drawUserPanel();
    void drawPairSelector();
    void drawPairListPanel();

    // New panels (Part 2)
    void drawOrderManagementWindow();   // H1
    void drawPaperTradingWindow();      // H2
    void drawBacktestWindow();          // M1
    void drawAlertsWindow();            // M3
    void drawMarketScannerWindow();     // L1
    void drawPineEditorWindow();        // L3
    void drawTradeHistoryWindow();      // Trade History

    // Load config from JSON file
    void loadConfig(const std::string& path);
    nlohmann::json configToJson() const;

    GLFWwindow* window_{nullptr};
    std::atomic<bool> shouldClose_{false};
    std::string configPath_;

    mutable std::mutex stateMutex_;
    GuiState state_;
    GuiConfig config_;

    bool showSettings_{false};
    bool showDemo_{false};
    bool showOrderBook_{false};  // Order book mode
    bool showUserPanel_{true};   // User panel (separate window)
    bool showPairList_{true};    // Pair list sidebar
    bool showVolumeDelta_{true}; // Volume delta panel
    bool showIndicators_{true};  // Indicators panel
    bool showLogs_{true};        // Logs panel

    // Pair selector state
    int selectedPairIdx_{-1};

    // Signal history for display
    std::deque<Signal> signalHistory_;

    // Equity history for mini chart
    std::deque<double> equityHistory_;

    // RSI history
    std::deque<double> rsiHistory_;

    // Price history for chart
    std::deque<double> priceHistory_;

    // Volume delta history (buy volume - sell volume)
    std::deque<double> volumeDeltaHistory_;

    static constexpr int kMaxHistory = 200;
    static constexpr int kMaxLogLines = 500;

    // Candlestick chart interactive state
    float chartBarWidth_{8.0f};    // current bar width (zoom level)
    int   chartScrollOffset_{0};   // horizontal scroll offset (bars from end)
    float chartMinBarWidth_{2.0f};
    float chartMaxBarWidth_{30.0f};
    bool  needsChartReset_{true};  // reset view on first load / pair change / TF change

    ConnectCallback    onConnect_;
    DisconnectCallback onDisconnect_;
    StartCallback      onStart_;
    StopCallback       onStop_;
    SaveConfigCallback onSaveConfig_;
    OrderCallback      onOrder_;
    LoadPairsCallback  onLoadPairs_;
    RefreshDataCallback onRefreshData_;
    SetLeverageCallback onSetLeverage_;

    // Trading panel state
    double orderQty_{0.001};
    double orderPrice_{0.0};
    int    orderTypeIdx_{0};  // 0=Market, 1=Limit

    // Pair list panel state
    char pairSearchBuf_[64]{};
    bool pairListLoading_{false};
    std::vector<SymbolInfo> cachedPairs_;
    std::chrono::steady_clock::time_point pairCacheTime_{};
    std::string pairListError_;

    // Active pair for highlight
    std::string activePair_;

    // User data refresh timing
    std::chrono::steady_clock::time_point lastBalanceRefresh_{};
    std::chrono::steady_clock::time_point lastPriceRefresh_{};
    std::chrono::steady_clock::time_point lastOrderRefresh_{};
    std::chrono::steady_clock::time_point lastTradeRefresh_{};

    // ── New module state ──────────────────────────────────────────────────

    // Window visibility toggles
    bool showOrderManagement_{false};
    bool showPaperTrading_{false};
    bool showBacktest_{false};
    bool showAlerts_{false};
    bool showScanner_{false};
    bool showPineEditor_{false};
    bool showTradeHistory_{false};

    // H1 — Order Management state
    int    omSideIdx_{0};           // 0=BUY, 1=SELL
    int    omTypeIdx_{0};           // 0=MARKET, 1=LIMIT, 2=STOP_LIMIT
    double omQuantity_{0.001};
    double omPrice_{0.0};
    double omStopPrice_{0.0};
    bool   omReduceOnly_{false};
    bool   omShowConfirm_{false};
    int    omLeverage_{1};          // Leverage for futures orders (1x-125x)
    std::string omValidationError_;

    // Chart right-click order placement
    bool   chartRightClickOpen_{false};
    double chartRightClickPrice_{0.0};

    // H1 — Managed positions for TP/SL popup
    bool   omShowTPSL_{false};
    int    omTPSLIdx_{-1};
    double omNewTP_{0.0};
    double omNewSL_{0.0};

    // H2 — Paper Trading
    PaperTrading paperTrader_{10000.0};
    bool paperResetConfirm_{false};

    // M1 — Backtest state
    char   btSymbol_[64]{"BTCUSDT"};
    int    btIntervalIdx_{0};
    double btBalance_{10000.0};
    double btCommission_{0.1};
    char   btStartDate_[16]{"2024-01-01"};
    char   btEndDate_[16]{"2024-12-31"};
    bool   btRunning_{false};
    BacktestEngine::Result btResult_;
    bool   btAIMode_{false};            // AI/ML adaptive optimization mode
    int    btRunCount_{0};              // number of adaptive runs in current session
    int    btFastPeriod_{9};            // current EMA fast period
    int    btSlowPeriod_{21};           // current EMA slow period

    // M3 — Alerts state
    AlertManager alertManager_;
    char   alertSymbol_[64]{};
    int    alertCondIdx_{0};
    double alertThreshold_{0.0};
    int    alertNotifyIdx_{0};

    // L1 — Scanner state
    MarketScanner scanner_;

    // L3 — Pine Editor state
    char pineCode_[500001]{};
    std::string pineEditorError_;

    // Trade History
    TradeHistory tradeHistory_;

    // Trade markers manager (H3)
    TradeMarkerManager tradeMarkerMgr_;

    // Equity curve history (M2)
    std::deque<double> equityCurveValues_;
    std::deque<double> equityCurveTsMs_;

    // Layout manager for fixed windows
    LayoutManager layoutMgr_;

    // Layout reset flag: when true, force window positions to defaults on next frame
    bool layoutNeedsReset_{true};

    // Auto-resize: track previous viewport size to detect fullscreen/windowed toggle
    float prevViewportW_{0.0f};
    float prevViewportH_{0.0f};

    // Layout .ini persistence
    std::string layoutIniPath_;
    void loadLayoutIni(const std::string& path);
    void saveLayoutIni(const std::string& path) const;

    // Volume Delta zoom/pan state
    float vdZoomX_{1.0f};
    float vdZoomY_{1.0f};
    float vdPanX_{0.0f};
    float vdPanY_{0.0f};

    // Indicators zoom/pan state
    float indZoomX_{1.0f};
    float indZoomY_{1.0f};
    float indPanX_{0.0f};
    float indPanY_{0.0f};

    // Database repository pointers (non-owning, set by main.cpp)
    BacktestRepository* btRepo_{nullptr};
    DrawingRepository*  drawRepo_{nullptr};
};

} // namespace crypto
