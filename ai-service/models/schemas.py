from pydantic import BaseModel, ConfigDict, Field
from typing import Optional

class TelemetryData(BaseModel):
    # Pydantic v2 configuration settings
    model_config = ConfigDict(populate_by_name=True)

    timestamp_us: int = Field(..., description="Microsecond timestamp of telemetry capture")
    source_node: str = Field(..., description="Name of the source router node")
    dest_node: str = Field(..., description="Name of the destination router node")
    packet_size: int = Field(..., description="Size of packets processed in bytes")
    latency_ns: float = Field(..., description="Average latency in nanoseconds")
    dropped_packets: int = Field(..., description="Count of dropped packets in this interval")
    throughput_kbps: float = Field(..., description="Throughput in kilobits per second")

class AnomalyPrediction(BaseModel):
    model_config = ConfigDict(populate_by_name=True)

    is_anomaly: bool = Field(..., description="True if a bottleneck is predicted")
    anomaly_probability: float = Field(..., description="Calculated probability score of incoming congestion [0.0 - 1.0]")
    predicted_bottleneck_node: Optional[str] = Field(None, description="The node predicted to bottleneck")
    alert_message: str = Field(..., description="Human-readable warning details")
    recommended_action: str = Field(..., description="Prescribed dynamic routing modification to avoid overflow")