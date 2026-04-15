#include "SpectrumAnalyzer.h"
#include "CrossmixerLookAndFeel.h"

SpectrumAnalyzer::SpectrumAnalyzer()
{
    setOpaque(false);
}

SpectrumAnalyzer::~SpectrumAnalyzer()
{
    stopTimer();
}

void SpectrumAnalyzer::setSource(std::function<void(float*, int)> src, double sr)
{
    source = std::move(src);
    sampleRate = sr > 0.0 ? sr : 48000.0;
}

void SpectrumAnalyzer::setActive(bool shouldBeActive)
{
    if (shouldBeActive == active)
        return;

    active = shouldBeActive;

    if (active)
    {
        startTimerHz(30);
    }
    else
    {
        stopTimer();
        for (auto& b : barLevels) b = 0.0f;
        repaint();
    }
}

void SpectrumAnalyzer::timerCallback()
{
    if (!source)
        return;

    // Clear scratch (FFT needs 2x size)
    std::fill(std::begin(fftData), std::end(fftData), 0.0f);

    source(fftData, fftSize);

    window.multiplyWithWindowingTable(fftData, (size_t) fftSize);
    fft.performFrequencyOnlyForwardTransform(fftData);

    const float binHz = (float) (sampleRate / (double) fftSize);
    const float fMin = 40.0f;
    const float fMax = juce::jmin(16000.0f, (float) (sampleRate * 0.45));
    const float logRange = std::log(fMax / fMin);

    const float minDb = -85.0f;
    const float maxDb = 0.0f;

    for (int b = 0; b < numBars; ++b)
    {
        float f0 = fMin * std::exp(logRange * ((float) b / (float) numBars));
        float f1 = fMin * std::exp(logRange * ((float) (b + 1) / (float) numBars));

        int bin0 = juce::jmax(1, (int) std::floor(f0 / binHz));
        int bin1 = juce::jmin(fftSize / 2, juce::jmax(bin0 + 1, (int) std::ceil(f1 / binHz)));

        float maxMag = 0.0f;
        for (int k = bin0; k < bin1; ++k)
            maxMag = juce::jmax(maxMag, fftData[k]);

        // Normalize FFT magnitude
        maxMag /= (float) (fftSize / 2);

        float db = juce::Decibels::gainToDecibels(maxMag, minDb);
        float norm = juce::jlimit(0.0f, 1.0f, (db - minDb) / (maxDb - minDb));

        // Ballistics matching PeakMeter: fast attack, exponential decay
        if (norm > barLevels[b])
            barLevels[b] = norm;
        else
            barLevels[b] *= 0.92f;
    }

    repaint();
}

void SpectrumAnalyzer::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);

    g.setColour(CrossmixerLookAndFeel::faderTrackColour);
    g.fillRoundedRectangle(bounds, 2.0f);

    const float w = bounds.getWidth();
    const float h = bounds.getHeight();
    const float gap = 1.0f;
    const float barW = juce::jmax(1.0f, (w - gap * (numBars + 1)) / (float) numBars);

    for (int b = 0; b < numBars; ++b)
    {
        float level = barLevels[b];
        if (level <= 0.001f)
            continue;

        float barH = level * h;
        float x = bounds.getX() + gap + b * (barW + gap);
        float y = bounds.getBottom() - barH;

        juce::ColourGradient grad(
            CrossmixerLookAndFeel::meterGreenColour, 0.0f, bounds.getBottom(),
            CrossmixerLookAndFeel::meterRedColour,   0.0f, bounds.getY(),
            false);
        grad.addColour(0.6, CrossmixerLookAndFeel::meterYellowColour);
        g.setGradientFill(grad);
        g.fillRect(x, y, barW, barH);
    }
}
