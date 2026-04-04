#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <array>
#include <cmath>
#include <functional>

/**
 * LiveLoopEngine  --  Professional-grade per-channel MIDI loop recorder + player.
 *
 * Features:
 *  - Per-channel independent loop lengths
 *  - Overdub (layer notes on existing loop)
 *  - Undo last recording layer
 *  - Mute / solo per channel
 *  - Per-channel volume (applied at playback velocity)
 *  - Attack + length quantization with configurable step
 *  - Count-in: WaitingForBar state exposes beats-until-record
 *  - RT-safe: no heap alloc, no locks on audio thread
 *
 * State machine per channel:
 *   Idle -> [arm()] -> Armed -> [first NoteOn] -> WaitingForBar
 *        -> [bar boundary] -> Recording -> [loopLength elapsed] -> Looping
 *        -> [overdub()] -> Overdubbing -> (loopLength elapsed) -> Looping
 *        -> [stop()] -> Idle
 *
 * Thread safety:
 *   Message thread : arm/stop/overdub/undo/setMute/setVolume/setLoopLength/setQuantize
 *   Audio thread   : processMidiEvent/processBlock
 *   Cross-thread   : all requests via atomics
 */
class LiveLoopEngine
{
public:
    static constexpr int kMaxChannels = 16;
    static constexpr int kMaxNotes    = 4096;   // per channel (2x previous for overdub layers)

    enum class State : uint8_t
    {
        Idle,
        Armed,           // queued, waiting for first NoteOn
        WaitingForBar,   // NoteOn seen, waiting for bar boundary -> count-in
        Recording,       // first-take capture
        Overdubbing,     // loop playing while adding notes
        Looping          // pure playback
    };

    // -- Message-thread API ----------------------------------------------------

    /** Arm channel with an explicit loop length in beats. */
    void arm(int ch, double loopLengthBeats) noexcept
    {
        if (ch < 0 || ch >= kMaxChannels) return;
        pendingLoopLength_[ch].store(juce::jmax(1.0, loopLengthBeats), std::memory_order_release);
        stopReq_[ch].store(false, std::memory_order_release);
        overdubReq_[ch].store(false, std::memory_order_release);
        armReq_[ch].store(true,  std::memory_order_release);
    }

    void stop(int ch) noexcept
    {
        if (ch < 0 || ch >= kMaxChannels) return;
        stopReq_[ch].store(true, std::memory_order_release);
    }

    void resetAll() noexcept
    {
        for (int ch = 0; ch < kMaxChannels; ++ch)
            stop(ch);
    }

    /** Enter overdub on a Looping channel (adds notes on top of existing loop). */
    void overdub(int ch) noexcept
    {
        if (ch < 0 || ch >= kMaxChannels) return;
        overdubReq_[ch].store(true, std::memory_order_release);
    }

    /** Roll back the last recording layer (one level of undo per channel). */
    void undo(int ch) noexcept
    {
        if (ch < 0 || ch >= kMaxChannels) return;
        undoReq_[ch].store(true, std::memory_order_release);
    }

    /** 0=free, 0.25=1/16, 0.5=1/8, 1.0=1/4 note. Applied to attack AND length. */
    void setQuantize(double stepBeats) noexcept
    {
        quantizeStep_.store(juce::jmax(0.0, stepBeats), std::memory_order_release);
    }
    double getQuantize() const noexcept { return quantizeStep_.load(std::memory_order_relaxed); }

    void setMute(int ch, bool muted) noexcept
    {
        if (ch < 0 || ch >= kMaxChannels) return;
        muted_[ch].store(muted, std::memory_order_release);
    }
    bool getMute(int ch) const noexcept
    {
        if (ch < 0 || ch >= kMaxChannels) return false;
        return muted_[ch].load(std::memory_order_relaxed);
    }

    void setVolume(int ch, float v) noexcept
    {
        if (ch < 0 || ch >= kMaxChannels) return;
        volume_[ch].store(juce::jlimit(0.0f, 2.0f, v), std::memory_order_release);
    }
    float getVolume(int ch) const noexcept
    {
        if (ch < 0 || ch >= kMaxChannels) return 1.0f;
        return volume_[ch].load(std::memory_order_relaxed);
    }

    State getState(int ch) const noexcept
    {
        if (ch < 0 || ch >= kMaxChannels) return State::Idle;
        return static_cast<State>(state_[ch].load(std::memory_order_relaxed));
    }

    /** Beats remaining until recording actually starts (count-in display). */
    double getBeatsTillRecord(int ch) const noexcept
    {
        if (ch < 0 || ch >= kMaxChannels) return 0.0;
        const auto st = static_cast<State>(state_[ch].load(std::memory_order_relaxed));
        if (st != State::WaitingForBar) return 0.0;
        return juce::jmax(0.0, ch_[ch].recordStartBeat - globalBeat_);
    }

    // -- Display helpers (UI-thread safe; minor read race accepted) -------------

    struct NoteDisplayItem { int pitch; float velocity; double startBeat, endBeat; };

    int getNotesForDisplay(int ch, NoteDisplayItem* out, int maxItems) const noexcept
    {
        if (ch < 0 || ch >= kMaxChannels || !out || maxItems <= 0) return 0;
        const auto& cd = ch_[ch];
        const int n = std::min(cd.noteCount, maxItems);
        for (int i = 0; i < n; ++i)
            out[i] = { cd.notes[i].pitch, cd.notes[i].velocity,
                       cd.notes[i].startBeat, cd.notes[i].endBeat };
        return n;
    }

    double getLoopPhase(int ch) const noexcept
    {
        if (ch < 0 || ch >= kMaxChannels) return 0.0;
        return ch_[ch].loopPhase;
    }

    double getChannelLoopLength(int ch) const noexcept
    {
        if (ch < 0 || ch >= kMaxChannels) return 4.0;
        return ch_[ch].loopLength;
    }

    // -- Audio-thread callbacks ------------------------------------------------
    std::function<void(int ch, int pitch, float velocity)> onNoteOn;
    std::function<void(int ch, int pitch)>                 onNoteOff;

    // -- Audio-thread API ------------------------------------------------------
    void processBlock(int numSamples, double bpm, double sampleRate) noexcept;
    void processMidiEvent(const juce::MidiMessage& msg, int ch) noexcept;
    void resetGlobalBeat() noexcept { globalBeat_ = 0.0; }
    double getGlobalBeat() const noexcept { return globalBeat_; }

private:
    struct RecordedNote
    {
        int    pitch     = 0;
        float  velocity  = 0.0f;
        double startBeat = 0.0;
        double endBeat   = 0.0;
        bool   valid     = false;
    };

    struct HeldNote
    {
        bool   active    = false;
        double startBeat = 0.0;
        float  velocity  = 0.0f;
    };

    struct Channel
    {
        double loopLength      = 4.0;
        double loopPhase       = 0.0;
        double recordStartBeat = 0.0;

        RecordedNote notes[kMaxNotes];
        int          noteCount     = 0;
        int          undoNoteCount = 0;   // saved before overdub layer

        HeldNote held[128];

        void reset() noexcept
        {
            loopPhase      = 0.0;
            noteCount      = 0;
            undoNoteCount  = 0;
            for (auto& h : held) h = {};
        }
    };

    Channel ch_[kMaxChannels];
    double  globalBeat_ = 0.0;

    // Cross-thread requests
    std::atomic<uint8_t> state_      [kMaxChannels] {};
    std::atomic<bool>    armReq_     [kMaxChannels] {};
    std::atomic<bool>    stopReq_    [kMaxChannels] {};
    std::atomic<bool>    overdubReq_ [kMaxChannels] {};
    std::atomic<bool>    undoReq_    [kMaxChannels] {};

    // Per-channel settings
    std::atomic<double>  pendingLoopLength_[kMaxChannels];
    std::atomic<bool>    muted_ [kMaxChannels] {};
    std::atomic<float>   volume_[kMaxChannels];

    // Global settings
    std::atomic<double>  quantizeStep_ { 0.25 };  // default: 1/16

    // -- Constructor: init atomics ---------------------------------------------
public:
    LiveLoopEngine() noexcept
    {
        for (int i = 0; i < kMaxChannels; ++i)
        {
            pendingLoopLength_[i].store(16.0, std::memory_order_relaxed);
            volume_[i].store(1.0f, std::memory_order_relaxed);
        }
    }

private:
    void firePlayback(int ch, double prevPhase, double nextPhase) noexcept;
    void finalizeHeldNotes(int ch) noexcept;
    double snapToGrid(double beat, double loopLength) const noexcept;
};

// -----------------------------------------------------------------------------
// Inline implementation
// -----------------------------------------------------------------------------

inline double LiveLoopEngine::snapToGrid(double beat, double loopLength) const noexcept
{
    const double qStep = quantizeStep_.load(std::memory_order_relaxed);
    if (qStep <= 0.0) return beat;
    double snapped = std::round(beat / qStep) * qStep;
    snapped = std::fmod(snapped, loopLength);
    if (snapped < 0.0) snapped += loopLength;
    return snapped;
}

inline void LiveLoopEngine::processBlock(int numSamples, double bpm, double sampleRate) noexcept
{
    if (sampleRate <= 0.0 || bpm <= 0.0) return;

    const double beatsThisBlock = numSamples * bpm / (60.0 * sampleRate);
    globalBeat_ += beatsThisBlock;

    for (int c = 0; c < kMaxChannels; ++c)
    {
        // -- Handle stop request (highest priority) ------------------------
        if (stopReq_[c].exchange(false, std::memory_order_acq_rel))
        {
            // Silence held notes
            if (onNoteOff)
                for (int p = 0; p < 128; ++p)
                    if (ch_[c].held[p].active) { onNoteOff(c, p); }
            ch_[c].reset();
            state_[c].store((uint8_t)State::Idle, std::memory_order_release);
            armReq_[c].store(false, std::memory_order_release);
            overdubReq_[c].store(false, std::memory_order_release);
            undoReq_[c].store(false, std::memory_order_release);
            continue;
        }

        // -- Handle arm request --------------------------------------------
        if (armReq_[c].exchange(false, std::memory_order_acq_rel))
        {
            ch_[c].reset();
            ch_[c].loopLength = pendingLoopLength_[c].load(std::memory_order_relaxed);
            state_[c].store((uint8_t)State::Armed, std::memory_order_release);
        }

        // -- Handle overdub request ----------------------------------------
        if (overdubReq_[c].exchange(false, std::memory_order_acq_rel))
        {
            const auto st = static_cast<State>(state_[c].load(std::memory_order_relaxed));
            if (st == State::Looping)
            {
                ch_[c].undoNoteCount = ch_[c].noteCount;  // save undo point
                state_[c].store((uint8_t)State::Overdubbing, std::memory_order_release);
            }
        }

        // -- Handle undo request -------------------------------------------
        if (undoReq_[c].exchange(false, std::memory_order_acq_rel))
        {
            const auto st = static_cast<State>(state_[c].load(std::memory_order_relaxed));
            if (st == State::Looping || st == State::Overdubbing)
            {
                // Silence any held notes from overdub
                if (onNoteOff)
                    for (int p = 0; p < 128; ++p)
                        if (ch_[c].held[p].active) { onNoteOff(c, p); ch_[c].held[p] = {}; }
                ch_[c].noteCount = ch_[c].undoNoteCount;
                state_[c].store((uint8_t)State::Looping, std::memory_order_release);
            }
        }

        const auto st = static_cast<State>(state_[c].load(std::memory_order_relaxed));

        // -- WaitingForBar -> Recording -------------------------------------
        if (st == State::WaitingForBar)
        {
            if (globalBeat_ >= ch_[c].recordStartBeat)
            {
                ch_[c].loopPhase = 0.0;
                state_[c].store((uint8_t)State::Recording, std::memory_order_release);
            }
            continue;
        }

        // -- Recording -> Looping (loop length elapsed) ---------------------
        if (st == State::Recording)
        {
            const double elapsed = globalBeat_ - ch_[c].recordStartBeat;
            if (elapsed >= ch_[c].loopLength)
            {
                ch_[c].undoNoteCount = 0;  // no undo before first take
                finalizeHeldNotes(c);
                ch_[c].loopPhase = 0.0;
                state_[c].store((uint8_t)State::Looping, std::memory_order_release);
            }
            continue;  // no playback during first-take recording
        }

        // -- Looping / Overdubbing: advance phase + fire playback ----------
        if (st == State::Looping || st == State::Overdubbing)
        {
            auto& cd = ch_[c];
            const double prevPhase = cd.loopPhase;
            cd.loopPhase += beatsThisBlock;

            // Overdubbing: finalise layer when loop wraps
            if (st == State::Overdubbing && cd.loopPhase >= cd.loopLength)
                finalizeHeldNotes(c);

            if (cd.loopPhase >= cd.loopLength)
            {
                firePlayback(c, prevPhase, cd.loopLength);
                cd.loopPhase = std::fmod(cd.loopPhase, cd.loopLength);
                firePlayback(c, 0.0, cd.loopPhase);

                // Overdubbing wraps back to Looping after one full pass
                if (st == State::Overdubbing)
                    state_[c].store((uint8_t)State::Looping, std::memory_order_release);
            }
            else
            {
                firePlayback(c, prevPhase, cd.loopPhase);
            }
        }
    }
}

inline void LiveLoopEngine::processMidiEvent(const juce::MidiMessage& msg, int ch) noexcept
{
    if (ch < 0 || ch >= kMaxChannels) return;
    if (!msg.isNoteOn() && !msg.isNoteOff()) return;

    const double globalBeat = globalBeat_;
    const int    pitch = juce::jlimit(0, 127, msg.getNoteNumber());
    const float  vel   = msg.getFloatVelocity();
    auto& cd  = ch_[ch];
    const auto st = static_cast<State>(state_[ch].load(std::memory_order_relaxed));

    // -- Armed: first NoteOn -> schedule start at next bar boundary --------
    if (st == State::Armed && msg.isNoteOn())
    {
        cd.loopLength      = pendingLoopLength_[ch].load(std::memory_order_relaxed);
        cd.recordStartBeat = (std::floor(globalBeat / 4.0) + 1.0) * 4.0;
        cd.noteCount       = 0;
        cd.loopPhase       = 0.0;
        for (auto& h : cd.held) h = {};
        state_[ch].store((uint8_t)State::WaitingForBar, std::memory_order_release);
        return;
    }

    // -- WaitingForBar: if bar already passed, jump straight to Recording --
    if (st == State::WaitingForBar && globalBeat >= cd.recordStartBeat)
    {
        cd.loopPhase = 0.0;
        state_[ch].store((uint8_t)State::Recording, std::memory_order_release);
    }

    // Capture for Recording and Overdubbing only
    const auto stNow = static_cast<State>(state_[ch].load(std::memory_order_relaxed));
    if (stNow != State::Recording && stNow != State::Overdubbing) return;

    // -- Compute beat position relative to loop start ----------------------
    double relBeat;
    if (stNow == State::Recording)
        relBeat = std::fmax(0.0, globalBeat - cd.recordStartBeat);
    else
        relBeat = cd.loopPhase;

    relBeat = std::fmod(relBeat, cd.loopLength);
    if (relBeat < 0.0) relBeat += cd.loopLength;

    if (msg.isNoteOn())
    {
        // Snap attack to grid
        const double snapped = snapToGrid(relBeat, cd.loopLength);
        cd.held[pitch] = { true, snapped, vel };
    }
    else if (msg.isNoteOff())
    {
        auto& hn = cd.held[pitch];
        if (!hn.active) return;

        const double qStep   = quantizeStep_.load(std::memory_order_relaxed);
        const double snapEnd = (qStep > 0.0) ? snapToGrid(relBeat, cd.loopLength) : relBeat;

        double len = snapEnd - hn.startBeat;
        if (len < 0.0) len += cd.loopLength;  // true wrap-around only

        // Quantize length: round to nearest step, enforce minimum 1 step
        if (qStep > 0.0)
        {
            len = std::round(len / qStep) * qStep;
            if (len < qStep) len = qStep;
        }
        else
        {
            len = std::fmax(0.0625, len);
        }

        if (cd.noteCount < kMaxNotes)
        {
            auto& note     = cd.notes[cd.noteCount++];
            note.pitch     = pitch;
            note.velocity  = hn.velocity;
            note.startBeat = hn.startBeat;
            note.endBeat   = std::fmod(hn.startBeat + len, cd.loopLength);
            note.valid     = true;
        }
        hn.active = false;
    }
}

inline void LiveLoopEngine::firePlayback(int ch, double prevPhase, double nextPhase) noexcept
{
    if (prevPhase >= nextPhase) return;
    if (muted_[ch].load(std::memory_order_relaxed)) return;

    auto& cd = ch_[ch];
    const float vol = volume_[ch].load(std::memory_order_relaxed);

    for (int n = 0; n < cd.noteCount; ++n)
    {
        const auto& note = cd.notes[n];
        if (!note.valid) continue;

        if (note.startBeat >= prevPhase && note.startBeat < nextPhase)
            if (onNoteOn) onNoteOn(ch, note.pitch,
                                   juce::jlimit(0.01f, 1.0f, note.velocity * vol));

        if (note.endBeat >= prevPhase && note.endBeat < nextPhase)
            if (onNoteOff) onNoteOff(ch, note.pitch);
    }
}

inline void LiveLoopEngine::finalizeHeldNotes(int ch) noexcept
{
    auto& cd = ch_[ch];
    const double qStep = quantizeStep_.load(std::memory_order_relaxed);

    for (int p = 0; p < 128; ++p)
    {
        auto& hn = cd.held[p];
        if (!hn.active) continue;

        // End held note at: nearest step >= current phase, or end of loop
        double endBeat = cd.loopLength;
        if (qStep > 0.0)
        {
            // Snap to next grid step
            const double phase = cd.loopPhase;
            endBeat = std::ceil(phase / qStep) * qStep;
            if (endBeat > cd.loopLength) endBeat = cd.loopLength;
        }

        double len = endBeat - hn.startBeat;
        if (len < 0.0) len += cd.loopLength;
        if (qStep > 0.0 && len < qStep) len = qStep;
        else if (len < 0.0625) len = 0.0625;

        if (cd.noteCount < kMaxNotes)
        {
            auto& note     = cd.notes[cd.noteCount++];
            note.pitch     = p;
            note.velocity  = hn.velocity;
            note.startBeat = hn.startBeat;
            note.endBeat   = std::fmod(hn.startBeat + len, cd.loopLength);
            note.valid     = true;
        }
        hn.active = false;
        if (onNoteOff) onNoteOff(ch, p);
    }
}
