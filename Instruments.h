// SPDX-License-Identifier: Apache-2.0
// FROZEN INTERFACE CONTRACT (r3) for tests U12-U15 and I13-I17. Pure C++17, no JUCE.
// Implement in Instruments.cpp at repo root.
#pragma once
#include <string>
#include <vector>

namespace mtg
{
    enum class Instrument { Guitar = 0, Bass, ViolinMandolin, ViolaMandola, Cello };
    constexpr int kNumInstruments = 5;

    struct InstrumentInfo
    {
        std::string name;   // exact UI/parameter choice text
        int minStrings;
        int maxStrings;
    };

    InstrumentInfo instrumentInfo (Instrument);

    // Open-string MIDI notes, index 0 = lowest string. requestedStrings is clamped to
    // [minStrings, maxStrings]. Strings are added below the instrument's standard set, never above its
    // standard highest string -- except Bass, whose 6-string layout adds a high C3 (owner, 2026-09-19).
    std::vector<int> openStringMidiNotes (Instrument, int requestedStrings);
}
