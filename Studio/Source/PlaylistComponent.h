/*
  ==============================================================================

    PlaylistComponent.h
    Created: 6 Mar 2026
    Author:  홍준영

    간단한 FL 스타일 플레이리스트 UI (타임라인 + 패턴 클립)
    - 아직 오디오 재생 로직과는 연결되지 않은 순수 UI 레벨 MVP

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "ProjectModel.h"

class PlaylistComponent : public juce::Component
{
public:
    PlaylistComponent();
    ~PlaylistComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

    // 총 곡 길이(바)를 반환 (마지막 클립 기준)
    double getTotalBars() const;

    // 외부 Project 모델을 연결 (nullptr 이면 내부 더미 클립 사용)
    void setProject(Project* projectToUse);

private:
    Project* project = nullptr;

    // project 가 없을 때 사용하는 데모용 로컬 클립
    std::vector<PlaylistClip> localDemoClips;

    static constexpr int headerHeight   = 24;
    static constexpr int trackHeight    = 40;
    static constexpr int trackGap       = 4;
    static constexpr int trackCount     = 4;
    static constexpr int barWidth       = 64;  // 한 바당 픽셀 수

    int  draggingClipId   = -1;
    int  dragStartBar     = 0;
    int  dragStartMouseX  = 0;

    void drawBackground(juce::Graphics& g);
    void drawTimeRuler(juce::Graphics& g);
    void drawTracks(juce::Graphics& g);
    void drawClips(juce::Graphics& g);

    PlaylistClip* findClipAt(int x, int y);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaylistComponent)
};

//==============================================================================
// Inline implementations to avoid linker issues if .cpp is not linked

inline PlaylistComponent::PlaylistComponent()
{
    // 데모용 더미 클립들 (Project 가 연결되지 않았을 때 사용)
    PlaylistClip c1;
    c1.id         = 1;
    c1.patternId  = 1;
    c1.name       = "Intro Beat";
    c1.trackIndex = 0;
    c1.startBar   = 0;
    c1.lengthBars = 4;
    localDemoClips.push_back(c1);

    PlaylistClip c2;
    c2.id         = 2;
    c2.patternId  = 1;
    c2.name       = "Main Beat";
    c2.trackIndex = 0;
    c2.startBar   = 4;
    c2.lengthBars = 8;
    localDemoClips.push_back(c2);

    PlaylistClip c3;
    c3.id         = 3;
    c3.patternId  = 1;
    c3.name       = "Break";
    c3.trackIndex = 1;
    c3.startBar   = 8;
    c3.lengthBars = 4;
    localDemoClips.push_back(c3);

    PlaylistClip c4;
    c4.id         = 4;
    c4.patternId  = 1;
    c4.name       = "Fill";
    c4.trackIndex = 2;
    c4.startBar   = 12;
    c4.lengthBars = 2;
    localDemoClips.push_back(c4);
}

inline void PlaylistComponent::paint(juce::Graphics& g)
{
    drawBackground(g);
    drawTimeRuler(g);
    drawTracks(g);
    drawClips(g);
}

inline void PlaylistComponent::resized()
{
}

inline double PlaylistComponent::getTotalBars() const
{
    int maxBar = 0;

    const auto& list = (project != nullptr) ? project->playlistClips
                                            : localDemoClips;

    for (const auto& clip : list)
        maxBar = juce::jmax(maxBar, clip.startBar + clip.lengthBars);

    return (double)maxBar;
}

inline void PlaylistComponent::setProject(Project* projectToUse)
{
    project = projectToUse;
    repaint();
}

inline void PlaylistComponent::drawBackground(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff11111f));

    // 상단과 트랙 영역을 살짝 나눠주기
    g.setColour(juce::Colour(0xff0f3460));
    g.fillRect(0, 0, getWidth(), headerHeight);
}

inline void PlaylistComponent::drawTimeRuler(juce::Graphics& g)
{
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));

    const int totalBarsToDraw = juce::jmax((getWidth() / barWidth) + 1, 16);

    for (int bar = 0; bar < totalBarsToDraw; ++bar)
    {
        int x = bar * barWidth;

        // 바 구분선
        g.setColour(bar % 4 == 0
                        ? juce::Colour(0xff3498db).withAlpha(0.4f)
                        : juce::Colour(0xff2c3e50).withAlpha(0.6f));
        g.drawLine((float)x, 0.0f, (float)x, (float)getHeight(), 1.0f);

        // 상단 룰러 텍스트
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        juce::String label = juce::String(bar + 1);
        g.drawText(label, x + 2, 0, barWidth - 4, headerHeight,
                   juce::Justification::centredLeft);
    }
}

inline void PlaylistComponent::drawTracks(juce::Graphics& g)
{
    int y = headerHeight;

    for (int track = 0; track < trackCount; ++track)
    {
        auto trackY = y + track * (trackHeight + trackGap);

        // 트랙 배경 (줄무늬 느낌)
        juce::Colour base = (track % 2 == 0)
                                ? juce::Colour(0xff1b1b30)
                                : juce::Colour(0xff151528);
        g.setColour(base);
        g.fillRect(0, trackY, getWidth(), trackHeight);

        // 트랙 이름 자리
        g.setColour(juce::Colour(0xff0f3460));
        g.fillRect(0, trackY, 80, trackHeight);

        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));
        juce::String name = "Track " + juce::String(track + 1);
        g.drawText(name, 8, trackY, 72, trackHeight,
                   juce::Justification::centredLeft);
    }
}

inline void PlaylistComponent::drawClips(juce::Graphics& g)
{
    const auto& list = (project != nullptr) ? project->playlistClips
                                            : localDemoClips;

    for (const auto& clip : list)
    {
        if (clip.trackIndex < 0 || clip.trackIndex >= trackCount)
            continue;

        const int x  = clip.startBar * barWidth;
        const int w  = clip.lengthBars * barWidth - 2;
        const int ty = headerHeight
            + clip.trackIndex * (trackHeight + trackGap)
            + 3;
        const int h  = trackHeight - 6;

        // 클립 본체
        juce::Colour fill = juce::Colour(0xff27ae60);
        juce::Colour border = juce::Colour(0xff2ecc71);

        g.setColour(fill.withAlpha(0.85f));
        g.fillRoundedRectangle((float)x + 1.0f, (float)ty, (float)w, (float)h, 4.0f);

        g.setColour(border);
        g.drawRoundedRectangle((float)x + 1.0f, (float)ty, (float)w, (float)h, 4.0f, 1.3f);

        // 클립 이름
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
        g.drawText(clip.name, x + 6, ty, w - 12, h,
                   juce::Justification::centredLeft);
    }
}

inline PlaylistClip* PlaylistComponent::findClipAt(int x, int y)
{
    auto& list = (project != nullptr) ? project->playlistClips
                                      : localDemoClips;

    for (auto& clip : list)
    {
        if (clip.trackIndex < 0 || clip.trackIndex >= trackCount)
            continue;

        const int clipX  = clip.startBar * barWidth;
        const int clipW  = clip.lengthBars * barWidth - 2;
        const int clipY  = headerHeight
            + clip.trackIndex * (trackHeight + trackGap)
            + 3;
        const int clipH  = trackHeight - 6;

        juce::Rectangle<int> bounds(clipX, clipY, clipW, clipH);
        if (bounds.contains(x, y))
            return &clip;
    }

    return nullptr;
}

inline void PlaylistComponent::mouseDown(const juce::MouseEvent& e)
{
    auto pos = e.getPosition();
    if (pos.y < headerHeight)
        return;

    PlaylistClip* clip = findClipAt(pos.x, pos.y);
    if (clip != nullptr)
    {
        draggingClipId  = clip->id;
        dragStartBar    = clip->startBar;
        dragStartMouseX = pos.x;
    }
    else
    {
        draggingClipId = -1;
    }
}

inline void PlaylistComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (draggingClipId < 0)
        return;

    auto pos = e.getPosition();
    int deltaX   = pos.x - dragStartMouseX;
    int deltaBar = (int)std::round((double)deltaX / (double)barWidth);

    auto& list = (project != nullptr) ? project->playlistClips
                                      : localDemoClips;

    for (auto& clip : list)
    {
        if (clip.id == draggingClipId)
        {
            clip.startBar = juce::jmax(0, dragStartBar + deltaBar);
            break;
        }
    }

    repaint();
}

inline void PlaylistComponent::mouseUp(const juce::MouseEvent&)
{
    draggingClipId = -1;
}

inline void PlaylistComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    auto pos = e.getPosition();
    if (pos.y < headerHeight)
        return;

    int trackYArea = pos.y - headerHeight;
    int track      = trackYArea / (trackHeight + trackGap);

    if (track < 0 || track >= trackCount)
        return;

    int bar = pos.x / barWidth;
    if (bar < 0)
        bar = 0;

    auto& list = (project != nullptr) ? project->playlistClips
                                      : localDemoClips;

    int newId = 1;
    for (const auto& c : list)
        newId = juce::jmax(newId, c.id + 1);

    PlaylistClip clip;
    clip.id         = newId;
    clip.patternId  = 1;
    clip.trackIndex = track;
    clip.startBar   = bar;
    clip.lengthBars = 4;
    clip.name       = "Clip " + juce::String(newId);

    list.push_back(clip);
    repaint();
}

