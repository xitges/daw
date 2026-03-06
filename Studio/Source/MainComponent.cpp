#include "MainComponent.h"

MainComponent::MainComponent()
{
    // 기본 프로젝트 세팅 (Pattern 1 + 데모 플레이리스트)
    project.bpm = 140.0;

    Pattern pattern1;
    pattern1.id   = 1;
    pattern1.name = "Pattern 1";
    pattern1.lengthBars = 1;
    project.patterns.push_back(pattern1);

    // Playlist 데모 클립들 (UI 초기값과 동일하게 맞춤)
    {
        PlaylistClip c1;
        c1.id         = 1;
        c1.patternId  = 1;
        c1.name       = "Intro Beat";
        c1.trackIndex = 0;
        c1.startBar   = 0;
        c1.lengthBars = 4;
        project.playlistClips.push_back(c1);

        PlaylistClip c2;
        c2.id         = 2;
        c2.patternId  = 1;
        c2.name       = "Main Beat";
        c2.trackIndex = 0;
        c2.startBar   = 4;
        c2.lengthBars = 8;
        project.playlistClips.push_back(c2);

        PlaylistClip c3;
        c3.id         = 3;
        c3.patternId  = 1;
        c3.name       = "Break";
        c3.trackIndex = 1;
        c3.startBar   = 8;
        c3.lengthBars = 4;
        project.playlistClips.push_back(c3);

        PlaylistClip c4;
        c4.id         = 4;
        c4.patternId  = 1;
        c4.name       = "Fill";
        c4.trackIndex = 2;
        c4.startBar   = 12;
        c4.lengthBars = 2;
        project.playlistClips.push_back(c4);
    }

    // Audio / UI 컴포넌트 초기화
    audioEngine.initialise();
    audioEngine.setProject(&project);

    addAndMakeVisible(toolbar);
    addAndMakeVisible(playlist);
    addAndMakeVisible(channelRack);

    playlist.setProject(&project);

    // 재생 스텝 콜백
    channelRack.getCurrentStep = [this]()
    {
        return audioEngine.getSequencer().getCurrentStep();
    };

    // 샘플 드롭 콜백
    channelRack.onSampleDropped = [this](int ch, juce::File file)
    {
        audioEngine.loadSample(ch, file);
        DBG("Sample loaded on ch " << ch << ": " << file.getFileName());
    };

    // Play 콜백
    toolbar.onPlay = [this]
    {
        // ChannelRack 현재 상태를 Pattern 1에 저장
        if (!project.patterns.empty())
        {
            auto& pat = project.patterns[0];
            int stepCount = juce::jmin(channelRack.getStepCount(), Pattern::kMaxSteps);
            pat.stepCount = stepCount;

            for (int ch = 0; ch < Pattern::kMaxChannels; ++ch)
            {
                for (int s = 0; s < stepCount; ++s)
                {
                    bool on = channelRack.getStep(ch, s);
                    pat.steps[ch][s] = on;

                    // Pattern 모드에서는 Sequencer 패턴에도 즉시 반영
                    if (toolbar.getPlayMode() == PlayMode::Pattern)
                        audioEngine.setStepPattern(ch, s, on);
                }
            }

            if (toolbar.getPlayMode() == PlayMode::Pattern)
                audioEngine.setPatternStepCount(stepCount);
        }

        project.bpm = toolbar.getBPM();
        audioEngine.setBPM(project.bpm);

        audioEngine.setPlayMode(toolbar.getPlayMode());
        audioEngine.play();
    };

    // Stop 콜백
    toolbar.onStop = [this]
    {
        audioEngine.stop();
        channelRack.setPlaybackStep(-1);  // 재생 헤드 초기화
    };

    // BPM 변경 콜백
    toolbar.onBPMChanged = [this](double bpm)
    {
        audioEngine.setBPM(bpm);
    };

    // 재생 모드 변경 콜백 (현재는 UI 용도로만 사용)
    toolbar.onPlayModeChanged = [this](PlayMode mode)
    {
        audioEngine.setPlayMode(mode);
    };

    setSize(1280, 720);
}

MainComponent::~MainComponent()
{
    audioEngine.shutdown();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a2e));
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    toolbar.setBounds(area.removeFromTop(60));

    // 중간: Playlist (대략 40% 높이)
    auto playlistHeight = area.getHeight() * 0.4f;
    playlist.setBounds(area.removeFromTop((int)playlistHeight));

    // 하단: Channel Rack
    channelRack.setBounds(area);
}
