#pragma once
#include "JuceHeader.h"

class PresetManager
{
public:
    PresetManager();

    // Get preset directory
    juce::File getPresetDirectory() const;

    // Get list of available preset names
    juce::StringArray getPresetNames() const;

    // Save current state to preset
    bool savePreset(const juce::String& name, const juce::ValueTree& state);

    // Load preset, returns empty ValueTree on failure
    juce::ValueTree loadPreset(const juce::String& name);

    // Delete a preset
    bool deletePreset(const juce::String& name);

    // Create default "Flat" preset if no presets exist
    void ensureDefaultPreset();

private:
    juce::File presetDir;
};
