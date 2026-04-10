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

    /**
     * Free record: arm channel with no pre-set length.
     * Recording starts immediately on first NoteOn (no WaitingForBar).
     * When stop() is called, elapsed time becomes the loop length -> Looping.
     */
    void armFree(int ch) noexcept
    {
        if (ch < 0 || ch >= kMaxChannels) return;
        pendingLoopLength_[ch].store(0.0, std::memory_order_release);  // 0 = free
        stopReq_[ch].store(false, std::memory_order_release);
        overdubReq_[ch].store(false, std::memory_order_release);
        armReq_[ch].store(true, std::memory_order_release);
    }

    /** Halve the loop length of a Looping channel (removes notes in second half). */
    void halfLoop(int ch) noexcept
    {
        if (ch < 0 || ch >= kMaxChannels) return;
        halfReq_[ch].store(true, std::memory_order_release);
    }

    /** Double the loop length (duplicates existing notes into second half). */
    void doubleLoop(int ch) noexcept
    {
        if (ch < 0 || ch >= kMaxChannels) return;
        doubleReq_[ch].store(true, std::memory_order_release);
    }

    /**
     * Scene launch: simultaneously arm all Idle channels with the given loop length.
     * They all wait for the same bar boundary, so they start in sync.
     */
    void launchAll(double loopBeats) noexcept
    {
        for (int c = 0; c < kMaxChannels; ++c)
        {
            const auto st = static_cast<State>(state_[c].load(std::memory_order_relaxed));
            if (st == State::Idle)
                arm(c, loopBeats);
        }
    }

    /** @returns true if channel is in free-record mode (loopLength not yet set). */
    bool isFreeRecord(int ch) const noexcept
    {
        if (ch < 0 || ch >= kMaxChannels) return false;
        const auto st = static_cast<State>(state_[ch].load(std::memory_order_relaxed));
        return (st == State::Armed || st == State::Recording)
            && ch_[ch].loopLength <= 0.0;
    }

    /** 0=free, 0.25=1/16, 0.5=1/8, 1.0=1/4 note. Applied to attack AND length. */
    void setQuantize(double stepBeats) noexcept
    {
        quantizeStep_.store(juce::jmax(0.0, stepBeats), std::memory_order_release);
    }
    double getQuantize() const noexcept { return quantizeStep_.load(std::memory_order_relaxed); }

    /**
     * Count-in before recording starts (in bars, where 1 bar = 4 beats).
     * 0 = record starts at the next bar boundary with no count-in wait.
     * 1 = wait 1 bar (default).  2 = 2 bars.  4 = 4 bars.
     */
    void setCountInBars(int bars) noexcept
    {
        countInBars_.store(juce::jlimit(0, 4, bars), std::memory_order_release);
    }
    int getCountInBars() const noexcept { return countInBars_.load(std::memory_order_relaxed); }

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

    // -- Scene save/recall (UI thread only - not realtime safe) ----------------
    static constexpr int kNumScenes = 4;

    struct SceneSlot
    {
        // POD copy of RecordedNote (duplicated to avoid forward-declaration issues)
        struct NoteSnap { int pitch=0; float velocity=0.f; double startBeat=0.0, endBeat=0.0; bool valid=false; };

        bool occupied = false;
        struct TrackSnap
        {
            bool     hasLoop    = false;
            double   loopLength = 0.0;
            int      noteCount  = 0;
            NoteSnap notes[kMaxNotes];
        } tracks[kMaxChannels];
    };

    /** Save all currently looping channels into scene slot [0-3]. UI thread only. */
    void saveScene(int sceneIdx)
    {
        if (sceneIdx < 0 || sceneIdx >= kNumScenes) return;
        auto& s = scenes_[sceneIdx];
        s.occupied = true;
        for (int c = 0; c < kMaxChannels; ++c)
        {
            const auto st = static_cast<State>(state_[c].load(std::memory_order_relaxed));
            const bool isLoop = (st == State::Looping || st == State::Overdubbing);
            s.tracks[c].hasLoop    = isLoop;
            s.tracks[c].loopLength = ch_[c].loopLength;
            s.tracks[c].noteCount  = isLoop ? ch_[c].noteCount : 0;
            if (isLoop)
            {
                for (int n = 0; n < ch_[c].noteCount; ++n)
                {
                    auto& src = ch_[c].notes[n];
                    auto& dst = s.tracks[c].notes[n];
                    dst.pitch = src.pitch; dst.velocity = src.velocity;
                    dst.startBeat = src.startBeat; dst.endBeat = src.endBeat;
                    dst.valid = src.valid;
                }
            }
        }
    }

    /** Recall scene: stops all channels, then restores and starts looping. UI thread only. */
    void recallScene(int sceneIdx)
    {
        if (sceneIdx < 0 || sceneIdx >= kNumScenes) return;
        const auto& s = scenes_[sceneIdx];
        if (!s.occupied) return;

        for (int c = 0; c < kMaxChannels; ++c)
            stopReq_[c].store(true, std::memory_order_release);

        for (int c = 0; c < kMaxChannels; ++c)
        {
            if (!s.tracks[c].hasLoop || s.tracks[c].noteCount <= 0) continue;
            ch_[c].loopLength    = s.tracks[c].loopLength;
            ch_[c].loopPhase     = 0.0;
            ch_[c].noteCount     = s.tracks[c].noteCount;
            ch_[c].undoNoteCount = 0;
            for (int n = 0; n < s.tracks[c].noteCount; ++n)
            {
                const auto& src = s.tracks[c].notes[n];
                auto& dst = ch_[c].notes[n];
                dst.pitch = src.pitch; dst.velocity = src.velocity;
                dst.startBeat = src.startBeat; dst.endBeat = src.endBeat;
                dst.valid = src.valid;
            }
            state_[c].store((uint8_t)State::Looping, std::memory_order_release);
        }
    }

    bool isSceneOccupied(int sceneIdx) const noexcept
    {
        if (sceneIdx < 0 || sceneIdx >= kNumScenes) return false;
        return scenes_[sceneIdx].occupied;
    }

    /** Export notes from channel as NoteDisplayItems (for pattern export). */
    int exportChannelNotes(int ch, NoteDisplayItem* out, int maxItems) const noexcept
    {
        return getNotesForDisplay(ch, out, maxItems);
    }

    double getChannelLength(int ch) const noexcept { return getChannelLoopLength(ch); }

    // -- Audio-thread callbacks ------------------------------------------------
    std::function<void(int ch, int pitch, float velocity)> onNoteOn;
    std::function<void(int ch, int pitch)>                 onNoteOff;

    // -- Audio-thread API ------------------------------------------------------
    void processBlock(int numSamples, double bpm, double sampleRate) noexcept;
    void processMidiEvent(const juce::MidiMessage& msg, int ch, double eventBeat) noexcept;
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

    // Scene storage (UI thread only)
    SceneSlot scenes_[kNumScenes] {};

    // Cross-thread requests
    std::atomic<uint8_t> state_      [kMaxChannels] {};
    std::atomic<bool>    armReq_     [kMaxChannels] {};
    std::atomic<bool>    stopReq_    [kMaxChannels] {};
    std::atomic<bool>    overdubReq_ [kMaxChannels] {};
    std::atomic<bool>    undoReq_    [kMaxChannels] {};
    std::atomic<bool>    halfReq_    [kMaxChannels] {};
    std::atomic<bool>    doubleReq_  [kMaxChannels] {};

    // Per-channel settings
    std::atomic<double>  pendingLoopLength_[kMaxChannels];
    std::atomic<bool>    muted_ [kMaxChannels] {};
    std::atomic<float>   volume_[kMaxChannels];

    // Global settings
    std::atomic<double>  quantizeStep_   { 0.25 };
    std::atomic<int>     countInBars_    { 1 };
    std::atomic<bool>    snapForward_    { false };

    // Scene recall request (UI→audio thread, lock-free)
    // -1 = no pending recall; 0-3 = scene index to recall at pendingRecallBeat_
    std::atomic<int>     pendingRecallIdx_  { -1 };
    std::atomic<double>  pendingRecallBeat_ { 0.0 };

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

    void setSnapForward(bool fwd) noexcept { snapForward_.store(fwd, std::memory_order_release); }
    bool getSnapForward() const noexcept   { return snapForward_.load(std::memory_order_relaxed); }

    /**
     * Schedule a scene recall to happen at the next bar boundary.
     * Called from UI thread. The audio thread executes it in processBlock.
     */
    void scheduleRecallScene(int sceneIdx) noexcept
    {
        if (sceneIdx < 0 || sceneIdx >= kNumScenes) return;
        if (!scenes_[sceneIdx].occupied) return;
        // Quantize to next bar boundary (4 beats)
        const double nextBar = (std::floor(globalBeat_ / 4.0) + 1.0) * 4.0;
        pendingRecallBeat_.store(nextBar, std::memory_order_release);
        pendingRecallIdx_.store(sceneIdx, std::memory_order_release);
    }

    /** Returns beats until the scheduled recall fires, or -1 if none pending. */
    double getRecallCountdown() const noexcept
    {
        const int idx = pendingRecallIdx_.load(std::memory_order_relaxed);
        if (idx < 0) return -1.0;
        return juce::jmax(0.0, pendingRecallBeat_.load(std::memory_order_relaxed) - globalBeat_);
    }

private:
    void firePlayback(int ch, double prevPhase, double nextPhase) noexcept;
    void finalizeHeldNotes(int ch) noexcept;
    double snapToGrid(double beat, double loopLength) const noexcept;
    void executeRecallScene(int sceneIdx) noexcept;  // called on audio thread only
};

// -----------------------------------------------------------------------------
// Inline implementation
// -----------------------------------------------------------------------------

inline double LiveLoopEngine::snapToGrid(double beat, double loopLength) const noexcept
{
    const double qStep = quantizeStep_.load(std::memory_order_relaxed);
    if (qStep <= 0.0) return beat;

    double snapped;
    if (snapForward_.load(std::memory_order_relaxed))
    {
        // Snap-forward: always move to the NEXT grid boundary.
        // This means late attacks are placed on the beat AHEAD, not behind.
        snapped = std::ceil(beat / qStep) * qStep;
        // If exactly on a grid line, stay there (ceil returns same value)
    }
    else
    {
        // Nearest: standard round-to-nearest
        snapped = std::round(beat / qStep) * qStep;
    }

    // Do NOT use fmod here: fmod(16.0, 16.0) == 0.0 which records a startBeat
    // of 0 for notes near the loop end. When the NoteOff arrives moments later
    // (still at phase ~15.9), rawLen = 15.9 - 0.0 = 15.9 beats -- the entire loop.
    // Instead, clamp: if snapping would reach exactly loopLength, step back one grid.
    if (loopLength > 0.0 && snapped >= loopLength)
        snapped = loopLength - qStep;

    if (snapped < 0.0) snapped = 0.0;
    return snapped;
}

inline void LiveLoopEngine::processBlock(int numSamples, double bpm, double sampleRate) noexcept
{
    if (sampleRate <= 0.0 || bpm <= 0.0) return;

    const double beatsThisBlock = numSamples * bpm / (60.0 * sampleRate);
    globalBeat_ += beatsThisBlock;

    // -- Bar-quantized scene recall (audio thread, lock-free) -----------------
    {
        const int recallIdx = pendingRecallIdx_.load(std::memory_order_acquire);
        if (recallIdx >= 0)
        {
            const double recallAt = pendingRecallBeat_.load(std::memory_order_relaxed);
            if (globalBeat_ >= recallAt)
            {
                pendingRecallIdx_.store(-1, std::memory_order_release);
                executeRecallScene(recallIdx);
            }
        }
    }

    for (int c = 0; c < kMaxChannels; ++c)
    {
        // -- Handle stop request (highest priority) ------------------------
        if (stopReq_[c].exchange(false, std::memory_order_acq_rel))
        {
            const auto stNow = static_cast<State>(state_[c].load(std::memory_order_relaxed));

            // Free record finalization: STOP during open-ended Recording
            if (stNow == State::Recording && ch_[c].loopLength <= 0.0)
            {
                const double elapsed = globalBeat_ - ch_[c].recordStartBeat;
                if (elapsed >= 0.5)  // need at least half a beat
                {
                    ch_[c].loopLength = elapsed;
                    finalizeHeldNotes(c);
                    ch_[c].loopPhase  = 0.0;
                    ch_[c].undoNoteCount = 0;
                    state_[c].store((uint8_t)State::Looping, std::memory_order_release);
                }
                else
                {
                    ch_[c].reset();
                    state_[c].store((uint8_t)State::Idle, std::memory_order_release);
                }
            }
            else if (stNow == State::Overdubbing)
            {
                // Stop overdub: finalize held notes, keep loop running
                finalizeHeldNotes(c);
                state_[c].store((uint8_t)State::Looping, std::memory_order_release);
            }
            else
            {
                // Normal stop: silence held notes and go Idle
                if (onNoteOff)
                    for (int p = 0; p < 128; ++p)
                        if (ch_[c].held[p].active) { onNoteOff(c, p); }
                ch_[c].reset();
                state_[c].store((uint8_t)State::Idle, std::memory_order_release);
            }

            armReq_[c].store(false, std::memory_order_release);
            overdubReq_[c].store(false, std::memory_order_release);
            undoReq_[c].store(false, std::memory_order_release);

            // If we finalized into Looping, fall through; otherwise skip
            if (static_cast<State>(state_[c].load(std::memory_order_relaxed)) != State::Looping)
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

        // -- Handle half loop request (÷2 length, trim notes) -------------
        if (halfReq_[c].exchange(false, std::memory_order_acq_rel))
        {
            const auto st = static_cast<State>(state_[c].load(std::memory_order_relaxed));
            if ((st == State::Looping || st == State::Overdubbing) && ch_[c].loopLength > 1.0)
            {
                const double newLen = ch_[c].loopLength * 0.5;
                int newCount = 0;
                for (int n = 0; n < ch_[c].noteCount; ++n)
                {
                    auto& note = ch_[c].notes[n];
                    if (!note.valid || note.startBeat >= newLen) continue;
                    if (note.endBeat > newLen || note.endBeat < note.startBeat)
                        note.endBeat = std::fmod(newLen - 0.001, newLen);
                    ch_[c].notes[newCount++] = note;
                }
                ch_[c].noteCount  = newCount;
                ch_[c].loopLength = newLen;
                ch_[c].loopPhase  = std::fmod(ch_[c].loopPhase, newLen);
                ch_[c].undoNoteCount = juce::jmin(ch_[c].undoNoteCount, newCount);
            }
        }

        // -- Handle double loop request (x2 length, duplicate notes) ------
        if (doubleReq_[c].exchange(false, std::memory_order_acq_rel))
        {
            const auto st = static_cast<State>(state_[c].load(std::memory_order_relaxed));
            if ((st == State::Looping || st == State::Overdubbing) && ch_[c].loopLength > 0.0)
            {
                const double oldLen   = ch_[c].loopLength;
                const double newLen   = oldLen * 2.0;
                const int    origCount = ch_[c].noteCount;
                for (int n = 0; n < origCount && ch_[c].noteCount < kMaxNotes; ++n)
                {
                    const auto& orig = ch_[c].notes[n];
                    if (!orig.valid) continue;
                    auto& dup     = ch_[c].notes[ch_[c].noteCount++];
                    dup           = orig;
                    dup.startBeat = orig.startBeat + oldLen;
                    dup.endBeat   = std::fmod(orig.endBeat + oldLen, newLen);
                }
                ch_[c].loopLength = newLen;
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

            // Overdubbing: finalise held notes at loop boundary, but stay in Overdubbing.
            // (Auto-revert to Looping removed -- user must call stop() explicitly.)
            if (st == State::Overdubbing && cd.loopPhase >= cd.loopLength)
                finalizeHeldNotes(c);

            if (cd.loopPhase >= cd.loopLength)
            {
                firePlayback(c, prevPhase, cd.loopLength);
                cd.loopPhase = std::fmod(cd.loopPhase, cd.loopLength);
                firePlayback(c, 0.0, cd.loopPhase);
            }
            else
            {
                firePlayback(c, prevPhase, cd.loopPhase);
            }
        }
    }
}

inline void LiveLoopEngine::processMidiEvent(const juce::MidiMessage& msg, int ch,
                                             double eventBeat) noexcept
{
    if (ch < 0 || ch >= kMaxChannels) return;
    if (!msg.isNoteOn() && !msg.isNoteOff()) return;

    const int    pitch = juce::jlimit(0, 127, msg.getNoteNumber());
    const float  vel   = msg.getFloatVelocity();
    auto& cd  = ch_[ch];
    const auto st = static_cast<State>(state_[ch].load(std::memory_order_relaxed));

    // -- Armed: first NoteOn -> schedule start --------------------------------
    if (st == State::Armed && msg.isNoteOn())
    {
        cd.loopLength = pendingLoopLength_[ch].load(std::memory_order_relaxed);
        cd.noteCount  = 0;
        cd.loopPhase  = 0.0;
        for (auto& h : cd.held) h = {};

        if (cd.loopLength <= 0.0)
        {
            // Free record: start immediately (no count-in)
            cd.recordStartBeat = eventBeat;
            state_[ch].store((uint8_t)State::Recording, std::memory_order_release);
        }
        else
        {
            // Fixed length: wait for next bar boundary + count-in bars
            const int countInBars = countInBars_.load(std::memory_order_relaxed);
            const double nextBarStart = (std::floor(eventBeat / 4.0) + 1.0) * 4.0;
            cd.recordStartBeat = nextBarStart + countInBars * 4.0;

            if (countInBars == 0)
            {
                // No count-in: start immediately at the very next bar
                cd.recordStartBeat = nextBarStart;
            }

            state_[ch].store((uint8_t)State::WaitingForBar, std::memory_order_release);
        }
        return;
    }

    // -- WaitingForBar: if bar already passed, jump straight to Recording --
    if (st == State::WaitingForBar && eventBeat >= cd.recordStartBeat)
    {
        cd.loopPhase = 0.0;
        state_[ch].store((uint8_t)State::Recording, std::memory_order_release);
    }

    // Capture for Recording and Overdubbing only
    const auto stNow = static_cast<State>(state_[ch].load(std::memory_order_relaxed));
    if (stNow != State::Recording && stNow != State::Overdubbing) return;

    // -- Compute beat position relative to loop start --------------------------
    // Use eventBeat (per-event precise timestamp) rather than globalBeat_ (end-of-block).
    // This prevents notes near the loop boundary from being recorded at the wrong phase
    // because processBlock already advanced globalBeat_ past the wrap point.
    double relBeat = std::fmax(0.0, eventBeat - cd.recordStartBeat);

    if (cd.loopLength > 0.0)
    {
        relBeat = std::fmod(relBeat, cd.loopLength);
        if (relBeat < 0.0) relBeat += cd.loopLength;
    }
    else
    {
        // Free record: no loopLength yet, relBeat = elapsed time
        // (no fmod; loopLength will be set on stop())
    }

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

        const double qStep = quantizeStep_.load(std::memory_order_relaxed);

        // Compute raw note length from start to release position.
        // We snap the LENGTH (not the end position) to avoid wrap-around
        // issues that caused notes to span almost the entire loop.
        double rawLen = relBeat - hn.startBeat;
        if (rawLen < 0.0) rawLen += cd.loopLength;  // genuine wrap-around

        // Minimum audible length: 1/64th note even when quantize is off
        const double minLen = (qStep > 0.0) ? qStep : 0.0625;

        if (qStep > 0.0)
        {
            // Snap length to nearest grid step
            rawLen = std::round(rawLen / qStep) * qStep;
        }

        // Clamp: at least minLen, at most one qStep below loop end
        const double maxLen = cd.loopLength - minLen;
        rawLen = std::fmax(minLen, std::fmin(rawLen, maxLen));

        if (cd.noteCount < kMaxNotes)
        {
            auto& note     = cd.notes[cd.noteCount++];
            note.pitch     = pitch;
            note.velocity  = hn.velocity;
            note.startBeat = hn.startBeat;
            note.endBeat   = std::fmod(hn.startBeat + rawLen, cd.loopLength);
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

inline void LiveLoopEngine::executeRecallScene(int sceneIdx) noexcept
{
    // Called on AUDIO THREAD ONLY. Reads scenes_[] written only by UI-thread saveScene.
    // saveScene and scheduleRecallScene are separated in time by user interaction,
    // so there is no concurrent write race on scenes_[].
    if (sceneIdx < 0 || sceneIdx >= kNumScenes) return;
    const auto& s = scenes_[sceneIdx];
    if (!s.occupied) return;

    // Stop all channels and finalize any held notes
    for (int c = 0; c < kMaxChannels; ++c)
    {
        const auto st = static_cast<State>(state_[c].load(std::memory_order_relaxed));
        if (st == State::Overdubbing || st == State::Recording)
            finalizeHeldNotes(c);
        ch_[c].reset();
        state_[c].store((uint8_t)State::Idle, std::memory_order_release);
    }

    // Restore tracks from scene snapshot
    for (int c = 0; c < kMaxChannels; ++c)
    {
        if (!s.tracks[c].hasLoop || s.tracks[c].noteCount <= 0) continue;
        ch_[c].loopLength    = s.tracks[c].loopLength;
        ch_[c].loopPhase     = 0.0;
        ch_[c].noteCount     = s.tracks[c].noteCount;
        ch_[c].undoNoteCount = 0;
        for (int n = 0; n < s.tracks[c].noteCount; ++n)
        {
            const auto& src = s.tracks[c].notes[n];
            auto& dst = ch_[c].notes[n];
            dst.pitch = src.pitch; dst.velocity = src.velocity;
            dst.startBeat = src.startBeat; dst.endBeat = src.endBeat;
            dst.valid = src.valid;
        }
        state_[c].store((uint8_t)State::Looping, std::memory_order_release);
    }
}
