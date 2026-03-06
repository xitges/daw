/*
  ==============================================================================

    Sequencer.h
    Created: 6 Mar 2026 12:02:09pm
    Author:  홍준영

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <functional>

class Sequencer
{
public:
    using TriggerCallback = std::function<void(int channelIndex)>;

    explicit Sequencer(TriggerCallback cb);

    void prepare(double sampleRate, int bufferSize);
    void start();
    void stop();
    void setBPM(double bpm);
    void setStep(int channel, int step, bool active);
    bool isPlaying() const { return playing; }
    int  getCurrentStep() const { return currentStep; }

    // AudioEngine 콜백에서 매 버퍼마다 호출
    void processBlock(int numSamples);

private:
    TriggerCallback onTrigger;

    static constexpr int CHANNEL_COUNT = 16;
    static constexpr int STEP_COUNT    = 16;

    std::array<std::array<bool, STEP_COUNT>, CHANNEL_COUNT> pattern {};

    double sampleRate       = 44100.0;
    double bpm              = 140.0;
    int    currentStep      = 0;
    double samplesPerStep   = 0.0;
    double sampleCounter    = 0.0;
    bool   playing          = false;

    void advanceStep();
    void recalcSamplesPerStep();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Sequencer)
};

