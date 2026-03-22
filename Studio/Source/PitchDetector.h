/*
  ==============================================================================

    PitchDetector.h
    Created: 22 Mar 2026
    Author:  홍준영

    Monophonic pitch detector using the YIN algorithm.
    Audio-thread safe: all buffers pre-allocated in prepare().

  ==============================================================================
*/

#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

class PitchDetector
{
public:
    PitchDetector() = default;

    void prepare(double sampleRate, int /*maxBlockSize*/)
    {
        sampleRate_ = sampleRate;

        // Analysis window: 2048 samples covers down to ~43 Hz (vocal C2+)
        windowSize_ = 2048;
        halfWindow_ = windowSize_ / 2;

        // Hop size: run YIN every windowSize samples (no overlap).
        // At 44100 Hz / 512 block size, this means every ~4 blocks ≈ 21 Hz update rate.
        hopSize_ = windowSize_;

        // Pre-allocate all buffers
        inputRing_.assign((size_t)windowSize_, 0.0f);
        yinBuf_.resize((size_t)halfWindow_, 0.0f);
        linearBuf_.resize((size_t)windowSize_, 0.0f);
        ringWritePos_ = 0;
        samplesAccum_ = 0;
        currentPitchHz_ = 0.0f;

        // Tau search range for vocal frequencies
        minTau_ = std::max(2, (int)(sampleRate_ / kMaxFreq));
        maxTau_ = std::min(halfWindow_ - 1, (int)(sampleRate_ / kMinFreq));

        // Median filter history
        medianBuf_[0] = medianBuf_[1] = medianBuf_[2] = 0.0f;
        medianIdx_ = 0;
    }

    /** Feed audio samples (mono). Call once per audio callback. */
    void processBlock(const float* monoData, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            inputRing_[(size_t)ringWritePos_] = monoData[i];
            ringWritePos_ = (ringWritePos_ + 1) % windowSize_;
            ++samplesAccum_;
        }

        // Run YIN every hopSize_ samples
        if (samplesAccum_ >= hopSize_)
        {
            samplesAccum_ = 0;

            // RMS gate: skip YIN on quiet signal to save CPU
            float energy = 0.0f;
            for (int j = 0; j < windowSize_; ++j)
            {
                const float s = inputRing_[(size_t)j];
                energy += s * s;
            }
            const float rms = std::sqrt(energy / (float)windowSize_);

            if (rms < kMinRms)
            {
                currentPitchHz_ = 0.0f; // unvoiced / too quiet
                return;
            }

            const float rawPitch = runYIN();

            // 3-sample median filter to smooth jitter
            medianBuf_[medianIdx_ % 3] = rawPitch;
            ++medianIdx_;
            currentPitchHz_ = median3(medianBuf_[0], medianBuf_[1], medianBuf_[2]);
        }
    }

    /** Returns detected pitch in Hz, or 0 if unvoiced. */
    float getPitchHz() const { return currentPitchHz_; }

    void reset()
    {
        if (!inputRing_.empty())
            std::fill(inputRing_.begin(), inputRing_.end(), 0.0f);
        ringWritePos_ = 0;
        samplesAccum_ = 0;
        currentPitchHz_ = 0.0f;
        medianBuf_[0] = medianBuf_[1] = medianBuf_[2] = 0.0f;
        medianIdx_ = 0;
    }

private:
    double sampleRate_ = 44100.0;
    int    windowSize_ = 2048;
    int    halfWindow_ = 1024;
    int    hopSize_    = 2048;

    std::vector<float> inputRing_;
    std::vector<float> linearBuf_;  // linearized copy for cache-friendly YIN
    int ringWritePos_ = 0;
    int samplesAccum_ = 0;

    std::vector<float> yinBuf_;
    float currentPitchHz_ = 0.0f;

    // Tau search range (precomputed from frequency bounds)
    int minTau_ = 29;
    int maxTau_ = 882;

    // Median filter
    float medianBuf_[3] = {};
    int   medianIdx_ = 0;

    static constexpr float kYinThreshold = 0.15f;
    static constexpr float kMinFreq      = 50.0f;   // lowest detectable
    static constexpr float kMaxFreq      = 1500.0f;  // highest detectable
    static constexpr float kMinRms       = 0.002f;   // RMS gate threshold

    // -----------------------------------------------------------------------
    // YIN core algorithm (optimized: linear buffer + limited tau range)
    // -----------------------------------------------------------------------
    float runYIN()
    {
        const int W = halfWindow_;

        // Linearize the ring buffer for cache-friendly access
        const int start = (ringWritePos_ - windowSize_ + (int)inputRing_.size() * 2)
                          % (int)inputRing_.size();
        for (int i = 0; i < windowSize_; ++i)
            linearBuf_[(size_t)i] = inputRing_[(size_t)((start + i) % windowSize_)];

        const float* buf = linearBuf_.data();

        // Step 1 & 2: Difference function + Cumulative mean normalized
        // Only compute for tau in vocal frequency range [minTau_, maxTau_]
        yinBuf_[0] = 1.0f;
        float runningSum = 0.0f;

        // We must compute for all tau up to maxTau_ because the cumulative
        // mean normalization depends on all previous tau values.
        for (int tau = 1; tau <= maxTau_ && tau < W; ++tau)
        {
            float sum = 0.0f;
            for (int j = 0; j < W; ++j)
            {
                const float delta = buf[j] - buf[j + tau];
                sum += delta * delta;
            }

            runningSum += sum;
            yinBuf_[(size_t)tau] = (runningSum > 0.0f)
                                 ? sum * (float)tau / runningSum
                                 : 1.0f;
        }

        // Step 3: Absolute threshold — find first dip below threshold
        // Only search within vocal frequency range
        int tauEstimate = -1;
        for (int tau = std::max(2, minTau_); tau <= maxTau_ && tau < W; ++tau)
        {
            if (yinBuf_[(size_t)tau] < kYinThreshold)
            {
                // Find local minimum
                while (tau + 1 <= maxTau_ && tau + 1 < W
                       && yinBuf_[(size_t)(tau + 1)] < yinBuf_[(size_t)tau])
                    ++tau;
                tauEstimate = tau;
                break;
            }
        }

        if (tauEstimate < 1)
            return 0.0f; // unvoiced

        // Step 4: Parabolic interpolation for sub-sample accuracy
        const float betterTau = parabolicInterp(tauEstimate);

        if (betterTau <= 0.0f)
            return 0.0f;

        const float freq = (float)(sampleRate_ / (double)betterTau);

        // Reject out-of-range
        if (freq < kMinFreq || freq > kMaxFreq)
            return 0.0f;

        return freq;
    }

    float parabolicInterp(int tau) const
    {
        if (tau < 1 || tau >= halfWindow_ - 1)
            return (float)tau;

        const float s0 = yinBuf_[(size_t)(tau - 1)];
        const float s1 = yinBuf_[(size_t)tau];
        const float s2 = yinBuf_[(size_t)(tau + 1)];

        const float denom = 2.0f * (2.0f * s1 - s2 - s0);
        if (std::abs(denom) < 1e-12f)
            return (float)tau;

        return (float)tau + (s0 - s2) / denom;
    }

    static float median3(float a, float b, float c)
    {
        if (a > b) std::swap(a, b);
        if (b > c) std::swap(b, c);
        if (a > b) std::swap(a, b);
        return b;
    }
};
