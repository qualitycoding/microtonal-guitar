#include "PluginProcessor.h"
#include "PluginEditor.h"

MicroGuitarEditor::MicroGuitarEditor (MicroGuitarProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), fretboard (p)
{
    addAndMakeVisible(fretboard);

    // --- 1. Setup Tuning Dropdown ---
    addAndMakeVisible(tuningDropdown);
    tuningLabel.setText("Microtonal Scale:", juce::dontSendNotification);
    tuningLabel.attachToComponent(&tuningDropdown, false);
    addAndMakeVisible(tuningLabel);

    tuningDropdown.addItem("12-TET", 1);
    tuningDropdown.addItem("24-TET", 2);
    tuningDropdown.addItem("31-EDO", 3);
    tuningDropdown.addItem("5-Limit Just", 4);

    tuningAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, "tuningType", tuningDropdown);

    // --- 2. ADDED: Setup Key Dropdown ---
    addAndMakeVisible(keyDropdown);
    keyLabel.setText("Musical Key:", juce::dontSendNotification);
    keyLabel.attachToComponent(&keyDropdown, false);
    addAndMakeVisible(keyLabel);

    juce::StringArray keys = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    for (int i = 0; i < keys.size(); ++i)
        keyDropdown.addItem(keys[i], i + 1);

    keyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, "musicalKey", keyDropdown);

    // --- 3. Setup String Count Slider ---
    stringCountSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    stringCountSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
    addAndMakeVisible(stringCountSlider);
    
    stringCountLabel.setText("Strings:", juce::dontSendNotification);
    stringCountLabel.attachToComponent(&stringCountSlider, false);
    addAndMakeVisible(stringCountLabel);

    stringCountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "stringCount", stringCountSlider);

    // --- 4. Setup Reference Frequency Slider ---
    refFreqSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    refFreqSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
    addAndMakeVisible(refFreqSlider);

    refFreqLabel.setText("A4 Ref (Hz):", juce::dontSendNotification);
    refFreqLabel.attachToComponent(&refFreqSlider, false);
    addAndMakeVisible(refFreqLabel);

    refFreqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "refFreq", refFreqSlider);

    setSize (720, 380);
    startTimerHz(30); 
}

MicroGuitarEditor::~MicroGuitarEditor()
{
}

void MicroGuitarEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff222222));

    g.setColour (juce::Colours::white);
    g.setFont (24.0f);
    g.drawFittedText ("Microtonal MPE Guitar", getLocalBounds().removeFromTop(45), juce::Justification::centred, 1);
}

void MicroGuitarEditor::resized()
{
    auto bounds = getLocalBounds().reduced(20);
    bounds.removeFromTop(45); // Leave space for main title

    // Allocate bottom space for the Fretboard component
    auto fretboardBounds = bounds.removeFromBottom(160);
    fretboard.setBounds(fretboardBounds);

    bounds.removeFromBottom(15); // Gap space between knobs and fretboard

    int itemHeight = 30;
    int spacing = 15;

    // Divide remaining top width across four side-by-side columns
    int columnWidth = bounds.getWidth() / 4 - 10;
    auto controlArea = bounds;

    // Column 1: Scale Choice
    auto col1 = controlArea.removeFromLeft(columnWidth);
    tuningDropdown.setBounds(col1.removeFromBottom(itemHeight));
    
    controlArea.removeFromLeft(spacing);

    // Column 2: Key Choice (ADDED)
    auto col2 = controlArea.removeFromLeft(columnWidth);
    keyDropdown.setBounds(col2.removeFromBottom(itemHeight));
    
    controlArea.removeFromLeft(spacing);

    // Column 3: Strings Slider
    auto col3 = controlArea.removeFromLeft(columnWidth);
    stringCountSlider.setBounds(col3.removeFromBottom(itemHeight));
    
    controlArea.removeFromLeft(spacing);

    // Column 4: Reference Pitch Slider
    auto col4 = controlArea;
    refFreqSlider.setBounds(col4.removeFromBottom(itemHeight));
}