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
                          public juce::Timer
{
public:
    // -- Callbacks (set by MainComponent) -------------------------------------
    std::function<void(int, double)>  onArmChannel;    // ch, loopBeats
    std::function<void(int)>          onStopChannel;
    std::function<void(int)>          onOverdubChannel;
    std::function<void(int)>          onUndoChannel;
    std::function<void()>             onStopAll;
    std::function<void(int, bool)>    onMuteChannel;   // ch, muted
    std::function<void(int, float)>   onVolumeChanged; // ch, 0-1
    std::function<void(double)>       onBpmChanged;
    std::function<void(double)>       onQuantizeChanged;
    std::function<void(bool)>         onMetronomeToggle;
    std::function<void()>             onTapTempo;

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

        // -- REC button -------------------------------------------------------
        styleBtn(recBtn_, kRed);
        recBtn_.setButtonText("REC");
        recBtn_.onClick = [this]
        {
            const int ch = sel();
            const auto st = engine_.liveLoopGetState(ch);
            if (st == LiveLoopEngine::State::Idle)
            {
                if (onArmChannel) onArmChannel(ch, loopLen_[ch]);
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

    void timerCallback() override { repaint(); syncRecBtn(); }

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
        paintNoteGrid(g);
        g.setColour(kFooterBg);
        g.fillRect(0, footerY(), getWidth(), kFooterH);
        g.setColour(juce::Colour(0xff333336));
        g.drawHorizontalLine(footerY(), 0.0f, (float)getWidth());
    }

    void resized() override
    {
        // Per-channel rows
        for (int c = 0; c < kNumCh; ++c)
        {
            const int ry = rowY(c);
            rows_[c].sel.setBounds(0, ry, getWidth(), kRowH);

            // Mute button: far right area
            const int muteW = 24, muteH = 20;
            const int muteX = getWidth() - muteW - 10;
            rows_[c].muteBtn.setBounds(muteX, ry + (kRowH - muteH) / 2, muteW, muteH);

            // Len buttons: left of mute
            const int lenW = 26, lenGap = 3;
            const int lenTotalW = kNumLen * lenW + (kNumLen - 1) * lenGap;
            const int lenX = muteX - lenTotalW - 8;
            for (int i = 0; i < kNumLen; ++i)
                rows_[c].len[i].setBounds(lenX + i * (lenW + lenGap),
                                          ry + (kRowH - 20) / 2, lenW, 20);

            // Volume slider: left of len buttons
            const int volW = 68, volH = 20;
            const int volX = lenX - volW - 10;
            rows_[c].volSlider.setBounds(volX, ry + (kRowH - volH) / 2, volW, volH);
        }

        // Header buttons
        const int hmy = (kHeaderH - 18) / 2;
        bpmMinusBtn_.setBounds(72,  hmy, 18, 18);
        bpmPlusBtn_ .setBounds(92,  hmy, 18, 18);
        quantBtn_   .setBounds(224, (kHeaderH - 20) / 2, 64, 20);
        clickBtn_   .setBounds(296, (kHeaderH - 20) / 2, 52, 20);
        tapBtn_     .setBounds(354, (kHeaderH - 20) / 2, 42, 20);

        // Footer buttons
        const int fy = footerY() + 10;
        const int fh = kFooterH - 20;
        const int fw = getWidth();
        recBtn_    .setBounds(12,           fy, fw / 4 - 16, fh);
        stopBtn_   .setBounds(fw / 4 - 2,   fy, fw / 4 - 4,  fh);
        undoBtn_   .setBounds(fw * 2 / 4 - 4, fy, fw / 4 - 4,  fh);
        stopAllBtn_.setBounds(fw * 3 / 4 - 6, fy, fw / 4 - 6,  fh);
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

    // --- layout --------------------------------------------------------------
    static constexpr int kNumCh   = 8;
    static constexpr int kNumLen  = 4;
    static constexpr int kNumQuant= 5;
    static constexpr int kHeaderH = 52;
    static constexpr int kRowH    = 52;
    static constexpr int kGridH   = 110;
    static constexpr int kFooterH = 68;
    static constexpr int kWinW    = 660;

    static constexpr double kLenBeats[4]      = { 4.0,  8.0, 16.0, 32.0 };
    static constexpr const char* kLenLabels[4]= { "1",  "2",  "4",  "8"  };
    static constexpr double kQuantSteps [5]   = { 0.0, 1.0, 0.5, 0.25, 0.125 };
    static constexpr const char* kQuantLabels[5] = { "FREE","1/4","1/8","1/16","1/32" };

    int rowY(int c)   const { return kHeaderH + c * kRowH; }
    int gridY()       const { return kHeaderH + kNumCh * kRowH; }
    int footerY()     const { return gridY() + kGridH; }
    int sel()         const { return getSelectedChannel ? getSelectedChannel() : 0; }

    // --- subcomponents -------------------------------------------------------
    struct Row
    {
        juce::TextButton sel, len[4], muteBtn;
        juce::Slider     volSlider;
    };
    Row              rows_[kNumCh];
    juce::TextButton recBtn_, stopBtn_, undoBtn_, stopAllBtn_;
    juce::TextButton bpmMinusBtn_, bpmPlusBtn_;
    juce::TextButton quantBtn_, clickBtn_, tapBtn_;
    LiveLoopLAF      laf_;

    double           loopLen_[kNumCh] {};
    int              quantIdx_ = 3;   // default: 1/16
    std::vector<double> tapTimes_;

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

        // BPM
        g.setColour(juce::Colours::white.withAlpha(0.32f));
        g.setFont(juce::Font(9.5f));
        g.drawText("BPM", 12, 8, 30, 13, juce::Justification::centredLeft);
        g.setColour(juce::Colours::white.withAlpha(0.88f));
        g.setFont(juce::Font(21.0f, juce::Font::bold));
        g.drawText(juce::String((int)bpmV), 12, 20, 58, 22, juce::Justification::centredLeft);

        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.drawVerticalLine(116, 8.0f, (float)kHeaderH - 8.0f);

        // BAR / BEAT
        g.setColour(juce::Colours::white.withAlpha(0.32f));
        g.setFont(juce::Font(9.5f));
        g.drawText("BAR", 124, 8, 28, 13, juce::Justification::centredLeft);
        g.setColour(juce::Colours::white.withAlpha(0.88f));
        g.setFont(juce::Font(21.0f, juce::Font::bold));
        g.drawText(juce::String(bar), 124, 20, 36, 22, juce::Justification::centredLeft);

        g.setColour(juce::Colours::white.withAlpha(0.32f));
        g.setFont(juce::Font(9.5f));
        g.drawText("BEAT", 164, 8, 34, 13, juce::Justification::centredLeft);
        g.setColour(kAccent.withAlpha(0.92f));
        g.setFont(juce::Font(21.0f, juce::Font::bold));
        g.drawText(juce::String(bib, 1), 164, 20, 46, 22, juce::Justification::centredLeft);

        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.drawVerticalLine(218, 8.0f, (float)kHeaderH - 8.0f);

        // Column headers (right)
        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.setFont(juce::Font(9.0f));
        g.drawText("VOL", getWidth() - 155, 32, 60, 12, juce::Justification::centred);
        g.drawText("BARS",getWidth() - 92, 32, 76, 12, juce::Justification::centred);
        g.drawText("M",   getWidth() - 15, 32, 12, 12, juce::Justification::centred);

        // Count-in overlay: show big countdown when any channel is waiting
        bool anyWaiting = false;
        int  countdownBeat = 0;
        for (int c = 0; c < kNumCh; ++c)
        {
            if (engine_.liveLoopGetState(c) == LiveLoopEngine::State::WaitingForBar)
            {
                anyWaiting = true;
                const double till = engine_.liveLoopGetBeatsTillRecord(c);
                countdownBeat = juce::jmax(1, (int)std::ceil(till));
                break;
            }
        }
        if (anyWaiting)
        {
            g.setColour(kRed.withAlpha(0.85f));
            g.setFont(juce::Font(28.0f, juce::Font::bold));
            g.drawText(juce::String(countdownBeat),
                       getWidth() / 2 - 40, 8, 80, kHeaderH - 8,
                       juce::Justification::centred);
            g.setColour(kRed.withAlpha(0.4f));
            g.setFont(juce::Font(10.0f));
            g.drawText("COUNT IN", getWidth() / 2 - 40, 38, 80, 12,
                       juce::Justification::centred);
        }
    }

    // --- Single track row -----------------------------------------------------
    void paintRow(juce::Graphics& g, int c, bool selected)
    {
        const int y   = rowY(c);
        const auto st = engine_.liveLoopGetState(c);

        const juce::Colour rowBg = selected ? kRowSel
                                 : (c % 2 == 0 ? kRowEven : kRowOdd);
        g.setColour(rowBg);
        g.fillRect(0, y, getWidth(), kRowH);

        if (selected) { g.setColour(kAccent); g.fillRect(0, y, 3, kRowH); }

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
        g.setColour(selected ? juce::Colours::white.withAlpha(0.5f)
                             : juce::Colours::white.withAlpha(0.26f));
        g.setFont(juce::Font(9.5f));
        g.drawText(juce::String::formatted("%02d", c + 1), 30, y, 22, kRowH,
                   juce::Justification::centredLeft);

        // -- Track name / instrument -------------------------------------------
        const juce::String name  = getChannelName    ? getChannelName(c)    : "Track " + juce::String(c+1);
        const juce::String instr = getInstrumentName ? getInstrumentName(c) : "";
        g.setColour(selected ? juce::Colours::white : juce::Colours::white.withAlpha(0.75f));
        g.setFont(juce::Font(12.5f, juce::Font::bold));
        g.drawText(name, 56, y + 6, 110, 17, juce::Justification::centredLeft);
        g.setColour(juce::Colours::white.withAlpha(0.30f));
        g.setFont(juce::Font(10.0f));
        g.drawText(instr, 56, y + 25, 110, 14, juce::Justification::centredLeft);

        // -- State badge -------------------------------------------------------
        paintBadge(g, stateLabel, dotCol, 170, y + (kRowH - 18) / 2, 60, 18);

        // -- Loop progress bar -------------------------------------------------
        if (st == LiveLoopEngine::State::Recording  ||
            st == LiveLoopEngine::State::Looping     ||
            st == LiveLoopEngine::State::Overdubbing)
        {
            const double phase   = engine_.liveLoopGetPhase(c);
            const double loopLen = engine_.liveLoopGetChannelLength(c);
            const double ratio   = (loopLen > 0.0) ? (phase / loopLen) : 0.0;
            const int bx = 170, bw = 60, bh = 3, bby = y + kRowH - 7;
            g.setColour(juce::Colour(0xff282830));
            g.fillRoundedRectangle((float)bx, (float)bby, (float)bw, (float)bh, 1.5f);
            g.setColour(dotCol.withAlpha(0.75f));
            g.fillRoundedRectangle((float)bx, (float)bby, (float)(bw * ratio), (float)bh, 1.5f);
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

    void closeButtonPressed() override { setVisible(false); }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LiveLoopWindow)
};
