# Changelog

All notable changes to the CryptoTrader project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.3.0] - 2026-03-06

### Fixed

#### Этап 1: BacktestEngine — Полное сохранение метаданных в БД
- **BacktestEngine.cpp computeMetrics()** — полностью переработана секция сохранения в базу данных:
  - ✅ `symbol` — теперь сохраняется из Config (ранее пустая строка)
  - ✅ `timeframe` — теперь сохраняется из Config (ранее пустая строка)
  - ✅ `initialBalance` — теперь сохраняется из Config (ранее 0.0)
  - ✅ `finalBalance` — теперь вычисляется из последней точки equity curve (ранее 0.0)
  - ✅ `strategy` — теперь "EMA_Crossover" (ранее пустая строка)
  - ✅ `commission` — теперь сохраняется из Config (ранее 0.0)
  - ✅ `dateFrom` — теперь берётся из первой сделки entryTime (ранее 0)
  - ✅ `dateTo` — теперь берётся из последней сделки exitTime (ранее 0)
- **BacktestEngine.h computeMetrics()** — сигнатура изменена с `(Result&, double)` на `(Result&, const Config&)` для доступа ко всем полям конфигурации.
- **BtTradeRecord** — теперь сохраняет `symbol` из Config и вычисляет `commission` как `(entryPrice * qty + exitPrice * qty) * cfg.commission` (ранее оба поля были 0).

#### Этап 2: PineConverter — Расширенная генерация C++ для Pine Script v6
- **PineConverter.cpp generateCpp()** — полностью переработан C++ генератор для поддержки всех основных функций Pine Script v6:
  - ✅ `ta.sma(source, length)` — SMA вычисляется из внутреннего deque closes_ (ранее НЕ поддерживался)
  - ✅ `ta.macd(source, fast, slow, signal)` — генерирует 3 EMA (fast/slow/signal) + macd/signal/histogram (ранее НЕ поддерживался)
  - ✅ `ta.bb(source, period, stddev)` — Bollinger Bands upper/middle/lower (ранее НЕ поддерживался)
  - ✅ `ta.crossover(a, b)` — сохраняет предыдущие значения, детектирует пересечение вверх (ранее НЕ поддерживался)
  - ✅ `ta.crossunder(a, b)` — детектирует пересечение вниз (ранее НЕ поддерживался)
  - ✅ `ta.highest(source, length)` — максимум за период из closes_ (ранее НЕ поддерживался)
  - ✅ `ta.lowest(source, length)` — минимум за период из closes_ (ранее НЕ поддерживался)
  - ✅ `ta.stdev(source, length)` — стандартное отклонение за период (ранее НЕ поддерживался)
  - ✅ `ta.change(source, length)` — изменение за N баров (ранее НЕ поддерживался)
  - ✅ `ta.vwap(source)` — VWAP через cumPV_/cumVol_ (ранее НЕ поддерживался)
  - Генерируемый C++ класс теперь включает `<deque>`, `<numeric>`, `<algorithm>` для SMA/BB/стат. функций
  - Обратная совместимость: EMA, RSI, ATR по-прежнему генерируются корректно

#### Этап 3: Версия приложения обновлена
- **AppGui.cpp** — версия обновлена с "v2.2.0" на "v2.3.0" в двух местах: welcome log и About menu.
- **main.cpp** — версия обновлена с "v2.2.0" на "v2.3.0" в startup log.

### Added

#### Этап 4: Тестирование v2.3.0 (13 новых тестов)
- **test_full_system.cpp** — 13 новых тестов:
  - `BacktestV230` (3): DatabaseSavesSymbolAndTimeframe — проверка symbol/timeframe/initialBalance/finalBalance/strategy/commission в БД, DatabaseSavesFinalBalance — проверка finalBalance из equity curve, TradeRecordsIncludeSymbolAndCommission — проверка BtTradeRecord с symbol и commission
  - `PineConverterV230` (9): GenerateCppWithSMA — SMA генерация, GenerateCppWithMACD — MACD генерация (3 EMA + macd/signal/hist), GenerateCppWithBollingerBands — BB upper/middle/lower, GenerateCppWithCrossover — crossover с prev_ переменными, GenerateCppWithHighestLowest — highest/lowest из closes_, GenerateCppWithStdev — stdev с sqrt, GenerateCppWithVWAP — VWAP с cumPV/cumVol, GenerateCppWithChange — change за N баров, GenerateCppEMAStillWorks — обратная совместимость EMA/RSI/ATR
  - `VersionV230` (1): VersionStringExists — проверка наличия строки "2.3.0"
- **Итого тестов:** 568 (564 пройдено, 4 пропущено — LibTorch/XGBoost)

### Changed
- **CHANGELOG.md** — добавлена секция v2.3.0 со всеми исправлениями и новыми функциями.
- **ANALYSIS_AND_PROMPT.md** — обновлён на основе v2.3.0: исправленные проблемы отмечены ✅ Done, обновлён промт для следующего этапа v2.4.0.

## [2.2.0] - 2026-03-06

### Fixed

#### Этап 1: UI — Layout Lock теперь функциональный
- **AppGui.cpp** — 6 контентных окон (Market Data, Indicators, Logs, Volume Delta, Pair List, User Panel) теперь используют `lockWindow()` (фиксация позиции каждый кадр) **только** при `config_.layoutLocked == true`. При `layoutLocked == false` используется `lockWindowOnce()` (позиция задаётся один раз, далее окна свободно перемещаемы). Ранее все окна были жёстко зафиксированы вне зависимости от настройки "Lock Windows".
- **AppGui.cpp** — Флаги окон (NoMove, NoResize) теперь также условны: при разблокированном layout используются только NoCollapse и NoBringToFrontOnFocus.
- **Main Toolbar и Filters Bar** — всегда зафиксированы (фиксированная структура).

#### Этап 2: Engine — SymbolFormatter интеграция
- **Engine.h** — добавлен `#include "../exchange/SymbolFormatter.h"` и метод `getExchangeName()` → возвращает raw имя биржи (binance/bybit/okx/kucoin/bitget).
- **Engine.cpp** — добавлено поле `exchangeNameQualified` (name_mode) для изоляции кеша свечей. Raw имя биржи хранится в `exchangeName`. Все вызовы CandleCache теперь используют `exchangeNameQualified` для корректной изоляции данных между режимами (paper/live).

#### Этап 3: Версия приложения обновлена
- **AppGui.cpp** — версия обновлена с "v1.0.0" на "v2.2.0" в трёх местах: About menu, status bar, welcome log.
- **main.cpp** — версия обновлена с "v1.0.0" на "v2.2.0" в startup log.

### Added

#### Этап 4: TelegramBot — Интеграция в Engine через main.cpp
- **main.cpp** — TelegramBot теперь инициализируется при запуске GUI:
  - Читает конфигурацию из `config["telegram"]["token"]` и `config["telegram"]["chat_id"]`
  - Автоматически регистрирует 4 реальных callback-команды:
    - `/balance` → `engine->getFuturesBalance()` → totalWalletBalance, totalMarginBalance, totalUnrealizedProfit
    - `/status` → текущее состояние: биржа, символ, интервал, режим, тип рынка
    - `/positions` → `engine->getPositionRisk()` → символ, qty, entry, unrealized PnL
    - `/pnl` → `engine->totalPnl()` + `engine->winRate()` → общий P&L и win rate
  - Polling запускается при подключении, останавливается при отключении и при завершении программы
  - Graceful fallback при отсутствии конфигурации (без token — бот не запускается)

#### Этап 5: Database — Автосоздание директории при первом запуске
- **main.cpp** (GUI и Console mode) — перед инициализацией TradingDatabase проверяется наличие директории конфигурации. Если директория не существует (первый запуск), она создаётся через `std::filesystem::create_directories()`. Ранее отсутствие директории приводило к ошибке SQLite при создании файла БД.

#### Этап 6: Тестирование v2.2.0 (18 новых тестов)
- **test_full_system.cpp** — 18 новых тестов:
  - `LayoutLockV220` (3): LockedWindowsUseCorrectFlags — проверка NoMove+NoResize+NoCollapse, LockedScrollFlagsIncludeHScroll — проверка HorizontalScrollbar, DefaultLayoutLockedIsFalse — дефолт layoutLocked=false
  - `SymbolFormatterV220` (5): EmptyInputsReturnEmpty — пустые base/quote → пустой результат, UnknownExchangeDefaultsBehavior — неизвестная биржа → concatenation, CaseInsensitiveExchange — нечувствительность к регистру, ExtractBaseBUSD — BUSD quote, ExtractBaseETHQuote — ETH quote
  - `TelegramBotV220` (4): BalanceCallbackReturnsData, StatusCallbackReturnsData, PositionsCallbackReturnsData, PnlCallbackReturnsData — проверка реальных callback-ов через processCommand
  - `DatabaseV220` (3): InitCreatesDbFileFromScratch — DB создаётся на пустом месте, DirectoryCreationForDb — автосоздание директории, RepeatedInitIdempotent — повторный init не ломает DB
  - `LayoutV220` (2): AllWindowsStrictlyOrdered — полная проверка вертикального/горизонтального порядка окон, WindowsDontTouchEachOther — колонки не перекрываются
  - `VersionV220` (1): VersionStringExists — проверка наличия строки "2.2.0"
- **Итого тестов:** 555 (551 пройдено, 4 пропущено — LibTorch/XGBoost)

### Changed
- **CHANGELOG.md** — добавлена секция v2.2.0 со всеми исправлениями и новыми функциями.
- **ANALYSIS_AND_PROMPT.md** — обновлён на основе v2.2.0: исправленные проблемы отмечены ✅ Done, обновлён промт для следующего этапа v2.3.0.

## [2.1.0] - 2026-03-06

### Fixed

#### Этап 1: TradingDatabase — nullptr guard
- **TradingDatabase.cpp execute()** — добавлена проверка `if (!db_)` перед вызовом `sqlite3_exec()`. Ранее при вызове `execute()` без предварительного `init()` (или после failed init) происходил crash, т.к. `db_` был nullptr. Теперь возвращает `false` с предупреждением в лог (TradingDatabase.cpp:35-38).

#### Этап 2: LayoutManager — logPct_ теперь функциональный
- **LayoutManager.cpp** — высота окна Logs теперь вычисляется как `max(40, floor(Hvp * logPct_))` вместо жёстко заданных 120px. Слайдер "Logs Height (%)" в Settings теперь реально влияет на высоту окна Logs. Минимальная высота 40px для предотвращения исчезновения окна на маленьких экранах (LayoutManager.cpp:28-30).
- **AppGui.cpp "Reset Layout to Defaults"** — исправлены значения по умолчанию: `vdPct=0.15` и `indPct=0.20` (ранее `0.13` и `0.25`). Теперь синхронизированы с LayoutManager.h defaults (AppGui.cpp:2406-2407).
- **AppGui.cpp Settings → Layout** — исправлено отображение "Market Data Height": теперь показывает `(1 - indPct) * (1 - logPct)` вместо `1 - logPct - vdPct - indPct`. VD находится в левой колонке и не влияет на высоту Market Data в центральной колонке. Добавлена пояснительная строка с разбивкой пропорций (AppGui.cpp:2379-2389).

### Added

#### Этап 3: SymbolFormatter — Унификация формата символов
- **SymbolFormatter** (`src/exchange/SymbolFormatter.h`, `src/exchange/SymbolFormatter.cpp`) — новый утилитный класс для конвертации символов между форматами бирж:
  - `toSpot(exchange, base, quote)` → Binance/Bybit/Bitget: "BTCUSDT", OKX/KuCoin: "BTC-USDT"
  - `toFutures(exchange, base, quote)` → OKX: "BTC-USDT-SWAP", Bitget: "BTCUSDT_UMCBL", остальные: "BTCUSDT"/"BTC-USDT"
  - `toUnified(exchange, rawSymbol)` → удаляет разделители и суффиксы (-SWAP, _UMCBL, -) → "BTCUSDT"
  - `extractBase(unified)` / `extractQuote(unified)` → разбор "BTCUSDT" → "BTC" + "USDT"
  - Поддерживает USDT, USDC, BUSD, BTC, ETH как quote assets
- **CMakeLists.txt** — добавлен `src/exchange/SymbolFormatter.cpp` в EXCHANGE_SOURCES

#### Этап 4: TelegramBot — Реальные command callbacks
- **TelegramBot.h** — добавлен `setCommandCallback(command, callback)` для регистрации обработчиков команд. `TelegramCommandCallback = std::function<std::string()>`. Хранятся в `commandCallbacks_` (map, protected by `callbackMutex_`).
- **TelegramBot.cpp processCommand()** — при наличии зарегистрированного callback для команды, вызывает его вместо stub-ответа. Callback exceptions обрабатываются с `try/catch` и логированием ошибки. Stub-ответы обновлены с пометкой "(no callback registered)" для ясности.
- Позволяет Engine регистрировать реальные обработчики:
  - `/balance` → запрос баланса с биржи
  - `/status` → текущий статус Engine
  - `/positions` → список открытых позиций
  - `/pnl` → сводка P&L

#### Этап 5: Тестирование v2.1.0
- **test_full_system.cpp** — 35 новых тестов:
  - `TradingDatabaseV210` (3): ExecuteWithNullDbReturnsFalse, ExecuteAfterInitSucceeds, IsOpenFalseBeforeInit
  - `SymbolFormatterV210` (16): ToSpot × 5 бирж, ToFutures × 4 биржи, ToUnified × 4 формата, ExtractBaseAndQuote, ExtractBaseUnrecognized
  - `LayoutV210` (6): LogsHeightUsesPercentage, LogsHeightMinimum40px, LogsHeightDefault10Percent, MarketDataGetsEnoughSpace, UserPanelOnRight, WindowsNoOverlapAfterLogPctChange
  - `TelegramBotV210` (5): CommandCallbackRegistration, CallbackOverridesStub, MultipleCallbacks, UnauthorizedWithCallback, CallbackExceptionHandled
  - `GridBotV210` (3): GridLevelsCreated, GeometricGridLevels, OnOrderFilledProfitTracking
  - `DatabaseV210` (3): TradeRepositoryRoundtrip, OrderRepositoryRoundtrip, EquityRepositoryRoundtrip
- **test_v170.cpp** — обновлены тесты: LogWindowHeight120 → LogWindowHeightPercentage (проверка percentage-based расчёта вместо fixed 120px)
- **Итого тестов:** 537 (533 пройдено, 4 пропущено — LibTorch/XGBoost)

### Changed
- **CHANGELOG.md** — добавлена секция v2.1.0 со всеми исправлениями и новыми функциями.
- **ANALYSIS_AND_PROMPT.md** — обновлён на основе v2.1.0: исправленные проблемы отмечены ✅ Done, обновлён промт для следующего этапа v2.2.0.

## [2.0.0] - 2026-03-06

### Fixed

#### Этап 1: UI Layout — Volume Delta перемещён в левую колонку
- **LayoutManager.cpp** — полностью переработана структура расположения окон:
  - **Volume Delta** перемещён из центральной колонки в **левую колонку**, ниже Pair List
  - Левая колонка теперь содержит: Pair List (верх) → Volume Delta (низ)
  - Центральная колонка упрощена: Market Data (верх) → Indicators (низ)
  - Volume Delta теперь имеет ширину 200px (как Pair List) вместо полной ширины центральной колонки
  - Pair List высота уменьшена до `Hcenter - Hvd` (оставляя место для VD)
  - Market Data получает больше вертикального пространства (нет VD сверху): `Hcenter - Hind`
  - Параметр `showLeft` = `showPairList || showVolumeDelta` — левая колонка видна при любом из двух
- **AppGui.cpp** — обновлены комментарии рендеринга для нового порядка окон

#### Этап 2: Exchange — WebSocket bounds checking (Bybit, KuCoin)
- **BybitExchange.cpp onWsMessage()** — добавлена проверка `j["data"].is_array()` и `j["data"].empty()` перед доступом к `[0]`. Ранее при пустом массиве data происходил crash/exception.
- **KuCoinExchange.cpp onWsMessage()** — добавлена проверка `j["data"].contains("candles")`, `is_array()` и `empty()` перед доступом к `j["data"]["candles"]`. Ранее при отсутствии поля candles происходил crash.
- **KuCoinExchange.cpp getPrice()** — добавлена проверка `json.contains("data")`, `is_object()` и `contains("price")` перед доступом к `json["data"]["price"]`. Ранее при пустом ответе API происходил crash.

#### Этап 3: BacktestEngine — Sharpe Ratio empty returns guard
- **BacktestEngine.cpp computeMetrics()** — добавлена проверка `!returns.empty()` перед вычислением `avgRet` и `stdRet`. Ранее при всех нулевых/отрицательных значениях equity вектор returns оставался пустым, вызывая деление на ноль в `accumulate / returns.size()`.

### Added

#### Этап 4: Тестирование v2.0.0 (10 новых тестов)
- **test_full_system.cpp** — 10 новых тестов:
  - `LayoutV200` (6): VolumeDeltaInLeftColumn — VD X == PairList X, VolumeDeltaBelowPairList — VD Y == PL bottom, MarketDataStartsAtTopOfCenter — MD Y == FiltersBar bottom, CenterColumnOnlyMarketDataAndIndicators — VD X ≠ MD X, LeftColumnPairListPlusVDEqualsHcenter — PL+VD height == UserPanel height, NoOverlapLeftColumnAndCenter — PL/VD right ≤ MD left
  - `ExchangeV200` (3): BybitWsEmptyDataArray, KuCoinGetPriceMissingData, KuCoinWsMissingCandles
  - `BacktestV200` (1): SharpeRatioEmptyReturns — нет crash при пустом returns вектора
- **Обновлены тесты:**
  - `LayoutStress.NoOverlapCenterColumns` — проверяет VD в левой колонке
  - `LayoutStress.NoOverlapCenterVertical` — проверяет PL → VD и MD → IND
  - `LayoutStress.NoOverlapLogsAndSidebars` — проверяет VD + logs
  - `LayoutStress.MultipleResolutionsNoOverlap` — проверяет PL → VD для всех разрешений
  - `LayoutStress.HidePairListExpandsCenter` — скрывает PL+VD для расширения центра
  - `LayoutStress.HideLogsExpandsCenter` — обновлён порог для VD-split
  - `LayoutStress.CustomProportionsAffectColumnHeights` — VD/IND в разных колонках
  - `LayoutManager.WindowsDoNotOverlap` — PL → VD, MD → IND
  - `LayoutManager.WindowsAreFullWidth` — VD 200px (left), MD 1430px (center)
- **Итого тестов:** 502 (498 пройдено, 4 пропущено — LibTorch/XGBoost)

### Changed
- **CHANGELOG.md** — добавлена секция v2.0.0 со всеми исправлениями и новыми функциями.
- **ANALYSIS_AND_PROMPT.md** — обновлён на основе v2.0.0: новые исправления, обновлённый промт для v2.1.0.

## [1.9.0] - 2026-03-06

### Fixed

#### Этап 1: Exchange — Безопасность JSON-парсинга (3 биржи)
- **OKXExchange.cpp getPrice()** — добавлена проверка `json["data"]` на наличие, тип array и непустоту перед доступом к `[0]`. Ранее при пустом ответе API происходил segfault/exception.
- **OKXExchange.cpp getOrderBook()** — добавлена проверка `json["data"]` перед доступом к `[0]`. Логирование предупреждения при пустом массиве.
- **BybitExchange.cpp getPrice()** — добавлена проверка `json["result"]["list"]` на наличие, тип array и непустоту. Предотвращён crash при пустом ответе API.
- **BitgetExchange.cpp getPrice()** — добавлена проверка `json["data"]` перед доступом к `[0]`. Предотвращён crash при пустом ответе API.

#### Этап 2: PaperTrading — Комиссии и валидация
- **PaperTrading.h** — добавлены:
  - `commissionRate_{0.001}` (0.1% default taker fee)
  - `setCommissionRate(double)` / `getCommissionRate()` для настройки ставки комиссии
- **PaperTrading.cpp openPosition()** — теперь:
  - Валидация `qty > 0` и `price > 0` перед открытием позиции (отклоняет нулевые/отрицательные значения)
  - Расчёт комиссии: `commission = cost * commissionRate_`
  - Баланс уменьшается на `cost + commission` вместо только `cost`
  - Проверка баланса включает комиссию: `cost + commission > balance`
- **PaperTrading.cpp closePosition()** — теперь:
  - Комиссия на закрытие: `commission = exitValue * commissionRate_`
  - PnL уменьшается на комиссию закрытия: `pnl -= commission`
  - Баланс увеличивается на `exitValue - commission`

#### Этап 3: Scheduler — Устранение deadlock
- **Scheduler.cpp loop()** — переработан для предотвращения deadlock:
  - Callbacks собираются в вектор `pendingCallbacks` внутри блокировки мьютекса
  - Выполнение callbacks происходит **после** освобождения мьютекса
  - Ранее callback выполнялся внутри `lock_guard`, что приводило к deadlock при обращении к Scheduler из callback
  - Try-catch для каждого callback сохранён

#### Этап 4: BacktestEngine — Защита от деления на ноль
- **BacktestEngine.cpp computeMetrics()** — добавлена проверка `prev > 0` в цикле расчёта returns для Sharpe ratio. Пропуск шагов с нулевой/отрицательной equity предотвращает деление на ноль.

#### Этап 5: ModelTrainer — Корректная обработка пустых данных
- **ModelTrainer.cpp retrain()** — при отсутствии данных для обучения:
  - Больше **не обновляет** `lastRetrainTime_` (ранее обновлял, блокируя повторные попытки на 24 часа)
  - Уровень логирования повышен с `debug` до `warn` для видимости проблемы
- **ModelTrainer.h** — добавлен публичный метод `getLastRetrainTime()` для тестирования и мониторинга

### Added

#### Этап 6: Тестирование v1.9.0 (15 новых тестов)
- **test_full_system.cpp** — 15 новых тестов:
  - `PaperTradingV190` (7): CommissionDeductedOnOpen — проверка комиссии при открытии, CommissionDeductedOnClose — проверка комиссии при закрытии, CustomCommissionRate — настраиваемая ставка, ShortPositionPnLWithCommission — SHORT с комиссией, RejectInvalidQtyPrice — отклонение невалидных параметров, MultipleTradeCycle — полный цикл нескольких сделок, DrawdownCalculation — проверка расчёта drawdown
  - `ExchangeSafetyV190` (3): OKXGetPriceEmptyData — OKX не падает при пустом ответе, BybitGetPriceEmptyData — Bybit не падает, BitgetGetPriceEmptyData — Bitget не падает
  - `BacktestV190` (3): SharpeRatioZeroEquity — нет crash при нулевой equity, LongTradeProfitable — бэктест LONG на восходящем тренде, ShortTradeProfitable — бэктест SHORT на нисходящем тренде
  - `SchedulerV190` (1): CallbackOutsideMutex — проверка что job добавляется и callback не вызывает deadlock
  - `ModelTrainerV190` (1): RetrainNoDataDoesNotUpdateTime — lastRetrainTime_ не обновляется при пустых данных
- **Обновлены тесты:**
  - `PaperTrading.OpenPosition` — обновлён для комиссии 0.1%
  - `PaperTrading.OpenClosePosition` — обновлён для комиссии 0.1%
- **Итого тестов:** 492 (488 пройдено, 4 пропущено — LibTorch/XGBoost)

### Changed
- **CHANGELOG.md** — добавлена секция v1.9.0 со всеми исправлениями и новыми функциями.
- **ANALYSIS_AND_PROMPT.md** — обновлён на основе v1.9.0: 7 новых проблем найдены и исправлены, обновлён промт для следующего этапа v2.0.0.

## [1.8.0] - 2026-03-06

### Fixed

#### Этап 1: ML/AI — OnlineLearningLoop Stale State Fix
- **OnlineLearningLoop.h/.cpp** — исправлен главный RL-баг: стейт-кэш для корректного обучения. Добавлены:
  - `captureStateForTrade(tradeId)` — сохраняет текущий вектор признаков на момент открытия позиции
  - `getCachedState(tradeId)` — извлекает кэшированный стейт для использования в `pollTrades()`
  - `stateCache_` (unordered_map) с мьютексом и автоматической очисткой (max 500 записей)
  - Поле `stateCacheMtx_` для thread-safe доступа к кэшу
- **OnlineLearningLoop.cpp pollTrades()** — полностью переписан:
  - `exp.state` берётся из кэша (состояние на момент входа), а не из текущего feat_.extract()
  - `exp.nextState` = текущее состояние рынка (после закрытия сделки) — `state ≠ nextState` (bootstrap корректен)
  - `exp.logProb/value` вычисляются через `agent_.forward_pass(exp.state)` вместо нулей
  - Сделки без кэшированного стейта пропускаются с debug-логом вместо обучения на некорректных данных
  - Использованные кэш-записи удаляются после обработки

#### Этап 2: Реализация заглушек (3 модуля)
- **NewsFeed.cpp** — реализован `fetchAndParse()` через CryptoPanic API:
  - HTTP GET с API-ключом и JSON-парсингом (title, source, url, sentiment из votes)
  - Graceful fallback при отсутствии API-ключа или libcurl
  - Rate limit: 5 минут между запросами (loop)
  - Добавлен `setApiKey()` метод в NewsFeed.h
- **FearGreed.cpp** — реализован `fetch()` через Alternative.me API:
  - GET `https://api.alternative.me/fng/?limit=1`
  - Парсинг value (0-100), value_classification, timestamp из JSON
  - Thread-safe обновление через `setData()`
  - Rate limit: 1 час между запросами
- **TelegramBot.cpp** — реализованы `sendMessage()` и `getUpdates()`:
  - `sendMessage()`: POST `https://api.telegram.org/bot{token}/sendMessage` с JSON body (chat_id, text, parse_mode=HTML)
  - `getUpdates()`: GET с long-polling (timeout=30s), парсинг update_id, text, chat_id
  - Graceful fallback при отсутствии токена (логирование без HTTP-вызова)

#### Этап 3: OrderManagement & UI — Валидация и формат цен
- **OrderManagement.cpp estimateCost()** — теперь включает оценку комиссии тейкера (0.1%): `notional + notional * 0.001`. Более точная оценка стоимости ордера.
- **OrderManagement.h/.cpp** — добавлены методы адаптивного формата цен:
  - `priceFormat(double)` — возвращает строку формата: `%.2f` для ≥1000, `%.4f` для ≥1, `%.6f` для ≥0.01, `%.8f` для <0.01
  - `formatPrice(buf, size, price)` — форматирует цену в буфер с адаптивным количеством десятичных знаков
- **AppGui.cpp** — заменены хардкоды `%.8f` на `OrderManagement::formatPrice()` в:
  - Шкала цен графика (price scale)
  - Текущая цена (current price line)
  - Кроссхэйр (crosshair price label)
  - Правый клик → контекстное меню (ChartOrderMenu)
  - Текущая цена символа (main panel)
  - Подтверждение ордера (Confirm Order popup)
  - Est. Cost и Balance используют `%.2f` для USD-значений

#### Этап 4: RLTradingAgent — PPO Hyper-parameter Tuning
- **RLTradingAgent.h PPOConfig** — улучшены гиперпараметры PPO:
  - `entropyCoeff` увеличен с 0.01 до 0.02 (лучше exploration для финансовых данных)
  - Добавлено поле `valueLossClipMax{10.0f}` для ограничения value loss
  - Добавлен метод `validate()` с clamping всех гиперпараметров:
    - entropyCoeff: [0.001, 0.1], valueLossCoeff: [0.1, 1.0]
    - epsilon: [0.05, 0.3], gamma: [0.9, 0.999], lambda: [0.8, 0.99]
    - maxGradNorm: [0.1, 5.0], valueLossClipMax: [1.0, 100.0]
- **RLTradingAgent.cpp** — конструктор вызывает `cfg_.validate()` для безопасной инициализации
- **RLTradingAgent.cpp train_step()** — value loss теперь клипается: `torch::clamp(valueLoss, 0.0f, cfg_.valueLossClipMax)` для предотвращения взрывных градиентов от outlier value predictions

### Added

#### Этап 5: Тестирование v1.8.0
- **test_full_system.cpp** — 14 новых тестов:
  - `OnlineLearningV180` (2): CaptureAndRetrieveState — кэширование стейта, NextStateDiffersFromState — иммутабельность кэша
  - `NewsFeedV180` (2): AddAndRetrieveItems — CRUD, HasApiKeySetter — наличие метода setApiKey
  - `FearGreedV180` (2): SetAndGetData — установка/чтение данных, DefaultIsNeutral — дефолтные значения
  - `TelegramBotV180` (3): ProcessCommandHelp — команда /help, SendMessageNoToken — graceful без токена, UnauthorizedChat — отклонение неавторизованного чата
  - `OrderManagementV180` (3): EstimateCostWithCommission — проверка 0.1% комиссии, AdaptivePriceFormat — проверка формата для разных цен, FormatPriceBuf — форматирование в буфер
  - `RLAgentV180` (2): PPOConfigValidation — clamping гиперпараметров, PPOConfigValueLossClip — проверка valueLossClipMax
- **Обновлены тесты:**
  - `OrderManagement.EstimateCost` — обновлён для 0.1% комиссии
  - `OrderManagement.CostEstimation` — обновлён для 0.1% комиссии
  - `SystemTest.OrderManagementEdgeCases` — обновлён для 0.1% комиссии
  - `RLTradingAgentAI.PPOConfigDefaults` — обновлён для entropyCoeff=0.02 и valueLossClipMax
- **Итого тестов:** 477 (473 пройдено, 4 пропущено — LibTorch/XGBoost)

### Changed
- **CHANGELOG.md** — добавлена секция v1.8.0 со всеми исправлениями и новыми функциями.
- **ANALYSIS_AND_PROMPT.md** — обновлён на основе v1.8.0: все 7 открытых проблем отмечены ✅ Done, обновлён промт для следующего этапа v1.9.0.

## [1.7.5] - 2026-03-06

### Fixed

#### Этап 1: Exchange — getFuturesBalance() для 4 бирж
- **BybitExchange.cpp** — реализован `getFuturesBalance()`: GET `/v5/account/wallet-balance?accountType=CONTRACT` (signed). Парсинг totalWalletBalance, totalPerpUPL, totalMarginBalance из результата. USDT-специфичные поля из coin array. Использует `safeStod()` (BybitExchange.cpp:536-576, BybitExchange.h:42).
- **OKXExchange.cpp** — реализован `getFuturesBalance()`: GET `/api/v5/account/balance` (signed). Парсинг totalEq, adjEq из account, cashBal/upl из USDT detail. Использует `safeStod()` (OKXExchange.cpp:719-756, OKXExchange.h:44).
- **KuCoinExchange.cpp** — реализован `getFuturesBalance()`: GET `/api/v1/account-overview?currency=USDT` (signed). Парсинг accountEquity, unrealisedPNL, marginBalance. Использует `safeStod()` (KuCoinExchange.cpp:647-675, KuCoinExchange.h:43).
- **BitgetExchange.cpp** — реализован `getFuturesBalance()`: GET `/api/v2/mix/account/accounts?productType=USDT-FUTURES` (signed). Парсинг equity, unrealizedPL, available. Использует `safeStod()` (BitgetExchange.cpp:593-621, BitgetExchange.h:43).

#### Этап 2: ML/AI Thread-Safety и корректность
- **OnlineLearningLoop.h** — `lastTradeTime_` изменён с `long long` на `std::atomic<long long>` для предотвращения data race при многопоточном доступе (OnlineLearningLoop.h:106).
- **OnlineLearningLoop.cpp** — `pollTrades()` переписан с атомарными load/store для lastTradeTime_: единый `lastTime` вычисляется за цикл, затем атомарно записывается (OnlineLearningLoop.cpp:117-147).
- **ModelTrainer.h** — добавлен параметр `timeframeMinutes{15}` в Config для конфигурируемого расчёта candles/day (ModelTrainer.h:23).
- **ModelTrainer.cpp** — `candlesPerDay` теперь вычисляется как `1440 / timeframeMinutes` вместо хардкода 96. Поддерживает 1m(1440), 5m(288), 15m(96), 1h(24) (ModelTrainer.cpp:16).
- **ai/FeatureExtractor.cpp** — `mid < 1e-12f` fallback изменён: вместо `mid = 1.0f` (фиктивное значение, генерирующее мусорные нормализованные значения) теперь `return` — все OB-фичи остаются нулевыми (нейтральными) (ai/FeatureExtractor.cpp:110).
- **SignalEnhancer.h/.cpp** — добавлена thread-safety: `mutable std::mutex mtx_` защищает `outcomes_`, `weightInd_`, `weightLstm_`, `weightXgb_`. `recordOutcome()` и `enhance()` используют `lock_guard` (SignalEnhancer.h:38, SignalEnhancer.cpp:14-26,45).

#### Этап 3: UI/Config — Настройки и конфигурация
- **AppGui.cpp configToJson()** — добавлена секция `"chart"` с сериализацией `bar_width` (chartBarWidth_) и `scroll_offset` (chartScrollOffset_). Ранее эти настройки терялись при перезапуске (AppGui.cpp:552-555).
- **AppGui.cpp loadConfig()** — добавлена загрузка секции `"chart"` из JSON при старте: chartBarWidth_ и chartScrollOffset_ восстанавливаются из конфигурации (AppGui.cpp:479-483).
- **Engine.cpp** — `maxCandleHistory` теперь загружается из `config["engine"]["max_candle_history"]` с дефолтом 5000 (Engine.cpp:59-62).
- **BacktestEngine.h** — добавлен параметр `positionSizePct{0.95}` в Config для конфигурируемого размера позиции (BacktestEngine.h:26).
- **BacktestEngine.cpp** — `balance * 0.95` заменён на `balance * cfg.positionSizePct` (BacktestEngine.cpp:62).

#### Этап 4: TaxReporter — Точность расчёта costBasis
- **TaxReporter.h** — добавлено поле `unitCost{0.0}` в TaxLot для хранения per-unit стоимости, исключающей накопление ошибок округления при partial fills (TaxReporter.h:19).
- **TaxReporter.cpp** — costBasis при частичных продажах теперь вычисляется как `used * lot.unitCost` вместо `used * (lot.costBasis / lot.qty)`. Устраняет деградацию точности при FIFO/LIFO (TaxReporter.cpp:33,47).

### Added

#### Этап 5: Тестирование v1.7.5
- **test_full_system.cpp** — 10 новых тестов:
  - `ExchangeV175` (4): BybitGetFuturesBalanceNoCrash, OKXGetFuturesBalanceNoCrash, KuCoinGetFuturesBalanceNoCrash, BitgetGetFuturesBalanceNoCrash
  - `ModelTrainerV175` (1): CandlesPerDayConfigurable — проверяет расчёт candles/day для 1m/5m/15m/1h
  - `SignalEnhancerV175` (1): ThreadSafeRecordOutcome — многопоточный стресс-тест recordOutcome()
  - `OnlineLearningV175` (1): LastTradeTimeAtomic — проверка компиляции atomic
  - `AppGuiV175` (1): ChartSettingsPersistence — компиляционная проверка
  - `BacktestV175` (1): ConfigurablePositionSize — тест с 50% позицией
  - `TaxReporterV175` (1): CostBasisPrecision — тест точности при partial fills
- **Итого тестов:** 463 (459 пройдено, 4 пропущено — LibTorch/XGBoost)

### Changed
- **CHANGELOG.md** — добавлена секция v1.7.5 со всеми исправлениями и новыми функциями.
- **ANALYSIS_AND_PROMPT.md** — обновлён на основе v1.7.5: исправленные проблемы отмечены ✅ Done, обновлён промт для следующего этапа v1.8.0.

## [1.7.4] - 2026-03-06

### Fixed

#### Этап 1: Exchange Bug Fixes — Аутентификация и HTTP методы
- **KuCoinExchange.cpp getPositionRisk()** — исправлена отсутствующая аутентификация: `httpGet(path)` → `httpGet(path, true)`. Эндпоинт `/api/v1/positions` требует подписи (KuCoinExchange.cpp:501). Без `signed=true` запрос отправлялся без API-ключей, получая ошибку 401.
- **BitgetExchange.cpp getPositionRisk()** — аналогичное исправление: `httpGet(path)` → `httpGet(path, true)`. Эндпоинт `/api/v2/mix/position/all-position` требует подписи (BitgetExchange.cpp:499).
- **BybitExchange.cpp getPositionRisk()** — замена `std::stod()` на `safeStod()` для всех полей позиции (size, avgPrice, unrealisedPnl, cumRealisedPnl, leverage). `std::stod()` бросает исключение на невалидных строках, `safeStod()` возвращает 0.0 (BybitExchange.cpp:446-451).
- **KuCoinExchange.cpp getPositionRisk()** — аналогичная замена `std::stod()` → `safeStod()` для всех числовых полей (KuCoinExchange.cpp:508-513).
- **BitgetExchange.cpp getPositionRisk()** — аналогичная замена `std::stod()` → `safeStod()` для всех числовых полей (BitgetExchange.cpp:506-510).
- **BinanceExchange.cpp cancelOrder()** — исправлен HTTP метод: `httpPost()` → `httpDelete()`. Binance API для отмены ордера требует DELETE-запрос на `/api/v3/order` (spot) и `/fapi/v1/order` (futures). Добавлен метод `httpDelete()` с поддержкой HMAC-SHA256 подписи и `CURLOPT_CUSTOMREQUEST "DELETE"` (BinanceExchange.cpp:794, 388-432).
- **KuCoinExchange.cpp cancelOrder()** — исправлен HTTP метод: `httpPost()` → `httpDelete()`. KuCoin API для отмены ордера требует DELETE-запрос на `/api/v1/orders/{orderId}`. Добавлен метод `httpDelete()` с подписью и `CURLOPT_CUSTOMREQUEST "DELETE"` (KuCoinExchange.cpp:588, 316-372).

#### Этап 2: WebhookServer Thread-Safety Fix
- **WebhookServer.cpp checkRateLimit()** — заменён `static int requestCount` на член класса `cleanupCounter_`. Static-переменная разделялась между всеми экземплярами WebhookServer, что нарушало изоляцию между серверами и создавало race condition при многопоточном доступе к разным экземплярам (WebhookServer.cpp:36-38, WebhookServer.h:71).

#### Этап 3: BacktestEngine Sharpe Ratio Fix
- **BacktestEngine.cpp computeMetrics()** — исправлен расчёт Sharpe ratio: std::dev теперь вычисляется как sample standard deviation (`sqrt(sq_sum / (n-1))`) вместо population standard deviation (`sqrt(sq_sum / n)`). Для финансовых данных sample stddev статистически корректнее (BacktestEngine.cpp:162-164).

### Added

#### Этап 4: UI/UX Improvements — Chart Order Placement & Leverage
- **AppGui.cpp — Контекстное меню правой кнопки мыши на графике** — добавлено контекстное меню при правом клике на графике (в зоне цены), позволяющее:
  - **Buy Limit** на выбранном уровне цены
  - **Sell Limit** на выбранном уровне цены
  - **Buy Stop-Limit** (stop price = уровень клика)
  - **Sell Stop-Limit** (stop price = уровень клика)
  - При выборе пункта автоматически заполняется Order Management и открывается окно (AppGui.cpp:1464-1503).
- **AppGui.cpp — Визуализация открытых ордеров на графике** — горизонтальные линии ордеров отображаются на графике: зелёные для BUY, красные для SELL. Каждая линия подписана (side, qty, price). Линии рисуются только для ордеров в видимом ценовом диапазоне (AppGui.cpp:1506-1526).
- **AppGui.cpp — Leverage Slider в Order Management** — добавлен слайдер "Leverage" (1x–125x) в секции Futures с кнопкой "Apply Leverage" для отправки через API (AppGui.cpp:3279-3290). Добавлен callback `SetLeverageCallback` и setter `setSetLeverageCallback()` (AppGui.h:233, 244, 357).
- **BinanceExchange.h/.cpp — httpDelete()** — новый метод для DELETE-запросов с HMAC-SHA256 подписью (BinanceExchange.h:108-109, BinanceExchange.cpp:388-432).
- **KuCoinExchange.h/.cpp — httpDelete()** — новый метод для DELETE-запросов с HMAC-SHA256+Base64 подписью (KuCoinExchange.h:64, KuCoinExchange.cpp:316-372).
- **AppGui.h** — новые поля: `omLeverage_` (int, default 1), `chartRightClickOpen_` (bool), `chartRightClickPrice_` (double).

#### Этап 5: Тестирование v1.7.4
- **test_full_system.cpp** — 8 новых тестов:
  - `WebhookV174` (1): RateLimitCleanupIsMemberVariable — проверяет что два экземпляра WebhookServer имеют независимые счётчики cleanup
  - `BacktestV174` (1): SharpeRatioSampleStddev — проверяет что Sharpe ratio конечное число
  - `AppGuiV174` (1): LeverageFieldDefault — компиляционная проверка наличия поля leverage
  - `ExchangeV174` (5): BybitGetPositionRiskSafeStod, KuCoinGetPositionRiskSigned, BitgetGetPositionRiskSigned, BinanceCancelOrderUsesDelete, KuCoinCancelOrderUsesDelete
- **Итого тестов:** 453 (449 пройдено, 4 пропущено — LibTorch/XGBoost)

### Changed
- **CHANGELOG.md** — добавлена секция v1.7.4 со всеми исправлениями и новыми функциями.
- **ANALYSIS_AND_PROMPT.md** — обновлён на основе v1.7.4: исправленные проблемы отмечены ✅ Done, обновлён промт для следующего этапа v1.8.0.

## [1.7.3] - 2026-03-06

### Fixed

#### Этап 1: Thread-Safety Fixes
- **Engine.h** — `componentsInitialized_` изменён с `bool` на `std::atomic<bool>` для предотвращения race condition при многопоточном доступе (Engine.h:95).
- **TradeHistory.cpp** — `winRate()` переписан с единым `lock_guard` на весь расчёт вместо двух раздельных вызовов `totalTrades()` + `winCount()`, каждый из которых отдельно захватывал мьютекс (TradeHistory.cpp:97-101). Устраняет TOCTOU race condition.
- **TradeHistory.cpp** — `profitFactor()` теперь корректно игнорирует breakeven-сделки (pnl==0): условие `else` заменено на `else if (t.pnl < 0)` (TradeHistory.cpp:128). Ранее pnl==0 ошибочно добавлял 0 в losses.

#### Этап 2: ML/AI Fixes
- **XGBoostModel.cpp** — устранена утечка памяти при повторном обучении: `XGBoosterFree(booster_)` вызывается перед созданием нового booster в `train()` (XGBoostModel.cpp:36). Ранее при каждом `train()` создавался новый booster без освобождения предыдущего.
- **LSTMModel.cpp** — исправлена числовая нестабильность loss-функции: `torch::log(out + 1e-9)` заменён на `torch::log_softmax(out, 1)` (LSTMModel.cpp:99). log_softmax численно стабильнее и математически корректнее для NLL loss.
- **ai/FeatureExtractor.cpp** — устранено деление на ноль в FVG detection и Order Block mid-points: добавлена проверка `ma > 0.0` перед делением на `ma` (ai/FeatureExtractor.cpp:254,257,266).

#### Этап 3: Settings & UI Fixes
- **AppGui.cpp configToJson()** — добавлено сохранение фильтров (`filterMinVolume`, `filterMinPrice`, `filterMaxPrice`, `filterMinChange`, `filterMaxChange`) и флагов индикаторов (`indEmaEnabled`, `indRsiEnabled`, `indAtrEnabled`, `indMacdEnabled`, `indBbEnabled`) в JSON. Ранее эти настройки терялись при перезапуске (AppGui.cpp:517-531).
- **AppGui.cpp loadConfig()** — добавлена загрузка секций `"filters"` и `"indicators_enabled"` из JSON при старте (AppGui.cpp:461-477).
- **AppGui.h** — синхронизированы дефолтные значения layout: `layoutVdPct{0.15f}` (было 0.13f), `layoutIndPct{0.20f}` (было 0.25f) — приведены в соответствие с LayoutManager.h (AppGui.h:196-197).
- **AppGui.cpp loadConfig()** — дефолтные значения при загрузке layout обновлены: `vd_pct` = 0.15f, `ind_pct` = 0.20f (AppGui.cpp:454-456).

#### Этап 4: Security & Utility Fixes
- **WebhookServer.cpp** — устранена утечка памяти в `rateLimit_` map: добавлен периодический cleanup (каждые 100 запросов удаляются записи старше 1 часа). Ранее map рос без ограничений (WebhookServer.cpp:29-39).
- **WebhookServer.cpp** — сравнение секрета заменено на constant-time: `CRYPTO_memcmp()` из OpenSSL вместо прямого `!=`. Устраняет уязвимость timing attack (WebhookServer.cpp:57-65).
- **CSVExporter.cpp** — добавлено экранирование CSV полей: поля содержащие запятые, кавычки или переводы строк оборачиваются в двойные кавычки, внутренние кавычки экранируются удвоением. Функция `escapeCsvField()` применяется ко всем строковым полям в `exportTrades()` (CSVExporter.cpp:10-22, 68-77).
- **OKXExchange.cpp getBalance()** — BTC баланс теперь использует `cashBal` вместо `availBal` для полного баланса (OKXExchange.cpp:351). `availBal` не учитывает замороженные средства.
- **RiskManager.h/.cpp** — параметр `highSince` переименован в `priceSinceEntry` для ясности: для LONG передаётся highest price, для SHORT — lowest price since entry. Устранена путаница в именовании (RiskManager.h:29-30, RiskManager.cpp:41).

### Added

#### Этап 5: Exchange Futures Support (Leverage API)
- **IExchange.h** — добавлены виртуальные методы `setLeverage(symbol, leverage)` → `bool` и `getLeverage(symbol)` → `int` с дефолтными реализациями (IExchange.h:188-198).
- **BinanceExchange** — реализованы:
  - `setLeverage()`: POST `/fapi/v1/leverage` с HMAC-SHA256 подписью (BinanceExchange.cpp:813-831)
  - `getLeverage()`: через `getPositionRisk()` → `leverage` (BinanceExchange.cpp:833-845)
  - `cancelOrder()`: POST/DELETE `/api/v3/order` (spot) или `/fapi/v1/order` (futures) (BinanceExchange.cpp:780-811)
- **BybitExchange** — реализованы:
  - `setLeverage()`: POST `/v5/position/set-leverage`, JSON body с buyLeverage/sellLeverage (BybitExchange.cpp:483-506)
  - `getLeverage()`: через `getPositionRisk()` → `leverage` (BybitExchange.cpp:508-520)
  - `cancelOrder()`: POST `/v5/order/cancel` (BybitExchange.cpp:462-481)
  - `getPositionRisk()`: GET `/v5/position/list?category=linear` (BybitExchange.cpp:430-460)
- **OKXExchange** — реализованы:
  - `setLeverage()`: POST `/api/v5/account/set-leverage`, JSON body (OKXExchange.cpp:672-694)
  - `getLeverage()`: через `getPositionRisk()` → `lever` (OKXExchange.cpp:696-708)
  - `cancelOrder()`: POST `/api/v5/trade/cancel-order` (OKXExchange.cpp:650-670)
- **KuCoinExchange** — реализованы:
  - `setLeverage()`: POST `/api/v2/changeLeverage` (KuCoinExchange.cpp:541-563)
  - `getLeverage()`: через `getPositionRisk()` → `realLeverage` (KuCoinExchange.cpp:565-577)
  - `cancelOrder()`: POST `/api/v1/orders/{orderId}` (KuCoinExchange.cpp:520-539)
  - `getPositionRisk()`: GET `/api/v1/positions` (KuCoinExchange.cpp:492-518)
- **BitgetExchange** — реализованы:
  - `setLeverage()`: POST `/api/v2/mix/account/set-leverage`, JSON body (BitgetExchange.cpp:551-576)
  - `getLeverage()`: через `getPositionRisk()` → `leverage` (BitgetExchange.cpp:578-590)
  - `cancelOrder()`: POST `/api/v2/spot/trade/cancel-order` (BitgetExchange.cpp:524-549)
  - `getPositionRisk()`: GET `/api/v2/mix/position/all-position` (BitgetExchange.cpp:490-522)

#### Этап 6: Тестирование v1.7.3
- **test_exchanges.cpp** — 29 новых тестов:
  - `ExchangeInterface` (2): SetLeverageDefaultReturnsFalse, GetLeverageDefaultReturns1
  - `ExchangeLeverage` (10): Set/GetLeverage NoCrash для всех 5 бирж
  - `ExchangeCancelOrder` (5): CancelOrder NoCrash для всех 5 бирж
  - `ExchangePositionRisk` (3): GetPositionRisk NoCrash для Bybit/KuCoin/Bitget
  - `GuiConfig` (3): ConfigSavesFilters, ConfigSavesIndicatorFlags, LayoutDefaultsMatchLayoutManager
- **test_full_system.cpp** — 6 новых тестов:
  - `TradeHistoryV173` (2): WinRateSingleLock, ProfitFactorIgnoresBreakeven
  - `CSVExporterV173` (1): EscapesCommasInFields
  - `WebhookV173` (1): SecretComparisonWorks
  - `RiskManagerV173` (2): TrailingStopLong, TrailingStopShort
- **Итого тестов:** 445 (441 пройден, 4 пропущено — LibTorch/XGBoost)

### Changed
- **CHANGELOG.md** — добавлена секция v1.7.3 со всеми исправлениями и новыми функциями.
- **ANALYSIS_AND_PROMPT.md** — обновлён на основе v1.7.3: исправленные проблемы отмечены ✅ Done, обновлён промт для следующего этапа v1.8.0.

## [1.7.2] - 2026-03-06

### Analysis & Testing
#### Полный анализ кода и глубокое тестирование всех модулей программы

**Проведено:**
- Полная сборка проекта (cmake + GCC 13.3, Debug) — **успешно, 0 ошибок**
- Полный запуск тест-сьюта: **416 тестов, 412 пройдено, 4 пропущено** (LibTorch/XGBoost отсутствуют)
- Глубокий анализ всех 5 бирж (Binance, Bybit, OKX, KuCoin, Bitget)
- Глубокий анализ торговых модулей (PaperTrading, BacktestEngine, RiskManager, GridBot, Scheduler)
- Глубокий анализ ML/AI модулей (LSTM, XGBoost, SignalEnhancer, RLTradingAgent, OnlineLearningLoop)
- Полный анализ UI layout (все 8 окон, все разрешения)
- Полный анализ настроек (settings.json, configToJson, loadConfig)
- Проверка фьючерсного режима торговли (Leverage через API)
- Верификация KuCoin API формата candles: подтверждено [time, open, close, high, low, volume] — код корректен

### Verified — Ранее исправленные проблемы (подтверждено v1.7.2)

**Биржи (6 исправлений подтверждены):**
- ✅ **placeOrder() для Bybit** — полная реализация: POST `/v5/order/create`, JSON body, подпись, обработка ошибок (BybitExchange.cpp:148-196)
- ✅ **placeOrder() для OKX** — полная реализация: POST `/api/v5/trade/order`, JSON body, подпись, парсинг ordId (OKXExchange.cpp:225-273)
- ✅ **placeOrder() для KuCoin** — полная реализация: POST `/api/v1/orders`, clientOid, подпись (KuCoinExchange.cpp:207-258)
- ✅ **placeOrder() для Bitget** — полная реализация: POST `/api/v2/spot/trade/place-order`, JSON body (BitgetExchange.cpp:214-260)
- ✅ **subscribeKline() для всех 4 бирж** — полная реализация с корректными подписками: Bybit (`kline.{interval}.{symbol}`), OKX (`candle{interval}`), KuCoin (`/market/candles:{symbol}_{interval}`), Bitget (`candle{interval}`)
- ✅ **KuCoin getKlines()** — формат [time, open, close, high, low, volume] подтверждён через официальную документацию KuCoin API; текущий код корректен (k[1]=open, k[2]=close, k[3]=high, k[4]=low). Ранее ошибочно классифицировано как баг (ложное срабатывание).

**Торговые модули (5 исправлений подтверждены):**
- ✅ **PaperTrading equity** — исправлен: `posValue += p.quantity * p.currentPrice` (PaperTrading.cpp:124)
- ✅ **BacktestEngine equity для SHORT** — исправлен: `equity = balance + posQty * (2.0 * entryPrice - price)` (BacktestEngine.cpp:108)
- ✅ **GridBot profit** — исправлен: `realizedProfit_ += (lvl.price - lvl.buyPrice) * qty` с проверкой quantity (GridBot.cpp:64-70)
- ✅ **GridLevel struct** — добавлены поля `buyPrice` и `quantity` (GridBot.h:21-22)
- ✅ **Scheduler callback** — исправлен: `if (job.callback) { try { job.callback(); } ... }` (Scheduler.cpp:127-134)

**ML/AI модули (4 исправления подтверждены):**
- ✅ **ml/FeatureExtractor division by zero** — проверки: `ema > 0.0 ?`, `hl > 0.0 ?` (FeatureExtractor.cpp:82,87)
- ✅ **SignalEnhancer нормализация весов** — веса нормализуются до суммы 1.0 (SignalEnhancer.cpp:71-78)
- ✅ **RLTradingAgent GAE bootstrap** — исправлен: `nextVal = (t + 1 < T) ? values[t + 1] : 0.0f` (RLTradingAgent.cpp:303)
- ✅ **Bybit rate limiting** — реализован скользящее окно с sleep логикой (BybitExchange.cpp:57-70)

### Known Issues — Обнаруженные оставшиеся проблемы

**Критические (влияют на торговлю):**
- ❌ **Фьючерсное плечо (Leverage)** — отсутствуют методы `setLeverage()` / `getLeverage()` для установки плеча через API. Leverage отображается только из данных позиций (Binance, OKX). Bybit, KuCoin, Bitget не имеют `getPositionRisk()`. Невозможно задать плечо перед открытием фьючерсной позиции.
- ❌ **cancelOrder()** — не реализован ни для одной биржи

**Высокие (влияют на корректность):**
- ❌ **XGBoost memory leak** — `XGBoosterFree(booster_)` не вызывается перед повторным `train()` (XGBoostModel.cpp:36)
- ❌ **Engine::componentsInitialized_** — тип `bool` вместо `std::atomic<bool>` (Engine.h:95)
- ❌ **TradeHistory::winRate() race condition** — три раздельных lock в одном вычислении (TradeHistory.cpp:97-100)
- ❌ **LSTM loss** — `torch::log(out + 1e-9)` вместо `torch::log_softmax()` (LSTMModel.cpp:99)
- ❌ **ai/FeatureExtractor FVG** — деление на `ma` без проверки > 0 (ai/FeatureExtractor.cpp:254,257)

**Средние (функциональные дефекты):**
- ❌ **AppGui configToJson()** — не сохраняет фильтры (filterMinVolume/Price/Change) и флаги индикаторов (indEmaEnabled и др.)
- ❌ **Layout defaults mismatch** — LayoutManager.h: vdPct=0.15, indPct=0.20 vs AppGui.h: layoutVdPct=0.13, layoutIndPct=0.25
- ❌ **OnlineLearningLoop** — использует текущий state вместо state на момент входа в сделку (OnlineLearningLoop.cpp:131)
- ❌ **WebhookServer rateLimit_** — map растёт без ограничений (WebhookServer.cpp:33)
- ❌ **CSVExporter** — нет экранирования запятых/кавычек в полях (CSVExporter.cpp:38-71)
- ❌ **WebhookServer** — сравнение секрета не constant-time (timing attack)
- ❌ **NewsFeed::fetch()** — заглушка, данные не получаются (NewsFeed.cpp:46-50)
- ❌ **FearGreed::fetch()** — заглушка (FearGreed.cpp:32-36)
- ❌ **TelegramBot::sendMessage()** — заглушка, сообщения не отправляются (TelegramBot.cpp:65-71)
- ❌ **RiskManager trailing stop** — параметр `highSince` для SHORT конфузен (должен быть `lowestSince`)

### Changed
- **ANALYSIS_AND_PROMPT.md** — полностью обновлён на основе анализа v1.7.2: 15 ранее исправленных проблем отмечены как Done, обновлён промт для следующего этапа разработки, добавлен анализ фьючерсного плеча (Leverage API), обновлена статистика тестов и покрытия.

## [1.7.1] - 2026-03-06

### Fixed
#### Исправление ошибок компиляции Windows 10 Pro x64 (VS2019)
- **CMakeLists.txt** — исправлена ошибка «Недопустимое число параметров» (`xcopy`) при копировании LibTorch DLL в каталог сборки. Команда `xcopy /Y /D` заменена на кроссплатформенный `cmake -E copy_if_different` с предварительным `file(GLOB *.dll)` на этапе configure. Путь к DLL теперь определяется динамически из переменной `Torch_DIR` вместо жёстко заданного `C:\libs\libtorch\lib`.
- **src/ml/FeatureExtractor.cpp** — устранено предупреждение C4005 (переопределение макроса `_USE_MATH_DEFINES`). Макрос уже определяется глобально через `add_definitions(-D_USE_MATH_DEFINES)` в CMakeLists.txt; добавлена защита `#ifndef _USE_MATH_DEFINES` перед повторным `#define`.

### Verified
- **Compile_Error_06_03_2026** — полный анализ лога сборки (6262 строки). Идентифицировано: 1 критическая ошибка (xcopy POST_BUILD), 1 предупреждение проекта (C4005), прочие предупреждения — из заголовков LibTorch и стандартной библиотеки MSVC (C4244, C4267, C4324), не требующие исправления.

## [1.7.0] - 2026-03-05

### Added
#### Стресс-тестирование и интеграция LibTorch + XGBoost
- **Стресс-тесты v1.7.0** (`tests/test_v170.cpp`) — 46 новых тестов, охватывающих все основные модули:
  - **LayoutStress** (17 тестов): проверка размеров окон (PairList=200, UserPanel=290, MarketData=W-490, TopBar=32, Logs=120), отсутствие наложения между всеми 8 окнами, 9 разрешений экрана (от 800×600 до 3840×2160), viewport offset, флаги видимости, кастомные пропорции.
  - **AIStress** (11 тестов): SPSCQueue high-throughput (10K элементов, FIFO), RLTradingAgent (1000 forward pass, 10 train step, mode switching, save/load), AIFeatureExtractor (500 свечей, 100 depth updates, SMC паттерны), OnlineLearningLoop (start/stop, experience batch).
  - **MLStress** (13 тестов): XGBoost (конструктор, train/predict, save/load roundtrip, feature importance), LSTM (конструктор, predict без обучения, addSample, train, save/load), SignalEnhancer (enhance, recordOutcome × 200), ModelTrainer (300 свечей).
  - **IntegrationStress** (5 тестов): AIFeatureExtractor→RLTradingAgent pipeline, IndicatorEngine→FeatureExtractor→Models pipeline, 1000 пересчётов layout, проверка USE_LIBTORCH и USE_XGBOOST флагов.
- **CMakeLists.txt** — раскомментирован `tests/test_v170.cpp` в списке тестовых файлов.

### Changed
- **LayoutManager.cpp** — ширина панели Pair List изменена с 210px на 200px для соответствия спецификации UI-диаграммы: PairList(200) + UserPanel(290) = 490, MarketData = W−490. Обеспечивает точное тайловое расположение без наложения окон.
- **test_full_system.cpp** — обновлены ожидаемые значения в тестах `ThreeColumnLayout` и `VisibilityHidesWindow` для PairList=200px (ранее 210px).
- **test_regression_part3.cpp** — обновлён тест `WindowsAreFullWidth`: центральная колонка = screenW − 200 − 290 (ранее screenW − 210 − 290).

### Verified
- **LibTorch 2.6.0 CPU** — успешная сборка и интеграция. RLTradingAgent корректно инициализируется на CPU (state_dim=128, hidden=256, actions=3). Forward pass, train_step (PPO), save/load модели работают без ошибок.
- **XGBoost 2.1.3** — успешная сборка из исходников и линковка через `xgboost::xgboost`. Обучение (train), предсказание (predict), сохранение/загрузка модели (UBJSON формат) работают корректно.
- **CMakeLists.txt** — `find_package(Torch QUIET)` и `find_package(XGBoost QUIET)` корректно находят библиотеки. Определения `USE_LIBTORCH` и `USE_XGBOOST` передаются как `PUBLIC` и наследуются тестовым таргетом.
- **CMakePresets.json** — пресеты Windows (VS2019 Release/Debug) с путями к LibTorch и XGBoost корректны. Linux сборка работает без пресетов через ручную передачу `-DTorch_DIR` и `-DXGBoost_DIR`.
- **Полный тест-сьют** — 414 тестов пройдено, 2 пропущено (LiveData — требуют сеть), 0 ошибок.

## [1.6.0] - 2026-03-XX

### Added
#### Dynamic Tile Layout System & AI Status Logging
- **LayoutManager::recalculate()** (`src/ui/LayoutManager.cpp`) — полностью переработан метод расчёта позиций и размеров 8 окон Tile Layout. Реализована математически точная модель разбиения viewport: фиксированные константы (Htop=32, Hfilters=28, Hlogs=120, Wleft=210, Wright=290), динамические пропорции центральной колонки (vdPct, indPct, Market Data = remainder), защита от отрицательных размеров через `std::max(0.0f, ...)`.
- **Dynamic Panel Toggling** — добавлена поддержка флагов видимости (`showPairList`, `showUserPanel`, `showVolumeDelta`, `showIndicators`, `showLogs`) в `LayoutManager::recalculate()`. При скрытии панели её площадь обнуляется, соседние окна автоматически расширяются. Тернарная логика для W_left^active, W_right^active, H_logs^active, H_vd^active, H_ind^active.
- **Viewport Offset Support** — `recalculate()` теперь принимает `offsetX`/`offsetY` (WorkPos), обеспечивая корректное позиционирование при наличии системных элементов (меню, taskbar).
- **AI Status Indicator** — в тулбаре отображается текущий статус ИИ: `[AI: Working]` (торговля активна + LibTorch), `[AI: Enabled]` (подключено + LibTorch), `[AI: Idle]` (ожидание), `[AI: Disabled]` (без LibTorch).
- **AI Log Messages** — логирование действий ИИ с тегом `[AI]` при подключении, отключении, старте и остановке торговли. Цветовое выделение `[AI]` записей в окне Logs (фиолетовый).

### Changed
- **Window Locking** — все 8 окон Tile Layout теперь всегда фиксированы (`ImGuiCond_Always` + `NoMove | NoResize | NoCollapse`). Убрана зависимость от `layoutLocked` / `layoutNeedsReset_` для позиционирования. ImGui.ini больше не перезаписывает позиции.
- **LayoutManager.h** — расширена сигнатура `recalculate()` с обратной совместимостью (default-параметры для offset и show flags). Добавлены `#include <algorithm>` и `#include <cmath>` для `std::max` и `std::floor`.

#### AI & Machine Learning Engine (LibTorch)
- **RLTradingAgent** (`src/ai/RLTradingAgent.h/.cpp`) — интеграция PyTorch C++ API (LibTorch) для запуска нейронных сетей в рантайме. Имплементация Continuous Reinforcement Learning (PPO) с вычислением GAE (Generalized Advantage Estimation) через тензорные операции.
- **Dual-Mode Logic** — внедрены два режима ИИ: `Fast Mode` (сверхбыстрый анализ L2 Orderbook напрямую из `DepthStreamManager`) и `Swing Mode` (анализ трендов и MTF матриц на базе `CandleCache`).
- **FeatureExtractor** (`src/ai/FeatureExtractor.h/.cpp`) — высокопроизводительный модуль формирования вектора состояний без динамических аллокаций. Встроена жесткая математическая логика детектирования Smart Money Concepts: Swing Failure Patterns (SFP) и Fair Value Gaps (FVG).
- **OnlineLearningLoop** (`src/ai/OnlineLearningLoop.h/.cpp`) — выделенный фоновый поток (`std::jthread`), реализующий lock-free буфер реплея (Replay Buffer). Автоматически забирает результаты сделок (PnL) из `TradeRepository` для формирования функции Reward и обновления весов градиентным спуском (AdamW) без блокировки основного торгового потока.
- **SPSCQueue** (`src/ai/SPSCQueue.h`) — lock-free SPSC кольцевой буфер с выравниванием по cache-line для передачи данных между потоками без блокировок.
- **Тесты AI** (`tests/test_ai_engine.cpp`) — 33 теста: SPSCQueue (push/pop/FIFO/capacity/move), RLTradingAgent (конструктор/forward_pass/train_step/mode/save-load/PPOConfig), AIFeatureExtractor (SFP/FVG/OrderBlock/Depth/Swing/Fast features), OnlineLearningLoop (start/stop/pushExperience), интеграция FeatureExtractor→Agent и Dual-Mode.

#### Binance API: Rate Limits & Iceberg Orders (март 2026)
- **BinanceExchange.h** — добавлены константы лимитов ордеров: `MAX_ORDERS_PER_10S_SPOT = 100` (100 ордеров/10 сек, спот), `MAX_ORDERS_PER_24H_SPOT = 200000` (200K ордеров/24 часа, спот), `MAX_ORDERS_PER_MINUTE_FUTURES = 1200` (1200 ордеров/мин, фьючерсы).
- **BinanceExchange.h** — добавлен лимит WebSocket подключений: `MAX_WS_CONNECTIONS_PER_5MIN = 300` (300 соединений за 5 минут с одного IP).
- **BinanceExchange.h** — добавлена константа `MAX_ICEBERG_PARTS = 100` (максимальное количество частей айсберг-ордера, увеличено до 100 для всех символов с 12 марта 2026).
- **BinanceExchange.cpp** — реализован `orderRateLimit()`: скользящее окно для отслеживания количества ордеров (10 сек для спота, 1 мин для фьючерсов) с суточным счётчиком (200K/24ч). Вызывается из `placeOrder()`.
- **BinanceExchange.cpp** — реализован `wsConnectionRateLimit()`: скользящее окно 5 минут для ограничения WebSocket подключений (300/5мин). Вызывается из `subscribeKline()`.
- **BinanceExchange.cpp** — добавлен комментарий-предупреждение о депрекации потоков `!ticker@arr` (отключение 26 марта 2026); программа использует только индивидуальные потоки `<symbol>@kline_<interval>`.

#### Балансы бирж: Реализация getBalance()
- **BybitExchange.cpp** — реализован `getBalance()`: запрос GET `/v5/account/wallet-balance?accountType=UNIFIED`, парсинг USDT/BTC балансов из массива `result.list[0].coin`. Ранее возвращал пустой `AccountBalance`.
- **OKXExchange.cpp** — реализован `getBalance()`: запрос GET `/api/v5/account/balance` (signed), парсинг USDT (`eqUsd`/`availBal`) и BTC из `data[0].details`. Ранее возвращал пустой `AccountBalance`.
- **KuCoinExchange.cpp** — реализован `getBalance()`: запрос GET `/api/v1/accounts` (signed), парсинг USDT/BTC балансов по `currency` + `type == "trade"`. Ранее возвращал пустой `AccountBalance`.
- **BitgetExchange.cpp** — реализован `getBalance()`: запрос GET `/api/v2/spot/account/assets` (signed), парсинг USDT (`available` + `frozen`) и BTC. Ранее возвращал пустой `AccountBalance`.

#### Тесты
- **test_exchanges.cpp** — добавлено 13 тестов: `BinanceRateLimits` (7 тестов: SpotRequestLimit, FuturesRequestLimit, SpotOrderLimitPer10s, SpotOrderLimitPer24h, FuturesOrderLimitPerMinute, WebSocketConnectionLimit, IcebergPartsLimit), `ExchangeBalance` (6 тестов: BybitGetBalanceNoCrash, OKXGetBalanceNoCrash, KuCoinGetBalanceNoCrash, BitgetGetBalanceNoCrash, BinanceGetBalanceSpotNoCrash, BinanceGetBalanceFuturesNoCrash).

### Changed
- **CMakeLists.txt** — добавлена интеграция `find_package(Torch QUIET)` и линковка тензорных библиотек к `crypto_lib`. Включены `TORCH_INCLUDE_DIRS` и `TORCH_CXX_FLAGS`. Добавлен `AI_SOURCES` блок. Для подключения LibTorch: `cmake -DTorch_DIR=<libtorch>/share/cmake/Torch` или `cmake -DCMAKE_PREFIX_PATH=<libtorch>`. Скачать: https://pytorch.org/get-started/locally/ (LibTorch C++ / CPU или CUDA). Обновлены таргеты для поддержки аппаратного ускорения при наличии.

### Fixed
- **AppGui.cpp** — добавлены отсутствующие `#include <windows.h>` и `#include <GLFW/glfw3native.h>` в блоке `#ifdef _WIN32` для корректной компиляции Win32 API вызовов (`HICON`, `HWND`, `LoadImageA`, `SendMessageA`, `glfwGetWin32Window`). Устраняет ~30 ошибок компиляции C2065/C2146/C3861 на MSVC.
- **AppGui.cpp** — устранено предупреждение C4005 (APIENTRY macro redefinition) при компиляции MSVC: `#include <windows.h>` перенесён перед `#include <GLFW/glfw3.h>`, добавлен `#undef APIENTRY` после GLFW для предотвращения конфликта макроопределений из `minwindef.h`.
- **app_resource.rc** — исправлен путь к иконке: `"resources/app_icon.ico"` → `"app_icon.ico"` (путь относительно каталога .rc файла). Устраняет ошибку RC2135 при компиляции ресурсов Windows.
- **AppGui.cpp / Dashboard.cpp** — формат отображения цен и балансов изменён с `%.2f`/`%.4f` на `%.8f` для поддержки полной точности API бирж (формат `X.XXXXXXXX`). Затронуты: OHLCV, EMA, ATR, Bollinger Bands, VWAP, SL/TP, ордера, позиции, PnL, балансы, эквити. Процентные значения (RSI, drawdown, confidence) и коэффициенты (Sharpe, PF) сохранены в прежнем формате.
- **AppGui.cpp (Settings > Exchange)** — улучшены метки полей для ясности live-режима: `Base URL` → `Base URL (Spot)`, `WS Host` → `WS Host (Futures)`, `WS Port` → `WS Port (Futures, default 9443)`. Спотовые WS поля (`Spot WS Host`, `Spot WS Port`) и `Futures Base URL` вынесены в секцию Advanced. Добавлена автоподстановка дефолтных URL при переключении Testnet (Binance): production — `api.binance.com`, `fstream.binance.com`, порт `9443`; testnet — `testnet.binance.vision`, `fstream.binancefuture.com`, порт `443`.
- **test_regression_part2.cpp** — устранено предупреждение `-Wunused-variable` для переменной `reporter` в тесте `TaxReporter::ExportCSV` (добавлен `(void)reporter`).
- **test_full_system.cpp** — устранено предупреждение `-Wunused-but-set-variable` для `parseOk` в тесте `Config_CorruptJsonUsesDefaults`: переменная теперь используется в `EXPECT_FALSE(parseOk)` для проверки что некорректный JSON не парсится.

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
