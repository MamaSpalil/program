# Crypto ML Trader

Production-ready C++17/20 algorithmic trading system with ML-enhanced signals.

## Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                         CryptoTrader System                          │
│                                                                       │
│  ┌───────────┐    ┌──────────────┐    ┌──────────────────────────┐  │
│  │  Exchange  │    │   DataFeed   │    │     IndicatorEngine      │  │
│  │ (Binance / │───▶│  (candle     │───▶│  EMA / RSI / ATR /      │  │
│  │  Bybit)    │    │   history)   │    │  MACD / BollingerBands  │  │
│  └─────┬──────┘    └──────────────┘    └────────────┬─────────────┘  │
│        │  REST/WS                                    │                │
│        │                               ┌─────────────▼─────────────┐ │
│        │                               │      FeatureExtractor      │ │
│        │                               │  (50-100 features/candle) │ │
│        │                               └─────────────┬─────────────┘ │
│        │                                             │                │
│        │                         ┌───────────────────┼─────────────┐ │
│        │                         │     ML Ensemble   │             │ │
│        │                         │  ┌────────────┐  ┌┴──────────┐  │ │
│        │                         │  │ LSTMModel  │  │XGBoostModel│ │ │
│        │                         │  │ (LibTorch) │  │ (XGBoost) │  │ │
│        │                         │  └─────┬──────┘  └─────┬─────┘  │ │
│        │                         │        └────────┬────────┘        │ │
│        │                         │          SignalEnhancer            │ │
│        │                         └──────────────┬────────────────────┘ │
│        │                                        │                       │
│        │                         ┌──────────────▼──────────────────┐   │
│        │                         │      MLEnhancedStrategy          │   │
│        │                         │  - Entry/exit logic              │   │
│        │                         │  - Trailing stop                 │   │
│        │                         └──────────────┬──────────────────┘   │
│        │                                        │                       │
│        │                         ┌──────────────▼──────────────────┐   │
│        └────────────────────────▶│        RiskManager               │   │
│                                  │  - Kelly / fixed-fraction sizing │   │
│                                  │  - Drawdown protection           │   │
│                                  └──────────────┬──────────────────┘   │
│                                                 │                       │
│                                  ┌──────────────▼──────────────────┐   │
│                                  │    Dashboard (ncurses / text)    │   │
│                                  └─────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────┘
```

## Project Structure

```
├── CMakeLists.txt
├── README.md
├── config/
│   └── settings.json        ← exchange keys, trading params, ML config
├── src/
│   ├── main.cpp
│   ├── core/                ← Engine, EventBus, Logger
│   ├── exchange/            ← Binance/Bybit REST + WebSocket
│   ├── data/                ← CandleData structs, DataFeed, HistoricalLoader
│   ├── indicators/          ← EMA, RSI, ATR, MACD, BB, IndicatorEngine
│   ├── ml/                  ← LSTM, XGBoost, FeatureExtractor, ModelTrainer
│   ├── strategy/            ← MLEnhancedStrategy, RiskManager
│   └── ui/                  ← ncurses Dashboard
└── tests/
    ├── test_indicators.cpp
    ├── test_features.cpp
    └── test_risk.cpp
```

## Dependencies

| Library          | Purpose                          |
|------------------|----------------------------------|
| libcurl          | HTTP REST requests               |
| Boost.Beast      | WebSocket connectivity           |
| nlohmann/json    | JSON parsing                     |
| OpenSSL          | HMAC-SHA256 request signing      |
| spdlog           | Structured logging               |
| LibTorch         | LSTM neural network (optional)   |
| XGBoost C++ API  | Gradient boosting (optional)     |
| Eigen3           | Matrix operations (optional)     |
| ncurses          | Console UI dashboard (optional)  |
| Google Test      | Unit tests (optional)            |

## Build Instructions

### Linux / macOS

```bash
# Install required dependencies (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install -y \
    build-essential cmake git \
    libcurl4-openssl-dev \
    libssl-dev \
    libboost-all-dev \
    nlohmann-json3-dev \
    libspdlog-dev \
    libgtest-dev \
    libeigen3-dev \
    libncurses-dev

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### macOS (Homebrew)

```bash
brew install cmake curl openssl boost nlohmann-json spdlog googletest eigen ncurses
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DOPENSSL_ROOT_DIR=$(brew --prefix openssl)
make -j$(sysctl -n hw.ncpu)
```

### Windows (vcpkg)

```powershell
vcpkg install curl openssl boost-beast nlohmann-json spdlog gtest eigen3
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=<vcpkg-root>/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

### Optional: LibTorch (LSTM)

```bash
# Download LibTorch from https://pytorch.org/get-started/locally/
cmake .. -DCMAKE_PREFIX_PATH=/path/to/libtorch
```

### Optional: XGBoost

```bash
git clone --recursive https://github.com/dmlc/xgboost
cd xgboost && mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make install
```

## Running

```bash
# Edit config/settings.json with your API keys
# Default: testnet / paper trading mode (safe)
./crypto_trader config/settings.json
```

## Running Tests

```bash
cd build
ctest --output-on-failure
# or directly:
./test_indicators
./test_features
./test_risk
```

## Configuration

Edit `config/settings.json`:

- **exchange.testnet**: `true` — use testnet (safe default)
- **trading.mode**: `"paper"` — simulate trades without real orders
- **trading.symbol**: e.g. `"BTCUSDT"`
- **trading.interval**: `"15m"`, `"1h"`, etc.
- **ml.ensemble.min_confidence**: minimum signal confidence (0.65 default)
- **risk.max_risk_per_trade**: max 2% per trade by default

## Safety

- Default configuration uses **testnet** + **paper trading** — no real money at risk
- Always test thoroughly before enabling live trading
- Never commit API keys to version control