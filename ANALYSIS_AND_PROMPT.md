# 🔍 ПОЛНЫЙ АНАЛИЗ И ПРОМТ ДЛЯ ИСПРАВЛЕНИЯ ПРОГРАММЫ CryptoTrader (VT)

> **Дата анализа:** 2026-03-06
> **Версия:** 1.7.2
> **Платформа сборки:** Linux (GCC 13.3) + Windows 10 Pro x64 (VS2019)
> **Результаты тестирования:** 416 тестов, 412 пройдено, 4 пропущено (LibTorch/XGBoost отсутствуют)
> **Обновление v1.7.2:** Проведён повторный полный анализ кода. 15 ранее критических/высоких проблем исправлены и отмечены как ✅ Done.

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
| 1 | Exchange | BybitExchange.cpp:148-196 | placeOrder() — заглушка, ордера не размещаются | ✅ Done |
| 2 | Exchange | OKXExchange.cpp:225-273 | placeOrder() — заглушка | ✅ Done |
| 3 | Exchange | KuCoinExchange.cpp:207-258 | placeOrder() — заглушка | ✅ Done |
| 4 | Exchange | BitgetExchange.cpp:214-260 | placeOrder() — заглушка | ✅ Done |
| 5 | Exchange | KuCoinExchange.cpp:179-188 | Kline high/low перепутаны (данные OHLC невалидны) | ✅ Done — Ложное срабатывание: KuCoin API формат [time, open, close, high, low, volume] подтверждён, код корректен |
| 6 | Exchange | Bybit/OKX/KuCoin/Bitget subscribeKline() | symbol и interval игнорируются — подписка не работает | ✅ Done |
| 7 | Trading | PaperTrading.cpp:124 | Двойной подсчёт equity (posValue += qty * entryPrice + pnl) | ✅ Done — Исправлено: qty * currentPrice |
| 8 | Backtest | BacktestEngine.cpp:108 | Некорректный расчёт equity при SHORT позициях | ✅ Done — Исправлено: posQty * (2*entryPrice - price) |
| 9 | Strategy | RiskManager.cpp:49-50 | Trailing stop для SHORT: используется highSince вместо lowSince | ⚠️ Частично — Логика корректна при правильном вызове, но параметр `highSince` конфузен |
| 10 | Scheduler | Scheduler.cpp:122-135 | Задания планировщика НИКОГДА не исполняются (нет callback) | ✅ Done — job.callback() вызывается |
| **NEW** | Exchange | Все 5 бирж | **Нет setLeverage() / getLeverage() для фьючерсов** — невозможно установить плечо через API | ❌ Открыт |
| **NEW** | Exchange | Все 5 бирж | **cancelOrder() не реализован** — невозможно отменить ордер | ❌ Открыт |

### 🟠 ВЫСОКИЕ (влияют на корректность)
| # | Модуль | Файл:Строка | Проблема | Статус |
|---|--------|-------------|----------|--------|
| 11 | Exchange | BybitExchange — нет rate limiting | Риск бана API при интенсивной работе | ✅ Done — Rate limiting реализован (BybitExchange.cpp:57-70) |
| 12 | Exchange | OKX/Bybit/KuCoin getBalance() | Нет проверки границ массива [0] — crash на пустом ответе | ✅ Done — safeStod() защита реализована |
| 13 | Exchange | OKXExchange.cpp:234-236 | BTC баланс: availBal вместо totalBal — неверная сумма | ⚠️ Открыт |
| 14 | ML | XGBoostModel.cpp:36 | Утечка памяти — booster_ не освобождается при переобучении | ❌ Открыт |
| 15 | ML | LSTMModel.cpp:99 | Числовая нестабильность: log(out+1e-9) вместо log_softmax | ❌ Открыт |
| 16 | ML | SignalEnhancer.cpp:72-78 | Веса ансамбля не нормализуются обратно к 1.0 | ✅ Done — Веса нормализуются корректно |
| 17 | AI | OnlineLearningLoop.cpp:131 | Стейт извлекается в текущий момент, а не на момент входа в сделку | ❌ Открыт |
| 18 | AI | RLTradingAgent.cpp:303 | GAE bootstrap: nextVal=0 для нетерминального шага | ✅ Done — values[t+1] используется корректно |
| 19 | Trading | GridBot.cpp:62 | Profit = price * 0.001 — заглушка, а не реальный расчёт | ✅ Done — (sellPrice - buyPrice) * qty |
| 20 | Core | Engine.cpp:94-95 | Race condition: componentsInitialized_ не atomic | ❌ Открыт |
| 21 | Trading | TradeHistory.cpp:98-100 | Race condition в winRate(): два отдельных lock | ❌ Открыт |
| 22 | Trading | TradeHistory.cpp:128 | profitFactor(): breakeven (pnl==0) считается как loss | ⚠️ Открыт — pnl==0 добавляет 0 в losses (минимальное влияние) |
| 23 | Integration | WebhookServer.cpp:33-36 | Rate-limit map растёт бесконтрольно — утечка памяти | ❌ Открыт |
| 24 | Settings | AppGui.cpp:464-524 | Фильтры и флаги индикаторов НЕ сохраняются в JSON | ❌ Открыт |

### 🟡 СРЕДНИЕ (функциональные дефекты)
| # | Модуль | Файл:Строка | Проблема | Статус |
|---|--------|-------------|----------|--------|
| 25 | ML | FeatureExtractor.cpp:87 | Division by zero: hl/c.close без проверки close>0 | ✅ Done — Проверки ema>0 и hl>0 добавлены |
| 26 | ML | ModelTrainer.cpp:16 | Хардкод 96 свечей/день (только 15-мин таймфрейм) | ❌ Открыт |
| 27 | AI | FeatureExtractor.cpp:247-250 | FVG detection: деление на ma без проверки ma > 0 | ❌ Открыт |
| 28 | AI | FeatureExtractor.cpp:110 | mid < 1e-12 → mid=1.0: фиктивное значение портит нормализацию | ❌ Открыт |
| 29 | Settings | LayoutManager.h vs AppGui.h | Несовпадение дефолтов: vdPct 0.15/0.13, indPct 0.20/0.25 | ❌ Открыт |
| 30 | UI | AppGui.h:339-340 | chartBarWidth_, chartScrollOffset_ не сохраняются | ❌ Открыт |
| 31 | Data | CSVExporter.cpp:45-49 | Нет экранирования запятых в CSV (поля с , ломают файл) | ❌ Открыт |
| 32 | External | NewsFeed.cpp:46-50, FearGreed.cpp:35 | API-вызовы — заглушки, реальные данные не получаются | ❌ Открыт |
| 33 | Integration | TelegramBot.cpp:40-56 | Все команды — заглушки, сообщения не отправляются | ❌ Открыт |
| 34 | Core | Engine.cpp:34, 308 | Хардкод: maxCandleHistory=5000, orderbook interval=2000ms | ❌ Открыт |
| 35 | Backtest | BacktestEngine.cpp:62 | Хардкод: позиция всегда 95% от баланса | ❌ Открыт |
| 36 | Tax | TaxReporter.cpp:46-47 | Потеря точности costBasis при частичных fills | ❌ Открыт |
| 37 | Security | WebhookServer.cpp:59 | String comparison не constant-time (timing attack) | ❌ Открыт |
| **NEW** | Exchange | Bybit/KuCoin/Bitget | **getPositionRisk() не реализован** — позиции фьючерсов не получаются | ❌ Открыт |
| **NEW** | Exchange | Bybit/KuCoin/Bitget | **getFuturesBalance() не реализован** — баланс фьючерсов не получается | ❌ Открыт |

---

## 2. АНАЛИЗ БИРЖ

### 2.1 Интерфейс IExchange (IExchange.h)
Обязательные методы: `testConnection()`, `getKlines()`, `getPrice()`, `placeOrder()`, `getBalance()`, `subscribeKline()`, `connect()`, `disconnect()`.
Дополнительные методы: `getPositionRisk()`, `getFuturesBalance()`, `getOpenOrders()`, `getMyTrades()`, `cancelOrder()`.

### 2.2 Статус реализации по биржам (обновлено v1.7.2)

| Функция | Binance | Bybit | OKX | KuCoin | Bitget |
|---------|---------|-------|-----|--------|--------|
| getPrice() | ✅ | ✅ | ✅ | ✅ | ✅ |
| getKlines() | ✅ | ✅ | ✅ | ✅ | ✅ |
| getBalance() | ✅ | ✅ | ⚠️ availBal | ✅ | ✅ |
| placeOrder() | ✅ | ✅ Done | ✅ Done | ✅ Done | ✅ Done |
| cancelOrder() | ❌ none | ❌ none | ❌ none | ❌ none | ❌ none |
| subscribeKline() | ✅ | ✅ Done | ✅ Done | ✅ Done | ✅ Done |
| Rate Limiting | ✅ full | ✅ Done | ✅ 10req/s | ✅ 10req/s | ✅ 10req/s |
| Authentication | ✅ HMAC-SHA256 | ✅ HMAC-SHA256 | ✅ HMAC-SHA256+Base64 | ✅ HMAC-SHA256+Base64 | ✅ HMAC-SHA256+Base64 |
| getPositionRisk() | ✅ | ❌ none | ✅ | ❌ none | ❌ none |
| getFuturesBalance() | ✅ | ❌ none | ❌ none | ❌ none | ❌ none |
| **setLeverage()** | ❌ none | ❌ none | ❌ none | ❌ none | ❌ none |
| **getLeverage()** | ❌ none | ❌ none | ❌ none | ❌ none | ❌ none |

### 2.3 Конкретные баги бирж

**KuCoin getKlines() — ✅ ПОДТВЕРЖДЕНО КОРРЕКТНЫМ (v1.7.2):**
```cpp
// KuCoinExchange.cpp:179-188 — ПРАВИЛЬНЫЙ:
c.open  = safeStod(k[1]); // ✓ open
c.close = safeStod(k[2]); // ✓ close (KuCoin: [time, open, close, high, low, vol])
c.high  = safeStod(k[3]); // ✓ high
c.low   = safeStod(k[4]); // ✓ low
// KuCoin API формат подтверждён: [time, open, close, high, low, volume, turnover]
// Это отличается от стандартного OHLCV, но код правильно обрабатывает формат KuCoin
```

**WebSocket subscribeKline() — ✅ ИСПРАВЛЕНО (v1.7.2):**
Все 4 биржи теперь корректно подписываются на конкретный символ и интервал:
- Bybit: `{"op":"subscribe","args":["kline.{interval}.{symbol}"]}`
- OKX: `{"op":"subscribe","args":[{"channel":"candle{interval}","instId":"{symbol}"}]}`
- KuCoin: `{"type":"subscribe","topic":"/market/candles:{symbol}_{interval}"}`
- Bitget: `{"op":"subscribe","args":[{"instType":"SPOT","channel":"candle{interval}","instId":"{symbol}"}]}`

**OKX BTC баланс — ❌ ОТКРЫТ:**
```cpp
// OKXExchange.cpp — используется availBal вместо totalBal:
bal.btcBalance = safeStod(d.value("availBal", "0")); // ❌ availBal ≠ total
// Нужно: использовать "cashBal" или "eq" для полного баланса
```

### 2.4 Фьючерсное плечо (Leverage) — ❌ НОВАЯ КРИТИЧЕСКАЯ ПРОБЛЕМА

**Текущее состояние:**
- `PositionInfo.leverage` (IExchange.h:74) — поле есть, парсится из ответа API (Binance: `"leverage"`, OKX: `"lever"`)
- Leverage ОТОБРАЖАЕТСЯ в UI: `ImGui::Text("Leverage: %.0fx", p.leverage)` (AppGui.cpp:3094)
- **НО**: Нет API методов для УСТАНОВКИ или ПОЛУЧЕНИЯ плеча перед открытием позиции
- **getPositionRisk()** реализован только для Binance и OKX; Bybit, KuCoin, Bitget — заглушки

**Требуется реализация:**
| Биржа | SET Leverage API | GET Leverage API |
|-------|-----------------|------------------|
| Binance | POST /fapi/v1/leverage | GET /fapi/v2/positionRisk (leverage field) |
| Bybit | POST /v5/position/set-leverage | GET /v5/position/list |
| OKX | POST /api/v5/account/set-leverage | GET /api/v5/account/leverage-info |
| KuCoin | POST /api/v1/position/risk-limit-level/change | GET /api/v1/positions |
| Bitget | POST /api/v2/mix/account/set-leverage | GET /api/v2/mix/position/all-position |

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

### 3.1 PaperTrading — ✅ Done (v1.7.2)
```cpp
// PaperTrading.cpp:124 — ИСПРАВЛЕНО:
posValue += p.quantity * p.currentPrice;  // ✅ Корректно
```
**Equity рассчитывается правильно через текущую рыночную цену.**

### 3.2 BacktestEngine — ✅ Done (v1.7.2)
```cpp
// BacktestEngine.cpp:105-108 — ИСПРАВЛЕНО:
if (positionSide == "BUY")
    equity = balance + price * posQty;          // ✅ LONG
else
    equity = balance + posQty * (2.0 * entryPrice - price);  // ✅ SHORT
```

### 3.3 RiskManager — ⚠️ Частично (именование параметра)
```cpp
// RiskManager.cpp:49-50 — Логика корректна, но параметр конфузен:
return std::min(entryPrice,
                highSince + cfg_.atrStopMultiplier * atr);
// Комментарий в коде: "highSince should be passed as the lowest price since entry"
// РЕКОМЕНДАЦИЯ: Переименовать параметр в lowestSince или добавить отдельный параметр
```

### 3.4 GridBot — ✅ Done (v1.7.2)
```cpp
// GridBot.cpp:64-70 — ИСПРАВЛЕНО:
if (lvl.hasBuy && lvl.buyPrice > 0.0) {
    double qty = lvl.quantity;
    if (qty <= 0.0) { qty = 1.0; }
    realizedProfit_ += (lvl.price - lvl.buyPrice) * qty;  // ✅ Реальный расчёт
}
```
GridLevel имеет поля `buyPrice` и `quantity` (GridBot.h:21-22).

### 3.5 Scheduler — ✅ Done (v1.7.2)
```cpp
// Scheduler.cpp:127-134 — ИСПРАВЛЕНО:
if (job.callback) {
    try {
        job.callback();  // ✅ Callback вызывается!
    } catch (const std::exception& e) {
        Logger::get()->warn("[Scheduler] Job {} failed: {}", job.name, e.what());
    }
}
```
ScheduledJob имеет поле `std::function<void()> callback` (Scheduler.h:23).

### 3.6 TradeHistory — ❌ race condition (открыт)
```cpp
// TradeHistory.cpp:97-100 — ПРОБЛЕМА: три раздельных lock:
double TradeHistory::winRate() const {
    int total = totalTrades();   // lock1 → unlock
    if (total == 0) return 0.0;
    return static_cast<double>(winCount()) / total;  // lock2 → unlock → total устарел!
}
// FIX: один lock_guard на весь расчёт

// TradeHistory.cpp:128 — breakeven pnl==0:
else totalLosses += std::abs(t.pnl); // pnl==0 → 0 добавляется (минимальное влияние)
// Рекомендация: else if (t.pnl < 0) totalLosses += std::abs(t.pnl);
```

### 3.7 OrderManagement — хардкоды (открыт)
- Нет валидации price > 0
- Форматирование: `std::to_string(accountBalance)` → `"$1000.000000"`
- estimateCost не учитывает комиссию

---

## 4. АНАЛИЗ ML/AI МОДУЛЕЙ

### 4.1 LSTMModel — ❌ Открыт
- **Проблема:** `torch::log(out + 1e-9)` — числовая нестабильность. Нужно `torch::log_softmax(logits, 1)` → `nll_loss`.
- **Проблема:** lr=0.001, patience=5 — хардкод; нужен LSTMConfig.
- **Проблема:** Нет валидации размера входного вектора.
- **Stub mode:** возвращает {0.333, 0.334, 0.333} без предупреждения в UI.

### 4.2 XGBoostModel — ❌ Утечка памяти (открыт)
- **УТЕЧКА ПАМЯТИ:** `XGBoosterFree(booster_)` не вызывается перед повторным `train()` (XGBoostModel.cpp:36).
- **Проблема:** outResult может содержать <3 float → мусорные данные.
- **Проблема:** NaN из feature → NAN как missing value, но нет обработки NaN в output.

### 4.3 FeatureExtractor (ML) — ✅ Done (v1.7.2)
- **Division by zero:** ✅ Исправлено — `ema > 0.0 ?` и `hl > 0.0 ?` проверки добавлены (строки 82, 87).
- **Silent masking:** `if (b <= 0.0) return 0.0` — маскирует нулевые цены. (косметическое)
- **Хардкод:** lags {1,3,5,10,20}, volatility windows {5,10,20} — не конфигурируемы. (открыт)
- **OBV overflow:** кумулятивный OBV растёт без bounds для долго работающих процессов. (открыт)

### 4.4 SignalEnhancer — ✅ Done (v1.7.2)
- **Нормализация весов:** ✅ Исправлено — веса нормализуются корректно при изменении.
- **Мёртвая зона:** При accuracy 40-60% веса не обновляются — могут застаиваться навсегда. (открыт)
- **Race condition:** outcomes_ не thread-safe. (открыт)

### 4.5 RLTradingAgent (PPO) — ✅ GAE Done (v1.7.2)
- **GAE bootstrap:** ✅ Исправлено — `nextVal = (t + 1 < T) ? values[t + 1] : 0.0f` (RLTradingAgent.cpp:303).
- **Нет clipping value loss:** PPO paper рекомендует clip value loss. (открыт)
- **Entropy coeff:** 0.01 — слишком мал для volatile крипторынка. (открыт)
- **Хардкод:** orthogonal init scales, mini-batch size. (открыт)

### 4.6 OnlineLearningLoop — ❌ Открыт
- **ГЛАВНЫЙ БАГ:** `exp.state = feat_.extract(agent_.getMode())` — берёт ТЕКУЩИЙ стейт, а не стейт на момент входа в сделку. RL-агент обучается на неправильных state-action парах.
- **nextState == state:** `exp.nextState = exp.state` — bootstrap нарушен.
- **logProb/value = 0:** Исторические сделки не имеют оригинальных logProb/value — GAE невалиден.
- **Race:** lastTradeTime_ не atomic.
- **Unbounded growth:** localBatch_ растёт если training медленнее сбора.

### 4.7 ModelTrainer — ❌ Открыт
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

### 7.1 Статистика (v1.7.2)
- **Всего тестов:** 416 (412 pass, 4 skip — LibTorch/XGBoost)
- **Файлов тестов:** 17
- **Оценка покрытия:** ~30% кода
- **Время выполнения:** ~9 секунд

### 7.2 Критически не протестировано
| Модуль | Что не тестируется |
|--------|--------------------|
| Exchanges | cancelOrder(), setLeverage(), getPositionRisk() (Bybit/KuCoin/Bitget) |
| WebSocket | subscribeKline() с реальными данными |
| PaperTrading | executeBuy/executeSell с реальной P&L логикой |
| BacktestEngine | Полный цикл бэктеста с результатами |
| RiskManager | validateOrder() под нагрузкой |
| ML (LSTM/XGBoost) | Реальные prediction с LibTorch/XGBoost (4 теста skip) |
| GridBot | executeBuy/executeSell уровни |
| AlertManager | checkCondition() триггеры |
| Concurrent access | Race conditions под нагрузкой |
| **Futures** | **Leverage set/get, position risk, futures balance** |

---

## 8. АНАЛИЗ CHANGELOG

### Ключевые наблюдения из CHANGELOG:
1. **v1.7.2** — полный анализ и верификация: 15 ранее исправленных проблем подтверждены, 22+ оставшихся задокументированы
2. **v1.7.1** — Windows compilation fixes (xcopy, C4005)
3. **v1.7.0** — крупное обновление: LibTorch + XGBoost + 46 стресс-тестов
4. **v1.6.0** — Tile Layout + AI Status + все 5 бирж getBalance()
5. **v1.5.3** — VT иконка + 75 системных тестов
6. **Позитивная тенденция:** между v1.7.0 и v1.7.2 исправлены 15 критических/высоких проблем (placeOrder, subscribeKline, equity, GridBot, Scheduler, rate limits)
7. **Остаётся:** cancelOrder() для всех бирж, setLeverage() для фьючерсов, thread-safety, интеграции (Telegram, NewsFeed)

---

## 9. ПОЛНЫЙ ПРОМТ ДЛЯ ИСПРАВЛЕНИЯ

---

### 🎯 ПРОМТ (PROMPT) ДЛЯ СЛЕДУЮЩЕГО ЭТАПА РАЗРАБОТКИ v1.8.0

```
Ты — senior C++ разработчик, работающий над проектом CryptoTrader (VT — Virtual Trade System) v1.7.2.
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

=== СТАТУС ПРОЕКТА (v1.7.2) ===

ИСПРАВЛЕНО (✅ Done — НЕ ТРОГАТЬ):
- placeOrder() для всех 5 бирж (Binance, Bybit, OKX, KuCoin, Bitget)
- subscribeKline() для всех 5 бирж с корректными подписками
- KuCoin getKlines() — формат [time, open, close, high, low, volume] подтверждён
- PaperTrading equity — posValue += p.quantity * p.currentPrice
- BacktestEngine equity для SHORT — posQty * (2.0 * entryPrice - price)
- GridBot profit — (sellPrice - buyPrice) * qty с buyPrice полем
- Scheduler callback — job.callback() вызывается
- ml/FeatureExtractor division by zero — проверки ema>0, hl>0
- SignalEnhancer нормализация весов — веса суммируются в 1.0
- RLTradingAgent GAE bootstrap — values[t+1] для нетерминального шага
- Bybit rate limiting — скользящее окно реализовано
- Bybit/OKX getBalance() bounds — safeStod() защита

=== ЗАДАЧА ===

Необходимо исправить ВСЕ оставшиеся логические ошибки и усовершенствовать программу.
Работу нужно выполнять ПОЭТАПНО, каждый этап — отдельный коммит.
После каждого исправления — запускай тесты для проверки.
НЕ ЛОМАЙ существующие тесты. НЕ ИЗМЕНЯЙ уже исправленный код (помечен ✅ Done).

=== ЭТАП 1: ФЬЮЧЕРСНОЕ ПЛЕЧО (LEVERAGE API) — КРИТИЧЕСКИЙ ===

ОБОСНОВАНИЕ: В режиме Фьючерсной торговли ОБЯЗАТЕЛЬНО должно быть Плечо,
подтягиваемое через API для каждой пары на всех биржах.

1.1. Добавь в IExchange.h виртуальные методы:
   virtual bool setLeverage(const std::string& symbol, int leverage) { return false; }
   virtual int getLeverage(const std::string& symbol) { return 1; }

1.2. Реализуй setLeverage() для Binance (BinanceExchange.cpp):
   - Endpoint: POST /fapi/v1/leverage
   - Body: symbol=BTCUSDT&leverage=20
   - Подпись: HMAC-SHA256 (уже реализовано для placeOrder)
   - Обработка ошибок (400 = Invalid leverage)

1.3. Реализуй setLeverage() для Bybit (BybitExchange.cpp):
   - Endpoint: POST /v5/position/set-leverage
   - Body: {"category":"linear","symbol":"BTCUSDT","buyLeverage":"20","sellLeverage":"20"}
   - Используй существующий signRequest()

1.4. Реализуй setLeverage() для OKX (OKXExchange.cpp):
   - Endpoint: POST /api/v5/account/set-leverage
   - Body: {"instId":"BTC-USDT-SWAP","lever":"20","mgnMode":"cross"}
   - Используй существующий httpPost()

1.5. Реализуй setLeverage() для KuCoin (KuCoinExchange.cpp):
   - Endpoint: POST /api/v2/changeLeverage (или /api/v1/position/risk-limit-level/change)
   - Body: {"symbol":"BTCUSDTM","leverage":"20"}
   - Используй существующий httpPost()

1.6. Реализуй setLeverage() для Bitget (BitgetExchange.cpp):
   - Endpoint: POST /api/v2/mix/account/set-leverage
   - Body: {"symbol":"BTCUSDT","productType":"USDT-FUTURES","marginCoin":"USDT","leverage":"20"}
   - Используй существующий httpPost()

1.7. Реализуй getPositionRisk() для Bybit, KuCoin, Bitget:
   - Bybit: GET /v5/position/list?category=linear&symbol={symbol}
   - KuCoin: GET /api/v1/positions?symbol={symbol}
   - Bitget: GET /api/v2/mix/position/all-position
   - Парси leverage, marginType, positionAmt, entryPrice, unrealizedProfit

1.8. Добавь UI элемент выбора плеча в AppGui.cpp (Settings/Futures):
   - Слайдер/input "Leverage: 1x - 125x" для текущей пары
   - При изменении — вызов exchange->setLeverage(symbol, value)
   - Отображение текущего плеча из getPositionRisk()

=== ЭТАП 2: cancelOrder() ДЛЯ ВСЕХ БИРЖ ===

2.1. Добавь в IExchange.h:
   virtual bool cancelOrder(const std::string& symbol, const std::string& orderId) { return false; }

2.2. Реализуй cancelOrder() для каждой биржи:
   - Binance: DELETE /api/v3/order (spot) или DELETE /fapi/v1/order (futures)
     Query: symbol=BTCUSDT&orderId={orderId}
   - Bybit: POST /v5/order/cancel
     Body: {"category":"spot","symbol":"BTCUSDT","orderId":"{orderId}"}
   - OKX: POST /api/v5/trade/cancel-order
     Body: {"instId":"BTC-USDT","ordId":"{orderId}"}
   - KuCoin: DELETE /api/v1/orders/{orderId}
   - Bitget: POST /api/v2/spot/trade/cancel-order
     Body: {"symbol":"BTCUSDT","orderId":"{orderId}"}

2.3. Добавь кнопку "Cancel" рядом с каждым открытым ордером в UI.

=== ЭТАП 3: ИСПРАВЛЕНИЯ THREAD-SAFETY ===

3.1. Сделай Engine::componentsInitialized_ атомарным (Engine.h):
   БЫЛО:  bool componentsInitialized_{false};
   НУЖНО: std::atomic<bool> componentsInitialized_{false};

3.2. Исправь TradeHistory::winRate() race condition (TradeHistory.cpp:97-100):
   double TradeHistory::winRate() const {
       std::lock_guard<std::mutex> lk(mutex_);
       if (trades_.empty()) return 0.0;
       int wins = 0;
       for (auto& t : trades_) if (t.pnl > 0) wins++;
       return static_cast<double>(wins) / trades_.size();
   }

3.3. Исправь TradeHistory::profitFactor() — breakeven (TradeHistory.cpp:128):
   if (t.pnl > 0) totalWins += t.pnl;
   else if (t.pnl < 0) totalLosses += std::abs(t.pnl);  // ← добавь if (t.pnl < 0)

3.4. Добавь thread-safety в SignalEnhancer (SignalEnhancer.cpp):
   - Оберни outcomes_ в mutex или используй SPSCQueue

3.5. Сделай OnlineLearningLoop::lastTradeTime_ атомарным.

=== ЭТАП 4: ИСПРАВЛЕНИЯ ML/AI МОДУЛЕЙ ===

4.1. Исправь LSTMModel loss (LSTMModel.cpp:99):
   БЫЛО:  auto loss = torch::nll_loss(torch::log(out + 1e-9), target);
   НУЖНО: auto logProbs = torch::log_softmax(logits, 1);
          auto loss = torch::nll_loss(logProbs, target);

4.2. Исправь утечку памяти XGBoost (XGBoostModel.cpp):
   В начале train():
     if (booster_) { XGBoosterFree(booster_); booster_ = nullptr; }

4.3. Исправь ai/FeatureExtractor FVG division by zero (ai/FeatureExtractor.cpp:254,257):
   БЫЛО:  set(83 + fvgCount, static_cast<float>(bullGap / ma));
   НУЖНО: set(83 + fvgCount, (ma > 0.0) ? static_cast<float>(bullGap / ma) : 0.0f);
   Аналогично для bearGap / ma.

4.4. Исправь OnlineLearningLoop stale state (OnlineLearningLoop.cpp:131):
   - При открытии позиции (PaperTrading/OrderManagement) СОХРАНЯЙ state в TradeRecord
   - В OnlineLearningLoop используй сохранённый state, а не текущий feat_.extract()
   - Если невозможно изменить TradeRecord, кэшируй state_at_entry в map<tradeId, Tensor>

4.5. Сделай ModelTrainer::candlesPerDay конфигурируемым:
   int candlesPerDay = 1440 / timeframeMinutes; // 1m→1440, 5m→288, 15m→96, 1h→24

=== ЭТАП 5: ИСПРАВЛЕНИЯ НАСТРОЕК И UI ===

5.1. Добавь сохранение фильтров в configToJson() (AppGui.cpp):
   j["filters"]["min_volume"] = cfg_.filterMinVolume;
   j["filters"]["min_price"] = cfg_.filterMinPrice;
   j["filters"]["max_price"] = cfg_.filterMaxPrice;
   j["filters"]["min_change"] = cfg_.filterMinChange;
   j["filters"]["max_change"] = cfg_.filterMaxChange;

5.2. Добавь сохранение флагов индикаторов:
   j["indicators_enabled"]["ema"] = cfg_.indEmaEnabled;
   j["indicators_enabled"]["rsi"] = cfg_.indRsiEnabled;
   j["indicators_enabled"]["atr"] = cfg_.indAtrEnabled;
   j["indicators_enabled"]["macd"] = cfg_.indMacdEnabled;
   j["indicators_enabled"]["bb"] = cfg_.indBbEnabled;

5.3. Добавь загрузку этих настроек в loadConfig():
   if (j.contains("filters")) { auto& f = j["filters"]; ... }
   if (j.contains("indicators_enabled")) { auto& ie = j["indicators_enabled"]; ... }

5.4. Синхронизируй дефолты layout:
   AppGui.h: layoutVdPct{0.15f} (вместо 0.13f)
   AppGui.h: layoutIndPct{0.20f} (вместо 0.25f)
   // Привести в соответствие с LayoutManager.h

5.5. Переименуй параметр RiskManager::highSince → priceSinceEntry:
   - Для LONG: передавать highestSince; для SHORT: передавать lowestSince
   - Это устранит путаницу в именовании

=== ЭТАП 6: ИСПРАВЛЕНИЯ БЕЗОПАСНОСТИ И ИНТЕГРАЦИЙ ===

6.1. WebhookServer — ограничь rateLimit_ map (WebhookServer.cpp):
   - Добавь periodic cleanup: удаляй записи старше 1 часа каждые 100 запросов
   - Или: ограничь максимальный размер (10000 IP, LRU eviction)

6.2. WebhookServer — constant-time comparison (WebhookServer.cpp:59):
   Замени: if (secret != secretKey_)
   На: bool match = (secret.size() == secretKey_.size()) &&
                    (CRYPTO_memcmp(secret.data(), secretKey_.data(), secret.size()) == 0);
       if (!match) { ... }
   (OpenSSL CRYPTO_memcmp уже подключен через OpenSSL dependency)

6.3. CSVExporter — добавь экранирование (CSVExporter.cpp):
   Для каждого строкового поля:
     if (field.find_first_of(",\"\n") != std::string::npos)
       field = "\"" + replace_all(field, "\"", "\"\"") + "\"";

6.4. OKX getBalance() — BTC баланс (OKXExchange.cpp):
   Замени: "availBal" → "cashBal" (или "eq") для полного баланса BTC

=== ЭТАП 7: РЕАЛИЗАЦИЯ ЗАГЛУШЕК ===

7.1. Реализуй NewsFeed (NewsFeed.cpp):
   - API: CryptoPanic API (GET /api/v1/posts/?auth_token={key}&kind=news&currencies=BTC,ETH)
   - Или: CoinGecko /api/v3/news (бесплатный)
   - Парси title, source, url, pubTime
   - Маппинг sentiment через keyword analysis (bullish/bearish/neutral)

7.2. Реализуй FearGreed (FearGreed.cpp):
   - API: GET https://api.alternative.me/fng/?limit=1
   - Парси: {"value":"73","value_classification":"Greed","timestamp":"1709683200"}
   - Маппинг в FearGreedData struct

7.3. Реализуй TelegramBot::sendMessage() (TelegramBot.cpp):
   - POST https://api.telegram.org/bot{token}/sendMessage
   - Body: {"chat_id": "{chatId}", "text": "{message}", "parse_mode": "HTML"}
   - Используй libcurl (уже подключен) для HTTP POST
   - Обработка ошибок (401 = invalid token, 400 = bad chat_id)

=== ЭТАП 8: ТЕСТИРОВАНИЕ ===

8.1. Добавь unit-тесты для Leverage API:
   - test_exchanges.cpp: BinanceSetLeverageNoCrash, BybitSetLeverageNoCrash, ...
   - test_exchanges.cpp: BinanceGetPositionRiskNoCrash, ...
   - Проверь дефолтное значение leverage = 1

8.2. Добавь unit-тесты для cancelOrder():
   - test_exchanges.cpp: BinanceCancelOrderNoCrash, ...

8.3. Обнови test_full_system.cpp — проверь:
   - TradeHistory winRate() thread-safe
   - configToJson() сохраняет фильтры и индикаторы
   - Layout defaults совпадают

8.4. Добавь тест ai/FeatureExtractor FVG с ma=0 (no crash).

8.5. Проверь что ВСЕ существующие 416 тестов проходят после изменений.

=== ПРИОРИТЕТЫ ===
Приоритет 1 (ФЬЮЧЕРСЫ): Этап 1 — setLeverage(), getPositionRisk() для всех 5 бирж
Приоритет 2 (ОРДЕРА): Этап 2 — cancelOrder() для всех 5 бирж
Приоритет 3 (СТАБИЛЬНОСТЬ): Этап 3 — thread-safety (atomic, mutex)
Приоритет 4 (ML/AI): Этап 4 — LSTM loss, XGBoost leak, FeatureExtractor FVG
Приоритет 5 (НАСТРОЙКИ): Этап 5 — сохранение фильтров, layout defaults
Приоритет 6 (БЕЗОПАСНОСТЬ): Этап 6 — webhook security, CSV escaping
Приоритет 7 (ИНТЕГРАЦИИ): Этап 7 — NewsFeed, FearGreed, Telegram
Приоритет 8 (ТЕСТЫ): Этап 8 — тесты для всех изменений

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

## 📊 ИТОГОВАЯ СТАТИСТИКА АНАЛИЗА (v1.7.2)

| Категория | Всего найдено | ✅ Исправлено | ❌ Открыто | Новых проблем |
|-----------|--------------|--------------|-----------|---------------|
| Биржи | 14 | 8 | 4 | 4 (setLeverage+getLeverage, cancelOrder, getPositionRisk×3, getFuturesBalance×4) |
| Торговые модули | 10 | 5 | 4 | 0 |
| ML/AI | 18 | 4 | 12 | 0 |
| UI/Настройки | 8 | 0 | 8 | 0 |
| Интеграции | 7 | 0 | 7 | 0 |
| **ИТОГО** | **57** | **17** | **35** | **4** |

### Прогресс исправлений:
- **v1.7.1 → v1.7.2:** 15 из 57 проблем исправлены (26%), включая 6 критических
- **Новые проблемы:** 4 обнаружены (leverage API, cancelOrder, getPositionRisk для 3 бирж, getFuturesBalance для 4 бирж)
- **Общий статус:** 17 Done, 35 Open, 4 New = **39 проблем к исправлению**

---

*Документ обновлён на основе полного анализа кодовой базы CryptoTrader v1.7.2, включая все 67 исходных файлов, 17 тестовых файлов, конфигурационные файлы и CHANGELOG. Полная сборка и запуск 416 тестов подтверждены.*
