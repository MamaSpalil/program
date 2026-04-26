package com.cryptotrader.ml.model;

import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.NoArgsConstructor;

import java.util.List;
import java.util.Map;

/**
 * Signal Request Model
 *
 * Request model for ML signal generation
 */
@Data
@NoArgsConstructor
@AllArgsConstructor
public class SignalRequest {

    /**
     * Trading symbol (e.g., BTCUSDT)
     */
    private String symbol;

    /**
     * Timeframe interval (e.g., 15m, 1h)
     */
    private String interval;

    /**
     * Candlestick data
     * Array of [timestamp, open, high, low, close, volume]
     */
    private List<double[]> candles;

    /**
     * Technical indicators
     * Map of indicator name to values
     */
    private Map<String, List<Double>> indicators;

    /**
     * Additional parameters
     */
    private Map<String, Object> parameters;
}
