# Changelog

All notable changes to the CryptoTrader project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
