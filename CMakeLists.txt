cmake_minimum_required(VERSION 3.16)
project(CryptoTrader VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Build type ────────────────────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# ── Find required packages ────────────────────────────────────────────────────
find_package(CURL QUIET)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(spdlog REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)
find_package(Eigen3 QUIET)

# Optional packages
find_package(Curses QUIET)
find_package(Torch QUIET)
find_package(XGBoost QUIET)
find_package(GTest QUIET)

# ── Compile options ────────────────────────────────────────────────────────────
set(COMMON_FLAGS
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
    $<$<CONFIG:Debug>:-g -O0>
)

# ── Source files ──────────────────────────────────────────────────────────────
set(CORE_SOURCES
    src/core/Logger.cpp
    src/core/EventBus.cpp
    src/core/Engine.cpp
)

set(EXCHANGE_SOURCES
    src/exchange/WebSocketClient.cpp
    src/exchange/BinanceExchange.cpp
    src/exchange/BybitExchange.cpp
)

set(DATA_SOURCES
    src/data/DataFeed.cpp
    src/data/HistoricalLoader.cpp
)

set(INDICATOR_SOURCES
    src/indicators/EMA.cpp
    src/indicators/RSI.cpp
    src/indicators/ATR.cpp
    src/indicators/PineScriptIndicator.cpp
    src/indicators/IndicatorEngine.cpp
)

set(ML_SOURCES
    src/ml/FeatureExtractor.cpp
    src/ml/LSTMModel.cpp
    src/ml/XGBoostModel.cpp
    src/ml/ModelTrainer.cpp
    src/ml/SignalEnhancer.cpp
)

set(STRATEGY_SOURCES
    src/strategy/RiskManager.cpp
    src/strategy/MLEnhancedStrategy.cpp
)

set(UI_SOURCES
    src/ui/Dashboard.cpp
)

set(ALL_LIB_SOURCES
    ${CORE_SOURCES}
    ${EXCHANGE_SOURCES}
    ${DATA_SOURCES}
    ${INDICATOR_SOURCES}
    ${ML_SOURCES}
    ${STRATEGY_SOURCES}
    ${UI_SOURCES}
)

# ── Library target ─────────────────────────────────────────────────────────────
add_library(crypto_lib STATIC ${ALL_LIB_SOURCES})
target_compile_options(crypto_lib PRIVATE ${COMMON_FLAGS})
target_include_directories(crypto_lib PUBLIC src)

target_link_libraries(crypto_lib PUBLIC
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    Boost::system
    pthread
)

# CURL (optional — needed for REST API)
if(CURL_FOUND)
    target_link_libraries(crypto_lib PUBLIC CURL::libcurl)
    target_compile_definitions(crypto_lib PUBLIC USE_CURL)
else()
    message(STATUS "libcurl not found — REST API disabled")
endif()

# Eigen3 (optional — used in feature extractor)
if(Eigen3_FOUND)
    target_link_libraries(crypto_lib PUBLIC Eigen3::Eigen)
    target_compile_definitions(crypto_lib PUBLIC USE_EIGEN)
endif()

# ncurses (optional)
if(CURSES_FOUND)
    target_link_libraries(crypto_lib PUBLIC ${CURSES_LIBRARIES})
    target_include_directories(crypto_lib PUBLIC ${CURSES_INCLUDE_DIRS})
    target_compile_definitions(crypto_lib PUBLIC USE_NCURSES)
endif()

# LibTorch (optional)
if(Torch_FOUND)
    target_link_libraries(crypto_lib PUBLIC ${TORCH_LIBRARIES})
    target_compile_definitions(crypto_lib PUBLIC USE_LIBTORCH)
endif()

# XGBoost (optional)
if(XGBoost_FOUND)
    target_link_libraries(crypto_lib PUBLIC xgboost::xgboost)
    target_compile_definitions(crypto_lib PUBLIC USE_XGBOOST)
endif()

# ── Main executable ────────────────────────────────────────────────────────────
add_executable(crypto_trader src/main.cpp)
target_compile_options(crypto_trader PRIVATE ${COMMON_FLAGS})
target_link_libraries(crypto_trader PRIVATE crypto_lib)

# ── Tests ──────────────────────────────────────────────────────────────────────
if(GTest_FOUND OR GTEST_FOUND)
    enable_testing()

    add_executable(test_indicators tests/test_indicators.cpp)
    target_link_libraries(test_indicators PRIVATE crypto_lib GTest::GTest GTest::Main)
    add_test(NAME test_indicators COMMAND test_indicators)

    add_executable(test_features tests/test_features.cpp)
    target_link_libraries(test_features PRIVATE crypto_lib GTest::GTest GTest::Main)
    add_test(NAME test_features COMMAND test_features)

    add_executable(test_risk tests/test_risk.cpp)
    target_link_libraries(test_risk PRIVATE crypto_lib GTest::GTest GTest::Main)
    add_test(NAME test_risk COMMAND test_risk)
else()
    message(STATUS "GTest not found — unit tests will not be built")
endif()

# ── Install ────────────────────────────────────────────────────────────────────
install(TARGETS crypto_trader DESTINATION bin)
install(DIRECTORY config/ DESTINATION etc/crypto_trader)
