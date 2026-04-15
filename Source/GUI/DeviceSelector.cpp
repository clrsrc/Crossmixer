#include "DeviceSelector.h"
#include "../Audio/WasapiLoopbackCapture.h" // for AudioDeviceInfo

DeviceSelector::DeviceSelector()
{
    inputLabel.setText("Input:", juce::dontSendNotification);
    inputLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(inputLabel);

    outputLabel.setText("Output:", juce::dontSendNotification);
    outputLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(outputLabel);

    inputCombo.onChange = [this]()
    {
        if (onDeviceChanged)
            onDeviceChanged();
    };
    addAndMakeVisible(inputCombo);

    outputCombo.onChange = [this]()
    {
        if (onDeviceChanged)
            onDeviceChanged();
    };
    addAndMakeVisible(outputCombo);
}

void DeviceSelector::resized()
{
    auto bounds = getLocalBounds().reduced(4, 2);

    auto topRow = bounds.removeFromTop(bounds.getHeight() / 2);
    auto bottomRow = bounds;

    int labelWidth = 60;

    inputLabel.setBounds(topRow.removeFromLeft(labelWidth));
    inputCombo.setBounds(topRow.reduced(2, 0));

    outputLabel.setBounds(bottomRow.removeFromLeft(labelWidth));
    outputCombo.setBounds(bottomRow.reduced(2, 0));
}

void DeviceSelector::setInputDevices(const std::vector<AudioDeviceInfo>& devices)
{
    inputCombo.clear(juce::dontSendNotification);
    inputIds.clear();

    for (int i = 0; i < static_cast<int>(devices.size()); ++i)
    {
        inputCombo.addItem(devices[i].name, i + 1);
        inputIds.push_back(devices[i].id);
    }

    if (!devices.empty())
        inputCombo.setSelectedItemIndex(0, juce::dontSendNotification);
}

void DeviceSelector::setOutputDevices(const std::vector<AudioDeviceInfo>& devices)
{
    outputCombo.clear(juce::dontSendNotification);
    outputIds.clear();

    for (int i = 0; i < static_cast<int>(devices.size()); ++i)
    {
        outputCombo.addItem(devices[i].name, i + 1);
        outputIds.push_back(devices[i].id);
    }

    if (!devices.empty())
        outputCombo.setSelectedItemIndex(0, juce::dontSendNotification);
}

juce::String DeviceSelector::getSelectedInputId() const
{
    int idx = inputCombo.getSelectedItemIndex();
    if (idx >= 0 && idx < static_cast<int>(inputIds.size()))
        return inputIds[idx];
    return {};
}

juce::String DeviceSelector::getSelectedOutputId() const
{
    int idx = outputCombo.getSelectedItemIndex();
    if (idx >= 0 && idx < static_cast<int>(outputIds.size()))
        return outputIds[idx];
    return {};
}

void DeviceSelector::selectInputById(const juce::String& id)
{
    for (int i = 0; i < static_cast<int>(inputIds.size()); ++i)
    {
        if (inputIds[i] == id)
        {
            inputCombo.setSelectedItemIndex(i, juce::dontSendNotification);
            return;
        }
    }
}

void DeviceSelector::selectOutputById(const juce::String& id)
{
    for (int i = 0; i < static_cast<int>(outputIds.size()); ++i)
    {
        if (outputIds[i] == id)
        {
            outputCombo.setSelectedItemIndex(i, juce::dontSendNotification);
            return;
        }
    }
}
