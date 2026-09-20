#include "TuningEngine.h"

TuningEngine::TuningEngine()
{
    // Default fallback state (Standard 6-string guitar, 12-TET) - matches the instrument default
    // (Guitar) and tuning default (12-EDO) that PluginProcessor's parameter layout declares.
    state.numStrings = 6;
    state.referenceFrequency = 440.0;
    state.openStringMidiNotes = { 40, 45, 50, 55, 59, 64 }; // E2 A2 D3 G3 B3 E4
    state.openStringFrequencies = { 82.41, 110.00, 146.83, 196.00, 246.94, 329.63 };
    state.currentTuning = createEdo (12);
}

void TuningEngine::setGuitarState (const GuitarState& newState)
{
    std::lock_guard<std::mutex> lock (stateLock);
    state = newState;
}

double TuningEngine::getFrequency (int stringIndex, int fretNumber) const
{
    std::lock_guard<std::mutex> lock (stateLock);

    if (stringIndex < 0 || stringIndex >= state.numStrings) return 0.0;
    if (stringIndex >= (int) state.openStringFrequencies.size()) return 0.0; // defensive: keeps a
                                                                              // mismatched state safe
    const double openFreq = state.openStringFrequencies[(size_t) stringIndex];

    if (state.currentTuning.isEqualTemperament)
    {
        return openFreq * std::pow (2.0, static_cast<double> (fretNumber) / state.currentTuning.stepsPerOctave);
    }
    else
    {
        const int numRatios = static_cast<int> (state.currentTuning.customRatios.size());
        if (numRatios == 0) return openFreq;

        int octaves = fretNumber / numRatios;
        int scaleDegree = fretNumber % numRatios;
        if (scaleDegree < 0) // C++'s % can return negative for negative fretNumber
        {
            scaleDegree += numRatios;
            octaves -= 1;
        }
        const double ratio = state.currentTuning.customRatios[(size_t) scaleDegree];
        return openFreq * ratio * std::pow (2.0, octaves);
    }
}

TuningSystem TuningEngine::getCurrentTuningSystem() const
{
    std::lock_guard<std::mutex> lock (stateLock);
    return state.currentTuning; // returned by value - safe copy under the lock
}

int TuningEngine::getNumStrings() const
{
    std::lock_guard<std::mutex> lock (stateLock);
    return state.numStrings;
}

int TuningEngine::getOpenStringMidiNote (int stringIndex) const
{
    std::lock_guard<std::mutex> lock (stateLock);
    if (stringIndex < 0 || stringIndex >= (int) state.openStringMidiNotes.size()) return -1;
    return state.openStringMidiNotes[(size_t) stringIndex];
}

GuitarState TuningEngine::getSnapshot() const
{
    std::lock_guard<std::mutex> lock (stateLock);
    return state; // one copy, one lock - the UI paints from this rather than re-locking per field
}

// --- FACTORIES ---

TuningSystem TuningEngine::createEdo (int stepsPerOctave)
{
    if (stepsPerOctave < 1) stepsPerOctave = 1;
    return { std::to_string (stepsPerOctave) + "-EDO", true, stepsPerOctave, {} };
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
