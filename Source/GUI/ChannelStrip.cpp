#include "ChannelStrip.h"
#include "../Audio/ChannelProcessor.h"

ChannelStrip::ChannelStrip(const juce::String& channelName)
{
    nameLabel.setText(channelName, juce::dontSendNotification);
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    addAndMakeVisible(nameLabel);

    addAndMakeVisible(meter);

    fader.setSliderStyle(juce::Slider::LinearVertical);
    fader.setRange(-60.0, 6.0, 0.1);
    fader.setValue(0.0);
    fader.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
    fader.setDoubleClickReturnValue(true, 0.0);
    fader.setTextValueSuffix(" dB");
    fader.onValueChange = [this]()
    {
        if (linkedProcessor != nullptr)
            linkedProcessor->setVolume(static_cast<float>(fader.getValue()));
    };
    addAndMakeVisible(fader);

    muteButton.setClickingTogglesState(true);
    muteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red.withAlpha(0.7f));
    muteButton.onClick = [this]()
    {
        if (linkedProcessor != nullptr)
            linkedProcessor->setMuted(muteButton.getToggleState());
    };
    addAndMakeVisible(muteButton);
}

void ChannelStrip::resized()
{
    auto bounds = getLocalBounds().reduced(2);

    nameLabel.setBounds(bounds.removeFromTop(20));
    muteButton.setBounds(bounds.removeFromBottom(24).reduced(4, 0));

    // Split remaining area: meter (left 25%) and fader (rest)
    auto meterFaderArea = bounds;
    int meterWidth = juce::jmax(12, meterFaderArea.getWidth() / 4);

    meter.setBounds(meterFaderArea.removeFromLeft(meterWidth).reduced(2, 4));
    fader.setBounds(meterFaderArea.reduced(2, 0));
}

void ChannelStrip::setProcessor(ChannelProcessor* processor)
{
    linkedProcessor = processor;

    if (processor != nullptr)
    {
        fader.setValue(processor->getVolume(), juce::dontSendNotification);
        muteButton.setToggleState(processor->isMuted(), juce::dontSendNotification);
    }
}

void ChannelStrip::setLevelSource(std::function<float()> source)
{
    meter.setLevelSource(std::move(source));
}

float ChannelStrip::getVolume() const { return static_cast<float>(fader.getValue()); }
bool ChannelStrip::isMuted() const { return muteButton.getToggleState(); }

void ChannelStrip::setVolume(float db)
{
    fader.setValue(db, juce::sendNotificationSync);
}

void ChannelStrip::setMuted(bool muted)
{
    muteButton.setToggleState(muted, juce::sendNotificationSync);
}
