#pragma once

#include "JuceHeader.h"
#include "TuningEngine.h"

class MicroGuitarProcessor : public juce::AudioProcessor, 
                             public juce::AudioProcessorValueTreeState::Listener
{
public:
    MicroGuitarProcessor();
    ~MicroGuitarProcessor() override;

    // --- Standard JUCE AudioProcessor Overrides ---
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    
    const juce::String getName() const override { return "MicroGuitar"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- APVTS Listener Override ---
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    // APVTS object must be public or accessible by the Editor (UI)
    juce::AudioProcessorValueTreeState apvts;

        struct ActiveNote {
        int stringIndex;
        int fretNumber;
    };

    std::vector<ActiveNote> getActiveNotes() const;
    int getFretCount() const;

private:
    // Our math engine
    TuningEngine tuningEngine;
    juce::MPESynthesiser mpeSynth;


    // Helper to build the parameters Ableton will see
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // Helper to push DAW parameter changes into our TuningEngine
    void updateGuitarState();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MicroGuitarProcessor)
};
