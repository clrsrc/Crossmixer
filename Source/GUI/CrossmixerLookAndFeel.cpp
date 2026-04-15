#include "CrossmixerLookAndFeel.h"

CrossmixerLookAndFeel::CrossmixerLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, backgroundColour);
    setColour(juce::Label::textColourId, textColour);
    setColour(juce::TextButton::buttonColourId, surfaceColour);
    setColour(juce::TextButton::textColourOffId, textColour);
    setColour(juce::ComboBox::backgroundColourId, surfaceColour);
    setColour(juce::ComboBox::textColourId, textColour);
    setColour(juce::ComboBox::outlineColourId, accentColour);
    setColour(juce::PopupMenu::backgroundColourId, surfaceColour);
    setColour(juce::PopupMenu::textColourId, textColour);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, accentColour);
}

void CrossmixerLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y,
                                              int width, int height,
                                              float sliderPos, float /*minSliderPos*/,
                                              float /*maxSliderPos*/,
                                              juce::Slider::SliderStyle style,
                                              juce::Slider& /*slider*/)
{
    if (style != juce::Slider::LinearVertical)
    {
        LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, 0, 0, style, juce::Slider());
        return;
    }

    float trackWidth = 4.0f;
    float trackX = static_cast<float>(x) + static_cast<float>(width) * 0.5f - trackWidth * 0.5f;

    // Draw track background
    g.setColour(faderTrackColour);
    g.fillRoundedRectangle(trackX, static_cast<float>(y), trackWidth, static_cast<float>(height), 2.0f);

    // Draw active portion (below thumb)
    float activeHeight = static_cast<float>(y + height) - sliderPos;
    if (activeHeight > 0)
    {
        g.setColour(accentColour.withAlpha(0.6f));
        g.fillRoundedRectangle(trackX, sliderPos, trackWidth, activeHeight, 2.0f);
    }

    // Draw thumb
    float thumbWidth = static_cast<float>(width) * 0.7f;
    float thumbHeight = 8.0f;
    float thumbX = static_cast<float>(x) + static_cast<float>(width) * 0.5f - thumbWidth * 0.5f;
    float thumbY = sliderPos - thumbHeight * 0.5f;

    g.setColour(faderThumbColour);
    g.fillRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, 3.0f);

    // Center line on thumb
    g.setColour(accentColour);
    g.drawHorizontalLine(static_cast<int>(sliderPos), thumbX + 2.0f, thumbX + thumbWidth - 2.0f);
}

void CrossmixerLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y,
                                              int width, int height,
                                              float sliderPosProportional,
                                              float rotaryStartAngle,
                                              float rotaryEndAngle,
                                              juce::Slider& /*slider*/)
{
    float radius = static_cast<float>(juce::jmin(width, height)) * 0.5f - 4.0f;
    float centreX = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
    float centreY = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
    float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Background circle
    g.setColour(faderTrackColour);
    g.fillEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

    // Arc showing value
    juce::Path arcPath;
    arcPath.addCentredArc(centreX, centreY, radius - 2.0f, radius - 2.0f,
                           0.0f, rotaryStartAngle, angle, true);
    g.setColour(accentColour);
    g.strokePath(arcPath, juce::PathStrokeType(3.0f));

    // Pointer line
    float pointerLength = radius * 0.6f;
    float pointerX = centreX + pointerLength * std::cos(angle - juce::MathConstants<float>::halfPi);
    float pointerY = centreY + pointerLength * std::sin(angle - juce::MathConstants<float>::halfPi);
    g.setColour(textColour);
    g.drawLine(centreX, centreY, pointerX, pointerY, 2.0f);
}
