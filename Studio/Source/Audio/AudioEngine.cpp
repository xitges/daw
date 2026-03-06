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
    sequencer.start();
}

void AudioEngine::stop()
{
    sequencer.stop();
}

void AudioEngine::setBPM(double bpm)
{
    sequencer.setBPM(bpm);
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
    sequencer.processBlock(numSamples); 
    // 각 SamplePlayer 믹스다운
    for (auto& player : players)
        player.renderNextBlock(buffer, numSamples);
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

