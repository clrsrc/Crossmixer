#pragma once
#include "JuceHeader.h"

class PeakMeter : public juce::Component, public juce::Timer
{
public:
    PeakMeter();
    ~PeakMeter() override;

    void paint(juce::Graphics& g) override;

    // Set current peak level (0.0 - 1.0), called from timer
    void setPeakLevel(float level);

    // Set the function that provides the current peak level
    void setLevelSource(std::function<float()> source);

private:
    void timerCallback() override;

    std::function<float()> levelSource;

    float currentLevel = 0.0f;
    float displayLevel = 0.0f;  // smoothed for display
    float peakHold = 0.0f;
    int peakHoldCounter = 0;

    static constexpr float decayRate = 0.92f;
    static constexpr int peakHoldFrames = 30; // ~1 second at 30Hz timer
};
