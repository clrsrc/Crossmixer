#pragma once
#include "JuceHeader.h"

class SpectrumAnalyzer : public juce::Component, private juce::Timer
{
public:
    SpectrumAnalyzer();
    ~SpectrumAnalyzer() override;

    // Source fills `fftSize` mono samples in chronological order
    void setSource(std::function<void(float*, int)> src, double sampleRate);

    // Enable / disable FFT updates (called when app is shown/hidden)
    void setActive(bool shouldBeActive);

    void paint(juce::Graphics& g) override;

    static constexpr int fftOrder = 10;
    static constexpr int fftSize = 1 << fftOrder;  // 1024
    static constexpr int numBars = 48;

private:
    void timerCallback() override;

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window {
        (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann
    };

    float fftData[fftSize * 2] {};
    float barLevels[numBars] {};

    std::function<void(float*, int)> source;
    double sampleRate = 48000.0;
    bool active = false;
};
