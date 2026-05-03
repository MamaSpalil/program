# Code Review — `src/ui`, `src/exchange`, `src/strategy`, `src/trading`

Targeted audit performed alongside the UI/web work in this PR. Findings are
prioritized; **fixed in this PR** items are linked to the change. Other items
are documented for follow-up — they were intentionally not changed to keep
this PR focused and low-risk for the trading core.

## Severity legend

- 🔴 **High** — observable broken behavior or risk to trading correctness
- 🟡 **Medium** — latent bug, edge case, or maintainability issue
- 🟢 **Low** — minor / stylistic

---

## `src/exchange`

### 🔴 `WebSocketClient::send()` was a no-op stub — **FIXED**

`WebSocketClient.cpp` (pre-fix): the `send()` method body was

```cpp
// Implementation would write to beast ws stream
(void)message;
```

…but `BybitExchange`, `OKXExchange`, `KuCoinExchange`, and `BitgetExchange` all
call `ws_->send(sub.dump())` immediately after `connect()` to subscribe to
kline streams. The dropped subscribe payload meant **none of these four
exchanges ever received WebSocket data** — `onWsMessage` was never invoked.
Only Binance was unaffected (it uses URL-path subscription).

Fix: pending payloads are queued under a mutex and flushed by the worker
thread immediately after the TLS/WebSocket handshake. The queue is **not**
drained, so the same subscriptions are automatically re-sent on every
reconnect. See `src/exchange/WebSocketClient.{h,cpp}`.

### 🟡 BinanceExchange — possible WS lifecycle leak

`BinanceExchange.cpp:558-560` re-creates `ws_` on each `subscribeKline` call
without first calling `disconnect()` on the prior instance. If a caller
re-subscribes on the fly the previous worker thread keeps running until
`shouldRun_=false` is read on the next loop iteration. Recommend explicitly
`if (ws_) ws_->disconnect();` before `ws_ = std::make_unique<…>()`.

### 🟡 Reconnect after `read` error doesn't re-flush subscriptions before this PR

Was implicitly broken because subscriptions never went out at all. With the
new fix, the queued snapshot is replayed on each reconnect — verify on a real
network blip (kill TCP, restart) that all 4 non-Binance exchanges resume.

### 🟢 Per-exchange config duplication

Each exchange repeats nearly identical REST URL / signing scaffolding. A
shared `RestClient` helper would reduce ~200 LoC across the 5 exchange files.
Out of scope for this PR.

---

## `src/ui`

### 🔴 Live ticks did not expand the chart Y-axis — **FIXED**

`AppGui.cpp::drawMarketDataWindow` computed `pMin`/`pMax` from the **stored**
`candleHistory[i].high/low` only. When the live tick (via `liveTickHigh_` /
`liveTickLow_`) pushed past the last bar's static high/low, the wick / price
line was clipped at the chart edge until the next candle close. Now the loop
folds in `displayHigh`/`displayLow`/`displayClose`/`liveVol` for the last bar
when it is visible, so the Y-axis grows in real time with each tick.

### 🔴 No empty space to the right of the latest bar — **FIXED**

The latest candle was drawn flush against the price scale, with no breathing
room — unusual in trading UIs and made the chart feel cramped. Added
`GuiConfig::chartRightPadPct` (default 18 %, persisted) and updated the
visible-range calculation to reserve that fraction of `chartW` as empty space
on the right. Also adjusted `needsChartReset_` so "fit last 50 bars" accounts
for the right margin and remains exactly 50 visible candles.

### 🟡 `displayClose > 0.0` checks are inconsistent

In `drawMarketDataWindow`, `displayClose` is set to `livePx > 0 ? livePx :
snap.lastCandle.close`, which is non-zero whenever there's a candle. The
later `displayClose > 0.0` guards are thus near-tautological — but harmless
defensive checks; left alone.

### 🟡 `signalHistory_` grows up to 50 even for HOLD-suppressed signals

`updateState` only appends non-HOLD signals — that's correct — but during a
long sideways market the history is essentially never trimmed below 50
because `pop_front` only fires after 50. If signals stop arriving for hours,
the panel still shows stale ones. Consider time-based aging.

### 🟡 `setupTheme()` re-applies style mid-frame

The new theme switcher invokes `setupTheme()` from inside the menu callback,
i.e. mid-frame. ImGui style is read by subsequent widget calls in the same
frame, so half the frame may still render with the old palette. The visible
glitch is one frame; acceptable but documented.

### 🟢 `kMaxHistory = 200` is shared between price/equity/RSI/volume

If history requirements diverge later, split per-series.

---

## `src/strategy`

### 🟡 `MLEnhancedStrategy::checkEarlyExit` favors take-profit on a gap candle

When a single candle has `low <= stopLoss` AND `high >= takeProfit`, the code
exits at `takeProfit`. On a real fill the order book sequence determines which
hit first; we cannot know intra-bar order. This is an **optimistic
assumption** that overstates backtest performance. Recommend: pessimistic
exit at `stopLoss` for backtest realism, or treat such bars as ambiguous
and exit at the candle close.

Not changed here — backtest reports may have been calibrated against this
behavior, so changing it without coordination would shift historical metrics.

### 🟡 `RiskManager::trailingStop` uses `std::max(entryPrice, …)` for longs

That floor pins the trailing stop to the entry price — i.e. the trailing
stop never moves *below* entry, but it also never moves up past `entryPrice`
unless `priceSinceEntry - atr*mul > entryPrice`. That's the intended
"break-even floor", but the parameter `priceSinceEntry` is described as
"the highest price since entry" in the comment — callers must enforce that
themselves. There's no peak-tracking here. If a caller passes the *current*
price instead of the running peak, the stop will ratchet down on a pullback,
defeating the purpose. Recommend adding a `peakPrice` member to `Position`
and computing the trail from that single source.

### 🟢 `kellySizing` clamps Kelly to `2 * maxRiskPerTrade`

Reasonable, but undocumented. Add a comment that this is "half-Kelly with a
hard cap" sizing.

---

## `src/trading`

### 🟡 `PaperTrading::openPosition` debits cost for SELL/short

```cpp
double cost = qty * price;
if (cost + commission > account_.balance) return false;
account_.balance -= (cost + commission);
```

For a long this is correct. For a short sale, real exchanges *credit*
proceeds and lock margin instead. The current model treats shorts as if you
bought negative-quantity units — `closePosition` then computes the right PnL
sign, but the intermediate balance figure during an open short is
nonsensical. For paper-trading parity with real exchange PnL this works at
close, but reported `equity` while a short is open will be wrong by `2 *
qty * price`. Recommend either:
- restrict `PaperTrading` to long-only, or
- add proper margin accounting (`balance_used`, `margin`) similar to futures.

### 🟡 `PaperTrading::closePosition` finds **first** matching symbol

If the same symbol is held in both LONG and SHORT (hedge mode) only the
first match is closed and `currentPrice` is used regardless of side. Fine for
single-position-per-symbol mode; document the assumption or take a `side`
argument.

### 🟢 No persistence of `commissionRate_`

`saveToFile`/`loadFromFile` round-trip account state but not the configured
commission rate. Restart with a different default would silently change
back-test economics.

---

## Cross-cutting

- **Logger format strings** mostly use `{}` (spdlog-style) consistently. ✓
- **No `using namespace std;`** in headers. ✓
- **Mutex coverage**: most state-mutating methods take `stateMutex_` /
  `mutex_`. The new live-tick path is correctly lock-free (atomics).
- **Thread sanitizer**: not run in this PR (no working build env in the
  sandbox). Recommend a one-off `-fsanitize=thread` build for the WS+UI
  interaction now that subscriptions actually go out.

---

## Out-of-scope for this PR (deliberately deferred)

- Refactor of `AppGui.cpp` (4 600 LoC monolith) into per-window files.
- Adding a true REST/WebSocket bridge from the C++ engine to the new
  `web-version-php/` so the dashboard displays live data instead of reading
  a JSON snapshot file.
- Backtest semantics for ambiguous SL/TP candles.
- PaperTrading short-sale accounting model.
