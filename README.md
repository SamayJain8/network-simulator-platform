# Distributed Network Simulator Platform

A custom, multi-service network simulator built from scratch to model OSI layer interactions, multi-hop routing, and network fault tolerance. 

The project is structured as a monorepo containing a bare-metal C++20 transport engine, integrated with a Python/FastAPI telemetry service and a React visualization dashboard.

## System Architecture

The platform utilizes a microservices architecture orchestrated via Docker Compose:

1. **Core Engine (C++20):** A multi-threaded network simulator handling raw POSIX socket I/O and packet encapsulation. Built to avoid unnecessary memory allocations by utilizing **zero-copy serialization** (`std::span` and explicit memory offsets).
2. **Telemetry Bus (C++):** A background monitoring thread that reads from a **lock-free circular buffer** (`std::atomic`). This allows the engine to stream latency and throughput metrics without introducing mutex bottlenecks to the main packet-processing thread.
3. **Intelligence Layer (Python / FastAPI):** An asynchronous service that ingests the C++ metrics stream. It implements a **Circuit Breaker pattern** to dynamically recalculate routing paths and prevent retry-storms when simulated nodes fail.
4. **Visualization (React):** A frontend dashboard displaying real-time network topologies and simulated traffic flows.

## Engineering Principles

* **Hardware-Aware Memory Safety:** Optimized specifically for ARM Unified Memory architectures using zero-copy serialization and explicit memory offsets to prevent struct padding corruption.
* **Lock-Free Concurrency:** Thread safety is guaranteed against ARM's weak memory ordering using atomic operations and memory barriers, avoiding standard slow mutexes.
* **Observability First:** Monitoring is built directly into the transport engine via a dedicated background thread, not bolted on as an afterthought.

## Incremental Roadmap

This project is executed in two major phases: the high-performance C++ Engine, and the Python/React Microservices integration.

### ACT 1: The Core Engine (C++20)
* [x] **Phase 1: Communication Core** - RAII POSIX socket abstractions, TCP client/server, and robust zombie-socket prevention (`SO_REUSEADDR`).
* [ ] **Phase 2: Layered Encapsulation** - Zero-Copy binary serialization handling Network Byte Order (Endianness) and memory alignment.
* [ ] **Phase 3: Link Layer** - In-memory ARP cache with custom LRU eviction logic.
* [ ] **Phase 4: Application Protocols** - Custom HTTP-like text parser for GET/POST commands and CRLF reading.
* [ ] **Phase 5: Network Layer** - Subnetting bitwise logic and CIDR notation routing restrictions.
* [ ] **Phase 6: Multi-Hop Routing** - Link-State router implementation using Dijkstra's Algorithm via Min-Heaps.
* [ ] **Phase 7: Observability Bridge** - Lock-free `std::atomic` circular buffer for background metric extraction.
* [ ] **Phase 8: Fault Tolerance** - Implementation of the Circuit Breaker pattern for dynamic node failover.

### ACT 2: Intelligence & Visualization
* [ ] **Phase 9: AI Telemetry Analysis** - FastAPI service to ingest metrics and predict congestion.
* [ ] **Phase 10: Real-Time Dashboard** - React UI to plot routing topologies dynamically.
* [ ] **Phase 11: Orchestration** - Full containerization via Docker Compose.

## Monorepo Structure

```text
network-simulator-platform/
├── README.md
├── infra/                     # Container orchestration
│   ├── docker-compose.yml
│   ├── core.Dockerfile
│   ├── ai.Dockerfile
│   └── dashboard.Dockerfile
├── dashboard/                 # React Frontend
│   ├── package.json
│   ├── src/
│   └── public/
├── ai-service/                # Python / FastAPI
│   ├── requirements.txt
│   ├── main.py
│   └── models/
└── core-engine/               # C++ Network Simulator
    ├── CMakeLists.txt
    ├── include/
    │   ├── core/
    │   │   ├── connection.hpp # socket abstraction
    │   │   ├── message.hpp    # logical message
    │   │   └── packet.hpp     # binary layered packet
    │   ├── network/
    │   │   ├── node.hpp       # IP/MAC identity
    │   │   ├── arp.hpp        # ARP cache
    │   │   ├── addressing.hpp # subnet + DNS
    │   │   └── router.hpp     # routing logic
    │   ├── protocols/
    │   │   ├── protocol.hpp   # interface
    │   │   └── http_sim.hpp   # HTTP-like protocol
    │   └── system/
    │       ├── metrics.hpp    # latency, throughput
    │       ├── monitor.hpp    # background monitoring
    │       ├── event_queue.hpp# lock-free central event bus 
    │       └── logger.hpp     # structured logging
    ├── src/
    │   ├── core/
    │   ├── network/
    │   ├── protocols/
    │   ├── system/
    │   └── main/
    │       └── controller.cpp # system orchestration
    └── tests/

``` ## Engineering Principles

* **Zero-Cost Abstractions:** System components are decoupled but highly optimized for performance.
* **Observability First:** Monitoring is built into the transport engine, not bolted on as an afterthought.
* **Memory Safety:** Strict adherence to modern C++ paradigms (smart pointers, RAII) to prevent file descriptor and memory leaks.





