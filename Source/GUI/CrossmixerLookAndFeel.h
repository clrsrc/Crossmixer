#pragma once
#include "JuceHeader.h"

class CrossmixerLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CrossmixerLookAndFeel();

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle style, juce::Slider& slider) override;

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;

    // Colors
    static inline const juce::Colour backgroundColour    { 0xff1e1e2e };
    static inline const juce::Colour surfaceColour       { 0xff2a2a3a };
    static inline const juce::Colour channelBgColour     { 0xff252535 };
    static inline const juce::Colour accentColour        { 0xff6c8ebf };
    static inline const juce::Colour textColour          { 0xffdcdcdc };
    static inline const juce::Colour faderTrackColour    { 0xff3a3a4a };
    static inline const juce::Colour faderThumbColour    { 0xffb0b0c0 };
    static inline const juce::Colour meterGreenColour    { 0xff4caf50 };
    static inline const juce::Colour meterYellowColour   { 0xffffeb3b };
    static inline const juce::Colour meterRedColour      { 0xfff44336 };
};
