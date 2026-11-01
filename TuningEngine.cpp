// SPDX-License-Identifier: Apache-2.0
#include "TuningEngine.h"

TuningEngine::TuningEngine()
{
    // Default fallback state (Standard 6-string guitar, 12-TET) - matches the instrument default
    // (Guitar) and tuning default (12-EDO) that PluginProcessor's parameter layout declares.
    GuitarState initial;
    initial.numStrings = 6;
    initial.referenceFrequency = 440.0;
    initial.openStringMidiNotes = { 40, 45, 50, 55, 59, 64 }; // E2 A2 D3 G3 B3 E4
    initial.openStringFrequencies = { 82.41, 110.00, 146.83, 196.00, 246.94, 329.63 };
    initial.currentTuning = createEdo (12);
    slots[0].state = std::move (initial);
}

int TuningEngine::beginRead() const
{
    for (;;)
    {
        const int index = current.load (std::memory_order_acquire);
        slots[(size_t) index].readers.fetch_add (1, std::memory_order_relaxed);
        if (current.load (std::memory_order_relaxed) == index)
            return index;
        // A writer published between our load and our registration - retry on the new slot.
        slots[(size_t) index].readers.fetch_sub (1, std::memory_order_relaxed);
    }
}

void TuningEngine::endRead (int slotIndex) const
{
    slots[(size_t) slotIndex].readers.fetch_sub (1, std::memory_order_release);
}

void TuningEngine::setGuitarState (const GuitarState& newState)
{
    std::lock_guard<std::mutex> lock (writerMutex);

    const int published = current.load (std::memory_order_relaxed);
    const int other = 1 - published;

    // Wait for in-flight readers of the slot we are about to overwrite. Readers hold a slot
    // only for the duration of a few plain reads (nanoseconds), so this drains almost
    // immediately and never blocks on thread scheduling - the writer never sleeps.
    while (slots[(size_t) other].readers.load (std::memory_order_acquire) != 0)
    {
    }

    slots[(size_t) other].state = newState; // copy-assign: may allocate on early writes,
                                            // reuses vector capacity in steady state
    current.store (other, std::memory_order_release);
}

double TuningEngine::getFrequency (int stringIndex, int fretNumber) const
{
    const int slotIndex = beginRead();
    const GuitarState& state = slots[(size_t) slotIndex].state;

    double result = 0.0;
    if (stringIndex >= 0 && stringIndex < state.numStrings
        && stringIndex < (int) state.openStringFrequencies.size()) // defensive: keeps a
                                                                   // mismatched state safe
    {
        const double openFreq = state.openStringFrequencies[(size_t) stringIndex];

        if (state.currentTuning.isEqualTemperament)
        {
            result = openFreq * std::pow (2.0, static_cast<double> (fretNumber) / state.currentTuning.stepsPerOctave);
        }
        else
        {
            const int numRatios = static_cast<int> (state.currentTuning.customRatios.size());
            if (numRatios > 0)
            {
                int octaves = fretNumber / numRatios;
                int scaleDegree = fretNumber % numRatios;
                if (scaleDegree < 0) // C++'s % can return negative for negative fretNumber
                {
                    scaleDegree += numRatios;
                    octaves -= 1;
                }
                const double ratio = state.currentTuning.customRatios[(size_t) scaleDegree];
                result = openFreq * ratio * std::pow (2.0, octaves);
            }
            else
            {
                result = openFreq;
            }
        }
    }

    endRead (slotIndex);
    return result;
}

TuningSystem TuningEngine::getCurrentTuningSystem() const
{
    const int slotIndex = beginRead();
    TuningSystem result = slots[(size_t) slotIndex].state.currentTuning; // copy under registration
    endRead (slotIndex);
    return result;
}

int TuningEngine::getNumStrings() const
{
    const int slotIndex = beginRead();
    const int result = slots[(size_t) slotIndex].state.numStrings;
    endRead (slotIndex);
    return result;
}

int TuningEngine::getOpenStringMidiNote (int stringIndex) const
{
    const int slotIndex = beginRead();
    const GuitarState& state = slots[(size_t) slotIndex].state;
    const int result = (stringIndex < 0 || stringIndex >= (int) state.openStringMidiNotes.size())
                           ? -1
                           : state.openStringMidiNotes[(size_t) stringIndex];
    endRead (slotIndex);
    return result;
}

GuitarState TuningEngine::getSnapshot() const
{
    const int slotIndex = beginRead();
    GuitarState result = slots[(size_t) slotIndex].state; // one copy, no lock
    endRead (slotIndex);
    return result;
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
