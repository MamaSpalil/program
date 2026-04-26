package com.cryptotrader.ml.service;

import org.springframework.stereotype.Service;

import java.util.List;
import java.util.Map;

/**
 * Feature Extractor Service
 *
 * Extracts features from market data for ML models
 */
@Service
public class FeatureExtractor {

    /**
     * Extract features from candles and indicators
     */
    public double[][] extractFeatures(List<double[]> candles, Map<String, List<Double>> indicators) {
        if (candles == null || candles.isEmpty()) {
            throw new IllegalArgumentException("Candles data is required");
        }

        int numCandles = Math.min(candles.size(), 100); // Use last 100 candles
        int numFeatures = calculateFeatureCount(indicators);

        double[][] features = new double[numCandles][numFeatures];

        for (int i = 0; i < numCandles; i++) {
            int candleIndex = candles.size() - numCandles + i;
            double[] candle = candles.get(candleIndex);

            int featureIndex = 0;

            // Candle features: open, high, low, close, volume
            features[i][featureIndex++] = candle[1]; // open
            features[i][featureIndex++] = candle[2]; // high
            features[i][featureIndex++] = candle[3]; // low
            features[i][featureIndex++] = candle[4]; // close
            features[i][featureIndex++] = candle[5]; // volume

            // Price changes
            if (i > 0) {
                double prevClose = candles.get(candleIndex - 1)[4];
                features[i][featureIndex++] = (candle[4] - prevClose) / prevClose; // price change %
                features[i][featureIndex++] = (candle[2] - candle[3]) / candle[3]; // high-low range %
            } else {
                features[i][featureIndex++] = 0.0;
                features[i][featureIndex++] = 0.0;
            }

            // Technical indicators
            if (indicators != null) {
                // EMA features
                if (indicators.containsKey("ema_9") && indicators.get("ema_9").size() > candleIndex) {
                    features[i][featureIndex++] = indicators.get("ema_9").get(candleIndex);
                }
                if (indicators.containsKey("ema_21") && indicators.get("ema_21").size() > candleIndex) {
                    features[i][featureIndex++] = indicators.get("ema_21").get(candleIndex);
                }
                if (indicators.containsKey("ema_50") && indicators.get("ema_50").size() > candleIndex) {
                    features[i][featureIndex++] = indicators.get("ema_50").get(candleIndex);
                }

                // RSI
                if (indicators.containsKey("rsi") && indicators.get("rsi").size() > candleIndex) {
                    features[i][featureIndex++] = indicators.get("rsi").get(candleIndex) / 100.0; // normalize
                }

                // MACD
                if (indicators.containsKey("macd") && indicators.get("macd").size() > candleIndex) {
                    features[i][featureIndex++] = indicators.get("macd").get(candleIndex);
                }
                if (indicators.containsKey("macd_signal") && indicators.get("macd_signal").size() > candleIndex) {
                    features[i][featureIndex++] = indicators.get("macd_signal").get(candleIndex);
                }

                // Bollinger Bands
                if (indicators.containsKey("bb_upper") && indicators.get("bb_upper").size() > candleIndex) {
                    features[i][featureIndex++] = indicators.get("bb_upper").get(candleIndex);
                }
                if (indicators.containsKey("bb_lower") && indicators.get("bb_lower").size() > candleIndex) {
                    features[i][featureIndex++] = indicators.get("bb_lower").get(candleIndex);
                }

                // ATR
                if (indicators.containsKey("atr") && indicators.get("atr").size() > candleIndex) {
                    features[i][featureIndex++] = indicators.get("atr").get(candleIndex);
                }
            }
        }

        return normalizeFeatures(features);
    }

    /**
     * Calculate total number of features
     */
    private int calculateFeatureCount(Map<String, List<Double>> indicators) {
        int count = 7; // Base features: OHLCV + 2 derived

        if (indicators != null) {
            if (indicators.containsKey("ema_9")) count++;
            if (indicators.containsKey("ema_21")) count++;
            if (indicators.containsKey("ema_50")) count++;
            if (indicators.containsKey("rsi")) count++;
            if (indicators.containsKey("macd")) count++;
            if (indicators.containsKey("macd_signal")) count++;
            if (indicators.containsKey("bb_upper")) count++;
            if (indicators.containsKey("bb_lower")) count++;
            if (indicators.containsKey("atr")) count++;
        }

        return count;
    }

    /**
     * Normalize features using min-max normalization
     */
    private double[][] normalizeFeatures(double[][] features) {
        if (features.length == 0) return features;

        int numFeatures = features[0].length;
        double[] min = new double[numFeatures];
        double[] max = new double[numFeatures];

        // Initialize min and max
        for (int j = 0; j < numFeatures; j++) {
            min[j] = Double.MAX_VALUE;
            max[j] = Double.MIN_VALUE;
        }

        // Find min and max for each feature
        for (double[] feature : features) {
            for (int j = 0; j < numFeatures; j++) {
                if (feature[j] < min[j]) min[j] = feature[j];
                if (feature[j] > max[j]) max[j] = feature[j];
            }
        }

        // Normalize
        for (int i = 0; i < features.length; i++) {
            for (int j = 0; j < numFeatures; j++) {
                if (max[j] - min[j] > 0) {
                    features[i][j] = (features[i][j] - min[j]) / (max[j] - min[j]);
                } else {
                    features[i][j] = 0.0;
                }
            }
        }

        return features;
    }
}
