package com.cryptotrader.ml.controller;

import com.cryptotrader.ml.model.SignalRequest;
import com.cryptotrader.ml.model.SignalResponse;
import com.cryptotrader.ml.service.FeatureExtractor;
import com.cryptotrader.ml.service.LSTMService;
import com.cryptotrader.ml.service.XGBoostService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.HashMap;
import java.util.Map;

/**
 * Signal Controller
 *
 * REST API controller for ML signal generation
 */
@RestController
@RequestMapping("/api/ml")
public class SignalController {

    private final FeatureExtractor featureExtractor;
    private final LSTMService lstmService;
    private final XGBoostService xgboostService;

    @Autowired
    public SignalController(
            FeatureExtractor featureExtractor,
            LSTMService lstmService,
            XGBoostService xgboostService) {
        this.featureExtractor = featureExtractor;
        this.lstmService = lstmService;
        this.xgboostService = xgboostService;
    }

    /**
     * Generate trading signal using ML ensemble
     * POST /api/ml/signal
     */
    @PostMapping("/signal")
    public ResponseEntity<SignalResponse> generateSignal(@RequestBody SignalRequest request) {
        try {
            // Extract features from market data
            double[][] features = featureExtractor.extractFeatures(
                    request.getCandles(),
                    request.getIndicators()
            );

            // Get LSTM prediction
            double lstmPrediction = lstmService.predict(features);

            // Get XGBoost prediction
            double xgboostPrediction = xgboostService.predict(features);

            // Ensemble: weighted average
            double lstmWeight = 0.6;
            double xgboostWeight = 0.4;
            double ensemblePrediction = (lstmPrediction * lstmWeight) +
                    (xgboostPrediction * xgboostWeight);

            // Determine signal
            String signal = "HOLD";
            double confidence = Math.abs(ensemblePrediction);

            if (ensemblePrediction > 0.65) {
                signal = "BUY";
            } else if (ensemblePrediction < -0.65) {
                signal = "SELL";
            }

            // Build response
            SignalResponse response = SignalResponse.builder()
                    .signal(signal)
                    .confidence(confidence)
                    .lstmPrediction(lstmPrediction)
                    .xgboostPrediction(xgboostPrediction)
                    .ensemblePrediction(ensemblePrediction)
                    .timestamp(System.currentTimeMillis())
                    .build();

            return ResponseEntity.ok(response);

        } catch (Exception e) {
            return ResponseEntity.status(500)
                    .body(SignalResponse.builder()
                            .signal("ERROR")
                            .confidence(0.0)
                            .timestamp(System.currentTimeMillis())
                            .build());
        }
    }

    /**
     * Extract features from market data
     * POST /api/ml/features
     */
    @PostMapping("/features")
    public ResponseEntity<Map<String, Object>> extractFeatures(@RequestBody SignalRequest request) {
        try {
            double[][] features = featureExtractor.extractFeatures(
                    request.getCandles(),
                    request.getIndicators()
            );

            Map<String, Object> response = new HashMap<>();
            response.put("success", true);
            response.put("feature_count", features.length > 0 ? features[0].length : 0);
            response.put("timestamp", System.currentTimeMillis());

            return ResponseEntity.ok(response);

        } catch (Exception e) {
            Map<String, Object> error = new HashMap<>();
            error.put("success", false);
            error.put("error", e.getMessage());
            return ResponseEntity.status(500).body(error);
        }
    }

    /**
     * Health check endpoint
     * GET /api/ml/health
     */
    @GetMapping("/health")
    public ResponseEntity<Map<String, Object>> health() {
        Map<String, Object> health = new HashMap<>();
        health.put("status", "ok");
        health.put("service", "ML Service");
        health.put("version", "1.0.0");
        health.put("models", Map.of(
                "lstm", lstmService.isLoaded(),
                "xgboost", xgboostService.isLoaded()
        ));
        health.put("timestamp", System.currentTimeMillis());

        return ResponseEntity.ok(health);
    }
}
