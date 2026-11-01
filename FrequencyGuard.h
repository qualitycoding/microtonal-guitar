// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cmath>

// Validates every input of the Karplus-Strong delay-length computation (issue #5).
//
// TuningEngine::getFrequency() returns 0.0 for an out-of-range string index, and the old code
// divided the sample rate by it unguarded: the delay length became +inf, which the
// `<= 0.0` check in renderNextBlock() cannot catch (read position -> -inf -> undefined
// delay-line indexing, i.e. inf/NaN audio or a crash on the audio thread).
//
// Returns a finite delay length in samples, or 0.0 meaning "defined silent state":
// renderNextBlock() renders silence while the delay length is not a usable finite value.
// Callers must also stop the excitation (noiseSamplesLeft = 0) when 0.0 is returned.
inline double safeDelayLength (double sampleRate, double targetFreq, double delayBufferSize)
{
    // Sample rate must be a usable positive finite value.
    if (! std::isfinite (sampleRate) || sampleRate <= 0.0)
        return 0.0;

    // Target frequency (after the pitch-bend adjustment) must be positive and finite;
    // this covers zero, negative, NaN and infinite inputs.
    if (! std::isfinite (targetFreq) || targetFreq <= 0.0)
        return 0.0;

    const double length = sampleRate / targetFreq;

    // The delay must be at least one sample ...
    if (! std::isfinite (length) || length < 1.0)
        return 0.0;

    // ... and strictly inside the usable delay-buffer range (no buffer -> no delay).
    if (delayBufferSize <= 0.0 || length >= delayBufferSize)
        return 0.0;

    return length;
}
