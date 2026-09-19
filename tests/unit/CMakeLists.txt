# FROZEN — pure C++ unit tests, no JUCE. Build standalone:
#   cmake -S tests/unit -B build-tests -G Ninja && cmake --build build-tests && ctest --test-dir build-tests
cmake_minimum_required(VERSION 3.22)
project(MicroGuitarUnitTests CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
add_executable(note_naming_tests
    test_note_naming.cpp
    test_fret_layout.cpp
    ${REPO_ROOT}/NoteNaming.cpp)
target_include_directories(note_naming_tests PRIVATE ${REPO_ROOT})
enable_testing()
add_test(NAME note_naming_tests COMMAND note_naming_tests)
