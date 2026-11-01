# Pure C++17 tests for TuningEngine (no JUCE). Build standalone:
#   cmake -S tests/tuning_engine -B build-te -G Ninja && cmake --build build-te && ctest --test-dir build-te
cmake_minimum_required(VERSION 3.22)
project(TuningEngineTests CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
add_executable(tuning_engine_tests
    test_tuning_engine.cpp
    ${REPO_ROOT}/TuningEngine.cpp)
target_include_directories(tuning_engine_tests PRIVATE ${REPO_ROOT})
if (UNIX AND NOT APPLE)
    target_link_libraries(tuning_engine_tests PRIVATE pthread)
endif()
enable_testing()
add_test(NAME tuning_engine_tests COMMAND tuning_engine_tests)
