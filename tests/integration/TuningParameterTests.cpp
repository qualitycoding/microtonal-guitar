// FROZEN TEST SUITE (re-frozen r4: I6 replaced twice, I8 retired) — see tests/FROZEN.md. Implementers must not edit, skip or weaken.
// JUCE integration tests. Built by the MicroGuitarIntegrationTests console target (plan step S7)
// and run headless:  MicroGuitarIntegrationTests.exe  (exit code != 0 on any failure).
// NOTE: compile-unverified at freeze time (JUCE is not available in the planning sandbox);
// a compile error that requires changing an ASSERTION is a Reset-Rule event, but fixing an
// include path or JUCE namespace qualifier that does not change what is asserted is allowed
// and must be logged in .checkpoints/state.json under "test_harness_fixes".

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
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

        beginTest ("I6 (r4) stringCount 4..15; guitar clamps to 10; finite, positive open strings");
        {
            MicroGuitarProcessor p; // default instrument = Guitar, 4..10 strings (r4)
            auto* sc = dynamic_cast<juce::AudioParameterInt*> (p.apvts.getParameter ("stringCount"));
            expect (sc != nullptr);
            if (sc != nullptr) { expectEquals (sc->getRange().getStart(), 4); expectEquals (sc->getRange().getEnd(), 15); }
            for (int n = 4; n <= 15; ++n)
            {
                setParam (p, "stringCount", (float) n);
                const int effective = juce::jlimit (4, 10, n);
                expectEquals (p.getTuningEngine().getNumStrings(), effective);
                expectEquals (p.getNumStrings(), effective); // router uses the same count (C3)
                for (int s = 0; s < effective; ++s)
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

        // I8 retired in r3 (owner decision: string layout depends on instrument and string count);
        // superseded by U12-U15 and I13-I16.
    }
};

static TuningParameterTests tuningParameterTests;
