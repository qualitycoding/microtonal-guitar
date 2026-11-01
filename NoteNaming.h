// SPDX-License-Identifier: Apache-2.0
// FROZEN INTERFACE CONTRACT for the tests in tests/unit. Implementers copy this to
// Source/NoteNaming.h (repo root today) and implement it in NoteNaming.cpp.
// Pure C++17, no JUCE dependency, so it is unit-testable without a plugin host.
#pragma once
#include <optional>
#include <string>

namespace mtg
{
    constexpr int kMinEdo = 1;
    constexpr int kMaxEdo = 96;

    struct EdoAnalysis
    {
        bool valid = false;  // false for 1,2,3,4,6,8 (no usable heptatonic notation)
        int edo = 0;
        int fifthSteps = 0;  // notational fifth: round(N*log2(3/2)), minus 1 if that makes m2 = 3N-5f negative
        int sharpSteps = 0;  // |7f - 4N|; a displayed '#' always RAISES pitch by this many steps
    };

    EdoAnalysis analyseEdo (int edo);

    // ASCII ups-and-downs spelling of (stepFromC mod edo): arrows first ('^'/'v', repeated),
    // then nominal A-G, then accidental: "#", "x" (double sharp), "b", "bb".
    // Degenerate edos return the step number as decimal text.
    std::string spellStep (int edo, int stepFromC);

    struct PitchName
    {
        std::string name = "?";  // "?" when inputs are invalid
        int octave = 0;          // scientific pitch notation, octave boundary at C
        double cents = 0.0;      // hz relative to the nearest lattice pitch (A4-anchored)
        int stepFromA4 = 0;
    };

    PitchName nameFrequency (double hz, double a4Hz, int edo);

    // "" if |cents| rounds to 0, else signed integer + U+00A2 (UTF-8), e.g. "+3¢", "-38¢".
    std::string formatCents (double cents);

    struct TuningChoice
    {
        enum class Mode { Edo, Just5 };
        Mode mode = Mode::Edo;
        int steps = 12;
    };

    // Accepts "13", "13edo", "13-EDO", "13 TET", preset labels, and anything containing
    // "just" (case-insensitive) -> Just5. Rejects out-of-range, non-integer, or junk input.
    std::optional<TuningChoice> parseTuningText (const std::string& text);

    // Frets shown = smallest multiple of edo that is >= 24 (12->24, 13->26, 31->31).
    int fretCountFor (int edo);

    // A per-fret name label is drawn only if the slot is at least labelWidth + 4 px.
    bool shouldDrawFretLabel (float slotWidthPx, float labelWidthPx);
}
