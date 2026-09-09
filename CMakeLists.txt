cmake_minimum_required(VERSION 3.20)

project(gba_emulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(gba
    src/main.cpp
    src/emulator.cpp
    src/cpu.cpp
    src/memory.cpp
    src/cartridge.cpp
    src/ppu.cpp
    src/timers.cpp
)

target_include_directories(gba PRIVATE include)

if (MSVC)
    target_compile_options(gba PRIVATE /W4)
else()
    target_compile_options(gba PRIVATE -Wall -Wextra -Wpedantic)
endif()
