#include "ChannelProcessor.h"

ChannelProcessor::ChannelProcessor()
{
}

void ChannelProcessor::prepare(double /*sampleRate*/, int /*blockSize*/)
{
    volumeGain = juce::Decibels::decibelsToGain(volumeDb.load(), -60.0f);
}

void ChannelProcessor::reset()
{
}

float ChannelProcessor::processSample(float input)
{
    if (++updateCounter >= updateInterval)
    {
        updateCounter = 0;
        float newVol = volumeDb.load(std::memory_order_relaxed);
        volumeGain = juce::Decibels::decibelsToGain(newVol, -60.0f);
    }

    if (muted.load(std::memory_order_relaxed))
        return 0.0f;

    return input * volumeGain;
}

void ChannelProcessor::setVolume(float db)
{
    volumeDb.store(db, std::memory_order_relaxed);
}

void ChannelProcessor::setMuted(bool m)
{
    muted.store(m, std::memory_order_relaxed);
}
