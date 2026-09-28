cmake_minimum_required(VERSION 3.15)
project(nes6502 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(nes6502
    src/Bus.cpp
    src/Cpu6502.cpp
    src/AddressingModes.cpp
    src/Opcodes.cpp
    src/main.cpp
)

target_include_directories(nes6502 PRIVATE include)
