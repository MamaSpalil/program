// Technical indicator calculations ported from C++

export class Indicators {
  // Exponential Moving Average
  static ema(data: number[], period: number): number[] {
    if (data.length < period) {
      throw new Error('Insufficient data for EMA calculation');
    }

    const result: number[] = [];
    const multiplier = 2 / (period + 1);

    // Calculate SMA for first value
    let sum = 0;
    for (let i = 0; i < period; i++) {
      sum += data[i];
    }
    const sma = sum / period;
    result.push(sma);

    // Calculate EMA for remaining values
    for (let i = period; i < data.length; i++) {
      const ema = (data[i] - result[result.length - 1]) * multiplier + result[result.length - 1];
      result.push(ema);
    }

    return result;
  }

  // Relative Strength Index
  static rsi(data: number[], period: number = 14): number[] {
    if (data.length < period + 1) {
      throw new Error('Insufficient data for RSI calculation');
    }

    const result: number[] = [];
    const gains: number[] = [];
    const losses: number[] = [];

    // Calculate price changes
    for (let i = 1; i < data.length; i++) {
      const change = data[i] - data[i - 1];
      gains.push(change > 0 ? change : 0);
      losses.push(change < 0 ? -change : 0);
    }

    // Calculate first average gain and loss
    let avgGain = gains.slice(0, period).reduce((a, b) => a + b, 0) / period;
    let avgLoss = losses.slice(0, period).reduce((a, b) => a + b, 0) / period;

    // Calculate RSI
    for (let i = period; i < gains.length; i++) {
      avgGain = (avgGain * (period - 1) + gains[i]) / period;
      avgLoss = (avgLoss * (period - 1) + losses[i]) / period;

      const rs = avgLoss === 0 ? 100 : avgGain / avgLoss;
      const rsi = 100 - 100 / (1 + rs);
      result.push(rsi);
    }

    return result;
  }

  // Average True Range
  static atr(high: number[], low: number[], close: number[], period: number = 14): number[] {
    if (high.length < period + 1 || low.length < period + 1 || close.length < period) {
      throw new Error('Insufficient data for ATR calculation');
    }

    const trueRanges: number[] = [];

    // Calculate true ranges
    for (let i = 1; i < high.length; i++) {
      const tr1 = high[i] - low[i];
      const tr2 = Math.abs(high[i] - close[i - 1]);
      const tr3 = Math.abs(low[i] - close[i - 1]);
      trueRanges.push(Math.max(tr1, tr2, tr3));
    }

    // Calculate ATR using EMA
    return this.ema(trueRanges, period);
  }

  // MACD (Moving Average Convergence Divergence)
  static macd(
    data: number[],
    fastPeriod: number = 12,
    slowPeriod: number = 26,
    signalPeriod: number = 9
  ): { macd: number[]; signal: number[]; histogram: number[] } {
    const fastEma = this.ema(data, fastPeriod);
    const slowEma = this.ema(data, slowPeriod);

    // Calculate MACD line
    const macdLine: number[] = [];
    const offset = slowPeriod - fastPeriod;
    for (let i = 0; i < slowEma.length; i++) {
      macdLine.push(fastEma[i + offset] - slowEma[i]);
    }

    // Calculate signal line
    const signalLine = this.ema(macdLine, signalPeriod);

    // Calculate histogram
    const histogram: number[] = [];
    const signalOffset = signalPeriod - 1;
    for (let i = 0; i < signalLine.length; i++) {
      histogram.push(macdLine[i + signalOffset] - signalLine[i]);
    }

    return {
      macd: macdLine,
      signal: signalLine,
      histogram,
    };
  }

  // Bollinger Bands
  static bollingerBands(
    data: number[],
    period: number = 20,
    stdDev: number = 2
  ): { upper: number[]; middle: number[]; lower: number[] } {
    if (data.length < period) {
      throw new Error('Insufficient data for Bollinger Bands calculation');
    }

    const upper: number[] = [];
    const middle: number[] = [];
    const lower: number[] = [];

    for (let i = period - 1; i < data.length; i++) {
      const slice = data.slice(i - period + 1, i + 1);

      // Calculate SMA (middle band)
      const sma = slice.reduce((a, b) => a + b, 0) / period;
      middle.push(sma);

      // Calculate standard deviation
      const variance = slice.reduce((sum, val) => sum + Math.pow(val - sma, 2), 0) / period;
      const std = Math.sqrt(variance);

      // Calculate bands
      upper.push(sma + stdDev * std);
      lower.push(sma - stdDev * std);
    }

    return { upper, middle, lower };
  }

  // Simple Moving Average
  static sma(data: number[], period: number): number[] {
    if (data.length < period) {
      throw new Error('Insufficient data for SMA calculation');
    }

    const result: number[] = [];

    for (let i = period - 1; i < data.length; i++) {
      const slice = data.slice(i - period + 1, i + 1);
      const avg = slice.reduce((a, b) => a + b, 0) / period;
      result.push(avg);
    }

    return result;
  }

  // Stochastic Oscillator
  static stochastic(
    high: number[],
    low: number[],
    close: number[],
    period: number = 14,
    smoothK: number = 3,
    smoothD: number = 3
  ): { k: number[]; d: number[] } {
    if (high.length < period || low.length < period || close.length < period) {
      throw new Error('Insufficient data for Stochastic calculation');
    }

    const rawK: number[] = [];

    // Calculate %K
    for (let i = period - 1; i < close.length; i++) {
      const highSlice = high.slice(i - period + 1, i + 1);
      const lowSlice = low.slice(i - period + 1, i + 1);

      const highestHigh = Math.max(...highSlice);
      const lowestLow = Math.min(...lowSlice);

      const k =
        highestHigh === lowestLow
          ? 50
          : ((close[i] - lowestLow) / (highestHigh - lowestLow)) * 100;
      rawK.push(k);
    }

    // Smooth %K
    const k = this.sma(rawK, smoothK);

    // Calculate %D (SMA of %K)
    const d = this.sma(k, smoothD);

    return { k, d };
  }
}
