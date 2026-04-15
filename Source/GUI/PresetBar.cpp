#include "PresetBar.h"

PresetBar::PresetBar()
{
    presetLabel.setText("Preset:", juce::dontSendNotification);
    presetLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(presetLabel);

    presetCombo.onChange = [this]()
    {
        if (onPresetSelected)
            onPresetSelected(presetCombo.getText());
    };
    addAndMakeVisible(presetCombo);

    saveButton.onClick = [this]()
    {
        if (onSaveClicked) onSaveClicked();
    };
    addAndMakeVisible(saveButton);

    saveAsButton.onClick = [this]()
    {
        if (onSaveAsClicked) onSaveAsClicked();
    };
    addAndMakeVisible(saveAsButton);

    deleteButton.onClick = [this]()
    {
        if (onDeleteClicked) onDeleteClicked();
    };
    addAndMakeVisible(deleteButton);
}

void PresetBar::resized()
{
    auto bounds = getLocalBounds().reduced(4, 2);

    int labelWidth = 50;
    int buttonWidth = 60;

    presetLabel.setBounds(bounds.removeFromLeft(labelWidth));
    deleteButton.setBounds(bounds.removeFromRight(buttonWidth).reduced(2, 0));
    saveAsButton.setBounds(bounds.removeFromRight(buttonWidth).reduced(2, 0));
    saveButton.setBounds(bounds.removeFromRight(buttonWidth).reduced(2, 0));
    presetCombo.setBounds(bounds.reduced(4, 0));
}

void PresetBar::setPresetNames(const juce::StringArray& names)
{
    presetCombo.clear(juce::dontSendNotification);

    for (int i = 0; i < names.size(); ++i)
        presetCombo.addItem(names[i], i + 1);

    if (names.size() > 0)
        presetCombo.setSelectedItemIndex(0, juce::dontSendNotification);
}

juce::String PresetBar::getSelectedPresetName() const
{
    return presetCombo.getText();
}

void PresetBar::setSelectedPreset(const juce::String& name)
{
    for (int i = 0; i < presetCombo.getNumItems(); ++i)
    {
        if (presetCombo.getItemText(i) == name)
        {
            presetCombo.setSelectedItemIndex(i, juce::dontSendNotification);
            return;
        }
    }
}
