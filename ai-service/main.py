from fastapi import FastAPI, HTTPException, status
from fastapi.middleware.cors import CORSMiddleware
from typing import Dict, List
import logging

from models.schemas import TelemetryData, AnomalyPrediction
from models.predictor import TelemetryPredictor

# Setup structured logging
logging.basicConfig(level=logging.INFO, format="%(asctime)s [%(levelname)s] %(message)s")
logger = logging.getLogger("TelemetryAI")

app = FastAPI(
    title="Full-Stack Observable Network Simulator AI Service",
    description="Asynchronous telemetry processing & linear trend bottleneck forecasting microservice.",
    version="2.0.0"
)

# Enable CORS for dashboard integration
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# In-memory telemetry cache
telemetry_db: Dict[str, List[TelemetryData]] = {}
analysis_cache: Dict[str, AnomalyPrediction] = {}

# Initialize our predictive trend engine
predictor = TelemetryPredictor()

@app.get("/health", status_code=status.HTTP_200_OK)
async def health_check():
    return {"status": "healthy", "service": "ai-telemetry-service"}

@app.post("/telemetry/stream", response_model=AnomalyPrediction, status_code=status.HTTP_201_CREATED)
async def stream_telemetry(data: TelemetryData):
    """
    Receives real-time metric streams from the C++ core engine.
    Applies the predictive Holt's trend model to calculate congestion probabilities.
    """
    node_pair = f"{data.source_node}->{data.dest_node}"
    
    if node_pair not in telemetry_db:
        telemetry_db[node_pair] = []
    
    # Store the metric event
    telemetry_db[node_pair].append(data)
    if len(telemetry_db[node_pair]) > 100:
        telemetry_db[node_pair].pop(0)

    # Execute Holt's linear trend forecasting analysis
    try:
        prediction = predictor.analyze(data)
        analysis_cache[node_pair] = prediction
        logger.info(
            "Telemetry ingested link=%s latency_ns=%.2f dropped=%s throughput_kbps=%.2f anomaly=%s",
            node_pair,
            data.latency_ns,
            data.dropped_packets,
            data.throughput_kbps,
            prediction.is_anomaly,
        )
        return prediction
    except Exception as e:
        logger.error(f"Prediction engine failed for key {node_pair}: {str(e)}")
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail="Predictive calculation failure"
        )

@app.get("/telemetry/analysis", response_model=Dict[str, AnomalyPrediction])
async def get_latest_analysis():
    """
    Exposes latest predictive evaluations for frontend dashboard polling.
    """
    return analysis_cache
