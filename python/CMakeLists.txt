include(FetchContent)

FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG        v2.13.6
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(pybind11)

pybind11_add_module(lobster_py MODULE lobster_py.cpp)
target_link_libraries(lobster_py PRIVATE lobster::lobster)
target_compile_options(lobster_py PRIVATE -Wno-error)

# scikit-build-core / pip install: drop the .so directly into the wheel root
# so `import lobster_py` works post-install.
install(TARGETS lobster_py LIBRARY DESTINATION .)
