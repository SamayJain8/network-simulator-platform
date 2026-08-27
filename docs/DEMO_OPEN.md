# Network Simulator Demo Guide

This guide shows how to run the complete Network Simulator Platform locally and verify that the C++ simulation core, FastAPI telemetry service, and React dashboard are connected end to end.

## What This Demo Shows

- The C++ core runs as a native simulation service.
- The core publishes simulated network telemetry over HTTP.
- The FastAPI service ingests telemetry and predicts link anomalies.
- The React dashboard polls the API and visualizes live network health.
- Docker Compose starts the full multi-service system with one command.

## Recommended Demo: Docker Compose

### 1. Start Docker Desktop

Open Docker Desktop and wait until the Docker engine is running.

### 2. Start The Full Stack

From the project root:

```bash
cd infra
docker compose up --build
```

Expected services:

- Dashboard: `http://localhost:3000`
- Telemetry API: `http://localhost:8000`
- C++ core daemon: `localhost:9090`

The C++ service publishes demo telemetry to the API through Docker's internal service network.

### 3. Open The Dashboard

Open:

```text
http://localhost:3000
```

The dashboard should move from an idle state into active link-health states after the stack starts. During the demo, point out:

- active router-to-router links
- healthy telemetry states
- predicted congestion/anomaly states
- alert history generated from API analysis

### 4. Verify The API

In another terminal:

```bash
curl http://localhost:8000/health
curl http://localhost:8000/telemetry/analysis
```

The health endpoint confirms the API is running. The analysis endpoint returns the latest per-link telemetry predictions used by the dashboard.

## Optional: C++ Validation Suite

On Linux, macOS, WSL, or a compatible container environment:

```bash
cmake -S core-engine -B core-engine/build-linux -DCMAKE_BUILD_TYPE=Release
cmake --build core-engine/build-linux
./core-engine/build-linux/network_test
```

The validation binary checks core simulator behavior including packet serialization, ARP cache eviction, fragmented HTTP stream parsing, subnet logic, route computation, failover handling, telemetry buffering, rate limiting, and memory arena behavior.

## Optional: Telemetry Queue Benchmark

```bash
g++ -std=c++17 -O2 -I core-engine/include tools/portfolio/telemetry_queue_benchmark.cpp -o tools/portfolio/telemetry_queue_benchmark_linux
./tools/portfolio/telemetry_queue_benchmark_linux
```

Observed local WSL result:

```text
Events processed: 1000000
Elapsed seconds: 0.049765
Throughput: 20094385 events/sec
Queue capacity: 1024 MetricEvent entries
```

This benchmark measures the lock-free single-producer/single-consumer telemetry queue used in the simulator's metric path.

## Windows Note

The dashboard and Docker workflow run on Windows. The C++ core itself uses POSIX networking APIs and native event-loop backends: `epoll` on Linux and `kqueue` on macOS/BSD. On Windows, run the core through Docker or Ubuntu WSL.

## Short Demo Pitch

This is a distributed network simulator platform. The C++ core models packet flow, routing, rate limiting, failover, and telemetry generation. The FastAPI service ingests telemetry and predicts anomalous links. The React dashboard visualizes live topology health and alert states. Docker Compose runs the system as separate services.
