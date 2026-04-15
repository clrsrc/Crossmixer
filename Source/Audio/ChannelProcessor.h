#pragma once
#include "JuceHeader.h"
#include <atomic>

// Per-channel audio processor: volume gain + mute
class ChannelProcessor
{
public:
    ChannelProcessor();

    void prepare(double sampleRate, int blockSize);
    void reset();

    // Process a single sample
    float processSample(float input);

    // Set parameters (thread-safe, called from GUI thread)
    void setVolume(float volumeDb);      // -60 to +6 dB
    void setMuted(bool muted);

    float getVolume() const { return volumeDb.load(); }
    bool isMuted() const { return muted.load(); }

private:
    std::atomic<float> volumeDb{0.0f};
    std::atomic<bool> muted{false};

    float volumeGain = 1.0f;

    int updateCounter = 0;
    static constexpr int updateInterval = 64;
};
