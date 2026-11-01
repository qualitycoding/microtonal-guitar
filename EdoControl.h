// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <juce_gui_basics/juce_gui_basics.h> // Component, ComboBox, Button (issue #8)
#include "NoteNaming.h"
#include "PluginProcessor.h"

// Editable tuning control: free numeric entry ("13", "13edo", "19 TET"...), preset items, plus
// up/down steppers - none of which require typing (plan §3.3; a stock JUCE ComboBoxAttachment can
// only select a fixed list item - research R6 - so this binds "tuningMode" and "edoSteps" directly).
class EdoControl : public juce::Component,
                    private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit EdoControl (MicroGuitarProcessor& processorToUse) : processor (processorToUse)
    {
        combo.setEditableText (true);
        combo.setJustificationType (juce::Justification::centred);
        combo.addItem ("12-TET", 1);
        combo.addItem ("17-EDO", 2);
        combo.addItem ("19-EDO", 3);
        combo.addItem ("22-EDO", 4);
        combo.addItem ("24-TET (Quarter Tones)", 5);
        combo.addItem ("31-EDO", 6);
        combo.addItem ("41-EDO", 7);
        combo.addItem ("53-EDO", 8);
        combo.addItem ("5-Limit Just", 9);
        combo.onChange = [this] { commitComboText(); };
        addAndMakeVisible (combo);

        upButton.setButtonText ("+");
        downButton.setButtonText ("-");
        upButton.onClick = [this] { step (1); };
        downButton.onClick = [this] { step (-1); };
        addAndMakeVisible (upButton);
        addAndMakeVisible (downButton);

        processor.apvts.addParameterListener ("edoSteps", this);
        processor.apvts.addParameterListener ("tuningMode", this);

        refreshFromParameters();
    }

    ~EdoControl() override
    {
        processor.apvts.removeParameterListener ("edoSteps", this);
        processor.apvts.removeParameterListener ("tuningMode", this);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto buttons = bounds.removeFromRight (22);
        upButton.setBounds (buttons.removeFromTop (bounds.getHeight() / 2));
        downButton.setBounds (buttons);
        combo.setBounds (bounds);
    }

private:
    // APVTS calls this from the audio thread; UI work must happen on the message thread.
    void parameterChanged (const juce::String&, float) override
    {
        juce::MessageManager::callAsync ([safeThis = juce::Component::SafePointer<EdoControl> (this)]
        {
            if (safeThis != nullptr) safeThis->refreshFromParameters();
        });
    }

    void refreshFromParameters()
    {
        if (comboHasFocus()) return; // don't yank text out from under the person while they're typing

        const int modeIndex = (int) processor.apvts.getRawParameterValue ("tuningMode")->load();
        if (modeIndex == 1)
        {
            combo.setText ("5-Limit Just", juce::dontSendNotification);
            return;
        }

        const int steps = (int) processor.apvts.getRawParameterValue ("edoSteps")->load();
        combo.setText (juce::String (steps) + "-EDO", juce::dontSendNotification);
    }

    bool comboHasFocus() const
    {
        return combo.hasKeyboardFocus (true); // true = include the internal editable-text child
    }

    void commitComboText()
    {
        const auto parsed = mtg::parseTuningText (combo.getText().toStdString());
        if (! parsed)
        {
            combo.setTooltip ("Enter a whole number of steps per octave, 1-96 (e.g. \"13\" or "
                               "\"13edo\"), or \"5-Limit Just\".");
            refreshFromParameters(); // revert the displayed text to the last valid value
            return;
        }

        if (parsed->mode == mtg::TuningChoice::Mode::Just5)
        {
            setParamPlain ("tuningMode", 1.0f);
        }
        else
        {
            setParamPlain ("edoSteps", (float) parsed->steps);
            setParamPlain ("tuningMode", 0.0f);
        }

        releaseComboFocus(); // R7: give focus back to the plugin window, not the host's QWERTY keyboard
    }

    void step (int delta)
    {
        const int modeIndex = (int) processor.apvts.getRawParameterValue ("tuningMode")->load();
        int steps = modeIndex == 1 ? 12 : (int) processor.apvts.getRawParameterValue ("edoSteps")->load();
        steps = juce::jlimit (mtg::kMinEdo, mtg::kMaxEdo, steps + delta);
        setParamPlain ("edoSteps", (float) steps);
        setParamPlain ("tuningMode", 0.0f);
    }

    void releaseComboFocus()
    {
        if (auto* topLevel = getTopLevelComponent())
            topLevel->grabKeyboardFocus();
    }

    void setParamPlain (const char* id, float plainValue)
    {
        auto* param = processor.apvts.getParameter (id);
        param->setValueNotifyingHost (param->convertTo0to1 (plainValue));
    }

    MicroGuitarProcessor& processor;
    juce::ComboBox combo;
    juce::TextButton upButton, downButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EdoControl)
};
