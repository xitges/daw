#pragma once
#include <JuceHeader.h>
#include "ProjectModel.h"
#include "ToolbarComponent.h"
#include "ChannelRackComponent.h"
#include "Audio/AudioEngine.h"
#include "PlaylistComponent.h"

class MainComponent : public juce::Component
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    Project            project;
    AudioEngine        audioEngine;
    ToolbarComponent   toolbar;
    PlaylistComponent  playlist;
    ChannelRackComponent channelRack;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
