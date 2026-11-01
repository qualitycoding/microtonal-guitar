// Boundary tests for issue #5 (U-B1..U-B4): MicrotonalVoice delay-length inputs.
// Pure C++17 — tests FrequencyGuard.h::safeDelayLength(), the single validation seam used by
// MicrotonalVoice::updateFrequency(), plus a small Karplus-Strong simulation of
// MicrotonalVoice::renderNextBlock() to prove finite output and recovery on a later valid note.
//
// Test-first (plan S-020): this file was committed and run RED against the unguarded baseline
// (safeDelayLength absent -> compile failure on baseline; runtime NaN evidence comes from
// integration test I-B1). Re-frozen into tests/FROZEN.sha256 (revision r5).
#include "FrequencyGuard.h"

#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { \
    std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++failures; } } while (0)
#define CHECK_EQ(got, want) do { if ((got) != (want)) { \
    std::printf("FAIL line %d: got %f want %f\n", __LINE__, (double)(got), (double)(want)); ++failures; } } while (0)

// Mirror of MicrotonalVoice::renderNextBlock()'s per-sample loop, driven by a delay length
// produced by the guard, so "finite output" is a property of the real render math.
static bool simulateRenderProducesFiniteOutput (double delayLength, int bufferSize, int numSamples)
{
    std::vector<float> delayLine ((size_t) bufferSize, 0.0f);
    int writePos = 0;
    float previous = 0.0f;
    float noiseState = 0.5f; // deterministic pseudo-noise for the excitation
    int noiseLeft = (int) delayLength;

    for (int n = 0; n < numSamples; ++n)
    {
        float current = 0.0f;
        if (noiseLeft > 0) { current = noiseState; noiseState = -0.7f * noiseState; --noiseLeft; }

        double readPos = writePos - delayLength;
        if (readPos < 0.0) readPos += bufferSize;
        const int i1 = (int) readPos;
        const int i2 = (i1 + 1) % bufferSize;
        const float frac = (float) (readPos - i1);
        const float delayed = (1.0f - frac) * delayLine[(size_t) i1] + frac * delayLine[(size_t) i2];

        const float filtered = 0.5f * (delayed + previous);
        const float next = current + 0.995f * filtered;
        delayLine[(size_t) writePos] = next;
        previous = filtered;
        writePos = (writePos + 1) % bufferSize;

        if (! std::isfinite (next)) return false;
    }
    return true;
}

// U-B1: zero target frequency -> defined silent state (delay length 0.0).
static void testZeroTargetFrequency()
{
    CHECK_EQ (safeDelayLength (48000.0, 0.0, 48001.0), 0.0);
}

// U-B2: negative / NaN / infinite target frequency -> defined silent state.
static void testNonFiniteAndNegativeTargetFrequency()
{
    CHECK_EQ (safeDelayLength (48000.0, -110.0, 48001.0), 0.0);
    CHECK_EQ (safeDelayLength (48000.0, std::nan (""), 48001.0), 0.0);
    CHECK_EQ (safeDelayLength (48000.0, std::numeric_limits<double>::infinity(), 48001.0), 0.0);
    CHECK_EQ (safeDelayLength (48000.0, -std::numeric_limits<double>::infinity(), 48001.0), 0.0);
}

// U-B3: invalid sample rate (zero, negative, NaN, infinite) -> defined silent state.
static void testInvalidSampleRate()
{
    CHECK_EQ (safeDelayLength (0.0, 110.0, 48001.0), 0.0);
    CHECK_EQ (safeDelayLength (-48000.0, 110.0, 48001.0), 0.0);
    CHECK_EQ (safeDelayLength (std::nan (""), 110.0, 48001.0), 0.0);
    CHECK_EQ (safeDelayLength (std::numeric_limits<double>::infinity(), 110.0, 48001.0), 0.0);
}

// U-B4: delay below one sample and delay outside the buffer are rejected; valid input keeps
// its exact value; a later valid call recovers after an invalid one; the simulated render
// produces only finite output with a guarded delay length.
static void testDelayRangeValidationAndRecovery()
{
    // Below one sample: frequency above the sample rate.
    CHECK_EQ (safeDelayLength (48000.0, 96000.0, 48001.0), 0.0);
    CHECK_EQ (safeDelayLength (48000.0, 48000.0, 48001.0), 0.0); // exactly one sample -> too short to read

    // Outside the usable delay buffer: frequency so low the delay line cannot hold it.
    CHECK_EQ (safeDelayLength (48000.0, 1.0, 48001.0), 0.0);            // length 48000 >= 48001? no: 48000 < 48001 OK
    CHECK_EQ (safeDelayLength (48000.0, 0.999, 48001.0), 0.0);          // length ~48048 >= 48001 -> silent
    CHECK_EQ (safeDelayLength (48000.0, 110.0, 48001.0),
              48000.0 / 110.0);                                          // valid input unchanged

    // Recovery on a later valid note: invalid first, then valid returns the exact delay.
    CHECK_EQ (safeDelayLength (48000.0, 0.0, 48001.0), 0.0);
    const double recovered = safeDelayLength (48000.0, 110.0, 48001.0);
    CHECK (std::isfinite (recovered) && recovered >= 1.0 && recovered < 48001.0);
    CHECK_EQ (recovered, 48000.0 / 110.0);

    // Finite output: the real render math driven by guarded delay lengths never goes non-finite.
    CHECK (simulateRenderProducesFiniteOutput (48000.0 / 110.0, 48001, 4096));
    CHECK (! simulateRenderProducesFiniteOutput (
        std::numeric_limits<double>::infinity(), 48001, 64)); // sanity: unguarded would fail
}

int main()
{
    testZeroTargetFrequency();
    testNonFiniteAndNegativeTargetFrequency();
    testInvalidSampleRate();
    testDelayRangeValidationAndRecovery();
    if (failures == 0) std::printf("ALL PASS (U-B1..U-B4)\n");
    return failures == 0 ? 0 : 1;
}
