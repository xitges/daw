/*
  ==============================================================================

    SamplePlayer.h
    Created: 6 Mar 2026 12:02:03pm
    Author:  홍준영

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class SamplePlayer
{
public:
    SamplePlayer();

    void loadFile(const juce::File& file);
    void trigger();
    void prepare(double sampleRate, int bufferSize);
    void reset();
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int numSamples);

    bool isLoaded() const { return fileBuffer.getNumSamples() > 0; }

private:
    juce::AudioBuffer<float>            fileBuffer;
    juce::AudioFormatManager            formatManager;
    std::unique_ptr<juce::AudioFormatReader> reader;

    double playerSampleRate = 44100.0;
    int    playPosition     = -1;   // -1 = not playing
    float  gain             = 0.8f;

    std::atomic<bool> triggered { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplePlayer)
};
