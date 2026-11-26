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
    // Returns false if the ID is not in the current list
    bool selectInputById(const juce::String& id);
    bool selectOutputById(const juce::String& id);

    // Show no selection (e.g. when the chosen device is disconnected)
    void clearInputSelection();
    void clearOutputSelection();

    // Callback when device selection changes
    std::function<void()> onDeviceChanged;

    // Callback when the refresh button is clicked
    std::function<void()> onRefreshClicked;

private:
    juce::Label inputLabel;
    juce::Label outputLabel;
    juce::ComboBox inputCombo;
    juce::ComboBox outputCombo;
    juce::TextButton refreshButton{"Refresh"};

    std::vector<juce::String> inputIds;
    std::vector<juce::String> outputIds;
};
