/*
  ==============================================================================

    SamplePlayer.cpp
    Created: 6 Mar 2026 12:02:03pm
    Author:  홍준영

  ==============================================================================
*/
#include "SamplePlayer.h"

SamplePlayer::SamplePlayer()
{
    formatManager.registerBasicFormats(); // WAV, AIFF, MP3
}

void SamplePlayer::loadFile(const juce::File& file)
{
    reader.reset(formatManager.createReaderFor(file));

    if (reader != nullptr)
    {
        fileBuffer.setSize((int)reader->numChannels,
                           (int)reader->lengthInSamples);
        reader->read(&fileBuffer, 0,
                     (int)reader->lengthInSamples, 0, true, true);
        DBG("Loaded: " << file.getFileName());
    }
}

void SamplePlayer::trigger()
{
    triggered = true;
}

void SamplePlayer::prepare(double sr, int)
{
    playerSampleRate = sr;
    playPosition = -1;
}

void SamplePlayer::reset()
{
    playPosition = -1;
}

void SamplePlayer::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                   int numSamples)
{
    if (!isLoaded()) return;

    // 트리거 감지
    if (triggered.exchange(false))
        playPosition = 0;

    if (playPosition < 0) return;

    int srcSamples  = fileBuffer.getNumSamples();
    int outChannels = outputBuffer.getNumChannels();
    int srcChannels = fileBuffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        if (playPosition >= srcSamples)
        {
            playPosition = -1;
            break;
        }

        for (int ch = 0; ch < outChannels; ++ch)
        {
            int srcCh = ch % srcChannels;
            outputBuffer.addSample(ch, i,
                fileBuffer.getSample(srcCh, playPosition) * gain);
        }

        ++playPosition;
    }
}
