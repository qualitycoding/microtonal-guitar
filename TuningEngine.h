#pragma once
#include "TuningStructures.h"
#include <cmath>
#include <mutex>

class TuningEngine
{
public:
    TuningEngine();
    ~TuningEngine() = default;

    // MPE lower zone has 15 usable member channels (2-16); string s is routed on channel 2+s, so
    // this is the hard ceiling on string count for every instrument (see plan §3.1 / A6). Shared by
    // PluginProcessor (voice count, activeFrets) and MicrotonalVoice (clamping the incoming channel).
    static constexpr int kMaxStrings = 15;

    void setGuitarState (const GuitarState& newState);
    double getFrequency (int stringIndex, int fretNumber) const;

    // Thread-safe snapshots - safe to call from the UI thread while
    // parameter changes update state from the audio thread.
    TuningSystem getCurrentTuningSystem() const;
    int getNumStrings() const;
    int getOpenStringMidiNote (int stringIndex) const; // -1 if stringIndex is out of range
    GuitarState getSnapshot() const;                   // one copy under one lock, for UI painting

    static TuningSystem createEdo (int stepsPerOctave);
    static TuningSystem create5LimitJust();

private:
    GuitarState state;
    mutable std::mutex stateLock;
};
