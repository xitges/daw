/*
  ==============================================================================

    AutoTuneProcessor.cpp
    Created: 22 Mar 2026
    Author:  홍준영

  ==============================================================================
*/

#include "AutoTuneProcessor.h"
#include "Audio/RubberBandStretcher.h"
#include <cmath>
#include <algorithm>

using RBS = RubberBand::RubberBandStretcher;

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

AutoTuneProcessor::AutoTuneProcessor() = default;
AutoTuneProcessor::~AutoTuneProcessor() = default;

AutoTuneProcessor::AutoTuneProcessor(AutoTuneProcessor&&) noexcept = default;
AutoTuneProcessor& AutoTuneProcessor::operator=(AutoTuneProcessor&&) noexcept = default;

// ---------------------------------------------------------------------------
// Helper: prime the stretcher so it's ready to produce output immediately.
// Pre-feeds silence as start padding, then drains any processed silence
// from the output buffer so the first real process() call yields real audio.
// ---------------------------------------------------------------------------

void AutoTuneProcessor::primeStretcher()
{
    if (stretcher_ == nullptr) return;

    const size_t startPad = stretcher_->getPreferredStartPad();
    if (startPad > 0)
    {
        std::vector<float> silence(startPad, 0.0f);
        float* silPtrs[2] = { silence.data(), silence.data() };
        stretcher_->process(silPtrs, startPad, false);

        // Drain all processed silence from the output buffer.
        // Without this, retrieve() would return silence instead of real audio.
        const int avail = (int)stretcher_->available();
        if (avail > 0)
        {
            std::vector<float> drain((size_t)avail, 0.0f);
            float* drainPtrs[2] = { drain.data(), drain.data() };
            stretcher_->retrieve(drainPtrs, (size_t)avail);
        }
    }
}

// ---------------------------------------------------------------------------
// prepare / reset
// ---------------------------------------------------------------------------

void AutoTuneProcessor::prepare(double sampleRate, int maxBlockSize)
{
    sampleRate_   = sampleRate;
    maxBlockSize_ = maxBlockSize;

    pitchDetector_.prepare(sampleRate, maxBlockSize);

    // RubberBand in real-time mode, stereo, pitch-only (timeRatio = 1.0)
    const int opts = RBS::OptionProcessRealTime
                   | RBS::OptionPitchHighQuality
                   | RBS::OptionFormantPreserved
                   | RBS::OptionWindowShort;

    stretcher_ = std::make_unique<RBS>((size_t)sampleRate, 2, opts, 1.0, 1.0);
    stretcher_->setMaxProcessSize((size_t)maxBlockSize);

    primeStretcher();

    // Pre-allocate work buffers
    const size_t sz = (size_t)maxBlockSize;
    monoBuf_.resize(sz, 0.0f);
    dryL_.resize(sz, 0.0f);
    dryR_.resize(sz, 0.0f);

    // Retrieve buffer — RubberBand may return slightly more than input
    retrieveL_.resize(sz * 2, 0.0f);
    retrieveR_.resize(sz * 2, 0.0f);

    currentPitchRatio_ = 1.0f;
    lastFormantPreserve_ = true;
}

void AutoTuneProcessor::reset()
{
    pitchDetector_.reset();

    if (stretcher_)
    {
        stretcher_->reset();
        primeStretcher();
    }

    if (!dryL_.empty())      std::fill(dryL_.begin(),      dryL_.end(),      0.0f);
    if (!dryR_.empty())      std::fill(dryR_.begin(),      dryR_.end(),      0.0f);
    if (!retrieveL_.empty()) std::fill(retrieveL_.begin(), retrieveL_.end(), 0.0f);
    if (!retrieveR_.empty()) std::fill(retrieveR_.begin(), retrieveR_.end(), 0.0f);

    currentPitchRatio_ = 1.0f;
    detectedPitchHz_ = 0.0f;
    targetPitchHz_   = 0.0f;
}

// ---------------------------------------------------------------------------
// processBlock — called from audio thread
// ---------------------------------------------------------------------------

void AutoTuneProcessor::processBlock(float* L, float* R, int numSamples,
                                     const AutoTuneParams& params)
{
    if (!params.enabled || stretcher_ == nullptr || numSamples <= 0)
        return;

    // Update formant option if changed
    if (params.formantPreserve != lastFormantPreserve_)
    {
        stretcher_->setFormantOption(params.formantPreserve
                                    ? RBS::OptionFormantPreserved
                                    : RBS::OptionFormantShifted);
        lastFormantPreserve_ = params.formantPreserve;
    }

    // --- Save original input (for dry mix and fallback) ---
    for (int i = 0; i < numSamples; ++i)
    {
        dryL_[(size_t)i] = L[i];
        dryR_[(size_t)i] = R[i];
    }

    // --- Pitch detection (mono mix) ---
    for (int i = 0; i < numSamples; ++i)
        monoBuf_[(size_t)i] = (L[i] + R[i]) * 0.5f;

    pitchDetector_.processBlock(monoBuf_.data(), numSamples);
    const float detectedHz = pitchDetector_.getPitchHz();
    detectedPitchHz_ = detectedHz;

    // --- Compute target pitch ratio ---
    float targetRatio = 1.0f;

    if (detectedHz > 0.0f && !std::isnan(detectedHz) && !std::isinf(detectedHz))
    {
        const float detectedMidi = hzToMidi(detectedHz);
        const float targetMidi   = snapToScale(detectedMidi, params.keyTonic, params.scaleType);
        const float targetHz     = midiToHz(targetMidi);
        targetPitchHz_ = targetHz;

        targetRatio = targetHz / detectedHz;

        // Clamp to reasonable range (±1 octave)
        targetRatio = std::clamp(targetRatio, 0.5f, 2.0f);
    }
    else
    {
        targetPitchHz_ = 0.0f;
        targetRatio = 1.0f; // no correction on unvoiced segments
    }

    // --- Apply retune speed smoothing ---
    // retuneSpeed 0 = instant, 1 = very slow
    // smoothing coefficient: 1.0 means jump instantly, 0.001 means very slow
    const float smoothCoef = 1.0f - std::clamp(params.retuneSpeed, 0.0f, 0.999f);
    currentPitchRatio_ += (targetRatio - currentPitchRatio_) * smoothCoef;

    // --- RubberBand pitch shift ---
    stretcher_->setPitchScale((double)currentPitchRatio_);

    float* inPtrs[2] = { L, R };
    stretcher_->process(inPtrs, (size_t)numSamples, false);

    // Retrieve available output
    const int avail = (int)stretcher_->available();
    const int toRetrieve = std::min(avail, numSamples);

    if (toRetrieve > 0)
    {
        float* outPtrs[2] = { retrieveL_.data(), retrieveR_.data() };
        stretcher_->retrieve(outPtrs, (size_t)toRetrieve);
    }

    // --- Dry/wet mix ---
    const float wet = std::clamp(params.mix, 0.0f, 1.0f);
    const float dry = 1.0f - wet;

    for (int i = 0; i < numSamples; ++i)
    {
        const float dryLSample = dryL_[(size_t)i];
        const float dryRSample = dryR_[(size_t)i];

        // Wet signal: use RubberBand output if available, else passthrough original
        const float wetLSample = (i < toRetrieve) ? retrieveL_[(size_t)i] : dryLSample;
        const float wetRSample = (i < toRetrieve) ? retrieveR_[(size_t)i] : dryRSample;

        L[i] = dryLSample * dry + wetLSample * wet;
        R[i] = dryRSample * dry + wetRSample * wet;
    }
}

// ---------------------------------------------------------------------------
// Pitch helpers
// ---------------------------------------------------------------------------

float AutoTuneProcessor::hzToMidi(float hz) const
{
    return 12.0f * std::log2(hz / 440.0f) + 69.0f;
}

float AutoTuneProcessor::midiToHz(float midi) const
{
    return 440.0f * std::pow(2.0f, (midi - 69.0f) / 12.0f);
}

float AutoTuneProcessor::snapToScale(float midiNote, int keyTonic, int scaleType) const
{
    const int roundedMidi = (int)std::round(midiNote);

    KeySignature key;
    key.tonic = keyTonic;
    key.scale = static_cast<ScaleType>(scaleType);

    const int snappedMidi = MusicTheory::snapPitchToScale(roundedMidi, key);

    // Return the snapped pitch (integer — instant snap for the robotic effect)
    return (float)snappedMidi;
}
