#pragma once

#include "JuceHeader.h"
#include "PluginProcessor.h"

// --- Custom Dynamic Fretboard UI Component ---
class FretboardComponent : public juce::Component
{
public:
    FretboardComponent(MicroGuitarProcessor& p) : processor(p) {}

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        g.setColour(juce::Colour(0xff1e1a17));
        g.fillRoundedRectangle(bounds, 4.0f);

        g.setColour(juce::Colours::dimgrey.darker());
        g.drawRoundedRectangle(bounds, 4.0f, 1.5f);

        int numStrings = (int)processor.apvts.getRawParameterValue("stringCount")->load();
        int numFrets = processor.getFretCount(); // viewport size

        if (numStrings <= 0 || numFrets <= 0) return;

        auto activeNotes = processor.getActiveNotes();
        updateScrollTarget(numFrets, activeNotes);
        advanceScroll();

        const auto tuning = processor.getTuningEngine().getCurrentTuningSystem();

        float width = bounds.getWidth();
        float height = bounds.getHeight();

        float nutX = 35.0f;
        float fretAreaWidth = width - nutX - 10.0f;

        // Position of the fret just past the visible window - normalizes so
        // the window always fills the available width exactly, wherever
        // it's currently scrolled to.
        double maxPos = relativeFretPosition(displayStartFret + numFrets, displayStartFret, tuning);
        if (maxPos <= 0.0) maxPos = 1.0;

        auto xForFret = [&] (int fret) -> float
        {
            double pos = relativeFretPosition(fret, displayStartFret, tuning);
            return nutX + (float)(pos / maxPos) * fretAreaWidth;
        };

        // 2. Draw Frets (vertical lines)
        if (displayStartFret == 0)
        {
            g.setColour(juce::Colours::lightgrey); // the real nut
            g.drawVerticalLine((int)nutX, 0.0f, height);
        }
        else
        {
            // Scrolled away from the open strings - mark it rather than
            // drawing a nut line that isn't really there
            g.setColour(juce::Colours::orange.withAlpha(0.35f));
            g.drawVerticalLine((int)nutX, 0.0f, height);
        }

        g.setColour(juce::Colour(0x77888888));
        for (int f = displayStartFret + 1; f <= displayStartFret + numFrets; ++f)
            g.drawVerticalLine((int)xForFret(f), 0.0f, height);

        // 3. Draw Strings
        float stringSpacing = height / (float)(numStrings + 1);
        for (int s = 0; s < numStrings; ++s)
        {
            float y = stringSpacing * (float)(s + 1);
            float thickness = 1.0f + (float)(numStrings - 1 - s) * 0.4f;
            g.setColour(juce::Colours::silver.withAlpha(0.7f));
            g.drawLine(0.0f, y, width, y, thickness);
        }

        // 4. Draw Octave & Inlay Markers
        g.setColour(juce::Colours::white.withAlpha(0.15f));
        int tuningChoice = (int)processor.apvts.getRawParameterValue("tuningType")->load();

        int stepsPerOctave = 12;
        if (tuningChoice == 1) stepsPerOctave = 24;
        else if (tuningChoice == 2) stepsPerOctave = 31;

        for (int f = displayStartFret + 1; f <= displayStartFret + numFrets; ++f)
        {
            // Dots sit midway between the two fret wires either side. With
            // non-uniform spacing that midpoint has to be computed from the
            // actual screen positions rather than a fixed offset.
            float xLow  = (f - 1 >= displayStartFret) ? xForFret(f - 1) : nutX;
            float xHigh = xForFret(f);
            float x = (xLow + xHigh) * 0.5f;

            if (f % stepsPerOctave == 0)
            {
                float centerY = height / 2.0f;
                g.fillEllipse(x - 3.0f, centerY - 10.0f, 6.0f, 6.0f);
                g.fillEllipse(x - 3.0f, centerY + 4.0f, 6.0f, 6.0f);
            }
            else if (stepsPerOctave == 12 && (f % 12 == 3 || f % 12 == 5 || f % 12 == 7 || f % 12 == 9))
            {
                g.fillEllipse(x - 3.0f, (height / 2.0f) - 3.0f, 6.0f, 6.0f);
            }
        }

        // 5. Highlight Active Notes
        float avgSpacing = fretAreaWidth / (float) numFrets; // rough estimate, used only for dot sizing

        for (const auto& note : activeNotes)
        {
            if (note.stringIndex < 0 || note.stringIndex >= numStrings || note.fretNumber < 0)
                continue;

            float y = stringSpacing * (float)(note.stringIndex + 1);

            float x;
            if (displayStartFret == 0 && note.fretNumber == 0)
            {
                x = nutX / 2.0f; // genuine open string, left of the nut
            }
            else
            {
                int clampedFret = juce::jlimit(displayStartFret, displayStartFret + numFrets, note.fretNumber);
                float xLow  = (clampedFret - 1 >= displayStartFret) ? xForFret(clampedFret - 1) : nutX;
                float xHigh = xForFret(clampedFret);
                x = (xLow + xHigh) * 0.5f;
            }

            float radius = juce::jmin(avgSpacing, stringSpacing) * 0.42f;
            radius = juce::jlimit(5.0f, 12.0f, radius);

            g.setColour(juce::Colours::gold);
            g.fillEllipse(x - radius, y - radius, radius * 2.0f, radius * 2.0f);

            g.setColour(juce::Colours::orange.withAlpha(0.5f));
            g.drawEllipse(x - radius - 2.0f, y - radius - 2.0f, (radius + 2.0f) * 2.0f, (radius + 2.0f) * 2.0f, 1.5f);
        }
    }

private:
    static double fretRatio(int fretNumber, const TuningSystem& tuning)
    {
        if (tuning.isEqualTemperament)
            return std::pow(2.0, (double) fretNumber / (double) tuning.stepsPerOctave);

        int n = (int) tuning.customRatios.size();
        if (n == 0) return 1.0;

        int octaves = fretNumber / n;
        int degree  = fretNumber % n;
        return tuning.customRatios[(size_t) degree] * std::pow(2.0, octaves);
    }

    // A fret's position as a fraction of scale length, re-anchored so that
    // `startFret` is the "virtual nut" (position 0). This is what lets the
    // physically-accurate spacing keep working once the view has scrolled
    // away from the real open string.
    static double relativeFretPosition(int fretNumber, int startFret, const TuningSystem& tuning)
    {
        double refRatio   = fretRatio(startFret, tuning);
        double localRatio = fretRatio(fretNumber, tuning) / refRatio;
        if (localRatio <= 0.0) return 0.0;
        return 1.0 - (1.0 / localRatio);
    }

    void updateScrollTarget(int numFrets, const std::vector<MicroGuitarProcessor::ActiveNote>& activeNotes)
    {
        const int edgeMargin = 1;
        int minActive = -1, maxActive = -1;

        for (auto& note : activeNotes)
        {
            if (note.fretNumber < 0) continue;
            if (minActive < 0 || note.fretNumber < minActive) minActive = note.fretNumber;
            if (note.fretNumber > maxActive) maxActive = note.fretNumber;
        }

        if (minActive < 0) return; // nothing sounding - leave the view where it is

        if (minActive < targetStartFret + edgeMargin)
            targetStartFret = juce::jmax(0, minActive - edgeMargin);
        else if (maxActive > targetStartFret + numFrets - edgeMargin)
            targetStartFret = juce::jmax(0, maxActive - numFrets + edgeMargin);
    }

    void advanceScroll()
    {
        if (displayStartFret != targetStartFret)
        {
            int diff = targetStartFret - displayStartFret;
            int step = diff > 0 ? juce::jmax(1, diff / 4) : juce::jmin(-1, diff / 4);
            displayStartFret += step;
        }
    }

    MicroGuitarProcessor& processor;
    int displayStartFret = 0;
    int targetStartFret = 0;
};

// --- Main Editor Window ---
class MicroGuitarEditor : public juce::AudioProcessorEditor,
                          private juce::Timer // Inherited to run UI repaints [3]
{
public:
    MicroGuitarEditor (MicroGuitarProcessor&);
    ~MicroGuitarEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override { repaint(); } // Timer updates visual state [3]

    MicroGuitarProcessor& audioProcessor;

    // --- Fretboard Component ---
    FretboardComponent fretboard;

    // --- UI Controls ---
    juce::ComboBox tuningDropdown;
    juce::Label tuningLabel;

    juce::ComboBox keyDropdown;
    juce::Label keyLabel;

    juce::Slider stringCountSlider;
    juce::Label stringCountLabel;

    juce::Slider refFreqSlider;
    juce::Label refFreqLabel;

    // --- APVTS Attachments ---
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> tuningAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> keyAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> stringCountAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> refFreqAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MicroGuitarEditor)
};