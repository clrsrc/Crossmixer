#pragma once
#include "JuceHeader.h"
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <atomic>
#include <thread>
#include <string>
#include <vector>
#include "RingBuffer.h"

struct AudioDeviceInfo
{
    juce::String id;
    juce::String name;
};

class WasapiLoopbackCapture
{
public:
    WasapiLoopbackCapture();
    ~WasapiLoopbackCapture();

    // Enumerate available render endpoints (for loopback capture)
    std::vector<AudioDeviceInfo> enumerateDevices();

    // Start capturing from the specified device (by device ID)
    bool start(const juce::String& deviceId, RingBuffer<float>& outputBuffer);

    // Stop capturing
    void stop();

    bool isRunning() const { return running.load(); }

    // Returns the sample rate of the captured stream
    int getSampleRate() const { return sampleRate; }
    int getNumChannels() const { return numChannels; }

private:
    void captureThreadFunc(RingBuffer<float>& outputBuffer);

    IMMDeviceEnumerator* enumerator = nullptr;
    IAudioClient* audioClient = nullptr;
    IAudioCaptureClient* captureClient = nullptr;

    std::thread captureThread;
    std::atomic<bool> running{false};
    std::atomic<bool> shouldStop{false};

    int sampleRate = 0;
    int numChannels = 0;
    int bitsPerSample = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WasapiLoopbackCapture)
};
