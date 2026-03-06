#include "ChannelRackComponent.h"

ChannelRackComponent::ChannelRackComponent()
{
    addChannel("Kick");
    addChannel("Snare");
    addChannel("HiHat");

    channels[0].steps[0]  = true;
    channels[0].steps[4]  = true;
    channels[0].steps[8]  = true;
    channels[0].steps[12] = true;

    channels[1].steps[4]  = true;
    channels[1].steps[12] = true;

    for (int i = 0; i < 16; ++i)
        channels[2].steps[i] = true;

    addAndMakeVisible(addChannelBtn);
    addChannelBtn.setColour(juce::TextButton::buttonColourId,
                            juce::Colour(0xff0f3460));
    addChannelBtn.setColour(juce::TextButton::textColourOnId,
                            juce::Colours::white);
    addChannelBtn.onClick = [this]
    {
        addChannel("Channel " + juce::String(channels.size() + 1));
        resized();
        repaint();
    };

    // 스텝 개수 선택 콤보박스 (16 / 32 / 64)
    addAndMakeVisible(stepCountBox);
    stepCountBox.addItem("16 Steps", 16);
    stepCountBox.addItem("32 Steps", 32);
    stepCountBox.addItem("64 Steps", 64);
    stepCountBox.setSelectedId(16, juce::dontSendNotification);
    stepCountBox.onChange = [this]
    {
        int newCount = stepCountBox.getSelectedId();
        newCount = juce::jlimit(1, Pattern::kMaxSteps, newCount);
        stepCount = newCount;
        repaint();
    };
    stepCount = 16;

    startTimerHz(30);
}

ChannelRackComponent::~ChannelRackComponent()
{
    stopTimer();
}

void ChannelRackComponent::addChannel(const juce::String& name)
{
    ChannelRow row;
    row.name = name;
    channels.push_back(row);
}

void ChannelRackComponent::timerCallback()
{
    if (getCurrentStep)
        setPlaybackStep(getCurrentStep());
}

void ChannelRackComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a2e));
    drawHeader(g);
    drawChannelLabels(g);
    drawStepGrid(g);
}

void ChannelRackComponent::drawHeader(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xff16213e));
    g.fillRect(0, 0, getWidth(), HEADER_HEIGHT);

    float stepW = (getWidth() - LABEL_WIDTH) / (float)stepCount;

    for (int i = 0; i < stepCount; ++i)
    {
        int x = LABEL_WIDTH + (int)(i * stepW);

        if (i == currentPlayStep)
        {
            g.setColour(juce::Colour(0xff3498db).withAlpha(0.4f));
            g.fillRect(x, 0, (int)stepW, HEADER_HEIGHT);
        }

        if (i % 4 == 0)
            g.setColour(juce::Colours::white);
        else
            g.setColour(juce::Colours::grey);

        g.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));
        g.drawText(juce::String(i + 1), x, 0, (int)stepW, HEADER_HEIGHT,
                   juce::Justification::centred);
    }
}

void ChannelRackComponent::drawChannelLabels(juce::Graphics& g)
{
    for (int i = 0; i < (int)channels.size(); ++i)
    {
        int y = HEADER_HEIGHT + i * ROW_HEIGHT;

        if (i == dragHoverChannel)
            g.setColour(juce::Colour(0xff2ecc71).withAlpha(0.3f));
        else
            g.setColour(i % 2 == 0 ? juce::Colour(0xff1e1e3a)
                                   : juce::Colour(0xff16213e));
        g.fillRect(0, y, LABEL_WIDTH, ROW_HEIGHT);

        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(juce::FontOptions().withHeight(13.0f)));
        g.drawText(channels[i].name, 10, y, LABEL_WIDTH - 20, ROW_HEIGHT / 2,
                   juce::Justification::centredLeft);

        g.setColour(juce::Colour(0xff3498db));
        g.setFont(juce::Font(juce::FontOptions().withHeight(10.0f)));
        g.drawText(channels[i].sampleName, 10, y + ROW_HEIGHT / 2,
                   LABEL_WIDTH - 20, ROW_HEIGHT / 2,
                   juce::Justification::centredLeft);

        g.setColour(juce::Colour(0xff0f3460));
        g.drawLine(0, y + ROW_HEIGHT, LABEL_WIDTH, y + ROW_HEIGHT, 1.0f);
    }
}

void ChannelRackComponent::drawStepGrid(juce::Graphics& g)
{
    int stepAreaX = LABEL_WIDTH;
    float stepW   = (getWidth() - stepAreaX) / (float)stepCount;

    for (int ch = 0; ch < (int)channels.size(); ++ch)
    {
        int y = HEADER_HEIGHT + ch * ROW_HEIGHT;

        for (int s = 0; s < stepCount; ++s)
        {
            int x = stepAreaX + (int)(s * stepW);
            int w = (int)stepW - 2;
            int h = ROW_HEIGHT - 4;

            bool isCurrentStep = (s == currentPlayStep);

            if (channels[ch].steps[s])
            {
                juce::Colour c = isCurrentStep
                    ? juce::Colour(0xffffffff)
                    : (s % 4 == 0 ? juce::Colour(0xff3498db)
                                  : juce::Colour(0xff2980b9));
                g.setColour(c);
                g.fillRoundedRectangle(x + 1, y + 2, w, h, 4.0f);
            }
            else
            {
                juce::Colour c = isCurrentStep
                    ? juce::Colour(0xff555588)
                    : juce::Colour(0xff2c2c54);
                g.setColour(c);
                g.fillRoundedRectangle(x + 1, y + 2, w, h, 4.0f);

                if (!isCurrentStep)
                {
                    g.setColour(juce::Colour(0xff3d3d6b));
                    g.drawRoundedRectangle(x + 1, y + 2, w, h, 4.0f, 1.0f);
                }
            }
        }
    }
}

void ChannelRackComponent::mouseDown(const juce::MouseEvent& e)
{
    int stepAreaX = LABEL_WIDTH;
    float stepW   = (getWidth() - stepAreaX) / (float)stepCount;

    int x = e.getPosition().getX();
    int y = e.getPosition().getY() - HEADER_HEIGHT;

    if (x < stepAreaX || y < 0) return;

    int ch = y / ROW_HEIGHT;
    int s  = (int)((x - stepAreaX) / stepW);

    if (ch >= 0 && ch < (int)channels.size() &&
        s  >= 0 && s  < stepCount)
    {
        channels[ch].steps[s] = !channels[ch].steps[s];
        repaint();
    }
}

void ChannelRackComponent::resized()
{
    int totalRows = (int)channels.size();
    int contentH  = HEADER_HEIGHT + totalRows * ROW_HEIGHT + 50;
    setSize(getWidth(), juce::jmax(contentH, getParentHeight()));

    int controlsY = HEADER_HEIGHT + totalRows * ROW_HEIGHT + 8;

    addChannelBtn.setBounds(10,
                            controlsY,
                            140, 30);

    stepCountBox.setBounds(160,
                           controlsY,
                           120, 30);
}

int ChannelRackComponent::getChannelIndexAt(int y) const
{
    int relY = y - HEADER_HEIGHT;
    if (relY < 0) return -1;
    int ch = relY / ROW_HEIGHT;
    if (ch >= (int)channels.size()) return -1;
    return ch;
}

bool ChannelRackComponent::isInterestedInFileDrag(const juce::StringArray&)
{
    return true;
}

void ChannelRackComponent::fileDragEnter(const juce::StringArray&, int, int y)
{
    dragHoverChannel = getChannelIndexAt(y);
    repaint();
}

void ChannelRackComponent::fileDragMove(const juce::StringArray&, int, int y)
{
    int newHover = getChannelIndexAt(y);
    if (newHover != dragHoverChannel)
    {
        dragHoverChannel = newHover;
        repaint();
    }
}

void ChannelRackComponent::fileDragExit(const juce::StringArray&)
{
    dragHoverChannel = -1;
    repaint();
}

void ChannelRackComponent::filesDropped(const juce::StringArray& files, int, int y)
{
    dragHoverChannel = -1;

    int ch = getChannelIndexAt(y);
    if (ch < 0) return;

    juce::File file(files[0]);

    channels[ch].sampleName = file.getFileNameWithoutExtension();
    repaint();

    if (onSampleDropped)
        onSampleDropped(ch, file);
}
