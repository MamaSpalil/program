# User Indicators (Pine Script v6)

Place your Pine Script v6 indicator files (`.pine` extension) in this directory.

The program will automatically:
1. Detect new `.pine` files on startup
2. Parse and convert them to the internal format
3. Generate corresponding C++ code (`.generated.h` files)
4. Run the indicators on chart data when an instrument is selected

## Supported Pine Script v6 Features

- `//@version=6` version annotation
- `indicator("name", overlay=true/false)`
- `input.int()`, `input.float()`, `input.source()` — input parameters
- `ta.ema()`, `ta.sma()`, `ta.rsi()`, `ta.atr()` — technical indicators
- `ta.crossover()`, `ta.crossunder()` — crossover detection
- `ta.highest()`, `ta.lowest()` — range functions
- `math.abs()`, `math.sqrt()`, `math.log()`, `math.max()`, `math.min()`
- `nz()` — replace NaN values
- `plot()` — output values
- Built-in series: `close`, `open`, `high`, `low`, `volume`
- Arithmetic, comparison, boolean, and ternary operators
- Series access with `[offset]` syntax
- `var` declarations for persistent variables

## Example

```pine
//@version=6
indicator("My EMA Crossover", overlay=true)

fast = input.int(9, "Fast EMA")
slow = input.int(21, "Slow EMA")

emaFast = ta.ema(close, fast)
emaSlow = ta.ema(close, slow)

plot(emaFast, "Fast EMA")
plot(emaSlow, "Slow EMA")
```
