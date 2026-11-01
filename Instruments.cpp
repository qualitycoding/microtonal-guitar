// SPDX-License-Identifier: Apache-2.0
// Instruments.cpp — per-instrument open-string layouts.
// Implements the contract frozen at plan/api/Instruments.h. No JUCE dependency;
// unit-tested by tests/unit/test_instruments.cpp (do not edit that file).
//
// Rules agreed with the instrument owner on 2026-09-19 (see plan/PLAN.md §2, decisions D1/D5 and
// applied choices A1-A6; sources in research/RESEARCH_LOG.md sections E-G):
//   - Every layout is tuned in fourths (guitar, bass) or fifths (violin/mandolin family), lowest
//     string first.
//   - High-register instruments (guitar, and the fifths-tuned presets) never gain a string above
//     their standard top string: extra strings are added BELOW, so the top string is fixed for every
//     string count.
//   - Bass is the exception: once the standard 4/5/6/7-string layouts are exhausted, further strings
//     are added ABOVE (bass strings can reach into the guitar's register).
//   - String counts outside an instrument's [min,max] are clamped, never rejected.

#include "Instruments.h"

#include <algorithm>
#include <array>

namespace mtg
{
    namespace
    {
        // Guitar: ascending MIDI notes for the largest supported layout (10 strings), low to high.
        // G#0 C#1 F#1 B1 E2 A2 D3 G3 B3 E4 — every gap a perfect fourth (5 semitones) except the
        // historical major third between G3 and B3. Fewer strings keep the top and drop from the
        // bottom (owner rule A2), so the table for n strings is simply the last n of these ten.
        constexpr std::array<int, 10> kGuitarFull { 20, 25, 30, 35, 40, 45, 50, 55, 59, 64 };

        // Fifths-tuned presets: ascending MIDI notes for the largest supported layout (5 strings).
        // Same "keep the top, drop from the bottom" rule as guitar (owner rule A2/A3).
        constexpr std::array<int, 5> kViolinMandolinFull { 48, 55, 62, 69, 76 }; // C3 G3 D4 A4 E5
        constexpr std::array<int, 5> kViolaMandolaFull   { 41, 48, 55, 62, 69 }; // F2 C3 G3 D4 A4
        constexpr std::array<int, 5> kCelloFull          { 29, 36, 43, 50, 57 }; // F1 C2 G2 D3 A3

        template <size_t N>
        std::vector<int> lastN (const std::array<int, N>& full, int n)
        {
            const size_t count = static_cast<size_t> (n);
            return { full.end() - static_cast<long> (count), full.end() };
        }

        // Bass: fourths from the lowest string up (owner rule A4). The low anchor rises in three
        // steps as more strings are requested, then every added string above 7 extends upward:
        //   4 strings:      E1 A1 D2 G2                     (anchor E1  = MIDI 28)
        //   5-7 strings:    B0 E1 A1 D2 G2 [C3] [F3]         (anchor B0  = MIDI 23)
        //   8-15 strings:   F#0 B0 E1 A1 D2 G2 C3 F3 [Bb3...]  (anchor F#0 = MIDI 18, extends upward)
        int bassLowAnchor (int n)
        {
            if (n <= 4) return 28;
            if (n <= 7) return 23;
            return 18;
        }

        std::vector<int> bassTable (int n)
        {
            std::vector<int> notes;
            notes.reserve (static_cast<size_t> (n));
            const int low = bassLowAnchor (n);
            for (int i = 0; i < n; ++i) notes.push_back (low + 5 * i);
            return notes;
        }
    }

    InstrumentInfo instrumentInfo (Instrument instrument)
    {
        switch (instrument)
        {
            case Instrument::Guitar:         return { "Guitar", 4, 10 };
            case Instrument::Bass:           return { "Bass", 4, 15 };
            case Instrument::ViolinMandolin: return { "Violin / Mandolin", 4, 5 };
            case Instrument::ViolaMandola:   return { "Viola / Mandola", 4, 5 };
            case Instrument::Cello:          return { "Cello", 4, 5 };
        }
        return { "Guitar", 4, 10 }; // unreachable; keeps -Wreturn-type quiet on some compilers
    }

    std::vector<int> openStringMidiNotes (Instrument instrument, int requestedStrings)
    {
        const auto info = instrumentInfo (instrument);
        const int n = std::clamp (requestedStrings, info.minStrings, info.maxStrings);

        switch (instrument)
        {
            case Instrument::Guitar:         return lastN (kGuitarFull, n);
            case Instrument::Bass:           return bassTable (n);
            case Instrument::ViolinMandolin: return lastN (kViolinMandolinFull, n);
            case Instrument::ViolaMandola:   return lastN (kViolaMandolaFull, n);
            case Instrument::Cello:          return lastN (kCelloFull, n);
        }
        return lastN (kGuitarFull, n); // unreachable
    }
}
