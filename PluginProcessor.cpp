#include "PluginProcessor.h"
#include "MicrotonalVoice.h"
#include "PluginEditor.h"

// --- Constructor & Parameter Layout ---
MicroGuitarProcessor::MicroGuitarProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
#endif
       apvts (*this, nullptr, "Parameters", createParameterLayout())
{
     // Create 6 voices, one for each string of a standard guitar
    for (int i = 0; i < 6; ++i) {
        // We pass 'i' as the stringIndex!
        mpeSynth.addVoice(new MicrotonalVoice(&tuningEngine));
    }

    // Register this class to listen for parameter changes
    apvts.addParameterListener("stringCount", this);
    apvts.addParameterListener("tuningType", this);
    apvts.addParameterListener("refFreq", this);

    // Initialize the tuning engine with the default parameters
    updateGuitarState();
}

MicroGuitarProcessor::~MicroGuitarProcessor()
{
    apvts.removeParameterListener("stringCount", this);
    apvts.removeParameterListener("tuningType", this);
    apvts.removeParameterListener("refFreq", this);
}

// Define the parameters exposed to the DAW
juce::AudioProcessorValueTreeState::ParameterLayout MicroGuitarProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // 1. String Count (Slider: 4 to 12 strings, default 6)
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        "stringCount", "Number of Strings", 4, 12, 6));

    // 2. Reference Frequency (Slider: 400Hz to 480Hz, default 440Hz)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "refFreq", "A4 Reference (Hz)", juce::NormalisableRange<float>(400.0f, 480.0f, 0.1f), 440.0f));

    // 3. Tuning Preset (Dropdown menu)
    juce::StringArray tuningOptions = { "12-TET", "24-TET", "31-EDO", "5-Limit Just" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "tuningType", "Microtonal Scale", tuningOptions, 0)); // 0 is 12-TET default

    return { params.begin(), params.end() };
}

// --- Parameter Handling Logic ---
void MicroGuitarProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    // If the user changes a knob in Ableton, update our engine
    if (parameterID == "stringCount" || parameterID == "tuningType" || parameterID == "refFreq") 
    {
        updateGuitarState();
    }
}

void MicroGuitarProcessor::updateGuitarState()
{
    GuitarState newState;
    
    // Read the current values from the APVTS
    newState.numStrings = apvts.getRawParameterValue("stringCount")->load();
    newState.referenceFrequency = apvts.getRawParameterValue("refFreq")->load();
    
    int tuningChoice = apvts.getRawParameterValue("tuningType")->load();

    // Map the dropdown choice to our factory methods
    switch (tuningChoice)
    {
        case 0: newState.currentTuning = TuningEngine::create12TET(); break;
        case 1: newState.currentTuning = TuningEngine::create24TET(); break;
        case 2: newState.currentTuning = TuningEngine::create31EDO(); break;
        case 3: newState.currentTuning = TuningEngine::create5LimitJust(); break;
        default: newState.currentTuning = TuningEngine::create12TET(); break;
    }

    // Set standard open string intervals (Standard E tuning mapping)
    // In a full build, this would also be dynamic based on numStrings
    newState.openStringFrequencies = { 82.41, 110.00, 146.83, 196.00, 246.94, 329.63 };

    // Push to the engine
    tuningEngine.setGuitarState(newState);
}

// --- Core Audio/MIDI Processing ---
void MicroGuitarProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mpeSynth.setCurrentPlaybackSampleRate(sampleRate);
}

void MicroGuitarProcessor::releaseResources()
{
    // Free up any memory when playback stops
}

void MicroGuitarProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Create a new buffer to hold our "Guitar-ified" MIDI
    juce::MidiBuffer processedMidi;
    int currentStringTarget = 0;

    for (const auto meta : midiMessages)
    {
        auto message = meta.getMessage();

        if (message.isNoteOn() || message.isNoteOff() || message.isPitchWheel())
        {
            // Simple "Round Robin" string allocation for the MVP.
            // In a real playing scenario, it cycles through strings 0 to 5.
            // MPE Channels start at 2 (Channel 1 is for global messages).
            int targetChannel = 2 + currentStringTarget;
            
            message.setChannel(targetChannel);
            processedMidi.addEvent(message, meta.samplePosition);

            // Move to the next string for the next note played
            if (message.isNoteOn()) {
                currentStringTarget = (currentStringTarget + 1) % 6;
            }
        }
        else 
        {
            // Pass through other MIDI data untouched
            processedMidi.addEvent(message, meta.samplePosition);
        }
    }

    // Pass our routed MIDI to the synthesizer instead of the raw keyboard MIDI
    mpeSynth.renderNextBlock(buffer, processedMidi, 0, buffer.getNumSamples());
}

// --- DAW State Saving/Loading ---
void MicroGuitarProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Saves the plugin state into the Ableton project file
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void MicroGuitarProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Loads the plugin state from the Ableton project file
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessorEditor* MicroGuitarProcessor::createEditor() 
{ 
    return new MicroGuitarEditor(*this); 
}

// This is the entry point that JUCE's plugin wrappers expect.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MicroGuitarProcessor();
}

