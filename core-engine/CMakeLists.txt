cmake_minimum_required(VERSION 3.20)
project(NetworkSimulatorCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
find_package(Threads REQUIRED)

# 1. LIBRARY: All source files must be listed here.
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
    src/system/event_loop.cpp     # Added for Option B Event Loop
)

# 2. EXECUTABLE: Driver entrypoint.
add_executable(network_core src/main/controller.cpp)

# 3. LINK: Connect the executable to the library.
target_link_libraries(network_core PRIVATE core_lib Threads::Threads)