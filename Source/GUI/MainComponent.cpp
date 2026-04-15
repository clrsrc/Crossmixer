#include "MainComponent.h"

MainComponent::MainComponent()
{
    setLookAndFeel(&lookAndFeel);

    // Create channel strips
    for (int i = 0; i < 6; ++i)
    {
        channelStrips[i] = std::make_unique<ChannelStrip>(channelNames[i]);
        addAndMakeVisible(*channelStrips[i]);
    }

    addAndMakeVisible(masterSection);
    addAndMakeVisible(deviceSelector);
    addAndMakeVisible(presetBar);
    addAndMakeVisible(inputSpectrum);

    statusLabel.setJustificationType(juce::Justification::centredLeft);
    statusLabel.setFont(juce::FontOptions(11.0f));
    statusLabel.setText("Ready", juce::dontSendNotification);
    addAndMakeVisible(statusLabel);

    trayButton.onClick = [this]()
    {
        if (auto* window = findParentComponentOfClass<juce::DocumentWindow>())
            window->setVisible(false);
    };
    addAndMakeVisible(trayButton);

    swapFrontButton.setClickingTogglesState(true);
    swapFrontButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff6c8ebf));
    swapFrontButton.onClick = [this]()
    {
        engine.getRouter().setSwapFront(swapFrontButton.getToggleState());
        saveDeviceSelection();
    };
    addAndMakeVisible(swapFrontButton);

    swapRearButton.setClickingTogglesState(true);
    swapRearButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff6c8ebf));
    swapRearButton.onClick = [this]()
    {
        engine.getRouter().setSwapRear(swapRearButton.getToggleState());
        saveDeviceSelection();
    };
    addAndMakeVisible(swapRearButton);

    // Device change callback
    deviceSelector.onDeviceChanged = [this]() { onDeviceChanged(); };

    // Preset callbacks
    presetBar.onPresetSelected = [this](const juce::String& name) { loadPreset(name); saveDeviceSelection(); };
    presetBar.onSaveClicked = [this]() { saveCurrentPreset(); };
    presetBar.onSaveAsClicked = [this]() { savePresetAs(); };
    presetBar.onDeleteClicked = [this]() { deleteCurrentPreset(); };

    // Initialize
    presetManager.ensureDefaultPreset();
    refreshDeviceLists();
    refreshPresetList();
    restoreDeviceSelection();

    // Auto-start engine with restored devices
    startEngine();

    // Load last preset (after engine is running so processors are connected)
    auto lastPreset = presetBar.getSelectedPresetName();
    if (lastPreset.isNotEmpty())
        loadPreset(lastPreset);

    // Re-apply swap state after engine start
    engine.getRouter().setSwapFront(swapFrontButton.getToggleState());
    engine.getRouter().setSwapRear(swapRearButton.getToggleState());

    setSize(900, 650);
}

MainComponent::~MainComponent()
{
    stopEngine();
    setLookAndFeel(nullptr);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(CrossmixerLookAndFeel::backgroundColour);
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();

    // Top section: device selector (2 rows) + preset bar (1 row), all rows equal height
    int rowHeight = 30;
    deviceSelector.setBounds(bounds.removeFromTop(rowHeight * 2));
    presetBar.setBounds(bounds.removeFromTop(rowHeight));

    // Bottom: status bar with swap buttons and tray button
    auto bottomBar = bounds.removeFromBottom(24);
    trayButton.setBounds(bottomBar.removeFromRight(120).reduced(4, 0));
    swapRearButton.setBounds(bottomBar.removeFromRight(90).reduced(4, 0));
    swapFrontButton.setBounds(bottomBar.removeFromRight(90).reduced(4, 0));
    statusLabel.setBounds(bottomBar.reduced(8, 0));

    // Separator line area
    bounds.removeFromTop(4);
    bounds.removeFromBottom(2);

    // Input spectrum: full-width strip above the status bar
    int spectrumHeight = 90;
    inputSpectrum.setBounds(bounds.removeFromBottom(spectrumHeight).reduced(6, 2));
    bounds.removeFromBottom(2);

    // Main mixer area
    auto mixerArea = bounds.reduced(4, 0);

    // Master section on the right
    int masterWidth = 80;
    auto masterArea = mixerArea.removeFromRight(masterWidth);
    masterSection.setBounds(masterArea);

    // Separator
    mixerArea.removeFromRight(4);

    // 6 channel strips
    int stripWidth = mixerArea.getWidth() / 6;
    for (int i = 0; i < 6; ++i)
        channelStrips[i]->setBounds(mixerArea.removeFromLeft(stripWidth));
}

void MainComponent::refreshDeviceLists()
{
    auto inputDevices = engine.getInputDevices();
    auto outputDevices = engine.getOutputDevices();

    deviceSelector.setInputDevices(inputDevices);
    deviceSelector.setOutputDevices(outputDevices);
}

void MainComponent::onDeviceChanged()
{
    stopEngine();
    saveDeviceSelection();
    startEngine();
}

void MainComponent::saveDeviceSelection()
{
    auto settingsFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                            .getChildFile("Crossmixer")
                            .getChildFile("devices.xml");

    juce::ValueTree state("DeviceSelection");
    state.setProperty("inputId", deviceSelector.getSelectedInputId(), nullptr);
    state.setProperty("outputId", deviceSelector.getSelectedOutputId(), nullptr);
    state.setProperty("lastPreset", presetBar.getSelectedPresetName(), nullptr);
    state.setProperty("swapFront", swapFrontButton.getToggleState(), nullptr);
    state.setProperty("swapRear", swapRearButton.getToggleState(), nullptr);

    auto xml = state.createXml();
    if (xml != nullptr)
        xml->writeTo(settingsFile);
}

void MainComponent::restoreDeviceSelection()
{
    auto settingsFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                            .getChildFile("Crossmixer")
                            .getChildFile("devices.xml");

    if (!settingsFile.existsAsFile())
        return;

    auto xml = juce::XmlDocument::parse(settingsFile);
    if (xml == nullptr)
        return;

    auto state = juce::ValueTree::fromXml(*xml);
    if (!state.isValid())
        return;

    auto inputId = state.getProperty("inputId").toString();
    auto outputId = state.getProperty("outputId").toString();

    if (inputId.isNotEmpty())
        deviceSelector.selectInputById(inputId);
    if (outputId.isNotEmpty())
        deviceSelector.selectOutputById(outputId);

    auto lastPreset = state.getProperty("lastPreset").toString();
    if (lastPreset.isNotEmpty())
        presetBar.setSelectedPreset(lastPreset);

    bool swapFront = static_cast<bool>(state.getProperty("swapFront", false));
    bool swapRear = static_cast<bool>(state.getProperty("swapRear", false));
    swapFrontButton.setToggleState(swapFront, juce::dontSendNotification);
    swapRearButton.setToggleState(swapRear, juce::dontSendNotification);
    engine.getRouter().setSwapFront(swapFront);
    engine.getRouter().setSwapRear(swapRear);
}

void MainComponent::startEngine()
{
    auto inputId = deviceSelector.getSelectedInputId();
    auto outputId = deviceSelector.getSelectedOutputId();

    if (inputId.isEmpty() || outputId.isEmpty())
    {
        statusLabel.setText("No devices selected", juce::dontSendNotification);
        return;
    }

    if (engine.start(inputId, outputId))
    {
        // Connect channel strips to engine
        for (int i = 0; i < 6; ++i)
        {
            channelStrips[i]->setProcessor(&engine.getChannelProcessor(i));
            int ch = i;
            channelStrips[i]->setLevelSource([this, ch]() { return engine.getPeakLevel(ch); });
        }

        // Connect input spectrum to engine input sum fifo
        double inSr = static_cast<double>(engine.getInputSampleRate());
        if (inSr <= 0.0) inSr = static_cast<double>(engine.getOutputSampleRate());
        inputSpectrum.setSource(
            [this](float* dest, int n) { engine.getInputSumFifo().copyLatest(dest, n); },
            inSr);

        // Activate spectrum if window is currently visible
        if (auto* w = getTopLevelComponent())
            setSpectrumEnabled(w->isVisible());

        masterSection.setEngine(&engine);

        // Re-apply swap state after engine start
        engine.getRouter().setSwapFront(swapFrontButton.getToggleState());
        engine.getRouter().setSwapRear(swapRearButton.getToggleState());

        int sr = engine.getOutputSampleRate();
        int ch = engine.getNumOutputChannels();
        statusLabel.setText("Running | " + juce::String(sr) + " Hz | " + juce::String(ch) + " ch",
                            juce::dontSendNotification);
    }
    else
    {
        statusLabel.setText("Failed to start audio engine", juce::dontSendNotification);
    }
}

void MainComponent::stopEngine()
{
    engine.stop();

    inputSpectrum.setActive(false);
    inputSpectrum.setSource(nullptr, 48000.0);

    for (int i = 0; i < 6; ++i)
    {
        channelStrips[i]->setProcessor(nullptr);
        channelStrips[i]->setLevelSource(nullptr);
    }

    masterSection.setEngine(nullptr);
    statusLabel.setText("Stopped", juce::dontSendNotification);
}

juce::ValueTree MainComponent::captureState()
{
    juce::ValueTree state("CrossmixerPreset");

    for (int ch = 0; ch < 6; ++ch)
    {
        juce::ValueTree channel("Channel");
        channel.setProperty("index", ch, nullptr);
        channel.setProperty("volume", static_cast<double>(channelStrips[ch]->getVolume()), nullptr);
        channel.setProperty("muted", channelStrips[ch]->isMuted(), nullptr);
        state.appendChild(channel, nullptr);
    }

    state.setProperty("masterVolume", static_cast<double>(masterSection.getVolume()), nullptr);

    return state;
}

void MainComponent::restoreState(const juce::ValueTree& state)
{
    if (!state.isValid())
        return;

    for (int i = 0; i < state.getNumChildren() && i < 6; ++i)
    {
        auto channel = state.getChild(i);
        int ch = channel.getProperty("index", i);
        if (ch >= 0 && ch < 6)
        {
            channelStrips[ch]->setVolume(static_cast<float>(static_cast<double>(channel.getProperty("volume", 0.0))));
            channelStrips[ch]->setMuted(static_cast<bool>(channel.getProperty("muted", false)));
        }
    }

    masterSection.setVolume(static_cast<float>(static_cast<double>(state.getProperty("masterVolume", 0.0))));
}

void MainComponent::setSpectrumEnabled(bool enabled)
{
    inputSpectrum.setActive(enabled);
}

void MainComponent::saveCurrentPreset()
{
    auto name = presetBar.getSelectedPresetName();
    if (name.isNotEmpty())
    {
        presetManager.savePreset(name, captureState());
    }
}

void MainComponent::savePresetAs()
{
    auto* alertWindow = new juce::AlertWindow("Save Preset As",
                                                "Enter preset name:",
                                                juce::MessageBoxIconType::QuestionIcon);
    alertWindow->addTextEditor("name", "New Preset");
    alertWindow->addButton("Save", 1);
    alertWindow->addButton("Cancel", 0);

    alertWindow->enterModalState(true, juce::ModalCallbackFunction::create(
        [this, alertWindow](int result)
        {
            if (result == 1)
            {
                auto name = alertWindow->getTextEditorContents("name");
                if (name.isNotEmpty())
                {
                    presetManager.savePreset(name, captureState());
                    refreshPresetList();
                    presetBar.setSelectedPreset(name);
                }
            }
            delete alertWindow;
        }
    ));
}

void MainComponent::deleteCurrentPreset()
{
    auto name = presetBar.getSelectedPresetName();
    if (name.isNotEmpty())
    {
        presetManager.deletePreset(name);
        refreshPresetList();
    }
}

void MainComponent::loadPreset(const juce::String& name)
{
    auto state = presetManager.loadPreset(name);
    restoreState(state);
}

void MainComponent::refreshPresetList()
{
    presetBar.setPresetNames(presetManager.getPresetNames());
}
