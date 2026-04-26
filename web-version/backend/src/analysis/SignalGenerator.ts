// Signal generation for entry/exit positions based on support/resistance zones

import { ChartAnalysis } from './ChartAnalysis';

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
}

interface Signal {
  type: 'BUY_ENTRY' | 'SELL_ENTRY' | 'BUY_EXIT' | 'SELL_EXIT';
  price: number;
  timestamp: Date;
  reason: string;
  confidence: number;
  stopLoss: number;
  takeProfit: number;
  zoneId?: string;
}

export class SignalGenerator {
  /**
   * Generate buy/sell signals based on support/resistance zones
   */
  static generateSignals(candles: Candle[], zones: Zone[]): Signal[] {
    if (candles.length < 20) {
      throw new Error('Insufficient data for signal generation');
    }

    const signals: Signal[] = [];
    const currentPrice = candles[candles.length - 1].close;
    const currentTimestamp = new Date(candles[candles.length - 1].timestamp);

    // Find nearest support and resistance zones
    const nearbySupports = zones
      .filter(z => z.type === 'SUPPORT' && z.priceLevel < currentPrice)
      .sort((a, b) => b.priceLevel - a.priceLevel);

    const nearbyResistances = zones
      .filter(z => z.type === 'RESISTANCE' && z.priceLevel > currentPrice)
      .sort((a, b) => a.priceLevel - b.priceLevel);

    // Generate buy signals near support
    if (nearbySupports.length > 0) {
      const strongestSupport = nearbySupports[0];
      const distanceToSupport = Math.abs(currentPrice - strongestSupport.priceLevel) / currentPrice;

      // Signal when price is within 1% of strong support
      if (distanceToSupport < 0.01 && strongestSupport.strength > 0.6) {
        const stopLoss = strongestSupport.priceLevel * 0.985; // 1.5% below support
        const takeProfit = nearbyResistances.length > 0
          ? nearbyResistances[0].priceLevel * 0.995 // Just below resistance
          : currentPrice * 1.03; // 3% profit target

        signals.push({
          type: 'BUY_ENTRY',
          price: currentPrice,
          timestamp: currentTimestamp,
          reason: `Strong support zone at ${strongestSupport.priceLevel.toFixed(2)} (strength: ${(strongestSupport.strength * 100).toFixed(1)}%, touches: ${strongestSupport.touches})`,
          confidence: strongestSupport.strength,
          stopLoss,
          takeProfit,
        });
      }
    }

    // Generate sell signals near resistance
    if (nearbyResistances.length > 0) {
      const strongestResistance = nearbyResistances[0];
      const distanceToResistance = Math.abs(currentPrice - strongestResistance.priceLevel) / currentPrice;

      // Signal when price is within 1% of strong resistance
      if (distanceToResistance < 0.01 && strongestResistance.strength > 0.6) {
        const stopLoss = strongestResistance.priceLevel * 1.015; // 1.5% above resistance
        const takeProfit = nearbySupports.length > 0
          ? nearbySupports[0].priceLevel * 1.005 // Just above support
          : currentPrice * 0.97; // 3% profit target

        signals.push({
          type: 'SELL_ENTRY',
          price: currentPrice,
          timestamp: currentTimestamp,
          reason: `Strong resistance zone at ${strongestResistance.priceLevel.toFixed(2)} (strength: ${(strongestResistance.strength * 100).toFixed(1)}%, touches: ${strongestResistance.touches})`,
          confidence: strongestResistance.strength,
          stopLoss,
          takeProfit,
        });
      }
    }

    // Check for breakout signals
    const breakoutSignals = this.detectBreakouts(candles, zones);
    signals.push(...breakoutSignals);

    return signals;
  }

  /**
   * Detect breakout from support/resistance zones
   */
  private static detectBreakouts(candles: Candle[], zones: Zone[]): Signal[] {
    if (candles.length < 5) return [];

    const signals: Signal[] = [];
    const currentPrice = candles[candles.length - 1].close;
    const previousPrice = candles[candles.length - 2].close;
    const currentTimestamp = new Date(candles[candles.length - 1].timestamp);
    const volume = candles[candles.length - 1].volume;
    const avgVolume = candles.slice(-20).reduce((sum, c) => sum + c.volume, 0) / 20;

    // High volume breakouts are more reliable
    const volumeConfirmation = volume > avgVolume * 1.5;

    zones.forEach(zone => {
      // Resistance breakout (bullish)
      if (zone.type === 'RESISTANCE') {
        const priceAboveResistance = currentPrice > zone.priceTop;
        const previousPriceBelowResistance = previousPrice < zone.priceTop;

        if (priceAboveResistance && previousPriceBelowResistance) {
          const confidence = zone.strength * (volumeConfirmation ? 1.2 : 0.8);
          const stopLoss = zone.priceLevel; // Use broken resistance as support
          const takeProfit = currentPrice + (currentPrice - zone.priceLevel); // Risk:reward 1:1

          signals.push({
            type: 'BUY_ENTRY',
            price: currentPrice,
            timestamp: currentTimestamp,
            reason: `Resistance breakout at ${zone.priceLevel.toFixed(2)} ${volumeConfirmation ? 'with high volume' : ''}`,
            confidence: Math.min(confidence, 1),
            stopLoss,
            takeProfit,
          });
        }
      }

      // Support breakdown (bearish)
      if (zone.type === 'SUPPORT') {
        const priceBelowSupport = currentPrice < zone.priceBottom;
        const previousPriceAboveSupport = previousPrice > zone.priceBottom;

        if (priceBelowSupport && previousPriceAboveSupport) {
          const confidence = zone.strength * (volumeConfirmation ? 1.2 : 0.8);
          const stopLoss = zone.priceLevel; // Use broken support as resistance
          const takeProfit = currentPrice - (zone.priceLevel - currentPrice); // Risk:reward 1:1

          signals.push({
            type: 'SELL_ENTRY',
            price: currentPrice,
            timestamp: currentTimestamp,
            reason: `Support breakdown at ${zone.priceLevel.toFixed(2)} ${volumeConfirmation ? 'with high volume' : ''}`,
            confidence: Math.min(confidence, 1),
            stopLoss,
            takeProfit,
          });
        }
      }
    });

    return signals;
  }

  /**
   * Generate exit signals for existing positions
   */
  static generateExitSignals(
    candles: Candle[],
    zones: Zone[],
    activePositions: Array<{ type: 'LONG' | 'SHORT'; entryPrice: number; stopLoss: number; takeProfit: number }>
  ): Signal[] {
    if (candles.length === 0 || activePositions.length === 0) {
      return [];
    }

    const signals: Signal[] = [];
    const currentPrice = candles[candles.length - 1].close;
    const currentTimestamp = new Date(candles[candles.length - 1].timestamp);

    activePositions.forEach(position => {
      // Check stop loss
      if (
        (position.type === 'LONG' && currentPrice <= position.stopLoss) ||
        (position.type === 'SHORT' && currentPrice >= position.stopLoss)
      ) {
        signals.push({
          type: position.type === 'LONG' ? 'SELL_EXIT' : 'BUY_EXIT',
          price: currentPrice,
          timestamp: currentTimestamp,
          reason: 'Stop loss triggered',
          confidence: 1.0,
          stopLoss: 0,
          takeProfit: 0,
        });
      }

      // Check take profit
      if (
        (position.type === 'LONG' && currentPrice >= position.takeProfit) ||
        (position.type === 'SHORT' && currentPrice <= position.takeProfit)
      ) {
        signals.push({
          type: position.type === 'LONG' ? 'SELL_EXIT' : 'BUY_EXIT',
          price: currentPrice,
          timestamp: currentTimestamp,
          reason: 'Take profit target reached',
          confidence: 1.0,
          stopLoss: 0,
          takeProfit: 0,
        });
      }

      // Check for reversal signals based on zones
      const reversalSignal = this.detectReversalAtZone(candles, zones, position);
      if (reversalSignal) {
        signals.push({
          ...reversalSignal,
          type: position.type === 'LONG' ? 'SELL_EXIT' : 'BUY_EXIT',
          timestamp: currentTimestamp,
        });
      }
    });

    return signals;
  }

  /**
   * Detect price reversal at zone
   */
  private static detectReversalAtZone(
    candles: Candle[],
    zones: Zone[],
    position: { type: 'LONG' | 'SHORT'; entryPrice: number }
  ): Omit<Signal, 'type' | 'timestamp'> | null {
    if (candles.length < 3) return null;

    const currentPrice = candles[candles.length - 1].close;
    const previousPrice = candles[candles.length - 2].close;

    // For LONG positions, check if price is rejecting at resistance
    if (position.type === 'LONG') {
      const nearbyResistance = zones.find(
        z => z.type === 'RESISTANCE' &&
        currentPrice >= z.priceBottom &&
        currentPrice <= z.priceTop &&
        z.strength > 0.5
      );

      if (nearbyResistance && currentPrice < previousPrice) {
        return {
          price: currentPrice,
          reason: `Price rejection at resistance zone ${nearbyResistance.priceLevel.toFixed(2)}`,
          confidence: nearbyResistance.strength,
          stopLoss: 0,
          takeProfit: 0,
        };
      }
    }

    // For SHORT positions, check if price is bouncing at support
    if (position.type === 'SHORT') {
      const nearbySupport = zones.find(
        z => z.type === 'SUPPORT' &&
        currentPrice >= z.priceBottom &&
        currentPrice <= z.priceTop &&
        z.strength > 0.5
      );

      if (nearbySupport && currentPrice > previousPrice) {
        return {
          price: currentPrice,
          reason: `Price bounce at support zone ${nearbySupport.priceLevel.toFixed(2)}`,
          confidence: nearbySupport.strength,
          stopLoss: 0,
          takeProfit: 0,
        };
      }
    }

    return null;
  }

  /**
   * Apply trailing stop logic
   */
  static updateTrailingStop(
    currentPrice: number,
    position: { type: 'LONG' | 'SHORT'; entryPrice: number; currentStopLoss: number },
    trailingPercent: number = 0.02 // 2% trailing
  ): number {
    if (position.type === 'LONG') {
      const profitableMove = currentPrice > position.entryPrice;
      if (profitableMove) {
        const newStopLoss = currentPrice * (1 - trailingPercent);
        return Math.max(newStopLoss, position.currentStopLoss);
      }
    } else {
      const profitableMove = currentPrice < position.entryPrice;
      if (profitableMove) {
        const newStopLoss = currentPrice * (1 + trailingPercent);
        return Math.min(newStopLoss, position.currentStopLoss);
      }
    }

    return position.currentStopLoss;
  }
}
