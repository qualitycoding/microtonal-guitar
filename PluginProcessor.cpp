// SPDX-License-Identifier: Apache-2.0
#include "PluginProcessor.h"
#include "MicrotonalVoice.h"
#include "PluginEditor.h"
#include "NoteNaming.h"

#include <cmath>

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

    for (int i = 0; i < maxStrings; ++i)
        mpeSynth.addVoice (new MicrotonalVoice (&tuningEngine));

    apvts.addParameterListener ("stringCount", this);
    apvts.addParameterListener ("refFreq", this);
    apvts.addParameterListener ("instrument", this);
    apvts.addParameterListener ("tuningMode", this);
    apvts.addParameterListener ("edoSteps", this);

    updateGuitarState();
}

MicroGuitarProcessor::~MicroGuitarProcessor()
{
    apvts.removeParameterListener ("stringCount", this);
    apvts.removeParameterListener ("refFreq", this);
    apvts.removeParameterListener ("instrument", this);
    apvts.removeParameterListener ("tuningMode", this);
    apvts.removeParameterListener ("edoSteps", this);
    // No "musicalKey" removal here: that listener is deliberately not registered (the parameter is
    // unwired, backlog B2), and removing a listener that was never added is misleading residue.
}

void MicroGuitarProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    juce::ignoreUnused (newValue);

    if (parameterID == "stringCount" || parameterID == "refFreq" || parameterID == "instrument"
        || parameterID == "tuningMode" || parameterID == "edoSteps")
    {
        updateGuitarState();
    }
}

// Define the parameters exposed to the DAW
juce::AudioProcessorValueTreeState::ParameterLayout MicroGuitarProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // 1. String Count (Slider: 4 to 15 strings, default 6). The usable range depends on the chosen
    //    instrument (see Instruments.h); updateGuitarState() clamps, so this parameter's own range
    //    stays wide enough for every instrument and for existing host automation to keep working.
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        "stringCount", "Number of Strings", 4, TuningEngine::kMaxStrings, 6));

    // 2. Reference Frequency (Slider: 400Hz to 480Hz, default 440Hz) - retunes the whole instrument.
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "refFreq", "A4 Reference (Hz)", juce::NormalisableRange<float> (400.0f, 480.0f, 0.1f), 440.0f));

    // 3. Instrument preset (Dropdown menu) - open-string layout; see Instruments.h.
    juce::StringArray instrumentOptions;
    for (int i = 0; i < mtg::kNumInstruments; ++i)
        instrumentOptions.add (mtg::instrumentInfo (static_cast<mtg::Instrument> (i)).name);
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "instrument", "Instrument", instrumentOptions, 0)); // default Guitar

    // 4. Tuning mode (Dropdown menu): an arbitrary EDO, or fixed 5-Limit Just Intonation.
    juce::StringArray tuningModeOptions { "EDO", "5-Limit Just" };
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "tuningMode", "Tuning Mode", tuningModeOptions, 0)); // default EDO

    // 5. Steps per octave (free integer entry 1-96, default 12) - only used when tuningMode is EDO.
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        "edoSteps", "Steps per Octave", 1, 96, 12));

    // 6. Musical Key Dropdown parameter (pre-existing; its listener is not wired - backlog B2).
    juce::StringArray keyOptions = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "musicalKey", "Musical Key", keyOptions, 0)); // Defaults to C (0)

    return { params.begin(), params.end() };
}

void MicroGuitarProcessor::updateGuitarState()
{
    GuitarState newState;

    const int requestedStrings = (int) apvts.getRawParameterValue ("stringCount")->load();
    const double refFreq = (double) apvts.getRawParameterValue ("refFreq")->load();

    const int instrumentChoice = (int) apvts.getRawParameterValue ("instrument")->load();
    const auto instrument = static_cast<mtg::Instrument> (
        juce::jlimit (0, mtg::kNumInstruments - 1, instrumentChoice));

    // Single source of truth for this instrument's open strings: every open-string frequency and
    // MIDI note below is derived from this one table, so labels, routing and pitch can never disagree.
    const auto midiNotes = mtg::openStringMidiNotes (instrument, requestedStrings);

    newState.numStrings = (int) midiNotes.size();
    newState.referenceFrequency = refFreq;
    newState.openStringMidiNotes = midiNotes;
    newState.openStringFrequencies.reserve (midiNotes.size());
    for (int midi : midiNotes)
        newState.openStringFrequencies.push_back (refFreq * std::pow (2.0, (midi - 69) / 12.0));

    const int tuningModeChoice = (int) apvts.getRawParameterValue ("tuningMode")->load();
    if (tuningModeChoice == 1)
    {
        newState.currentTuning = TuningEngine::create5LimitJust();
    }
    else
    {
        const int edoSteps = (int) apvts.getRawParameterValue ("edoSteps")->load();
        newState.currentTuning = TuningEngine::createEdo (edoSteps);
    }

    tuningEngine.setGuitarState (newState);
    currentNumStrings.store (newState.numStrings); // keeps processBlock's routing count in sync
}

void MicroGuitarProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    mpeSynth.setCurrentPlaybackSampleRate (sampleRate);
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
            const int noteNum = message.getNoteNumber();

            // Pick the string that puts this note at the lowest non-negative fret - the same choice
            // a guitarist would make: the highest-pitched open string that's still at or below the
            // note being played. Open-string notes come from the engine snapshot (the instrument's
            // current layout), not a fixed formula, so this works for every instrument and string count.
            int stringIdx = 0;
            int fret = 0;
            bool found = false;

            for (int s = 0; s < numStrings; ++s)
            {
                const int openNote = tuningEngine.getOpenStringMidiNote (s);
                const int candidateFret = noteNum - openNote;
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
            const int noteNum = message.getNoteNumber();
            const int stringIdx = noteToStringMap[(size_t) noteNum];
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
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState == nullptr || ! xmlState->hasTagName (apvts.state.getType()))
        return;

    // Legacy state (commit 9644510 and earlier) used a single "tuningType" choice parameter and had
    // no "edoSteps" / "tuningMode" / "instrument" nodes at all. Detect it before applying the tree so
    // we can migrate its meaning, rather than silently reinterpreting a missing node as some default.
    bool isLegacyState = false;
    int legacyTuningType = 0;
    for (auto* param : xmlState->getChildIterator())
    {
        if (param->getStringAttribute ("id") == "tuningType")
        {
            isLegacyState = true;
            legacyTuningType = (int) param->getDoubleAttribute ("value");
        }
    }

    apvts.replaceState (juce::ValueTree::fromXml (*xmlState));

    if (isLegacyState)
    {
        // The old "tuningType" combo offered exactly these four presets, in this order.
        auto* edoParam = apvts.getParameter ("edoSteps");
        auto* modeParam = apvts.getParameter ("tuningMode");
        auto* instrumentParam = apvts.getParameter ("instrument");

        switch (legacyTuningType)
        {
            case 1: edoParam->setValueNotifyingHost (edoParam->convertTo0to1 (24.0f));
                    modeParam->setValueNotifyingHost (0.0f);
                    break;
            case 2: edoParam->setValueNotifyingHost (edoParam->convertTo0to1 (31.0f));
                    modeParam->setValueNotifyingHost (0.0f);
                    break;
            case 3: modeParam->setValueNotifyingHost (1.0f); // 5-Limit Just
                    break;
            default: edoParam->setValueNotifyingHost (edoParam->convertTo0to1 (12.0f));
                     modeParam->setValueNotifyingHost (0.0f);
                     break;
        }
        instrumentParam->setValueNotifyingHost (0.0f); // Guitar - the only layout the old code had
    }

    // A parameter listener does not fire when replaceState leaves a value unchanged from its current
    // (default) value, so the engine is not guaranteed to be in sync purely from the notifications
    // above. Call this unconditionally for both legacy and current-format loads.
    updateGuitarState();
}

juce::AudioProcessorEditor* MicroGuitarProcessor::createEditor()
{
    return new MicroGuitarEditor (*this);
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
        if (auto* voice = dynamic_cast<const MicrotonalVoice*> (mpeSynth.getVoice (i)))
        {
            if (voice->isActive())
            {
                active.push_back ({ voice->getStringIndex(), voice->getFretNumber() });
            }
        }
    }

    return active;
}

int MicroGuitarProcessor::getFretCount() const
{
    const auto tuning = tuningEngine.getCurrentTuningSystem();
    if (! tuning.isEqualTemperament)
        return (int) tuning.customRatios.size(); // 5-Limit Just -> 12 frets (unchanged behaviour)
    return mtg::fretCountFor (tuning.stepsPerOctave);
}
