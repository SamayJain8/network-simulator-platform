cmake_minimum_required(VERSION 3.20)
project(NetworkSimulatorCore VERSION 1.0.0 LANGUAGES CXX)

# Enforce Modern C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Production-grade compiler flags (Apple Clang / M2 Optimized)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -mcpu=apple-m2 -flto -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fno-omit-frame-pointer -fsanitize=address,undefined")

# Include directories mapping to our modular design
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Find threading library (Crucial for Phase 7 Observability)
find_package(Threads REQUIRED)

# Core Library Definition
add_library(core_lib src/core/connection.cpp src/core/tcp_server.cpp)

# Main Executable
add_executable(network_core src/main/controller.cpp)
target_link_libraries(network_core PRIVATE core_lib Threads::Threads)
