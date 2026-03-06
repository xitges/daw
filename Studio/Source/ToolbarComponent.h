/*
  ==============================================================================

    ToolbarComponent.h
    Created: 6 Mar 2026 11:31:51am
    Author:  홍준영

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class ToolbarComponent : public juce::Component,
                         public juce::Button::Listener,
                         public juce::Slider::Listener
{
public:
    ToolbarComponent();
    ~ToolbarComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void buttonClicked(juce::Button*) override;
    void sliderValueChanged(juce::Slider*) override;

    bool isPlaying() const { return playing; }
    
    std::function<void()>       onPlay;
    std::function<void()>       onStop;
    std::function<void(double)> onBPMChanged;
    double getBPM() const { return bpmSlider.getValue(); }

private:
    juce::TextButton playButton   { "Play" };
    juce::TextButton stopButton   { "Stop" };
    juce::TextButton recordButton { "Rec" };

    juce::Slider bpmSlider;
    juce::Label  bpmLabel;
    juce::Label  titleLabel;

    bool playing = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToolbarComponent)
};
