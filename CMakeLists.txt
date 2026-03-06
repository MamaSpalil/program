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

# SQLite3 (required for local candle cache)
find_package(unofficial-sqlite3 CONFIG QUIET)
if(NOT unofficial-sqlite3_FOUND)
    find_package(SQLite3 QUIET)
endif()

# Optional packages
find_package(Curses QUIET)
find_package(Torch QUIET)
find_package(XGBoost QUIET)
find_package(GTest QUIET)
find_package(glfw3 QUIET)
find_package(OpenGL QUIET)

# ── Compile options ────────────────────────────────────────────────────────────
if(MSVC)
    set(COMMON_FLAGS /W4 /permissive- /wd4267)
    add_definitions(
        -D_CRT_SECURE_NO_WARNINGS
        -DNOMINMAX
        -DWIN32_LEAN_AND_MEAN
        -D_WIN32_WINNT=0x0601
        -D_USE_MATH_DEFINES
    )
else()
    set(COMMON_FLAGS
        -Wall -Wextra -Wpedantic
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
        $<$<CONFIG:Debug>:-g -O0>
    )
endif()

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
    src/exchange/OKXExchange.cpp
    src/exchange/BitgetExchange.cpp
    src/exchange/KuCoinExchange.cpp
)

set(DATA_SOURCES
    src/data/DataFeed.cpp
    src/data/HistoricalLoader.cpp
    src/data/ExchangeDB.cpp
    src/data/CandleCache.cpp
    src/data/TradingDatabase.cpp
    src/data/TradeRepository.cpp
    src/data/OrderRepository.cpp
    src/data/PositionRepository.cpp
    src/data/EquityRepository.cpp
    src/data/AlertRepository.cpp
    src/data/BacktestRepository.cpp
    src/data/DrawingRepository.cpp
    src/data/AuxRepository.cpp
    src/data/DatabaseMigrator.cpp
    src/data/DatabaseMaintenance.cpp
)

set(INDICATOR_SOURCES
    src/indicators/EMA.cpp
    src/indicators/RSI.cpp
    src/indicators/ATR.cpp
    src/indicators/PineScriptIndicator.cpp
    src/indicators/IndicatorEngine.cpp
    src/indicators/PineConverter.cpp
    src/indicators/UserIndicatorManager.cpp
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

set(TRADING_SOURCES
    src/trading/OrderManagement.cpp
    src/trading/PaperTrading.cpp
    src/trading/TradeMarker.cpp
    src/trading/TradeHistory.cpp
)

set(SECURITY_SOURCES
    src/security/KeyVault.cpp
)

set(BACKTEST_SOURCES
    src/backtest/BacktestEngine.cpp
)

set(ALERT_SOURCES
    src/alerts/AlertManager.cpp
)

set(SCANNER_SOURCES
    src/scanner/MarketScanner.cpp
)

set(AUTOMATION_SOURCES
    src/automation/Scheduler.cpp
    src/automation/GridBot.cpp
)

set(EXTERNAL_SOURCES
    src/external/NewsFeed.cpp
    src/external/FearGreed.cpp
)

set(INTEGRATION_SOURCES
    src/integration/WebhookServer.cpp
    src/integration/TelegramBot.cpp
)

set(TAX_SOURCES
    src/tax/TaxReporter.cpp
)

set(DRAWING_SOURCES
    src/ui/DrawingTools.cpp
    src/ui/MultiTimeframe.cpp
)

# NOTE: src/futures/FuturesData.cpp — добавить когда файл будет создан
# set(FUTURES_SOURCES
#     src/futures/FuturesData.cpp
# )

set(DEPTH_SOURCES
    src/exchange/DepthStream.cpp
)

set(AI_SOURCES
    src/ai/RLTradingAgent.cpp
    src/ai/FeatureExtractor.cpp
    src/ai/OnlineLearningLoop.cpp
)

set(UI_SOURCES
    src/ui/Dashboard.cpp
    src/ui/LayoutManager.cpp
)

set(PINEVIZ_SOURCES
    src/indicators/PineVisuals.cpp
)

set(CSVEXPORT_SOURCES
    src/data/CSVExporter.cpp
)

# ── ImGui sources (vendored in third_party/imgui) ────────────────────────────
set(IMGUI_SOURCES
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/imgui_impl_glfw.cpp
    third_party/imgui/imgui_impl_opengl3.cpp
    src/ui/AppGui.cpp
)

set(ALL_LIB_SOURCES
    ${CORE_SOURCES}
    ${EXCHANGE_SOURCES}
    ${DATA_SOURCES}
    ${INDICATOR_SOURCES}
    ${ML_SOURCES}
    ${STRATEGY_SOURCES}
    ${UI_SOURCES}
    ${TRADING_SOURCES}
    ${SECURITY_SOURCES}
    ${BACKTEST_SOURCES}
    ${ALERT_SOURCES}
    ${SCANNER_SOURCES}
    ${PINEVIZ_SOURCES}
    ${CSVEXPORT_SOURCES}
    ${AUTOMATION_SOURCES}
    ${EXTERNAL_SOURCES}
    ${INTEGRATION_SOURCES}
    ${TAX_SOURCES}
    ${DRAWING_SOURCES}
    # ${FUTURES_SOURCES}   # раскомментировать после создания src/futures/FuturesData.cpp
    ${DEPTH_SOURCES}
    ${AI_SOURCES}
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
)

# Link pthread on non-Windows platforms
if(NOT WIN32)
    target_link_libraries(crypto_lib PUBLIC pthread)
endif()

# Link Winsock on Windows (needed for Boost.Asio / networking)
if(WIN32)
    target_link_libraries(crypto_lib PUBLIC ws2_32 wsock32 crypt32)
endif()

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

# SQLite3 (required for local candle cache)
if(unofficial-sqlite3_FOUND)
    target_link_libraries(crypto_lib PUBLIC unofficial::sqlite3::sqlite3)
elseif(SQLite3_FOUND)
    target_link_libraries(crypto_lib PUBLIC SQLite::SQLite3)
else()
    message(FATAL_ERROR "SQLite3 not found — required for candle cache")
endif()

# ncurses (optional)
if(CURSES_FOUND)
    target_link_libraries(crypto_lib PUBLIC ${CURSES_LIBRARIES})
    target_include_directories(crypto_lib PUBLIC ${CURSES_INCLUDE_DIRS})
    target_compile_definitions(crypto_lib PUBLIC USE_NCURSES)
endif()

# LibTorch (optional — provides PyTorch C++ API for AI modules)
# Integrate via: cmake -DTorch_DIR=C:/libs/libtorch/share/cmake/Torch
# Download from: https://pytorch.org/get-started/locally/
if(Torch_FOUND)
    message(STATUS "LibTorch found — AI engine enabled (libs: ${TORCH_LIBRARIES})")
    target_include_directories(crypto_lib PUBLIC ${TORCH_INCLUDE_DIRS})
    target_link_libraries(crypto_lib PUBLIC ${TORCH_LIBRARIES})
    target_compile_definitions(crypto_lib PUBLIC USE_LIBTORCH)
    if(TORCH_CXX_FLAGS)
        target_compile_options(crypto_lib PUBLIC ${TORCH_CXX_FLAGS})
    endif()
else()
    message(STATUS "LibTorch not found — AI engine will use stub (no ML inference)")
endif()

# XGBoost (optional)
if(XGBoost_FOUND)
    target_link_libraries(crypto_lib PUBLIC xgboost::xgboost)
    target_compile_definitions(crypto_lib PUBLIC USE_XGBOOST)
endif()

# ── Copy config and user indicators to build directory so relative paths work ──
file(COPY ${CMAKE_SOURCE_DIR}/config DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/user_indicator DESTINATION ${CMAKE_BINARY_DIR})

# ── Main executable ────────────────────────────────────────────────────────────
add_executable(crypto_trader src/main.cpp)
target_compile_options(crypto_trader PRIVATE ${COMMON_FLAGS})
target_link_libraries(crypto_trader PRIVATE crypto_lib)

# ── Копирование LibTorch DLL рядом с .exe ─────────────────────────────────────
# $<TARGET_RUNTIME_DLLS> требует CMake >= 3.21, а VS2019 — CMake 3.20.
# xcopy через CMake VS-генератор вызывает ошибку «Недопустимое число параметров»
# из-за некорректного экранирования путей и подстановочных знаков.
# Решение: file(GLOB) на этапе configure + cmake -E copy_if_different.
if(Torch_FOUND AND WIN32)
    # Определяем каталог DLL из Torch_DIR (share/cmake/Torch → lib)
    get_filename_component(_TORCH_ROOT "${Torch_DIR}/../../.." ABSOLUTE)
    set(_TORCH_DLL_DIR "${_TORCH_ROOT}/lib")

    file(GLOB _TORCH_DLLS "${_TORCH_DLL_DIR}/*.dll")
    if(_TORCH_DLLS)
        add_custom_command(TARGET crypto_trader POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${_TORCH_DLLS}
                $<TARGET_FILE_DIR:crypto_trader>
            COMMENT "Copying LibTorch DLLs to output directory"
        )
    else()
        message(WARNING "No LibTorch DLLs found in ${_TORCH_DLL_DIR} — runtime DLLs not copied")
    endif()
endif()

# ── Embed application icon on Windows (.rc resource) ──────────────────────────
if(WIN32)
    target_sources(crypto_trader PRIVATE resources/app_resource.rc)
endif()

# ── ImGui / GLFW / OpenGL (optional — provides windowed GUI) ──────────────────
if(glfw3_FOUND AND OPENGL_FOUND)
    add_library(imgui_lib STATIC ${IMGUI_SOURCES})
    target_include_directories(imgui_lib PUBLIC
        third_party/imgui
        src
    )
    target_compile_options(imgui_lib PRIVATE ${COMMON_FLAGS})
    target_link_libraries(imgui_lib PUBLIC
        crypto_lib
        glfw
        OpenGL::GL
    )

    target_link_libraries(crypto_trader PRIVATE imgui_lib)
    target_compile_definitions(crypto_trader PRIVATE USE_IMGUI)
    message(STATUS "GLFW + OpenGL found — GUI enabled")
else()
    message(STATUS "GLFW or OpenGL not found — GUI disabled (console mode)")
endif()

# ── Tests (single combined executable) ────────────────────────────────────────
if(GTest_FOUND OR GTEST_FOUND)
    enable_testing()

    add_executable(crypto_trader_tests
        tests/test_indicators.cpp
        tests/test_features.cpp
        tests/test_risk.cpp
        tests/test_exchanges.cpp
        tests/test_logger.cpp
        tests/test_modules.cpp
        tests/test_candle_cache.cpp
        tests/test_system.cpp
        tests/test_regression_part1.cpp
        tests/test_regression_part2.cpp
        tests/test_regression_part3.cpp
        tests/test_trading_db.cpp
        tests/test_full_system.cpp
        tests/test_ai_engine.cpp
        tests/test_v170.cpp
    )
    target_link_libraries(crypto_trader_tests PRIVATE crypto_lib GTest::GTest GTest::Main)
    target_compile_definitions(crypto_trader_tests PRIVATE CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
    add_test(NAME crypto_trader_tests COMMAND crypto_trader_tests)
else()
    message(STATUS "GTest not found — unit tests will not be built")
endif()

# ── Install ────────────────────────────────────────────────────────────────────
install(TARGETS crypto_trader DESTINATION bin)
install(DIRECTORY config/ DESTINATION etc/crypto_trader)
