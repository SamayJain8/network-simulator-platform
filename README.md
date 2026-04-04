#  Distributed Network Simulator Platform

An observable, multi-service networked system simulator built from scratch. This project incrementally implements core OSI layer concepts (from raw TCP sockets to multi-hop routing) as a modular C++ engine, integrated with a Python-based predictive AI and a real-time React dashboard.

## System Architecture

The platform follows a modern microservices architecture, orchestrated via Docker.

1. **Core Engine (C++20):** A multi-threaded, highly optimized network simulator that handles raw socket I/O, packet encapsulation, and multi-hop routing. Built with strict RAII principles and custom memory management.
2. **Observability Bridge (C++):** An embedded HTTP server within the core engine that acts as a central event bus, streaming live metrics (latency, throughput, queue congestion).
3. **Intelligence Layer (Python / FastAPI):** An AI microservice that ingests telemetry data to predict node failures and optimize routing paths dynamically.
4. **Visualization (React):** A frontend dashboard displaying real-time network topologies and simulated traffic flows.

## Incremental Roadmap (Core Engine)

The core simulator is built phase-by-phase, mapping to fundamental Computer Science and Networking concepts:

* **Phase 1: Communication Core** - RAII socket abstractions, TCP client/server, and robust error handling.
* **Phase 2: Layered Encapsulation** - Message framing and packet layered architecture.
* **Phase 3: Link Layer** - Node IP/MAC identity and ARP cache simulation.
* **Phase 4: Application Protocols** - Custom HTTP-like request/response handling.
* **Phase 5: Addressing & Subnets** - IP assignment, subnet isolation, and DNS mapping.
* **Phase 6: Routing (Applied DSA)** - Routing tables, queue management, and shortest-path multi-hop forwarding.
* **Phase 7: Observability** - Multi-threaded background monitoring, event queues, and incident correlation.
* **Phase 8: Distributed Systems** - Load balancing and failover simulation.

## Tech Stack

* **Systems / Core:** C++20, POSIX Sockets, Pthreads, CMake
* **Intelligence:** Python 3.11, FastAPI
* **Frontend:** React, Tailwind CSS
* **Infrastructure:** Docker, Docker Compose, Git (Conventional Commits)

##  Engineering Principles

* **Zero-Cost Abstractions:** System components are decoupled but highly optimized for performance.
* **Observability First:** Monitoring is built into the transport engine, not bolted on as an afterthought.
* **Memory Safety:** Strict adherence to modern C++ paradigms (smart pointers, RAII) to prevent file descriptor and memory leaks.
