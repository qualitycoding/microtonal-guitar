// FROZEN TEST SUITE (r4 — re-frozen after owner answers to A1/A3) — see tests/FROZEN.md. Implementers must not edit, skip or weaken.
// Owner rules (2026-09-19): tune from the lowest string up; follow real extended-range instruments;
// high-register instruments (guitar, fifths presets) never add strings above their standard top string;
// bass instruments may extend upward (owner, r4). Bass: all fourths from the lowest string up, low B0
// from 5 strings, low F#0 from 8 strings, further strings added upward (8-string F#0..F3 is the common
// real layout; 9-15 are RULE). Limits (owner, r4): guitar 4-10, bass 4-15, fifths 4-5.
// Sources: https://en.wikipedia.org/wiki/Standard_tuning (guitar 6/7, bass 4/5/6, violin, viola,
// cello, mandolin, mandola); https://www.sweetwater.com/sweetcare/articles/8-string-guitar-tuning-guide/
// (8-string F#1). Rows marked RULE are derived from the owner's rules, not a published tuning.
#include "../third_party/doctest.h"
#include "Instruments.h"
#include <cmath>
#include <vector>

using namespace mtg;
using V = std::vector<int>;

TEST_CASE ("U12 open-string tables (low -> high MIDI)")
{
    // Guitar
    CHECK (openStringMidiNotes (Instrument::Guitar, 4) == V { 50, 55, 59, 64 });                  // RULE
    CHECK (openStringMidiNotes (Instrument::Guitar, 5) == V { 45, 50, 55, 59, 64 });              // RULE
    CHECK (openStringMidiNotes (Instrument::Guitar, 6) == V { 40, 45, 50, 55, 59, 64 });          // E2 A2 D3 G3 B3 E4
    CHECK (openStringMidiNotes (Instrument::Guitar, 7) == V { 35, 40, 45, 50, 55, 59, 64 });      // + B1
    CHECK (openStringMidiNotes (Instrument::Guitar, 8) == V { 30, 35, 40, 45, 50, 55, 59, 64 });  // + F#1
    CHECK (openStringMidiNotes (Instrument::Guitar, 9) == V { 25, 30, 35, 40, 45, 50, 55, 59, 64 });      // + C#1 (RULE)
    CHECK (openStringMidiNotes (Instrument::Guitar, 10) == V { 20, 25, 30, 35, 40, 45, 50, 55, 59, 64 }); // + G#0 (RULE)
    // Bass
    CHECK (openStringMidiNotes (Instrument::Bass, 4) == V { 28, 33, 38, 43 });                    // E1 A1 D2 G2
    CHECK (openStringMidiNotes (Instrument::Bass, 5) == V { 23, 28, 33, 38, 43 });                // + B0
    CHECK (openStringMidiNotes (Instrument::Bass, 6) == V { 23, 28, 33, 38, 43, 48 });            // + C3
    CHECK (openStringMidiNotes (Instrument::Bass, 7) == V { 23, 28, 33, 38, 43, 48, 53 });        // + F3
    CHECK (openStringMidiNotes (Instrument::Bass, 8) == V { 18, 23, 28, 33, 38, 43, 48, 53 });    // F#0..F3
    CHECK (openStringMidiNotes (Instrument::Bass, 9) == V { 18, 23, 28, 33, 38, 43, 48, 53, 58 }); // + Bb3 (RULE)
    {
        V b15; for (int k = 0; k < 15; ++k) b15.push_back (18 + 5 * k);                             // F#0..E6 (RULE)
        CHECK (openStringMidiNotes (Instrument::Bass, 15) == b15);
    }
    // Fifths presets
    CHECK (openStringMidiNotes (Instrument::ViolinMandolin, 4) == V { 55, 62, 69, 76 });          // G3 D4 A4 E5
    CHECK (openStringMidiNotes (Instrument::ViolinMandolin, 5) == V { 48, 55, 62, 69, 76 });      // + C3 (RULE)
    CHECK (openStringMidiNotes (Instrument::ViolaMandola, 4)   == V { 48, 55, 62, 69 });          // C3 G3 D4 A4
    CHECK (openStringMidiNotes (Instrument::ViolaMandola, 5)   == V { 41, 48, 55, 62, 69 });      // + F2 (RULE)
    CHECK (openStringMidiNotes (Instrument::Cello, 4)          == V { 36, 43, 50, 57 });          // C2 G2 D3 A3
    CHECK (openStringMidiNotes (Instrument::Cello, 5)          == V { 29, 36, 43, 50, 57 });      // + F1 (RULE)
}

TEST_CASE ("U13 names, ranges and clamping")
{
    const char* names[] = { "Guitar", "Bass", "Violin / Mandolin", "Viola / Mandola", "Cello" };
    const int   minS[]  = { 4, 4, 4, 4, 4 };
    const int   maxS[]  = { 10, 15, 5, 5, 5 };
    for (int i = 0; i < kNumInstruments; ++i)
    {
        const auto inst = static_cast<Instrument> (i);
        const auto info = instrumentInfo (inst);
        INFO ("instrument " << i);
        CHECK (info.name == names[i]);
        CHECK (info.minStrings == minS[i]);
        CHECK (info.maxStrings == maxS[i]);
        for (int n = -3; n <= 24; ++n)
        {
            const int expectCount = n < minS[i] ? minS[i] : (n > maxS[i] ? maxS[i] : n);
            CHECK (static_cast<int> (openStringMidiNotes (inst, n).size()) == expectCount);
        }
        CHECK (openStringMidiNotes (inst, 99) == openStringMidiNotes (inst, maxS[i]));
        CHECK (openStringMidiNotes (inst, 1)  == openStringMidiNotes (inst, minS[i]));
    }
}

TEST_CASE ("U14 realism invariants: high-register top string fixed, all strings audible, MPE-routable")
{
    const int top[] = { 64, -1, 76, 69, 57 }; // -1: bass may extend upward
    for (int i = 0; i < kNumInstruments; ++i)
    {
        const auto inst = static_cast<Instrument> (i);
        const auto info = instrumentInfo (inst);
        CHECK (info.maxStrings <= 15); // MPE lower zone: member channels 2-16 = one channel per string
        for (int n = info.minStrings; n <= info.maxStrings; ++n)
        {
            INFO ("instrument " << i << ", " << n << " strings");
            const auto v = openStringMidiNotes (inst, n);
            if (top[i] >= 0) CHECK (v.back() == top[i]);
            for (size_t k = 1; k < v.size(); ++k) CHECK (v[k] > v[k - 1]);
            CHECK (v.front() >= 0);
            CHECK (v.back() <= 127);
            const double lowestAt400 = 400.0 * std::pow (2.0, (v.front() - 69) / 12.0); // lowest refFreq
            CHECK (lowestAt400 >= 20.0);
        }
    }
}

TEST_CASE ("U15 string intervals")
{
    auto steps = [] (const V& v) { V d; for (size_t k = 1; k < v.size(); ++k) d.push_back (v[k] - v[k - 1]); return d; };
    CHECK (steps (openStringMidiNotes (Instrument::Guitar, 10)) == V { 5, 5, 5, 5, 5, 5, 5, 4, 5 }); // G-B major third
    for (int n = 4; n <= 15; ++n)
        CHECK (steps (openStringMidiNotes (Instrument::Bass, n)) == V ((size_t) n - 1, 5));        // all fourths
    for (auto inst : { Instrument::ViolinMandolin, Instrument::ViolaMandola, Instrument::Cello })
        CHECK (steps (openStringMidiNotes (inst, 5)) == V { 7, 7, 7, 7 });
}
