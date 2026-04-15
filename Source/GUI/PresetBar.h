#pragma once
#include "JuceHeader.h"
#include <functional>

class PresetBar : public juce::Component
{
public:
    PresetBar();

    void resized() override;

    void setPresetNames(const juce::StringArray& names);
    juce::String getSelectedPresetName() const;
    void setSelectedPreset(const juce::String& name);

    // Callbacks
    std::function<void(const juce::String& name)> onPresetSelected;
    std::function<void()> onSaveClicked;
    std::function<void()> onSaveAsClicked;
    std::function<void()> onDeleteClicked;

private:
    juce::Label presetLabel;
    juce::ComboBox presetCombo;
    juce::TextButton saveButton{"Save"};
    juce::TextButton saveAsButton{"Save As"};
    juce::TextButton deleteButton{"Delete"};
};
