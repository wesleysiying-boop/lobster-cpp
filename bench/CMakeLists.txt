include(FetchContent)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(BENCHMARK_DOWNLOAD_DEPENDENCIES ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.9.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(benchmark)

add_executable(lobster_bench bench_engine.cpp)
target_link_libraries(lobster_bench PRIVATE lobster::lobster benchmark::benchmark)
target_compile_options(lobster_bench PRIVATE -Wno-error)
