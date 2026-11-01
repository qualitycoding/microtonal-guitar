cmake_minimum_required(VERSION 3.22)
project(MicroGuitar VERSION 1.0.0)

# 1. Point this to exactly where you extracted JUCE!
# Use forward slashes (/) even on Windows.
# Ensure JUCE does not try to build VST2 or pick up a VST2 SDK on this machine.
# We only want to build VST3 (and Standalone) for this project.
set(JUCE_BUILD_VST2 OFF CACHE BOOL "Disable building VST2 support in JUCE")
set(JUCE_VST2_SDK_PATH "" CACHE PATH "Clear any VST2 SDK path so JUCE won't enable VST2")

# Also add global compile definitions so JUCE's module sources see that we only
# want VST3. This helps avoid the VST2/VST3 parameter-automation conflict check
# which can be triggered when both are detected.
add_compile_definitions(JucePlugin_Build_VST3=1 JucePlugin_Build_VST2=0)
add_compile_definitions(JUCE_VST3_CAN_REPLACE_VST2=0)
add_subdirectory("D:/JUCE" JUCE)

# 2. Define the Plugin
juce_add_plugin(MicroGuitar
    COMPANY_NAME "Galactic Headquarters For Space Cadets"
    BUNDLE_ID "org.GalacticHQ.MicroGuitar"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE GaHq
    PLUGIN_CODE MicG
    FORMATS VST3 Standalone
    PRODUCT_NAME "MicroGuitar")

# 3. Add all the files we created
target_sources(MicroGuitar
    PRIVATE
        PluginProcessor.cpp
        PluginProcessor.h
        PluginEditor.cpp
        PluginEditor.h
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

# 4. Link the JUCE libraries our code relies on
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

# Ensure preprocessor macros force a VST3-only build to avoid JUCE's
# parameter-automation conflict check between VST2 and VST3 builds.
target_compile_definitions(MicroGuitar
    PRIVATE
        JucePlugin_Build_VST3=1
        JucePlugin_Build_VST2=0
)
