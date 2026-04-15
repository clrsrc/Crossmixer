#pragma once
#include "JuceHeader.h"
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <atomic>
#include <thread>
#include <vector>
#include "RingBuffer.h"

struct AudioDeviceInfo;

class WasapiMultichannelOutput
{
public:
    WasapiMultichannelOutput();
    ~WasapiMultichannelOutput();

    // Enumerate output devices that support 6-channel (5.1) output
    std::vector<AudioDeviceInfo> enumerateDevices();

    // Start outputting to the specified device
    bool start(const juce::String& deviceId, RingBuffer<float>& inputBuffer);

    // Stop output
    void stop();

    bool isRunning() const { return running.load(); }

    int getSampleRate() const { return sampleRate; }
    int getNumOutputChannels() const { return numOutputChannels.load(); }

private:
    void outputThreadFunc(RingBuffer<float>& inputBuffer);

    std::thread outputThread;
    std::atomic<bool> running{false};
    std::atomic<bool> shouldStop{false};

    int sampleRate = 0;
    std::atomic<int> numOutputChannels{6};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WasapiMultichannelOutput)
};
