cmake_minimum_required(VERSION 3.16)
project(PRISM LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(prism
    src/main.cpp

    src/capture/pcap.cpp
    src/parser/modbus_parser.cpp
    src/window/window_manager.cpp
    src/feature/feat_miner.cpp

    src/detection/hst.cpp
    src/detection/burst.cpp

    src/risk/risk_model.cpp
    src/enforcement/enforcer.cpp

    # src/utils/logger.cpp
)

find_package(Threads REQUIRED)
target_link_libraries(prism PRIVATE Threads::Threads pcap)

target_include_directories(prism PRIVATE include src)
