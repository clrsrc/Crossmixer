#pragma once
#include "JuceHeader.h"
#include <atomic>
#include <thread>
#include <array>
#include "RingBuffer.h"
#include "WasapiLoopbackCapture.h"
#include "WasapiMultichannelOutput.h"
#include "StereoTo51Router.h"
#include "ChannelProcessor.h"

// Lock-free ring buffer of recent mono samples for spectrum display.
// Single-writer (audio thread), single-reader (GUI thread).
class SpectrumFifo
{
public:
    static constexpr int size = 2048;    // power of two, >= fft size

    void pushBlock(const float* src, int n) noexcept
    {
        int w = writeIndex.load(std::memory_order_relaxed);
        for (int i = 0; i < n; ++i)
            buffer[(w + i) & (size - 1)] = src[i];
        writeIndex.store(w + n, std::memory_order_release);
    }

    // Copy the most recent `count` samples into dest (chronological order).
    void copyLatest(float* dest, int count) const noexcept
    {
        const int w = writeIndex.load(std::memory_order_acquire);
        const int start = w - count;
        for (int i = 0; i < count; ++i)
            dest[i] = buffer[(start + i) & (size - 1)];
    }

private:
    float buffer[size] = {};
    std::atomic<int> writeIndex{0};
};

class AudioEngine
{
public:
    AudioEngine();
    ~AudioEngine();

    // Start with specified input and output device IDs
    bool start(const juce::String& inputDeviceId, const juce::String& outputDeviceId);
    void stop();

    bool isRunning() const { return running.load(); }

    // Access channel processors for GUI control
    ChannelProcessor& getChannelProcessor(int channel) { return channelProcessors[channel]; }

    // Master volume
    void setMasterVolume(float db);
    float getMasterVolume() const { return masterVolumeDb.load(); }

    // Peak levels for GUI meters (0.0 - 1.0)
    float getPeakLevel(int channel) const { return peakLevels[channel].load(); }

    // Input peak level (stereo)
    float getInputPeakLevel(int channel) const { return inputPeakLevels[channel].load(); }

    // Get sample rates and channel count
    int getInputSampleRate() const;
    int getOutputSampleRate() const;
    int getNumOutputChannels() const { return output.getNumOutputChannels(); }

    // Access router for swap controls
    StereoTo51Router& getRouter() { return router; }

    // Spectrum access for GUI (stereo input L+R mono sum)
    const SpectrumFifo& getInputSumFifo() const { return inputSumFifo; }

    // Device enumeration
    std::vector<AudioDeviceInfo> getInputDevices() { return capture.enumerateDevices(); }
    std::vector<AudioDeviceInfo> getOutputDevices() { return output.enumerateDevices(); }

private:
    void processThreadFunc();

    WasapiLoopbackCapture capture;
    WasapiMultichannelOutput output;
    StereoTo51Router router;

    std::array<ChannelProcessor, 6> channelProcessors;

    // Ring buffers: capture → processing (stereo), processing → output (6ch)
    std::unique_ptr<RingBuffer<float>> captureBuffer;
    std::unique_ptr<RingBuffer<float>> outputBuffer;

    std::thread processThread;
    std::atomic<bool> running{false};
    std::atomic<bool> shouldStop{false};

    std::atomic<float> masterVolumeDb{0.0f};
    float masterGain = 1.0f;

    // Peak levels per output channel
    std::array<std::atomic<float>, 6> peakLevels;
    std::array<std::atomic<float>, 2> inputPeakLevels;

    // Spectrum buffer for stereo input sum
    SpectrumFifo inputSumFifo;

    static constexpr int ringBufferSize = 16384;
    static constexpr int processBlockSize = 256;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};
