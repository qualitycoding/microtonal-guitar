// SPDX-License-Identifier: Apache-2.0
// NoteNaming.cpp — ups-and-downs note spelling for arbitrary EDOs, plus frequency-to-name lookup.
// Implements the contract frozen at plan/api/NoteNaming.h and the spelling rule in plan/PLAN.md §3.2.
// No JUCE dependency; unit-tested by tests/unit/test_note_naming.cpp (do not edit that file).
//
// Rule summary (derivation and citations: research/RESEARCH_LOG.md, sections A and E):
//   For N-EDO, the notational fifth f = round(N*log2(1.5)); if that makes the minor 2nd (3N-5f)
//   negative, use the second-best fifth f-1 instead (this is what makes 13-EDO "13b" and 18-EDO "18b").
//   N is degenerate (falls back to plain step numbers) when N in {1,2,3,4,6,8}, or when the fifth is
//   degenerate (2f-N <= 0) or still gives a descending minor 2nd after the f-1 correction.
//   Sharpness s = |7f - 4N|: a written '#' always RAISES pitch by s steps (research R1).
//   The seven naturals sit, unwrapped over one octave, at C=0, D=2f-N, E=4f-2N, F=N-f, G=f, A=3f-N,
//   B=5f-2N, C'=N. To spell step r (0<=r<N): if r lands exactly on a natural, use it. Otherwise take
//   the two naturals bracketing r and, for each, the accidental (-2..+2 steps of s, or just 0 if s==0)
//   and arrow count u = r - (naturalPos + accidental*s) that reaches r; keep the spelling that
//   minimises (|arrows|+|accidental|, |arrows|, flat-before-sharp, down-before-up, earlier-letter).

#include "NoteNaming.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <tuple>

namespace mtg
{
    namespace
    {
        int floorMod (int a, int n) noexcept
        {
            const int r = a % n;
            return r < 0 ? r + n : r;
        }

        struct Candidate
        {
            int cost1 = 0;   // |arrows| + |accidental|
            int cost2 = 0;   // |arrows|
            int cost3 = 0;   // 0 if accidental >= 0 (sharp-ish), 1 if flat-ish — sharps preferred on ties
            int cost4 = 0;   // 0 if arrows >= 0 (up-ish), 1 if down-ish — ups preferred on ties
            int cost5 = 0;   // natural index, earlier letter preferred on ties
            std::string text;

            bool operator< (const Candidate& other) const noexcept
            {
                return std::tie (cost1, cost2, cost3, cost4, cost5)
                     < std::tie (other.cost1, other.cost2, other.cost3, other.cost4, other.cost5);
            }
        };

        const char* accidentalSuffix (int accidental) noexcept
        {
            switch (accidental)
            {
                case -2: return "bb";
                case -1: return "b";
                case  1: return "#";
                case  2: return "x";
                default: return "";
            }
        }

        std::string arrowPrefix (int arrows)
        {
            return std::string (static_cast<size_t> (std::abs (arrows)), arrows > 0 ? '^' : 'v');
        }
    }

    EdoAnalysis analyseEdo (int edo)
    {
        EdoAnalysis result;
        result.edo = edo;

        if (edo < kMinEdo || edo > kMaxEdo) return result;
        if (edo == 1 || edo == 2 || edo == 3 || edo == 4 || edo == 6 || edo == 8) return result;

        int fifth = static_cast<int> (std::lround (edo * std::log2 (1.5)));
        if (3 * edo - 5 * fifth < 0) fifth -= 1; // second-best fifth (13b, 18b) — see research R1

        const bool fifthIsUsable = (2 * fifth - edo) > 0 && (3 * edo - 5 * fifth) >= 0;
        if (! fifthIsUsable) return result;

        result.valid = true;
        result.fifthSteps = fifth;
        result.sharpSteps = std::abs (7 * fifth - 4 * edo);
        return result;
    }

    std::string spellStep (int edo, int stepFromC)
    {
        if (edo < 1) return "?";

        const auto analysis = analyseEdo (edo);
        const int r = floorMod (stepFromC, edo);
        if (! analysis.valid) return std::to_string (r);

        const int f = analysis.fifthSteps;
        const int s = analysis.sharpSteps;

        // Unwrapped natural positions across one octave, in letter order C D E F G A B C'.
        const std::array<int, 8> naturalPos { 0, 2 * f - edo, 4 * f - 2 * edo, edo - f,
                                               f, 3 * f - edo, 5 * f - 2 * edo, edo };
        static const std::array<char, 8> letters { 'C', 'D', 'E', 'F', 'G', 'A', 'B', 'C' };

        int lower = 0, upper = 7;
        for (int i = 0; i < 8; ++i) if (naturalPos[(size_t) i] <= r) lower = i;
        for (int i = 7; i >= 0; --i) if (naturalPos[(size_t) i] >  r) upper = i;

        if (naturalPos[(size_t) lower] == r) return std::string (1, letters[(size_t) lower]);

        bool haveBest = false;
        Candidate best;

        for (const int idx : { lower, upper })
        {
            const int base = naturalPos[(size_t) idx];
            const int accidentalLimit = (s == 0) ? 0 : 2;

            for (int accidental = -accidentalLimit; accidental <= accidentalLimit; ++accidental)
            {
                const int arrows = r - (base + accidental * s);

                Candidate candidate;
                candidate.cost1 = std::abs (arrows) + std::abs (accidental);
                candidate.cost2 = std::abs (arrows);
                candidate.cost3 = accidental >= 0 ? 0 : 1;
                candidate.cost4 = arrows >= 0 ? 0 : 1;
                candidate.cost5 = idx;
                candidate.text  = arrowPrefix (arrows) + letters[(size_t) idx] + accidentalSuffix (accidental);

                if (! haveBest || candidate < best) { best = candidate; haveBest = true; }
            }
        }

        return best.text;
    }

    PitchName nameFrequency (double hz, double a4Hz, int edo)
    {
        PitchName result;

        if (! (hz > 0.0) || ! std::isfinite (hz) || ! (a4Hz > 0.0) || ! std::isfinite (a4Hz)
            || edo < kMinEdo || edo > kMaxEdo)
            return result;

        const double centsFromA4 = 1200.0 * std::log2 (hz / a4Hz);
        const int stepFromA4 = static_cast<int> (std::lround (centsFromA4 * edo / 1200.0));

        const auto analysis = analyseEdo (edo);
        const int aPositionInC = analysis.valid ? floorMod (3 * analysis.fifthSteps, edo) : 0; // A is 3 fifths above C

        const int stepFromC = stepFromA4 + aPositionInC;

        result.name = spellStep (edo, stepFromC);
        result.octave = 4 + static_cast<int> (std::floor (static_cast<double> (stepFromC) / edo));
        result.cents = centsFromA4 - stepFromA4 * (1200.0 / edo);
        result.stepFromA4 = stepFromA4;
        return result;
    }

    std::string formatCents (double cents)
    {
        const long rounded = std::lround (cents);
        if (rounded == 0) return "";
        return (rounded > 0 ? "+" : "") + std::to_string (rounded) + "\xC2\xA2"; // U+00A2 CENT SIGN, UTF-8
    }

    namespace
    {
        std::string toLower (const std::string& text)
        {
            std::string out (text.size(), '\0');
            std::transform (text.begin(), text.end(), out.begin(),
                             [] (unsigned char c) { return static_cast<char> (std::tolower (c)); });
            return out;
        }

        std::string trim (const std::string& text)
        {
            const auto first = text.find_first_not_of (" \t\r\n");
            if (first == std::string::npos) return {};
            const auto last = text.find_last_not_of (" \t\r\n");
            return text.substr (first, last - first + 1);
        }
    }

    std::optional<TuningChoice> parseTuningText (const std::string& text)
    {
        const std::string lower = toLower (trim (text));
        if (lower.empty()) return std::nullopt;

        if (lower.find ("just") != std::string::npos)
            return TuningChoice { TuningChoice::Mode::Just5, 12 };

        // Leading run of digits is the step count; everything after must be blank, "edo", "tet",
        // or a preset label starting with one of those (e.g. "24-TET (Quarter Tones)").
        size_t digitsEnd = 0;
        while (digitsEnd < lower.size() && std::isdigit (static_cast<unsigned char> (lower[digitsEnd])))
            ++digitsEnd;
        if (digitsEnd == 0 || digitsEnd > 3) return std::nullopt; // no digits, or > 96

        std::string rest = lower.substr (digitsEnd);
        size_t restStart = 0;
        while (restStart < rest.size()
               && (std::isspace (static_cast<unsigned char> (rest[restStart])) || rest[restStart] == '-'))
            ++restStart;
        rest = rest.substr (restStart);

        const bool restIsBlank = rest.empty();
        const bool restIsEdo   = rest.rfind ("edo", 0) == 0;
        const bool restIsTet   = rest.rfind ("tet", 0) == 0;
        if (! (restIsBlank || restIsEdo || restIsTet)) return std::nullopt;

        // After "edo"/"tet", only whitespace or a parenthetical label may follow (e.g. "(Quarter Tones)").
        if (restIsEdo || restIsTet)
        {
            std::string tail = rest.substr (3);
            size_t tailStart = 0;
            while (tailStart < tail.size() && std::isspace (static_cast<unsigned char> (tail[tailStart])))
                ++tailStart;
            if (tailStart < tail.size() && tail[tailStart] != '(') return std::nullopt;
        }

        const int steps = std::atoi (lower.substr (0, digitsEnd).c_str());
        if (steps < kMinEdo || steps > kMaxEdo) return std::nullopt;

        return TuningChoice { TuningChoice::Mode::Edo, steps };
    }

    int fretCountFor (int edo)
    {
        if (edo < 1) return 24;
        return ((24 + edo - 1) / edo) * edo; // smallest multiple of edo that is >= 24
    }

    bool shouldDrawFretLabel (float slotWidthPx, float labelWidthPx)
    {
        return slotWidthPx > 0.0f && slotWidthPx >= labelWidthPx + 4.0f;
    }
}
