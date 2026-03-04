#pragma once

#include "../data/CandleData.h"
#include "../exchange/IExchange.h"
#include "../data/ExchangeDB.h"
#include "../indicators/IndicatorEngine.h"
#include "../strategy/RiskManager.h"
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

    void setConnectCallback(ConnectCallback cb)       { onConnect_ = std::move(cb); }
    void setDisconnectCallback(DisconnectCallback cb)  { onDisconnect_ = std::move(cb); }
    void setStartCallback(StartCallback cb)            { onStart_ = std::move(cb); }
    void setStopCallback(StopCallback cb)              { onStop_ = std::move(cb); }
    void setSaveConfigCallback(SaveConfigCallback cb)  { onSaveConfig_ = std::move(cb); }
    void setOrderCallback(OrderCallback cb)            { onOrder_ = std::move(cb); }
    void setLoadPairsCallback(LoadPairsCallback cb)    { onLoadPairs_ = std::move(cb); }
    void setRefreshDataCallback(RefreshDataCallback cb) { onRefreshData_ = std::move(cb); }

    // Add a log line (thread-safe)
    void addLog(const std::string& line);

    // Save current config to JSON file
    void saveConfigToFile(const std::string& path);

    bool shouldClose() const { return shouldClose_; }

private:
    void setupTheme();
    void renderFrame();

    // UI Panels
    void drawMenuBar();
    void drawToolbar();
    void drawMarketDataWindow();  // combined Market Data window with chart
    void drawVolumeDeltaPanel();
    void drawOrderBookPanel();
    void drawIndicatorsPanel();
    void drawSignalPanel();
    void drawTradingPanel();
    void drawPortfolioPanel();
    void drawSettingsPanel();
    void drawLogPanel();
    void drawStatusBar();
    void drawFilterPanel();
    void drawUserIndicatorDashboard();
    void drawUserPanel();
    void drawPairSelector();
    void drawPairListPanel();

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
    bool showUserPanel_{false};  // User panel (separate window)

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
};

} // namespace crypto
