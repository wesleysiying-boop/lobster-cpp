cmake_minimum_required(VERSION 3.20)

project(lobster
    VERSION 0.1.0
    DESCRIPTION "Sub-microsecond limit order book matching engine in C++20"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

option(LOBSTER_BUILD_TESTS "Build unit tests" ON)
option(LOBSTER_BUILD_BENCHMARKS "Build benchmarks" ON)
option(LOBSTER_BUILD_EXAMPLES "Build examples" ON)
option(LOBSTER_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)

# Header-only interface library — the matching engine itself is templated /
# inline, so consumers get the full implementation by including the headers.
add_library(lobster INTERFACE)
add_library(lobster::lobster ALIAS lobster)

target_include_directories(lobster INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(lobster INTERFACE cxx_std_20)

# Strict warnings — anything new shows up early.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    set(_lobster_warn -Wall -Wextra -Wpedantic -Wshadow -Wconversion
        -Wnon-virtual-dtor -Wold-style-cast -Wnull-dereference
        -Wdouble-promotion -Wformat=2)
    if(LOBSTER_WARNINGS_AS_ERRORS)
        list(APPEND _lobster_warn -Werror)
    endif()
    target_compile_options(lobster INTERFACE ${_lobster_warn})
endif()

if(LOBSTER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(LOBSTER_BUILD_BENCHMARKS)
    add_subdirectory(bench)
endif()

if(LOBSTER_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
