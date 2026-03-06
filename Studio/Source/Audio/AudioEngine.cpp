/*
  ==============================================================================

    AudioEngine.cpp
    Created: 6 Mar 2026 12:25:31pm
    Author:  홍준영

  ==============================================================================
*/

#include "AudioEngine.h"

AudioEngine::AudioEngine()
    : sequencer([this](int ch) { triggerChannel(ch); })
{
}

AudioEngine::~AudioEngine()
{
    shutdown();
}

void AudioEngine::initialise()
{
    auto err = deviceManager.initialiseWithDefaultDevices(0, 2);
    if (err.isNotEmpty())
        DBG("AudioDeviceManager error: " << err);

    deviceManager.addAudioCallback(this);
}

void AudioEngine::shutdown()
{
    deviceManager.removeAudioCallback(this);
    deviceManager.closeAudioDevice();
}

void AudioEngine::play()
{
    if (playMode == PlayMode::Pattern)
    {
        sequencer.start();
    }
    else
    {
        songSamplePosition = 0;
        songPlaying        = true;
    }
}

void AudioEngine::stop()
{
    if (playMode == PlayMode::Pattern)
    {
        sequencer.stop();
    }
    else
    {
        songPlaying = false;
    }
}

void AudioEngine::setBPM(double bpm)
{
    this->bpm = bpm;
    sequencer.setBPM(bpm);
}

bool AudioEngine::isPlaying() const
{
    if (playMode == PlayMode::Pattern)
        return sequencer.isPlaying();

    return songPlaying;
}

void AudioEngine::setPlayMode(PlayMode mode)
{
    playMode = mode;
}

void AudioEngine::setProject(Project* projectPtr)
{
    project = projectPtr;
}

void AudioEngine::setPatternStepCount(int stepCount)
{
    sequencer.setStepCount(stepCount);
}

void AudioEngine::loadSample(int channelIndex, const juce::File& file)
{
    if (channelIndex >= 0 && channelIndex < 16)
        players[channelIndex].loadFile(file);
}

void AudioEngine::triggerChannel(int channelIndex)
{
    if (channelIndex >= 0 && channelIndex < 16)
        players[channelIndex].trigger();
}

void AudioEngine::setStepPattern(int channelIndex, int stepIndex, bool active)
{
    sequencer.setStep(channelIndex, stepIndex, active);
}

void AudioEngine::audioDeviceIOCallbackWithContext(
    const float* const*,
    int,
    float* const* outputChannelData,
    int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext&)
{
    juce::AudioBuffer<float> buffer(outputChannelData, numOutputChannels, numSamples);
    buffer.clear();

    if (playMode == PlayMode::Pattern)
    {
        processPatternMode(buffer, numSamples, numOutputChannels);
    }
    else
    {
        processSongMode(buffer, numSamples, numOutputChannels);
    }
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    sampleRate = device->getCurrentSampleRate();
    bufferSize = device->getCurrentBufferSizeSamples();

    sequencer.prepare(sampleRate, bufferSize);

    for (auto& player : players)
        player.prepare(sampleRate, bufferSize);
}

void AudioEngine::audioDeviceStopped()
{
    for (auto& player : players)
        player.reset();
}

void AudioEngine::processPatternMode(juce::AudioBuffer<float>& buffer,
                                     int numSamples,
                                     int /*numOutputChannels*/)
{
    sequencer.processBlock(numSamples);

    for (auto& player : players)
        player.renderNextBlock(buffer, numSamples);
}

void AudioEngine::processSongMode(juce::AudioBuffer<float>& buffer,
                                  int numSamples,
                                  int /*numOutputChannels*/)
{
    if (!songPlaying || project == nullptr || sampleRate <= 0.0)
        return;

    if (project->playlistClips.empty() || project->patterns.empty())
        return;

    const double samplesPerBeat = sampleRate * 60.0 / bpm;
    const double beatsPerBar    = 4.0;
    const double samplesPerBar  = samplesPerBeat * beatsPerBar;

    auto barFromSample = [samplesPerBar](long s)
    {
        return (double)s / samplesPerBar;
    };

    long startSample = songSamplePosition;
    long endSample   = songSamplePosition + numSamples;

    double startBar = barFromSample(startSample);
    double endBar   = barFromSample(endSample);

    // 타임라인 상에서 16분음표 단위로 스텝 인덱스를 계산
    const int timelineStepsPerBar = 16;
    int startStepIndex = (int)std::floor(startBar * timelineStepsPerBar);
    int endStepIndex   = (int)std::floor(endBar   * timelineStepsPerBar);

    for (int stepIndex = startStepIndex; stepIndex <= endStepIndex; ++stepIndex)
    {
        double stepBarPos  = (double)stepIndex / (double)timelineStepsPerBar;
        long   stepSample  = (long)(stepBarPos * samplesPerBar);
        int    offsetInBuf = (int)(stepSample - startSample);

        if (offsetInBuf < 0 || offsetInBuf >= numSamples)
            continue;

        // 이 스텝에 해당하는 클립들 찾기
        for (const auto& clip : project->playlistClips)
        {
            if (stepBarPos < clip.startBar ||
                stepBarPos >= clip.startBar + clip.lengthBars)
                continue;

            const Pattern* pattern = findPatternById(clip.patternId);
            if (pattern == nullptr || pattern->stepCount <= 0)
                continue;

            // 패턴 전체 길이 (bar 단위, 16분음표 기준)
            const double patternBars = (double)pattern->stepCount / (double)timelineStepsPerBar;
            if (patternBars <= 0.0)
                continue;

            const double localBarInClip = stepBarPos - clip.startBar; // 0.0 ~ lengthBars

            // 패턴 내에서의 0~1 위치
            double localInPatternBars = std::fmod(juce::jmax(0.0, localBarInClip), patternBars);
            double pos01              = localInPatternBars / patternBars;

            int localStep = (int)std::floor(pos01 * (double)pattern->stepCount);
            localStep     = juce::jlimit(0, pattern->stepCount - 1, localStep);

            for (int ch = 0; ch < Pattern::kMaxChannels; ++ch)
            {
                if (pattern->steps[ch][localStep])
                    players[ch].trigger();
            }
        }
    }

    for (auto& player : players)
        player.renderNextBlock(buffer, numSamples);

    songSamplePosition += numSamples;
}

const Pattern* AudioEngine::findPatternById(int patternId) const
{
    if (project == nullptr)
        return nullptr;

    for (const auto& p : project->patterns)
        if (p.id == patternId)
            return &p;

    return nullptr;
}

