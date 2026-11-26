#pragma once
#include "JuceHeader.h"
#include <functional>

// Listens for Windows audio endpoint changes (device added/removed/state changed,
// e.g. a Bluetooth headset connecting) via IMMNotificationClient and invokes
// a callback on the JUCE message thread.
class DeviceChangeNotifier : private juce::AsyncUpdater
{
public:
    explicit DeviceChangeNotifier(std::function<void()> onChange);
    ~DeviceChangeNotifier() override;

private:
    void handleAsyncUpdate() override;

    class Client;
    Client* client = nullptr;
    void* enumerator = nullptr; // IMMDeviceEnumerator*
    std::function<void()> callback;

    JUCE_DECLARE_NON_COPYABLE(DeviceChangeNotifier)
};
