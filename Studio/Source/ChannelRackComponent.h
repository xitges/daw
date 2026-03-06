#pragma once
#include <JuceHeader.h>
#include <functional>

struct ChannelRow
{
    juce::String name;
    std::array<bool, 16> steps {};
    bool muted  = false;
    bool soloed = false;
    juce::String sampleName = "Drop Sample";
};

class ChannelRackComponent : public juce::Component,
                             public juce::FileDragAndDropTarget,
                             public juce::Timer
{
public:
    ChannelRackComponent();
    ~ChannelRackComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void timerCallback() override;

    // 드래그앤드롭
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void fileDragEnter(const juce::StringArray&, int, int) override;
    void fileDragExit(const juce::StringArray&) override;
    void fileDragMove(const juce::StringArray&, int, int y) override;

    bool getStep(int channel, int step) const
    {
        if (channel < (int)channels.size())
            return channels[channel].steps[step];
        return false;
    }

    void setPlaybackStep(int step)
    {
        if (currentPlayStep != step)
        {
            currentPlayStep = step;
            repaint();
        }
    }

    std::function<void(int channelIndex, juce::File file)> onSampleDropped;
    std::function<int()> getCurrentStep;

private:
    std::vector<ChannelRow> channels;
    juce::TextButton addChannelBtn { "+ Add Channel" };
    int dragHoverChannel = -1;
    int currentPlayStep  = -1;

    static constexpr int ROW_HEIGHT    = 40;
    static constexpr int LABEL_WIDTH   = 160;
    static constexpr int STEP_COUNT    = 16;
    static constexpr int HEADER_HEIGHT = 30;

    void addChannel(const juce::String& name);
    void drawStepGrid(juce::Graphics& g);
    void drawChannelLabels(juce::Graphics& g);
    void drawHeader(juce::Graphics& g);
    int  getChannelIndexAt(int y) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelRackComponent)
};
