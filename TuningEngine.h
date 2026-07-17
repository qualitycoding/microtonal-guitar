#pragma once
#include "TuningStructures.h"
#include <cmath>

class TuningEngine 
{
public:
    TuningEngine();
    ~TuningEngine() = default;

    // Load a complete configuration
    void setGuitarState(const GuitarState& newState);

    // Get the exact frequency for any fret on any string
    double getFrequency(int stringIndex, int fretNumber) const;

    // Factory methods to generate standard scales
    static TuningSystem create12TET();
    static TuningSystem create24TET();
    static TuningSystem create31EDO();
    static TuningSystem create5LimitJust();

private:
    GuitarState state;
};
