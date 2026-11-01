// Tests for TuningEngine's lock-free read path (issue #4: no mutex on the audio thread).
// Part 1: behavioural equivalence with the previous mutex-based implementation.
// Part 2: multithreaded smoke test - writers and concurrent readers must never crash,
//         never return non-finite values, and converge to the last published state.
#include "TuningEngine.h"
#include "FrequencyGuard.h"
#include <atomic>
#include <cmath>
#include <cstdio>
#include <limits>
#include <thread>
#include <vector>

static const double dblInf = std::numeric_limits<double>::infinity();
static const double dblNan = std::numeric_limits<double>::quiet_NaN();

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { \
    std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++failures; } } while (0)
#define CHECK_NEAR(got, want, tol) do { if (std::fabs((got) - (want)) > (tol)) { \
    std::printf("FAIL line %d: got %.9f want %.9f\n", __LINE__, (double)(got), (double)(want)); ++failures; } } while (0)

static GuitarState makeState (int numStrings, TuningSystem tuning)
{
    GuitarState s;
    s.numStrings = numStrings;
    s.referenceFrequency = 440.0;
    s.openStringMidiNotes = { 40, 45, 50, 55, 59, 64, 67, 71, 76, 81, 86, 91, 96, 100, 105 };
    s.openStringFrequencies = { 82.41, 110.00, 146.83, 196.00, 246.94, 329.63,
                                82.41 * 2, 110.0 * 2, 146.83 * 2, 196.0 * 2, 246.94 * 2,
                                329.63 * 2, 82.41 * 4, 110.0 * 4, 146.83 * 4 };
    s.currentTuning = tuning;
    return s;
}

static void testDefaultBehaviour()
{
    TuningEngine e;
    CHECK(e.getNumStrings() == 6);
    CHECK_NEAR(e.getFrequency(0, 12), 82.41 * 2.0, 1e-9);          // 12-EDO octave
    CHECK_NEAR(e.getFrequency(0, 7), 82.41 * std::pow(2.0, 7.0/12.0), 1e-9); // fifth
    CHECK(e.getFrequency(99, 0) == 0.0);                            // invalid index
    CHECK(e.getFrequency(-1, 0) == 0.0);                            // invalid index
    CHECK(e.getOpenStringMidiNote(0) == 40);
    CHECK(e.getOpenStringMidiNote(5) == 64);
    CHECK(e.getOpenStringMidiNote(99) == -1);
    CHECK(e.getCurrentTuningSystem().stepsPerOctave == 12);
    CHECK(e.getSnapshot().openStringFrequencies.size() == 6);
}

static void testEdoAndJust()
{
    TuningEngine e;
    e.setGuitarState (makeState (6, TuningEngine::createEdo (31)));
    CHECK_NEAR(e.getFrequency(0, 31), 82.41 * 2.0, 1e-9);           // 31-EDO octave

    e.setGuitarState (makeState (6, TuningEngine::createEdo (24)));
    CHECK_NEAR(e.getFrequency(0, 2), 82.41 * std::pow(2.0, 1.0/12.0), 1e-9);

    e.setGuitarState (makeState (6, TuningEngine::create5LimitJust()));
    CHECK_NEAR(e.getFrequency(0, 7), 82.41 * 1.5, 1e-9);            // 3/2 fifth

    e.setGuitarState (makeState (15, TuningEngine::createEdo (13)));
    CHECK(e.getNumStrings() == 15);
    CHECK_NEAR(e.getFrequency(14, 13), 146.83 * 4 * 2.0, 1e-9);     // top string octave
}

static void testConcurrentReadWrite()
{
    TuningEngine e;

    std::atomic<bool> stop { false };
    std::atomic<int> badReads { 0 };
    std::atomic<long> readCount { 0 };

    // 3 reader threads hammer the audio-thread entry points
    std::vector<std::thread> readers;
    for (int t = 0; t < 3; ++t)
    {
        readers.emplace_back ([&]
        {
            while (!stop.load (std::memory_order_relaxed))
            {
                const double f = e.getFrequency (2, 7);
                if (!std::isfinite (f) || f <= 0.0) ++badReads;
                if (e.getNumStrings() < 4 || e.getNumStrings() > 15) ++badReads;
                if (e.getOpenStringMidiNote (0) < 0) ++badReads;
                readCount.fetch_add (1, std::memory_order_relaxed);
            }
        });
    }

    // Writer alternates between two full states (as updateGuitarState does on parameter change)
    for (int i = 0; i < 20000; ++i)
    {
        e.setGuitarState (makeState (6, TuningEngine::createEdo (12)));
        e.setGuitarState (makeState (15, TuningEngine::createEdo (31)));
    }

    stop.store (true, std::memory_order_relaxed);
    for (auto& r : readers) r.join();

    CHECK(badReads.load() == 0);
    CHECK(readCount.load() > 1000); // readers were genuinely concurrent
    CHECK(e.getNumStrings() == 15); // converged to the last published state
    CHECK_NEAR(e.getFrequency (2, 31), 146.83 * 2.0, 1e-9); // string 2 = D3, octave up
}

// Issue #7: the A4 reference frequency must measurably retune output. PluginProcessor derives
// each open-string frequency as refFreq * 2^((midi-69)/12) (updateGuitarState) and the engine
// multiplies by the tuning ratio; this locks the end-to-end contract at engine level.
static GuitarState makeStateWithRefFreq (double refFreq, TuningSystem tuning)
{
    GuitarState s;
    s.numStrings = 6;
    s.referenceFrequency = refFreq;
    s.openStringMidiNotes = { 40, 45, 50, 55, 59, 64 }; // E2 A2 D3 G3 B3 E4
    for (int midi : s.openStringMidiNotes)
        s.openStringFrequencies.push_back (refFreq * std::pow (2.0, (midi - 69) / 12.0));
    s.currentTuning = std::move (tuning);
    return s;
}

static void testReferenceFrequencyRetunes()
{
    // 12-EDO: known note, fixed string/fret pair -> frequency scales exactly with refFreq.
    TuningEngine e440, e432;
    e440.setGuitarState (makeStateWithRefFreq (440.0, TuningEngine::createEdo (12)));
    e432.setGuitarState (makeStateWithRefFreq (432.0, TuningEngine::createEdo (12)));

    CHECK_NEAR (e440.getFrequency (0, 0), 82.4068892282, 1e-6); // E2 at A4=440
    CHECK_NEAR (e432.getFrequency (0, 0), 82.4068892282 * 432.0 / 440.0, 1e-6);

    // Fret pitch also scales: string 1 (A2), fret 12, one octave up, both tunings.
    CHECK_NEAR (e440.getFrequency (1, 12), 220.0, 1e-6);
    CHECK_NEAR (e432.getFrequency (1, 12), 220.0 * 432.0 / 440.0, 1e-6);
    CHECK_NEAR (e432.getFrequency (1, 12) / e440.getFrequency (1, 12), 432.0 / 440.0, 1e-12);

    // 5-Limit Just must retune identically (ratio path, not just the EDO pow path).
    TuningEngine j440, j415;
    j440.setGuitarState (makeStateWithRefFreq (440.0, TuningEngine::create5LimitJust()));
    j415.setGuitarState (makeStateWithRefFreq (415.0, TuningEngine::create5LimitJust()));
    CHECK_NEAR (j440.getFrequency (0, 7), 82.4068892282 * 1.5, 1e-6);       // perfect fifth
    CHECK_NEAR (j415.getFrequency (0, 7), 82.4068892282 * 1.5 * 415.0 / 440.0, 1e-6);
    CHECK_NEAR (j415.getFrequency (0, 7) / j440.getFrequency (0, 7), 415.0 / 440.0, 1e-12);
}

// Issue #5: the delay-length computation feeding the Karplus-Strong loop must reject
// zero/negative/non-finite frequencies instead of producing inf/NaN delay lengths.
static void testSafeDelayLength()
{
    // 48001 = the voice's delay-buffer size at 44.1/48 kHz (sampleRate + 1 samples).
    const double buf = 48001.0;
    CHECK_NEAR (safeDelayLength (44100.0, 220.0, buf), 200.454545454545, 1e-9); // normal path unchanged
    CHECK (safeDelayLength (44100.0, 0.0, buf) == 0.0);    // engine's invalid-index answer
    CHECK (safeDelayLength (44100.0, -5.0, buf) == 0.0);   // defensive: negative frequency
    CHECK (safeDelayLength (44100.0, dblInf, buf) == 0.0);    // inf frequency -> inf length
    CHECK (safeDelayLength (44100.0, dblNan, buf) == 0.0);    // NaN must never propagate
    CHECK (safeDelayLength (44100.0, 1e-308, buf) == 0.0); // length overflows to +inf
    CHECK (safeDelayLength (44100.0, 220.0, 0.0) == 0.0);  // no delay buffer yet
    CHECK (safeDelayLength (dblNan, 220.0, buf) == 0.0);   // invalid sample rate
    CHECK (safeDelayLength (44100.0, 100000.0, buf) == 0.0);  // delay < 1 sample
    CHECK (safeDelayLength (44100.0, 0.9, buf) == 0.0);       // delay outside the buffer

    // And the engine really does return 0.0 for indices the voice must treat as silent:
    TuningEngine e;
    CHECK (e.getFrequency (-1, 0) == 0.0);
    CHECK (e.getFrequency (TuningEngine::kMaxStrings, 0) == 0.0);
    CHECK (safeDelayLength (44100.0, e.getFrequency (-1, 0), buf) == 0.0);
}

int main()
{
    testDefaultBehaviour();
    testEdoAndJust();
    testConcurrentReadWrite();
    testReferenceFrequencyRetunes();
    testSafeDelayLength();
    if (failures == 0) std::printf ("ALL TUNING ENGINE TESTS PASSED\n");
    else std::printf ("%d FAILURE(S)\n", failures);
    return failures == 0 ? 0 : 1;
}
