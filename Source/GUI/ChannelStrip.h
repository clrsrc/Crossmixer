#pragma once
#include "JuceHeader.h"
#include "PeakMeter.h"

class ChannelProcessor;

class ChannelStrip : public juce::Component
{
public:
    ChannelStrip(const juce::String& channelName);

    void resized() override;

    // Connect to audio engine
    void setProcessor(ChannelProcessor* processor);
    void setLevelSource(std::function<float()> source);

    // Get current values for preset save
    float getVolume() const;
    bool isMuted() const;

    // Set values for preset load
    void setVolume(float db);
    void setMuted(bool muted);

    juce::Slider& getFader() { return fader; }

private:
    juce::Label nameLabel;
    PeakMeter meter;
    juce::Slider fader;
    juce::TextButton muteButton{"Mute"};

    ChannelProcessor* linkedProcessor = nullptr;
};
