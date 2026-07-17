#pragma once
#include "JuceHeader.h"
#include "TuningEngine.h"
#include <random>
#include <atomic> // Added for thread-safe UI communication

class MicrotonalVoice : public juce::MPESynthesiserVoice
{
public:
    MicrotonalVoice(TuningEngine* enginePtr) 
        : tuningEngine(enginePtr) {}

    void noteStarted() override;
    void noteStopped (bool allowTailOff) override;
    void notePitchbendChanged() override;
    void notePressureChanged() override {}
    void noteTimbreChanged() override {}
    void noteKeyStateChanged() override {}
    
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    // Thread-safe getters for the GUI thread
    int getStringIndex() const { return currentStringIndex.load(); }
    int getFretNumber() const { return currentFretNumber.load(); }

private:
    TuningEngine* tuningEngine;
    
    // Changed to atomics to prevent threading race conditions
    std::atomic<int> currentStringIndex{ 0 };
    std::atomic<int> currentFretNumber{ 0 };
    
    double currentSampleRate = 44100.0;
    float level = 0.0f;
    float tailOff = 0.0f;

    // --- Karplus-Strong Variables ---
    juce::AudioBuffer<float> delayBuffer;
    int delayWritePosition = 0;
    double currentDelayLength = 0.0;
    
    int noiseSamplesLeft = 0;
    float previousOutput = 0.0f;
    
    // Random number generator for the "Pluck"
    std::mt19937 randomGenerator;
    std::uniform_real_distribution<float> randomDistribution{ -1.0f, 1.0f };

    void updateFrequency();

    int myStringIndex;
};