import math
from typing import List, Dict, Tuple, Optional
from models.schemas import TelemetryData, AnomalyPrediction

class TelemetryPredictor:
    def __init__(self, window_size: int = 10, alpha: float = 0.8, beta: float = 0.5) -> None:
        self.window_size = window_size
        self.alpha = alpha  # Level smoothing factor
        self.beta = beta    # Trend smoothing factor
        
        # Maps node_pair -> list of raw values
        self.latency_history: Dict[str, List[float]] = {}
        
        # Holt's level (l) and trend (b) states: node_pair -> (l, b)
        self.holt_states: Dict[str, Tuple[float, float]] = {}

    def analyze(self, data: TelemetryData) -> AnomalyPrediction:
        key = f"{data.source_node}->{data.dest_node}"
        
        if key not in self.latency_history:
            self.latency_history[key] = []
            
        history = self.latency_history[key]
        history.append(data.latency_ns)
        
        # Restrict history to window size
        if len(history) > self.window_size:
            history.pop(0)

        # Baseline check: require at least 3 points to initialize trend calculations
        if len(history) < 3:
            return AnomalyPrediction(
                is_anomaly=False,
                anomaly_probability=0.0,
                predicted_bottleneck_node=None,
                alert_message="Collecting initial telemetry baseline...",
                recommended_action="Maintain current OSPF routing metrics."
            )

        # Holt's Linear Trend State Update
        y_t = data.latency_ns
        if key not in self.holt_states:
            # Initialize level and trend based on initial readings
            l_prev = history[0]
            b_prev = history[1] - history[0]
        else:
            l_prev, b_prev = self.holt_states[key]

        # Holt's Equations:
        # Level: l_t = alpha * y_t + (1 - alpha) * (l_prev + b_prev)
        # Trend: b_t = beta * (l_t - l_prev) + (1 - beta) * b_prev
        l_t = self.alpha * y_t + (1.0 - self.alpha) * (l_prev + b_prev)
        b_t = self.beta * (l_t - l_prev) + (1.0 - self.beta) * b_prev
        self.holt_states[key] = (l_t, b_t)

        # Forecast forward m intervals (e.g., predict latency 3 intervals ahead)
        m = 3
        forecast_latency = l_t + m * b_t

        # Calculate standard deviation to check for statistical deviations
        mean_latency = sum(history) / len(history)
        variance = sum((x - mean_latency) ** 2 for x in history) / len(history)
        std_dev = math.sqrt(variance) if variance > 0.0 else 1.0

        # Calculate anomaly probability based on:
        # 1. Projected latency spikes
        # 2. Base latency limits (e.g., latency exceeding 300ns indicates high load)
        # 3. Active packet drops
        z_score_forecast = (forecast_latency - mean_latency) / std_dev if std_dev > 0 else 0.0
        
        anomaly_prob = 0.0
        if z_score_forecast > 2.0:
            anomaly_prob += 0.4
        if forecast_latency > 300.0:
            anomaly_prob += 0.3
        if data.dropped_packets > 0:
            anomaly_prob += 0.3
            
        anomaly_prob = min(max(anomaly_prob, 0.0), 1.0)
        is_anomaly = anomaly_prob >= 0.5

        # Prescribe dynamic routing adjustments
        if is_anomaly:
            alert_msg = f"Predicted latency anomaly ({forecast_latency:.1f} ns) on link {key} within the next {m} intervals."
            rec_action = f"Congestion imminent. Set Link Cost for {key} to 999999 to trigger dynamic Dijkstra routing bypass."
            bottleneck_node = data.dest_node
        else:
            alert_msg = "Link stability within normal operating parameters."
            rec_action = "Maintain current routing table configuration."
            bottleneck_node = None

        return AnomalyPrediction(
            is_anomaly=is_anomaly,
            anomaly_probability=anomaly_prob,
            predicted_bottleneck_node=bottleneck_node,
            alert_message=alert_msg,
            recommended_action=rec_action
        )