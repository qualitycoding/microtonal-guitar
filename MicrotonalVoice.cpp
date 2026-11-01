// SPDX-License-Identifier: Apache-2.0
#include "MicrotonalVoice.h"
#include "FrequencyGuard.h"
#include <cmath>

void MicrotonalVoice::noteStarted()
{
    // Store the string index atomically
    currentStringIndex.store (juce::jlimit (0, TuningEngine::kMaxStrings - 1,
                                             (int) currentlyPlayingNote.midiChannel - 2));

    currentSampleRate = getSampleRate();
    level = currentlyPlayingNote.noteOnVelocity.asUnsignedFloat();
    tailOff = 0.0;
    
    if (delayBuffer.getNumSamples() == 0)
        delayBuffer.setSize(1, (int)currentSampleRate + 1);
        
    delayBuffer.clear();
    delayWritePosition = 0;
    previousOutput = 0.0f;

    updateFrequency();

    noiseSamplesLeft = (int)currentDelayLength;
}

void MicrotonalVoice::updateFrequency()
{
    const int stringIndex = currentStringIndex.load();
    const int midiNote = currentlyPlayingNote.initialNote;
    // Reads the same per-string MIDI table the router and the UI use (TuningStructures.h), so a
    // fret computed here always matches what's shown and what routeed the note to this string.
    const int openNoteMidi = tuningEngine->getOpenStringMidiNote (stringIndex);
    const int fretNumber = std::max (0, midiNote - (openNoteMidi >= 0 ? openNoteMidi : midiNote));

    currentFretNumber.store(fretNumber);   // <-- this was missing; it's what getFretNumber() actually returns

    double targetFreq = tuningEngine->getFrequency (stringIndex, fretNumber);
    double bendInSemitones = currentlyPlayingNote.totalPitchbendInSemitones;
    targetFreq *= std::pow(2.0, bendInSemitones / 12.0);

    // Guarded (issue #5): validates the sample rate, the bend-adjusted target frequency and
    // the computed delay length (>= 1 sample, strictly inside the delay buffer). An invalid
    // state is a DEFINED silent state: zero delay length stops the render loop and the
    // excitation is cleared. A later valid note (or bend) recomputes everything here.
    currentDelayLength = safeDelayLength (currentSampleRate, targetFreq,
                                          (double) delayBuffer.getNumSamples());
    if (currentDelayLength == 0.0)
        noiseSamplesLeft = 0;
}

void MicrotonalVoice::noteStopped(bool allowTailOff)
{
    if (allowTailOff)
    {
        if (tailOff == 0.0) tailOff = 1.0;
    }
    else
    {
        clearCurrentNote();
    }
}

void MicrotonalVoice::notePitchbendChanged()
{
    updateFrequency(); 
}

void MicrotonalVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    // Defense in depth (issue #5): whatever state the voice is in, the render loop may only
    // ever run with a finite delay length of at least one sample, strictly inside the delay
    // buffer. Any other state renders silence (the excitation was already cleared in
    // updateFrequency); a tail-off is still allowed to finish so the voice can clear.
    const int delayBufferSize = delayBuffer.getNumSamples();
    const bool silent = delayBufferSize == 0
                        || ! std::isfinite (currentDelayLength)
                        || currentDelayLength < 1.0
                        || currentDelayLength >= (double) delayBufferSize;

    if (silent)
    {
        if (tailOff > 0.0)
        {
            tailOff *= 0.99;
            if (tailOff <= 0.005) clearCurrentNote();
        }
        return;
    }

    auto* delayData = delayBuffer.getWritePointer(0);

    while (--numSamples >= 0)
    {
        float currentSample = 0.0f;

        // 1. Generate the Pluck (Excitation)
        if (noiseSamplesLeft > 0)
        {
            currentSample = randomDistribution(randomGenerator) * level;
            noiseSamplesLeft--;
        }

        // 2. Read from the Delay Line using Fractional Delay (Linear Interpolation)
        // Find out where the read head is, wrapping around the circular buffer
        double readPosition = delayWritePosition - currentDelayLength;
        if (readPosition < 0.0) readPosition += delayBufferSize;

        int index1 = (int)readPosition;
        int index2 = (index1 + 1) % delayBufferSize;
        float fraction = (float)(readPosition - index1);

        float delayedSample = (1.0f - fraction) * delayData[index1] + fraction * delayData[index2];

        // 3. The Filter & Feedback
        // Simple low-pass filter: average the current delayed sample with the previous one
        // Multiply by 0.995 to simulate string decay (feedback)
        float feedback = 0.995f; 
        float filteredSample = 0.5f * (delayedSample + previousOutput);
        float feedbackSample = filteredSample * feedback;

        // 4. Combine Pluck and Feedback, then write back into the buffer
        float newSample = currentSample + feedbackSample;
        delayData[delayWritePosition] = newSample;
        
        // Output the sound to the DAW
        float outSample = newSample;

        // Handle Note Off fadeout
        if (tailOff > 0.0) 
        {
            outSample *= tailOff;
            tailOff *= 0.99; // Fast dampening (like muting the string with your hand)
            if (tailOff <= 0.005) { clearCurrentNote(); break; }
        }

        for (int i = outputBuffer.getNumChannels(); --i >= 0;)
            outputBuffer.addSample(i, startSample, outSample);

        // Advance the buffers
        previousOutput = filteredSample;
        delayWritePosition = (delayWritePosition + 1) % delayBufferSize;
        ++startSample;
    }
}

