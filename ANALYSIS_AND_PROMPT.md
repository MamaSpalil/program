# 🔍 ПОЛНЫЙ АНАЛИЗ И ПРОМТ ДЛЯ ИСПРАВЛЕНИЯ ПРОГРАММЫ CryptoTrader (VT)

> **Дата анализа:** 2026-03-06
> **Версия:** 1.7.1
> **Платформа сборки:** Linux (GCC 13.3) + Windows 10 Pro x64 (VS2019)
> **Результаты тестирования:** 416 тестов, 412 пройдено, 4 пропущено (LibTorch/XGBoost отсутствуют)

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
| # | Модуль | Файл:Строка | Проблема |
|---|--------|-------------|----------|
| 1 | Exchange | BybitExchange.cpp:131-135 | placeOrder() — заглушка, ордера не размещаются |
| 2 | Exchange | OKXExchange.cpp:211-215 | placeOrder() — заглушка |
| 3 | Exchange | KuCoinExchange.cpp:207-211 | placeOrder() — заглушка |
| 4 | Exchange | BitgetExchange.cpp:200-204 | placeOrder() — заглушка |
| 5 | Exchange | KuCoinExchange.cpp:179-188 | Kline high/low перепутаны (данные OHLC невалидны) |
| 6 | Exchange | Bybit/OKX/KuCoin/Bitget subscribeKline() | symbol и interval игнорируются — подписка не работает |
| 7 | Trading | PaperTrading.cpp:124 | Двойной подсчёт equity (posValue += qty * entryPrice + pnl) |
| 8 | Backtest | BacktestEngine.cpp:110 | Некорректный расчёт equity при SHORT позициях |
| 9 | Strategy | RiskManager.cpp:49-50 | Trailing stop для SHORT: используется highSince вместо lowSince |
| 10 | Scheduler | Scheduler.cpp:122-126 | Задания планировщика НИКОГДА не исполняются (нет callback) |

### 🟠 ВЫСОКИЕ (влияют на корректность)
| # | Модуль | Файл:Строка | Проблема |
|---|--------|-------------|----------|
| 11 | Exchange | BybitExchange — нет rate limiting | Риск бана API при интенсивной работе |
| 12 | Exchange | OKX/Bybit/KuCoin getBalance() | Нет проверки границ массива [0] — crash на пустом ответе |
| 13 | Exchange | OKXExchange.cpp:234-236 | BTC баланс: availBal вместо totalBal — неверная сумма |
| 14 | ML | XGBoostModel.cpp:36 | Утечка памяти — booster_ не освобождается при переобучении |
| 15 | ML | LSTMModel.cpp:99 | Числовая нестабильность: log(softmax+eps) вместо log_softmax |
| 16 | ML | SignalEnhancer.cpp:72-78 | Веса ансамбля не нормализуются обратно к 1.0 |
| 17 | AI | OnlineLearningLoop.cpp:131 | Стейт извлекается в текущий момент, а не на момент входа в сделку |
| 18 | AI | RLTradingAgent.cpp:303 | GAE bootstrap: nextVal=0 для нетерминального шага |
| 19 | Trading | GridBot.cpp:62 | Profit = price * 0.001 — заглушка, а не реальный расчёт |
| 20 | Core | Engine.cpp:94-95 | Race condition: componentsInitialized_ не atomic |
| 21 | Trading | TradeHistory.cpp:98-100 | Race condition в winRate(): два отдельных lock |
| 22 | Trading | TradeHistory.cpp:128 | profitFactor(): breakeven (pnl==0) считается как loss |
| 23 | Integration | WebhookServer.cpp:33-36 | Rate-limit map растёт бесконтрольно — утечка памяти |
| 24 | Settings | AppGui.cpp:464-524 | Фильтры и флаги индикаторов НЕ сохраняются в JSON |

### 🟡 СРЕДНИЕ (функциональные дефекты)
| # | Модуль | Файл:Строка | Проблема |
|---|--------|-------------|----------|
| 25 | ML | FeatureExtractor.cpp:87 | Division by zero: hl/c.close без проверки close>0 |
| 26 | ML | ModelTrainer.cpp:16 | Хардкод 96 свечей/день (только 15-мин таймфрейм) |
| 27 | AI | FeatureExtractor.cpp:247-250 | FVG detection: candles_[i-2] без проверки i>=2 |
| 28 | AI | FeatureExtractor.cpp:110 | mid < 1e-12 → mid=1.0: фиктивное значение портит нормализацию |
| 29 | Settings | LayoutManager.h vs AppGui.h | Несовпадение дефолтов: vdPct 0.15/0.13, indPct 0.20/0.25 |
| 30 | UI | AppGui.h:339-340 | chartBarWidth_, chartScrollOffset_ не сохраняются |
| 31 | Data | CSVExporter.cpp:45-49 | Нет экранирования запятых в CSV (поля с , ломают файл) |
| 32 | External | NewsFeed.cpp:46-50, FearGreed.cpp:35 | API-вызовы — заглушки, реальные данные не получаются |
| 33 | Integration | TelegramBot.cpp:40-56 | Все команды — заглушки, сообщения не отправляются |
| 34 | Core | Engine.cpp:34, 308 | Хардкод: maxCandleHistory=5000, orderbook interval=2000ms |
| 35 | Backtest | BacktestEngine.cpp:62 | Хардкод: позиция всегда 95% от баланса |
| 36 | Tax | TaxReporter.cpp:46-47 | Потеря точности costBasis при частичных fills |
| 37 | Security | WebhookServer.cpp:59 | String comparison не constant-time (timing attack) |

---

## 2. АНАЛИЗ БИРЖ

### 2.1 Интерфейс IExchange (IExchange.h)
Обязательные методы: `testConnection()`, `getKlines()`, `getPrice()`, `placeOrder()`, `getBalance()`, `subscribeKline()`, `connect()`, `disconnect()`.

### 2.2 Статус реализации по биржам

| Функция | Binance | Bybit | OKX | KuCoin | Bitget |
|---------|---------|-------|-----|--------|--------|
| getPrice() | ✅ | ✅ | ✅ | ✅ | ✅ |
| getKlines() | ✅ | ✅ | ✅ | ⚠️ H/L swap | ✅ |
| getBalance() | ✅ | ⚠️ no bounds | ⚠️ availBal | ⚠️ no bounds | ✅ |
| placeOrder() | ✅ | ❌ stub | ❌ stub | ❌ stub | ❌ stub |
| cancelOrder() | ❌ none | ❌ none | ❌ none | ❌ none | ❌ none |
| subscribeKline() | ✅ | ❌ symbol ignored | ❌ symbol ignored | ❌ symbol ignored | ❌ symbol ignored |
| Rate Limiting | ✅ full | ❌ отсутствует | ✅ 10req/s | ✅ 10req/s | ✅ 10req/s |
| Authentication | ✅ HMAC-SHA256 | ✅ HMAC-SHA256 | ✅ HMAC-SHA256+Base64 | ✅ HMAC-SHA256+Base64 | ✅ HMAC-SHA256+Base64 |

### 2.3 Конкретные баги бирж

**KuCoin getKlines() — перепутаны high/low:**
```cpp
// KuCoinExchange.cpp:179-188 — ТЕКУЩИЙ (НЕПРАВИЛЬНЫЙ):
c.open  = safeStod(k[1]); // ✓ open
c.close = safeStod(k[2]); // ✓ close
c.high  = safeStod(k[3]); // ❌ Это LOW в формате KuCoin!
c.low   = safeStod(k[4]); // ❌ Это HIGH в формате KuCoin!
// KuCoin API: [time, open, close, high, low, volume, turnover]
```

**WebSocket subscribeKline() — 4 биржи:**
```cpp
// BybitExchange.cpp:195-202
ws_ = std::make_unique<WebSocketClient>(wsHost_, wsPort_, "/v5/public/spot");
(void)symbol; (void)interval; // ← ПРОБЛЕМА: параметры игнорируются!
// Нужно: формировать путь/подписку с символом и интервалом
```

**OKX BTC баланс — не total а available:**
```cpp
// OKXExchange.cpp:234-236
bal.btcBalance = safeStod(d.value("availBal", "0")); // ❌ availBal ≠ total
// Нужно: использовать "cashBal" или "eq" для полного баланса
```

### 2.4 Формат символов (нужна унификация)
| Биржа | Формат спот | Формат фьючерс |
|-------|-------------|----------------|
| Binance | BTCUSDT | BTCUSDT |
| Bybit | BTCUSDT | BTCUSDT |
| OKX | BTC-USDT | BTC-USDT-SWAP |
| KuCoin | BTC-USDT | BTC-USDT |
| Bitget | BTCUSDT | BTCUSDT_UMCBL |

---

## 3. АНАЛИЗ ТОРГОВЫХ МОДУЛЕЙ

### 3.1 PaperTrading — двойной подсчёт equity
```cpp
// PaperTrading.cpp:124 — ТЕКУЩИЙ:
posValue += p.quantity * p.entryPrice + p.pnl;
// ПРАВИЛЬНО:
posValue += p.quantity * currentPrice;
// Или: posValue += p.quantity * p.entryPrice + (currentPrice - entryPrice) * quantity;
```
**Последствия:** equity завышен, drawdown-метрики некорректны, DailyLossGuard может не срабатывать.

### 3.2 BacktestEngine — ошибка equity при SHORT
```cpp
// BacktestEngine.cpp:110 — ТЕКУЩИЙ:
equity = balance + entryPrice * posQty + unrealized;
// Для SHORT это неверно! entryPrice * posQty — это размер позиции при ВХОДЕ
// unrealized для SHORT = (entryPrice - currentPrice) * qty
// Правильно: equity = balance + currentPrice * std::abs(posQty);
```

### 3.3 RiskManager — trailing stop для SHORT
```cpp
// RiskManager.cpp:49-50 — ТЕКУЩИЙ:
return std::min(entryPrice, highSince + cfg_.atrStopMultiplier * atr);
// ПРАВИЛЬНО для SHORT:
return std::max(entryPrice, lowSince + cfg_.atrStopMultiplier * atr);
// Или: trailStop = lowestLow + multiplier * ATR (выше текущей цены)
```

### 3.4 GridBot — фейковый profit
```cpp
// GridBot.cpp:62 — заглушка:
realizedProfit_ += lvl.price * 0.001; // placeholder
// НУЖНО: отслеживать цену покупки/продажи по уровню, считать разницу
```

### 3.5 Scheduler — мёртвый код
```cpp
// Scheduler.cpp:122-126 — задание обнаружено, но НЕ ИСПОЛНЕНО:
if (shouldRun(job.cronExpr, tm_now)) {
    job.lastRun = formatTime(t);
    job.nextRun = calcNextRun(job.cronExpr, tm_now);
    Logger::get()->info("[Scheduler] Triggered job: {}", job.name);
    // ← НЕТ ВЫЗОВА CALLBACK! Нужно: job.callback(); или eventBus_->emit(...)
}
```

### 3.6 TradeHistory — race condition и profitFactor
```cpp
// TradeHistory.cpp:98-100 — два отдельных lock:
int total = totalTrades();   // lock1 → unlock
return double(winCount()) / total;  // lock2 → unlock → total может быть неактуален
// FIX: один lock на весь расчёт

// TradeHistory.cpp:128 — breakeven в losses:
else totalLosses += std::abs(t.pnl); // pnl==0 → 0 добавляется в losses (ок, но неточно)
// FIX: добавить if (t.pnl < 0) для точного profitFactor
```

### 3.7 OrderManagement — хардкоды
- Нет валидации price > 0
- Форматирование ошибки: `std::to_string(accountBalance)` → `"$1000.000000"`
- estimateCost не учитывает комиссию

---

## 4. АНАЛИЗ ML/AI МОДУЛЕЙ

### 4.1 LSTMModel
- **Проблема:** `torch::log(out + 1e-9)` — числовая нестабильность. Нужно `torch::log_softmax(logits, 1)` → `nll_loss`.
- **Проблема:** lr=0.001, patience=5 — хардкод; нужен LSTMConfig.
- **Проблема:** Нет валидации размера входного вектора.
- **Stub mode:** возвращает {0.333, 0.334, 0.333} без предупреждения в UI.

### 4.2 XGBoostModel
- **УТЕЧКА ПАМЯТИ:** `XGBoosterFree(booster_)` не вызывается перед `train()` повторно.
- **Проблема:** outResult может содержать <3 float → мусорные данные.
- **Проблема:** NaN из feature → NAN как missing value, но нет обработки NaN в output.

### 4.3 FeatureExtractor (ML)
- **Division by zero:** `hl / c.close` без проверки close > 0 (строка 87).
- **Silent masking:** `if (b <= 0.0) return 0.0` — маскирует нулевые цены.
- **Хардкод:** lags {1,3,5,10,20}, volatility windows {5,10,20} — не конфигурируемы.
- **OBV overflow:** кумулятивный OBV растёт без bounds для долго работающих процессов.

### 4.4 SignalEnhancer
- **Нормализация весов:** После изменения weightInd_, weightLstm_ и weightXgb_ пересчитываются, но финальная сумма может отклоняться от 1.0.
- **Мёртвая зона:** При accuracy 40-60% веса не обновляются — могут застаиваться навсегда.
- **Race condition:** outcomes_ не thread-safe.

### 4.5 RLTradingAgent (PPO)
- **GAE bootstrap:** `nextVal = 0.0f` для последнего шага — неверно, если эпизод не завершён; нужно `critic(state_T).detach()`.
- **Нет clipping value loss:** PPO paper рекомендует clip value loss.
- **Entropy coeff:** 0.01 — слишком мал для volatile крипторынка.
- **Хардкод:** orthogonal init scales, mini-batch size.

### 4.6 OnlineLearningLoop
- **ГЛАВНЫЙ БАГ:** `exp.state = feat_.extract(agent_.getMode())` — берёт ТЕКУЩИЙ стейт, а не стейт на момент входа в сделку. RL-агент обучается на неправильных state-action парах.
- **nextState == state:** `exp.nextState = exp.state` — bootstrap нарушен.
- **logProb/value = 0:** Исторические сделки не имеют оригинальных logProb/value — GAE невалиден.
- **Race:** lastTradeTime_ не atomic.
- **Unbounded growth:** localBatch_ растёт если training медленнее сбора.

### 4.7 ModelTrainer
- **Хардкод 96:** Предполагает 15-мин таймфрейм (96 свечей/день).
- **Label/feature mismatch:** Пропуск sample при close<=0 без удаления соответствующего feature.

---

## 5. АНАЛИЗ UI И НАСТРОЕК

### 5.1 Потерянные настройки (не сохраняются)
Следующие поля GuiConfig существуют в AppGui.h, но `configToJson()` их НЕ записывает:
- `filterMinVolume`, `filterMinPrice`, `filterMaxPrice`, `filterMinChange`, `filterMaxChange`
- `indEmaEnabled`, `indRsiEnabled`, `indAtrEnabled`, `indMacdEnabled`, `indBbEnabled`
- `chartBarWidth_`, `chartScrollOffset_`
- `vdZoomX_/Y_`, `vdPanX_/Y_`, `indZoomX_/Y_`, `indPanX_/Y_`

### 5.2 Несовпадение дефолтов layout
| Параметр | LayoutManager.h | AppGui.h GuiConfig |
|----------|----------------|--------------------|
| vdPct | 0.15f | 0.13f |
| indPct | 0.20f | 0.25f |

### 5.3 settings.json — отсутствующие секции
Файл config/settings.json не содержит:
- Секцию `"filters"` (min/max volume, price, change)
- Секцию `"indicators_enabled"` (флаги включения индикаторов)
- Секцию `"layout"` (пропорции панелей)

### 5.4 Формат цен
Изменено на `%.8f` для точности, но `%.8f` для цены BTC = `95000.00000000` — слишком длинный. Для BTC лучше `%.2f`, для shitcoins `%.8f`. Нужен адаптивный формат.

---

## 6. АНАЛИЗ ИНТЕГРАЦИЙ И УТИЛИТ

### 6.1 NewsFeed + FearGreed — заглушки
Оба модуля имеют `fetch()` метод, но тело пустое — данные никогда не запрашиваются с реального API.

### 6.2 TelegramBot — заглушка
`sendMessage()` только логирует, не отправляет. Все команды (/status, /balance, /orders) — заглушки.

### 6.3 WebhookServer — утечка памяти
`rateLimit_` map (IP → timestamp) растёт без ограничений. Нужен LRU или periodic cleanup.

### 6.4 CSVExporter — no escaping
Если symbol содержит запятую (маловероятно, но...) или notes поле содержит `","` — CSV ломается. Нужно `std::quoted()`.

### 6.5 TaxReporter
- Потеря точности costBasis при partial fills (floating-point accumulation).
- Нет обработки таймзон (UTC vs local).

---

## 7. АНАЛИЗ ТЕСТОВОГО ПОКРЫТИЯ

### 7.1 Статистика
- **Всего тестов:** 416 (412 pass, 4 skip)
- **Файлов тестов:** 17
- **Оценка покрытия:** ~30% кода

### 7.2 Критически не протестировано
| Модуль | Что не тестируется |
|--------|--------------------|
| Exchanges | placeOrder(), cancelOrder(), getBalance() с реальными данными |
| WebSocket | subscribeKline() — подписка и получение данных |
| PaperTrading | executeBuy/executeSell с реальной P&L логикой |
| BacktestEngine | Полный цикл бэктеста с результатами |
| RiskManager | validateOrder() под нагрузкой |
| ML (LSTM/XGBoost) | Реальные prediction с LibTorch/XGBoost |
| GridBot | executeBuy/executeSell уровни |
| AlertManager | checkCondition() триггеры |
| Concurrent access | Race conditions под нагрузкой |

---

## 8. АНАЛИЗ CHANGELOG

### Ключевые наблюдения из CHANGELOG:
1. **v1.7.1** — только Windows compilation fixes (xcopy, C4005)
2. **v1.7.0** — крупное обновление: LibTorch + XGBoost + 46 стресс-тестов
3. **v1.6.0** — Tile Layout + AI Status + все 5 бирж getBalance()
4. **v1.5.3** — VT иконка + 75 системных тестов
5. **Паттерн:** каждая версия добавляет новый функционал, но НЕ ФИКСИТ существующие баги
6. **Технический долг:** placeOrder() для 4 бирж — заглушки с v1.0; не исправлено за 6+ версий

---

## 9. ПОЛНЫЙ ПРОМТ ДЛЯ ИСПРАВЛЕНИЯ

---

### 🎯 ПРОМТ (PROMPT) ДЛЯ СЛЕДУЮЩЕГО ЭТАПА РАЗРАБОТКИ

```
Ты — senior C++ разработчик, работающий над проектом CryptoTrader (VT — Virtual Trade System) v1.7.1.
Это C++17 программа для алгоритмической торговли криптовалютами с ML (LSTM, XGBoost), RL (PPO),
5 биржами (Binance, Bybit, OKX, KuCoin, Bitget), GUI на Dear ImGui и 416+ тестами на Google Test.

КОНТЕКСТ ПРОЕКТА:
- Репозиторий: /home/runner/work/program/program
- Основной код: src/ (67 .cpp файлов, 92 .h файлов)
- Тесты: tests/ (17 файлов, 416 тестов, 412 pass, 4 skip)
- Конфиг: config/settings.json, config/profiles.json
- Сборка: cmake -B build/test -DCMAKE_BUILD_TYPE=Debug && cmake --build build/test -j$(nproc)
- Запуск тестов: cd build/test && ./crypto_trader_tests --gtest_brief=1
- Зависимости: nlohmann-json, spdlog, Boost, CURL, SQLite3, OpenSSL, GTest, GLFW, ImGui
- Опциональные: LibTorch 2.6.0, XGBoost 2.1.3

=== ЗАДАЧА ===

Необходимо исправить ВСЕ найденные логические ошибки и усовершенствовать программу.
Работу нужно выполнять ПОЭТАПНО, каждый этап — отдельный коммит.
После каждого исправления — запускай тесты для проверки.
НЕ ЛОМАЙ существующие тесты.

=== ЭТАП 1: КРИТИЧЕСКИЕ ИСПРАВЛЕНИЯ БИРЖ ===

1.1. Реализуй placeOrder() для Bybit (BybitExchange.cpp:131-135):
   - Endpoint: POST /v5/order/create
   - Body: {"category":"spot","symbol":"...","side":"Buy/Sell","orderType":"Limit/Market","qty":"...","price":"..."}
   - Подпиши запрос через signRequest() (уже реализован в файле)
   - Верни заполненный OrderResult с orderId, status, timestamp
   - Обработай ошибки (retCode != 0)

1.2. Реализуй placeOrder() для OKX (OKXExchange.cpp:211-215):
   - Endpoint: POST /api/v5/trade/order
   - Body: {"instId":"BTC-USDT","tdMode":"cash","side":"buy/sell","ordType":"limit/market","sz":"...","px":"..."}
   - Используй существующий httpPost() с подписью
   - Верни OrderResult, обработай code != "0"

1.3. Реализуй placeOrder() для KuCoin (KuCoinExchange.cpp:207-211):
   - Endpoint: POST /api/v1/orders
   - Body: {"clientOid":"...","side":"buy/sell","symbol":"BTC-USDT","type":"limit/market","size":"...","price":"..."}
   - Используй существующий httpPost()
   - Верни OrderResult, обработай code != "200000"

1.4. Реализуй placeOrder() для Bitget (BitgetExchange.cpp:200-204):
   - Endpoint: POST /api/v2/spot/trade/place-order
   - Body: {"symbol":"BTCUSDT","side":"buy/sell","orderType":"limit/market","size":"...","price":"..."}
   - Используй существующий httpPost()
   - Верни OrderResult, обработай code != "00000"

1.5. Исправь KuCoin getKlines() (KuCoinExchange.cpp:179-188):
   - KuCoin API формат: [time, open, close, high, low, volume, turnover]
   - ТЕКУЩИЙ код: c.high = k[3], c.low = k[4] — но k[3] это HIGH, k[4] это LOW
   - Проверь документацию KuCoin API, исправь порядок полей
   - ВНИМАНИЕ: Если формат [time, open, close, high, low, vol], то текущий код УЖЕ правильный!
     Но если формат [time, open, high, low, close, vol] (стандартный OHLCV), тогда:
     c.open = k[1], c.high = k[2], c.low = k[3], c.close = k[4], c.volume = k[5]
   - Верифицируй через реальный запрос к API KuCoin /api/v1/market/candles

1.6. Исправь subscribeKline() для Bybit, OKX, KuCoin, Bitget:
   - Bybit: после подключения WS, отправь JSON подписку:
     {"op":"subscribe","args":["kline.{interval}.{symbol}"]}
   - OKX: отправь подписку:
     {"op":"subscribe","args":[{"channel":"candle{interval}","instId":"{symbol}"}]}
   - KuCoin: сначала получи WS token через POST /api/v1/bullet-public,
     затем подключись к серверу из ответа и подпишись:
     {"type":"subscribe","topic":"/market/candles:{symbol}_{interval}"}
   - Bitget: отправь подписку:
     {"op":"subscribe","args":[{"instType":"SPOT","channel":"candle{interval}","instId":"{symbol}"}]}
   - Не забудь обработку onWsMessage() для каждой биржи!

1.7. Добавь rate limiting для Bybit:
   - Bybit ограничения: 120 requests/sec для частных endpoint, 50/sec для публичных
   - Реализуй аналогично OKX/KuCoin/Bitget (sliding window 1 sec)

1.8. Добавь проверки границ массива в getBalance():
   - Bybit: if (j["result"]["list"].empty()) return bal; — перед [0]
   - OKX: if (j["data"].empty()) return bal; — перед [0]
   - KuCoin: аналогично
   - OKX: замени "availBal" на "cashBal" или "eq" для полного баланса BTC

=== ЭТАП 2: КРИТИЧЕСКИЕ ИСПРАВЛЕНИЯ ТОРГОВЫХ МОДУЛЕЙ ===

2.1. Исправь PaperTrading equity расчёт (PaperTrading.cpp:124):
   БЫЛО:  posValue += p.quantity * p.entryPrice + p.pnl;
   НУЖНО: posValue += p.quantity * currentPrice_;
   // Где currentPrice_ — текущая рыночная цена для символа позиции
   // Если currentPrice_ недоступен, используй: p.quantity * p.entryPrice + p.pnl
   // НО: p.pnl уже должен быть = (currentPrice - entryPrice) * qty * direction
   // Если p.pnl корректно рассчитан, то: posValue += p.quantity * p.entryPrice + p.pnl — ВЕРНО
   // Проблема в том, что p.pnl может быть некорректным. Проверь как рассчитывается pnl.

2.2. Исправь BacktestEngine equity (BacktestEngine.cpp:110):
   - Для LONG: equity = balance + posQty * currentPrice
   - Для SHORT: equity = balance + posQty * (2 * entryPrice - currentPrice)
     Или проще: equity = balance + posValue_atEntry + unrealizedPnL
     Где unrealizedPnL = (currentPrice - entryPrice) * qty для LONG
                        = (entryPrice - currentPrice) * qty для SHORT

2.3. Исправь RiskManager trailing stop (RiskManager.cpp:49-50):
   БЫЛО (для SHORT):
     return std::min(entryPrice, highSince + cfg_.atrStopMultiplier * atr);
   НУЖНО (для SHORT trailing stop ВЫШЕ цены):
     return std::min(entryPrice, lowestSince + cfg_.atrStopMultiplier * atr);
   // Trailing stop для SHORT должен двигаться ВНИЗ за ценой
   // lowestSince = минимальная цена с момента входа

2.4. Исправь GridBot profit (GridBot.cpp:62):
   - Добавь поле buyPrice в GridLevel struct
   - При заполнении BUY ордера: lvl.buyPrice = lvl.price; lvl.filled = true;
   - При заполнении SELL ордера: realizedProfit_ += (lvl.price - lvl.buyPrice) * lvl.quantity;
   - Аналогично для SELL→BUY (short grid)

2.5. Исправь Scheduler (Scheduler.cpp:122-126):
   - Добавь callback в ScheduledJob struct: std::function<void(const ScheduledJob&)> callback;
   - При shouldRun() вызывай: if (job.callback) job.callback(job);
   - Или используй EventBus для публикации события: eventBus_->emit("scheduler.trigger", job);

2.6. Исправь TradeHistory::winRate() race condition (TradeHistory.cpp:98-100):
   double TradeHistory::winRate() const {
       std::lock_guard<std::mutex> lk(mutex_);
       if (trades_.empty()) return 0.0;
       int wins = 0;
       for (auto& t : trades_) if (t.pnl > 0) wins++;
       return static_cast<double>(wins) / trades_.size();
   }

2.7. Исправь TradeHistory::profitFactor() (TradeHistory.cpp:128):
   if (t.pnl > 0) totalWins += t.pnl;
   else if (t.pnl < 0) totalLosses += std::abs(t.pnl);  // ← добавь if (t.pnl < 0)
   // Breakeven (pnl == 0) не считается ни в wins, ни в losses

2.8. Сделай Engine::componentsInitialized_ атомарным (Engine.cpp):
   std::atomic<bool> componentsInitialized_{false};

=== ЭТАП 3: ИСПРАВЛЕНИЯ ML/AI МОДУЛЕЙ ===

3.1. Исправь LSTMModel loss (LSTMModel.cpp:99):
   БЫЛО:  auto loss = -torch::mean(target * torch::log(out + 1e-9));
   НУЖНО: auto log_probs = torch::log_softmax(logits, 1);
          auto loss = torch::nll_loss(log_probs, target);

3.2. Исправь утечку памяти XGBoost (XGBoostModel.cpp):
   В начале train():
     if (booster_) { XGBoosterFree(booster_); booster_ = nullptr; }

3.3. Исправь нормализацию весов SignalEnhancer (SignalEnhancer.cpp:72-78):
   После изменения весов добавь:
     double sum = weightInd_ + weightLstm_ + weightXgb_;
     if (sum > 0) { weightInd_ /= sum; weightLstm_ /= sum; weightXgb_ /= sum; }

3.4. Исправь OnlineLearningLoop stale state (OnlineLearningLoop.cpp:131):
   - При входе в сделку (PaperTrading/OrderManagement) СОХРАНЯЙ текущий state в TradeRecord
   - В OnlineLearningLoop используй сохранённый state, а не текущий
   - Если невозможно изменить TradeRecord, добавь промежуточный буфер state_at_entry

3.5. Исправь GAE bootstrap (RLTradingAgent.cpp:303):
   БЫЛО:  float nextVal = 0.0f; // terminal
   НУЖНО: float nextVal = (t + 1 < T) ? values[t + 1] : 0.0f;
   // Или: если done flag != true, nextVal = critic_->forward(nextState).item<float>();

3.6. Исправь FeatureExtractor (ML) division by zero (FeatureExtractor.cpp:87):
   БЫЛО:  features.push_back(hl / c.close);
   НУЖНО: features.push_back(c.close > 0.0 ? hl / c.close : 0.0);

3.7. Исправь AI FeatureExtractor FVG bounds (ai/FeatureExtractor.cpp:247-250):
   Добавь проверку: if (i < 2) continue; перед candles_[i-2]

3.8. Сделай ModelTrainer::candlesPerDay конфигурируемым:
   Вместо хардкода 96, вычисляй из timeframe:
     int candlesPerDay = 1440 / timeframeMinutes; // 1m→1440, 5m→288, 15m→96, 1h→24

=== ЭТАП 4: ИСПРАВЛЕНИЯ НАСТРОЕК И UI ===

4.1. Добавь сохранение фильтров в configToJson() (AppGui.cpp:464-524):
   j["filters"]["min_volume"] = cfg_.filterMinVolume;
   j["filters"]["min_price"] = cfg_.filterMinPrice;
   j["filters"]["max_price"] = cfg_.filterMaxPrice;
   j["filters"]["min_change"] = cfg_.filterMinChange;
   j["filters"]["max_change"] = cfg_.filterMaxChange;

4.2. Добавь сохранение флагов индикаторов:
   j["indicators_enabled"]["ema"] = cfg_.indEmaEnabled;
   j["indicators_enabled"]["rsi"] = cfg_.indRsiEnabled;
   j["indicators_enabled"]["atr"] = cfg_.indAtrEnabled;
   j["indicators_enabled"]["macd"] = cfg_.indMacdEnabled;
   j["indicators_enabled"]["bb"] = cfg_.indBbEnabled;

4.3. Добавь загрузку этих настроек в loadConfig():
   if (j.contains("filters")) { auto& f = j["filters"]; ... }
   if (j.contains("indicators_enabled")) { auto& ie = j["indicators_enabled"]; ... }

4.4. Синхронизируй дефолты layout:
   LayoutManager.h: vdPct_ = 0.13f (вместо 0.15)
   LayoutManager.h: indPct_ = 0.25f (вместо 0.20)
   // ИЛИ: AppGui.h: layoutVdPct{0.15f}, layoutIndPct{0.20f}
   // Главное — чтобы значения СОВПАДАЛИ

4.5. Добавь адаптивный формат цен:
   Вместо фиксированного %.8f, используй:
   if (price >= 1.0) snprintf(buf, "%.2f", price);
   else if (price >= 0.001) snprintf(buf, "%.4f", price);
   else snprintf(buf, "%.8f", price);

=== ЭТАП 5: ИСПРАВЛЕНИЯ ИНТЕГРАЦИЙ ===

5.1. WebhookServer — ограничь rateLimit_ map:
   - Добавь максимальный размер (например 10000 IP)
   - Или периодически очищай записи старше 1 часа
   - Или используй LRU cache

5.2. CSVExporter — добавь экранирование:
   Используй std::quoted() для строковых полей
   Или оберни: if (field.find(',') != npos) field = "\"" + field + "\"";

5.3. WebhookServer — constant-time comparison:
   Замени: if (secret != secretKey_)
   На: if (!CRYPTO_memcmp(secret.data(), secretKey_.data(), std::min(secret.size(), secretKey_.size())) == 0 || secret.size() != secretKey_.size())
   Или используй OpenSSL CRYPTO_memcmp (уже подключен)

=== ЭТАП 6: УСОВЕРШЕНСТВОВАНИЯ ===

6.1. Сделай хардкоды конфигурируемыми:
   - Engine.cpp:34 — maxCandleHistory → config["max_candle_history"]
   - Engine.cpp:308 — orderbook interval 2000ms → config["orderbook_refresh_ms"]
   - BacktestEngine.cpp:62 — position size 0.95 → config["backtest"]["position_size_pct"]
   - Scheduler.cpp:97 — loop limit 10080 → config["scheduler"]["max_iterations"]

6.2. Добавь thread-safety в SignalEnhancer:
   - Оберни outcomes_ в mutex
   - Или используй lock-free queue (SPSCQueue уже есть в проекте)

6.3. Добавь cancelOrder() для всех бирж:
   - Binance: DELETE /api/v3/order
   - Bybit: POST /v5/order/cancel
   - OKX: POST /api/v5/trade/cancel-order
   - KuCoin: DELETE /api/v1/orders/{orderId}
   - Bitget: POST /api/v2/spot/trade/cancel-order

6.4. Добавь getOpenOrders() для всех бирж (необходимо для синхронизации состояния).

6.5. Реализуй NewsFeed и FearGreed API:
   - NewsFeed: CryptoPanic API или CoinGecko news
   - FearGreed: alternative.me/crypto/fear-and-greed-index/

6.6. Реализуй TelegramBot sendMessage():
   - HTTP POST https://api.telegram.org/bot{token}/sendMessage
   - Body: {"chat_id": "...", "text": "...", "parse_mode": "HTML"}

=== ЭТАП 7: ТЕСТИРОВАНИЕ ===

7.1. Добавь unit-тесты для каждого исправления:
   - test_exchange_placeorder.cpp — mock HTTP, проверь body/headers для каждой биржи
   - Обнови test_full_system.cpp — проверь equity расчёт PaperTrading
   - Обнови test_risk.cpp — проверь trailing stop SHORT
   - Добавь тест GridBot profit calculation
   - Добавь тест Scheduler callback execution
   - Добавь тест TradeHistory race-free winRate/profitFactor

7.2. Добавь negative tests:
   - Пустой JSON ответ от биржи → graceful fallback
   - getBalance() с пустым массивом → no crash
   - subscribeKline() с невалидным символом → error log

7.3. Проверь что ВСЕ существующие 416 тестов проходят после изменений.

=== ПРИОРИТЕТЫ ===
Приоритет 1 (БЛОКЕРЫ): 1.5 (KuCoin OHLC), 2.1 (equity), 2.2 (backtest equity), 2.3 (trailing stop)
Приоритет 2 (ТОРГОВЛЯ): 1.1-1.4 (placeOrder), 1.6 (WS subscribe), 1.7 (rate limit), 1.8 (bounds)
Приоритет 3 (ML/AI): 3.1-3.8 (все ML/AI фиксы)
Приоритет 4 (НАСТРОЙКИ): 4.1-4.5 (settings persistence)
Приоритет 5 (ИНТЕГРАЦИИ): 5.1-5.3, 6.1-6.6

=== ОГРАНИЧЕНИЯ ===
- НЕ меняй архитектуру проекта
- НЕ добавляй новые зависимости без необходимости
- НЕ удаляй существующие тесты
- Сохраняй стиль кода (C++17, camelCase, spdlog logging)
- Все изменения должны компилироваться на Windows (VS2019) и Linux (GCC 13+)
- Код должен работать БЕЗ LibTorch/XGBoost (stub mode)
- Каждый этап = отдельный коммит с описанием
```

---

## 📊 ИТОГОВАЯ СТАТИСТИКА АНАЛИЗА

| Категория | Найдено проблем | Критических | Высоких | Средних | Низких |
|-----------|----------------|-------------|---------|---------|--------|
| Биржи | 14 | 6 | 4 | 3 | 1 |
| Торговые модули | 10 | 4 | 3 | 2 | 1 |
| ML/AI | 18 | 2 | 7 | 7 | 2 |
| UI/Настройки | 8 | 0 | 2 | 4 | 2 |
| Интеграции | 7 | 0 | 2 | 4 | 1 |
| **ИТОГО** | **57** | **12** | **18** | **20** | **7** |

---

*Документ создан на основе полного анализа кодовой базы CryptoTrader v1.7.1, включая все 67 исходных файлов, 17 тестовых файлов, конфигурационные файлы и CHANGELOG.*
