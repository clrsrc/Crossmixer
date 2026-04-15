#include "StereoTo51Router.h"
#include <cmath>

StereoTo51Router::StereoTo51Router()
{
    prepare(48000.0);
}

void StereoTo51Router::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
    // One-pole low-pass coefficient for ~120Hz
    // alpha = 1 - exp(-2 * pi * fc / fs)
    double fc = 120.0;
    lfeFilterCoeff = static_cast<float>(1.0 - std::exp(-2.0 * juce::MathConstants<double>::pi * fc / sampleRate));
    lfeFilterState = 0.0f;
}

void StereoTo51Router::reset()
{
    lfeFilterState = 0.0f;
}

void StereoTo51Router::processSample(float left, float right, float* output6ch)
{
    float mono = (left + right) * 0.5f;

    // LFE: low-pass filtered mono signal
    lfeFilterState += lfeFilterCoeff * (mono - lfeFilterState);

    output6ch[FrontLeft]  = left;
    output6ch[FrontRight] = right;
    output6ch[Center]     = mono;
    output6ch[LFE]        = lfeFilterState * lfeGain;
    output6ch[RearLeft]   = left;
    output6ch[RearRight]  = right;
}

void StereoTo51Router::processBlock(const float* stereoInterleaved, float* output6chInterleaved, int numFrames)
{
    for (int i = 0; i < numFrames; ++i)
    {
        float left  = stereoInterleaved[i * 2];
        float right = stereoInterleaved[i * 2 + 1];

        processSample(left, right, output6chInterleaved + i * NumChannels51);
    }
}
