/*
  ==============================================================================
    StudioLookAndFeel.h  —  Professional dark theme (Space Gray / Silver)
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class StudioLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // ---- Palette : ultra-dark + cool silver accent -------------------------
    static constexpr juce::uint32
        kBg       = 0xff0d0d0d,
        kSurface  = 0xff181818,
        kSurface2 = 0xff242424,
        kSurface3 = 0xff323232,
        kAccent   = 0xffb0b0b8,   // cool silver
        kAccentHi = 0xffe0e0e8,
        kText     = 0xfff0f0f2,
        kTextDim  = 0xff888892,
        kRed      = 0xffcc3333,
        kAmber    = 0xffb89010;

    static juce::Colour rim()     { return juce::Colours::white.withAlpha(0.11f); }
    static juce::Colour specular(){ return juce::Colours::white.withAlpha(0.07f); }

    StudioLookAndFeel()
    {
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(kBg));

        setColour(juce::PopupMenu::backgroundColourId,            juce::Colour(kSurface2));
        setColour(juce::PopupMenu::textColourId,                  juce::Colour(kText));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(kAccent));
        setColour(juce::PopupMenu::highlightedTextColourId,       juce::Colour(0xff000000));

        setColour(juce::AlertWindow::backgroundColourId, juce::Colour(kSurface2));
        setColour(juce::AlertWindow::textColourId,       juce::Colour(kText));
        setColour(juce::AlertWindow::outlineColourId,    rim());

        setColour(juce::TextEditor::backgroundColourId,     juce::Colour(kBg));
        setColour(juce::TextEditor::textColourId,           juce::Colour(kText));
        setColour(juce::TextEditor::outlineColourId,        rim());
        setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(kAccent));
        setColour(juce::TextEditor::highlightColourId,      juce::Colour(kAccent).withAlpha(0.2f));

        setColour(juce::Label::textColourId,       juce::Colour(kText));
        setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

        setColour(juce::ComboBox::backgroundColourId, juce::Colour(kSurface3));
        setColour(juce::ComboBox::textColourId,       juce::Colour(kText));
        setColour(juce::ComboBox::outlineColourId,    juce::Colours::transparentBlack);
        setColour(juce::ComboBox::arrowColourId,      juce::Colour(kTextDim));

        setColour(juce::TextButton::buttonColourId,   juce::Colour(kSurface3));
        setColour(juce::TextButton::buttonOnColourId, juce::Colour(kAccent));
        setColour(juce::TextButton::textColourOffId,  juce::Colour(kText));
        setColour(juce::TextButton::textColourOnId,   juce::Colour(0xff060606));

        setColour(juce::Slider::rotarySliderFillColourId,    juce::Colour(kAccent));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(kSurface3));
        setColour(juce::Slider::thumbColourId,               juce::Colour(kAccentHi));
        setColour(juce::Slider::trackColourId,               juce::Colour(kAccent));
        setColour(juce::Slider::backgroundColourId,          juce::Colour(kSurface3));
        setColour(juce::Slider::textBoxTextColourId,         juce::Colour(kTextDim));
        setColour(juce::Slider::textBoxBackgroundColourId,   juce::Colour(kBg));
        setColour(juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxHighlightColourId,    juce::Colour(kAccent).withAlpha(0.2f));

        setColour(juce::ScrollBar::thumbColourId, juce::Colours::white.withAlpha(0.22f));

        setColour(juce::ToggleButton::textColourId,         juce::Colour(kText));
        setColour(juce::ToggleButton::tickColourId,         juce::Colour(kAccent));
        setColour(juce::ToggleButton::tickDisabledColourId, juce::Colour(kSurface3));
    }

    // =========================================================================
    // Button — professional rounded-rect (not pill), inset shadow on press
    // =========================================================================
    void drawButtonBackground(juce::Graphics& g, juce::Button& btn,
                              const juce::Colour& bg,
                              bool isMouseOver, bool isButtonDown) override
    {
        const auto  b = btn.getLocalBounds().toFloat().reduced(0.5f);
        const float r = 5.0f;

        const bool isOn = btn.getToggleState();

        if (isOn)
        {
            // Active: solid silver fill
            const juce::Colour fill = isButtonDown ? bg.brighter(0.2f)
                                                   : isMouseOver ? bg.brighter(0.1f) : bg;
            g.setColour(fill);
            g.fillRoundedRectangle(b, r);
            // Inner shadow at top when on (pressed-in feel)
            g.setColour(juce::Colours::black.withAlpha(0.25f));
            g.fillRoundedRectangle(b.withHeight(6.0f), r);
        }
        else if (isButtonDown)
        {
            // Pressed: darker, inset look
            g.setColour(juce::Colour(kBg));
            g.fillRoundedRectangle(b, r);
            g.setColour(juce::Colours::black.withAlpha(0.3f));
            g.fillRoundedRectangle(b.withHeight(5.0f), r);
        }
        else
        {
            // Normal: dark surface with subtle top-to-bottom gradient
            juce::Colour base = juce::Colour(kSurface3);
            if (isMouseOver) base = base.brighter(0.14f);

            juce::ColourGradient grad(
                base.brighter(0.10f), 0.0f, b.getY(),
                base.darker(0.08f),   0.0f, b.getBottom(), false);
            g.setGradientFill(grad);
            g.fillRoundedRectangle(b, r);

            // Top specular line
            g.setColour(specular());
            g.drawLine(b.getX() + r, b.getY() + 1.0f,
                       b.getRight() - r, b.getY() + 1.0f, 1.0f);
        }

        // Outer rim
        g.setColour(isButtonDown ? juce::Colours::black.withAlpha(0.4f) : rim());
        g.drawRoundedRectangle(b, r, 1.0f);
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& btn,
                        bool, bool isButtonDown) override
    {
        const bool isOn = btn.getToggleState();
        juce::Colour col = isOn
            ? btn.findColour(juce::TextButton::textColourOnId)
            : btn.findColour(juce::TextButton::textColourOffId);

        if (isButtonDown) col = col.withAlpha(0.7f);

        g.setColour(col);
        g.setFont(juce::Font(juce::FontOptions().withHeight(11.0f).withStyle("Bold")));
        g.drawFittedText(btn.getButtonText(),
                         btn.getLocalBounds().reduced(5, 2),
                         juce::Justification::centred, 1, 0.85f);
    }

    // =========================================================================
    // Rotary — hardware encoder feel
    // =========================================================================
    void drawRotarySlider(juce::Graphics& g,
                          int x, int y, int width, int height,
                          float sliderPos,
                          float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override
    {
        const float radius = (float)juce::jmin(width, height) * 0.5f - 5.0f;
        const float cx     = (float)x + (float)width  * 0.5f;
        const float cy     = (float)y + (float)height * 0.5f;
        const float angle  = rotaryStartAngle
                             + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        const float pi     = juce::MathConstants<float>::pi;

        // Outer drop shadow
        g.setColour(juce::Colour(0x70000000));
        g.fillEllipse(cx - radius - 1.0f, cy - radius + 3.0f,
                      (radius + 1.0f) * 2.0f, (radius + 1.0f) * 2.0f);

        // Outer ring (bevel)
        {
            juce::ColourGradient ring(
                juce::Colour(0xff484850), cx - radius, cy - radius,
                juce::Colour(0xff1a1a1a), cx + radius, cy + radius, false);
            g.setGradientFill(ring);
            g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
        }

        // Inner body
        {
            const float ir = radius - 2.0f;
            juce::ColourGradient body(
                juce::Colour(0xff3a3a3e), cx - ir * 0.4f, cy - ir * 0.4f,
                juce::Colour(0xff181818), cx + ir * 0.4f, cy + ir * 0.4f, true);
            g.setGradientFill(body);
            g.fillEllipse(cx - ir, cy - ir, ir * 2.0f, ir * 2.0f);
        }

        // Top specular arc
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.drawEllipse(cx - radius + 1.5f, cy - radius + 1.5f,
                      (radius - 1.5f) * 2.0f, (radius - 1.5f) * 2.0f, 1.0f);

        // Track ring
        {
            juce::Path bgArc;
            bgArc.addCentredArc(cx, cy, radius - 5.0f, radius - 5.0f, 0.0f,
                                rotaryStartAngle, rotaryEndAngle, true);
            g.setColour(juce::Colour(kBg));
            g.strokePath(bgArc, juce::PathStrokeType(3.0f,
                juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Value arc
        {
            const juce::Colour fill = slider.findColour(juce::Slider::rotarySliderFillColourId);
            juce::Path arc;
            arc.addCentredArc(cx, cy, radius - 5.0f, radius - 5.0f, 0.0f,
                              rotaryStartAngle, angle, true);
            g.setColour(fill.withAlpha(0.15f));
            g.strokePath(arc, juce::PathStrokeType(7.0f,
                juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour(fill);
            g.strokePath(arc, juce::PathStrokeType(2.5f,
                juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Indicator needle
        {
            const float dx  = std::cos(angle - pi * 0.5f);
            const float dy  = std::sin(angle - pi * 0.5f);
            const float in  = radius * 0.2f;
            const float out = radius - 7.0f;
            g.setColour(juce::Colour(kAccentHi));
            g.drawLine(cx + dx * in, cy + dy * in,
                       cx + dx * out, cy + dy * out, 2.0f);
        }

        // Centre hub
        {
            const float hr = 5.0f;
            juce::ColourGradient hub(
                juce::Colour(0xff4a4a52), cx - hr, cy - hr,
                juce::Colour(0xff222226), cx + hr, cy + hr, true);
            g.setGradientFill(hub);
            g.fillEllipse(cx - hr, cy - hr, hr * 2.0f, hr * 2.0f);
        }
    }

    // =========================================================================
    // Linear Slider
    // =========================================================================
    void drawLinearSliderBackground(juce::Graphics& g,
                                    int x, int y, int width, int height,
                                    float sliderPos, float, float,
                                    juce::Slider::SliderStyle style,
                                    juce::Slider& slider) override
    {
        const float tt  = 3.0f;
        const bool  isV = (style == juce::Slider::LinearVertical);
        const juce::Colour fill = slider.findColour(juce::Slider::trackColourId);

        juce::Rectangle<float> track;
        if (isV)
            track = { (float)x + (float)width * 0.5f - tt * 0.5f,
                      (float)y, tt, (float)height };
        else
            track = { (float)x, (float)y + (float)height * 0.5f - tt * 0.5f,
                      (float)width, tt };

        // Inset groove
        g.setColour(juce::Colour(kBg));
        g.fillRoundedRectangle(track.expanded(0.5f), tt * 0.5f + 0.5f);
        g.setColour(juce::Colour(kSurface3));
        g.fillRoundedRectangle(track, tt * 0.5f);

        // Fill
        juce::Rectangle<float> filled;
        if (isV)
            filled = { track.getX(), sliderPos, tt, (float)(y + height) - sliderPos };
        else
            filled = { track.getX(), track.getY(), sliderPos - (float)x, tt };

        if (filled.getWidth() > 0 && filled.getHeight() > 0)
        {
            g.setColour(fill.withAlpha(0.12f));
            g.fillRoundedRectangle(filled.expanded(isV ? 2.0f : 0.0f,
                                                   isV ? 0.0f : 2.0f), tt);
            g.setColour(fill);
            g.fillRoundedRectangle(filled, tt * 0.5f);
        }
    }

    void drawLinearSliderThumb(juce::Graphics& g,
                               int x, int y, int width, int height,
                               float sliderPos, float, float,
                               juce::Slider::SliderStyle style,
                               juce::Slider&) override
    {
        const float tr = 7.0f;
        juce::Point<float> pt;
        if (style == juce::Slider::LinearHorizontal || style == juce::Slider::LinearBar)
            pt = { sliderPos, (float)y + (float)height * 0.5f };
        else
            pt = { (float)x + (float)width * 0.5f, sliderPos };

        // Shadow
        g.setColour(juce::Colour(0x55000000));
        g.fillEllipse(pt.x - tr + 1.0f, pt.y - tr + 2.5f, tr * 2.0f, tr * 2.0f);

        // Bevel ring
        juce::ColourGradient bevel(
            juce::Colour(0xff686870), pt.x - tr, pt.y - tr,
            juce::Colour(0xff2a2a2e), pt.x + tr, pt.y + tr, true);
        g.setGradientFill(bevel);
        g.fillEllipse(pt.x - tr, pt.y - tr, tr * 2.0f, tr * 2.0f);

        // Inner face
        const float ir = tr - 1.5f;
        juce::ColourGradient face(
            juce::Colour(0xffe8e8ee), pt.x - ir * 0.3f, pt.y - ir * 0.3f,
            juce::Colour(0xffb0b0b8), pt.x + ir * 0.3f, pt.y + ir * 0.3f, true);
        g.setGradientFill(face);
        g.fillEllipse(pt.x - ir, pt.y - ir, ir * 2.0f, ir * 2.0f);
    }

    // =========================================================================
    // Scrollbar
    // =========================================================================
    void drawScrollbar(juce::Graphics& g, juce::ScrollBar&,
                       int x, int y, int width, int height,
                       bool, int thumbStart, int thumbSize,
                       bool isMouseOver, bool isMouseDown) override
    {
        if (thumbSize <= 0) return;
        const bool vert = (height > width);
        juce::Rectangle<float> thumb;
        if (vert)
            thumb = { (float)x + 2.0f, (float)(y + thumbStart),
                      (float)width - 4.0f, (float)thumbSize };
        else
            thumb = { (float)(x + thumbStart), (float)y + 2.0f,
                      (float)thumbSize, (float)height - 4.0f };

        const float a = isMouseDown ? 0.65f : (isMouseOver ? 0.42f : 0.18f);
        g.setColour(juce::Colours::white.withAlpha(a));
        g.fillRoundedRectangle(thumb, 3.0f);
    }

    int getDefaultScrollbarWidth() override { return 8; }

    // =========================================================================
    // ComboBox
    // =========================================================================
    void drawComboBox(juce::Graphics& g, int width, int height,
                      bool, int, int, int, int,
                      juce::ComboBox& box) override
    {
        const juce::Rectangle<float> b(0.5f, 0.5f, (float)width - 1.0f, (float)height - 1.0f);
        const float r = 5.0f;
        const juce::Colour bg = box.findColour(juce::ComboBox::backgroundColourId);

        juce::ColourGradient grad(bg.brighter(0.07f), 0.0f, b.getY(),
                                  bg.darker(0.05f),   0.0f, b.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(b, r);
        g.setColour(rim());
        g.drawRoundedRectangle(b, r, 1.0f);

        const float ax = (float)width - 16.0f;
        const float ay = (float)height * 0.5f - 2.5f;
        juce::Path chevron;
        chevron.startNewSubPath(ax, ay);
        chevron.lineTo(ax + 5.0f, ay + 5.0f);
        chevron.lineTo(ax + 10.0f, ay);
        g.setColour(box.findColour(juce::ComboBox::arrowColourId));
        g.strokePath(chevron, juce::PathStrokeType(1.5f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // =========================================================================
    // AlertWindow
    // =========================================================================
    void drawAlertBox(juce::Graphics& g, juce::AlertWindow& aw,
                      const juce::Rectangle<int>& textArea,
                      juce::TextLayout& textLayout) override
    {
        const auto b = aw.getLocalBounds().toFloat();
        g.setColour(juce::Colour(kSurface2));
        g.fillRoundedRectangle(b, 10.0f);
        g.setColour(rim());
        g.drawRoundedRectangle(b.reduced(0.5f), 10.0f, 1.0f);
        textLayout.draw(g, textArea.toFloat());
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StudioLookAndFeel)
};
