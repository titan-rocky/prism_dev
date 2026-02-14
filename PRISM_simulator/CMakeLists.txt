cmake_minimum_required(VERSION 3.15)
project(PRISM_Simulator CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(scada-sim
    core/cli.cpp
    
    proto/modbus/server.cpp
    proto/modbus/client.cpp

    proto/dnp3/server.cpp
    proto/dnp3/client.cpp

    proto/iec104/server.cpp
    proto/iec104/client.cpp
)


target_include_directories(scada-sim PRIVATE
    core
    proto/modbus
    proto/dnp3
)

