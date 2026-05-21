cmake_minimum_required(VERSION 3.20)
project(NetworkSimulatorCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
find_package(Threads REQUIRED)

# 1. LIBRARY: The code 'owner'. 
# All logic files (.cpp) go here, and ONLY here.
add_library(core_lib STATIC
    src/core/connection.cpp
    src/core/tcp_server.cpp
    src/core/packet.cpp
    src/network/arp.cpp
)

# 2. EXECUTABLE: The 'consumer'.
# ONLY main.cpp goes here. 
add_executable(network_core src/main/controller.cpp)

# 3. LINK: Connect the executable to the library.
target_link_libraries(network_core PRIVATE core_lib Threads::Threads)