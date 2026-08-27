# Network Simulator Portfolio Evidence

Raw dashboard screenshots captured from the local React dashboard running against a local telemetry endpoint:

- `network-simulator-dashboard-idle-desktop-raw.png` - topology dashboard with no active telemetry.
- `network-simulator-dashboard-healthy-desktop-raw.png` - active telemetry across all router links with healthy predictions.
- `network-simulator-dashboard-anomaly-desktop-raw.png` - predicted congestion on `RouterB->RouterC` with dashboard alert output.
- `network-simulator-dashboard-docker-live-raw.png` - dashboard captured while the Docker Compose stack was running with C++ telemetry flowing into FastAPI.

Suggested project-card caption:

> Built a full-stack network simulator with C++ packet-processing primitives, Dijkstra routing, GCRA rate limiting, lock-free telemetry buffering, FastAPI anomaly analysis, and a React Canvas topology dashboard. Captured live dashboard states for idle traffic, healthy links, and predicted congestion alerts.

Benchmark status:

- Added `tools/portfolio/telemetry_queue_benchmark.cpp` as a small reproducible benchmark for the lock-free SPSC telemetry queue.
- Ubuntu WSL benchmark result on this device: 1,000,000 events processed in 0.049765 seconds, or 20,094,385 events/sec.
- Queue capacity during the benchmark: 1024 `MetricEvent` entries.

Safe measurable claim already supported by the existing validation code:

> Validated GCRA rate limiter behavior with a 10 packets/sec sustained rate and burst capacity of 5: the initial burst is accepted, the next packet is dropped, and capacity refills after a 500ms wait.
