// SPDX-License-Identifier: Apache-2.0
// Issue #6 verification (S-050): the constructor's parameter-listener set and the destructor's
// removal set must match, so repeated construction / parameter change / destruction cycles stay
// leak- and assertion-clean. Added by the issue-remediation plan; not part of the r4 freeze.
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

namespace
{
    void setParam (MicroGuitarProcessor& p, const juce::String& id, float plainValue)
    {
        if (auto* param = p.apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (plainValue));
    }
}

class LifecycleTests : public juce::UnitTest
{
public:
    LifecycleTests() : juce::UnitTest ("LifecycleTests", "MicroGuitar") {}

    void runTest() override
    {
        beginTest ("I-B2 repeated processor construction, parameter changes, destruction (issue #6)");
        {
            for (int i = 0; i < 200; ++i)
            {
                MicroGuitarProcessor p;
                p.prepareToPlay (48000.0, 512);
                setParam (p, "stringCount", (float) (4 + i % 12));
                setParam (p, "refFreq", 400.0f + (float) (i % 80));
                setParam (p, "instrument", (float) (i % 5));
                setParam (p, "tuningMode", (float) (i % 2));
                setParam (p, "edoSteps", (float) (1 + i % 96));
                // JUCE leak detection fires at teardown if a listener (or anything else) leaks.
            }
            expect (true); // reaching here without assertion/leak failure is the pass
        }
    }
};

static LifecycleTests lifecycleTests;
