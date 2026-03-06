/*
  ==============================================================================

    AudioEngine.h
    Created: 6 Mar 2026 12:25:31pm
    Author:  홍준영

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "../ProjectModel.h"
#include "SamplePlayer.h"
#include "Sequencer.h"

class AudioEngine : public juce::AudioIODeviceCallback
{
public:
    AudioEngine();
    ~AudioEngine() override;

    void initialise();
    void shutdown();

    // 재생 제어
    void play();
    void stop();
    void setBPM(double bpm);
    bool isPlaying() const;

    void setPatternStepCount(int stepCount);

    void setPlayMode(PlayMode mode);
    void setProject(Project* projectPtr);

    // 샘플 관리
    void loadSample(int channelIndex, const juce::File& file);
    void triggerChannel(int channelIndex);

    // 스텝 상태 연결 (ChannelRack에서 가져옴)
    void setStepPattern(int channelIndex, int stepIndex, bool active);

    // AudioIODeviceCallback
    void audioDeviceIOCallbackWithContext(
        const float* const* inputChannelData,
        int numInputChannels,
        float* const* outputChannelData,
        int numOutputChannels,
        int numSamples,
        const juce::AudioIODeviceCallbackContext& context) override;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    Sequencer& getSequencer() { return sequencer; }

private:
    juce::AudioDeviceManager deviceManager;
    juce::MixerAudioSource   mixer;

    std::array<SamplePlayer, 16> players;
    Sequencer sequencer;

    Project* project = nullptr;
    PlayMode playMode = PlayMode::Pattern;

    // Song 모드용 타임라인 상태
    bool  songPlaying        = false;
    long  songSamplePosition = 0;
    double bpm               = 140.0;

    void processPatternMode(juce::AudioBuffer<float>& buffer, int numSamples, int numOutputChannels);
    void processSongMode(juce::AudioBuffer<float>& buffer, int numSamples, int numOutputChannels);

    const Pattern* findPatternById(int patternId) const;

    double sampleRate = 44100.0;
    int    bufferSize = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};

