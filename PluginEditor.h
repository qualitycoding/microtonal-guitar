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
        
        // 1. Background - Dark rosewood/ebony wood color
        g.setColour(juce::Colour(0xff1e1a17)); 
        g.fillRoundedRectangle(bounds, 4.0f);
        
        g.setColour(juce::Colours::dimgrey.darker());
        g.drawRoundedRectangle(bounds, 4.0f, 1.5f);

        int numStrings = (int)processor.apvts.getRawParameterValue("stringCount")->load();
        int numFrets = processor.getFretCount();
        
        if (numStrings <= 0 || numFrets <= 0) return;

        float width = bounds.getWidth();
        float height = bounds.getHeight();
        
        // Left margin to represent open-string notes before the nut
        float nutX = 35.0f;
        float fretAreaWidth = width - nutX - 10.0f;
        float fretWidth = fretAreaWidth / (float)numFrets;

        // 2. Draw Frets (vertical lines)
        g.setColour(juce::Colours::lightgrey); // Thick Nut line
        g.drawVerticalLine((int)nutX, 0.0f, height);
        
        g.setColour(juce::Colour(0x77888888)); // Standard silver frets
        for (int f = 1; f <= numFrets; ++f)
        {
            float x = nutX + (float)f * fretWidth;
            g.drawVerticalLine((int)x, 0.0f, height);
        }

        // 3. Draw Strings (horizontal lines with dynamic thicknesses!)
        float stringSpacing = height / (float)(numStrings + 1);
        
        for (int s = 0; s < numStrings; ++s)
        {
            float y = stringSpacing * (float)(s + 1);
            
            // Render graduated thickness (lower strings are slightly thicker)
            float thickness = 1.0f + (float)(numStrings - 1 - s) * 0.4f;
            
            g.setColour(juce::Colours::silver.withAlpha(0.7f));
            // FIXED: Using g.drawLine now correctly utilizes the thickness variable!
            g.drawLine (0.0f, y, width, y, thickness); 
        }

        // 4. Draw Octave & Inlay Markers
        g.setColour(juce::Colours::white.withAlpha(0.15f));
        // FIXED: Added explicit (int) cast to resolve warning C4244
        int tuningChoice = (int)processor.apvts.getRawParameterValue("tuningType")->load();
        
        int stepsPerOctave = 12;
        if (tuningChoice == 1) stepsPerOctave = 24;
        else if (tuningChoice == 2) stepsPerOctave = 31;
        
        for (int f = 1; f <= numFrets; ++f)
        {
            float x = nutX + ((float)f - 0.5f) * fretWidth;
            
            if (f % stepsPerOctave == 0) // Octave marker (Double dot)
            {
                float centerY = height / 2.0f;
                g.fillEllipse(x - 3.0f, centerY - 10.0f, 6.0f, 6.0f);
                g.fillEllipse(x - 3.0f, centerY + 4.0f, 6.0f, 6.0f);
            }
            else if (stepsPerOctave == 12 && (f == 3 || f == 5 || f == 7 || f == 9 || f == 15 || f == 17 || f == 19 || f == 21))
            {
                g.fillEllipse(x - 3.0f, (height / 2.0f) - 3.0f, 6.0f, 6.0f);
            }
        }

        // 5. Highlight Active Notes
        auto activeNotes = processor.getActiveNotes();
        for (const auto& note : activeNotes)
        {
            if (note.stringIndex >= 0 && note.stringIndex < numStrings && note.fretNumber >= 0 && note.fretNumber <= numFrets)
            {
                float y = stringSpacing * (float)(note.stringIndex + 1);
                float x = (note.fretNumber == 0) ? (nutX / 2.0f) : (nutX + ((float)note.fretNumber - 0.5f) * fretWidth);

                float radius = juce::jmin(fretWidth, stringSpacing) * 0.42f;
                radius = juce::jlimit(5.0f, 12.0f, radius);

                g.setColour(juce::Colours::gold);
                g.fillEllipse(x - radius, y - radius, radius * 2.0f, radius * 2.0f);
                
                g.setColour(juce::Colours::orange.withAlpha(0.5f));
                g.drawEllipse(x - radius - 2.0f, y - radius - 2.0f, (radius + 2.0f) * 2.0f, (radius + 2.0f) * 2.0f, 1.5f);
            }
        }
    }

private:
    MicroGuitarProcessor& processor;
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