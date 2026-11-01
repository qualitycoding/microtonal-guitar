// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <vector>
#include <string>

// Defines the rules for how the octave is divided
struct TuningSystem
{
    std::string name;
    bool isEqualTemperament;  // True for EDO (any steps-per-octave), False for Just Intonation
    int stepsPerOctave;       // e.g. 12, 13, 24, 31 (for equal temperaments)
    std::vector<double> customRatios; // Used only if isEqualTemperament is false
};

// Defines the physical state of the instrument
struct GuitarState
{
    int numStrings;
    double referenceFrequency;                 // e.g., 440.0 for A4
    std::vector<double> openStringFrequencies; // Frequencies of strings 0 to N-1
    std::vector<int> openStringMidiNotes;      // Same strings' nearest 12-TET MIDI note, computed
                                                // together with openStringFrequencies from one
                                                // instrument + refFreq (single source of truth: see
                                                // PluginProcessor::updateGuitarState). Note routing,
                                                // fret display and pitch all read from this pair, so
                                                // a fret label can never disagree with what sounds.
    TuningSystem currentTuning;
};
