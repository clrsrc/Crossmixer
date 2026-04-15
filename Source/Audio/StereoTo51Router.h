#pragma once
#include "JuceHeader.h"
#include <atomic>

// Channel indices for 5.1 output
enum Channel51
{
    FrontLeft = 0,
    FrontRight,
    Center,
    LFE,
    RearLeft,
    RearRight,
    NumChannels51 = 6
};

// Routes stereo input to 5.1 output channels.
// Includes a low-pass filter for the LFE channel.
class StereoTo51Router
{
public:
    StereoTo51Router();

    void prepare(double sampleRate);
    void reset();

    // Swap front/rear L/R channels
    void setSwapFront(bool swap) { swapFront.store(swap, std::memory_order_relaxed); }
    void setSwapRear(bool swap) { swapRear.store(swap, std::memory_order_relaxed); }
    bool getSwapFront() const { return swapFront.load(std::memory_order_relaxed); }
    bool getSwapRear() const { return swapRear.load(std::memory_order_relaxed); }

    // Process a single stereo frame (L, R) and output 6 channels
    void processSample(float left, float right, float* output6ch);

    // Process a block of stereo frames to 6-channel output
    void processBlock(const float* stereoInterleaved, float* output6chInterleaved, int numFrames);

private:
    std::atomic<bool> swapFront{false};
    std::atomic<bool> swapRear{false};
    // Simple one-pole low-pass for LFE (120Hz)
    float lfeFilterState = 0.0f;
    float lfeFilterCoeff = 0.0f; // coefficient for ~120Hz cutoff
    float lfeGain = 6.0f;        // +10 dB boost for LFE
    double currentSampleRate = 48000.0;
};
