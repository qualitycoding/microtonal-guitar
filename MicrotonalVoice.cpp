#include "MicrotonalVoice.h"

void MicrotonalVoice::noteStarted()
{
    currentStringIndex = juce::jlimit(0, 5, (int) currentlyPlayingNote.midiChannel - 2);

    currentSampleRate = getSampleRate();
    level = currentlyPlayingNote.noteOnVelocity.asUnsignedFloat();
    tailOff = 0.0;
    
    // Create a buffer large enough for the lowest possible bass notes (e.g., 1 second max delay)
    if (delayBuffer.getNumSamples() == 0)
        delayBuffer.setSize(1, (int)currentSampleRate + 1);
        
    delayBuffer.clear();
    delayWritePosition = 0;
    previousOutput = 0.0f;

    updateFrequency();

    // "Pluck" the string: We will output pure noise for exactly one wavelength
    noiseSamplesLeft = (int)currentDelayLength;
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

void MicrotonalVoice::updateFrequency()
{
    int midiNote = currentlyPlayingNote.initialNote;
    
    // Calculate the fret based on THIS string's open note
    // (Assuming standard MIDI tuning for the open strings as a baseline)
    int openNoteMidi = 40 + (currentStringIndex * 5); // Rough standard tuning approximation
    int fretNumber = std::max(0, midiNote - openNoteMidi); 

    // USE currentStringIndex HERE INSTEAD OF 0
    double targetFreq = tuningEngine->getFrequency(currentStringIndex, fretNumber);

    double bendInSemitones = currentlyPlayingNote.totalPitchbendInSemitones;
    targetFreq *= std::pow(2.0, bendInSemitones / 12.0);

    currentDelayLength = currentSampleRate / targetFreq;
}

void MicrotonalVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (currentDelayLength <= 0.0) return;

    auto* delayData = delayBuffer.getWritePointer(0);
    int delayBufferSize = delayBuffer.getNumSamples();

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

