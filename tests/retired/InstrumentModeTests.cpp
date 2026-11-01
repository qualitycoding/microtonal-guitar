// FROZEN TEST SUITE (revision r2) — see tests/FROZEN.md. Implementers must not edit, skip or weaken.
// Adds the owner's G0 answer to D1 (2026-09-19): an Instrument mode parameter.
//   Guitar: standard tuning E2 A2 D3 G3 B3 E4 (+ fourths above E4 for strings 7-12)
//   Bass:   all fourths from E1: E1 A1 D2 G2 C3 F3 ... (28 + 5s)
// Sources: https://en.wikipedia.org/wiki/Standard_tuning , https://en.wikipedia.org/wiki/Bass_guitar_tuning
// Contract: enum class InstrumentMode { Guitar, Bass } in TuningStructures.h and
//   static int TuningEngine::openStringMidiNote (int stringIndex, InstrumentMode mode);
// The one-argument overload (used by frozen test I8) must equal the Guitar table.
// Compile-unverified at freeze; harness-only fixes per PLAN S4.

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <cmath>

namespace
{
    void setParamR2 (MicroGuitarProcessor& p, const juce::String& id, float plainValue)
    {
        auto* param = p.apvts.getParameter (id);
        jassert (param != nullptr);
        param->setValueNotifyingHost (param->convertTo0to1 (plainValue));
    }

    // Sends one note-on through processBlock exactly as the host would.
    void playNote (MicroGuitarProcessor& p, int midiNote)
    {
        juce::AudioBuffer<float> buffer (2, 512);
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, midiNote, (juce::uint8) 100), 0);
        p.processBlock (buffer, midi);
    }

    bool hasActiveNote (const MicroGuitarProcessor& p, int stringIndex, int fret)
    {
        for (const auto& n : p.getActiveNotes())
            if (n.stringIndex == stringIndex && n.fretNumber == fret) return true;
        return false;
    }
}

class InstrumentModeTests : public juce::UnitTest
{
public:
    InstrumentModeTests() : juce::UnitTest ("InstrumentModeTests", "MicroGuitar") {}

    void runTest() override
    {
        beginTest ("I9 instrumentMode parameter: choice {Guitar, Bass}, default Guitar");
        {
            MicroGuitarProcessor p;
            auto* mode = dynamic_cast<juce::AudioParameterChoice*> (p.apvts.getParameter ("instrumentMode"));
            expect (mode != nullptr);
            if (mode != nullptr)
            {
                expectEquals (mode->choices.size(), 2);
                expectEquals (mode->choices[0], juce::String ("Guitar"));
                expectEquals (mode->choices[1], juce::String ("Bass"));
                expectEquals (mode->getIndex(), 0);
            }
        }

        beginTest ("I10 open-string tables: guitar standard, bass all fourths from E1");
        {
            const int guitar[] = { 40, 45, 50, 55, 59, 64, 69, 74, 79, 84, 89, 94 };
            for (int s = 0; s < 12; ++s)
            {
                expectEquals (TuningEngine::openStringMidiNote (s, InstrumentMode::Guitar), guitar[s]);
                expectEquals (TuningEngine::openStringMidiNote (s), guitar[s]);
                expectEquals (TuningEngine::openStringMidiNote (s, InstrumentMode::Bass), 28 + 5 * s);
            }

            MicroGuitarProcessor p;
            setParamR2 (p, "stringCount", 12.0f);
            setParamR2 (p, "refFreq", 432.0f);
            setParamR2 (p, "instrumentMode", 1.0f);
            for (int s = 0; s < 12; ++s)
            {
                const double expected = 432.0 * std::pow (2.0, (28 + 5 * s - 69) / 12.0);
                expectWithinAbsoluteError (p.getTuningEngine().getFrequency (s, 0), expected, 1e-6);
            }
        }

        beginTest ("I11 note routing follows the instrument mode (router, voice and UI agree)");
        {
            {   // Guitar: MIDI 59 (B3) is the open B string - today it lands on the G string, fret 4.
                MicroGuitarProcessor p;
                p.prepareToPlay (44100.0, 512);
                playNote (p, 59);
                expectEquals (p.getActiveFret (4), 0);
                expect (hasActiveNote (p, 4, 0));
            }
            {   // Bass, 6 strings (28 33 38 43 48 53): MIDI 45 -> string 3 (G2), fret 2.
                MicroGuitarProcessor p;
                setParamR2 (p, "instrumentMode", 1.0f);
                p.prepareToPlay (44100.0, 512);
                playNote (p, 45);
                expectEquals (p.getActiveFret (3), 2);
                expect (hasActiveNote (p, 3, 2));
            }
        }

        beginTest ("I12 state: legacy sets load as Guitar; Bass round-trips");
        {
            juce::XmlElement root ("Parameters");
            auto* e = root.createNewChildElement ("PARAM");
            e->setAttribute ("id", "tuningType");
            e->setAttribute ("value", 0.0);
            juce::MemoryBlock legacy;
            juce::AudioProcessor::copyXmlToBinary (root, legacy);

            MicroGuitarProcessor a;
            a.setStateInformation (legacy.getData(), (int) legacy.getSize());
            expectWithinAbsoluteError (a.getTuningEngine().getFrequency (0, 0),
                                       440.0 * std::pow (2.0, (40 - 69) / 12.0), 1e-6);

            juce::MemoryBlock saved;
            {
                MicroGuitarProcessor b;
                setParamR2 (b, "instrumentMode", 1.0f);
                b.getStateInformation (saved);
            }
            MicroGuitarProcessor c;
            c.setStateInformation (saved.getData(), (int) saved.getSize());
            expectWithinAbsoluteError (c.getTuningEngine().getFrequency (0, 0),
                                       440.0 * std::pow (2.0, (28 - 69) / 12.0), 1e-6);
        }
    }
};

static InstrumentModeTests instrumentModeTests;
