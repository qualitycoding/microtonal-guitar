#pragma once

#include "JuceHeader.h"
#include "PluginProcessor.h"
#include "EdoControl.h"
#include "NoteNaming.h"
#include "Instruments.h"
#include <cstdint>
#include <unordered_map>

// --- Custom Dynamic Fretboard UI Component ---
class FretboardComponent : public juce::Component
{
public:
    FretboardComponent (MicroGuitarProcessor& p) : processor (p) {}

    void setShowAllNoteNames (bool shouldShow) { showAllNoteNames = shouldShow; repaint(); }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        g.setColour (juce::Colour (0xff1e1a17));
        g.fillRoundedRectangle (bounds, 4.0f);

        g.setColour (juce::Colours::dimgrey.darker());
        g.drawRoundedRectangle (bounds, 4.0f, 1.5f);

        // One lock, one copy: every draw below reads this snapshot rather than re-querying the
        // engine per field, so painting can't tear against a parameter change on the audio thread,
        // and can't contend with the audio thread's own lock for every fret label (plan §3.1, F6).
        const auto state = processor.getTuningEngine().getSnapshot();
        const int numStrings = state.numStrings;
        const int numFrets = processor.getFretCount();

        if (numStrings <= 0 || numFrets <= 0) return;

        auto activeNotes = processor.getActiveNotes();
        updateScrollTarget (numFrets, activeNotes);
        advanceScroll();

        const auto& tuning = state.currentTuning;
        refreshLabelCache (state);

        float width = bounds.getWidth();
        float height = bounds.getHeight();

        float nutX = 35.0f;
        float fretAreaWidth = width - nutX - 10.0f;

        double maxPos = relativeFretPosition (displayStartFret + numFrets, displayStartFret, tuning);
        if (maxPos <= 0.0) maxPos = 1.0;

        auto xForFret = [&] (int fret) -> float
        {
            double pos = relativeFretPosition (fret, displayStartFret, tuning);
            return nutX + (float) (pos / maxPos) * fretAreaWidth;
        };

        // 2. Draw Frets (vertical lines)
        if (displayStartFret == 0)
        {
            g.setColour (juce::Colours::lightgrey); // the real nut
            g.drawVerticalLine ((int) nutX, 0.0f, height);
        }
        else
        {
            g.setColour (juce::Colours::orange.withAlpha (0.35f));
            g.drawVerticalLine ((int) nutX, 0.0f, height);
        }

        g.setColour (juce::Colour (0x77888888));
        for (int f = displayStartFret + 1; f <= displayStartFret + numFrets; ++f)
            g.drawVerticalLine ((int) xForFret (f), 0.0f, height);

        // 3. Draw Strings
        float stringSpacing = height / (float) (numStrings + 1);
        for (int s = 0; s < numStrings; ++s)
        {
            float y = stringSpacing * (float) (s + 1);
            float thickness = 1.0f + (float) (numStrings - 1 - s) * 0.4f;
            g.setColour (juce::Colours::silver.withAlpha (0.7f));
            g.drawLine (0.0f, y, width, y, thickness);
        }

        // 4. Draw Octave Markers - any EDO, not just 12/24/31 (plan §3.3)
        g.setColour (juce::Colours::white.withAlpha (0.15f));
        const int stepsPerOctave = juce::jmax (1, tuning.stepsPerOctave);

        for (int f = displayStartFret + 1; f <= displayStartFret + numFrets; ++f)
        {
            float xLow  = (f - 1 >= displayStartFret) ? xForFret (f - 1) : nutX;
            float xHigh = xForFret (f);
            float x = (xLow + xHigh) * 0.5f;

            if (f % stepsPerOctave == 0)
            {
                float centerY = height / 2.0f;
                g.fillEllipse (x - 3.0f, centerY - 10.0f, 6.0f, 6.0f);
                g.fillEllipse (x - 3.0f, centerY + 4.0f, 6.0f, 6.0f);
            }
            else if (stepsPerOctave == 12 && (f % 12 == 3 || f % 12 == 5 || f % 12 == 7 || f % 12 == 9))
            {
                g.fillEllipse (x - 3.0f, (height / 2.0f) - 3.0f, 6.0f, 6.0f);
            }
        }

        // 5. All-frets note-name labels (only when there's room; culled per plan §3.3 / test L2)
        if (showAllNoteNames)
        {
            g.setFont (juce::Font (juce::FontOptions (10.0f)));
            g.setColour (juce::Colours::lightgrey.withAlpha (0.55f));

            for (int s = 0; s < numStrings; ++s)
            {
                const float y = stringSpacing * (float) (s + 1);
                for (int f = displayStartFret; f <= displayStartFret + numFrets; ++f)
                {
                    if (f == displayStartFret && displayStartFret != 0) continue; // off-screen anchor only

                    const float xLow  = (f - 1 >= displayStartFret) ? xForFret (f - 1) : nutX;
                    const float xHigh = (f == displayStartFret) ? nutX : xForFret (f);
                    const float slotWidth = (f == displayStartFret) ? nutX : (xHigh - xLow);
                    const float x = (f == displayStartFret) ? nutX * 0.5f : (xLow + xHigh) * 0.5f;

                    const auto& text = labelFor (s, f);
                    const float textWidth = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), text);

                    if (mtg::shouldDrawFretLabel (slotWidth, textWidth))
                        g.drawText (text, juce::Rectangle<float> (x - slotWidth * 0.5f, y - 12.0f,
                                                                    slotWidth, 11.0f),
                                    juce::Justification::centred, false);
                }
            }
        }

        // 6. Highlight Active Notes, with their name inside the dot
        float avgSpacing = fretAreaWidth / (float) numFrets;

        for (const auto& note : activeNotes)
        {
            if (note.stringIndex < 0 || note.stringIndex >= numStrings || note.fretNumber < 0)
                continue;

            float y = stringSpacing * (float) (note.stringIndex + 1);

            float x;
            if (displayStartFret == 0 && note.fretNumber == 0)
                x = nutX / 2.0f;
            else
            {
                int clampedFret = juce::jlimit (displayStartFret, displayStartFret + numFrets, note.fretNumber);
                float xLow  = (clampedFret - 1 >= displayStartFret) ? xForFret (clampedFret - 1) : nutX;
                float xHigh = xForFret (clampedFret);
                x = (xLow + xHigh) * 0.5f;
            }

            float radius = juce::jmin (avgSpacing, stringSpacing) * 0.42f;
            radius = juce::jlimit (5.0f, 12.0f, radius);

            g.setColour (juce::Colours::gold);
            g.fillEllipse (x - radius, y - radius, radius * 2.0f, radius * 2.0f);

            g.setColour (juce::Colours::orange.withAlpha (0.5f));
            g.drawEllipse (x - radius - 2.0f, y - radius - 2.0f, (radius + 2.0f) * 2.0f, (radius + 2.0f) * 2.0f, 1.5f);

            const auto& name = labelFor (note.stringIndex, note.fretNumber);
            g.setColour (juce::Colours::black);
            const float fontSize = juce::jmin (radius * 0.85f, 11.0f);
            g.setFont (juce::Font (juce::FontOptions (fontSize)).boldened());
            const float textWidth = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), name);

            if (textWidth <= radius * 2.0f - 2.0f)
            {
                g.drawText (name, juce::Rectangle<float> (x - radius, y - fontSize * 0.5f, radius * 2.0f, fontSize),
                            juce::Justification::centred, false);
            }
            else
            {
                // Doesn't fit inside the dot (long ups-and-downs spellings, e.g. "vvB1") - draw it
                // just above instead of truncating a name the player needs to read correctly.
                g.setColour (juce::Colours::white);
                g.drawText (name, juce::Rectangle<float> (x - 30.0f, y - radius - fontSize - 1.0f, 60.0f, fontSize),
                            juce::Justification::centred, false);
            }
        }
    }

private:
    static double fretRatio (int fretNumber, const TuningSystem& tuning)
    {
        if (tuning.isEqualTemperament)
            return std::pow (2.0, (double) fretNumber / (double) tuning.stepsPerOctave);

        int n = (int) tuning.customRatios.size();
        if (n == 0) return 1.0;

        int octaves = fretNumber / n;
        int degree  = fretNumber % n;
        if (degree < 0) { degree += n; octaves -= 1; }
        return tuning.customRatios[(size_t) degree] * std::pow (2.0, octaves);
    }

    static double relativeFretPosition (int fretNumber, int startFret, const TuningSystem& tuning)
    {
        double refRatio   = fretRatio (startFret, tuning);
        double localRatio = fretRatio (fretNumber, tuning) / refRatio;
        if (localRatio <= 0.0) return 0.0;
        return 1.0 - (1.0 / localRatio);
    }

    void updateScrollTarget (int numFrets, const std::vector<MicroGuitarProcessor::ActiveNote>& activeNotes)
    {
        const int edgeMargin = 1;
        int minActive = -1, maxActive = -1;

        for (auto& note : activeNotes)
        {
            if (note.fretNumber < 0) continue;
            if (minActive < 0 || note.fretNumber < minActive) minActive = note.fretNumber;
            if (note.fretNumber > maxActive) maxActive = note.fretNumber;
        }

        if (minActive < 0) return;

        if (minActive < targetStartFret + edgeMargin)
            targetStartFret = juce::jmax (0, minActive - edgeMargin);
        else if (maxActive > targetStartFret + numFrets - edgeMargin)
            targetStartFret = juce::jmax (0, maxActive - numFrets + edgeMargin);
    }

    void advanceScroll()
    {
        if (displayStartFret != targetStartFret)
        {
            int diff = targetStartFret - displayStartFret;
            int step = diff > 0 ? juce::jmax (1, diff / 4) : juce::jmin (-1, diff / 4);
            displayStartFret += step;
        }
    }

    // A fret's label depends only on the tuning system, the reference frequency and which open
    // string it's on - not on which fret is currently scrolled into view - so it's cached once per
    // configuration change instead of recomputed every paint (plan §3.3 / premortem F6, F14).
    struct CacheKey
    {
        int numStrings = -1, stepsPerOctave = -1;
        bool isEqualTemperament = true;
        double referenceFrequency = 0.0;
        std::vector<int> openStringMidiNotes;

        bool operator!= (const CacheKey& other) const
        {
            return numStrings != other.numStrings || stepsPerOctave != other.stepsPerOctave
                || isEqualTemperament != other.isEqualTemperament
                || std::abs (referenceFrequency - other.referenceFrequency) > 1.0e-9
                || openStringMidiNotes != other.openStringMidiNotes;
        }
    };

    void refreshLabelCache (const GuitarState& state)
    {
        CacheKey key { state.numStrings, state.currentTuning.stepsPerOctave,
                       state.currentTuning.isEqualTemperament, state.referenceFrequency,
                       state.openStringMidiNotes };
        if (key != cacheKey)
        {
            cacheKey = key;
            labelCache.clear();
        }
    }

    const juce::String& labelFor (int stringIndex, int fret)
    {
        const int64_t key = ((int64_t) stringIndex << 32) | (uint32_t) (fret + 4096);
        auto it = labelCache.find (key);
        if (it != labelCache.end()) return it->second;

        // The naming edo for Just Intonation is fixed at 12 - the ratios approximate familiar
        // 12-TET scale degrees, so the same spelling rule reads correctly there too (plan §3.1).
        const int namingEdo = cacheKey.isEqualTemperament ? juce::jmax (1, cacheKey.stepsPerOctave) : 12;
        const double openFreq = (stringIndex >= 0 && stringIndex < (int) cacheKey.openStringMidiNotes.size())
            ? cacheKey.referenceFrequency * std::pow (2.0, (cacheKey.openStringMidiNotes[(size_t) stringIndex] - 69) / 12.0)
            : cacheKey.referenceFrequency;

        double freq = openFreq;
        if (cacheKey.isEqualTemperament)
            freq = openFreq * std::pow (2.0, (double) fret / namingEdo);
        else if (! cacheKey.openStringMidiNotes.empty())
            freq = openFreq * std::pow (2.0, (double) fret / 12.0); // rough JI display fallback

        const auto pitch = mtg::nameFrequency (freq, cacheKey.referenceFrequency, namingEdo);
        auto text = juce::String::fromUTF8 (pitch.name.c_str()) + juce::String (pitch.octave);

        return labelCache.emplace (key, std::move (text)).first->second;
    }

    MicroGuitarProcessor& processor;
    int displayStartFret = 0;
    int targetStartFret = 0;
    bool showAllNoteNames = false;

    CacheKey cacheKey;
    std::unordered_map<int64_t, juce::String> labelCache;
};

// --- Main Editor Window ---
class MicroGuitarEditor : public juce::AudioProcessorEditor,
                          private juce::Timer
{
public:
    MicroGuitarEditor (MicroGuitarProcessor&);
    ~MicroGuitarEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateStringCountHint();

    MicroGuitarProcessor& audioProcessor;

    // --- Fretboard Component ---
    FretboardComponent fretboard;

    // --- Readout: what's sounding right now ---
    juce::Label readout;

    // --- UI Controls ---
    juce::ComboBox instrumentDropdown;
    juce::Label instrumentLabel;

    EdoControl edoControl;
    juce::Label tuningLabel;

    juce::ComboBox keyDropdown;
    juce::Label keyLabel;

    juce::Slider stringCountSlider;
    juce::Label stringCountLabel;
    juce::Label stringCountHint; // "max N for <instrument>" (plan O15)

    juce::Slider refFreqSlider;
    juce::Label refFreqLabel;

    juce::ToggleButton showAllNamesToggle { "Show all note names" };

    // --- APVTS Attachments ---
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> instrumentAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> keyAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> stringCountAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> refFreqAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MicroGuitarEditor)
};
