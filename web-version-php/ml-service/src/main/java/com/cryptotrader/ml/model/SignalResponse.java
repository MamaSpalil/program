package com.cryptotrader.ml.model;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;
import lombok.NoArgsConstructor;

/**
 * Signal Response Model
 *
 * Response model for ML signal generation
 */
@Data
@Builder
@NoArgsConstructor
@AllArgsConstructor
public class SignalResponse {

    /**
     * Trading signal: BUY, SELL, or HOLD
     */
    private String signal;

    /**
     * Confidence level (0.0 to 1.0)
     */
    private double confidence;

    /**
     * LSTM model prediction
     */
    private double lstmPrediction;

    /**
     * XGBoost model prediction
     */
    private double xgboostPrediction;

    /**
     * Ensemble prediction (combined)
     */
    private double ensemblePrediction;

    /**
     * Timestamp of signal generation
     */
    private long timestamp;

    /**
     * Additional metadata
     */
    private Object metadata;
}
