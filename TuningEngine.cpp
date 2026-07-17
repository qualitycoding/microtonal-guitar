#include "TuningEngine.h"

TuningEngine::TuningEngine() 
{
    // Default fallback state (Standard 6-string, 12-TET)
    state.numStrings = 6;
    state.referenceFrequency = 440.0;
    state.openStringFrequencies = { 82.41, 110.00, 146.83, 196.00, 246.94, 329.63 };
    state.currentTuning = create12TET();
}

void TuningEngine::setGuitarState(const GuitarState& newState) 
{
    state = newState;
}

double TuningEngine::getFrequency(int stringIndex, int fretNumber) const 
{
    // Safety check to prevent out-of-bounds errors
    if (stringIndex < 0 || stringIndex >= state.numStrings) return 0.0;

    double openFreq = state.openStringFrequencies[stringIndex];

    if (state.currentTuning.isEqualTemperament) 
    {
        // Math for EDO (Equal Divisions of the Octave)
        return openFreq * std::pow(2.0, static_cast<double>(fretNumber) / state.currentTuning.stepsPerOctave);
    } 
    else 
    {
        // Math for Just Intonation / Ratio-based scales
        int numRatios = static_cast<int>(state.currentTuning.customRatios.size());
        if (numRatios == 0) return openFreq; // Fallback

        // Calculate how many full octaves we've gone up, and the remainder
        int octaves = fretNumber / numRatios;
        int scaleDegree = fretNumber % numRatios;

        double ratio = state.currentTuning.customRatios[scaleDegree];
        return openFreq * ratio * std::pow(2.0, octaves);
    }
}

// --- FACTORY PRESETS ---

TuningSystem TuningEngine::create12TET() 
{
    return { "12-TET (Standard)", true, 12, {} };
}

TuningSystem TuningEngine::create24TET() 
{
    return { "24-TET (Quarter Tones)", true, 24, {} };
}

TuningSystem TuningEngine::create31EDO() 
{
    return { "31-EDO", true, 31, {} };
}

TuningSystem TuningEngine::create5LimitJust() 
{
    // Standard 5-Limit Just Intonation ratios for one octave
    std::vector<double> ratios = { 
        1.0, 16.0/15.0, 9.0/8.0, 6.0/5.0, 5.0/4.0, 4.0/3.0, 
        45.0/32.0, 3.0/2.0, 8.0/5.0, 5.0/3.0, 9.0/5.0, 15.0/8.0 
    };
    return { "5-Limit Just Intonation", false, 12, ratios };
}
