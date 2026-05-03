// Advanced chart analysis for support/resistance zones and entry/exit signals

interface Candle {
  timestamp: number;
  open: number;
  high: number;
  low: number;
  close: number;
  volume: number;
}

interface Zone {
  type: 'SUPPORT' | 'RESISTANCE' | 'PIVOT';
  priceLevel: number;
  priceTop: number;
  priceBottom: number;
  strength: number;
  touches: number;
  firstTouch: Date;
  lastTouch: Date;
}

interface PivotPoints {
  pivot: number;
  r1: number;
  r2: number;
  r3: number;
  s1: number;
  s2: number;
  s3: number;
}

interface FibonacciLevels {
  high: number;
  low: number;
  level236: number;
  level382: number;
  level500: number;
  level618: number;
  level786: number;
}

export class ChartAnalysis {
  /**
   * Detect support and resistance zones from price history
   * Uses swing highs/lows and price consolidation areas
   */
  static detectSupportResistanceZones(candles: Candle[], sensitivity: number = 0.02): Zone[] {
    if (candles.length < 20) {
      throw new Error('Insufficient data for zone detection (need at least 20 candles)');
    }

    const zones: Zone[] = [];
    const priceThreshold = sensitivity; // 2% default threshold

    // Find swing highs and lows
    const swingHighs = this.findSwingHighs(candles, 5);
    const swingLows = this.findSwingLows(candles, 5);

    // Group similar price levels into zones
    const resistanceZones = this.groupPriceLevels(swingHighs, priceThreshold, 'RESISTANCE');
    const supportZones = this.groupPriceLevels(swingLows, priceThreshold, 'SUPPORT');

    zones.push(...resistanceZones, ...supportZones);

    // Calculate zone strength based on touches and volume
    zones.forEach(zone => {
      zone.strength = this.calculateZoneStrength(zone, candles);
    });

    // Sort by strength (strongest first)
    return zones.sort((a, b) => b.strength - a.strength);
  }

  /**
   * Find swing highs (local peaks)
   */
  private static findSwingHighs(candles: Candle[], period: number = 5): Array<{ price: number; index: number; timestamp: number }> {
    const swingHighs: Array<{ price: number; index: number; timestamp: number }> = [];

    for (let i = period; i < candles.length - period; i++) {
      const current = candles[i].high;
      let isSwingHigh = true;

      // Check if current high is higher than surrounding candles
      for (let j = 1; j <= period; j++) {
        if (current <= candles[i - j].high || current <= candles[i + j].high) {
          isSwingHigh = false;
          break;
        }
      }

      if (isSwingHigh) {
        swingHighs.push({
          price: current,
          index: i,
          timestamp: candles[i].timestamp
        });
      }
    }

    return swingHighs;
  }

  /**
   * Find swing lows (local troughs)
   */
  private static findSwingLows(candles: Candle[], period: number = 5): Array<{ price: number; index: number; timestamp: number }> {
    const swingLows: Array<{ price: number; index: number; timestamp: number }> = [];

    for (let i = period; i < candles.length - period; i++) {
      const current = candles[i].low;
      let isSwingLow = true;

      // Check if current low is lower than surrounding candles
      for (let j = 1; j <= period; j++) {
        if (current >= candles[i - j].low || current >= candles[i + j].low) {
          isSwingLow = false;
          break;
        }
      }

      if (isSwingLow) {
        swingLows.push({
          price: current,
          index: i,
          timestamp: candles[i].timestamp
        });
      }
    }

    return swingLows;
  }

  /**
   * Group similar price levels into zones
   */
  private static groupPriceLevels(
    levels: Array<{ price: number; index: number; timestamp: number }>,
    threshold: number,
    type: 'SUPPORT' | 'RESISTANCE'
  ): Zone[] {
    if (levels.length === 0) return [];

    const zones: Zone[] = [];
    const processed = new Set<number>();

    levels.forEach((level, idx) => {
      if (processed.has(idx)) return;

      const group: typeof levels = [level];
      processed.add(idx);

      // Find similar price levels
      for (let i = idx + 1; i < levels.length; i++) {
        if (processed.has(i)) continue;

        const priceDiff = Math.abs(levels[i].price - level.price) / level.price;
        if (priceDiff <= threshold) {
          group.push(levels[i]);
          processed.add(i);
        }
      }

      // Create zone if we have at least 2 touches
      if (group.length >= 2) {
        const prices = group.map(g => g.price);
        const timestamps = group.map(g => g.timestamp);

        zones.push({
          type,
          priceLevel: prices.reduce((a, b) => a + b, 0) / prices.length,
          priceTop: Math.max(...prices),
          priceBottom: Math.min(...prices),
          strength: 0, // Will be calculated later
          touches: group.length,
          firstTouch: new Date(Math.min(...timestamps)),
          lastTouch: new Date(Math.max(...timestamps)),
        });
      }
    });

    return zones;
  }

  /**
   * Calculate zone strength (0-1) based on multiple factors
   */
  private static calculateZoneStrength(zone: Zone, candles: Candle[]): number {
    let strength = 0;

    // Factor 1: Number of touches (max 0.4)
    const touchScore = Math.min(zone.touches / 5, 1) * 0.4;
    strength += touchScore;

    // Factor 2: Zone tightness (max 0.3)
    const range = zone.priceTop - zone.priceBottom;
    const rangePct = range / zone.priceLevel;
    const tightnessScore = Math.max(0, 1 - rangePct / 0.05) * 0.3;
    strength += tightnessScore;

    // Factor 3: Time span (max 0.3)
    const timeSpan = zone.lastTouch.getTime() - zone.firstTouch.getTime();
    const daySpan = timeSpan / (1000 * 60 * 60 * 24);
    const timeScore = Math.min(daySpan / 30, 1) * 0.3;
    strength += timeScore;

    return Math.min(strength, 1);
  }

  /**
   * Calculate pivot points (standard, Fibonacci, and Camarilla)
   */
  static calculatePivotPoints(candles: Candle[], type: 'standard' | 'fibonacci' | 'camarilla' = 'standard'): PivotPoints {
    if (candles.length === 0) {
      throw new Error('No candles provided for pivot calculation');
    }

    // Use last candle for pivot calculation
    const lastCandle = candles[candles.length - 1];
    const high = lastCandle.high;
    const low = lastCandle.low;
    const close = lastCandle.close;

    const pivot = (high + low + close) / 3;

    if (type === 'standard') {
      return {
        pivot,
        r1: 2 * pivot - low,
        r2: pivot + (high - low),
        r3: high + 2 * (pivot - low),
        s1: 2 * pivot - high,
        s2: pivot - (high - low),
        s3: low - 2 * (high - pivot),
      };
    } else if (type === 'fibonacci') {
      const range = high - low;
      return {
        pivot,
        r1: pivot + 0.382 * range,
        r2: pivot + 0.618 * range,
        r3: pivot + range,
        s1: pivot - 0.382 * range,
        s2: pivot - 0.618 * range,
        s3: pivot - range,
      };
    } else {
      // Camarilla
      const range = high - low;
      return {
        pivot,
        r1: close + range * 1.1 / 12,
        r2: close + range * 1.1 / 6,
        r3: close + range * 1.1 / 4,
        s1: close - range * 1.1 / 12,
        s2: close - range * 1.1 / 6,
        s3: close - range * 1.1 / 4,
      };
    }
  }

  /**
   * Calculate Fibonacci retracement levels
   */
  static calculateFibonacciLevels(candles: Candle[], lookback: number = 50): FibonacciLevels {
    if (candles.length < lookback) {
      lookback = candles.length;
    }

    const recentCandles = candles.slice(-lookback);
    const high = Math.max(...recentCandles.map(c => c.high));
    const low = Math.min(...recentCandles.map(c => c.low));
    const range = high - low;

    return {
      high,
      low,
      level236: high - range * 0.236,
      level382: high - range * 0.382,
      level500: high - range * 0.500,
      level618: high - range * 0.618,
      level786: high - range * 0.786,
    };
  }

  /**
   * Detect zone confluence (multiple zones at similar price levels)
   */
  static detectZoneConfluence(zones: Zone[], threshold: number = 0.005): Array<{ priceLevel: number; zones: Zone[]; strength: number }> {
    const confluences: Array<{ priceLevel: number; zones: Zone[]; strength: number }> = [];

    zones.forEach((zone, idx) => {
      const confluent: Zone[] = [zone];

      // Find zones at similar price levels
      for (let i = idx + 1; i < zones.length; i++) {
        const priceDiff = Math.abs(zones[i].priceLevel - zone.priceLevel) / zone.priceLevel;
        if (priceDiff <= threshold) {
          confluent.push(zones[i]);
        }
      }

      if (confluent.length >= 2) {
        const avgPrice = confluent.reduce((sum, z) => sum + z.priceLevel, 0) / confluent.length;
        const totalStrength = confluent.reduce((sum, z) => sum + z.strength, 0);

        confluences.push({
          priceLevel: avgPrice,
          zones: confluent,
          strength: totalStrength / confluent.length, // Average strength
        });
      }
    });

    // Remove duplicates and sort by strength
    const unique = confluences.filter((conf, idx, self) =>
      idx === self.findIndex(c => Math.abs(c.priceLevel - conf.priceLevel) / conf.priceLevel < 0.001)
    );

    return unique.sort((a, b) => b.strength - a.strength);
  }

  /**
   * Analyze volume profile to find high volume nodes (HVN) and low volume nodes (LVN)
   */
  static analyzeVolumeProfile(candles: Candle[], bins: number = 50): Array<{ priceLevel: number; volume: number; type: 'HVN' | 'LVN' }> {
    if (candles.length === 0) return [];

    const high = Math.max(...candles.map(c => c.high));
    const low = Math.min(...candles.map(c => c.low));
    const range = high - low;
    const binSize = range / bins;

    // Create volume profile bins
    const profile: Array<{ priceLevel: number; volume: number }> = [];
    for (let i = 0; i < bins; i++) {
      profile.push({
        priceLevel: low + (i + 0.5) * binSize,
        volume: 0,
      });
    }

    // Aggregate volume into bins
    candles.forEach(candle => {
      const avgPrice = (candle.high + candle.low) / 2;
      const binIndex = Math.min(Math.floor((avgPrice - low) / binSize), bins - 1);
      if (binIndex >= 0 && binIndex < bins) {
        profile[binIndex].volume += candle.volume;
      }
    });

    // Calculate average volume
    const avgVolume = profile.reduce((sum, p) => sum + p.volume, 0) / bins;

    // Identify HVN and LVN
    return profile
      .map(p => ({
        ...p,
        type: p.volume > avgVolume * 1.5 ? 'HVN' as const : p.volume < avgVolume * 0.5 ? 'LVN' as const : null,
      }))
      .filter(p => p.type !== null)
      .map(p => ({ priceLevel: p.priceLevel, volume: p.volume, type: p.type! }));
  }
}
