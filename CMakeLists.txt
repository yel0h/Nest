cmake_minimum_required(VERSION 3.15)
project(nes6502 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(nes_core STATIC
    src/Apu2A03.cpp
    src/Bus.cpp
    src/Cpu6502.cpp
    src/AddressingModes.cpp
    src/Opcodes.cpp
    src/Ppu2C02.cpp
    src/Cartridge.cpp
    src/Mapper.cpp
    src/Mapper000.cpp
    src/Mapper001.cpp
    src/Mapper002.cpp
    src/Mapper003.cpp
    src/Mapper004.cpp
    src/Mapper007.cpp
    src/Mapper009.cpp
    src/Mapper011.cpp
    src/Mapper066.cpp
    src/Mapper071.cpp
    src/Mapper079.cpp
    src/Mapper180.cpp
)
target_include_directories(nes_core PUBLIC include)

add_executable(nes6502 src/main.cpp)
target_link_libraries(nes6502 PRIVATE nes_core)

add_executable(nes_mapper_tests src/MapperTests.cpp)
target_link_libraries(nes_mapper_tests PRIVATE nes_core)

add_executable(nes_apu_tests src/ApuTests.cpp)
target_link_libraries(nes_apu_tests PRIVATE nes_core)

enable_testing()
add_test(NAME mapper_tests COMMAND nes_mapper_tests)
add_test(NAME apu_tests COMMAND nes_apu_tests)

option(NES_BUILD_DEBUGGER "Build the raylib-based visual debugger" ON)
if(NES_BUILD_DEBUGGER)
    include(FetchContent)

    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(raylib)

    add_executable(nes_debugger src/DebugApp.cpp)
    target_link_libraries(nes_debugger PRIVATE nes_core nes_audio raylib)

    add_library(nes_audio STATIC src/AudioSynth.cpp)
    target_include_directories(nes_audio PUBLIC include)
    target_link_libraries(nes_audio PUBLIC raylib)

    add_executable(nes_sound_demo src/SoundDemo.cpp)
    target_link_libraries(nes_sound_demo PRIVATE nes_audio)
endif()
