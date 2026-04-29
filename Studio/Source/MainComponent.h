#pragma once
#include <JuceHeader.h>
#include "ProjectModel.h"
#include "ProjectSerializer.h"
#include "StudioLookAndFeel.h"
#include "ToolbarComponent.h"
#include "ChannelRackComponent.h"
#include "Audio/AudioEngine.h"
#include "PlaylistComponent.h"
#include "PianoRollComponent.h"
#include "MixerComponent.h"
#include "SynthEditorComponent.h"
#include "FXEditorComponent.h"
#include "AutoTuneEditorComponent.h"
#include "LaunchpadComponent.h"
#include "SampleBrowserComponent.h"
#include "PluginBrowserComponent.h"
#include "DynamicEQComponent.h"
#include "TrackpadController.h"
#include "LivePerformance/ClipLauncher.h"
#include "LivePerformance/LivePerformanceComponent.h"
#include "LivePerformance/LiveLoopWindow.h"

// Tab bar: INSTRUMENT / SEQUENCER / MIXER  (Phase-6 redesign)
class InspectorTabBar : public juce::Component
{
public:
    std::function<void(int)> onTabChanged;

    void setTab(int t) { activeTab_ = t; repaint(); }
    int  getTab() const { return activeTab_; }

    // Dynamic sub-label setters — call from MainComponent when state changes
    void setInstrumentSub(int chIdx, const juce::String& name)
    {
        instrSub_ = "CH " + juce::String(chIdx + 1).paddedLeft('0', 2)
                  + juce::String::fromUTF8("  \xe2\x80\x94  ") + name.toUpperCase();
        gutterSub_ = instrSub_;
        repaint();
    }
    void setSequencerSub(const juce::String& patternName)
    {
        seqSub_ = patternName.toUpperCase();
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        using LF = StudioLookAndFeel;
        const int W = getWidth();
        const int H = getHeight();

        // Chassis gradient background
        juce::ColourGradient bg(juce::Colour(LF::kChassis2), 0.0f, 0.0f,
                                juce::Colour(LF::kChassis),  0.0f, (float)H, false);
        g.setGradientFill(bg);
        g.fillAll();

        // Top + bottom hairlines
        g.setColour(juce::Colour(LF::kPanelRim));
        g.drawLine(0.0f, 0.5f, (float)W, 0.5f, 1.0f);
        g.drawLine(0.0f, (float)(H - 1), (float)W, (float)(H - 1), 1.0f);

        // Reserve right gutter (160px) before tab layout
        const int gutterW = 170;
        const int tabZoneW = W - gutterW;
        const int tw = tabZoneW / 3;

        static const char* kLabels[] = { "INSTRUMENT", "SEQUENCER", "MIXER" };
        const juce::String kSubs[3] = {
            instrSub_,
            seqSub_,
            juce::String::fromUTF8("8 INSERT  \xc2\xb7  1 MASTER"),
        };

        for (int i = 0; i < 3; ++i)
        {
            const bool on = (i == activeTab_);
            juce::Rectangle<int> tab(i * tw, 0, tw, H);

            if (on)
            {
                // Active tab: panel fill + 2px accent top border
                g.setColour(juce::Colour(LF::kPanel));
                g.fillRect(tab.reduced(1, 0));
                // Left/right rim lines
                g.setColour(juce::Colour(LF::kPanelRim));
                g.drawLine((float)(tab.getX() + 1), 0.0f, (float)(tab.getX() + 1), (float)H, 0.8f);
                g.drawLine((float)(tab.getRight() - 1), 0.0f, (float)(tab.getRight() - 1), (float)H, 0.8f);
                // 2px accent top
                g.setColour(juce::Colour(LF::kAccent));
                g.fillRect(tab.getX() + 2, 0, tab.getWidth() - 4, 2);
            }

            // Label (INSTRUMENT / SEQUENCER / MIXER)
            g.setFont(LF::monoFont(9.5f, juce::Font::bold));
            g.setColour(on ? juce::Colour(LF::kText) : juce::Colour(LF::kTextDim));
            g.drawText(kLabels[i], tab.getX(), 7, tab.getWidth(), 14, juce::Justification::centred);

            // Sub-label
            g.setFont(LF::monoFont(7.5f, juce::Font::bold));
            g.setColour(on ? juce::Colour(LF::kAccent) : juce::Colour(LF::kTextFaint));
            juce::String sub = kSubs[i];
            g.drawText(sub, tab.getX() + 2, 22, tab.getWidth() - 4, 10,
                       juce::Justification::centred, true);
        }

        // Right gutter: "INSPECTOR" tag + track tag
        {
            const int gx = tabZoneW + 8;
            const int gy = 5;
            const int gh = H - 10;

            // "INSPECTOR" pill
            const int inspW = 62;
            juce::Rectangle<float> inspR((float)gx, (float)gy, (float)inspW, (float)gh);
            g.setColour(juce::Colour(LF::kChassis));
            g.fillRoundedRectangle(inspR, 3.0f);
            g.setColour(juce::Colour(LF::kPanelRim));
            g.drawRoundedRectangle(inspR.reduced(0.5f), 3.0f, 0.8f);
            g.setFont(LF::monoFont(7.5f, juce::Font::bold));
            g.setColour(juce::Colour(LF::kTextFaint));
            g.drawText("INSPECTOR", inspR.toNearestInt(), juce::Justification::centred);

            // Track info pill
            const int trackX = gx + inspW + 6;
            const int trackW = gutterW - inspW - 22;
            juce::Rectangle<float> trackR((float)trackX, (float)gy, (float)trackW, (float)gh);
            g.setColour(juce::Colour(LF::kChassis));
            g.fillRoundedRectangle(trackR, 3.0f);
            g.setColour(juce::Colour(LF::kPanelRim));
            g.drawRoundedRectangle(trackR.reduced(0.5f), 3.0f, 0.8f);
            g.setFont(LF::monoFont(7.5f, juce::Font::bold));
            g.setColour(juce::Colour(LF::kAccent).withAlpha(0.85f));
            g.drawText(gutterSub_, trackR.toNearestInt(), juce::Justification::centred, true);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        const int gutterW  = 170;
        const int tabZoneW = getWidth() - gutterW;
        if (e.x >= tabZoneW) return;   // click in gutter — ignore
        const int tw = tabZoneW / 3;
        const int t  = juce::jlimit(0, 2, e.x / tw);
        if (t != activeTab_)
        {
            activeTab_ = t;
            repaint();
            if (onTabChanged) onTabChanged(activeTab_);
        }
    }

private:
    int          activeTab_ = 1;   // default: SEQUENCER
    juce::String instrSub_  = "CH 01";
    juce::String seqSub_    = "PATTERN";
    juce::String gutterSub_ = "CH 01";
};

class MainComponent : public juce::Component,
                      public juce::Timer,
                      public juce::DragAndDropContainer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    StudioLookAndFeel    lookAndFeel;    // M11 — must be first so it outlives all components

    Project              project;
    AudioEngine          audioEngine;
    ToolbarComponent     toolbar;
    PlaylistComponent    playlist;
    juce::Viewport       playlistViewport;
    juce::ComboBox       playlistSnapBox;
    juce::TextButton     playlistZoomInBtn  { "+" };
    juce::TextButton     playlistZoomOutBtn { "-" };
    ChannelRackComponent channelRack;
    juce::Viewport       channelRackViewport;
    MixerComponent       mixer;

    // M15 — sample browser panel
    SampleBrowserComponent sampleBrowser;
    juce::Viewport         browserViewport;
    juce::TextButton       browserCollapseBtn;   // always-visible collapse/expand tab
    bool isBrowserOpen = true;                   // default: open on launch

    // M3 — floating piano roll window
    std::unique_ptr<PianoRollWindow> pianoRollWindow;
    int  pianoRollChannel = -1;
    bool showMixer = false;
    bool pianoRollPlaybackOverridesPlayMode = false;
    PlayMode playModeBeforePianoRollPlayback = PlayMode::Pattern;

    // M13/M14 — floating synth + FX editors
    std::unique_ptr<SynthEditorWindow> synthEditorWindow;
    std::unique_ptr<FXEditorWindow>    fxEditorWindow;
    std::unique_ptr<AutoTuneEditorWindow> autoTuneEditorWindow;
    int autoTuneEditorTrack = -1;
    int synthEditorChannel = -1;
    int fxEditorTrack      = -1;

    // Inspector tab bar (INSTRUMENT=0 / SEQUENCER=1 / MIXER=2)
    InspectorTabBar  inspectorTabBar_;
    int              inspectorTab_ = 1;   // default: SEQUENCER (channel rack)
    juce::Rectangle<int> rightPanelBounds_;        // cached in resized(), used in paint()
    juce::Rectangle<int> inspectorContentBounds_;  // area below tab bar (inspector content)

    // Launchpad — inline right panel (toggled)
    LaunchpadPanel   launchpadPanel;
    bool             showLaunchpad = false;

    std::unique_ptr<juce::DocumentWindow> audioDeviceWindow;

    // M8 — plugin browser + per-channel plugin editor windows
    std::unique_ptr<PluginBrowserWindow>                    pluginBrowserWindow;
    std::array<std::unique_ptr<PluginEditorWindow>, 16>     pluginEditorWindows;

    // Dynamic EQ windows (0-7 = mixer tracks, 8 = master)
    std::array<std::unique_ptr<DynamicEQWindow>, 9>         dynEQWindows;

    // Trackpad multitouch → launchpad pads 0-15
    TrackpadController trackpadController;

    // M2 — pattern management state
    int activePatternId = 1;

    // Pause/resume — saved bar position when Space-paused in Song mode (< 0 = no pause state)
    double pausedBarSong = -1.0;
    int patternStartStep = 0;
    int pianoRollStartStep = 0;

    // Double-Space detection: timestamp when Space last resumed playback
    juce::int64 lastSpaceResumeTime = 0;

    Pattern* findPattern(int id);
    int      nextPatternId() const;
    void     selectPattern(int id);
    void     syncPatternToEngine();
    void     syncChannelRackToProject();
    int      ensureAutoBassChannel();
    void     focusPianoRollChannel(int channel);
    void     switchToPatternModeForEditing();
    void     followPlaylistPlayhead();
    void     beginPianoRollPatternPlayback();
    void     restorePlayModeAfterPianoRollPlayback();
    void     exportCurrentPianoRollToMidi();
    void     importCurrentPianoRollFromMidi();
    void     refreshSynthEditorPresetList(const juce::String& selectPresetName = {});

    // M6 — undo/redo
    juce::UndoManager undoManager;

    // M4 — file state
    juce::File currentFile;
    bool       projectDirty = false;

    // Rebuilds all UI from the current project (used after load / new)
    void reloadProjectIntoUI();
    void markDirty();

    // File operations
    void newProject();
    void openProject();
    void saveProject();
    void saveProjectAs();

    // Kept alive until async chooser completes
    std::shared_ptr<juce::FileChooser> fileChooser;

    bool recordTransitioning_ = false;
    bool loopRecordEnabled_   = false;   // mirrors audioEngine.isLoopRecording() for message thread
    bool liveMode_            = false;   // Live Performance Mode active state

    ClipLauncher clipLauncher_;          // Live Performance clip launch state (message thread)

    // Live Performance debug window
    std::unique_ptr<LivePerformanceWindow> liveDebugWindow_;
    std::unique_ptr<LiveLoopWindow>        liveLoopWindow_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
