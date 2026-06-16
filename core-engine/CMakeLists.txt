cmake_minimum_required(VERSION 3.20)
project(NetworkSimulatorCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
find_package(Threads REQUIRED)

# 1. LIBRARY: All source files compiled into core_lib static binary
add_library(core_lib STATIC
    src/core/connection.cpp
    src/core/tcp_server.cpp
    src/core/packet.cpp
    src/core/message.cpp
    src/network/arp.cpp
    src/protocols/http_sim.cpp
    src/network/addressing.cpp
    src/network/node.cpp
    src/network/router.cpp
    src/system/monitor.cpp
    src/network/circuit_breaker.cpp
    src/system/event_loop.cpp
)

# 2. EXECUTABLES: Separate the automated test suite from the production server daemon!
# This keeps the test runner isolated and compiles the daemon to its own binary.
add_executable(network_test src/main/controller.cpp)
add_executable(network_core src/main/core_engine.cpp) # Compiles core_engine to network_core!

# 3. LINKS: Link targets with static core library and Threads
target_link_libraries(network_test PRIVATE core_lib Threads::Threads)
target_link_libraries(network_core PRIVATE core_lib Threads::Threads)