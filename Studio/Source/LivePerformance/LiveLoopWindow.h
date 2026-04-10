#pragma once
#include <JuceHeader.h>
#include "../Audio/AudioEngine.h"
#include "LiveLoopEngine.h"

// --- Custom LookAndFeel -------------------------------------------------------
class LiveLoopLAF : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground(juce::Graphics& g, juce::Button& btn,
                              const juce::Colour& bg, bool over, bool down) override
    {
        const float r = (float)btn.getHeight() * 0.42f;
        const auto  b = btn.getLocalBounds().toFloat().reduced(0.5f);
        auto col = bg;
        if (down) col = col.darker(0.25f);
        else if (over) col = col.brighter(0.12f);
        g.setColour(col);
        g.fillRoundedRectangle(b, r);
        g.setColour(col.brighter(0.18f).withAlpha(0.5f));
        g.drawRoundedRectangle(b, r, 0.8f);
    }
    void drawButtonText(juce::Graphics& g, juce::TextButton& btn,
                        bool /*over*/, bool /*down*/) override
    {
        g.setColour(btn.findColour(juce::TextButton::textColourOffId));
        g.setFont(juce::Font(10.5f, juce::Font::bold));
        g.drawFittedText(btn.getButtonText(), btn.getLocalBounds(),
                         juce::Justification::centred, 1);
    }
    void drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h,
                          float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                          juce::Slider::SliderStyle, juce::Slider& slider) override
    {
        const float trackH = 3.0f;
        const float cy = y + h * 0.5f;
        // Track bg
        g.setColour(juce::Colour(0xff303040));
        g.fillRoundedRectangle((float)x, cy - trackH * 0.5f, (float)w, trackH, trackH * 0.5f);
        // Fill
        g.setColour(juce::Colour(0xff4466ff).withAlpha(0.7f));
        g.fillRoundedRectangle((float)x, cy - trackH * 0.5f, sliderPos - x, trackH, trackH * 0.5f);
        // Thumb
        const float th = 10.0f;
        g.setColour(juce::Colours::white.withAlpha(0.88f));
        g.fillEllipse(sliderPos - th * 0.5f, cy - th * 0.5f, th, th);
    }
    int getSliderThumbRadius(juce::Slider&) override { return 5; }
};

// --- LiveLoopComponent --------------------------------------------------------
class LiveLoopComponent : public juce::Component,
                          public juce::Timer,
                          public juce::FileDragAndDropTarget
{
public:
    // -- Callbacks (set by MainComponent) -------------------------------------
    std::function<void(int, double)>  onArmChannel;    // ch, loopBeats
    std::function<void(int)>          onArmFreeChannel; // ch — free record
    std::function<void(int)>          onStopChannel;
    std::function<void(int)>          onOverdubChannel;
    std::function<void(int)>          onUndoChannel;
    std::function<void()>             onStopAll;
    std::function<void(int, bool)>    onMuteChannel;      // ch, muted
    std::function<void(int, float)>   onVolumeChanged;    // ch, 0-1
    std::function<void(int)>          onHalfLoop;         // halve selected channel loop
    std::function<void(int)>          onDoubleLoop;       // double selected channel loop
    std::function<void(double)>       onLaunchAll;        // arm all idle channels
    std::function<void(double)>       onBpmChanged;
    std::function<void(double)>       onQuantizeChanged;
    std::function<void(bool)>         onMetronomeToggle;
    std::function<void()>             onTapTempo;
    std::function<void(int)>          onCountInChanged;   // 0/1/2/4 bars
    std::function<void(bool)>         onSnapForwardChanged;
    std::function<void(int, juce::File)> onSampleDropped; // ch, file

    // -- Data providers --------------------------------------------------------
    std::function<void(int)>           onChannelSelected;
    std::function<int()>               getSelectedChannel;
    std::function<juce::String(int)>   getChannelName;
    std::function<juce::String(int)>   getInstrumentName;
    std::function<double()>            getCurrentBeat;
    std::function<double()>            getBpm;

    AudioEngine& engine_;

    explicit LiveLoopComponent(AudioEngine& engine) : engine_(engine)
    {
        setSize(kWinW, kHeaderH + kNumCh * kRowH + kGridH + kFooterH);
        setLookAndFeel(&laf_);
        setWantsKeyboardFocus(true);

        // -- REC button -------------------------------------------------------
        styleBtn(recBtn_, kRed);
        recBtn_.setButtonText("REC");
        recBtn_.onClick = [this]
        {
            const int ch = sel();
            const auto st = engine_.liveLoopGetState(ch);
            if (st == LiveLoopEngine::State::Idle)
            {
                if (freeRecMode_)
                { if (onArmFreeChannel) onArmFreeChannel(ch); }
                else
                { if (onArmChannel) onArmChannel(ch, loopLen_[ch]); }
            }
            else if (st == LiveLoopEngine::State::Looping)
            {
                if (onOverdubChannel) onOverdubChannel(ch);
            }
            else
            {
                if (onStopChannel) onStopChannel(ch);
            }
        };
        addAndMakeVisible(recBtn_);

        // -- FREE toggle button -----------------------------------------------
        styleBtn(freeBtn_, juce::Colour(0xff1e2830));
        freeBtn_.setButtonText("FREE");
        freeBtn_.setClickingTogglesState(true);
        freeBtn_.onClick = [this]
        {
            freeRecMode_ = freeBtn_.getToggleState();
            freeBtn_.setColour(juce::TextButton::buttonColourId,
                               freeRecMode_ ? kOrange.darker(0.3f) : juce::Colour(0xff1e2830));
        };
        addAndMakeVisible(freeBtn_);

        // -- STOP / STOP ALL / UNDO buttons -----------------------------------
        styleBtn(stopBtn_, juce::Colour(0xff2a2a2a));
        stopBtn_.setButtonText("STOP");
        stopBtn_.onClick = [this] { if (onStopChannel) onStopChannel(sel()); };
        addAndMakeVisible(stopBtn_);

        styleBtn(undoBtn_, juce::Colour(0xff2a2020));
        undoBtn_.setButtonText("UNDO");
        undoBtn_.onClick = [this] { if (onUndoChannel) onUndoChannel(sel()); };
        addAndMakeVisible(undoBtn_);

        styleBtn(stopAllBtn_, juce::Colour(0xff1a1a1a));
        stopAllBtn_.setButtonText("STOP ALL");
        stopAllBtn_.onClick = [this] { if (onStopAll) onStopAll(); };
        addAndMakeVisible(stopAllBtn_);

        // -- ARM ALL (scene launch) button ------------------------------------
        styleBtn(armAllBtn_, juce::Colour(0xff1e3020));
        armAllBtn_.setButtonText("ARM ALL");
        armAllBtn_.onClick = [this]
        {
            // Use the selected channel's current loop length for all
            const double len = loopLen_[sel()];
            if (onLaunchAll) onLaunchAll(len);
        };
        addAndMakeVisible(armAllBtn_);

        // -- Scene A/B/C/D buttons (Cmd/Ctrl+click = save; click = recall) ---
        static const char* kSceneNames[4] = { "SCN A", "SCN B", "SCN C", "SCN D" };
        for (int i = 0; i < LiveLoopEngine::kNumScenes; ++i)
        {
            styleBtn(sceneBtn_[i], juce::Colour(0xff1a2030));
            sceneBtn_[i].setButtonText(kSceneNames[i]);
            const int idx = i;
            sceneBtn_[i].onClick = [this, idx]
            {
                const bool save = juce::ModifierKeys::getCurrentModifiers().isCommandDown()
                               || juce::ModifierKeys::getCurrentModifiers().isCtrlDown();
                if (save)
                    engine_.liveLoopSaveScene(idx);
                else if (engine_.liveLoopIsSceneOccupied(idx))
                    engine_.liveLoopRecallScene(idx);
                updateSceneBtns();
            };
            addAndMakeVisible(sceneBtn_[i]);
        }

        // -- BPM buttons ------------------------------------------------------
        auto setupTiny = [this](juce::TextButton& b, const juce::String& txt,
                                std::function<void()> fn)
        {
            b.setButtonText(txt);
            b.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff28283a));
            b.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.75f));
            b.onClick = fn;
            addAndMakeVisible(b);
        };
        setupTiny(bpmMinusBtn_, "-", [this]
        {
            const double cur = getBpm ? getBpm() : 120.0;
            if (onBpmChanged) onBpmChanged(juce::jmax(20.0, cur - 1.0));
        });
        setupTiny(bpmPlusBtn_,  "+", [this]
        {
            const double cur = getBpm ? getBpm() : 120.0;
            if (onBpmChanged) onBpmChanged(juce::jmin(300.0, cur + 1.0));
        });

        // -- Quantize toggle --------------------------------------------------
        styleBtn(quantBtn_, kAccent.darker(0.2f));
        quantBtn_.setButtonText("Q:1/16");
        quantBtn_.onClick = [this]
        {
            quantIdx_ = (quantIdx_ + 1) % kNumQuant;
            updateQuantBtn();
            if (onQuantizeChanged) onQuantizeChanged(kQuantSteps[quantIdx_]);
        };
        addAndMakeVisible(quantBtn_);

        // -- CLICK (metronome) button -----------------------------------------
        styleBtn(clickBtn_, juce::Colour(0xff222232));
        clickBtn_.setButtonText("CLICK");
        clickBtn_.setClickingTogglesState(true);
        clickBtn_.onClick = [this]
        {
            const bool on = clickBtn_.getToggleState();
            clickBtn_.setColour(juce::TextButton::buttonColourId,
                                on ? kAccent.darker(0.3f) : juce::Colour(0xff222232));
            if (onMetronomeToggle) onMetronomeToggle(on);
        };
        addAndMakeVisible(clickBtn_);

        // -- TAP TEMPO button -------------------------------------------------
        styleBtn(tapBtn_, juce::Colour(0xff1e2030));
        tapBtn_.setButtonText("TAP");
        tapBtn_.onClick = [this]
        {
            const double now = juce::Time::getMillisecondCounterHiRes();
            tapTimes_.push_back(now);
            if (tapTimes_.size() > 8) tapTimes_.erase(tapTimes_.begin());

            if (tapTimes_.size() >= 2)
            {
                double sum = 0.0;
                for (size_t i = 1; i < tapTimes_.size(); ++i)
                    sum += tapTimes_[i] - tapTimes_[i-1];
                const double avgMs = sum / (double)(tapTimes_.size() - 1);
                const double bpm   = juce::jlimit(20.0, 300.0, 60000.0 / avgMs);
                if (onBpmChanged) onBpmChanged(bpm);
            }
        };
        addAndMakeVisible(tapBtn_);

        // -- Count-in button --------------------------------------------------
        styleBtn(countInBtn_, juce::Colour(0xff336633).darker(0.1f));
        updateCountInBtn();
        countInBtn_.onClick = [this]
        {
            countInIdx_ = (countInIdx_ + 1) % 4;
            updateCountInBtn();
            if (onCountInChanged) onCountInChanged(kCountInBars[countInIdx_]);
        };
        addAndMakeVisible(countInBtn_);

        // -- Snap-forward toggle button ----------------------------------------
        styleBtn(snapFwdBtn_, juce::Colour(0xff1e1e2e));
        snapFwdBtn_.setButtonText("Q:NEAR");
        snapFwdBtn_.onClick = [this]
        {
            snapForward_ = !snapForward_;
            snapFwdBtn_.setButtonText(snapForward_ ? "Q:FWD" : "Q:NEAR");
            snapFwdBtn_.setColour(juce::TextButton::buttonColourId,
                                  snapForward_ ? kAccent.darker(0.3f) : juce::Colour(0xff1e1e2e));
            if (onSnapForwardChanged) onSnapForwardChanged(snapForward_);
        };
        addAndMakeVisible(snapFwdBtn_);

        // -- Per-channel rows -------------------------------------------------
        for (int c = 0; c < kNumCh; ++c)
        {
            auto& row = rows_[c];

            // Background hit-test button
            row.sel.setButtonText("");
            row.sel.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
            row.sel.onClick = [this, c] { if (onChannelSelected) onChannelSelected(c); };
            addAndMakeVisible(row.sel);

            // Loop length buttons: 1/2/4/8 bars
            for (int i = 0; i < kNumLen; ++i)
            {
                styleBtn(row.len[i], juce::Colour(0xff222232));
                row.len[i].setButtonText(kLenLabels[i]);
                const double lv = kLenBeats[i];
                row.len[i].onClick = [this, c, lv]
                {
                    loopLen_[c] = lv;
                    refreshLenButtons(c);
                };
                addAndMakeVisible(row.len[i]);
            }
            loopLen_[c] = 16.0;
            refreshLenButtons(c);

            // /2 button
            row.halfBtn.setButtonText("/2");
            row.halfBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1e2030));
            row.halfBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff7788aa));
            row.halfBtn.onClick = [this, c] { if (onHalfLoop) onHalfLoop(c); };
            addAndMakeVisible(row.halfBtn);

            // x2 button
            row.dblBtn.setButtonText("x2");
            row.dblBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1e2030));
            row.dblBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff7788aa));
            row.dblBtn.onClick = [this, c] { if (onDoubleLoop) onDoubleLoop(c); };
            addAndMakeVisible(row.dblBtn);

            // Mute button
            row.muteBtn.setButtonText("M");
            row.muteBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff252535));
            row.muteBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff888899));
            row.muteBtn.setClickingTogglesState(true);
            row.muteBtn.onClick = [this, c]
            {
                const bool m = rows_[c].muteBtn.getToggleState();
                rows_[c].muteBtn.setColour(juce::TextButton::buttonColourId,
                                      m ? juce::Colour(0xffcc6622) : juce::Colour(0xff252535));
                rows_[c].muteBtn.setColour(juce::TextButton::textColourOffId,
                                      m ? juce::Colours::white : juce::Colour(0xff888899));
                if (onMuteChannel) onMuteChannel(c, m);
            };
            addAndMakeVisible(row.muteBtn);

            // Volume slider
            row.volSlider.setRange(0.0, 1.0);
            row.volSlider.setValue(1.0, juce::dontSendNotification);
            row.volSlider.setSliderStyle(juce::Slider::LinearHorizontal);
            row.volSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
            row.volSlider.setLookAndFeel(&laf_);
            row.volSlider.onValueChange = [this, c]
            {
                if (onVolumeChanged) onVolumeChanged(c, (float)rows_[c].volSlider.getValue());
            };
            addAndMakeVisible(row.volSlider);
        }

        startTimerHz(30);
    }

    ~LiveLoopComponent() override
    {
        stopTimer();
        for (auto& row : rows_) row.volSlider.setLookAndFeel(nullptr);
        setLookAndFeel(nullptr);
    }

    void timerCallback() override { repaint(); syncRecBtn(); updateSceneBtns(); }

    // -- Keyboard shortcuts ---------------------------------------------------
    bool keyPressed(const juce::KeyPress& key) override
    {
        const int ch = sel();
        const juce::juce_wchar c = key.getTextCharacter();

        // R — arm/overdub: if idle, arm with current loop length; if looping, overdub
        if (c == 'r' || c == 'R')
        {
            const auto st = engine_.liveLoopGetState(ch);
            if (st == LiveLoopEngine::State::Idle)
            {
                if (onArmChannel) onArmChannel(ch, loopLen_[ch]);
            }
            else if (st == LiveLoopEngine::State::Looping)
            {
                if (onOverdubChannel) onOverdubChannel(ch);
            }
            return true;
        }
        // S — stop selected channel
        if (c == 's' || c == 'S')
        {
            if (onStopChannel) onStopChannel(ch);
            return true;
        }
        // M — mute toggle
        if (c == 'm' || c == 'M')
        {
            const bool muted = engine_.liveLoopGetMute(ch);
            if (onMuteChannel) onMuteChannel(ch, !muted);
            rows_[ch].muteBtn.setColour(juce::TextButton::buttonColourId,
                !muted ? kRed.darker(0.2f) : juce::Colour(0xff2a2a3a));
            return true;
        }
        // U — undo
        if (c == 'u' || c == 'U')
        {
            if (onUndoChannel) onUndoChannel(ch);
            return true;
        }
        // Space — stop all
        if (key.getKeyCode() == juce::KeyPress::spaceKey)
        {
            if (onStopAll) onStopAll();
            return true;
        }
        // Up/Down — select track
        if (key.getKeyCode() == juce::KeyPress::upKey)
        {
            if (onChannelSelected) onChannelSelected(std::max(0, ch - 1));
            return true;
        }
        if (key.getKeyCode() == juce::KeyPress::downKey)
        {
            if (onChannelSelected) onChannelSelected(std::min(kNumCh - 1, ch + 1));
            return true;
        }
        return false;
    }

    bool keyStateChanged(bool /*isDown*/) override { return false; }

    // -- File drag-and-drop (sample onto track row) ---------------------------
    bool isInterestedInFileDrag(const juce::StringArray& files) override
    {
        for (const auto& f : files)
        {
            const juce::String ext = juce::File(f).getFileExtension().toLowerCase();
            if (ext == ".wav" || ext == ".aiff" || ext == ".aif" ||
                ext == ".mp3" || ext == ".ogg"  || ext == ".flac")
                return true;
        }
        return false;
    }

    void fileDragEnter(const juce::StringArray&, int /*x*/, int y) override
    {
        dragOverChannel_ = channelAtY(y);
        repaint();
    }

    void fileDragMove(const juce::StringArray&, int /*x*/, int y) override
    {
        const int ch = channelAtY(y);
        if (ch != dragOverChannel_) { dragOverChannel_ = ch; repaint(); }
    }

    void fileDragExit(const juce::StringArray&) override
    {
        dragOverChannel_ = -1;
        repaint();
    }

    void filesDropped(const juce::StringArray& files, int /*x*/, int y) override
    {
        dragOverChannel_ = -1;
        const int ch = channelAtY(y);
        if (ch < 0 || ch >= kNumCh) { repaint(); return; }

        for (const auto& path : files)
        {
            const juce::File f(path);
            const juce::String ext = f.getFileExtension().toLowerCase();
            if (ext == ".wav" || ext == ".aiff" || ext == ".aif" ||
                ext == ".mp3" || ext == ".ogg"  || ext == ".flac")
            {
                if (onSampleDropped) onSampleDropped(ch, f);
                if (onChannelSelected) onChannelSelected(ch);
                break;
            }
        }
        repaint();
    }

    // -- Paint -----------------------------------------------------------------
    void paint(juce::Graphics& g) override
    {
        g.fillAll(kBg);
        g.setColour(kHeaderBg);
        g.fillRect(0, 0, getWidth(), kHeaderH);
        paintTransport(g);
        const int selCh = sel();
        for (int c = 0; c < kNumCh; ++c)
            paintRow(g, c, c == selCh);
        // Drag highlight overlay
        if (dragOverChannel_ >= 0 && dragOverChannel_ < kNumCh)
        {
            const int dy = rowY(dragOverChannel_);
            g.setColour(kTrackColors[dragOverChannel_ % kNumTrackColors].withAlpha(0.25f));
            g.fillRect(0, dy, getWidth(), kRowH);
            g.setColour(kTrackColors[dragOverChannel_ % kNumTrackColors].withAlpha(0.9f));
            g.drawRect(0, dy, getWidth(), kRowH, 2);
            g.setColour(juce::Colours::white.withAlpha(0.8f));
            g.setFont(juce::Font(13.0f, juce::Font::bold));
            g.drawText("Drop sample here", 0, dy, getWidth(), kRowH,
                       juce::Justification::centred);
        }
        paintNoteGrid(g);
        g.setColour(kFooterBg);
        g.fillRect(0, footerY(), getWidth(), kFooterH);
        g.setColour(juce::Colour(0xff333336));
        g.drawHorizontalLine(footerY(), 0.0f, (float)getWidth());
    }

    void resized() override
    {
        const int btnH = 20;

        // ---- Header buttons -----------------------------------------------
        // Zone 1: BPM ± (painted text at x=8..66, buttons follow)
        bpmMinusBtn_.setBounds(68, (kHeaderH - 18) / 2, 18, 18);
        bpmPlusBtn_ .setBounds(88, (kHeaderH - 18) / 2, 18, 18);
        // Zone 2: BAR/BEAT painted at x=112..200 (no buttons in this range)
        // Zone 3: control buttons start at x=204
        const int hby = (kHeaderH - btnH) / 2;
        freeBtn_ .setBounds(204, hby, 42, btnH);
        quantBtn_.setBounds(250, hby, 64, btnH);
        clickBtn_  .setBounds(318, hby, 52, btnH);
        tapBtn_    .setBounds(374, hby, 42, btnH);
        countInBtn_.setBounds(420, hby, 52, btnH);
        snapFwdBtn_.setBounds(476, hby, 58, btnH);

        // ---- Per-channel rows (use shared column constants) ---------------
        for (int c = 0; c < kNumCh; ++c)
        {
            const int ry  = rowY(c);
            const int midY = ry + (kRowH - btnH) / 2;
            rows_[c].sel.setBounds(0, ry, kWinW, kRowH);

            rows_[c].muteBtn.setBounds(kMuteX, midY, kMuteW,      btnH);
            rows_[c].dblBtn .setBounds(kDblX,  midY, kHxW,        btnH);
            rows_[c].halfBtn.setBounds(kHalfX, midY, kHxW,        btnH);

            for (int i = 0; i < kNumLen; ++i)
                rows_[c].len[i].setBounds(kLenX + i * (kLenW + kLenGap),
                                           midY, kLenW, btnH);

            rows_[c].volSlider.setBounds(kVolX, midY, kVolW, btnH);
        }

        // ---- Footer: 5 main buttons + 4 scene buttons -----------------------
        const int fy = footerY() + 6;
        const int fh = (kFooterH - 12) / 2 - 2;
        const int bw = (kWinW - 20) / 5;
        recBtn_    .setBounds(10 + bw * 0, fy, bw - 4, fh);
        stopBtn_   .setBounds(10 + bw * 1, fy, bw - 4, fh);
        undoBtn_   .setBounds(10 + bw * 2, fy, bw - 4, fh);
        armAllBtn_ .setBounds(10 + bw * 3, fy, bw - 4, fh);
        stopAllBtn_.setBounds(10 + bw * 4, fy, bw - 4, fh);

        // Scene buttons on second footer row
        const int sy  = fy + fh + 4;
        const int sbw = (kWinW - 20) / LiveLoopEngine::kNumScenes;
        for (int i = 0; i < LiveLoopEngine::kNumScenes; ++i)
            sceneBtn_[i].setBounds(10 + sbw * i, sy, sbw - 4, fh);
    }

private:
    // --- colours -------------------------------------------------------------
    const juce::Colour kBg        { 0xff141418 };
    const juce::Colour kHeaderBg  { 0xff1a1a24 };
    const juce::Colour kFooterBg  { 0xff111114 };
    const juce::Colour kRowEven   { 0xff181820 };
    const juce::Colour kRowOdd    { 0xff161618 };
    const juce::Colour kRowSel    { 0xff1e2040 };
    const juce::Colour kAccent    { 0xff4466ff };
    const juce::Colour kRed       { 0xffcc2233 };
    const juce::Colour kGreen     { 0xff22cc55 };
    const juce::Colour kYellow    { 0xffddbb00 };
    const juce::Colour kPurple    { 0xffaa33dd };
    const juce::Colour kOrange    { 0xffdd7722 };

    // Per-track identity colours (8 tracks, distinct, accessible)
    static constexpr int kNumTrackColors = 8;
    const juce::Colour kTrackColors[kNumTrackColors] = {
        juce::Colour(0xff4488ff),  // 1 – sky blue
        juce::Colour(0xff55dd88),  // 2 – mint green
        juce::Colour(0xffdd5566),  // 3 – coral
        juce::Colour(0xffffbb33),  // 4 – amber
        juce::Colour(0xffcc66ff),  // 5 – violet
        juce::Colour(0xff55ccdd),  // 6 – teal
        juce::Colour(0xffffaa55),  // 7 – peach
        juce::Colour(0xffaaddaa),  // 8 – sage
    };

    // --- layout --------------------------------------------------------------
    static constexpr int kNumCh   = 8;
    static constexpr int kNumLen  = 4;
    static constexpr int kNumQuant= 5;
    static constexpr int kHeaderH = 56;   // taller for breathing room
    static constexpr int kRowH    = 52;
    static constexpr int kGridH   = 110;
    static constexpr int kFooterH = 68;
    static constexpr int kWinW    = 760;  // wider window

    // ---- Row column positions (derived constants, computed from right) -------
    // All expressed relative to kWinW so they stay consistent between
    // resized() (widget placement) and paintTransport() (column header labels).
    static constexpr int kRightMargin = 8;
    static constexpr int kMuteW  = 24;
    static constexpr int kHxW    = 22;   // /2 and x2 button width
    static constexpr int kLenW   = 24;
    static constexpr int kLenGap = 3;
    static constexpr int kLenTotalW = kNumLen * kLenW + (kNumLen - 1) * kLenGap; // 105
    static constexpr int kVolW   = 64;

    // Right-to-left column start positions:
    static constexpr int kMuteX  = kWinW - kRightMargin - kMuteW;          // 724
    static constexpr int kDblX   = kMuteX - kHxW - 6;                      // 696
    static constexpr int kHalfX  = kDblX  - kHxW - 3;                      // 671
    static constexpr int kLenX   = kHalfX - kLenTotalW - 8;                // 558
    static constexpr int kVolX   = kLenX  - kVolW   - 10;                  // 484

    static constexpr double kLenBeats[4]      = { 4.0,  8.0, 16.0, 32.0 };
    static constexpr const char* kLenLabels[4]= { "1",  "2",  "4",  "8"  };
    static constexpr double kQuantSteps [5]   = { 0.0, 1.0, 0.5, 0.25, 0.125 };
    static constexpr const char* kQuantLabels[5] = { "FREE","1/4","1/8","1/16","1/32" };

    int rowY(int c)   const { return kHeaderH + c * kRowH; }
    int gridY()       const { return kHeaderH + kNumCh * kRowH; }
    int footerY()     const { return gridY() + kGridH; }
    int sel()         const { return getSelectedChannel ? getSelectedChannel() : 0; }

    int channelAtY(int y) const
    {
        if (y < kHeaderH) return -1;
        const int c = (y - kHeaderH) / kRowH;
        return (c >= 0 && c < kNumCh) ? c : -1;
    }

    // --- subcomponents -------------------------------------------------------
    struct Row
    {
        juce::TextButton sel, len[4], halfBtn, dblBtn, muteBtn;
        juce::Slider     volSlider;
    };
    Row              rows_[kNumCh];
    juce::TextButton recBtn_, freeBtn_, stopBtn_, undoBtn_, stopAllBtn_, armAllBtn_;
    juce::TextButton bpmMinusBtn_, bpmPlusBtn_;
    juce::TextButton quantBtn_, clickBtn_, tapBtn_, countInBtn_, snapFwdBtn_;
    juce::TextButton sceneBtn_[LiveLoopEngine::kNumScenes];
    LiveLoopLAF      laf_;

    double           loopLen_[kNumCh] {};
    bool             freeRecMode_  = false;
    bool             snapForward_  = false;
    int              quantIdx_   = 3;   // default: 1/16
    int              countInIdx_ = 1;   // index into kCountInBars[]
    int              dragOverChannel_ = -1;
    std::vector<double> tapTimes_;

    static constexpr int kCountInBars[4] = { 0, 1, 2, 4 };
    static constexpr const char* kCountInLabels[4] = { "0 bar", "1 bar", "2 bar", "4 bar" };

    // --- helpers -------------------------------------------------------------
    void styleBtn(juce::TextButton& b, juce::Colour bg)
    {
        b.setColour(juce::TextButton::buttonColourId, bg);
        b.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.9f));
    }

    void updateQuantBtn()
    {
        const bool active = kQuantSteps[quantIdx_] > 0.0;
        quantBtn_.setButtonText(juce::String("Q:") + kQuantLabels[quantIdx_]);
        quantBtn_.setColour(juce::TextButton::buttonColourId,
                            active ? kAccent.darker(0.2f) : juce::Colour(0xff1e1e2e));
    }

    void updateCountInBtn()
    {
        const int bars = kCountInBars[countInIdx_];
        const bool hasCountIn = (bars > 0);
        countInBtn_.setButtonText(juce::String("CI:") + kCountInLabels[countInIdx_]);
        countInBtn_.setColour(juce::TextButton::buttonColourId,
                              hasCountIn ? juce::Colour(0xff336633).darker(0.1f)
                                         : juce::Colour(0xff1e1e2e));
    }

    void updateSceneBtns()
    {
        static const char* kSceneNames[4] = { "SCN A", "SCN B", "SCN C", "SCN D" };
        for (int i = 0; i < LiveLoopEngine::kNumScenes; ++i)
        {
            const bool occ = engine_.liveLoopIsSceneOccupied(i);
            sceneBtn_[i].setColour(juce::TextButton::buttonColourId,
                occ ? juce::Colour(0xff224444) : juce::Colour(0xff1a2030));
            sceneBtn_[i].setButtonText(occ ? juce::String(kSceneNames[i]) + " *"
                                           : juce::String(kSceneNames[i]));
        }
    }

    void refreshLenButtons(int c)
    {
        for (int i = 0; i < kNumLen; ++i)
        {
            const bool on = (loopLen_[c] == kLenBeats[i]);
            rows_[c].len[i].setColour(juce::TextButton::buttonColourId,
                on ? kAccent.darker(0.1f) : juce::Colour(0xff222232));
            rows_[c].len[i].setColour(juce::TextButton::textColourOffId,
                on ? juce::Colours::white : juce::Colour(0xff888899));
        }
    }

    void syncRecBtn()
    {
        const auto st = engine_.liveLoopGetState(sel());
        switch (st)
        {
            case LiveLoopEngine::State::Idle:
                recBtn_.setButtonText("REC");
                recBtn_.setColour(juce::TextButton::buttonColourId, kRed);
                break;
            case LiveLoopEngine::State::Looping:
                recBtn_.setButtonText("+ OVR");
                recBtn_.setColour(juce::TextButton::buttonColourId, kOrange);
                break;
            default:
                recBtn_.setButtonText("STOP REC");
                recBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff882222));
                break;
        }
    }

    // --- Transport header -----------------------------------------------------
    void paintTransport(juce::Graphics& g)
    {
        const double beat = getCurrentBeat ? getCurrentBeat() : 0.0;
        const double bpmV = getBpm         ? getBpm()         : 120.0;
        const int    bar  = (int)(beat / 4.0) + 1;
        const double bib  = std::fmod(beat, 4.0) + 1.0;

        // Zone 1: BPM (x=8..107)
        g.setColour(juce::Colours::white.withAlpha(0.32f));
        g.setFont(juce::Font(9.5f));
        g.drawText("BPM", 8, 8, 30, 13, juce::Justification::centredLeft);
        g.setColour(juce::Colours::white.withAlpha(0.88f));
        g.setFont(juce::Font(21.0f, juce::Font::bold));
        g.drawText(juce::String((int)bpmV), 8, 22, 58, 24, juce::Justification::centredLeft);
        // separator
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.drawVerticalLine(108, 8.0f, (float)kHeaderH - 8.0f);

        // Zone 2: BAR / BEAT (x=112..200)
        g.setColour(juce::Colours::white.withAlpha(0.32f));
        g.setFont(juce::Font(9.5f));
        g.drawText("BAR",  112, 8, 30, 13, juce::Justification::centredLeft);
        g.setColour(juce::Colours::white.withAlpha(0.88f));
        g.setFont(juce::Font(21.0f, juce::Font::bold));
        g.drawText(juce::String(bar), 112, 22, 38, 24, juce::Justification::centredLeft);

        g.setColour(juce::Colours::white.withAlpha(0.32f));
        g.setFont(juce::Font(9.5f));
        g.drawText("BEAT", 154, 8, 34, 13, juce::Justification::centredLeft);
        g.setColour(kAccent.withAlpha(0.92f));
        g.setFont(juce::Font(21.0f, juce::Font::bold));
        g.drawText(juce::String(bib, 1), 154, 22, 44, 24, juce::Justification::centredLeft);
        // separator
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.drawVerticalLine(200, 8.0f, (float)kHeaderH - 8.0f);

        // Zone 3: button labels are on the buttons themselves — no painted text

        // Column headers (right section, aligned with row buttons)
        g.setColour(juce::Colours::white.withAlpha(0.22f));
        g.setFont(juce::Font(9.0f));
        g.drawText("VOL",  kVolX,                     3, kVolW,        14, juce::Justification::centred);
        g.drawText("BARS", kLenX,                     3, kLenTotalW,   14, juce::Justification::centred);
        g.drawText("/2 x2",kHalfX,                    3, kDblX + kHxW - kHalfX, 14, juce::Justification::centred);
        g.drawText("M",    kMuteX,                    3, kMuteW,       14, juce::Justification::centred);

        // Count-in overlay: big countdown when any channel is waiting
        bool anyWaiting = false;
        double tillBeats = 0.0;
        int    waitingCh = -1;
        for (int c = 0; c < kNumCh; ++c)
        {
            if (engine_.liveLoopGetState(c) == LiveLoopEngine::State::WaitingForBar)
            {
                anyWaiting = true;
                tillBeats  = engine_.liveLoopGetBeatsTillRecord(c);
                waitingCh  = c;
                break;
            }
        }
        if (anyWaiting)
        {
            // Show beat-level countdown: 4…3…2…1  with sub-beat pulse
            const int beatsLeft = juce::jmax(1, (int)std::ceil(tillBeats));
            const double subBeat = tillBeats - std::floor(tillBeats);
            const float pulse = 0.55f + 0.45f * (float)(1.0 - subBeat); // bright at beat top

            // Full-header dim red wash
            g.setColour(kRed.withAlpha(0.08f));
            g.fillRect(0, 0, getWidth(), kHeaderH);

            // Track colour accent strip
            const juce::Colour tCol = kTrackColors[waitingCh % kNumTrackColors];
            g.setColour(tCol.withAlpha(0.18f));
            g.fillRect(0, 0, getWidth(), kHeaderH);

            // Big countdown number centred
            g.setColour(kRed.withAlpha(pulse));
            g.setFont(juce::Font(36.0f, juce::Font::bold));
            g.drawText(juce::String(beatsLeft),
                       getWidth() / 2 - 50, 2, 100, kHeaderH - 4,
                       juce::Justification::centred);

            // "COUNT IN" label below
            g.setColour(juce::Colours::white.withAlpha(0.45f));
            g.setFont(juce::Font(9.0f, juce::Font::bold));
            g.drawText("COUNT IN", getWidth() / 2 - 40, kHeaderH - 14, 80, 11,
                       juce::Justification::centred);
        }

        // Scene recall countdown (if pending)
        {
            const double recallCountdown = engine_.liveLoopGetRecallCountdown();
            if (recallCountdown >= 0.0)
            {
                const int bLeft = juce::jmax(1, (int)std::ceil(recallCountdown));
                g.setColour(juce::Colour(0xff224444).withAlpha(0.18f));
                g.fillRect(0, 0, getWidth(), kHeaderH);
                g.setColour(juce::Colour(0xff55ccdd).withAlpha(0.9f));
                g.setFont(juce::Font(11.0f, juce::Font::bold));
                g.drawText("SCENE IN " + juce::String(bLeft),
                           getWidth() - 90, kHeaderH - 16, 84, 12,
                           juce::Justification::centredRight);
            }
        }
    }

    // --- Single track row -----------------------------------------------------
    void paintRow(juce::Graphics& g, int c, bool selected)
    {
        const int y   = rowY(c);
        const auto st = engine_.liveLoopGetState(c);

        // Per-track accent colour
        const juce::Colour trackCol = kTrackColors[c % kNumTrackColors];

        const juce::Colour rowBg = selected
            ? trackCol.withAlpha(0.12f).overlaidWith(juce::Colour(0xff1a1a2a))
            : (c % 2 == 0 ? kRowEven : kRowOdd);
        g.setColour(rowBg);
        g.fillRect(0, y, getWidth(), kRowH);

        // Left accent stripe — thick and vivid when selected
        if (selected)
        {
            g.setColour(trackCol.withAlpha(0.92f));
            g.fillRect(0, y, 4, kRowH);
        }
        else
        {
            g.setColour(trackCol.withAlpha(0.25f));
            g.fillRect(0, y, 2, kRowH);
        }

        g.setColour(juce::Colour(0xff252528));
        g.drawHorizontalLine(y + kRowH - 1, 0.0f, (float)getWidth());

        // -- State dot --------------------------------------------------------
        const auto [dotCol, stateLabel] = stateStyle(st);
        auto dot = dotCol;
        if (st == LiveLoopEngine::State::Armed      ||
            st == LiveLoopEngine::State::Recording  ||
            st == LiveLoopEngine::State::Overdubbing)
        {
            const double t = juce::Time::getMillisecondCounterHiRes() / 550.0;
            if (std::fmod(t, 1.0) > 0.5) dot = dot.withAlpha(0.25f);
        }
        const float cx = 18.0f, cy = y + kRowH * 0.5f;
        if (st != LiveLoopEngine::State::Idle)
        {
            g.setColour(dot.withAlpha(0.15f));
            g.fillEllipse(cx - 7.0f, cy - 7.0f, 14.0f, 14.0f);
        }
        g.setColour(dot);
        g.fillEllipse(cx - 5.0f, cy - 5.0f, 10.0f, 10.0f);

        // -- Channel number ----------------------------------------------------
        g.setColour(selected ? trackCol.withAlpha(0.9f)
                             : juce::Colours::white.withAlpha(0.26f));
        g.setFont(juce::Font(9.5f, selected ? juce::Font::bold : 0));
        g.drawText(juce::String::formatted("%02d", c + 1), 30, y, 22, kRowH,
                   juce::Justification::centredLeft);

        // -- Track name / instrument -------------------------------------------
        const juce::String name  = getChannelName    ? getChannelName(c)    : "Track " + juce::String(c+1);
        const juce::String instr = getInstrumentName ? getInstrumentName(c) : "";
        g.setColour(selected ? juce::Colours::white : juce::Colours::white.withAlpha(0.75f));
        g.setFont(juce::Font(12.5f, juce::Font::bold));
        g.drawText(name, 56, y + 6, 110, 17, juce::Justification::centredLeft);
        g.setColour(selected ? trackCol.withAlpha(0.7f) : juce::Colours::white.withAlpha(0.30f));
        g.setFont(juce::Font(10.0f));
        g.drawText(instr, 56, y + 25, 110, 14, juce::Justification::centredLeft);

        // -- State badge -------------------------------------------------------
        paintBadge(g, stateLabel, dotCol, 170, y + (kRowH - 18) / 2, 60, 18);

        // -- Info + mini note strip area ---------------------------------------
        {
            const int ix  = 240;
            const int iw  = kVolX - ix - 8;
            const int icy = y + kRowH / 2;

            const double loopLen = engine_.liveLoopGetChannelLength(c);

            if (st != LiveLoopEngine::State::Idle && loopLen > 0.0)
            {
                // Loop length as bars
                const int bars = (int)std::round(loopLen / 4.0);
                const juce::String lenStr = (bars <= 0) ? juce::String(loopLen, 1) + "b"
                                          : (bars == 1 ? "1 bar" : juce::String(bars) + " bars");
                g.setColour(juce::Colours::white.withAlpha(0.50f));
                g.setFont(juce::Font(10.5f, juce::Font::bold));
                g.drawText(lenStr, ix, icy - 16, 70, 14, juce::Justification::centredLeft);

                // Mini piano-roll strip
                if (st == LiveLoopEngine::State::Looping || st == LiveLoopEngine::State::Overdubbing)
                {
                    LiveLoopEngine::NoteDisplayItem dispNotes[512];
                    const int nc = engine_.liveLoopGetNotesForDisplay(c, dispNotes, 512);

                    if (nc > 0)
                    {
                        // Find pitch range for this track
                        int pitchMin = 127, pitchMax = 0;
                        for (int i = 0; i < nc; ++i)
                        {
                            pitchMin = std::min(pitchMin, dispNotes[i].pitch);
                            pitchMax = std::max(pitchMax, dispNotes[i].pitch);
                        }
                        const int pitchRange = std::max(pitchMax - pitchMin, 11); // at least one octave

                        const int sx  = ix;
                        const int sw  = iw;
                        const int shy = y + 6;
                        const int shh = kRowH - 16;

                        // Strip background
                        g.setColour(juce::Colour(0xff1a1a28));
                        g.fillRoundedRectangle((float)sx, (float)shy, (float)sw, (float)shh, 2.0f);

                        // Notes
                        for (int i = 0; i < nc; ++i)
                        {
                            const auto& n = dispNotes[i];
                            const float nx  = sx + (float)(n.startBeat / loopLen) * sw;
                            double nlen = n.endBeat - n.startBeat;
                            if (nlen <= 0.0) nlen += loopLen;
                            const float nw  = std::fmax(1.5f, (float)(nlen / loopLen) * sw);
                            const float ny  = shy + shh - 2 - (float)(n.pitch - pitchMin) / pitchRange * (shh - 4);
                            const float nh  = std::fmax(2.0f, n.velocity * 4.0f);

                            g.setColour(trackCol.withAlpha(0.75f));
                            g.fillRoundedRectangle(nx, ny - nh * 0.5f, nw, nh, 1.0f);
                        }

                        // Playhead cursor
                        const double phase  = engine_.liveLoopGetPhase(c);
                        const float  curX   = sx + (float)(phase / loopLen) * sw;
                        g.setColour(juce::Colours::white.withAlpha(0.7f));
                        g.drawVerticalLine((int)curX, (float)shy, (float)(shy + shh));

                        g.setColour(juce::Colours::white.withAlpha(0.22f));
                        g.setFont(juce::Font(8.0f));
                        g.drawText(juce::String(nc) + "n", sx + 2, shy + 1, 30, 10,
                                   juce::Justification::centredLeft);
                    }
                }
            }
            else if (st == LiveLoopEngine::State::Idle)
            {
                const juce::String instrHint = getInstrumentName ? getInstrumentName(c) : "";
                if (instrHint.isNotEmpty())
                {
                    g.setColour(trackCol.withAlpha(0.45f));
                    g.setFont(juce::Font(9.5f));
                    g.drawText(instrHint, ix, icy - 6, iw, 14, juce::Justification::centredLeft);
                }
            }

            // Progress bar along bottom edge
            if (st == LiveLoopEngine::State::Looping     ||
                st == LiveLoopEngine::State::Recording   ||
                st == LiveLoopEngine::State::Overdubbing)
            {
                const double phase = engine_.liveLoopGetPhase(c);
                const double llen  = engine_.liveLoopGetChannelLength(c);
                const double ratio = (llen > 0.0) ? juce::jlimit(0.0, 1.0, phase / llen) : 0.0;
                g.setColour(juce::Colour(0xff202028));
                g.fillRect(0, y + kRowH - 3, getWidth(), 3);
                g.setColour(trackCol.withAlpha(0.6f));
                g.fillRect(0, y + kRowH - 3, (int)(getWidth() * ratio), 3);
            }
        }
    }

    // --- State -> colour + label -----------------------------------------------
    std::pair<juce::Colour, juce::String> stateStyle(LiveLoopEngine::State st) const
    {
        switch (st)
        {
            case LiveLoopEngine::State::Armed:        return { kPurple, "ARMED" };
            case LiveLoopEngine::State::WaitingForBar:return { kYellow, "WAIT"  };
            case LiveLoopEngine::State::Recording:    return { kRed,    "REC"   };
            case LiveLoopEngine::State::Overdubbing:  return { kOrange, "OVR"   };
            case LiveLoopEngine::State::Looping:      return { kGreen,  "LOOP"  };
            default:                                  return { juce::Colour(0xff404048), "IDLE" };
        }
    }

    // --- Note input grid ------------------------------------------------------
    void paintNoteGrid(juce::Graphics& g)
    {
        const int selCh = sel();
        const auto st   = engine_.liveLoopGetState(selCh);
        const int py = gridY();

        g.setColour(juce::Colour(0xff0e0e14));
        g.fillRect(0, py, getWidth(), kGridH);
        g.setColour(juce::Colour(0xff252530));
        g.drawHorizontalLine(py, 0.0f, (float)getWidth());

        const int gx = 10, gy = py + 8;
        const int gw = getWidth() - 20, gh = kGridH - 16;

        g.setColour(juce::Colour(0xff0a0a10));
        g.fillRoundedRectangle((float)gx, (float)gy, (float)gw, (float)gh, 4.0f);
        g.setColour(juce::Colour(0xff252530));
        g.drawRoundedRectangle((float)gx + 0.5f, (float)gy + 0.5f,
                               (float)gw - 1.0f, (float)gh - 1.0f, 4.0f, 0.6f);

        const double loopLen = engine_.liveLoopGetChannelLength(selCh);
        if (loopLen <= 0.0)
        {
            g.setColour(juce::Colours::white.withAlpha(0.16f));
            g.setFont(juce::Font(11.0f));
            g.drawText("Press REC to start recording  |  Choose loop length with bar buttons",
                       gx, gy, gw, gh, juce::Justification::centred);
            return;
        }

        // Quantize grid lines
        if (quantIdx_ > 0)
        {
            const double qStep = kQuantSteps[quantIdx_];
            g.setColour(juce::Colour(0xff1e1e30));
            for (double b = qStep; b < loopLen; b += qStep)
            {
                const float x = gx + (float)(b / loopLen * gw);
                g.fillRect(x, (float)gy, 0.8f, (float)gh);
            }
        }
        // Beat lines
        for (int b = 1; b <= (int)loopLen; ++b)
        {
            const float x = gx + (float)(b / loopLen * gw);
            const bool isBar = (b % 4 == 0);
            g.setColour(isBar ? juce::Colour(0xff353548) : juce::Colour(0xff252535));
            g.fillRect(x, (float)gy, isBar ? 1.2f : 0.7f, (float)gh);
        }
        // Pitch octave lanes
        for (int oct = 1; oct < 7; ++oct)
        {
            const float y = gy + gh - (float)(oct / 7.0 * gh);
            g.setColour(juce::Colour(0xff1a1a28));
            g.drawHorizontalLine((int)y, (float)gx, (float)(gx + gw));
        }

        // Recorded notes
        if (st != LiveLoopEngine::State::Idle)
        {
            LiveLoopEngine::NoteDisplayItem notes[512];
            const int n = engine_.liveLoopGetNotesForDisplay(selCh, notes, 512);
            for (int i = 0; i < n; ++i)
            {
                const auto& nd = notes[i];
                const float x1 = gx + (float)(nd.startBeat / loopLen * gw);
                float       x2 = gx + (float)(nd.endBeat   / loopLen * gw);
                if (x2 <= x1) x2 = gx + gw;
                const float fw = std::fmax(3.0f, x2 - x1);
                const float pitchNorm = juce::jlimit(0.0f, 1.0f, (nd.pitch - 24) / 84.0f);
                const float ny = gy + gh - pitchNorm * gh - 3.0f;
                const auto col = kAccent.withBrightness(0.45f + nd.velocity * 0.55f);
                g.setColour(col.withAlpha(0.8f));
                g.fillRoundedRectangle(x1, ny, fw, 5.0f, 2.0f);
            }
        }

        // Playback cursor
        if (st == LiveLoopEngine::State::Looping    ||
            st == LiveLoopEngine::State::Recording  ||
            st == LiveLoopEngine::State::Overdubbing)
        {
            const double phase = engine_.liveLoopGetPhase(selCh);
            const float  cx    = gx + (float)(phase / loopLen * gw);
            g.setColour(juce::Colours::white.withAlpha(0.8f));
            g.fillRect(cx, (float)gy, 1.5f, (float)gh);
        }

        // Label
        g.setColour(juce::Colours::white.withAlpha(0.26f));
        g.setFont(juce::Font(9.0f));
        const juce::String label = "NOTE GRID  TRACK " + juce::String(selCh + 1)
                                 + "  |  " + juce::String(loopLen, 0) + " BEATS";
        g.drawText(label, gx + 6, gy + 3, gw - 12, 12, juce::Justification::centredLeft);

        if (st == LiveLoopEngine::State::Idle || st == LiveLoopEngine::State::Armed)
        {
            g.setColour(juce::Colours::white.withAlpha(0.18f));
            g.setFont(juce::Font(11.0f));
            g.drawText("Press REC then play notes to record", gx, gy, gw, gh,
                       juce::Justification::centred);
        }
    }

    // --- Pill badge -----------------------------------------------------------
    void paintBadge(juce::Graphics& g, const juce::String& text,
                    juce::Colour col, int x, int y, int w, int h)
    {
        const float r = h * 0.4f;
        g.setColour(col.withAlpha(0.15f));
        g.fillRoundedRectangle((float)x, (float)y, (float)w, (float)h, r);
        g.setColour(col.withAlpha(0.55f));
        g.drawRoundedRectangle((float)x + 0.5f, (float)y + 0.5f,
                               (float)w - 1.0f, (float)h - 1.0f, r, 0.8f);
        g.setColour(col.brighter(0.3f));
        g.setFont(juce::Font(9.5f, juce::Font::bold));
        g.drawText(text, x, y, w, h, juce::Justification::centred);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LiveLoopComponent)
};

// --- Window wrapper -----------------------------------------------------------
class LiveLoopWindow : public juce::DocumentWindow
{
public:
    LiveLoopComponent content;

    explicit LiveLoopWindow(AudioEngine& engine)
        : juce::DocumentWindow("Live Loop",
                               juce::Colour(0xff141418),
                               juce::DocumentWindow::closeButton),
          content(engine)
    {
        setUsingNativeTitleBar(true);
        setContentNonOwned(&content, true);
        setResizable(false, false);
        centreWithSize(content.getWidth(), content.getHeight());
    }

    std::function<void()> onClose;

    void closeButtonPressed() override
    {
        setVisible(false);
        if (onClose) onClose();
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LiveLoopWindow)
};
