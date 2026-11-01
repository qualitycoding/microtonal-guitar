// Tests for TuningEngine's lock-free read path (issue #4: no mutex on the audio thread).
// Part 1: behavioural equivalence with the previous mutex-based implementation.
// Part 2: multithreaded smoke test - writers and concurrent readers must never crash,
//         never return non-finite values, and converge to the last published state.
#include "TuningEngine.h"
#include <atomic>
#include <cmath>
#include <cstdio>
#include <thread>
#include <vector>

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

int main()
{
    testDefaultBehaviour();
    testEdoAndJust();
    testConcurrentReadWrite();
    if (failures == 0) std::printf ("ALL TUNING ENGINE TESTS PASSED\n");
    else std::printf ("%d FAILURE(S)\n", failures);
    return failures == 0 ? 0 : 1;
}
