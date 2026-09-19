// FROZEN TEST SUITE — see tests/FROZEN.md. Implementers must not edit, skip or weaken.
// If a test here is believed invalid: HALT and reopen the protocol from Phase 0.
//
// Oracle: Xenharmonic Wiki, "Kite's ups and downs notation", permalink
//   https://en.xen.wiki/index.php?title=Kite%27s_ups_and_downs_notation&oldid=237152
//   section "Examples: edos 12-24" (for flat-N edos the "sharp raises the pitch" row).
// Cross-check: https://en.xen.wiki/index.php?title=13edo&oldid=222544
// Tables were validated against the spec in research/oracle_check.py (217/217 cells).

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../third_party/doctest.h"
#include "NoteNaming.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace mtg;

namespace
{
    std::vector<std::string> split (const std::string& s, char sep)
    {
        std::vector<std::string> out; std::string cur; std::istringstream is (s);
        while (std::getline (is, cur, sep)) out.push_back (cur);
        return out;
    }

    // Step of natural D above C for the notational fifth f: 2f - N.
    int dPos (int edo) { auto a = analyseEdo (edo); return 2 * a.fifthSteps - edo; }

    // Each cell lists every acceptable spelling ("/"-separated); the chosen one must be among them.
    void checkRowFromD (int edo, const std::string& row)
    {
        const auto cells = split (row, '|');
        REQUIRE (static_cast<int> (cells.size()) <= edo);
        for (int i = 0; i < static_cast<int> (cells.size()); ++i)
        {
            const auto got = spellStep (edo, dPos (edo) + i);
            const auto ok  = split (cells[(size_t) i], '/');
            INFO (edo << "edo, D+" << i << ": got '" << got << "', allowed '" << cells[(size_t) i] << "'");
            CHECK (std::find (ok.begin(), ok.end(), got) != ok.end());
        }
    }
}

TEST_CASE ("U1 analyseEdo: notational fifth and sharpness")
{
    auto a12 = analyseEdo (12); CHECK (a12.valid); CHECK (a12.fifthSteps == 7);  CHECK (a12.sharpSteps == 1);
    auto a24 = analyseEdo (24); CHECK (a24.valid); CHECK (a24.fifthSteps == 14); CHECK (a24.sharpSteps == 2);
    auto a31 = analyseEdo (31); CHECK (a31.valid); CHECK (a31.fifthSteps == 18); CHECK (a31.sharpSteps == 2);
    auto a22 = analyseEdo (22); CHECK (a22.fifthSteps == 13); CHECK (a22.sharpSteps == 3);
    // 13 and 18 use the second-best fifth (13b, 18b) because the best fifth makes m2 descending.
    auto a13 = analyseEdo (13); CHECK (a13.valid); CHECK (a13.fifthSteps == 7);  CHECK (a13.sharpSteps == 3);
    auto a18 = analyseEdo (18); CHECK (a18.valid); CHECK (a18.fifthSteps == 10); CHECK (a18.sharpSteps == 2);
    // Perfect (sharp-0) edos.
    CHECK (analyseEdo (14).sharpSteps == 0);
    CHECK (analyseEdo (21).sharpSteps == 0);
    // Flat-1 edos are reported with positive sharpness (sharp always raises pitch).
    CHECK (analyseEdo (16).sharpSteps == 1);
    CHECK (analyseEdo (23).sharpSteps == 1);
}

TEST_CASE ("U2 degenerate edos fall back to step numbers")
{
    for (int n : { 1, 2, 3, 4, 6, 8 })
    {
        INFO ("edo " << n);
        CHECK_FALSE (analyseEdo (n).valid);
        for (int r = 0; r < n; ++r) CHECK (spellStep (n, r) == std::to_string (r));
    }
    for (int n = 5; n <= kMaxEdo; ++n)
        if (n != 6 && n != 8) { INFO ("edo " << n); CHECK (analyseEdo (n).valid); }
}

TEST_CASE ("U3 12-TET spelling is exactly the existing key list (sharps)")
{
    const std::vector<std::string> expect { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    for (int r = 0; r < 12; ++r) CHECK (spellStep (12, r) == expect[(size_t) r]);
}

TEST_CASE ("U4 wiki tables edos 12-24 (from D)")
{
    checkRowFromD (12, "D|D#/Eb|E|F|F#/Gb|G|G#/Ab|A|A#/Bb|B|C|C#/Db");
    checkRowFromD (13, "D|E|^E/Fb|vE#/^Fb|E#/vF|F|G|A|B|^B/Cb|vB#/^Cb|B#/vC|C");
    checkRowFromD (14, "D|^D/vE|E|^E/vF|F|^F/vG|G|^G/vA|A|^A/vB|B|^B/vC|C|^C/vD");
    checkRowFromD (15, "D|^D|vE|E/F|^F|vG|G|^G|vA|A|^A|vB|B/C|^C|vD");
    checkRowFromD (16, "D|D#/Eb|E|E#|Fb|F|F#/Gb|G|G#/Ab|A|A#/Bb|B|B#|Cb|C|C#/Db");
    checkRowFromD (17, "D|^D/Eb|D#/vE|E|F|^F/Gb|F#/vG|G|^G/Ab|G#/vA|A|^A/Bb|A#/vB|B|C|^C/Db|C#/vD");
    checkRowFromD (18, "D|^D/vE|E|^E|E#/Fb|vF|F|^F/vG|G|^G/vA|A|^A/vB|B|^B|B#/Cb|vC|C|^C/vD");
    checkRowFromD (19, "D|D#|Eb|E|E#/Fb|F|F#|Gb|G|G#|Ab|A|A#|Bb|B|B#/Cb|C|C#|Db");
    checkRowFromD (20, "D|^D|^^D/vvE|vE|E/F|^F|^^F/vvG|vG|G|^G|^^G/vvA|vA|A|^A|^^A/vvB|vB|B/C|^C|^^C/vvD|vD");
    checkRowFromD (21, "D|^D|vE|E|^E|vF|F|^F|vG|G|^G|vA|A|^A|vB|B|^B|vC|C|^C|vD");
    checkRowFromD (22, "D|^D/Eb|vD#/^Eb|D#/vE|E|F|^F/Gb|vF#/^Gb|F#/vG|G|^G/Ab|vG#/^Ab|G#/vA|A");
    checkRowFromD (23, "D|D#|Eb|E|E#|Ex/Fbb|Fb|F|F#|Gb|G|G#|Ab|A|A#|Bb|B|B#|Bx/Cbb|Cb|C|C#|Db");
    checkRowFromD (24, "D|^D/vEb|D#/Eb|^D#/vE|E|^E/vF|F|^F|F#/Gb|vG|G|^G/vAb|G#/Ab|^G#/vA|A");
}

TEST_CASE ("U5 13edo cross-check against the 13edo page (narrow fifth, major wider)")
{
    checkRowFromD (13, "D|^D/E|^E/Fb|vE#/^Fb|E#/vF|F/vG|G/vA|^G/A|^A/B|^B/Cb|vB#/^Cb|B#/vC|C/vD");
}

TEST_CASE ("U6 31edo regression vector (spec-derived, not wiki-sourced)")
{
    const std::vector<std::string> expect { "C","^C","C#","Db","vD","D" };
    for (int r = 0; r < 6; ++r) CHECK (spellStep (31, r) == expect[(size_t) r]);
}

TEST_CASE ("U7 properties for every valid edo in range")
{
    for (int n = kMinEdo; n <= kMaxEdo; ++n)
    {
        if (! analyseEdo (n).valid) continue;
        INFO ("edo " << n);
        std::set<std::string> names;
        for (int r = 0; r < n; ++r)
        {
            const auto s = spellStep (n, r);
            CHECK_FALSE (s.empty());
            CHECK (s.size() <= 8);                 // must fit on a fret label
            names.insert (s);
            CHECK (spellStep (n, r + n) == s);     // octave periodic
            CHECK (spellStep (n, r - n) == s);     // negative steps reduce correctly
        }
        CHECK (static_cast<int> (names.size()) == n); // distinct steps -> distinct names
        CHECK (spellStep (n, 0) == "C");
    }
}

TEST_CASE ("U8 nameFrequency: nearest lattice note, SPN octave, cents")
{
    auto a = nameFrequency (440.0, 440.0, 12);
    CHECK (a.name == "A"); CHECK (a.octave == 4); CHECK (a.stepFromA4 == 0); CHECK (a.cents == doctest::Approx (0.0).epsilon (1e-9));

    auto c4 = nameFrequency (261.6255653, 440.0, 12);
    CHECK (c4.name == "C"); CHECK (c4.octave == 4); CHECK (c4.stepFromA4 == -9);

    auto b3 = nameFrequency (246.94, 440.0, 12);
    CHECK (b3.name == "B"); CHECK (b3.octave == 3);

    auto e2_31 = nameFrequency (82.41, 440.0, 31);
    CHECK (e2_31.name == "E"); CHECK (e2_31.octave == 2); CHECK (e2_31.stepFromA4 == -75);
    CHECK (e2_31.cents == doctest::Approx (3.29).epsilon (0.01));

    auto e2_13 = nameFrequency (82.41, 440.0, 13);   // 12-TET E is off-lattice in 13edo
    CHECK (e2_13.stepFromA4 == -31);
    CHECK (e2_13.cents == doctest::Approx (-38.40).epsilon (0.01));

    auto ref432 = nameFrequency (432.0, 432.0, 19);  // reference frequency is honoured
    CHECK (ref432.name == "A"); CHECK (ref432.octave == 4); CHECK (std::abs (ref432.cents) < 1e-6);

    auto ji = nameFrequency (261.6255653 * 5.0 / 4.0, 440.0, 12); // 5-limit JI major third
    CHECK (ji.name == "E"); CHECK (ji.cents == doctest::Approx (-13.69).epsilon (0.01));
}

TEST_CASE ("U9 nameFrequency rejects garbage without crashing")
{
    for (double bad : { 0.0, -1.0, std::nan (""), std::numeric_limits<double>::infinity() })
    {
        auto p = nameFrequency (bad, 440.0, 12);
        CHECK (p.name == "?");
    }
    CHECK (nameFrequency (440.0, 440.0, 0).name == "?");
    CHECK (nameFrequency (440.0, 440.0, kMaxEdo + 1).name == "?");
}

TEST_CASE ("U10 formatCents")
{
    CHECK (formatCents (0.0)   == "");
    CHECK (formatCents (0.49)  == "");
    CHECK (formatCents (3.29)  == "+3\xC2\xA2");
    CHECK (formatCents (-38.4) == "-38\xC2\xA2");
}

TEST_CASE ("U11 parseTuningText: free numeric entry and presets")
{
    auto edo = [] (const char* t) { auto c = parseTuningText (t); return c && c->mode == TuningChoice::Mode::Edo ? c->steps : -1; };
    CHECK (edo ("13") == 13);
    CHECK (edo ("  13  ") == 13);
    CHECK (edo ("13-EDO") == 13);
    CHECK (edo ("13edo") == 13);
    CHECK (edo ("13 TET") == 13);
    CHECK (edo ("12-TET") == 12);
    CHECK (edo ("24-TET (Quarter Tones)") == 24);
    CHECK (edo ("31-EDO") == 31);
    CHECK (edo ("1") == 1);
    CHECK (edo ("96") == 96);

    auto just = parseTuningText ("5-Limit Just");
    REQUIRE (just); CHECK (just->mode == TuningChoice::Mode::Just5);
    auto just2 = parseTuningText ("just");
    REQUIRE (just2); CHECK (just2->mode == TuningChoice::Mode::Just5);

    for (const char* bad : { "", "   ", "abc", "0", "-5", "97", "1000", "13.5", "1e3", "13 14", "99999999999999999999" })
    {
        INFO ("input '" << bad << "'");
        CHECK_FALSE (parseTuningText (bad).has_value());
    }
}
