# Distributed Network Simulator Platform

A custom, multi-service network simulator built from scratch to model OSI layer interactions, multi-hop routing, and network fault tolerance. 

The project is structured as a monorepo containing a bare-metal C++20 transport engine, integrated with a Python/FastAPI telemetry service and a React visualization dashboard.

## System Architecture

The platform utilizes a microservices architecture orchestrated via Docker Compose:

1. **Core Engine (C++20):** A multi-threaded network simulator handling raw POSIX socket I/O and packet encapsulation. Built to avoid unnecessary memory allocations by utilizing **zero-copy serialization** (`std::span` and explicit memory offsets).
2. **Telemetry Bus (C++):** A background monitoring thread that reads from a cache-aligned **lock-free circular buffer** (`std::atomic`). This allows the engine to stream latency and throughput metrics without introducing mutex bottlenecks to the main packet-processing thread.
3. **Intelligence Layer (Python / FastAPI):** An asynchronous service that ingests the C++ metrics stream. It implements a **Circuit Breaker pattern** to dynamically recalculate routing paths and prevent retry-storms when simulated nodes fail. It utilizes a **Holt's Linear Trend model** for predictive congestion and bottleneck forecasting.
4. **Visualization (React):** A frontend dashboard displaying real-time network topologies and simulated traffic flows using immediate-mode **HTML5 Canvas rendering** to bypass Virtual DOM rendering limits.

## Engineering Principles

* **Zero-Cost Abstractions:** System components are decoupled but highly optimized for performance, utilizing compile-time evaluations to eliminate runtime overhead.
* **Observability First:** Monitoring and telemetry collection are built directly into the transport engine's core loops, not bolted on as an afterthought.
* **Hardware-Aware Memory Safety & Cache Alignment:** Optimized specifically for ARM Unified Memory architectures (Apple Silicon M2) using zero-copy serialization and explicit memory alignment (`alignas(64)`) to prevent false sharing and struct padding corruption.
* **Lock-Free Concurrency:** Thread safety is guaranteed against ARM's weak memory ordering using atomic operations, Compare-And-Swap (CAS) loops, and acquire-release barriers, avoiding standard slow mutexes.

## Incremental Roadmap

Every phase of development, including core transport layers and microservices orchestration, has been completed and verified:

### ACT 1: The Core Engine (C++20)
* [x] **Phase 1: Communication Core** - RAII POSIX socket abstractions, TCP client/server, and robust zombie-socket prevention (`SO_REUSEADDR`).
* [x] **Phase 2: Layered Encapsulation** - Zero-copy binary packet serialization, network byte order conversions, and memory alignment.
* [x] **Phase 3: Link Layer** - Address Resolution Protocol (ARP) implementation with an in-memory, $O(1)$ LRU cache eviction policy.
* [x] **Phase 4: Application Protocols** - Deterministic, fragmented HTTP text parser and logical message serialization schemas.
* [x] **Phase 5: Network Layer** - Bitwise subnet masking, CIDR notation validation, and local network boundary checks.
* [x] **Phase 6: Multi-Hop Routing** - Link-State routing tables and Longest Prefix Match (LPM) route lookup driven by Dijkstra's shortest-path algorithm.
* [x] **Phase 7: Observability Bridge** - Lock-free, cache-aligned SPSC telemetry queue and background aggregation thread.
* [x] **Phase 8: Fault Tolerance** - Link-state Circuit Breaker integration and Exponential Backoff with Full Jitter delay calculations.
* [x] **Concurrency Upgrade:** Lock-free, atomic Generic Cell Rate Algorithm (GCRA) rate limiter integration in the packet path.

### ACT 2: Intelligence & Visualization
* [x] **Phase 9: AI Telemetry Analysis** - Async FastAPI service with Pydantic v2 validation and real-time Holt's Linear Trend forecasting.
* [x] **Phase 10: Real-Time Dashboard** - React UI with immediate-mode HTML5 Canvas rendering for high-frequency topology visualization.
* [x] **Phase 11: Orchestration** - Containerization of all tiers using multi-stage builds and isolated network namespaces via Docker Compose.

## Monorepo Structure

```text
network-simulator-platform/
├── README.md
├── infra/                     # Container orchestration configurations
│   ├── docker-compose.yml
│   ├── core.Dockerfile        # Multi-stage release C++ build
│   ├── ai.Dockerfile          # Optimized Python slim runtime
│   └── dashboard.Dockerfile   # Static assets served via Nginx alpine
├── dashboard/                 # React UI Dashboard
│   ├── package.json
│   ├── src/
│   └── public/
├── ai-service/                # Python / FastAPI Predictive Service
│   ├── requirements.txt
│   ├── main.py
│   └── models/
└── core-engine/               # C++20 Network Emulator Core
    ├── CMakeLists.txt
    ├── include/
    │   ├── core/
    │   │   ├── connection.hpp # Socket abstraction
    │   │   ├── message.hpp    # Logical message
    │   │   └── packet.hpp     # Binary layered packet
    │   ├── network/
    │   │   ├── node.hpp       # IP/MAC identity & rate limiting
    │   │   ├── arp.hpp        # LRU ARP cache
    │   │   ├── addressing.hpp # Subnet mathematics
    │   │   └── router.hpp     # Dijkstra & LPM routing
    │   │   └── circuit_breaker.hpp # Link state monitors
    │   ├── protocols/
    │   │   ├── protocol.hpp   # Base protocol interface
    │   │   └── http_sim.hpp   # HTTP FSM parser
    │   └── system/
    │       ├── metrics.hpp    # Telemetry schema
    │       ├── monitor.hpp    # Background metric aggregator
    │       └── event_queue.hpp# Cache-aligned, lock-free SPSC queue
    ├── src/
    │   ├── core/
    │   ├── network/
    │   ├── protocols/
    │   ├── system/
    │   └── main/
    │       └── controller.cpp # Simulation test driver
    └── tests/