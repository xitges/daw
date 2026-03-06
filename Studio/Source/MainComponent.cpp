#include "MainComponent.h"

MainComponent::MainComponent()
{
    audioEngine.initialise();

    addAndMakeVisible(toolbar);
    addAndMakeVisible(channelRack);

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
        for (int ch = 0; ch < 16; ++ch)
            for (int s = 0; s < 16; ++s)
                audioEngine.setStepPattern(ch, s, channelRack.getStep(ch, s));

        audioEngine.setBPM(toolbar.getBPM());
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
    channelRack.setBounds(area);
}
