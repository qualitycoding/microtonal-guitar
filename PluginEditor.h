#pragma once

#include "JuceHeader.h"
#include "PluginProcessor.h"

class MicroGuitarEditor : public juce::AudioProcessorEditor
{
public:
    MicroGuitarEditor (MicroGuitarProcessor&);
    ~MicroGuitarEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    MicroGuitarProcessor& audioProcessor;

    // --- UI Components ---
    juce::ComboBox tuningDropdown;
    juce::Label tuningLabel;

    juce::Slider stringCountSlider;
    juce::Label stringCountLabel;

    juce::Slider refFreqSlider;
    juce::Label refFreqLabel;

    // --- APVTS Attachments ---
    // These automatically keep the UI and the backend parameters in sync
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> tuningAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> stringCountAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> refFreqAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MicroGuitarEditor)
};
