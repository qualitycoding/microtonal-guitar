#include "PluginProcessor.h"
#include "PluginEditor.h"

MicroGuitarEditor::MicroGuitarEditor (MicroGuitarProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // --- 1. Setup the Tuning Dropdown ---
    addAndMakeVisible(tuningDropdown);
    tuningLabel.setText("Microtonal Scale:", juce::dontSendNotification);
    tuningLabel.attachToComponent(&tuningDropdown, false);
    addAndMakeVisible(tuningLabel);

    // The options match what we defined in PluginProcessor::createParameterLayout
    tuningDropdown.addItem("12-TET", 1);
    tuningDropdown.addItem("24-TET", 2);
    tuningDropdown.addItem("31-EDO", 3);
    tuningDropdown.addItem("5-Limit Just", 4);

    // Link the dropdown to the APVTS backend
    tuningAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, "tuningType", tuningDropdown);

    // --- 2. Setup String Count Slider ---
    stringCountSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    stringCountSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
    addAndMakeVisible(stringCountSlider);
    
    stringCountLabel.setText("Strings:", juce::dontSendNotification);
    stringCountLabel.attachToComponent(&stringCountSlider, true);
    addAndMakeVisible(stringCountLabel);

    stringCountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "stringCount", stringCountSlider);

    // --- 3. Setup Reference Frequency Slider ---
    refFreqSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    refFreqSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
    addAndMakeVisible(refFreqSlider);

    refFreqLabel.setText("A4 Ref (Hz):", juce::dontSendNotification);
    refFreqLabel.attachToComponent(&refFreqSlider, true);
    addAndMakeVisible(refFreqLabel);

    refFreqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "refFreq", refFreqSlider);

    // Set the overall window size
    setSize (400, 300);
}

MicroGuitarEditor::~MicroGuitarEditor()
{
}

void MicroGuitarEditor::paint (juce::Graphics& g)
{
    // Fill the background with a dark grey
    g.fillAll (juce::Colour (0xff222222));

    // Draw a title at the top
    g.setColour (juce::Colours::white);
    g.setFont (24.0f);
    g.drawFittedText ("Microtonal MPE Guitar", getLocalBounds().removeFromTop(50), juce::Justification::centred, 1);
}

void MicroGuitarEditor::resized()
{
    // This function is called when the window is created or resized.
    // We use juce::Rectangle to easily stack our components downwards.
    
    auto bounds = getLocalBounds().reduced(20);
    bounds.removeFromTop(40); // Leave room for the title and labels

    int itemHeight = 30;
    int spacing = 30;

    // Layout the components row by row
    tuningDropdown.setBounds(bounds.removeFromTop(itemHeight));
    bounds.removeFromTop(spacing);

    // Nudge the bounds slightly to the right to make room for side-labels
    auto sliderBounds = bounds.withTrimmedLeft(80); 
    
    stringCountSlider.setBounds(sliderBounds.removeFromTop(itemHeight));
    sliderBounds.removeFromTop(spacing);
    
    refFreqSlider.setBounds(sliderBounds.removeFromTop(itemHeight));
}
