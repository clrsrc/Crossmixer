#pragma once
#include "JuceHeader.h"
#include "ChannelStrip.h"
#include "MasterSection.h"
#include "DeviceSelector.h"
#include "PresetBar.h"
#include "SpectrumAnalyzer.h"
#include "CrossmixerLookAndFeel.h"
#include "../Audio/AudioEngine.h"
#include "../Audio/DeviceChangeNotifier.h"
#include "../Preset/PresetManager.h"
#include "../Preset/ParameterLayout.h"

class MainComponent : public juce::Component,
                      private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Enable/disable spectrum updates (called when window is shown/hidden)
    void setSpectrumEnabled(bool enabled);

private:
    void startEngine();
    void stopEngine();
    void refreshDeviceLists();
    void onDeviceChanged();
    void saveDeviceSelection();
    void restoreDeviceSelection();
    void handleDeviceListChange();
    void scheduleDeviceListChange();
    void timerCallback() override;

    // Preset management
    void saveCurrentPreset();
    void savePresetAs();
    void deleteCurrentPreset();
    void loadPreset(const juce::String& name);
    juce::ValueTree captureState();
    void restoreState(const juce::ValueTree& state);
    void refreshPresetList();

    CrossmixerLookAndFeel lookAndFeel;

    DeviceSelector deviceSelector;
    PresetBar presetBar;

    std::array<std::unique_ptr<ChannelStrip>, 6> channelStrips;
    MasterSection masterSection;
    SpectrumAnalyzer inputSpectrum;

    juce::Label statusLabel;
    juce::TextButton trayButton{"Minimize to Tray"};
    juce::TextButton swapFrontButton{"Swap FR/FL"};
    juce::TextButton swapRearButton{"Swap RR/RL"};

    AudioEngine engine;
    PresetManager presetManager;

    // Devices the user chose (kept while a device is temporarily disconnected)
    juce::String preferredInputId, preferredOutputId;
    // Devices the running engine uses (empty when stopped)
    juce::String activeInputId, activeOutputId;

    // Watchdog timer state (message thread only)
    bool deviceChangePending = false;
    juce::uint32 deviceChangeDueMs = 0;
    juce::uint32 nextRetryMs = 0;
    bool engineStartFailed = false;

    // Must be destroyed before the members its callback uses
    std::unique_ptr<DeviceChangeNotifier> deviceNotifier;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
