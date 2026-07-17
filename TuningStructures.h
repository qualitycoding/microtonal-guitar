#pragma once
#include <vector>
#include <string>

// Defines the rules for how the octave is divided
struct TuningSystem 
{
    std::string name;
    bool isEqualTemperament; // True for EDO (12-TET, 24-TET), False for Just Intonation
    int stepsPerOctave;      // e.g., 12, 24, 31
    std::vector<double> customRatios; // Used only if isEqualTemperament is false
};

// Defines the physical state of the instrument
struct GuitarState 
{
    int numStrings;
    double referenceFrequency; // e.g., 440.0 for A4
    std::vector<double> openStringFrequencies; // Frequencies of strings 0 to N
    TuningSystem currentTuning;
};
