#pragma once
#include "TuningStructures.h"
#include <cmath>
#include <mutex>

class TuningEngine 
{
public:
    TuningEngine();
    ~TuningEngine() = default;

    void setGuitarState(const GuitarState& newState);
    double getFrequency(int stringIndex, int fretNumber) const;

    // Thread-safe snapshots - safe to call from the UI thread while
    // parameter changes update state from the audio thread.
    TuningSystem getCurrentTuningSystem() const;
    int getNumStrings() const;

    static TuningSystem create12TET();
    static TuningSystem create24TET();
    static TuningSystem create31EDO();
    static TuningSystem create5LimitJust();

    // Single source of truth for the open-string MIDI approximation, used by
    // the voice, the processor's note tracking, and the UI alike.
    static int openStringMidiNote (int stringIndex) { return 40 + stringIndex * 5; }

private:
    GuitarState state;
    mutable std::mutex stateLock;
};