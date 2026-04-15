#pragma once
#include "JuceHeader.h"
#include "PeakMeter.h"

class AudioEngine;

class MasterSection : public juce::Component
{
public:
    MasterSection();

    void resized() override;

    void setEngine(AudioEngine* engine);

    float getVolume() const;
    void setVolume(float db);

private:
    juce::Label nameLabel;
    juce::Slider fader;
    PeakMeter meterL;
    PeakMeter meterR;

    AudioEngine* linkedEngine = nullptr;
};
