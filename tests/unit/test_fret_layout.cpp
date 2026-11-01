// FROZEN TEST SUITE — see tests/FROZEN.md. Implementers must not edit, skip or weaken.
#include "../third_party/doctest.h"
#include "NoteNaming.h"

using namespace mtg;

TEST_CASE ("L1 fretCountFor preserves today's presets and generalises")
{
    CHECK (fretCountFor (12) == 24);  // unchanged from PluginProcessor::getFretCount
    CHECK (fretCountFor (24) == 24);  // unchanged
    CHECK (fretCountFor (31) == 31);  // unchanged
    CHECK (fretCountFor (13) == 26);
    CHECK (fretCountFor (5)  == 25);
    CHECK (fretCountFor (7)  == 28);
    CHECK (fretCountFor (1)  == 24);
    CHECK (fretCountFor (72) == 72);
    CHECK (fretCountFor (96) == 96);
    for (int n = kMinEdo; n <= kMaxEdo; ++n)
    {
        INFO ("edo " << n);
        const int f = fretCountFor (n);
        CHECK (f >= 24); CHECK (f % n == 0); CHECK (f - n < 24);
    }
}

TEST_CASE ("L2 label culling threshold")
{
    CHECK (shouldDrawFretLabel (24.0f, 20.0f));
    CHECK_FALSE (shouldDrawFretLabel (23.9f, 20.0f));
    CHECK_FALSE (shouldDrawFretLabel (0.0f, 1.0f));
    CHECK_FALSE (shouldDrawFretLabel (-5.0f, 1.0f));
}
