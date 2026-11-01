// SPDX-License-Identifier: Apache-2.0
// Issue #5 integration tests (I-B1) and issue #7 audible refFreq verification (I-A4).
// Added by the issue-remediation plan (S-020); not part of the r4 freeze. Runs headless via
// the MicroGuitarIntegrationTests console target; renders real audio through MicroGuitarProcessor.
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

// I-B1 needs white-box access to the playing voice: shrinking the string count while a note is
// held makes the engine reject the note's string (getFrequency() -> 0.0), and the processor's
// pitch-wheel routing only addresses strings < currentNumStrings, so the invalid-state
// updateFrequency() call cannot be triggered through MIDI alone. JUCE headers are included
// above (before the macro) so only first-party headers are affected.
#define private public
#include "PluginProcessor.h"
#include "MicrotonalVoice.h"
#undef private

#include <cmath>
#include <vector>

namespace
{
    void setParam (MicroGuitarProcessor& p, const juce::String& id, float plainValue)
    {
        auto* param = p.apvts.getParameter (id);
        jassert (param != nullptr);
        param->setValueNotifyingHost (param->convertTo0to1 (plainValue));
    }

    void noteOn (juce::MidiBuffer& midi, int note, int samplePos = 0)
    {
        midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), samplePos);
    }

    void noteOff (juce::MidiBuffer& midi, int note, int samplePos = 0)
    {
        midi.addEvent (juce::MidiMessage::noteOff (1, note), samplePos);
    }

    void pitchBend (juce::MidiBuffer& midi, int value, int samplePos = 0)
    {
        midi.addEvent (juce::MidiMessage::pitchWheel (1, value), samplePos);
    }

    // Render `numBlocks` blocks; appends each block's samples to `out`. Returns false as soon
    // as any non-finite sample is produced.
    bool renderBlocks (MicroGuitarProcessor& p, juce::AudioBuffer<float>& block,
                       juce::MidiBuffer& midi, int numBlocks, std::vector<float>& out)
    {
        for (int b = 0; b < numBlocks; ++b)
        {
            block.clear();
            p.processBlock (block, midi);
            midi.clear();
            for (int s = 0; s < block.getNumSamples(); ++s)
            {
                const float v = block.getSample (0, s);
                if (! std::isfinite (v)) return false;
                out.push_back (v);
            }
        }
        return true;
    }

    float maxMagnitude (const std::vector<float>& v)
    {
        float m = 0.0f;
        for (float x : v) m = std::max (m, std::fabs (x));
        return m;
    }

    // Autocorrelation pitch estimate over the tail of the rendered signal.
    double estimatePitch (const std::vector<float>& signal, double sampleRate, int blockSize)
    {
        const size_t usable = signal.size() > (size_t) blockSize * 8
                                  ? signal.size() - (size_t) blockSize * 4 : 0;
        if (usable < (size_t) sampleRate / 2) return 0.0; // need at least ~0.5s

        const size_t start = (size_t) blockSize * 4;
        const int minLag = (int) (sampleRate / 500.0);  // up to 500 Hz
        const int maxLag = (int) (sampleRate / 40.0);   // down to 40 Hz

        double best = 0.0;
        int bestLag = 0;
        for (int lag = minLag; lag <= maxLag; ++lag)
        {
            double r = 0.0;
            for (size_t i = start; i + (size_t) lag < signal.size(); ++i)
                r += (double) signal[i] * signal[i + (size_t) lag];
            if (r > best) { best = r; bestLag = lag; }
        }
        return bestLag > 0 ? sampleRate / (double) bestLag : 0.0;
    }
}

class VoiceGuardAndPitchTests : public juce::UnitTest
{
public:
    VoiceGuardAndPitchTests() : juce::UnitTest ("VoiceGuardAndPitchTests", "MicroGuitar") {}

    void runTest() override
    {
        beginTest ("I-B1 held note survives string-count shrink (issue #5: finite output, recovery)");
        {
            MicroGuitarProcessor p;
            setParam (p, "stringCount", 10.0f);
            p.prepareToPlay (48000.0, 512);

            const int topOpenNote = p.getTuningEngine().getOpenStringMidiNote (9);
            expect (topOpenNote >= 0);

            juce::AudioBuffer<float> block (2, 512);
            juce::MidiBuffer midi;
            std::vector<float> rendered;

            // Start a note on the highest string and let it ring.
            noteOn (midi, topOpenNote);
            expect (renderBlocks (p, block, midi, 8, rendered));
            expect (maxMagnitude (rendered) > 0.0f); // audible pluck

            MicrotonalVoice* heldVoice = nullptr;
            for (int i = 0; i < p.mpeSynth.getNumVoices(); ++i)
                if (auto* v = dynamic_cast<MicrotonalVoice*> (p.mpeSynth.getVoice (i)))
                    if (v->isActive() && v->getStringIndex() == 9)
                        heldVoice = v;
            expect (heldVoice != nullptr);

            // Shrink the instrument to 4 strings while the note is held: the engine now has
            // no string 9, so the next updateFrequency() computes targetFreq == 0.0. (The
            // processor's pitch-bend routing cannot deliver this — it only addresses strings
            // below currentNumStrings — hence the white-box call; see file header.)
            rendered.clear();
            setParam (p, "stringCount", 4.0f);
            heldVoice->updateFrequency();
            expect (renderBlocks (p, block, midi, 8, rendered));
            for (float v : rendered) expect (std::isfinite (v)); // no inf/NaN may reach the output

            // Recovery: a later valid configuration must fully re-initialise the voice.
            rendered.clear();
            setParam (p, "stringCount", 10.0f);
            heldVoice->updateFrequency();
            expect (renderBlocks (p, block, midi, 8, rendered));
            expect (maxMagnitude (rendered) > 0.0f); // the string sounds again

            noteOff (midi, topOpenNote);
            expect (renderBlocks (p, block, midi, 4, rendered));
        }

        beginTest ("I-A4 refFreq changes ACTUAL output pitch by 432/440 (issue #7)");
        {
            constexpr double sampleRate = 48000.0;
            constexpr int blockSize = 512;
            constexpr int blocks = 192; // 2 seconds

            auto renderNote = [&] (float refFreq, double& pitchOut)
            {
                MicroGuitarProcessor p;
                setParam (p, "refFreq", refFreq);
                p.prepareToPlay (sampleRate, blockSize);

                juce::AudioBuffer<float> block (2, blockSize);
                juce::MidiBuffer midi;
                std::vector<float> rendered;

                noteOn (midi, 45); // A2, open A string on the default guitar
                expect (renderBlocks (p, block, midi, blocks, rendered));
                pitchOut = estimatePitch (rendered, sampleRate, blockSize);
                expect (pitchOut > 0.0);
            };

            double pitch440 = 0.0, pitch432 = 0.0;
            renderNote (440.0f, pitch440);
            renderNote (432.0f, pitch432);

            expectWithinAbsoluteError (pitch432 / pitch440, 432.0 / 440.0, 0.01);
        }
    }
};

static VoiceGuardAndPitchTests voiceGuardAndPitchTests;
