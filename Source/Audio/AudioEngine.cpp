#include "AudioEngine.h"
#include <cmath>

AudioEngine::AudioEngine()
{
    for (auto& p : peakLevels) p.store(0.0f);
    for (auto& p : inputPeakLevels) p.store(0.0f);
}

AudioEngine::~AudioEngine()
{
    stop();
}

bool AudioEngine::start(const juce::String& inputDeviceId, const juce::String& outputDeviceId)
{
    stop();

    // Create ring buffers
    captureBuffer = std::make_unique<RingBuffer<float>>(ringBufferSize, 2);  // stereo
    outputBuffer  = std::make_unique<RingBuffer<float>>(ringBufferSize, 6);  // 5.1

    // Start capture
    if (!capture.start(inputDeviceId, *captureBuffer))
    {
        DBG("Failed to start WASAPI loopback capture");
        return false;
    }

    // Wait for capture to initialize so we know the sample rate
    juce::Thread::sleep(100);
    int captureSR = capture.getSampleRate();
    if (captureSR <= 0) captureSR = 48000;

    // Start output
    if (!output.start(outputDeviceId, *outputBuffer))
    {
        DBG("Failed to start WASAPI multichannel output");
        capture.stop();
        return false;
    }

    int outputSR = output.getSampleRate();
    if (outputSR <= 0) outputSR = 48000;

    // Prepare processing components
    router.prepare(static_cast<double>(outputSR));
    for (auto& cp : channelProcessors)
        cp.prepare(static_cast<double>(outputSR), processBlockSize);

    // Start processing thread
    shouldStop.store(false);
    running.store(true);

    processThread = std::thread([this]() { processThreadFunc(); });

    return true;
}

void AudioEngine::stop()
{
    shouldStop.store(true);

    if (processThread.joinable())
        processThread.join();

    capture.stop();
    output.stop();

    captureBuffer.reset();
    outputBuffer.reset();

    running.store(false);

    for (auto& p : peakLevels) p.store(0.0f);
    for (auto& p : inputPeakLevels) p.store(0.0f);
}

void AudioEngine::setMasterVolume(float db)
{
    masterVolumeDb.store(db, std::memory_order_relaxed);
}

int AudioEngine::getInputSampleRate() const
{
    return capture.getSampleRate();
}

int AudioEngine::getOutputSampleRate() const
{
    return output.getSampleRate();
}

void AudioEngine::processThreadFunc()
{
    // Processing buffers
    std::vector<float> stereoBlock(processBlockSize * 2);
    std::vector<float> routedBlock(processBlockSize * 6);
    std::vector<float> processedBlock(processBlockSize * 6);
    std::vector<float> inputMono(processBlockSize);

    while (!shouldStop.load())
    {
        // Update master gain
        masterGain = juce::Decibels::decibelsToGain(masterVolumeDb.load(std::memory_order_relaxed), -60.0f);

        int available = captureBuffer->getAvailableFrames();

        if (available >= processBlockSize)
        {
            int framesRead = captureBuffer->read(stereoBlock.data(), processBlockSize);

            if (framesRead > 0)
            {
                // Calculate input peak levels and build mono sum for spectrum
                float peakL = 0.0f, peakR = 0.0f;
                for (int i = 0; i < framesRead; ++i)
                {
                    float l = stereoBlock[i * 2];
                    float r = stereoBlock[i * 2 + 1];
                    float absL = std::abs(l);
                    float absR = std::abs(r);
                    if (absL > peakL) peakL = absL;
                    if (absR > peakR) peakR = absR;
                    inputMono[i] = 0.5f * (l + r);
                }
                inputPeakLevels[0].store(peakL, std::memory_order_relaxed);
                inputPeakLevels[1].store(peakR, std::memory_order_relaxed);
                inputSumFifo.pushBlock(inputMono.data(), framesRead);

                // Route stereo to 5.1
                router.processBlock(stereoBlock.data(), routedBlock.data(), framesRead);

                // Process each channel through its ChannelProcessor
                for (int ch = 0; ch < 6; ++ch)
                {
                    float peak = 0.0f;

                    for (int i = 0; i < framesRead; ++i)
                    {
                        float sample = routedBlock[i * 6 + ch];
                        sample = channelProcessors[ch].processSample(sample);
                        sample *= masterGain;

                        processedBlock[i * 6 + ch] = sample;

                        float absSample = std::abs(sample);
                        if (absSample > peak)
                            peak = absSample;
                    }

                    peakLevels[ch].store(peak, std::memory_order_relaxed);
                }

                // Apply channel swap (after all processing including mute)
                bool sf = router.getSwapFront();
                bool sr = router.getSwapRear();
                if (sf || sr)
                {
                    for (int i = 0; i < framesRead; ++i)
                    {
                        if (sf)
                            std::swap(processedBlock[i * 6 + FrontLeft],
                                      processedBlock[i * 6 + FrontRight]);
                        if (sr)
                            std::swap(processedBlock[i * 6 + RearLeft],
                                      processedBlock[i * 6 + RearRight]);
                    }
                }

                // Write to output ring buffer
                outputBuffer->write(processedBlock.data(), framesRead);
            }
        }
        else
        {
            // Not enough data, sleep briefly
            Sleep(2);
        }
    }
}
