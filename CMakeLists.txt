cmake_minimum_required(VERSION 3.22)

# Single source of truth for the release version (issue #9): the VERSION file at the
# repo root. Scheme: semantic versioning (MAJOR.MINOR.PATCH), tagged as vX.Y.Z; CI
# publishes a GitHub Release from v* tags. Anything not from a tag is a dev build.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" MG_VERSION LIMIT_COUNT 1)
string(STRIP "${MG_VERSION}" MG_VERSION)
if(NOT MG_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "VERSION must contain a single MAJOR.MINOR.PATCH line, got '${MG_VERSION}'")
endif()
project(MicroGuitar VERSION ${MG_VERSION})

# --- VST2 stays off (unchanged intent) ---
set(JUCE_BUILD_VST2 OFF CACHE BOOL "Disable building VST2 support in JUCE")
set(JUCE_VST2_SDK_PATH "" CACHE PATH "Clear any VST2 SDK path so JUCE won't enable VST2")

# Preprocessor macros so JUCE's module sources see that we only want VST3,
# avoiding the VST2/VST3 parameter-automation conflict check.
add_compile_definitions(JucePlugin_Build_VST3=1 JucePlugin_Build_VST2=0)
add_compile_definitions(JUCE_VST3_CAN_REPLACE_VST2=0)

# --- Portable JUCE acquisition (fixes REVIEW.md G-3: no more "D:/JUCE") ---
#
# Default: FetchContent clones the pinned JUCE tag (existence verified at planning
# time against the JUCE repo's tag list; see plan/ENVIRONMENT.md). The VST3 SDK is
# bundled with JUCE 9, so no external SDK is needed on any platform.
#
# Local/air-gapped override: pass it as a -D CONFIGURE ARGUMENT (never an env var —
# CMake ignores env overrides):
#   cmake -S . -B build -DFETCHCONTENT_SOURCE_DIR_JUCE=/absolute/path/to/JUCE
#
# To pin a different JUCE version:
#   cmake -S . -B build -DMICROGUITAR_JUCE_TAG=8.0.16-security
include(FetchContent)
set(MICROGUITAR_JUCE_TAG "9.0.4" CACHE STRING
    "JUCE git tag fetched by FetchContent (must exist in juce-framework/JUCE)")
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        ${MICROGUITAR_JUCE_TAG}
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

# --- Plugin definition ---
# Don't try to install into system plugin folders on CI runners.
if(DEFINED ENV{CI})
    set(MG_COPY_PLUGIN_AFTER_BUILD OFF)
else()
    set(MG_COPY_PLUGIN_AFTER_BUILD ON)
endif()

juce_add_plugin(MicroGuitar
    COMPANY_NAME "Galactic Headquarters For Space Cadets"
    BUNDLE_ID "org.GalacticHQ.MicroGuitar"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${MG_COPY_PLUGIN_AFTER_BUILD}
    PLUGIN_MANUFACTURER_CODE GaHq
    PLUGIN_CODE MicG
    FORMATS VST3 Standalone
    PRODUCT_NAME "MicroGuitar"
    VERSION ${MG_VERSION})

# --- Sources ---
target_sources(MicroGuitar
    PRIVATE
        PluginProcessor.cpp
        PluginProcessor.h
        PluginEditor.cpp
        PluginEditor.h
        EdoControl.h
        TuningEngine.cpp
        TuningEngine.h
        TuningStructures.h
        MicrotonalVoice.cpp
        MicrotonalVoice.h
        NoteNaming.cpp
        NoteNaming.h
        Instruments.cpp
        Instruments.h
)

# --- JUCE module linkage ---
target_link_libraries(MicroGuitar
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

target_compile_definitions(MicroGuitar
    PRIVATE
        JucePlugin_Build_VST3=1
        JucePlugin_Build_VST2=0
)
