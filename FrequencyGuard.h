#pragma once
#include <cmath>

// Guards the Karplus-Strong delay-length computation against frequencies that would
// produce an infinite or NaN delay line (issue #5: TuningEngine::getFrequency returns
// 0.0 for an out-of-range string index, and the old code divided by it unguarded).
// Returns 0.0 for any non-finite or non-positive target frequency; renderNextBlock
// already treats delay length <= 0 as "voice silent", so 0.0 is a safe sentinel.
inline double safeDelayLength (double sampleRate, double targetFreq)
{
    if (! std::isfinite (targetFreq) || targetFreq <= 0.0)
        return 0.0;

    const double length = sampleRate / targetFreq;
    return (std::isfinite (length) && length > 0.0) ? length : 0.0;
}
