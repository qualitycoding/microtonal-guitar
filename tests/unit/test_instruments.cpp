// FROZEN TEST SUITE (r3) — see tests/FROZEN.md. Implementers must not edit, skip or weaken.
// Owner rules (2026-09-19): tune from the lowest string up; follow real extended-range instruments;
// when adding strings never go above the instrument's standard highest string (bass 6-string high C
// is the owner-stated exception); fifths instruments are presets.
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
    // Bass
    CHECK (openStringMidiNotes (Instrument::Bass, 4) == V { 28, 33, 38, 43 });                    // E1 A1 D2 G2
    CHECK (openStringMidiNotes (Instrument::Bass, 5) == V { 23, 28, 33, 38, 43 });                // + B0
    CHECK (openStringMidiNotes (Instrument::Bass, 6) == V { 23, 28, 33, 38, 43, 48 });            // + C3
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
    const int   maxS[]  = { 8, 6, 5, 5, 5 };
    for (int i = 0; i < kNumInstruments; ++i)
    {
        const auto inst = static_cast<Instrument> (i);
        const auto info = instrumentInfo (inst);
        INFO ("instrument " << i);
        CHECK (info.name == names[i]);
        CHECK (info.minStrings == minS[i]);
        CHECK (info.maxStrings == maxS[i]);
        for (int n = -3; n <= 20; ++n)
        {
            const int expectCount = n < minS[i] ? minS[i] : (n > maxS[i] ? maxS[i] : n);
            CHECK (static_cast<int> (openStringMidiNotes (inst, n).size()) == expectCount);
        }
        CHECK (openStringMidiNotes (inst, 12) == openStringMidiNotes (inst, maxS[i]));
        CHECK (openStringMidiNotes (inst, 1)  == openStringMidiNotes (inst, minS[i]));
    }
}

TEST_CASE ("U14 realism invariants: top string fixed, lowest string audible")
{
    const int top[] = { 64, 43, 76, 69, 57 };
    for (int i = 0; i < kNumInstruments; ++i)
    {
        const auto inst = static_cast<Instrument> (i);
        const auto info = instrumentInfo (inst);
        for (int n = info.minStrings; n <= info.maxStrings; ++n)
        {
            INFO ("instrument " << i << ", " << n << " strings");
            const auto v = openStringMidiNotes (inst, n);
            const int expectedTop = (inst == Instrument::Bass && n >= 6) ? 48 : top[i];
            CHECK (v.back() == expectedTop);
            for (size_t k = 1; k < v.size(); ++k) CHECK (v[k] > v[k - 1]);
            const double lowestAt400 = 400.0 * std::pow (2.0, (v.front() - 69) / 12.0); // lowest refFreq
            CHECK (lowestAt400 >= 20.0);
        }
    }
}

TEST_CASE ("U15 string intervals")
{
    auto steps = [] (const V& v) { V d; for (size_t k = 1; k < v.size(); ++k) d.push_back (v[k] - v[k - 1]); return d; };
    CHECK (steps (openStringMidiNotes (Instrument::Guitar, 8)) == V { 5, 5, 5, 5, 5, 4, 5 }); // G-B major third
    CHECK (steps (openStringMidiNotes (Instrument::Bass, 6))   == V { 5, 5, 5, 5, 5 });
    for (auto inst : { Instrument::ViolinMandolin, Instrument::ViolaMandola, Instrument::Cello })
        CHECK (steps (openStringMidiNotes (inst, 5)) == V { 7, 7, 7, 7 });
}
