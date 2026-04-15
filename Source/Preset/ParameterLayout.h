#pragma once
#include "JuceHeader.h"
#include <array>

// Channel names for the 5.1 layout
static const std::array<juce::String, 6> channelNames = {
    "FL", "FR", "Ctr", "LFE", "RL", "RR"
};

// Parameter ID helpers
namespace ParamIDs
{
    inline juce::String channelVolume(int ch) { return "ch" + juce::String(ch) + "_volume"; }
    inline juce::String channelMute(int ch) { return "ch" + juce::String(ch) + "_mute"; }
    inline juce::String masterVolume() { return "master_volume"; }
}
