# Changelog

All notable changes to the CryptoTrader project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.6.0] - 2026-03-XX

### Added
#### AI & Machine Learning Engine (LibTorch)
- **RLTradingAgent** (`src/ai/RLTradingAgent.h/.cpp`) — интеграция PyTorch C++ API (LibTorch) для запуска нейронных сетей в рантайме. Имплементация Continuous Reinforcement Learning (PPO) с вычислением GAE (Generalized Advantage Estimation) через тензорные операции.
- **Dual-Mode Logic** — внедрены два режима ИИ: `Fast Mode` (сверхбыстрый анализ L2 Orderbook напрямую из `DepthStreamManager`) и `Swing Mode` (анализ трендов и MTF матриц на базе `CandleCache`).
- **FeatureExtractor** (`src/ai/FeatureExtractor.h/.cpp`) — высокопроизводительный модуль формирования вектора состояний без динамических аллокаций. Встроена жесткая математическая логика детектирования Smart Money Concepts: Swing Failure Patterns (SFP) и Fair Value Gaps (FVG).
- **OnlineLearningLoop** (`src/ai/OnlineLearningLoop.h/.cpp`) — выделенный фоновый поток (`std::jthread`), реализующий lock-free буфер реплея (Replay Buffer). Автоматически забирает результаты сделок (PnL) из `TradeRepository` для формирования функции Reward и обновления весов градиентным спуском (AdamW) без блокировки основного торгового потока.

### Changed
- **CMakeLists.txt** — добавлена интеграция `find_package(Torch REQUIRED)` и линковка тензорных библиотек к `crypto_lib`. Обновлены таргеты для поддержки аппаратного ускорения (CUDA/TensorRT) при наличии.

### Fixed
- **AppGui.cpp** — добавлены отсутствующие `#include <windows.h>` и `#include <GLFW/glfw3native.h>` в блоке `#ifdef _WIN32` для корректной компиляции Win32 API вызовов (`HICON`, `HWND`, `LoadImageA`, `SendMessageA`, `glfwGetWin32Window`). Устраняет ~30 ошибок компиляции C2065/C2146/C3861 на MSVC.

## [1.5.3] - 2026-03-XX

### Added

- **Иконка VT** (`resources/app_icon.ico`) —
  новая иконка "Virtual Trade System" с буквами VT
  и мини-графиком свечей; 5 размеров (16/32/48/
  64/256px) в одном .ico; встроена в .exe через
  resources/app_resource.rc; установлена в тайтлбар
  GLFW через glfwSetWindowIcon(); заголовок окна
  изменён на "VT — Virtual Trade System";
  скрипт генерации tools/generate_icon.py (Pillow)

- **Полный набор системных тестов**
  (`tests/test_full_system.cpp`) — 75+ тестов
  охватывающих все модули: инфраструктура и сборка
  (отсутствие linux-debug пресета, наличие glfw3
  в vcpkg.json), CandleCache (store/load/upsert/
  latestOpenTime), TradingDatabase все 12+1 таблиц
  (WAL, FK, индексы), все 8 репозиториев
  (Trade/Order/Position/Equity/Alert/Backtest/
  Drawing/Aux), DatabaseMigrator _meta таблица,
  индикаторы EMA/RSI/ATR/BB/VWAP/Pearson,
  RiskManager и DailyLossGuard, PaperTrading,
  OrderManagement, TradeHistory, BacktestEngine,
  AlertManager, Binance kline парсинг и safeStod,
  ExchangeEndpointManager (geoblock/round-robin),
  LayoutManager 3-колоночный layout v1.5.2
  (showUserPanel_ default=true, все 8 окон),
  SortState три состояния, форматы Pair List,
  Config save/load (atomicWrite, layoutProps,
  imgui.ini путь), Scheduler cron, GridBot
  уровни, WebhookServer (auth/rateLimit/IP),
  KeyVault, TaxReporter FIFO/LIFO, MarketScanner,
  CSVExporter, Volume Delta log1p нормализация;
  Windows 10 Pro совместимость: GetTempPathA,
  std::filesystem, gmtime_s, нет POSIX-only API

### Fixed

- **CMakePresets.json** — удалена конфигурация
  "linux-debug" с генератором "Unix Makefiles"
  который вызывал ошибку "CMAKE_MAKE_PROGRAM
  is not set" на Windows 10; оставлены только
  vs2019-x64-release и vs2019-x64-debug

### Changed

- **CMakeLists.txt** — добавлены:
  resources/app_resource.rc в WIN32 sources,
  tests/test_full_system.cpp в тест-исполняемый;
  target_sources условный WIN32 блок для .rc

- **Заголовок окна** — изменён с "Crypto ML Trader"
  на "VT — Virtual Trade System"

### Tests

- **75+ новых тестов** в test_full_system.cpp;
  покрытие всех модулей начиная с v1.0.0 до v1.5.2;
  подтверждена совместимость Windows 10 Pro
  MSVC VS2019 x64; время выполнения < 30 секунд;
  все предыдущие тесты (test_regression_part1/2/3,
  test_modules, test_trading_db, test_layout_and_ui)
  продолжают проходить без изменений

## [1.5.2] - 2026-03-05

### Fixed

- **GUI window not appearing** — added `glfw3` to `vcpkg.json` dependencies so GLFW is installed via vcpkg on Windows; without it `find_package(glfw3)` failed silently, `USE_IMGUI` was not defined, and the program compiled in console-only mode (no GUI window)
- **AppGui::init()** — added `glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE)` and diagnostic `fprintf` messages for `glfwInit()` / `glfwCreateWindow()` failures to aid debugging

### Added

#### Restored All 8 Mandatory UI Panels (Group 0)

- **"Main Toolbar"** window — standalone `ImGui::Begin("Main Toolbar")` at (0,0), full screen width × 32px; contains menu bar (File/View/Help) and toolbar buttons (Connect, Start Trading, Settings, Order Book, User Panel, mode indicator, pair selector, market type)
- **"Filters Bar"** window — standalone `ImGui::Begin("Filters Bar")` at (0,32), full screen width × 28px; contains filter controls (min volume, min/max price, min/max change%)
- **"Pair List"** — positioned via LayoutManager at (0,60), 210 × centerH; gated by `showPairList_` flag (default `true`)
- **"Volume Delta"** — positioned via LayoutManager at (210,60), centerW × centerH\*0.15; gated by `showVolumeDelta_` flag (default `true`)
- **"Market Data"** — positioned via LayoutManager at (210,volY), centerW × centerH\*0.65
- **"Indicators"** — positioned via LayoutManager at (210,indY), centerW × centerH\*0.20; gated by `showIndicators_` flag (default `true`)
- **"User Panel"** — positioned via LayoutManager at (screenW−290,60), 290 × centerH; `showUserPanel_` default changed from `false` to `true`
- **"Logs"** — positioned via LayoutManager at (0,screenH−120), screenW × 120; gated by `showLogs_` flag (default `true`)
- **View menu** — added toggle menu items for Pair List, Volume Delta, Indicators, and Logs panels

#### LayoutManager (Group 1)

- **`src/ui/LayoutManager.cpp`** — new file; `recalculate()` implementation moved from header to .cpp; produces layouts for all 8 mandatory windows using the 3-column layout (Pair List | center column | User Panel) with toolbar/filters at top and logs at bottom
- **`WindowLayout.visible`** field — added `bool visible{true}` to the `WindowLayout` struct stored in `std::map`
- **Center column proportions** — `vdPct_` (default 0.15), `indPct_` (default 0.20), Market Data gets remainder (0.65)

### Changed

- **`vcpkg.json`** — added `glfw3` dependency for Windows GUI builds
- **`CMakeLists.txt`** — added `src/ui/LayoutManager.cpp` to `UI_SOURCES`
- **`AppGui.h`** — added `showPairList_`, `showVolumeDelta_`, `showIndicators_`, `showLogs_` flags (all default `true`); changed `showUserPanel_` default to `true`; added `drawMainToolbarWindow()` and `drawFiltersBarWindow()` declarations
- **`AppGui.cpp` / `renderFrame()`** — replaced full-screen `##MainWin` background window with 8 individual positioned windows; all core panels gated by show flags
- **`LayoutManager.h`** — `recalculate()` moved to `.cpp`; `WindowLayout` struct gains `visible` field; default `vdPct_` changed from 0.13 to 0.15, `indPct_` from 0.25 to 0.20
- **LayoutManager tests** — updated `RecalculateCreatesLayouts` (checks all 8 windows), `WindowsDoNotOverlap` (new 3-column assertions), `WindowsAreFullWidth` (center column width = screenW−210−290), `GetDefaultsForUnknown` (checks `visible` field)

## [1.5.1] - 2026-03-05

### Added

#### SQLite Trading Database (config/trading.db)

- **TradingDatabase** (`src/data/TradingDatabase.h/.cpp`) — centralized SQLite3 database manager for `config/trading.db`; creates 12 tables (trades, orders, positions, equity_history, alerts, backtest_results, backtest_trades, drawings, funding_rates, scanner_cache, webhook_log, tax_events) with indices; WAL journal mode, synchronous=NORMAL, foreign_keys=ON, 32MB cache, temp_store=MEMORY; shared mutex for thread-safe repository access
- **TradeRepository** (`src/data/TradeRepository.h/.cpp`) — CRUD for trades table: upsert, close, getOpen, getHistory with symbol/limit filters, migrateFromJSON for `config/trade_history.json`
- **OrderRepository** (`src/data/OrderRepository.h/.cpp`) — CRUD for orders table: insert, updateStatus with fill info, getOpen (NEW/PARTIALLY_FILLED), getAll with pagination, deleteOld (prune completed orders older than N days)
- **PositionRepository** (`src/data/PositionRepository.h/.cpp`) — CRUD for positions table with UNIQUE(symbol, exchange, side, is_paper): upsert, updatePrice, updateTPSL, get, getAll, remove
- **EquityRepository** (`src/data/EquityRepository.h/.cpp`) — buffered writes for equity_history: push to in-memory buffer, flush to DB in batch transaction, getHistory, pruneOld (>365 days)
- **AlertRepository** (`src/data/AlertRepository.h/.cpp`) — CRUD for alerts: insert, update, remove, setTriggered (increment trigger_count), resetTriggered, getAll, getEnabled, migrateFromJSON for `config/alerts.json`
- **BacktestRepository** (`src/data/BacktestRepository.h/.cpp`) — save backtest results + trades in single transaction, getAll with symbol filter, getTrades, remove with CASCADE delete of backtest_trades
- **DrawingRepository** (`src/data/DrawingRepository.h/.cpp`) — CRUD for chart drawings: insert with RGBA color encoding, remove, update, clear by symbol/exchange, load by symbol/exchange
- **AuxRepository** (`src/data/AuxRepository.h/.cpp`) — four auxiliary tables in one class: funding_rates (save/get/prune), scanner_cache (upsert/get/clearStale), webhook_log (log/get/prune), tax_events (save/get/clear)
- **DatabaseMigrator** (`src/data/DatabaseMigrator.h/.cpp`) — one-time migration from JSON to SQLite: reads `config/trade_history.json` and `config/alerts.json`, inserts into DB, marks migration complete in `_meta` table; does not delete original JSON files
- **DatabaseMaintenance** (`src/data/DatabaseMaintenance.h/.cpp`) — background cleanup thread: runs every 24 hours; prunes equity (>365 days), funding rates (>30 days), webhook log (>500 entries), old orders (>30 days); WAL checkpoint

#### Database Integration into Application

- **main.cpp** — TradingDatabase initialized after Logger in both GUI and console modes; all 8 repositories created; DatabaseMigrator runs asynchronously in GUI mode (std::async); DatabaseMaintenance background thread started; equity flush and maintenance stop on shutdown
- **TradeHistory** — `setRepository(TradeRepository*)` enables parallel write to both JSON and SQLite on every `addTrade()` call
- **AlertManager** — `setRepository(AlertRepository*)` persists alerts to DB on add/remove/reset; `loadFromDB()` loads alerts from database at startup
- **PaperTrading** — `setPositionRepository(PositionRepository*)` persists paper positions to DB on open/close
- **BacktestEngine** — `setRepository(BacktestRepository*)` auto-saves backtest results and trade list to DB after `computeMetrics()`
- **DrawingManager** — `setRepository(DrawingRepository*)` and `setExchange()` persist drawings to DB on add/remove
- **WebhookServer** — `setAuxRepository(AuxRepository*)` logs webhook requests to DB after processing
- **MarketScanner** — `setAuxRepository(AuxRepository*)` caches scan results to DB on `updateResults()`
- **AppGui::setDatabaseRepositories()** — wires all repository pointers to GUI-owned module instances (TradeHistory, AlertManager, PaperTrading, BacktestEngine, DrawingManager)

#### Restored UI Windows

- **Pair List window** — standalone `ImGui::Begin("Pair List")` window (200x400, top-left); SPOT/FUTURES toggle, search filter, clickable pair list; always visible in render loop
- **User Panel window** — standalone `ImGui::Begin("User Panel")` window (280x500, top-right); current price, connection status, balance, open orders, positions, signal; toggled via View menu and toolbar button

#### Tests

- **8 new unit tests** in `test_trading_db.cpp` — IndicesCreated (verifies 14 indices), AlertSetAndResetTriggered (trigger count increment, reset), PositionClose (upsert then remove), TradeGetHistory (filter by symbol), ScannerCacheUpsert (scanner cache round-trip), MigrationFlagsTable (_meta table creation), BacktestSaveAndGetTrades (save with trades, verify counts), ForeignKeysEnabled (orphan FK rejection)

### Changed

- **AppGui.h** — added includes for all 7 repository headers; added `setDatabaseRepositories()` public method; added `btRepo_` and `drawRepo_` non-owning pointers
- **main.cpp** — added includes for all database layer headers; database initialization in both GUI and console modes
- **MarketScanner.h** — added `setAuxRepository()` setter and forward declaration for AuxRepository
- **MarketScanner.cpp** — `updateResults()` now persists scan results to AuxRepository if set

## [1.5.0] - 2026-03-04

### Added

#### Configurable Fixed Window Layout with imgui.ini Persistence

- **Standalone Logs window** — new `drawLogWindow()` renders logs as a full-width top-level ImGui window with vertical scroll; replaces inline log panel from center column; color-coded log lines (Error/SELL red, BUY/OK green, Config/Info blue, Warn yellow); auto-scroll to bottom on new entries
- **Full-width stacked layout** — four main windows arranged vertically: Logs (top) → Volume Delta → Market Data (tallest) → Indicators (bottom); all windows span full viewport width; no overlapping; gap spacing between windows
- **Layout tab in Settings** — new "Layout" tab in Settings panel with height proportion sliders for Logs, Volume Delta, and Indicators; Market Data automatically gets remaining space; lock/unlock toggle; "Apply Layout" and "Reset Layout to Defaults" buttons; saves configuration on apply
- **imgui.ini persistence** — window positions and sizes saved to `config/imgui.ini` via ImGui's built-in persistence; `lockWindowOnce()` uses `ImGuiCond_FirstUseEver` to respect saved positions; `lockWindow()` uses `ImGuiCond_Always` for forced reset; `SaveIniSettingsToDisk()` called on layout apply
- **Layout config in settings.json** — layout proportions (`log_pct`, `vd_pct`, `ind_pct`) and lock state persisted in settings.json under `"layout"` key; loaded on startup, saved on apply
- **Volume Delta zoom/pan** — mouse wheel to zoom X axis (Shift+wheel for Y axis); left-click drag to pan; clip rect prevents rendering outside canvas bounds; zoom info overlay shows current X/Y zoom percentages
- **Indicators zoom/pan** — mouse wheel to zoom X axis (Shift+wheel for Y axis); left-click drag to pan; scrollable child area with zoom-scaled virtual dimensions; zoom info bar at bottom
- **Standalone Order Book window** — Order Book panel converted from inline collapsible header to standalone ImGui::Begin() window; toggled via toolbar button; closeable via window title bar

#### Tests

- **test_regression_part3.cpp** — 5 new LayoutManager tests: `MarketDataIsTallest` verifies Market Data has the largest height; `WindowsAreFullWidth` checks all windows have equal full width; `CustomProportions` validates setter/getter for layout proportions; `LogsWindowExists` confirms Logs window is created; `HasLayoutFalseBeforeRecalculate` extended with Logs check; existing `RecalculateCreatesLayouts` and `WindowsDoNotOverlap` updated to include Logs window

### Changed

- **LayoutManager.h** — added `lockWindowOnce()` with `ImGuiCond_FirstUseEver` for imgui.ini compatibility; `recalculate()` now computes 4 windows (Logs, Volume Delta, Market Data, Indicators) at full viewport width; height proportions configurable via `setLogPct()`/`setVdPct()`/`setIndPct()` with getters; Market Data gets remaining space after other windows
- **AppGui.h** — added `drawLogWindow()` declaration; added layout config fields to `GuiConfig` (`layoutLogPct`, `layoutVdPct`, `layoutIndPct`, `layoutLocked`); added `layoutNeedsReset_` flag; added zoom/pan state for Volume Delta (`vdZoomX_`, `vdZoomY_`, `vdPanX_`, `vdPanY_`) and Indicators (`indZoomX_`, `indZoomY_`, `indPanX_`, `indPanY_`)
- **AppGui.cpp** — `init()` sets explicit `imgui.ini` path in config directory; `renderFrame()` simplified to menu bar + 4 stacked windows; removed three-column layout (PairList/Center/UserPanel); all windows use `lockWindowOnce()` by default, `lockWindow()` when locked or resetting; `configToJson()`/`loadConfig()` serialize/deserialize layout settings; `drawSettingsPanel()` includes Layout tab; `drawOrderBookPanel()` converted to standalone window

## [1.4.1] - 2026-03-04

### Added

#### Group 1 — Critical Fix: Geo-blocking Fallback (Balance $0.00)

- **BinanceExchange geo-blocking fallback** — `httpGet()` now detects HTTP 403/451 (geo-blocked) responses and automatically retries with alternative endpoints from `ExchangeEndpointManager`; round-robin through `binanceEndpoints()` (spot) or `binanceFuturesEndpoints()` (futures); logs warning with alternative endpoint URL; adds `replaceBase()` helper for URL domain substitution
- **Alt endpoint indicator in User Panel** — yellow "⚠ Alt endpoint" text displayed in User Panel connection status when an alternative endpoint is being used; `GuiState::usingAltEndpoint` flag propagated from `BinanceExchange::isUsingAltEndpoint()`
- **BinanceExchange public accessor** — `isUsingAltEndpoint()` method exposed for UI display

#### Group 2 — Fixed Windows with Scrollable Content (LayoutManager)

- **LayoutManager** (`src/ui/LayoutManager.h`) — header-only layout manager for fixed ImGui windows; `lockWindow()` applies `ImGuiCond_Always` position/size each frame; `lockedFlags()` returns NoMove|NoResize|NoCollapse|NoBringToFrontOnFocus; `lockedScrollFlags()` adds HorizontalScrollbar; `recalculate(screenW, screenH)` computes dynamic positions for Volume Delta, Market Data, and Indicators windows; `get(name)` returns layout by window name; `hasLayout(name)` checks existence; falls back to stub types in non-GUI test builds
- **Fixed window layout** — Volume Delta, Market Data, and Indicators windows are now locked (no move, no resize, no collapse) with content scrolling enabled; positions dynamically recalculated each frame based on screen dimensions

#### Tests

- **test_regression_part3.cpp** — 16 tests covering: EndpointManager isGeoBlocked (403, 451, 200, 429), round-robin spot endpoints, round-robin wrapping, futures endpoints existence, BinanceExchange alt endpoint default, LayoutManager locked flags, scroll flags, scroll-includes-locked, recalculate creates layouts, get defaults for unknown, windows do not overlap, different screen sizes, hasLayout before recalculate

### Changed

- **CMakeLists.txt** — added `tests/test_regression_part3.cpp` to test executable
- **BinanceExchange.h** — added `EndpointManager.h` include, geo-blocking state members (`spotEndpointIdx_`, `futuresEndpointIdx_`, `useAltEndpoint_`), `replaceBase()` helper, and `isUsingAltEndpoint()` public accessor
- **AppGui.h** — added `LayoutManager.h` include, `LayoutManager layoutMgr_` member, `GuiState::usingAltEndpoint` field
- **AppGui.cpp** — Volume Delta, Market Data, and Indicators windows now use `LayoutManager::lockWindow()` and locked flags instead of `ImGuiCond_FirstUseEver` positioning; layout recalculated each frame in `renderFrame()`

## [1.4.0] - 2026-03-04

### Added

#### Block 8 — Automation: Scheduler & Grid Bot

- **Scheduler** (`src/automation/Scheduler.h/.cpp`) — cron-based strategy scheduler with simple cron expression parser (minute/hour/day-of-month/month/day-of-week), supports wildcards (`*`), ranges (`1-5`), and comma-separated values; `addJob()`, `removeJob()`, `getJobs()`; persistent save/load to `config/scheduler.json`; background thread loop with 30-second check interval; thread-safe via `std::mutex`
- **GridBot** (`src/automation/GridBot.h/.cpp`) — grid trading bot with arithmetic and geometric level spacing; `configure()`, `start()`, `stop()`, `onOrderFilled()`; `levelCount()`, `levelAt()`, `levels()` accessors; tracks `realizedProfit()` and `filledOrders()`; thread-safe via `std::mutex`

#### Block 9 — External Data: News Feed & Fear/Greed

- **NewsFeed** (`src/external/NewsFeed.h/.cpp`) — news aggregator with `NewsItem` struct (title, source, url, pubTime, sentiment); background polling thread with 5-minute interval; `addItem()`, `getItems()`, `itemCount()`, `clear()`; max 200 items retained; thread-safe via `std::mutex`
- **FearGreed** (`src/external/FearGreed.h/.cpp`) — Fear & Greed index data source with `FearGreedData` struct (value 0-100, classification, timestamp); background polling thread with 1-hour interval; `getData()`, `setData()` accessors; thread-safe via `std::mutex`

#### Block 10 — Webhook Server (TradingView Integration)

- **WebhookServer** (`src/integration/WebhookServer.h/.cpp`) — incoming webhook handler with secret-based authentication; per-IP rate limiting (10 requests/minute per IP); `simulateRequest()` for testing; parses JSON body with action/symbol/qty/price; stores recent signals; `setSecret()`, `getSecret()`, `start()`, `stop()`, `getRecentSignals()`; thread-safe via `std::mutex`

#### Block 11 — Telegram Bot (Bidirectional)

- **TelegramBot** (`src/integration/TelegramBot.h/.cpp`) — bidirectional Telegram bot with long polling; authorized chat ID security; commands: `/help`, `/balance`, `/status`, `/positions`, `/pnl`, `/buy`, `/sell`, `/stop`, `/start`; `processCommand()` public for testing; `startPolling()`, `stopPolling()`, `sendMessage()`; thread-safe via `std::mutex`

#### Block 12 — Tax Report (FIFO/LIFO)

- **TaxReporter** (`src/tax/TaxReporter.h/.cpp`) — tax calculation engine supporting FIFO and LIFO lot matching methods; groups trades by symbol; filters by tax year; calculates proceeds, cost basis, gain/loss per lot; determines long-term vs short-term (365-day threshold); CSV export with `exportCSV()`

#### Part 1 — Bug Fixes & Core Modules

- **DrawingTools** (`src/ui/DrawingTools.h/.cpp`) — drawing object manager with add/remove/count/getAll/save/load; supports TREND_LINE, HORIZONTAL_LINE, FIBONACCI, RECTANGLE, TEXT_LABEL types; persists to JSON per symbol; thread-safe via `std::mutex`
- **MultiTimeframeWindow** (`src/ui/MultiTimeframe.h/.cpp`) — 4-pane multi-timeframe chart window (1m, 5m, 1h, 1d); `loadAll()`, `setPaneBars()`, `paneReady()`, `paneBarCount()`, `paneBars()` accessors; thread-safe via `std::mutex`
- **VWAPIndicator** (`src/indicators/VWAPIndicator.h`) — header-only VWAP (Volume-Weighted Average Price), TWAP (Time-Weighted Average Price), and Pearson correlation coefficient calculations
- **PositionCalc** (`src/strategy/PositionCalc.h`) — header-only position size calculator based on balance, risk percentage, entry price, and stop loss; includes `riskAmt()`, `priceDiff()`, `posSize()`
- **DailyLossGuard** (`src/strategy/PositionCalc.h`) — daily loss limit guard; `init()` with starting balance and limit percentage; `update()` with current balance; `isTriggered()` returns true when daily loss exceeds limit; `reset()` to clear trigger
- **DepthStreamManager** (`src/exchange/DepthStream.h/.cpp`) — depth (order book) WebSocket stream manager; `subscribe()`, `unsubscribe()`, `isConnected()`, `getBook()`, `setBook()` for testing; thread-safe via `std::mutex`
- **ExchangeEndpointManager** (`src/exchange/EndpointManager.h`) — header-only fallback endpoint manager for geo-blocking (403/451); provides Binance spot and futures alternative endpoints; round-robin `getNextEndpoint()` and `isGeoBlocked()` utilities

#### Tests

- **test_regression_part1.cpp** — 15 tests covering: topbar button text (no question marks), DepthStream connectivity, DrawingTools add/remove/save/load, MultiTimeframe pane loading, VWAP correctness, Pearson correlation (positive/negative/uncorrelated), PositionCalc sizing (including zero stop), DailyLossGuard trigger and reset
- **test_regression_part2.cpp** — 21 tests covering: Scheduler cron parsing (every minute, specific hour, specific minute+hour, add/remove jobs, save/load), GridBot levels (arithmetic, geometric, start/stop), Webhook security (wrong secret, correct secret, rate limiting, IP isolation), TelegramBot commands (help, unauthorized, status, available commands), TaxReporter FIFO/LIFO gain calculation and CSV export, NewsFeed add/get/max items, FearGreed set/get

### Fixed

- **Topbar "Order Book" button** — replaced Cyrillic `u8"Режим Стакан"` with ASCII `"Order Book"` to prevent question mark display (`?????`) when ImGui font lacks Cyrillic glyph ranges; also fixed Order Book collapsing header text

### Changed

- **CMakeLists.txt** — added new source groups (AUTOMATION_SOURCES, EXTERNAL_SOURCES, INTEGRATION_SOURCES, TAX_SOURCES, DRAWING_SOURCES, DEPTH_SOURCES) and test files (test_regression_part1.cpp, test_regression_part2.cpp) to the build

## [1.3.1] - 2026-03-04

### Fixed

- **`getFuturesBalance` "invalid stod argument" error** — added `safeStod()` helper function across all exchange implementations (Binance, Bybit, OKX, Bitget, KuCoin) and `HistoricalLoader` to safely parse empty or non-numeric JSON string values; returns `0.0` instead of throwing `std::invalid_argument`
- **"Price error" red text in User Panel** — price fetch now uses candle close price as immediate fallback; errors are logged at debug level instead of displayed in UI
- **User Panel not opening correctly** — the User Panel right column is now conditionally rendered only when `showUserPanel_` toggle is active; previously it was always visible regardless of the toggle state

### Changed

- **Chart tick update interval** — price polling reduced from 1-second intervals to 100ms for near-real-time price updates; candle close price is used as immediate source before API poll completes
- **Volume Delta — standalone resizable window** — converted from inline collapsible panel to a separate `ImGui::Begin("Volume Delta")` window with diagonal resize support and size constraints (min 200×80); visualization now uses logarithmic scale (`log1p`) normalization to prevent oversized bars when volume differences are large
- **Indicators — standalone resizable window** — converted from inline collapsible panel to a separate `ImGui::Begin("Indicators")` window with diagonal resize support and size constraints (min 300×100)
- **Market Data window position** — default initial position moved below Volume Delta window (y=190) to avoid overlap
- **Pine Script Editor improvements** — buffer size increased from 65536 to 100000 characters; "Run" button now saves script to file and validates/compiles via `PineConverter::parseSource()` with error feedback; "Load" button reads `custom_script.pine` from user indicator directory; character count display added; editor textarea dynamically fills available window height

## [1.3.0] - 2026-03-04

### Added

- **SQLite3 local candle cache** — `CandleCache` class (`src/data/CandleCache.h/.cpp`) using SQLite3 for persistent local storage of historical bars (candles) keyed by (exchange, symbol, interval, openTime); supports `store()` (batch upsert in a transaction), `load()` (sorted by openTime), `latestOpenTime()`, `count()`, `clear()`, `clearAll()`; WAL journal mode for concurrent read performance; thread-safe via `std::mutex`
- **Smart candle loading in Engine** — `Engine::run()` and `Engine::reloadCandles()` now load cached candles from the local SQLite DB first, then fetch only missing (newer) bars from the exchange API, merge and deduplicate by `openTime`, and persist the result back to cache; eliminates redundant API calls on restart and enables instant chart startup
- **SQLite3 dependency** — added `sqlite3` to `vcpkg.json`; `CMakeLists.txt` finds `unofficial-sqlite3` (vcpkg) or system `SQLite3` and links to `crypto_lib`

#### Tests

- **10 new unit tests** — `test_candle_cache.cpp` covering store/load round-trip, sorted output, upsert replace, key isolation (exchange/symbol/interval), latestOpenTime, count, clear (specific key), clearAll, empty store no-op, persistence across DB instances

### Changed

- **CMakeLists.txt** — added `src/data/CandleCache.cpp` to `DATA_SOURCES`; added `tests/test_candle_cache.cpp` to test executable; added SQLite3 `find_package` and `target_link_libraries`
- **`.gitignore`** — added `*.db` to exclude SQLite database files from version control

## [1.2.0] - 2026-03-04

### Added

#### HIGH PRIORITY — Trading Infrastructure

- **H1 — Order Management UI** — new `ImGui::Begin("Order Management")` window (420×520) with BUY/SELL side buttons, MARKET/LIMIT/STOP_LIMIT type selector, quantity/price/stop-price inputs, reduce-only checkbox (futures), estimated cost display, and confirmation popup before order submission; includes open positions table with per-position Close button and TP/SL editing; validates orders through `RiskManager::canTrade()` with inline error display (`src/trading/OrderManagement.h/.cpp`)
- **H2 — Paper Trading Panel** — new `ImGui::Begin("Paper Trading")` window showing virtual balance, equity, total PnL (%), max drawdown; open positions table (pair/qty/entry/current/pnl); reset button with confirmation popup; state persisted to `config/paper_account.json`; `PaperTrading` class with `openPosition()`, `closePosition()`, `updatePrices()`, `reset()`, `saveToFile()`, `loadFromFile()` (`src/trading/PaperTrading.h/.cpp`)
- **H2 — Paper/Live mode indicator** — colored toolbar button: yellow `PAPER TRADING` when `mode == "paper"`, red `LIVE TRADING` otherwise
- **H3 — Trade markers on chart** — entry markers (green triangle-up for BUY, red triangle-down for SELL), exit markers (white X with PnL tag), and TP/SL horizontal lines rendered via `ImDrawList` in the candlestick chart; managed by `TradeMarkerManager` with thread-safe add/clear/getMarkers/setTPSL/clearTPSL (`src/trading/TradeMarker.h/.cpp`)
- **H4 — AES-256-GCM key encryption** — `KeyVault` class using PBKDF2-HMAC-SHA256 (100 000 iterations, 16-byte salt) for key derivation and AES-256-GCM for authenticated encryption; provides `encrypt()`/`decrypt()`, hex convenience wrappers, and `generateSalt()`; wire format: salt(16) + iv(12) + tag(16) + ciphertext (`src/security/KeyVault.h/.cpp`)

#### MEDIUM PRIORITY — Analytics & Alerting

- **Trade History storage** — `TradeHistory` class with `HistoricalTrade` struct (id, symbol, side, type, qty, entry/exit price, pnl, commission, entry/exit time, isPaper flag); CRUD operations, filtering by symbol/date/paper-vs-live, statistics (winRate, avgWin, avgLoss, profitFactor, maxDrawdown); JSON persistence (`src/trading/TradeHistory.h/.cpp`)
- **Trade History & Analytics window** — new ImGui window showing aggregate statistics, equity mini-chart, and scrollable trades table with date/pair/side/entry/exit/pnl columns and CSV export button
- **M1 — Backtesting engine** — `BacktestEngine` class with configurable symbol, interval, initial balance, commission; runs EMA-crossover strategy or custom `SignalFunc`; computes Sharpe ratio (`avgReturn / stdReturn * sqrt(252)`), max drawdown, profit factor, win rate, avg win/loss; produces equity curve and trade list (`src/backtest/BacktestEngine.h/.cpp`)
- **M1 — Backtesting window** — new `ImGui::Begin("Backtesting")` window (700×600) with symbol/timeframe/date-range/balance/commission inputs, "Run Backtest" button, results summary (PnL, trades, drawdown, Sharpe, PF), equity curve `PlotLines`, and scrollable trades table with CSV export
- **M2 — Equity Curve** — `GuiState` now carries `equityCurveData` and `equityCurveTimes` for live equity visualization; equity mini-chart rendered in Trade History window
- **M3 — Alerts & Notifications** — `AlertManager` class with alert CRUD, condition checking (RSI_ABOVE/BELOW, PRICE_ABOVE/BELOW, EMA_CROSS_UP/DOWN), Telegram notifications via libcurl (`curl_easy_escape` for URL encoding), Windows sound via `MessageBeep`, UI notification callback, JSON persistence to `config/alerts.json` (`src/alerts/AlertManager.h/.cpp`)
- **M3 — Alerts window** — new ImGui window with new-alert form (symbol, condition, threshold, notify type), alert list with triggered/active indicators, reset/delete buttons

#### LOW PRIORITY — Scanner, Export, Pine Script

- **L1 — Multi-pair Market Scanner** — `MarketScanner` class with thread-safe `updateResults()`/`getResults()`, sort by field, volume filter (`src/scanner/MarketScanner.h/.cpp`)
- **L1 — Market Scanner window** — new `ImGui::Begin("Market Scanner")` window (800×500) with sortable 7-column table (Pair, Price, 24h%, Volume, RSI, Signal, Conf%); color-coded values; click to switch pair
- **L2 — CSV Export** — `CSVExporter` class with static methods `exportBars()`, `exportTrades()`, `exportBacktestTrades()`, `exportEquityCurve()`; cross-platform time formatting (`gmtime_r`/`gmtime_s`) (`src/data/CSVExporter.h/.cpp`)
- **L2 — Export buttons** — "Export Bars" button in Market Data toolbar; "Export CSV" buttons in Backtesting and Trade History windows
- **L3 — Pine Script Editor** — new `ImGui::Begin("Pine Script Editor")` window with `InputTextMultiline` (65536 chars, tab-input), Run/Save As/Load buttons, error display
- **L4 — Pine Visuals** — `PineVisuals` class with `PlotShapeMarker` (time, price, shape, color, text) and `BgColorBar` (time, color) structs; thread-safe add/clear/get (`src/indicators/PineVisuals.h/.cpp`)
- **M4 — Depth of Market** — `DepthLevel::isWall` support added to `OrderBook` data path through existing `getOrderBook()` infrastructure

#### Tests

- **47 new unit tests** — `test_modules.cpp` covering OrderManagement (7 tests: validation, cost estimation, close order, exchange conversion), PaperTrading (6 tests: balance, positions, close, reset, price updates), TradeMarker (3 tests: CRUD, TP/SL), KeyVault (5 tests: encrypt/decrypt round-trip, wrong password, hex, salt), TradeHistory (5 tests: stats, filters, profit factor), BacktestEngine (3 tests: empty, trending, custom signal), AlertManager (5 tests: CRUD, trigger, reset), MarketScanner (2 tests: update, volume filter), PineVisuals (2 tests: shapes, clear)

#### Chart Interactivity & Rendering (Part 2)

- **OHLCV tooltip on hover** — hovering over the candlestick chart now shows a tooltip with the nearest bar's Open, High, Low, Close, Volume, and formatted date/time (`%Y-%m-%d %H:%M`)
- **Time label under crosshair** — a formatted `HH:MM` time label appears at the bottom of the chart, following the cursor X position and snapping to the nearest visible bar
- **Connection/WS status indicator** — colored status badge in the Market Data toolbar: green `● LIVE` (connected + trading), yellow `● IDLE` (connected, not trading), grey `● OFFLINE` (disconnected)
- **Nearest-bar crosshair** — crosshair now finds the nearest visible bar to the cursor position for accurate data display in tooltip and time label

#### Tests

- **3 new unit tests** — `BarTimeIsMilliseconds` (validates ms→s conversion gives a reasonable year), `ParseBarsMultipleBarsInOrder` (verifies chronological order), `BarPriceRangeValid` (asserts low ≤ open,close ≤ high and non-negative volume)

### Fixed

- **Candlestick chart not rendering** — candle bars in "Market Data" window were invisible despite loaded data (Price/RSI/EMA9 showed correct values):
  - Added minimum window size guard (100×100 px) with user-visible warning when area is too small
  - Added empty-bars guard with "Waiting…" message
  - Added Y-axis range validation (`pMin >= pMax` / non-finite) to prevent draw calls with degenerate coordinates
  - Added spdlog diagnostic logs (`bars_.size()`, `plotSize`, first/last bar data, `axisYmin`/`axisYmax`) for runtime debugging
- **Diagnostic log spam** — per-frame debug logging (8 `Logger::debug()` calls × 60 fps = 480 calls/sec) replaced with a single consolidated log call every ~5 seconds (300 frames), preventing FPS degradation and log file flooding
- **Thread-unsafe `localtime()` in crosshair** — replaced `localtime()` with thread-safe `localtime_r()` (POSIX) / `localtime_s()` (Windows) for bar time formatting in crosshair tooltip
- **Binance futures testnet URL** — `BinanceExchange` constructor now auto-detects testnet mode from `baseUrl_` and sets `futuresBaseUrl_` to `https://testnet.binancefuture.com` (was hardcoded to production `https://fapi.binance.com`, causing HTTP 403 CloudFront blocks on testnet connections)
- **Binance futures API paths** — `getOrderBook()` now uses `/fapi/v1/depth` when `marketType_ == "futures"` (was `/api/v3/depth`); `getOpenOrders()` uses `/fapi/v1/openOrders` (was `/api/v3/openOrders`); `getMyTrades()` uses `/fapi/v1/userTrades` (was `/api/v3/myTrades`)
- **Binance `getFuturesBalance` crash** — added response format validation (`j.is_object()` + `j.contains("totalWalletBalance")`) before `std::stod()` calls to prevent `invalid stod argument` exception when API returns error JSON or HTML
- **Binance `getPositionRisk` crash** — added `j.is_array()` validation before iterating position risk response

### Changed

- **View menu** — expanded with entries for Order Management, Paper Trading, Backtesting, Alerts, Market Scanner, Pine Editor, and Trade History windows
- **Toolbar** — added Paper/Live mode indicator button between User Panel toggle and pair selector
- **CMakeLists.txt** — added 10 new source file groups (TRADING, SECURITY, BACKTEST, ALERT, SCANNER, PINEVIZ, CSVEXPORT) to `ALL_LIB_SOURCES`; added `tests/test_modules.cpp` to test executable

## [1.1.0] - 2026-03-04

### Added

- **Standalone "Market Data" window** — candlestick chart moved from center-column `CollapsingHeader` into its own `ImGui::Begin("Market Data")` window with `NoScrollbar` / `NoScrollWithMouse` flags (PR #15)
- **Window size & position** — initial size 900×600, position (220, 30), min constraints 400×300; all set via `ImGuiCond_FirstUseEver` so the user can freely resize and ImGui remembers the layout in `imgui.ini` (PR #15)
- **Auto-fit to 50 bars** — `needsChartReset_` flag dynamically computes bar width to show the last 50 candles on first load, pair change, or timeframe change (PR #15)
- **Mouse zoom & pan** — mouse wheel zooms the X axis (bar width 2–30 px); left-mouse drag pans horizontally; removed old Ctrl+wheel / plain wheel split (PR #15)
- **"Reset View" / "Fit All" buttons** — toolbar buttons above the chart: "Reset View" fits last 50 bars, "Fit All" scales to the entire candle history (PR #15)
- **EMA9 overlay** — yellow polyline drawn over the price candles, computed locally via `calcEMAFromCandles()` (PR #15)
- **RSI(14) subplot** — dedicated subplot below volume (layout: candles 65 %, volume 20 %, RSI 15 %) with 30/50/70 horizontal level lines and right-side labels; computed via `calcRSIFromCandles()` (PR #15)
- **Signal panel** — single-row info bar above the chart showing Price, RSI, EMA9, Signal (BUY/SELL/HOLD color-coded), and Confidence % (PR #15)
- **Crosshair with price label** — vertical line across full canvas height; horizontal line limited to the price zone; price tag rendered in the scale area at cursor Y position (PR #15)

### Changed

- **Chart layout proportions** — price section reduced from 75 % → 65 %, volume from 25 % → 20 %, to make room for the new RSI subplot (15 %) (PR #15)
- **Y-axis padding** — increased from 2 % to 5 % and extracted into named constant `kPricePadding` for readability (PR #15)
- **`drawMarketPanel()` / `drawCandlestickChart()` removed** — both old methods replaced by a single `drawMarketDataWindow()` method combining market info, toolbar, chart, and indicators (PR #15)

### Fixed

- **ImGui Begin/End balance** — early return for empty candle history now exits before `BeginChild` to avoid assertion failures from unbalanced calls (PR #15)
- **Scroll offset on timeframe change** — `chartScrollOffset_` is now reset to 0 together with `needsChartReset_` when switching timeframes, preventing stale scroll positions (PR #15)
- **Binance klines limit parameter** — `BinanceExchange::getKlines()` now clamps `limit` to the API maximum (1000 for spot, 1500 for futures) to prevent HTTP 400 errors that caused the "Market Data" window to fail loading historical bars (PR #16)

## [1.0.0] - 2026-03-03

### Added

- **Core trading engine** — C++17 algorithmic trading system with `Engine` orchestration, `EventBus` (type-erased pub/sub via `std::any`), and `Logger` (spdlog rotating 10 MB × 7 files) (PR #1)
- **Binance exchange** — REST API client with HMAC-SHA256 signing, rate limiting, and exponential-backoff retry (PR #1)
- **Bybit exchange** — REST API client with the same signing and retry logic (PR #1)
- **WebSocket client** — Boost.Beast SSL with auto-reconnect and heartbeat for real-time kline, depth, and trade streams (PR #1)
- **Technical indicators** — EMA, RSI (Wilder smoothing), ATR, MACD, Bollinger Bands; all incremental O(1) updates with circular buffer history (PR #1)
- **PineScriptIndicator** — composite indicator combining MACD + Bollinger Bands + EMA crossover signals with configurable long/short conditions (PR #1)
- **ML ensemble** — `LSTMModel` (LibTorch 2-layer LSTM), `XGBoostModel` (multi:softprob 3-class), `SignalEnhancer` (weighted ensemble), `ModelTrainer` (walk-forward 90-day retraining) (PR #1)
- **Feature engineering** — `FeatureExtractor` producing ~60-dim feature vector: log returns, rolling volatility, EMA deviations, MACD/BB/RSI values, VWAP, OBV, cyclical time encoding (PR #1)
- **Strategy** — `MLEnhancedStrategy` requiring indicator + ML agreement for entry, with immediate SL/TP, ML reversal exits, and trailing ATR stop (PR #1)
- **Risk management** — `RiskManager` with fixed-fraction + Kelly sizing, ATR-based SL (1.5×), 1:2 R:R enforcement, max drawdown circuit-breaker, and open-position cap (PR #1)
- **Console UI** — ncurses real-time dashboard with text fallback (PR #1)
- **Unit tests** — GTest suites for indicators, feature extractor, and risk manager (PR #1)
- **CMakeLists.txt** — full build with required (OpenSSL, Boost.Beast, nlohmann/json, spdlog) and optional (libcurl, LibTorch, XGBoost, Eigen3, ncurses, GTest) dependencies (PR #1)
- **config/settings.json** — runtime configuration: exchange keys, trading params, indicator periods, ML hyperparams, risk limits; defaults to testnet + paper trading (PR #1)
- **Windows build support** — CMake cross-platform fixes (MSVC flags, `_WIN32` guards), GitHub Actions CI workflow producing downloadable Windows x64 artifacts (PR #2)
- **vcpkg manifest** — `vcpkg.json` with dependency manifest (curl, openssl, boost, nlohmann-json, spdlog) and CMake presets for Visual Studio 2019 (PR #3)
- **ImGui GUI** — Dear ImGui v1.90.4 windowed GUI with GLFW+OpenGL3 backend and dark metal theme; console mode fallback when GUI deps are unavailable (PR #9)
- **OKX exchange** — REST `/api/v5/market/*`, WebSocket `/ws/v5/public`, base64-encoded HMAC-SHA256, passphrase auth (PR #10)
- **Bitget exchange** — REST `/api/v2/mix/market/*`, WebSocket `/v2/ws/public`, same signing scheme (PR #10)
- **KuCoin exchange** — REST `/api/v1/market/*`, WebSocket `/ws/v1/public`, signed passphrase (`KC-API-KEY-VERSION: 2`) (PR #10)
- **Order book visualization** — toggled via toolbar; renders 15 levels of bids/asks with volume-proportional bars and spread display (PR #10)
- **Volume delta bars** — custom `ImDrawList` bar chart approximating buy/sell delta per candle (PR #10)
- **Candlestick chart** — drawn via `ImDrawList` with wicks, bodies, and a 5-step price grid (PR #10)
- **Indicator selection UI** — checkboxes for EMA/RSI/ATR/MACD/BB with max-5 enforcement (PR #10)
- **Market filters** — inline filter bar for min volume, price range, and % change bounds (PR #10)
- **Extended timeframes** — expanded from 9 to 39: `1m..55m`, `1h..22h`, `1d..6d`, `1w..3w`, `1M..3M` (PR #10)
- **Market type toggle** — spot/futures switch in toolbar and settings (PR #10)
- **Passphrase field** — added to exchange settings for OKX/Bitget/KuCoin auth (PR #10)
- **API connection verification** — `IExchange::testConnection()` method performing a lightweight `getPrice` call across all 5 exchanges (PR #13)
- **Debug mode** — `Logger::setDebugMode(bool)` toggles console debug output; enabled via `"debug": true` in `config/settings.json` (PR #13)
- **Multi-exchange profile management** — `ExchangeDB` (JSON persistence in `config/exchanges.json`) with CRUD operations and active profile switching (PR #13)
- **PineConverter parser enhancements** — `strategy()` support, typed variable declarations (`int`/`float`/`string`/`bool`), `name(params) => expr` function definitions, and `type` block skipping (PR #14)
- **Resilient Pine Script parsing** — per-statement exception handling that skips to the next line on failure instead of aborting the whole script (PR #14)
- **PineRuntime new builtins** — `math.round/ceil/floor/pow/sign/avg`, `ta.stdev`, `ta.change`, `input.string`, `input.bool`, `bar_index`, and namespace handlers for `str.*`, `strategy.*`, `color.*` (PR #14)
- **User indicator GUI integration** — `GuiState::userIndicatorPlots` carries Pine Script plot results to the render thread; indicators panel shows values under "User Indicators (Pine Script)"; candlestick chart overlays user indicator values as horizontal reference lines (PR #14)
- **Pine Script v260 integration** — both `user_indicator/Script_v260.pine` files now load successfully (~3 119 statements each) (PR #14)

### Fixed

- **CMakePresets.json version** — downgraded from version 3 to version 2 for VS2019 compatibility (CMake 3.20) (PR #4)
- **CMake binaryDir conflicts** — each preset now uses a unique `binaryDir` (e.g. `build/vs2019-x64-release`, `build/linux-debug`) to prevent generator cache conflicts (PR #7)
- **MSVC `std::tolower` truncation** — cast `int` → `char` to silence warning (PR #8)
- **MSVC undeclared `M_PI`** — added `_USE_MATH_DEFINES` (PR #8)
- **MSVC missing `<algorithm>` header** — added include (PR #8)
- **MSVC `gmtime_r` compilation error** — replaced POSIX-only `gmtime_r` with cross-platform `#ifdef _WIN32` guard using `gmtime_s` (reversed parameter order) (PR #11)
- **Config path resolution** — config loading no longer fails with relative paths when the executable runs from the build directory (PR #11)
- **Logger duplicate registration** — added `if (logger_) return;` idempotency guard in `Logger::init()` since both `main()` and `Engine::Engine()` call it (PR #12)
- **UTF-8 support** — added locale/codepage setup for correct display of non-ASCII characters on Windows (PR #12)
- **Undefined behavior and race conditions** — fixed UB and data races in concurrent code paths (PR #12)
- **WebSocket SSL/TLS handshake failure** — set SNI hostname via `SSL_set_tlsext_host_name()` before handshake; switched context from `tlsv12_client` → `tls_client` with `verify_peer` (PR #14)

### Changed

- **CMakeLists.txt** — added MSVC-specific compiler flags, `_WIN32` guards, and `vcpkg` toolchain integration (PR #2, #5)
- **Pine Script auto-conversion** — auto-loads `.pine` files from `user_indicator/` directory at runtime via `UserIndicatorManager` (PR #8)

[1.3.0]: https://github.com/MamaSpalil/program/compare/v1.2.0...v1.3.0
[1.2.0]: https://github.com/MamaSpalil/program/compare/v1.1.0...v1.2.0
[1.1.0]: https://github.com/MamaSpalil/program/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/MamaSpalil/program/releases/tag/v1.0.0
