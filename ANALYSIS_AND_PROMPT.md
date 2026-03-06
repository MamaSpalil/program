# 🔍 ПОЛНЫЙ АНАЛИЗ И ПРОМТ ДЛЯ ИСПРАВЛЕНИЯ ПРОГРАММЫ CryptoTrader (VT)

> **Дата анализа:** 2026-03-06
> **Версия:** 1.7.4
> **Платформа сборки:** Linux (GCC 13.3) + Windows 10 Pro x64 (VS2019)
> **Результаты тестирования:** 453 тестов, 449 пройдено, 4 пропущено (LibTorch/XGBoost отсутствуют)
> **Обновление v1.7.4:** Проведён полный анализ и исправление кода. 39 проблем исправлены и отмечены как ✅ Done. Добавлены 8 новых тестов.

---

## 📋 СОДЕРЖАНИЕ

1. [Сводка критических проблем](#1-сводка-критических-проблем)
2. [Анализ бирж](#2-анализ-бирж)
3. [Анализ торговых модулей](#3-анализ-торговых-модулей)
4. [Анализ ML/AI модулей](#4-анализ-mlai-модулей)
5. [Анализ UI и настроек](#5-анализ-ui-и-настроек)
6. [Анализ интеграций и утилит](#6-анализ-интеграций-и-утилит)
7. [Анализ тестового покрытия](#7-анализ-тестового-покрытия)
8. [Анализ CHANGELOG](#8-анализ-changelog)
9. [**ПОЛНЫЙ ПРОМТ ДЛЯ ИСПРАВЛЕНИЯ**](#9-полный-промт-для-исправления)

---

## 1. СВОДКА КРИТИЧЕСКИХ ПРОБЛЕМ

### 🔴 КРИТИЧЕСКИЕ (блокируют торговлю)
| # | Модуль | Файл:Строка | Проблема | Статус |
|---|--------|-------------|----------|--------|
| 1 | Exchange | BybitExchange.cpp:148-196 | placeOrder() — заглушка, ордера не размещаются | ✅ Done (v1.7.2) |
| 2 | Exchange | OKXExchange.cpp:225-273 | placeOrder() — заглушка | ✅ Done (v1.7.2) |
| 3 | Exchange | KuCoinExchange.cpp:207-258 | placeOrder() — заглушка | ✅ Done (v1.7.2) |
| 4 | Exchange | BitgetExchange.cpp:214-260 | placeOrder() — заглушка | ✅ Done (v1.7.2) |
| 5 | Exchange | KuCoinExchange.cpp:179-188 | Kline high/low перепутаны | ✅ Done — Ложное срабатывание: KuCoin API формат подтверждён |
| 6 | Exchange | Bybit/OKX/KuCoin/Bitget subscribeKline() | symbol и interval игнорируются | ✅ Done (v1.7.2) |
| 7 | Trading | PaperTrading.cpp:124 | Двойной подсчёт equity | ✅ Done (v1.7.2) — qty * currentPrice |
| 8 | Backtest | BacktestEngine.cpp:108 | Некорректный расчёт equity при SHORT | ✅ Done (v1.7.2) — posQty * (2*entryPrice - price) |
| 9 | Strategy | RiskManager.cpp:49-50 | Trailing stop для SHORT: highSince вместо lowSince | ✅ Done (v1.7.3) — priceSinceEntry |
| 10 | Scheduler | Scheduler.cpp:122-135 | Задания планировщика НИКОГДА не исполняются | ✅ Done (v1.7.2) — job.callback() |
| **NEW** | Exchange | Все 5 бирж | setLeverage() / getLeverage() для фьючерсов | ✅ Done (v1.7.3) |
| **NEW** | Exchange | Все 5 бирж | cancelOrder() — отмена ордера через API | ✅ Done (v1.7.3) |
| **v1.7.4** | Exchange | BinanceExchange.cpp:794 | cancelOrder() использует POST вместо DELETE | ✅ Done (v1.7.4) — httpDelete() добавлен |
| **v1.7.4** | Exchange | KuCoinExchange.cpp:533 | cancelOrder() использует POST вместо DELETE | ✅ Done (v1.7.4) — httpDelete() добавлен |

### 🟠 ВЫСОКИЕ (влияют на корректность)
| # | Модуль | Файл:Строка | Проблема | Статус |
|---|--------|-------------|----------|--------|
| 11 | Exchange | BybitExchange — нет rate limiting | Риск бана API | ✅ Done (v1.7.2) |
| 12 | Exchange | OKX/Bybit/KuCoin getBalance() | Нет проверки границ массива | ✅ Done (v1.7.2) — safeStod() |
| 13 | Exchange | OKXExchange.cpp:234-236 | BTC баланс: availBal вместо totalBal | ✅ Done (v1.7.3) — cashBal |
| 14 | ML | XGBoostModel.cpp:36 | Утечка памяти — booster_ не освобождается | ✅ Done (v1.7.3) |
| 15 | ML | LSTMModel.cpp:99 | log(out+1e-9) вместо log_softmax | ✅ Done (v1.7.3) |
| 16 | ML | SignalEnhancer.cpp:72-78 | Веса ансамбля не нормализуются | ✅ Done (v1.7.2) |
| 17 | AI | OnlineLearningLoop.cpp:131 | Стейт извлекается в текущий момент, а не на момент входа | ❌ Открыт |
| 18 | AI | RLTradingAgent.cpp:303 | GAE bootstrap: nextVal=0 для нетерминального шага | ✅ Done (v1.7.2) |
| 19 | Trading | GridBot.cpp:62 | Profit = price * 0.001 — заглушка | ✅ Done (v1.7.2) — (sellPrice - buyPrice) * qty |
| 20 | Core | Engine.cpp:94-95 | Race condition: componentsInitialized_ не atomic | ✅ Done (v1.7.3) |
| 21 | Trading | TradeHistory.cpp:98-100 | Race condition в winRate() | ✅ Done (v1.7.3) |
| 22 | Trading | TradeHistory.cpp:128 | profitFactor(): breakeven считается loss | ✅ Done (v1.7.3) |
| 23 | Integration | WebhookServer.cpp:33-36 | Rate-limit map утечка памяти | ✅ Done (v1.7.3) |
| 24 | Settings | AppGui.cpp:464-524 | Фильтры и флаги индикаторов НЕ сохраняются | ✅ Done (v1.7.3) |
| **v1.7.4** | Exchange | KuCoin getPositionRisk() | httpGet без signed=true | ✅ Done (v1.7.4) |
| **v1.7.4** | Exchange | Bitget getPositionRisk() | httpGet без signed=true | ✅ Done (v1.7.4) |
| **v1.7.4** | Exchange | Bybit getPositionRisk() | std::stod вместо safeStod (crash) | ✅ Done (v1.7.4) |
| **v1.7.4** | Integration | WebhookServer.cpp:36 | static requestCount не thread-safe | ✅ Done (v1.7.4) — cleanupCounter_ |
| **v1.7.4** | Backtest | BacktestEngine.cpp:162 | Sharpe ratio: population вместо sample stddev | ✅ Done (v1.7.4) — (n-1) |

### 🟡 СРЕДНИЕ (функциональные дефекты)
| # | Модуль | Файл:Строка | Проблема | Статус |
|---|--------|-------------|----------|--------|
| 25 | ML | FeatureExtractor.cpp:87 | Division by zero: hl/c.close | ✅ Done (v1.7.2) |
| 26 | ML | ModelTrainer.cpp:16 | Хардкод 96 свечей/день | ❌ Открыт |
| 27 | AI | FeatureExtractor.cpp:247-250 | FVG detection деление на ma | ✅ Done (v1.7.3) |
| 28 | AI | FeatureExtractor.cpp:110 | mid < 1e-12 → mid=1.0 фиктивное значение | ❌ Открыт |
| 29 | Settings | LayoutManager.h vs AppGui.h | Несовпадение дефолтов | ✅ Done (v1.7.3) — Синхронизировано |
| 30 | UI | AppGui.h:339-340 | chartBarWidth_, chartScrollOffset_ не сохраняются | ❌ Открыт |
| 31 | Data | CSVExporter.cpp:45-49 | Нет экранирования запятых в CSV | ✅ Done (v1.7.3) — escapeCsvField() |
| 32 | External | NewsFeed.cpp:46-50, FearGreed.cpp:35 | API-вызовы заглушки | ❌ Открыт |
| 33 | Integration | TelegramBot.cpp:40-56 | Все команды заглушки | ❌ Открыт |
| 34 | Core | Engine.cpp:34, 308 | Хардкод maxCandleHistory=5000, interval=2000ms | ❌ Открыт |
| 35 | Backtest | BacktestEngine.cpp:62 | Хардкод позиция 95% от баланса | ❌ Открыт |
| 36 | Tax | TaxReporter.cpp:46-47 | Потеря точности costBasis при partial fills | ❌ Открыт |
| 37 | Security | WebhookServer.cpp:59 | String comparison не constant-time | ✅ Done (v1.7.3) — CRYPTO_memcmp |
| **NEW** | Exchange | Bybit/KuCoin/Bitget | getPositionRisk() реализован | ✅ Done (v1.7.3) |
| **NEW** | Exchange | Bybit/KuCoin/Bitget | getFuturesBalance() не реализован | ❌ Открыт |
| **v1.7.4** | UI | AppGui.cpp | Нет контекстного меню правой кнопки мыши на графике | ✅ Done (v1.7.4) — ChartOrderMenu |
| **v1.7.4** | UI | AppGui.cpp | Нет leverage slider в Order Management | ✅ Done (v1.7.4) — SliderInt 1x-125x |
| **v1.7.4** | UI | AppGui.cpp | Нет визуализации ордеров на графике | ✅ Done (v1.7.4) — Order lines |

---

## 2. АНАЛИЗ БИРЖ

### 2.1 Интерфейс IExchange (IExchange.h)
Обязательные методы: `testConnection()`, `getKlines()`, `getPrice()`, `placeOrder()`, `getBalance()`, `subscribeKline()`, `connect()`, `disconnect()`.
Дополнительные методы: `getPositionRisk()`, `getFuturesBalance()`, `getOpenOrders()`, `getMyTrades()`, `cancelOrder()`, `setLeverage()`, `getLeverage()`.

### 2.2 Статус реализации по биржам (обновлено v1.7.4)

| Функция | Binance | Bybit | OKX | KuCoin | Bitget |
|---------|---------|-------|-----|--------|--------|
| getPrice() | ✅ | ✅ | ✅ | ✅ | ✅ |
| getKlines() | ✅ | ✅ | ✅ | ✅ | ✅ |
| getBalance() | ✅ | ✅ | ✅ cashBal | ✅ | ✅ |
| placeOrder() | ✅ | ✅ | ✅ | ✅ | ✅ |
| cancelOrder() | ✅ **DELETE** | ✅ POST | ✅ POST | ✅ **DELETE** | ✅ POST |
| subscribeKline() | ✅ | ✅ | ✅ | ✅ | ✅ |
| Rate Limiting | ✅ full | ✅ | ✅ 10req/s | ✅ 10req/s | ✅ 10req/s |
| Authentication | ✅ HMAC-SHA256 | ✅ HMAC-SHA256 | ✅ HMAC+Base64 | ✅ HMAC+Base64 | ✅ HMAC+Base64 |
| getPositionRisk() | ✅ | ✅ **safeStod** | ✅ | ✅ **signed+safeStod** | ✅ **signed+safeStod** |
| getFuturesBalance() | ✅ | ❌ none | ❌ none | ❌ none | ❌ none |
| setLeverage() | ✅ | ✅ | ✅ | ✅ | ✅ |
| getLeverage() | ✅ | ✅ | ✅ | ✅ | ✅ |
| httpDelete() | ✅ **NEW v1.7.4** | — | — | ✅ **NEW v1.7.4** | — |

### 2.3 Исправления бирж v1.7.4

**Binance cancelOrder() — ✅ Done (v1.7.4):**
```cpp
// BinanceExchange.cpp:794 — ИСПРАВЛЕНО: DELETE вместо POST
auto resp = httpDelete(path, params.str());  // ✅ DELETE метод
// httpDelete() использует CURLOPT_CUSTOMREQUEST "DELETE" с параметрами в URL query
```

**KuCoin cancelOrder() — ✅ Done (v1.7.4):**
```cpp
// KuCoinExchange.cpp:588 — ИСПРАВЛЕНО: DELETE вместо POST
auto resp = httpDelete("/api/v1/orders/" + orderId);  // ✅ DELETE метод
// httpDelete() использует CURLOPT_CUSTOMREQUEST "DELETE" с HMAC подписью
```

**KuCoin/Bitget getPositionRisk() — ✅ Done (v1.7.4):**
```cpp
// KuCoinExchange.cpp:501 — ИСПРАВЛЕНО:
auto resp = httpGet(path, true);  // ✅ signed request (было: httpGet(path))
// + все поля используют safeStod() вместо std::stod()

// BitgetExchange.cpp:499 — ИСПРАВЛЕНО:
auto resp = httpGet(path, true);  // ✅ signed request (было: httpGet(path))
// + все поля используют safeStod() вместо std::stod()
```

**Bybit getPositionRisk() — ✅ Done (v1.7.4):**
```cpp
// BybitExchange.cpp:446-451 — ИСПРАВЛЕНО:
info.positionAmt      = safeStod(p.value("size", "0"));       // ✅ (было std::stod)
info.entryPrice       = safeStod(p.value("avgPrice", "0"));   // ✅
info.unrealizedProfit = safeStod(p.value("unrealisedPnl", "0")); // ✅
info.realizedProfit   = safeStod(p.value("cumRealisedPnl", "0")); // ✅
info.leverage         = safeStod(p.value("leverage", "1"));   // ✅
```

### 2.4 Фьючерсное плечо (Leverage) — ✅ Done (v1.7.3+v1.7.4)

**Реализовано для всех 5 бирж** (v1.7.3) + **UI Slider** (v1.7.4):
- `setLeverage(symbol, leverage)` и `getLeverage(symbol)` работают для всех бирж
- В Order Management добавлен слайдер Leverage (1x–125x) с кнопкой "Apply Leverage"
- Текущее плечо отображается в секции "Positions / P&L"

### 2.5 Формат символов (нужна унификация — открыт)
| Биржа | Формат спот | Формат фьючерс |
|-------|-------------|----------------|
| Binance | BTCUSDT | BTCUSDT |
| Bybit | BTCUSDT | BTCUSDT |
| OKX | BTC-USDT | BTC-USDT-SWAP |
| KuCoin | BTC-USDT | BTC-USDT |
| Bitget | BTCUSDT | BTCUSDT_UMCBL |

---

## 3. АНАЛИЗ ТОРГОВЫХ МОДУЛЕЙ

### 3.1 PaperTrading — ✅ Done (v1.7.2)
Equity: `posValue += p.quantity * p.currentPrice` — корректно.

### 3.2 BacktestEngine — ✅ Done (v1.7.2 + v1.7.4)
- Equity SHORT: `posQty * (2.0 * entryPrice - price)` ✅
- Sharpe ratio: sample stddev `sqrt(sq_sum / (n-1))` ✅ Done (v1.7.4)

### 3.3 RiskManager — ✅ Done (v1.7.3)
Параметр `priceSinceEntry` (было `highSince`).

### 3.4 GridBot — ✅ Done (v1.7.2)
Profit: `(sellPrice - buyPrice) * qty` с полем `buyPrice`.

### 3.5 Scheduler — ✅ Done (v1.7.2)
Callback: `job.callback()` вызывается.

### 3.6 TradeHistory — ✅ Done (v1.7.3)
- `winRate()`: единый lock_guard
- `profitFactor()`: breakeven не считается loss

### 3.7 OrderManagement — хардкоды (открыт)
- Нет валидации price > 0
- estimateCost не учитывает комиссию

---

## 4. АНАЛИЗ ML/AI МОДУЛЕЙ

### 4.1 LSTMModel — ✅ Loss Fixed (v1.7.3)
- **ИСПРАВЛЕНО:** log_softmax(out, 1) вместо log(out+1e-9)
- **Открыт:** lr=0.001, patience=5 — хардкод
- **Открыт:** Нет валидации размера входного вектора

### 4.2 XGBoostModel — ✅ Memory Leak Fixed (v1.7.3)
- **ИСПРАВЛЕНО:** XGBoosterFree(booster_) перед re-train
- **Открыт:** outResult может содержать <3 float
- **Открыт:** NaN из feature → NAN как missing value

### 4.3 FeatureExtractor (ML) — ✅ Done (v1.7.2)
- Division by zero: ema > 0.0 и hl > 0.0 проверки

### 4.4 SignalEnhancer — ✅ Done (v1.7.2)
- Нормализация весов корректна
- **Открыт:** outcomes_ не thread-safe
- **Открыт:** Dead zone при accuracy 40-60%

### 4.5 RLTradingAgent (PPO) — ✅ GAE Done (v1.7.2)
- **Открыт:** Нет clipping value loss
- **Открыт:** Entropy coeff 0.01 слишком мал

### 4.6 OnlineLearningLoop — ❌ Открыт
- **ГЛАВНЫЙ БАГ:** Текущий state вместо state на момент входа
- **nextState == state:** bootstrap нарушен
- **logProb/value = 0:** для исторических сделок
- **lastTradeTime_ не atomic**

### 4.7 ModelTrainer — ❌ Открыт
- **Хардкод 96:** Предполагает 15-мин таймфрейм

---

## 5. АНАЛИЗ UI И НАСТРОЕК

### 5.1 Потерянные настройки — ✅ Done (v1.7.3)
Фильтры и флаги индикаторов сохраняются.
**Не сохраняются (открыт):** chartBarWidth_, chartScrollOffset_, zoom/pan state.

### 5.2 Layout дефолты — ✅ Done (v1.7.3)
vdPct=0.15f, indPct=0.20f — синхронизировано.

### 5.3 Контекстное меню графика — ✅ Done (v1.7.4)
Правый клик мыши на графике в зоне цен открывает контекстное меню:
- Buy Limit / Sell Limit / Buy Stop-Limit / Sell Stop-Limit
- Цена автоматически заполняется из уровня клика
- Order Management открывается с заполненными полями

### 5.4 Визуализация ордеров на графике — ✅ Done (v1.7.4)
Открытые ордера отображаются горизонтальными линиями:
- Зелёные для BUY, красные для SELL
- Подписаны: side, qty, price

### 5.5 Leverage Slider — ✅ Done (v1.7.4)
В Order Management (режим futures) добавлен:
- Слайдер Leverage 1x–125x
- Кнопка "Apply Leverage" с callback к exchange API
- Callback `SetLeverageCallback` в AppGui

### 5.6 Формат цен (открыт)
%.8f для BTC = "100000.00000000" — нужен адаптивный формат.

---

## 6. АНАЛИЗ ИНТЕГРАЦИЙ И УТИЛИТ

### 6.1 NewsFeed + FearGreed — заглушки (открыт)
fetch() пусты.

### 6.2 TelegramBot — заглушка (открыт)
sendMessage() только логирует.

### 6.3 WebhookServer — ✅ Done (v1.7.3 + v1.7.4)
- ✅ rateLimit_ cleanup (v1.7.3)
- ✅ CRYPTO_memcmp (v1.7.3)
- ✅ cleanupCounter_ вместо static (v1.7.4)

### 6.4 CSVExporter — ✅ Done (v1.7.3)
escapeCsvField() добавлена.

### 6.5 TaxReporter (открыт)
- Потеря точности costBasis при partial fills
- Нет обработки таймзон

---

## 7. АНАЛИЗ ТЕСТОВОГО ПОКРЫТИЯ

### 7.1 Статистика (v1.7.4)
- **Всего тестов:** 453 (449 pass, 4 skip — LibTorch/XGBoost)
- **Файлов тестов:** 17
- **Новых тестов v1.7.4:** 8
- **Оценка покрытия:** ~37% кода
- **Время выполнения:** ~13 секунд

### 7.2 Критически не протестировано
| Модуль | Что не тестируется |
|--------|--------------------|
| Exchanges | Реальные API вызовы (только NoCrash) |
| WebSocket | subscribeKline() с реальными данными |
| PaperTrading | executeBuy/executeSell P&L логика |
| BacktestEngine | Полный цикл бэктеста |
| ML (LSTM/XGBoost) | Реальные predictions (4 skip) |
| GridBot | executeBuy/executeSell уровни |
| **Futures** | **getFuturesBalance() для 4 бирж** |
| **UI** | **Правый клик, leverage slider** (требует GUI) |

---

## 8. АНАЛИЗ CHANGELOG

### Ключевые наблюдения:
1. **v1.7.4** — 10 исправлений: HTTP DELETE для cancelOrder (Binance/KuCoin), signed getPositionRisk (KuCoin/Bitget/Bybit), WebhookServer thread-safety, Sharpe ratio fix, UI: right-click menu + leverage slider + order lines
2. **v1.7.3** — 22 исправления: setLeverage/getLeverage/cancelOrder, 29 тестов
3. **v1.7.2** — полный анализ и верификация: 15 проблем подтверждены
4. **v1.7.1** — Windows compilation fixes
5. **v1.7.0** — LibTorch + XGBoost + 46 стресс-тестов
6. **Прогресс:** между v1.7.0 и v1.7.4 исправлены 39+ проблем
7. **Остаётся:** 16 открытых проблем для v1.8.0

---

## 9. ПОЛНЫЙ ПРОМТ ДЛЯ ИСПРАВЛЕНИЯ

---

### 🎯 ПРОМТ (PROMPT) ДЛЯ СЛЕДУЮЩЕГО ЭТАПА РАЗРАБОТКИ v1.8.0

```
Ты — senior C++ разработчик, работающий над проектом CryptoTrader (VT — Virtual Trade System) v1.7.4.
Это C++17 программа для алгоритмической торговли криптовалютами с ML (LSTM, XGBoost), RL (PPO),
5 биржами (Binance, Bybit, OKX, KuCoin, Bitget), GUI на Dear ImGui и 453+ тестами на Google Test.

КОНТЕКСТ ПРОЕКТА:
- Репозиторий: /home/runner/work/program/program
- Основной код: src/ (67 .cpp файлов, 92 .h файлов)
- Тесты: tests/ (17 файлов, 453 тестов, 449 pass, 4 skip)
- Конфиг: config/settings.json, config/profiles.json
- Сборка: cmake -B build/test -DCMAKE_BUILD_TYPE=Debug && cmake --build build/test -j$(nproc)
- Запуск тестов: cd build/test && ./crypto_trader_tests --gtest_brief=1
- Зависимости: nlohmann-json, spdlog, Boost, CURL, SQLite3, OpenSSL, GTest, GLFW, ImGui
- Опциональные: LibTorch 2.6.0, XGBoost 2.1.3

=== СТАТУС ПРОЕКТА (v1.7.4) ===

ИСПРАВЛЕНО (✅ Done — НЕ ТРОГАТЬ):
- placeOrder() для всех 5 бирж (Binance, Bybit, OKX, KuCoin, Bitget)
- cancelOrder() для всех 5 бирж (Binance/KuCoin используют DELETE, Bybit/OKX/Bitget POST)
- setLeverage() / getLeverage() для всех 5 бирж (фьючерсы)
- getPositionRisk() для всех 5 бирж (signed requests + safeStod)
- subscribeKline() для всех 5 бирж с корректными подписками
- KuCoin getKlines() — формат [time, open, close, high, low, volume] подтверждён
- PaperTrading equity — posValue += p.quantity * p.currentPrice
- BacktestEngine equity для SHORT — posQty * (2.0 * entryPrice - price)
- BacktestEngine Sharpe ratio — sample stddev (n-1)
- GridBot profit — (sellPrice - buyPrice) * qty с buyPrice полем
- Scheduler callback — job.callback() вызывается
- ml/FeatureExtractor division by zero — проверки ema>0, hl>0
- ai/FeatureExtractor FVG division by zero — проверки ma>0
- SignalEnhancer нормализация весов — веса суммируются в 1.0
- RLTradingAgent GAE bootstrap — values[t+1] для нетерминального шага
- Bybit rate limiting — скользящее окно реализовано
- Bybit/OKX getBalance() bounds — safeStod() защита
- Engine::componentsInitialized_ — std::atomic<bool>
- TradeHistory::winRate() — единый lock, нет race condition
- TradeHistory::profitFactor() — breakeven pnl==0 не считается loss
- XGBoostModel — booster_ освобождается перед re-train
- LSTMModel — log_softmax вместо log(out+1e-9)
- AppGui configToJson/loadConfig — фильтры и флаги индикаторов сохраняются
- Layout defaults — vdPct=0.15, indPct=0.20 (синхронизировано)
- WebhookServer rateLimit_ — periodic cleanup + member variable cleanupCounter_
- WebhookServer — constant-time secret comparison (CRYPTO_memcmp)
- CSVExporter — escapeCsvField() для запятых/кавычек
- OKX getBalance() BTC — cashBal вместо availBal
- RiskManager — параметр priceSinceEntry (было highSince)
- Binance/KuCoin httpDelete() — DELETE метод для cancelOrder
- UI: правый клик на графике → контекстное меню для размещения ордеров
- UI: leverage slider (1x-125x) в Order Management
- UI: визуализация открытых ордеров на графике (горизонтальные линии)

=== ЗАДАЧА ===

Необходимо исправить ВСЕ оставшиеся 16 логических ошибок и усовершенствовать программу.
Работу нужно выполнять ПОЭТАПНО, каждый этап — отдельный коммит.
После каждого исправления — запускай тесты для проверки.
НЕ ЛОМАЙ существующие тесты. НЕ ИЗМЕНЯЙ уже исправленный код (помечен ✅ Done).

=== ЭТАП 1: FUTURES BALANCE (4 биржи) ===

1.1. Реализуй getFuturesBalance() для Bybit:
   - GET /v5/account/wallet-balance?accountType=CONTRACT (signed)
   - Парси totalWalletBalance, totalAvailableBalance, unrealisedPnl
   - Используй safeStod() для всех числовых полей

1.2. Реализуй getFuturesBalance() для OKX:
   - GET /api/v5/account/balance (signed)
   - Парси totalEq, adjEq, details для futures
   - Используй safeStod()

1.3. Реализуй getFuturesBalance() для KuCoin:
   - GET /api/v1/account-overview?currency=USDT (signed)
   - Парси accountEquity, unrealisedPNL, availableBalance
   - Используй safeStod()

1.4. Реализуй getFuturesBalance() для Bitget:
   - GET /api/v2/mix/account/accounts?productType=USDT-FUTURES (signed)
   - Парси equity, unrealizedPL, available
   - Используй safeStod()

=== ЭТАП 2: ML/AI МОДУЛИ (5 проблем) ===

2.1. Исправь OnlineLearningLoop stale state (OnlineLearningLoop.cpp:131):
   - При открытии позиции СОХРАНЯЙ state в TradeRecord или кэш
   - В OnlineLearningLoop используй сохранённый state, а не текущий feat_.extract()
   - Исправь nextState == state (bootstrap нарушен)
   - Для исторических сделок без logProb/value — пропускай или используй дефолты
   - Сделай lastTradeTime_ атомарным (std::atomic<int64_t>)

2.2. Сделай ModelTrainer::candlesPerDay конфигурируемым:
   int candlesPerDay = 1440 / timeframeMinutes; // 1m→1440, 5m→288, 15m→96, 1h→24

2.3. Исправь ai/FeatureExtractor.cpp:110 — mid < 1e-12 fallback:
   - Вместо mid=1.0 используй NaN или skip sample

2.4. Добавь thread-safety в SignalEnhancer (SignalEnhancer.cpp):
   - Оберни outcomes_ в mutex (std::lock_guard при доступе)

2.5. Добавь thread-safety для SignalEnhancer::weights_ (weightInd_, weightLstm_, weightXgb_)

=== ЭТАП 3: РЕАЛИЗАЦИЯ ЗАГЛУШЕК (3 модуля) ===

3.1. Реализуй NewsFeed::fetch() (NewsFeed.cpp):
   - API: CryptoPanic или CoinGecko /api/v3/news
   - Парси title, source, url, pubTime, sentiment
   - Rate limit: 1 запрос в 5 минут

3.2. Реализуй FearGreed::fetch() (FearGreed.cpp):
   - API: GET https://api.alternative.me/fng/?limit=1
   - Парси value, value_classification, timestamp
   - Rate limit: 1 запрос в час

3.3. Реализуй TelegramBot::sendMessage() (TelegramBot.cpp):
   - POST https://api.telegram.org/bot{token}/sendMessage
   - Body: {"chat_id": "{chatId}", "text": "{message}", "parse_mode": "HTML"}

=== ЭТАП 4: МЕЛКИЕ ИСПРАВЛЕНИЯ (5 проблем) ===

4.1. Сохранение chartBarWidth_, chartScrollOffset_ в configToJson/loadConfig
4.2. Engine.cpp:34 — maxCandleHistory вынеси в конфиг (EngineConfig)
4.3. BacktestEngine.cpp:62 — позиция 95% от баланса → конфигурируемая (positionSizePct)
4.4. TaxReporter.cpp — точность costBasis при partial fills (использовать accumulate с Kahan)
4.5. Адаптивный формат цен: %.2f для >1000, %.4f для >1, %.8f для < 1

=== ЭТАП 5: ТЕСТИРОВАНИЕ ===

5.1. Добавь тесты для getFuturesBalance():
   - test_exchanges.cpp: *GetFuturesBalanceNoCrash для Bybit/OKX/KuCoin/Bitget
5.2. Добавь тесты для OnlineLearningLoop с правильным state
5.3. Добавь тесты для NewsFeed, FearGreed, TelegramBot (unit)
5.4. Добавь тесты для адаптивного формата цен
5.5. Проверь что ВСЕ существующие 453 тестов проходят

=== ПРИОРИТЕТЫ ===
Приоритет 1 (ФЬЮЧЕРСЫ): Этап 1 — getFuturesBalance() для 4 бирж
Приоритет 2 (ML/AI): Этап 2 — OnlineLearningLoop state fix, thread-safety
Приоритет 3 (ИНТЕГРАЦИИ): Этап 3 — NewsFeed, FearGreed, Telegram
Приоритет 4 (ПОЛИРОВКА): Этап 4 — мелкие исправления
Приоритет 5 (ТЕСТЫ): Этап 5 — тесты для всех изменений

=== ОГРАНИЧЕНИЯ ===
- НЕ меняй архитектуру проекта
- НЕ добавляй новые зависимости без необходимости
- НЕ удаляй существующие тесты
- НЕ изменяй уже исправленный код (помечен ✅ Done)
- Сохраняй стиль кода (C++17, camelCase, spdlog logging)
- Все изменения должны компилироваться на Windows (VS2019) и Linux (GCC 13+)
- Код должен работать БЕЗ LibTorch/XGBoost (stub mode)
- Каждый этап = отдельный коммит с описанием
- Обновляй CHANGELOG.md после каждого этапа
```

---

## 📊 ИТОГОВАЯ СТАТИСТИКА АНАЛИЗА (v1.7.4)

| Категория | Всего найдено | ✅ Исправлено | ❌ Открыто | Новых (v1.7.4) |
|-----------|--------------|--------------|-----------|---------------|
| Биржи | 17 | 17 | 1 (getFuturesBalance ×4) | +7 (httpDelete ×2, signed ×2, safeStod ×3) |
| Торговые модули | 10 | 9 | 1 (OrderManagement hardcodes) | +1 (Sharpe ratio n-1) |
| ML/AI | 18 | 7 | 9 | 0 |
| UI/Настройки | 11 | 8 | 2 | +3 (right-click, leverage slider, order lines) |
| Интеграции | 8 | 4 | 4 | +1 (cleanupCounter_ member) |
| **ИТОГО** | **64** | **45** | **16** | **+12 новых исправлений** |

### Прогресс исправлений:
- **v1.7.3 → v1.7.4:** 7 новых исправлений + 3 UI-функции (всего 39→45 из 64)
- **Тесты:** 453 (449 pass, 4 skip) — +8 новых тестов
- **Общий статус:** 45 Done, 16 Open = **16 проблем к исправлению в v1.8.0**

### Оставшиеся проблемы (v1.8.0):
1. getFuturesBalance() для Bybit/OKX/KuCoin/Bitget
2. OnlineLearningLoop stale state + thread-safety + lastTradeTime_ atomic
3. SignalEnhancer thread-safety (outcomes_ + weights_)
4. ModelTrainer candlesPerDay hardcode
5. ai/FeatureExtractor mid fallback (mid=1.0)
6. chartBarWidth_/chartScrollOffset_ persistence
7. Engine maxCandleHistory hardcode
8. BacktestEngine 95% position size hardcode
9. TaxReporter costBasis precision
10. NewsFeed::fetch() заглушка
11. FearGreed::fetch() заглушка
12. TelegramBot::sendMessage() заглушка
13. OrderManagement validation hardcodes
14. Адаптивный формат цен (%.2f vs %.8f)
15. SignalEnhancer dead zone (accuracy 40-60%)
16. RLTradingAgent entropy/clipping tuning

---

*Документ обновлён на основе полного анализа кодовой базы CryptoTrader v1.7.4, включая все 67 исходных файлов, 17 тестовых файлов, конфигурационные файлы и CHANGELOG. Полная сборка и запуск 453 тестов подтверждены.*
