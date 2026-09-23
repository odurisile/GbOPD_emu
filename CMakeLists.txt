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

include(CTest)
if (BUILD_TESTING)
    add_executable(cpu_tests tests/cpu_tests.cpp src/cpu.cpp src/memory.cpp src/cartridge.cpp)
    target_include_directories(cpu_tests PRIVATE include)
    if (MSVC)
        target_compile_options(cpu_tests PRIVATE /W4)
    else()
        target_compile_options(cpu_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME cpu_instructions COMMAND cpu_tests)
    add_test(NAME cli_smoke COMMAND ${CMAKE_COMMAND}
        "-DGBA_EXECUTABLE=$<TARGET_FILE:gba>"
        "-DTEST_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/cli_test_data"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/cli_tests.cmake")
endif()
