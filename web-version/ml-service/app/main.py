from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
import numpy as np
from typing import List, Optional

app = FastAPI(
    title="CryptoTrader ML Service",
    description="Machine Learning service for cryptocurrency trading signals",
    version="1.0.0"
)

# CORS middleware
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # In production, specify actual origins
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Models
class FeatureInput(BaseModel):
    candles: List[List[float]]  # [timestamp, open, high, low, close, volume]
    indicators: Optional[dict] = None

class PredictionOutput(BaseModel):
    signal: str  # BUY, SELL, HOLD
    confidence: float
    model_scores: dict

# Health check
@app.get("/health")
async def health_check():
    return {
        "status": "healthy",
        "service": "ml-service",
        "version": "1.0.0"
    }

# Feature extraction endpoint
@app.post("/features/extract")
async def extract_features(input_data: FeatureInput):
    """
    Extract features from candle data for ML models
    Based on C++ FeatureExtractor logic
    """
    try:
        candles = np.array(input_data.candles)

        if len(candles) < 50:
            raise HTTPException(status_code=400, detail="Insufficient candle data (need at least 50)")

        # Extract basic features
        closes = candles[:, 4]
        highs = candles[:, 2]
        lows = candles[:, 3]
        volumes = candles[:, 5]

        # Price-based features
        returns = np.diff(closes) / closes[:-1]
        volatility = np.std(returns[-20:]) if len(returns) >= 20 else 0

        # Volume features
        volume_ma = np.mean(volumes[-20:]) if len(volumes) >= 20 else 0
        volume_std = np.std(volumes[-20:]) if len(volumes) >= 20 else 0

        features = {
            "price_return": float(returns[-1]) if len(returns) > 0 else 0,
            "volatility": float(volatility),
            "volume_ma": float(volume_ma),
            "volume_std": float(volume_std),
            "high_low_range": float(highs[-1] - lows[-1]) if len(highs) > 0 else 0,
        }

        return {
            "success": True,
            "features": features,
            "feature_count": len(features)
        }
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

# Prediction endpoint
@app.post("/predict", response_model=PredictionOutput)
async def predict_signal(input_data: FeatureInput):
    """
    Generate trading signal using ML ensemble
    Combines LSTM and XGBoost models (stub implementation)
    """
    try:
        # Extract features first
        feature_response = await extract_features(input_data)
        features = feature_response["features"]

        # Stub ML predictions (to be replaced with actual models)
        # In production, load trained LSTM and XGBoost models
        lstm_score = 0.6  # Placeholder
        xgboost_score = 0.65  # Placeholder

        # Ensemble prediction with weights
        lstm_weight = 0.6
        xgboost_weight = 0.4
        ensemble_score = (lstm_score * lstm_weight + xgboost_score * xgboost_weight)

        # Determine signal based on score
        if ensemble_score > 0.65:
            signal = "BUY"
        elif ensemble_score < 0.35:
            signal = "SELL"
        else:
            signal = "HOLD"

        return PredictionOutput(
            signal=signal,
            confidence=ensemble_score,
            model_scores={
                "lstm": lstm_score,
                "xgboost": xgboost_score,
                "ensemble": ensemble_score
            }
        )
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

# Model training endpoint (stub)
@app.post("/train")
async def train_model(training_config: dict):
    """
    Trigger model retraining
    """
    return {
        "success": True,
        "message": "Model training scheduled",
        "config": training_config
    }

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
