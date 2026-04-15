#include "MasterSection.h"
#include "../Audio/AudioEngine.h"

MasterSection::MasterSection()
{
    nameLabel.setText("MST", juce::dontSendNotification);
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    addAndMakeVisible(nameLabel);

    fader.setSliderStyle(juce::Slider::LinearVertical);
    fader.setRange(-60.0, 6.0, 0.1);
    fader.setValue(0.0);
    fader.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
    fader.setDoubleClickReturnValue(true, 0.0);
    fader.setTextValueSuffix(" dB");
    fader.onValueChange = [this]()
    {
        if (linkedEngine != nullptr)
            linkedEngine->setMasterVolume(static_cast<float>(fader.getValue()));
    };
    addAndMakeVisible(fader);

    addAndMakeVisible(meterL);
    addAndMakeVisible(meterR);
}

void MasterSection::resized()
{
    auto bounds = getLocalBounds().reduced(2);

    nameLabel.setBounds(bounds.removeFromTop(20));

    // Meters on left side, fader on right
    auto meterArea = bounds.removeFromLeft(20);
    meterL.setBounds(meterArea.removeFromLeft(9).reduced(0, 4));
    meterR.setBounds(meterArea.reduced(0, 4));

    fader.setBounds(bounds.reduced(2, 0));
}

void MasterSection::setEngine(AudioEngine* engine)
{
    linkedEngine = engine;

    if (engine != nullptr)
    {
        fader.setValue(engine->getMasterVolume(), juce::dontSendNotification);

        meterL.setLevelSource([engine]() { return engine->getInputPeakLevel(0); });
        meterR.setLevelSource([engine]() { return engine->getInputPeakLevel(1); });
    }
}

float MasterSection::getVolume() const
{
    return static_cast<float>(fader.getValue());
}

void MasterSection::setVolume(float db)
{
    fader.setValue(db, juce::sendNotificationSync);
}
