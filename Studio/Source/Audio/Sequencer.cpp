/*
  ==============================================================================

    Sequencer.cpp
    Created: 6 Mar 2026 12:02:09pm
    Author:  홍준영

  ==============================================================================
*/

#include "Sequencer.h"

Sequencer::Sequencer(TriggerCallback cb)
    : onTrigger(std::move(cb))
{
}

void Sequencer::prepare(double sr, int)
{
    sampleRate = sr;
    recalcSamplesPerStep();
}

void Sequencer::start()
{
    currentStep   = 0;
    sampleCounter = 0.0;
    playing       = true;
}

void Sequencer::stop()
{
    playing = false;
}

void Sequencer::setBPM(double newBpm)
{
    bpm = newBpm;
    recalcSamplesPerStep();
}

void Sequencer::setStep(int channel, int step, bool active)
{
    if (channel >= 0 && channel < CHANNEL_COUNT &&
        step    >= 0 && step    < STEP_COUNT)
        pattern[channel][step] = active;
}

void Sequencer::recalcSamplesPerStep()
{
    // 4/4박자 기준 16분음표
    // 1분 = bpm 박자 → 1박 = 60/bpm 초 → 16분음표 = (60/bpm)/4 초
    samplesPerStep = (60.0 / bpm / 4.0) * sampleRate;
}

void Sequencer::processBlock(int numSamples)
{
    if (!playing) return;

    sampleCounter += numSamples;

    while (sampleCounter >= samplesPerStep)
    {
        sampleCounter -= samplesPerStep;
        advanceStep();
    }
}

void Sequencer::advanceStep()
{
    currentStep = (currentStep + 1) % STEP_COUNT;
    
    for (int ch = 0; ch < CHANNEL_COUNT; ++ch)
        if (pattern[ch][currentStep])
            onTrigger(ch);
}

