#pragma once
#include "JuceHeader.h"
#include <functional>

struct AudioDeviceInfo;

class DeviceSelector : public juce::Component
{
public:
    DeviceSelector();

    void resized() override;

    void setInputDevices(const std::vector<AudioDeviceInfo>& devices);
    void setOutputDevices(const std::vector<AudioDeviceInfo>& devices);

    juce::String getSelectedInputId() const;
    juce::String getSelectedOutputId() const;

    // Select devices by ID (for restoring saved selection)
    void selectInputById(const juce::String& id);
    void selectOutputById(const juce::String& id);

    // Callback when device selection changes
    std::function<void()> onDeviceChanged;

private:
    juce::Label inputLabel;
    juce::Label outputLabel;
    juce::ComboBox inputCombo;
    juce::ComboBox outputCombo;

    std::vector<juce::String> inputIds;
    std::vector<juce::String> outputIds;
};
