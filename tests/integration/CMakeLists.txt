# JUCE integration tests (frozen I1-I7, I13-I16 + remediation I-B1, I-A4).
# Standalone build:
#   cmake -S tests/integration -B build-it -G Ninja -DCMAKE_BUILD_TYPE=Release \
#     [-DFETCHCONTENT_SOURCE_DIR_JUCE=/path/to/JUCE]
#   ctest --test-dir build-it --output-on-failure
cmake_minimum_required(VERSION 3.22)
project(MicroGuitarIntegrationTests C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)

include(FetchContent)
set(MICROGUITAR_JUCE_TAG "9.0.4" CACHE STRING
    "JUCE git tag fetched by FetchContent (must exist in juce-framework/JUCE)")
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        ${MICROGUITAR_JUCE_TAG}
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_console_app(MicroGuitarIntegrationTests VERSION 1.0.0 PRODUCT_NAME "MicroGuitarIntegrationTests")

target_sources(MicroGuitarIntegrationTests
    PRIVATE
        TuningParameterTests.cpp
        InstrumentTests.cpp
        VoiceGuardAndPitchTests.cpp
        RunIntegrationTests.cpp
        ${REPO_ROOT}/PluginProcessor.cpp
        ${REPO_ROOT}/PluginProcessor.h
        ${REPO_ROOT}/TuningEngine.cpp
        ${REPO_ROOT}/TuningEngine.h
        ${REPO_ROOT}/TuningStructures.h
        ${REPO_ROOT}/MicrotonalVoice.cpp
        ${REPO_ROOT}/MicrotonalVoice.h
        ${REPO_ROOT}/NoteNaming.cpp
        ${REPO_ROOT}/NoteNaming.h
        ${REPO_ROOT}/Instruments.cpp
        ${REPO_ROOT}/Instruments.h)

target_include_directories(MicroGuitarIntegrationTests PRIVATE ${REPO_ROOT})

target_link_libraries(MicroGuitarIntegrationTests
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_processors
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# Feature flags: disable platform backends whose dev headers are absent so the harness builds
# on minimal sandboxes; full CI/dev machines keep every backend (headers present -> flags on).
include(CheckIncludeFile)
check_include_file("X11/extensions/Xrandr.h" HAVE_XRANDR)
check_include_file("X11/extensions/Xinerama.h" HAVE_XINERAMA)
check_include_file("X11/Xcursor/Xcursor.h" HAVE_XCURSOR)
check_include_file("alsa/asoundlib.h" HAVE_ALSA)
check_include_file("jack/jack.h" HAVE_JACK)
check_include_file("X11/extensions/XInput2.h" HAVE_XINPUT)
target_compile_definitions(MicroGuitarIntegrationTests PRIVATE
    JucePlugin_Build_VST3=0
    JucePlugin_Build_VST2=0
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    $<$<NOT:$<BOOL:${HAVE_XRANDR}>>:JUCE_USE_XRANDR=0>
    $<$<NOT:$<BOOL:${HAVE_XINERAMA}>>:JUCE_USE_XINERAMA=0>
    $<$<NOT:$<BOOL:${HAVE_XCURSOR}>>:JUCE_USE_XCURSOR=0>
    $<$<NOT:$<BOOL:${HAVE_ALSA}>>:JUCE_ALSA=0>
    $<$<NOT:$<BOOL:${HAVE_JACK}>>:JUCE_JACK=0>
    $<$<NOT:$<BOOL:${HAVE_XINPUT}>>:JUCE_USE_XINPUT=0>)

enable_testing()
add_test(NAME MicroGuitarIntegrationTests COMMAND MicroGuitarIntegrationTests)
