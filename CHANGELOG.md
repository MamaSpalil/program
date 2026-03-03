# Changelog

All notable changes to the CryptoTrader project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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

[1.0.0]: https://github.com/MamaSpalil/program/releases/tag/v1.0.0
