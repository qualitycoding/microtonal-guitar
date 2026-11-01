// Spike (1.3.6 command verification / R6 empirical verification):
// TuningEngine has no JUCE dependency, so it compiles standalone with the
// pinned g++. Verifies: 12-TET octave doubling, 24-TET half-semitone,
// 5-limit just ratios, and invalid-index behaviour (returns 0.0).
#include "../../TuningEngine.h"
#include <cstdio>
#include <cmath>

static int failures = 0;
static void check(const char* name, double got, double want, double tol)
{
    if (std::fabs(got - want) > tol) { std::printf("FAIL %s: got %.6f want %.6f\n", name, got, want); ++failures; }
    else std::printf("pass %s (%.6f)\n", name, got);
}

int main()
{
    TuningEngine engine; // default: 6 strings, 12-TET, E2=82.41 ...

    // 12-TET: one octave up = exactly 2x
    check("12TET octave", engine.getFrequency(0, 12), 82.41 * 2.0, 1e-9);
    // 12-TET: fret 7 = perfect fifth = 2^(7/12)
    check("12TET fifth", engine.getFrequency(0, 7), 82.41 * std::pow(2.0, 7.0/12.0), 1e-9);

    // 24-TET: two frets = one semitone
    {
        GuitarState s; s.numStrings = 6; s.referenceFrequency = 440.0;
        s.openStringFrequencies = { 82.41, 110.00, 146.83, 196.00, 246.94, 329.63 };
        s.currentTuning = TuningEngine::create24TET();
        engine.setGuitarState(s);
        check("24TET 2 frets", engine.getFrequency(0, 2), 82.41 * std::pow(2.0, 1.0/12.0), 1e-9);
    }

    // 5-limit just: fret 7 = 3/2
    {
        GuitarState s; s.numStrings = 6; s.referenceFrequency = 440.0;
        s.openStringFrequencies = { 82.41, 110.00, 146.83, 196.00, 246.94, 329.63 };
        s.currentTuning = TuningEngine::create5LimitJust();
        engine.setGuitarState(s);
        check("5limit fifth", engine.getFrequency(0, 7), 82.41 * 1.5, 1e-9);
    }

    // 31-EDO sanity: 31 frets = 2x
    {
        GuitarState s; s.numStrings = 6; s.referenceFrequency = 440.0;
        s.openStringFrequencies = { 82.41, 110.00, 146.83, 196.00, 246.94, 329.63 };
        s.currentTuning = TuningEngine::create31EDO();
        engine.setGuitarState(s);
        check("31EDO octave", engine.getFrequency(0, 31), 82.41 * 2.0, 1e-9);
    }

    // Invalid string index -> 0.0 (documented; see REVIEW.md G-6 re inf/NaN in voice)
    check("invalid index", engine.getFrequency(99, 0), 0.0, 0.0);

    std::printf(failures == 0 ? "SPIKE RESULT: ALL PASS\n" : "SPIKE RESULT: %d FAILURE(S)\n", failures);
    return failures == 0 ? 0 : 1;
}
