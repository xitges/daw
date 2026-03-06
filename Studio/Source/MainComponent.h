#pragma once
#include <JuceHeader.h>
#include "ToolbarComponent.h"
#include "ChannelRackComponent.h"
#include "Audio/AudioEngine.h"

class MainComponent : public juce::Component
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    AudioEngine        audioEngine;
    ToolbarComponent   toolbar;
    ChannelRackComponent channelRack;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
