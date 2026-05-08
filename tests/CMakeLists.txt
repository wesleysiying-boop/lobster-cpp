include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Catch2)

add_executable(lobster_tests
    test_arena.cpp
    test_price_level.cpp
    test_book.cpp
    test_engine.cpp
    test_spsc_queue.cpp
)
target_link_libraries(lobster_tests PRIVATE lobster::lobster Catch2::Catch2WithMain)

# Catch2's own headers trip our strict warnings on some toolchains.
target_compile_options(lobster_tests PRIVATE -Wno-error)

include(CTest)
include(Catch)
catch_discover_tests(lobster_tests)
