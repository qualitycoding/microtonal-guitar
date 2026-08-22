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
    noteToStringMap.fill (-1);
    for (auto& f : activeFrets) f.store (-1);

    for (int i = 0; i < maxStrings; ++i) {
        mpeSynth.addVoice(new MicrotonalVoice(&tuningEngine));
    }

    apvts.addParameterListener("stringCount", this);
    apvts.addParameterListener("tuningType", this);
    apvts.addParameterListener("refFreq", this);

    updateGuitarState();
}

MicroGuitarProcessor::~MicroGuitarProcessor()
{
    apvts.removeParameterListener("stringCount", this);
    apvts.removeParameterListener("tuningType", this);
    apvts.removeParameterListener("refFreq", this);
    apvts.removeParameterListener("musicalKey", this); // REMOVED listener
}

void MicroGuitarProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    juce::ignoreUnused (newValue);

    if (parameterID == "stringCount" || parameterID == "tuningType" || 
        parameterID == "refFreq" || parameterID == "musicalKey") // ADDED check
    {
        updateGuitarState();
    }
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
        "tuningType", "Microtonal Scale", tuningOptions, 0));

    // 4. ADDED: Musical Key Dropdown parameter [2, 4]
    juce::StringArray keyOptions = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "musicalKey", "Musical Key", keyOptions, 0)); // Defaults to C (0)

    return { params.begin(), params.end() };
}

void MicroGuitarProcessor::updateGuitarState()
{
    GuitarState newState;
    
    // FIXED: Added explicit (int) casts to resolve warning C4244
    newState.numStrings = (int)apvts.getRawParameterValue("stringCount")->load();
    newState.referenceFrequency = apvts.getRawParameterValue("refFreq")->load();
    
    int tuningChoice = (int)apvts.getRawParameterValue("tuningType")->load();

    switch (tuningChoice)
    {
        case 0: newState.currentTuning = TuningEngine::create12TET(); break;
        case 1: newState.currentTuning = TuningEngine::create24TET(); break;
        case 2: newState.currentTuning = TuningEngine::create31EDO(); break;
        case 3: newState.currentTuning = TuningEngine::create5LimitJust(); break;
        default: newState.currentTuning = TuningEngine::create12TET(); break;
    }

    newState.openStringFrequencies = { 82.41, 110.00, 146.83, 196.00, 246.94, 329.63 };

    tuningEngine.setGuitarState(newState);
}

void MicroGuitarProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock); // FIXED: Clears the unreferenced parameter warning
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

    juce::MidiBuffer processedMidi;
    const int numStrings = currentNumStrings.load();

    for (const auto meta : midiMessages)
    {
        auto message = meta.getMessage();

        if (message.isNoteOn())
        {
            int noteNum = message.getNoteNumber();

            // Pick the string that puts this note at the lowest non-negative fret -
            // the same choice a guitarist would make: the highest-pitched open
            // string that's still at or below the note being played.
            int stringIdx = 0;
            int fret = 0;
            bool found = false;

            for (int s = 0; s < numStrings; ++s)
            {
                int openNote = TuningEngine::openStringMidiNote (s);
                int candidateFret = noteNum - openNote;
                if (candidateFret >= 0 && (! found || candidateFret < fret))
                {
                    found = true;
                    fret = candidateFret;
                    stringIdx = s;
                }
            }
            // If the note is below every open string, this falls back to string 0, fret 0.

            noteToStringMap[(size_t) noteNum] = stringIdx;
            activeFrets[(size_t) stringIdx].store (fret);

            message.setChannel (2 + stringIdx);
            processedMidi.addEvent (message, meta.samplePosition);
        }
        else if (message.isNoteOff())
        {
            int noteNum = message.getNoteNumber();
            int stringIdx = noteToStringMap[(size_t) noteNum];
            if (stringIdx >= 0)
            {
                message.setChannel (2 + stringIdx);
                processedMidi.addEvent (message, meta.samplePosition);
                activeFrets[(size_t) stringIdx].store (-1);
                noteToStringMap[(size_t) noteNum] = -1;
            }
        }
        else if (message.isPitchWheel())
        {
            for (int s = 0; s < numStrings; ++s)
                if (activeFrets[(size_t) s].load() >= 0)
                {
                    auto bendMsg = message;
                    bendMsg.setChannel (2 + s);
                    processedMidi.addEvent (bendMsg, meta.samplePosition);
                }
        }
        else
        {
            processedMidi.addEvent (message, meta.samplePosition);
        }
    }

    mpeSynth.renderNextBlock (buffer, processedMidi, 0, buffer.getNumSamples());
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

std::vector<MicroGuitarProcessor::ActiveNote> MicroGuitarProcessor::getActiveNotes() const
{
    std::vector<ActiveNote> active;
    
    for (int i = 0; i < mpeSynth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<const MicrotonalVoice*>(mpeSynth.getVoice(i)))
        {
            if (voice->isActive())
            {
                active.push_back({ voice->getStringIndex(), voice->getFretNumber() });
            }
        }
    }
    
    return active;
}

int MicroGuitarProcessor::getFretCount() const
{
    // FIXED: Added explicit (int) cast to resolve warning C4244
    int tuningChoice = (int)apvts.getRawParameterValue("tuningType")->load();
    switch (tuningChoice)
    {
        case 0: return 24; // 12-TET -> 24 frets (2 octaves)
        case 1: return 24; // 24-TET -> 24 frets (1 octave)
        case 2: return 31; // 31-EDO -> 31 frets (1 octave)
        case 3: return 12; // 5-Limit Just -> 12 frets (1 octave)
        default: return 24;
    }
}