// SPDX-License-Identifier: Apache-2.0
#include "PluginProcessor.h"
#include "PluginEditor.h"

MicroGuitarEditor::MicroGuitarEditor (MicroGuitarProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), fretboard (p), edoControl (p)
{
    addAndMakeVisible (fretboard);

    // --- Readout ---
    readout.setJustificationType (juce::Justification::centredLeft);
    readout.setFont (juce::Font (juce::FontOptions (15.0f)));
    readout.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible (readout);

    // --- 1. Instrument dropdown ---
    addAndMakeVisible (instrumentDropdown);
    instrumentLabel.setText ("Instrument:", juce::dontSendNotification);
    instrumentLabel.attachToComponent (&instrumentDropdown, false);
    addAndMakeVisible (instrumentLabel);

    for (int i = 0; i < mtg::kNumInstruments; ++i)
        instrumentDropdown.addItem (mtg::instrumentInfo (static_cast<mtg::Instrument> (i)).name, i + 1);

    instrumentAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        audioProcessor.apvts, "instrument", instrumentDropdown);
    instrumentDropdown.onChange = [this] { updateStringCountHint(); };

    // --- 2. Tuning control (free entry + presets; see EdoControl.h) ---
    addAndMakeVisible (edoControl);
    tuningLabel.setText ("Tuning (steps/octave):", juce::dontSendNotification);
    tuningLabel.attachToComponent (&edoControl, false);
    addAndMakeVisible (tuningLabel);

    // --- 3. Musical Key dropdown (unchanged; pre-existing wiring gap - backlog B2) ---
    addAndMakeVisible (keyDropdown);
    keyLabel.setText ("Musical Key:", juce::dontSendNotification);
    keyLabel.attachToComponent (&keyDropdown, false);
    addAndMakeVisible (keyLabel);

    juce::StringArray keys = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    for (int i = 0; i < keys.size(); ++i)
        keyDropdown.addItem (keys[i], i + 1);

    keyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        audioProcessor.apvts, "musicalKey", keyDropdown);

    // --- 4. String Count Slider ---
    stringCountSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    stringCountSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 50, 20);
    addAndMakeVisible (stringCountSlider);

    stringCountLabel.setText ("Strings:", juce::dontSendNotification);
    stringCountLabel.attachToComponent (&stringCountSlider, false);
    addAndMakeVisible (stringCountLabel);

    stringCountHint.setFont (juce::Font (juce::FontOptions (11.0f)));
    stringCountHint.setColour (juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible (stringCountHint);

    stringCountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "stringCount", stringCountSlider);
    updateStringCountHint();

    // --- 5. Reference Frequency Slider ---
    refFreqSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    refFreqSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 50, 20);
    addAndMakeVisible (refFreqSlider);

    refFreqLabel.setText ("A4 Ref (Hz):", juce::dontSendNotification);
    refFreqLabel.attachToComponent (&refFreqSlider, false);
    addAndMakeVisible (refFreqLabel);

    refFreqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "refFreq", refFreqSlider);

    // --- 6. All-frets toggle ---
    showAllNamesToggle.setColour (juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    showAllNamesToggle.onClick = [this] { fretboard.setShowAllNoteNames (showAllNamesToggle.getToggleState()); };
    addAndMakeVisible (showAllNamesToggle);

    setSize (760, 420);
    startTimerHz (30);
}

MicroGuitarEditor::~MicroGuitarEditor()
{
}

void MicroGuitarEditor::timerCallback()
{
    repaint();

    const auto notes = audioProcessor.getActiveNotes();
    if (notes.empty())
    {
        readout.setText ({}, juce::dontSendNotification);
        return;
    }

    const auto state = audioProcessor.getTuningEngine().getSnapshot();
    const auto& tuning = state.currentTuning;
    const int namingEdo = tuning.isEqualTemperament ? juce::jmax (1, tuning.stepsPerOctave) : 12;

    juce::StringArray parts;
    for (const auto& note : notes)
    {
        if (note.stringIndex < 0 || note.stringIndex >= (int) state.openStringMidiNotes.size()) continue;

        const double openFreq = state.referenceFrequency
            * std::pow (2.0, (state.openStringMidiNotes[(size_t) note.stringIndex] - 69) / 12.0);
        const double freq = tuning.isEqualTemperament
            ? openFreq * std::pow (2.0, (double) note.fretNumber / namingEdo)
            : openFreq * std::pow (2.0, (double) note.fretNumber / 12.0);

        const auto pitch = mtg::nameFrequency (freq, state.referenceFrequency, namingEdo);
        auto text = juce::String::fromUTF8 (pitch.name.c_str()) + juce::String (pitch.octave);

        const auto centsText = mtg::formatCents (pitch.cents);
        if (! centsText.empty())
            text << "  " << juce::String::fromUTF8 (centsText.c_str());

        if (tuning.isEqualTemperament)
            text << "  (step " << pitch.stepFromA4 << ")";

        parts.add (text);
    }

    readout.setText (parts.joinIntoString ("   |   "), juce::dontSendNotification);
}

void MicroGuitarEditor::updateStringCountHint()
{
    const int instrumentChoice = (int) audioProcessor.apvts.getRawParameterValue ("instrument")->load();
    const auto instrument = static_cast<mtg::Instrument> (
        juce::jlimit (0, mtg::kNumInstruments - 1, instrumentChoice));
    const auto info = mtg::instrumentInfo (instrument);
    stringCountHint.setText (juce::String (info.minStrings) + "-" + juce::String (info.maxStrings)
                              + " for " + juce::String (info.name),
                              juce::dontSendNotification);
}

void MicroGuitarEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff222222));

    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (juce::FontOptions (24.0f)));
    g.drawFittedText ("Microtonal MPE Guitar", getLocalBounds().removeFromTop (45),
                       juce::Justification::centred, 1);
}

void MicroGuitarEditor::resized()
{
    auto bounds = getLocalBounds().reduced (20);
    bounds.removeFromTop (45); // Leave space for main title

    auto fretboardBounds = bounds.removeFromBottom (150);
    fretboard.setBounds (fretboardBounds);

    bounds.removeFromBottom (10);
    auto readoutRow = bounds.removeFromBottom (22);
    readout.setBounds (readoutRow);

    bounds.removeFromBottom (8);
    auto toggleRow = bounds.removeFromBottom (22);
    showAllNamesToggle.setBounds (toggleRow.removeFromLeft (200));

    bounds.removeFromBottom (15);

    int itemHeight = 30;
    int spacing = 15;
    int columnWidth = bounds.getWidth() / 4 - 10;
    auto controlArea = bounds;

    // Column 1: Instrument
    auto col1 = controlArea.removeFromLeft (columnWidth);
    instrumentDropdown.setBounds (col1.removeFromBottom (itemHeight));

    controlArea.removeFromLeft (spacing);

    // Column 2: Tuning (edoControl) + Key stacked
    auto col2 = controlArea.removeFromLeft (columnWidth);
    auto keyArea = col2.removeFromBottom (itemHeight);
    keyDropdown.setBounds (keyArea);
    auto tuningArea = col2.removeFromBottom (itemHeight);
    edoControl.setBounds (tuningArea);

    controlArea.removeFromLeft (spacing);

    // Column 3: Strings slider + hint stacked
    auto col3 = controlArea.removeFromLeft (columnWidth);
    stringCountHint.setBounds (col3.removeFromBottom (14));
    stringCountSlider.setBounds (col3.removeFromBottom (itemHeight));

    controlArea.removeFromLeft (spacing);

    // Column 4: Reference Pitch Slider
    auto col4 = controlArea;
    refFreqSlider.setBounds (col4.removeFromBottom (itemHeight));
}
