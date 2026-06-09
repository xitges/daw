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

void Sequencer::start(int startStep)
{
    const int safeStartStep = (stepCount > 0) ? juce::jlimit(0, stepCount - 1, startStep) : 0;
    currentStep   = safeStartStep;
    sampleCounter = 0.0;
    playing       = true;
    fireStepZeroOnNextBlock = true;
    clearPendingTriggers();
}

void Sequencer::stop()
{
    playing = false;
    clearPendingTriggers();
}

void Sequencer::setBPM(double newBpm)
{
    bpm = juce::jmax(1.0, newBpm);
    recalcSamplesPerStep();
}

void Sequencer::setStep(int channel, int step, bool active)
{
    if (channel >= 0 && channel < CHANNEL_COUNT &&
        step    >= 0 && step    < stepCount &&
        step    <  MAX_STEPS)
        pattern[channel][step] = active;
}

void Sequencer::setStepCount(int newCount)
{
    stepCount = juce::jlimit(1, MAX_STEPS, newCount);
    if (currentStep >= stepCount)
        currentStep = 0;
}

void Sequencer::setStepTimingOffset(int step, float offset)
{
    if (step >= 0 && step < MAX_STEPS)
        stepTimingOffset[step] = juce::jlimit(-0.5f, 0.5f, offset);
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
    if (numSamples <= 0) return;

    drainPendingTriggers(numSamples);

    if (fireStepZeroOnNextBlock)
    {
        fireStepZeroOnNextBlock = false;
        triggerCurrentStep(0, numSamples);
    }

    const double prevCounter = sampleCounter;
    sampleCounter += numSamples;

    // timeToFire: samples from the start of this buffer until the next step fires
    double timeToFire = samplesPerStep - prevCounter;

    while (sampleCounter >= samplesPerStep)
    {
        sampleCounter -= samplesPerStep;

        // Compute base offset in buffer
        int baseOffset = juce::jlimit(0, numSamples - 1, (int)std::round(timeToFire));

        advanceStep(baseOffset, numSamples);
        timeToFire += samplesPerStep;
    }
}

void Sequencer::advanceStep(int offsetInBuffer, int numSamples)
{
    if (stepCount <= 0)
        return;

    currentStep = (currentStep + 1) % stepCount;
    triggerCurrentStep(offsetInBuffer, numSamples);
}

void Sequencer::triggerCurrentStep(int offsetInBuffer, int numSamples)
{
    // === Apply swing ===
    // Swing shifts even-numbered steps (1, 3, 5, ...) — the "off-beat" 16th notes.
    // swingAmount 0.0 = straight, 0.33 = triplet feel, 0.5 = dotted feel.
    int adjustedOffset = offsetInBuffer;

    if (swingAmount > 0.001f && (currentStep % 2 == 1))
    {
        // Push odd steps (off-beats) later by swingAmount * samplesPerStep
        adjustedOffset += (int)std::round(swingAmount * samplesPerStep);
    }

    // === Apply per-step timing offset ===
    // timingOffset: -0.5..+0.5 of one step duration
    const float perStepOffset = stepTimingOffset[currentStep];
    if (std::abs(perStepOffset) > 0.001f)
    {
        adjustedOffset += (int)std::round((double)perStepOffset * samplesPerStep);
    }

    adjustedOffset = juce::jmax(0, adjustedOffset);

    for (int ch = 0; ch < CHANNEL_COUNT; ++ch)
        if (pattern[ch][currentStep])
        {
            if (adjustedOffset < numSamples)
                onTrigger(ch, currentStep, adjustedOffset);
            else
                enqueuePendingTrigger(ch, currentStep, adjustedOffset - numSamples);
        }
}

void Sequencer::clearPendingTriggers()
{
    pendingTriggerCount = 0;
}

void Sequencer::drainPendingTriggers(int numSamples)
{
    int write = 0;

    for (int i = 0; i < pendingTriggerCount; ++i)
    {
        auto trigger = pendingTriggers[(size_t)i];
        if (trigger.samplesUntilFire < numSamples)
        {
            onTrigger(trigger.channel, trigger.step, trigger.samplesUntilFire);
        }
        else
        {
            trigger.samplesUntilFire -= numSamples;
            pendingTriggers[(size_t)write++] = trigger;
        }
    }

    pendingTriggerCount = write;
}

void Sequencer::enqueuePendingTrigger(int channel, int step, int samplesUntilFire)
{
    if (pendingTriggerCount >= MAX_PENDING_TRIGGERS)
        return;

    auto& trigger = pendingTriggers[(size_t)pendingTriggerCount++];
    trigger.channel = channel;
    trigger.step = step;
    trigger.samplesUntilFire = juce::jmax(0, samplesUntilFire);
}
