// FROZEN TEST SUITE (r3) — see tests/FROZEN.md. Implementers must not edit, skip or weaken.
// Instrument presets: parameter, engine, routing and state. Tables come from mtg::openStringMidiNotes
// (pinned independently by unit tests U12-U15). Compile-unverified at freeze; harness-only fixes per PLAN S4.
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "Instruments.h"
#include <cmath>

namespace
{
    void setP (MicroGuitarProcessor& p, const juce::String& id, float plainValue)
    {
        auto* param = p.apvts.getParameter (id);
        jassert (param != nullptr);
        param->setValueNotifyingHost (param->convertTo0to1 (plainValue));
    }

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

class InstrumentTests : public juce::UnitTest
{
public:
    InstrumentTests() : juce::UnitTest ("InstrumentTests", "MicroGuitar") {}

    void runTest() override
    {
        beginTest ("I13 'instrument' parameter lists the five presets, default Guitar");
        {
            MicroGuitarProcessor p;
            auto* inst = dynamic_cast<juce::AudioParameterChoice*> (p.apvts.getParameter ("instrument"));
            expect (inst != nullptr);
            if (inst != nullptr)
            {
                expectEquals (inst->choices.size(), mtg::kNumInstruments);
                for (int i = 0; i < mtg::kNumInstruments; ++i)
                    expectEquals (inst->choices[i], juce::String (mtg::instrumentInfo (static_cast<mtg::Instrument> (i)).name));
                expectEquals (inst->getIndex(), 0);
            }
        }

        beginTest ("I14 engine open strings follow instrument, string count and refFreq");
        {
            MicroGuitarProcessor p;
            setP (p, "refFreq", 432.0f);
            for (int i = 0; i < mtg::kNumInstruments; ++i)
            {
                setP (p, "instrument", (float) i);
                for (int n = 4; n <= 12; ++n)
                {
                    setP (p, "stringCount", (float) n);
                    const auto table = mtg::openStringMidiNotes (static_cast<mtg::Instrument> (i), n);
                    expectEquals (p.getTuningEngine().getNumStrings(), (int) table.size());
                    for (int s = 0; s < (int) table.size(); ++s)
                        expectWithinAbsoluteError (p.getTuningEngine().getFrequency (s, 0),
                                                   432.0 * std::pow (2.0, (table[(size_t) s] - 69) / 12.0), 1e-6);
                }
            }
        }

        beginTest ("I15 note routing uses the active instrument (router, voice and UI agree)");
        {
            struct Case { int instrument, strings, note, expString, expFret; };
            const Case cases[] = {
                { 0, 6, 59, 4, 0 },   // guitar: open B3 (today it lands on the G string, fret 4)
                { 0, 7, 59, 5, 0 },   // 7-string: B3 string moves up one index
                { 0, 7, 35, 0, 0 },   // 7-string: open low B1
                { 1, 5, 23, 0, 0 },   // 5-string bass: open low B0
                { 1, 6, 48, 5, 0 },   // 6-string bass: open high C3
                { 1, 4, 45, 3, 2 },   // 4-string bass: A2 on the G2 string, fret 2
                { 2, 4, 76, 3, 0 },   // violin: open E5
                { 4, 5, 29, 0, 0 },   // 5-string cello: open low F1
            };
            for (const auto& c : cases)
            {
                MicroGuitarProcessor p;
                setP (p, "instrument", (float) c.instrument);
                setP (p, "stringCount", (float) c.strings);
                p.prepareToPlay (44100.0, 512);
                playNote (p, c.note);
                const juce::String what = "instrument " + juce::String (c.instrument) + ", note " + juce::String (c.note);
                expectEquals (p.getActiveFret (c.expString), c.expFret, what);
                expect (hasActiveNote (p, c.expString, c.expFret), what);
            }
        }

        beginTest ("I16 state: legacy sets load as Guitar; Cello round-trips");
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
                setP (b, "instrument", 4.0f);
                b.getStateInformation (saved);
            }
            MicroGuitarProcessor c;
            c.setStateInformation (saved.getData(), (int) saved.getSize());
            expectWithinAbsoluteError (c.getTuningEngine().getFrequency (0, 0),
                                       440.0 * std::pow (2.0, (36 - 69) / 12.0), 1e-6);
        }
    }
};

static InstrumentTests instrumentTests;
