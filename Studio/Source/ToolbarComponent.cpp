/*
  ==============================================================================

    ToolbarComponent.cpp
    Created: 6 Mar 2026 11:31:42am
    Author:  홍준영

  ==============================================================================
*/

#include "ToolbarComponent.h"

ToolbarComponent::ToolbarComponent()
{
    // 재생 버튼
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(recordButton);
    playButton.addListener(this);
    stopButton.addListener(this);
    recordButton.addListener(this);

    // 버튼 색상
    playButton.setColour(juce::TextButton::buttonColourId,
                         juce::Colour(0xff2ecc71));
    stopButton.setColour(juce::TextButton::buttonColourId,
                         juce::Colour(0xffe74c3c));
    recordButton.setColour(juce::TextButton::buttonColourId,
                           juce::Colour(0xffe74c3c));

    // BPM 슬라이더
    addAndMakeVisible(bpmSlider);
    bpmSlider.setRange(60.0, 200.0, 1.0);
    bpmSlider.setValue(140.0);
    bpmSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    bpmSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
    bpmSlider.addListener(this);
    bpmSlider.setColour(juce::Slider::rotarySliderFillColourId,
                        juce::Colour(0xff3498db));

    // BPM 라벨
    addAndMakeVisible(bpmLabel);
    bpmLabel.setText("BPM", juce::dontSendNotification);
    bpmLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    bpmLabel.setJustificationType(juce::Justification::centred);

    // 타이틀
    addAndMakeVisible(titleLabel);
    titleLabel.setText("Studio  MVP", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions().withHeight(18.0f).withStyle("Bold")));
    titleLabel.setColour(juce::Label::textColourId,
                         juce::Colour(0xffecf0f1));
    titleLabel.setJustificationType(juce::Justification::centredRight);
}

ToolbarComponent::~ToolbarComponent() {}

void ToolbarComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff16213e));

    // 하단 구분선
    g.setColour(juce::Colour(0xff0f3460));
    g.drawLine(0, getHeight() - 1, getWidth(), getHeight() - 1, 2.0f);
}

void ToolbarComponent::resized()
{
    auto area = getLocalBounds().reduced(8);

    // 왼쪽: 재생 버튼들
    playButton.setBounds  (area.removeFromLeft(44).reduced(4));
    stopButton.setBounds  (area.removeFromLeft(44).reduced(4));
    recordButton.setBounds(area.removeFromLeft(44).reduced(4));

    area.removeFromLeft(16); // 여백

    // BPM
    bpmLabel.setBounds (area.removeFromLeft(40).reduced(2));
    bpmSlider.setBounds(area.removeFromLeft(60).reduced(2));

    // 오른쪽: 타이틀
    titleLabel.setBounds(area);
}

void ToolbarComponent::buttonClicked(juce::Button* btn)
{
    if (btn == &playButton)
    {
        playing = true;
        if (onPlay) onPlay();
    }
    else if (btn == &stopButton)
    {
        playing = false;
        if (onStop) onStop();
    }
}

void ToolbarComponent::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &bpmSlider)
        if (onBPMChanged) onBPMChanged(bpmSlider.getValue());
}
