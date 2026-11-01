# Pure C++17 boundary tests for issue #5 (U-B1..U-B4). Build standalone:
#   cmake -S tests/boundary -B build-bd -G Ninja && cmake --build build-bd && ctest --test-dir build-bd
cmake_minimum_required(VERSION 3.22)
project(MicroGuitarBoundaryTests CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
add_executable(voice_frequency_guard_tests
    test_voice_frequency_guards.cpp)
target_include_directories(voice_frequency_guard_tests PRIVATE ${REPO_ROOT})
enable_testing()
add_test(NAME voice_frequency_guard_tests COMMAND voice_frequency_guard_tests)
