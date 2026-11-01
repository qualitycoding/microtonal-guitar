#pragma once
#include "TuningStructures.h"
#include <array>
#include <atomic>
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

    // Called from PluginProcessor::updateGuitarState. May be called from the audio thread
    // (APVTS parameterChanged) and the message thread; writers are serialized internally.
    // May allocate on first writes (vector growth); steady-state writes reuse capacity.
    void setGuitarState (const GuitarState& newState);

    // --- Audio-thread read path (called from MicrotonalVoice::updateFrequency and
    //     PluginProcessor::processBlock). LOCK-FREE and ALLOCATION-FREE: registers on the
    //     current slot, retries once if a writer publishes mid-read. Never blocks.
    double getFrequency (int stringIndex, int fretNumber) const;

    // Thread-safe snapshots for the UI thread: same lock-free read path, copied out by value.
    TuningSystem getCurrentTuningSystem() const;
    int getNumStrings() const;
    int getOpenStringMidiNote (int stringIndex) const; // -1 if stringIndex is out of range
    GuitarState getSnapshot() const;                   // one copy, for UI painting

    static TuningSystem createEdo (int stepsPerOctave);
    static TuningSystem create5LimitJust();

private:
    // Read-copy-update with two slots. The writer writes the slot that is not currently
    // published and not still being read (drained via the per-slot reader counters), then
    // publishes it with a single atomic store. Readers hold no locks and never free-run
    // against a writer: a slot being read is never written until its readers drain.
    struct Slot
    {
        GuitarState state;
        std::atomic<int> readers { 0 };
    };

    mutable std::array<Slot, 2> slots; // mutated (reader counts) even in const read methods
    std::atomic<int> current { 0 };    // index of the published slot
    std::mutex writerMutex;            // serializes writers (never touched by readers)

    // Registers the calling thread on the currently published slot. Returns the slot index,
    // or -1 if the published slot changed between the initial load and registration —
    // in that case the caller retries.
    int beginRead() const;
    void endRead (int slotIndex) const;
};
