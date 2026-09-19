// FROZEN TEST SUITE — see tests/FROZEN.md. Implementers must not edit, skip or weaken.
// JUCE integration tests. Built by the MicroGuitarIntegrationTests console target (plan step S7)
// and run headless:  MicroGuitarIntegrationTests.exe  (exit code != 0 on any failure).
// NOTE: compile-unverified at freeze time (JUCE is not available in the planning sandbox);
// a compile error that requires changing an ASSERTION is a Reset-Rule event, but fixing an
// include path or JUCE namespace qualifier that does not change what is asserted is allowed
// and must be logged in .checkpoints/state.json under "test_harness_fixes".

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <cmath>

namespace
{
    void setParam (MicroGuitarProcessor& p, const juce::String& id, float plainValue)
    {
        auto* param = p.apvts.getParameter (id);
        jassert (param != nullptr);
        param->setValueNotifyingHost (param->convertTo0to1 (plainValue));
    }

    juce::MemoryBlock legacyState (int tuningTypeIndex)
    {
        // Shape of state written by the current release (commit 9644510): APVTS XML
        // with a "tuningType" choice parameter and no "edoSteps"/"tuningMode".
        juce::XmlElement root ("Parameters");
        auto add = [&root] (const char* id, double v)
        {
            auto* e = root.createNewChildElement ("PARAM");
            e->setAttribute ("id", id);
            e->setAttribute ("value", v);
        };
        add ("stringCount", 6.0);
        add ("refFreq", 440.0);
        add ("tuningType", (double) tuningTypeIndex);
        add ("musicalKey", 0.0);

        juce::MemoryBlock mb;
        juce::AudioProcessor::copyXmlToBinary (root, mb);
        return mb;
    }
}

class TuningParameterTests : public juce::UnitTest
{
public:
    TuningParameterTests() : juce::UnitTest ("TuningParameterTests", "MicroGuitar") {}

    void runTest() override
    {
        beginTest ("I1 parameter layout: edoSteps int [1,96] default 12, tuningMode, no tuningType");
        {
            MicroGuitarProcessor p;
            auto* edo = dynamic_cast<juce::AudioParameterInt*> (p.apvts.getParameter ("edoSteps"));
            expect (edo != nullptr);
            if (edo != nullptr)
            {
                expectEquals (edo->getRange().getStart(), 1);
                expectEquals (edo->getRange().getEnd(), 96);
                expectEquals (edo->get(), 12);
            }
            auto* mode = dynamic_cast<juce::AudioParameterChoice*> (p.apvts.getParameter ("tuningMode"));
            expect (mode != nullptr);
            if (mode != nullptr) expectEquals (mode->getIndex(), 0); // 0 = EDO
            expect (p.apvts.getParameter ("tuningType") == nullptr);
        }

        beginTest ("I2 edoSteps change reaches TuningEngine (listener wired)");
        {
            MicroGuitarProcessor p;
            setParam (p, "edoSteps", 13.0f);
            auto t = p.getTuningEngine().getCurrentTuningSystem();
            expect (t.isEqualTemperament);
            expectEquals (t.stepsPerOctave, 13);
            expectEquals (p.getFretCount(), 26);
        }

        beginTest ("I3 tuningMode Just5 selects 5-limit JI");
        {
            MicroGuitarProcessor p;
            setParam (p, "tuningMode", 1.0f);
            auto t = p.getTuningEngine().getCurrentTuningSystem();
            expect (! t.isEqualTemperament);
            expectEquals ((int) t.customRatios.size(), 12);
            expectEquals (p.getFretCount(), 12); // unchanged from current behaviour
        }

        beginTest ("I4 legacy state migration: tuningType index -> edoSteps / tuningMode");
        {
            const int expectedSteps[] = { 12, 24, 31 };
            for (int legacy = 0; legacy < 3; ++legacy)
            {
                MicroGuitarProcessor p;
                auto mb = legacyState (legacy);
                p.setStateInformation (mb.getData(), (int) mb.getSize());
                auto t = p.getTuningEngine().getCurrentTuningSystem();
                expect (t.isEqualTemperament);
                expectEquals (t.stepsPerOctave, expectedSteps[legacy]);
            }
            MicroGuitarProcessor p;
            auto mb = legacyState (3);
            p.setStateInformation (mb.getData(), (int) mb.getSize());
            expect (! p.getTuningEngine().getCurrentTuningSystem().isEqualTemperament);
        }

        beginTest ("I5 new state round-trips edoSteps");
        {
            juce::MemoryBlock saved;
            {
                MicroGuitarProcessor p;
                setParam (p, "edoSteps", 13.0f);
                p.getStateInformation (saved);
            }
            MicroGuitarProcessor q;
            q.setStateInformation (saved.getData(), (int) saved.getSize());
            expectEquals (q.getTuningEngine().getCurrentTuningSystem().stepsPerOctave, 13);
        }

        beginTest ("I6 every string count 4..12 yields finite, positive open-string frequencies");
        {
            MicroGuitarProcessor p;
            for (int n = 4; n <= 12; ++n)
            {
                setParam (p, "stringCount", (float) n);
                expectEquals (p.getTuningEngine().getNumStrings(), n);
                for (int s = 0; s < n; ++s)
                {
                    const double f = p.getTuningEngine().getFrequency (s, 0);
                    expect (std::isfinite (f) && f > 0.0, "string " + juce::String (s) + " of " + juce::String (n));
                }
            }
        }

        beginTest ("I7 refFreq reaches the engine (labels and pitch agree)");
        {
            MicroGuitarProcessor p;
            const double before = p.getTuningEngine().getFrequency (1, 0); // A string
            setParam (p, "refFreq", 432.0f);
            const double after = p.getTuningEngine().getFrequency (1, 0);
            expectWithinAbsoluteError (after / before, 432.0 / 440.0, 1e-6);
        }

        beginTest ("I8 open-string MIDI note and open-string frequency are one source of truth");
        {
            // Today string 4 is MIDI 60 (C4) for note routing but 246.94 Hz (B3) for pitch.
            MicroGuitarProcessor p;
            setParam (p, "stringCount", 12.0f);
            const int standard[] = { 40, 45, 50, 55, 59, 64 }; // E2 A2 D3 G3 B3 E4
            for (int s = 0; s < 6; ++s)
                expectEquals (TuningEngine::openStringMidiNote (s), standard[s]);
            for (int s = 0; s < 12; ++s)
            {
                const double expected = 440.0 * std::pow (2.0, (TuningEngine::openStringMidiNote (s) - 69) / 12.0);
                expectWithinAbsoluteError (p.getTuningEngine().getFrequency (s, 0), expected, 1e-6);
            }
        }
    }
};

static TuningParameterTests tuningParameterTests;
