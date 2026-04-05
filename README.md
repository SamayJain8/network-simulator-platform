# Network Simulator Platform

An observable, multi-service networked system simulator built from scratch. This project incrementally implements core OSI layer concepts (from raw bare-metal TCP sockets to multi-hop routing) as a modular C++ engine, integrated with a Python-based intelligence service and a real-time React dashboard.

## System Architecture

The platform follows a modern microservices architecture, orchestrated via Docker.

1. **Core Engine (C++20):** A multi-threaded, highly optimized network simulator that handles raw socket I/O, packet encapsulation, and multi-hop routing. Built with strict RAII principles and custom memory management.
2. **Observability Bridge (C++):** An embedded HTTP server within the core engine that acts as a central event bus, streaming live metrics (latency, throughput, queue congestion).
3. **Intelligence Layer (Python / FastAPI):** A backend microservice that ingests telemetry data to predict node failures and optimize routing paths dynamically.
4. **Visualization (React):** A frontend dashboard displaying real-time network topologies and simulated traffic flows.

## Incremental Roadmap

This project is divided into Two Major Acts: the high-performance C++ Engine, and the Python/React Microservices.

### ACT 1: The Core Engine (C++20)
The core simulator is built phase-by-phase, prioritizing memory efficiency and zero-cost abstractions:

* [x] **Phase 1: Communication Core** - RAII POSIX socket abstractions, TCP client/server, and robust OS-level error handling (patched `SO_REUSEADDR` to prevent OS zombie sockets).
* [ ] **Phase 2: Layered Encapsulation & Serialization** - Implementing high-performance **Binary Serialization** using tightly packed C++ `structs` and explicit memory offsets for zero-overhead data transfer.
* [ ] **Phase 3: Link Layer** - Node IP/MAC identity and ARP cache simulation.
* [ ] **Phase 4: Application Protocols** - Custom HTTP-like request/response handling.
* [ ] **Phase 5: Addressing & Subnets** - IP assignment, subnet isolation, and DNS mapping.
* [ ] **Phase 6: Routing (Applied DSA)** - Routing tables, priority queue management, and shortest-path multi-hop forwarding using graph traversal algorithms.
* [ ] **Phase 7: Observability Bridge** - Implementing C++ multi-threading with lock-free circular queues (`std::atomic`). This thread drains high-speed binary traffic, converts summaries to JSON, and streams them out without blocking the main engine.
* [ ] **Phase 8: Distributed Systems** - Load balancing and failover simulation.

### ACT 2: The Intelligence & Visualization Layers
* [ ] **Step 9: Intelligence Layer (Python/FastAPI)** - Ingests telemetry to predict node failures, using discrete-event clock synchronization to maintain parity with the C++ engine.
* [ ] **Step 10: Visualization Layer (React)** - Frontend dashboard to fetch live topologies and plot routing paths dynamically.
* [ ] **Step 11: Infrastructure (Docker)** - Docker Compose orchestration to spin up the monorepo as a unified, isolated cluster.

## Tech Stack

* **Systems / Core:** C++20, POSIX Sockets, Pthreads, CMake, Binary Serialization
* **Intelligence:** Python 3.11, FastAPI
* **Frontend:** React, Tailwind CSS
* **Infrastructure:** Docker, Docker Compose, Git

## Production-Ready Monorepo Structure

This project utilizes a monorepo approach to isolate microservices while maintaining unified version control.

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





